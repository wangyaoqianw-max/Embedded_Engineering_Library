/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file service_uart.c
 * @brief UART 接收 Service 生命周期实现
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "service_uart.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
/* Platform Thread Flag 仅作为 Consumer 重新检查 Service 真值的私有唤醒提示。 */
#define SERVICE_UART_NOTIFY_WAKE_FLAG    (1U << 0)
//******************************** Defines **********************************//

//******************************** Functions *********************************//
/**
 * @brief 根据 RingBuffer 和运行上下文重建 Consumer 可观察的 Service 事件
 * @param[in] service : 已初始化且处于允许等待状态的 Service 对象
 * @param[out] events : 重建后的 Service 事件位
 * @return platform_error_t : RingBuffer 查询结果
 */
static platform_error_t service_uart_rebuild_events(const service_uart_t *service,
                                                    uint32_t *events)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_size_t readableSize = 0U;

    result = ring_buffer_get_readable_size(&service->context.rxRingBuffer,
                                           &readableSize);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *events = 0U;
    if (readableSize != 0U) {
        *events |= SERVICE_UART_EVENT_RX_AVAILABLE;
    }

    if (service->context.dataLossOccurred == PLATFORM_TRUE) {
        *events |= SERVICE_UART_EVENT_DATA_LOSS;
    }

    if (service->context.state == SERVICE_UART_STATE_ERROR) {
        *events |= SERVICE_UART_EVENT_ERROR;
    }

    if (service->context.state == SERVICE_UART_STATE_STOPPED) {
        *events |= SERVICE_UART_EVENT_STOPPED;
    }

    return PLATFORM_ERR_OK;
}

/*
 * 统一完成成功、取消和错误发送路径；可由 UART 回调触发，因此仅写入
 * TX 事务字段后用 ISR 安全的通知唤醒等待方。
 */
static void service_uart_finish_tx(service_uart_t *service,
                                   platform_error_t result,
                                   platform_size_t completedLength)
{
    if ((service == NULL) || (service->context.txState != SERVICE_UART_TX_STATE_ACTIVE)) {
        return;
    }

    service->context.txState = SERVICE_UART_TX_STATE_IDLE;
    service->context.txResult = result;
    if (result == PLATFORM_ERR_OK) {
        service->statistics.txCompleteCount++;
        service->statistics.txBytesCompleted += completedLength;
    } else if (result == PLATFORM_ERR_CANCELED) {
        service->statistics.txCancelCount++;
    } else {
        service->statistics.txErrorCount++;
    }

    (void)platform_notify_set_from_isr(service->config.ownerThread,
                                       SERVICE_UART_NOTIFY_WAKE_FLAG);
}

/* 仅取消活动 TX；取消成功后保证等待方能够观察到 CANCELED 结果。 */
static platform_error_t service_uart_cancel_active_tx(service_uart_t *service)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (service->context.txState != SERVICE_UART_TX_STATE_ACTIVE) {
        return PLATFORM_ERR_OK;
    }

    result = platform_uart_cancel(service->config.uart, PLATFORM_UART_DIRECTION_TX);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /* Platform cancel 成功后 DMA 已停止；此分支仅覆盖底层未同步回调的异常实现。 */
    service_uart_finish_tx(service, PLATFORM_ERR_CANCELED, 0U);

    return PLATFORM_ERR_OK;
}

/* 按剩余超时等待 TX 完成通知，并以 TX 状态而非通知位作为最终真值。 */
static platform_error_t service_uart_wait_tx_complete(service_uart_t *service,
                                                       uint32_t timeoutMs)
{
    platform_error_t result = PLATFORM_ERR_OK;
    uint32_t startTimeMs = 0U;
    uint32_t currentTimeMs = 0U;
    uint32_t elapsedMs = 0U;
    uint32_t remainingTimeoutMs = 0U;
    uint32_t previousFlags = 0U;
    uint32_t receivedFlags = 0U;

    if (service->context.txState != SERVICE_UART_TX_STATE_ACTIVE) {
        return service->context.txResult;
    }

    if (timeoutMs != PLATFORM_OS_WAIT_FOREVER) {
        result = platform_time_get_ms(&startTimeMs);
        if (result != PLATFORM_ERR_OK) {
            (void)service_uart_cancel_active_tx(service);
            return result;
        }
    }

    for (;;) {
        if (service->context.txState != SERVICE_UART_TX_STATE_ACTIVE) {
            return service->context.txResult;
        }

        if (timeoutMs == PLATFORM_OS_WAIT_FOREVER) {
            remainingTimeoutMs = PLATFORM_OS_WAIT_FOREVER;
        } else {
            result = platform_time_get_ms(&currentTimeMs);
            if (result != PLATFORM_ERR_OK) {
                (void)service_uart_cancel_active_tx(service);
                return result;
            }

            elapsedMs = currentTimeMs - startTimeMs;
            if (elapsedMs >= timeoutMs) {
                result = service_uart_cancel_active_tx(service);
                if (result != PLATFORM_ERR_OK) {
                    return result;
                }

                service->statistics.txTimeoutCount++;
                return PLATFORM_ERR_TIMEOUT;
            }

            remainingTimeoutMs = timeoutMs - elapsedMs;
        }

        result = platform_notify_clear(SERVICE_UART_NOTIFY_WAKE_FLAG, &previousFlags);
        if (result != PLATFORM_ERR_OK) {
            (void)service_uart_cancel_active_tx(service);
            return result;
        }

        if (service->context.txState != SERVICE_UART_TX_STATE_ACTIVE) {
            return service->context.txResult;
        }

        result = platform_notify_wait(SERVICE_UART_NOTIFY_WAKE_FLAG,
                                      PLATFORM_FALSE,
                                      PLATFORM_TRUE,
                                      remainingTimeoutMs,
                                      &receivedFlags);
        if ((result != PLATFORM_ERR_OK) && (result != PLATFORM_ERR_TIMEOUT)) {
            (void)service_uart_cancel_active_tx(service);
            return result;
        }
    }
}

/**
 * @brief 接收 Platform UART 异步事件
 * @param[in] uart            : 产生事件的 UART 对象
 * @param[in] event           : 异步事件
 * @param[in] callbackContext : Service 对象
 * @return 无
 */
static void service_uart_handle_platform_event(
    platform_uart_t *uart,
    const platform_uart_event_t *event,
    void *callbackContext)
{
    service_uart_t *service = (service_uart_t *)callbackContext;
    platform_error_t result = PLATFORM_ERR_OK;
    platform_size_t writtenLength = 0U;
    platform_size_t readableSize = 0U;

    /**
     * CANCELED 可能来自 stop 的同步 Task Context，也可能是组合合同被破坏后的非预期事件；
     * 两种情况都不能在 callback 内调用 Notify，因为 callback 的执行上下文不可假设。
     **/
    if ((service == NULL) || (uart == NULL) || (event == NULL) ||
        (uart != service->config.uart)) {
        return;
    }

    if (event->direction == PLATFORM_UART_DIRECTION_TX) {
        if (event->type == PLATFORM_UART_EVENT_TX_COMPLETE) {
            service_uart_finish_tx(service, PLATFORM_ERR_OK, event->dataLength);
        } else if (event->type == PLATFORM_UART_EVENT_ERROR) {
            service_uart_finish_tx(service, event->error, 0U);
        } else if (event->type == PLATFORM_UART_EVENT_CANCELED) {
            service_uart_finish_tx(service, PLATFORM_ERR_CANCELED, 0U);
        }

        return;
    }

    if (event->direction == PLATFORM_UART_DIRECTION_BOTH) {
        if ((event->type != PLATFORM_UART_EVENT_ERROR) ||
            (service->context.state != SERVICE_UART_STATE_RUNNING)) {
            return;
        }

        service->context.lastError = event->error;
        service->statistics.uartErrorCount++;
        service->context.state = SERVICE_UART_STATE_ERROR;
        if (service->context.txState == SERVICE_UART_TX_STATE_ACTIVE) {
            service_uart_finish_tx(service, event->error, 0U);
        } else {
            (void)platform_notify_set_from_isr(service->config.ownerThread,
                                               SERVICE_UART_NOTIFY_WAKE_FLAG);
        }

        return;
    }

    if (event->direction != PLATFORM_UART_DIRECTION_RX) {
        return;
    }

    if (event->type == PLATFORM_UART_EVENT_CANCELED) {
        service->statistics.cancelCount++;
        service->context.state = SERVICE_UART_STATE_STOPPED;
        return;
    }

    if (event->type == PLATFORM_UART_EVENT_ERROR) {
        if (service->context.state != SERVICE_UART_STATE_RUNNING) {
            return;
        }

        service->context.lastError = event->error;
        service->statistics.uartErrorCount++;
        service->context.state = SERVICE_UART_STATE_ERROR;

        result = platform_notify_set_from_isr(service->config.ownerThread,
                                              SERVICE_UART_NOTIFY_WAKE_FLAG);
        if (result != PLATFORM_ERR_OK) {
            return;
        }

        return;
    }

    if ((event->type != PLATFORM_UART_EVENT_RX_DATA) ||
        (service->context.state != SERVICE_UART_STATE_RUNNING)) {
        return;
    }

    service->statistics.rxEventCount++;
    service->statistics.rxBytesReceived += event->dataLength;
    result = ring_buffer_write(&service->context.rxRingBuffer,
                               event->data,
                               event->dataLength,
                               &writtenLength);
    if ((result != PLATFORM_ERR_OK) && (result != PLATFORM_ERR_OVERFLOW)) {
        return;
    }

    service->statistics.rxBytesBuffered += writtenLength;
    if (event->dataLength != writtenLength) {
        service->statistics.rxBytesDropped += event->dataLength - writtenLength;
        service->statistics.ringBufferOverflowCount++;
        service->context.dataLossOccurred = PLATFORM_TRUE;
    }

    result = ring_buffer_get_readable_size(&service->context.rxRingBuffer,
                                           &readableSize);
    if (result != PLATFORM_ERR_OK) {
        return;
    }

    if (readableSize > service->statistics.ringBufferHighWaterMark) {
        service->statistics.ringBufferHighWaterMark = readableSize;
    }

    result = platform_notify_set_from_isr(service->config.ownerThread,
                                          SERVICE_UART_NOTIFY_WAKE_FLAG);
    if (result != PLATFORM_ERR_OK) {
        /**
         * ISR 中没有安全的同步恢复路径；状态和 RingBuffer 仍是真值，Consumer 可由后续唤醒发现。
         **/
        return;
    }
}

/**
 * @brief 校验 Service 初始化配置
 * @param[in] service : Service 对象
 * @param[in] config  : Service 配置
 * @param[out] 无
 * @return platform_error_t : 参数校验结果
 */
static platform_error_t service_uart_validate_init_config(
    const service_uart_t *service,
    const service_uart_config_t *config)
{
    /**
     * 外部 UART、线程和两块调用者持有的存储都属于 Service 的必要依赖。
     **/
    if ((service == NULL) || (config == NULL) || (config->uart == NULL) ||
        (config->dmaRxBuffer == NULL) || (config->dmaRxBufferSize == 0U) ||
        (config->ringBufferStorage == NULL) ||
        (config->ringBufferStorageSize < 2U) ||
        (config->ownerThread == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

/**
 * @brief 校验 Service 对象已完成初始化
 * @param[in] service : Service 对象
 * @param[out] 无
 * @return platform_error_t : 对象状态校验结果
 */
static platform_error_t service_uart_validate_initialized(
    const service_uart_t *service)
{
    if (service == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (service->context.state == SERVICE_UART_STATE_UNINITIALIZED) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

/**
 * @brief 初始化 UART Service 对象并绑定 Platform UART 回调
 * @param[in,out] service : 使用 SERVICE_UART_INITIALIZER 初始化的 Service 对象
 * @param[in] config      : Service 配置
 * @param[out] 无
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_init(service_uart_t *service,
                                   const service_uart_config_t *config)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /**
     * 只有全部内部状态完成且回调绑定成功后，才对外发布 INITIALIZED。
     **/
    result = service_uart_validate_init_config(service, config);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (service->context.state != SERVICE_UART_STATE_UNINITIALIZED) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    result = ring_buffer_init(&service->context.rxRingBuffer,
                              config->ringBufferStorage,
                              config->ringBufferStorageSize);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    service->config = *config;
    service->context.lastError = PLATFORM_ERR_OK;
    service->context.dataLossOccurred = PLATFORM_FALSE;
    service->context.txState = SERVICE_UART_TX_STATE_IDLE;
    service->context.txResult = PLATFORM_ERR_OK;
    service->statistics = (service_uart_statistics_t){0};

    result = platform_uart_set_callback(service->config.uart,
                                        service_uart_handle_platform_event,
                                        service);
    if (result != PLATFORM_ERR_OK) {
        *service = (service_uart_t)SERVICE_UART_INITIALIZER;
        return result;
    }

    service->context.state = SERVICE_UART_STATE_INITIALIZED;

    return PLATFORM_ERR_OK;
}

/**
 * @brief 开启新的 UART RX Session
 * @param[in,out] service : 已初始化的 UART Service 对象
 * @param[out] 无
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_start(service_uart_t *service)
{
    platform_error_t result = PLATFORM_ERR_OK;
    service_uart_state_t previousState = SERVICE_UART_STATE_UNINITIALIZED;
    platform_error_t previousLastError = PLATFORM_ERR_OK;
    platform_bool_t previousDataLossOccurred = PLATFORM_FALSE;

    /**
     * RUNNING 必须在 read_async 前发布，避免 Impl 立即通知的首个 RX callback 被错误丢弃。
     **/
    if (service == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (service->context.state == SERVICE_UART_STATE_UNINITIALIZED) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if ((service->context.state != SERVICE_UART_STATE_INITIALIZED) &&
        (service->context.state != SERVICE_UART_STATE_STOPPED) &&
        (service->context.state != SERVICE_UART_STATE_ERROR)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    previousState = service->context.state;
    previousLastError = service->context.lastError;
    previousDataLossOccurred = service->context.dataLossOccurred;

    result = ring_buffer_reset(&service->context.rxRingBuffer);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    service->context.dataLossOccurred = PLATFORM_FALSE;
    service->context.lastError = PLATFORM_ERR_OK;
    service->context.state = SERVICE_UART_STATE_RUNNING;

    result = platform_uart_read_async(service->config.uart,
                                      service->config.dmaRxBuffer,
                                      service->config.dmaRxBufferSize);
    if (result != PLATFORM_ERR_OK) {
        service->context.state = previousState;
        service->context.lastError = previousLastError;
        service->context.dataLossOccurred = previousDataLossOccurred;
        return result;
    }

    return PLATFORM_ERR_OK;
}

/**
 * @brief 取消活动 RX Session 并唤醒 Consumer 重新检查 Service 状态
 * @param[in,out] service : 正在运行的 UART Service 对象
 * @param[out] 无
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_stop(service_uart_t *service)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /**
     * 当前 STM32 Impl 保证 cancel 在返回前同步触发 CANCELED，因此 STOPPING 由 callback 转为 STOPPED。
     **/
    if (service == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (service->context.state == SERVICE_UART_STATE_UNINITIALIZED) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (service->context.state != SERVICE_UART_STATE_RUNNING) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    service->context.state = SERVICE_UART_STATE_STOPPING;
    result = service_uart_cancel_active_tx(service);
    if (result != PLATFORM_ERR_OK) {
        service->context.state = SERVICE_UART_STATE_RUNNING;
        return result;
    }

    result = platform_uart_cancel(service->config.uart, PLATFORM_UART_DIRECTION_RX);
    if (result != PLATFORM_ERR_OK) {
        service->context.state = SERVICE_UART_STATE_RUNNING;
        return result;
    }

    result = platform_notify_set(service->config.ownerThread,
                                 SERVICE_UART_NOTIFY_WAKE_FLAG);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t service_uart_write(service_uart_t *service,
                                    const uint8_t *data,
                                    platform_size_t dataLength,
                                    uint32_t timeoutMs)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if ((service == NULL) || (data == NULL) || (dataLength == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (service->context.state == SERVICE_UART_STATE_UNINITIALIZED) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (service->context.state != SERVICE_UART_STATE_RUNNING) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (service->context.txState != SERVICE_UART_TX_STATE_IDLE) {
        service->statistics.txBusyRejectCount++;
        return PLATFORM_ERR_BUSY;
    }

    service->context.txState = SERVICE_UART_TX_STATE_ACTIVE;
    service->context.txResult = PLATFORM_ERR_OK;
    service->statistics.txRequestCount++;
    result = platform_uart_write_async(service->config.uart, data, dataLength);
    if (result != PLATFORM_ERR_OK) {
        service_uart_finish_tx(service, result, 0U);
        return result;
    }

    return service_uart_wait_tx_complete(service, timeoutMs);
}

/**
 * @brief 解绑 Platform UART 回调并清理 Service 对象
 * @param[in,out] service : 已初始化且无活动 RX Session 的 Service 对象
 * @param[out] 无
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_deinit(service_uart_t *service)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /**
     * 运行中的 RX Session 必须先由 stop 完结，解绑失败时保留完整 Service 状态供调用者处理。
     **/
    if (service == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (service->context.state == SERVICE_UART_STATE_UNINITIALIZED) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if ((service->context.state != SERVICE_UART_STATE_INITIALIZED) &&
        (service->context.state != SERVICE_UART_STATE_STOPPED) &&
        (service->context.state != SERVICE_UART_STATE_ERROR)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (service->context.txState != SERVICE_UART_TX_STATE_IDLE) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = platform_uart_set_callback(service->config.uart, NULL, NULL);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *service = (service_uart_t)SERVICE_UART_INITIALIZER;

    return PLATFORM_ERR_OK;
}

/**
 * @brief 非阻塞读取已缓存的 RX 数据
 * @param[in,out] service : 已初始化的 Service 对象
 * @param[out] buffer      : 输出缓冲区
 * @param[in] bufferSize   : 输出缓冲区容量
 * @param[out] readLength  : 实际读取长度
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_read(service_uart_t *service,
                                   uint8_t *buffer,
                                   platform_size_t bufferSize,
                                   platform_size_t *readLength)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (readLength == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = service_uart_validate_initialized(service);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((service->context.state != SERVICE_UART_STATE_RUNNING) &&
        (service->context.state != SERVICE_UART_STATE_STOPPED) &&
        (service->context.state != SERVICE_UART_STATE_ERROR)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = ring_buffer_read(&service->context.rxRingBuffer,
                              buffer,
                              bufferSize,
                              readLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    service->statistics.rxBytesRead += *readLength;

    return PLATFORM_ERR_OK;
}

/**
 * @brief 等待私有唤醒提示并返回由 Service 真值重建的事件
 * @param[in,out] service : 已初始化的 Service 对象
 * @param[in] timeoutMs   : 最大等待时间，单位为毫秒
 * @param[out] events     : 收到的 Service 事件位
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_wait_event(service_uart_t *service,
                                         uint32_t timeoutMs,
                                         uint32_t *events)
{
    platform_error_t result = PLATFORM_ERR_OK;
    uint32_t previousFlags = 0U;
    uint32_t receivedFlags = 0U;

    if (events == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = service_uart_validate_initialized(service);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((service->context.state != SERVICE_UART_STATE_RUNNING) &&
        (service->context.state != SERVICE_UART_STATE_STOPPED) &&
        (service->context.state != SERVICE_UART_STATE_ERROR)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    *events = 0U;
    result = platform_notify_clear(SERVICE_UART_NOTIFY_WAKE_FLAG, &previousFlags);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = service_uart_rebuild_events(service, events);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (*events != 0U) {
        return PLATFORM_ERR_OK;
    }

    result = platform_notify_wait(SERVICE_UART_NOTIFY_WAKE_FLAG,
                                  PLATFORM_FALSE,
                                  PLATFORM_TRUE,
                                  timeoutMs,
                                  &receivedFlags);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = service_uart_rebuild_events(service, events);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (*events == 0U) {
        return PLATFORM_ERR_EMPTY;
    }

    return PLATFORM_ERR_OK;
}

/**
 * @brief 查询当前 RingBuffer 可读字节数
 * @param[in] service          : 已初始化的 Service 对象
 * @param[out] readableSize    : 当前可读字节数
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_get_readable_size(
    const service_uart_t *service,
    platform_size_t *readableSize)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (readableSize == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = service_uart_validate_initialized(service);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return ring_buffer_get_readable_size(&service->context.rxRingBuffer, readableSize);
}

/**
 * @brief 获取 Service 当前运行状态快照
 * @param[in] service : 已初始化的 Service 对象
 * @param[out] status : 输出状态快照
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_get_status(const service_uart_t *service,
                                         service_uart_status_t *status)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (status == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = service_uart_validate_initialized(service);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    status->state = service->context.state;
    status->lastError = service->context.lastError;
    status->dataLossOccurred = service->context.dataLossOccurred;

    return PLATFORM_ERR_OK;
}

/**
 * @brief 获取 Service 统计快照
 * @param[in] service       : 已初始化的 Service 对象
 * @param[out] statistics   : 输出统计快照
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_get_statistics(
    const service_uart_t *service,
    service_uart_statistics_t *statistics)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (statistics == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = service_uart_validate_initialized(service);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    statistics->rxEventCount = service->statistics.rxEventCount;
    statistics->rxBytesReceived = service->statistics.rxBytesReceived;
    statistics->rxBytesBuffered = service->statistics.rxBytesBuffered;
    statistics->rxBytesRead = service->statistics.rxBytesRead;
    statistics->rxBytesDropped = service->statistics.rxBytesDropped;
    statistics->ringBufferOverflowCount = service->statistics.ringBufferOverflowCount;
    statistics->ringBufferHighWaterMark = service->statistics.ringBufferHighWaterMark;
    statistics->uartErrorCount = service->statistics.uartErrorCount;
    statistics->cancelCount = service->statistics.cancelCount;
    statistics->txRequestCount = service->statistics.txRequestCount;
    statistics->txCompleteCount = service->statistics.txCompleteCount;
    statistics->txBytesCompleted = service->statistics.txBytesCompleted;
    statistics->txBusyRejectCount = service->statistics.txBusyRejectCount;
    statistics->txTimeoutCount = service->statistics.txTimeoutCount;
    statistics->txErrorCount = service->statistics.txErrorCount;
    statistics->txCancelCount = service->statistics.txCancelCount;

    return PLATFORM_ERR_OK;
}

/**
 * @brief 清除跨 RX Session 累计统计
 * @param[in,out] service : 已初始化且 Consumer 静止的 Service 对象
 * @param[out] 无
 * @return platform_error_t : 函数执行状态
 */
platform_error_t service_uart_clear_statistics(service_uart_t *service)
{
    platform_error_t result = PLATFORM_ERR_OK;

    result = service_uart_validate_initialized(service);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((service->context.state != SERVICE_UART_STATE_INITIALIZED) &&
        (service->context.state != SERVICE_UART_STATE_STOPPED) &&
        (service->context.state != SERVICE_UART_STATE_ERROR)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    service->statistics.rxEventCount = 0U;
    service->statistics.rxBytesReceived = 0U;
    service->statistics.rxBytesBuffered = 0U;
    service->statistics.rxBytesRead = 0U;
    service->statistics.rxBytesDropped = 0U;
    service->statistics.ringBufferOverflowCount = 0U;
    service->statistics.ringBufferHighWaterMark = 0U;
    service->statistics.uartErrorCount = 0U;
    service->statistics.cancelCount = 0U;
    service->statistics.txRequestCount = 0U;
    service->statistics.txCompleteCount = 0U;
    service->statistics.txBytesCompleted = 0U;
    service->statistics.txBusyRejectCount = 0U;
    service->statistics.txTimeoutCount = 0U;
    service->statistics.txErrorCount = 0U;
    service->statistics.txCancelCount = 0U;

    return PLATFORM_ERR_OK;
}
