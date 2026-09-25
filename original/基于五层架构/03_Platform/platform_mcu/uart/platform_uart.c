/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_uart.c
 * @brief Platform UART 抽象接口实现
 * @author YaoQian Wang
 * @date 2026-08-28
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_uart.h"
//******************************** Includes *********************************//

//******************************** Functions *********************************//
/**
 * @brief 校验 UART 配置的公共取值范围
 * @param[in] config : UART 配置
 * @param[out] 无
 * @return platform_error_t : 配置校验结果
 */
static platform_error_t platform_uart_validate_config(
    const platform_uart_config_t *config)
{
    /**
     * 按公共契约逐项校验，不假设枚举值必然连续。
     **/
    if ((config == NULL) || (config->baudRate == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((config->dataBits != PLATFORM_UART_DATA_BITS_7) &&
        (config->dataBits != PLATFORM_UART_DATA_BITS_8) &&
        (config->dataBits != PLATFORM_UART_DATA_BITS_9)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (((int32_t)config->stopBits < (int32_t)PLATFORM_UART_STOP_BITS_1) ||
        (config->stopBits >= PLATFORM_UART_STOP_BITS_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (((int32_t)config->parity < (int32_t)PLATFORM_UART_PARITY_NONE) ||
        (config->parity >= PLATFORM_UART_PARITY_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (((int32_t)config->flowControl < (int32_t)PLATFORM_UART_FLOW_CONTROL_NONE) ||
        (config->flowControl >= PLATFORM_UART_FLOW_CONTROL_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (config->defaultTimeoutMs == PLATFORM_UART_TIMEOUT_USE_DEFAULT) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

/**
 * @brief 校验 UART 对象是否允许执行数据操作
 * @param[in] uart : UART 对象
 * @param[out] 无
 * @return platform_error_t : 对象和状态校验结果
 */
static platform_error_t platform_uart_validate_ready(const platform_uart_t *uart)
{
    /**
     * 先区分空指针、未构造对象和生命周期状态错误。
     **/
    if (uart == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((PLATFORM_TRUE !=
         platform_object_is_valid(&uart->device.object, PLATFORM_OBJECT_DEVICE)) ||
        (uart->device.dev_class != PLATFORM_DEVICE_CLASS_UART)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (uart->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return PLATFORM_ERR_OK;
}

/**
 * @brief 校验 UART 对象是否已构造
 * @param[in] uart : UART 对象
 * @param[out] 无
 * @return platform_error_t : 对象校验结果
 */
static platform_error_t platform_uart_validate_constructed(
    const platform_uart_t *uart)
{
    /**
     * 回调绑定允许发生在多个非运行生命周期状态，不复用仅允许 STARTED 的数据操作校验。
     **/
    if (uart == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((PLATFORM_TRUE !=
         platform_object_is_valid(&uart->device.object, PLATFORM_OBJECT_DEVICE)) ||
        (uart->device.dev_class != PLATFORM_DEVICE_CLASS_UART)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

/**
 * @brief 将默认超时标记转换为对象配置值
 * @param[in] uart      : UART 对象
 * @param[in] timeoutMs : 调用者指定的超时值
 * @param[out] 无
 * @return 传递给 Impl 的实际超时值
 */
static uint32_t platform_uart_resolve_timeout(const platform_uart_t *uart,
                                              uint32_t timeoutMs)
{
    /**
     * 仅对显式默认标记做替换，0 和永久等待值原样传递。
     **/
    if (timeoutMs == PLATFORM_UART_TIMEOUT_USE_DEFAULT) {
        return uart->config.defaultTimeoutMs;
    }

    return timeoutMs;
}

/**
 * @brief 校验 UART 异步事件的字段组合
 * @param[in] event : UART 异步事件
 * @param[out] 无
 * @return platform_error_t : 事件校验结果
 */
static platform_error_t platform_uart_validate_event(
    const platform_uart_event_t *event)
{
    /**
     * 先校验公共枚举范围，再校验每类事件的专用约束。
     **/
    if ((event == NULL) ||
        ((int32_t)event->type < (int32_t)PLATFORM_UART_EVENT_TX_COMPLETE) ||
        (event->type >= PLATFORM_UART_EVENT_MAX) ||
        ((int32_t)event->direction < (int32_t)PLATFORM_UART_DIRECTION_TX) ||
        (event->direction >= PLATFORM_UART_DIRECTION_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    switch (event->type) {
        case PLATFORM_UART_EVENT_TX_COMPLETE:
            if ((event->direction != PLATFORM_UART_DIRECTION_TX) ||
                (event->data == NULL) || (event->dataLength == 0U) ||
                (event->error != PLATFORM_ERR_OK)) {
                return PLATFORM_ERR_INVALID_PARAM;
            }
            break;

        case PLATFORM_UART_EVENT_RX_DATA:
            if ((event->direction != PLATFORM_UART_DIRECTION_RX) ||
                (event->data == NULL) || (event->dataLength == 0U) ||
                (event->error != PLATFORM_ERR_OK)) {
                return PLATFORM_ERR_INVALID_PARAM;
            }
            break;

        case PLATFORM_UART_EVENT_ERROR:
            if (event->error == PLATFORM_ERR_OK) {
                return PLATFORM_ERR_INVALID_PARAM;
            }
            break;

        case PLATFORM_UART_EVENT_CANCELED:
            if (event->error != PLATFORM_ERR_CANCELED) {
                return PLATFORM_ERR_INVALID_PARAM;
            }
            break;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

/**
 * @brief 构造 Platform UART 对象
 * @param[out] uart  : 待构造的 UART 对象
 * @param[in] params : UART 初始化参数
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_init(platform_uart_t *uart,
                                    const platform_uart_init_params_t *params)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /**
     * 对象基类和 Ops 是 UART 对象可用的必要条件。
     **/
    if ((uart == NULL) || (params == NULL) || (params->name == NULL) ||
        (params->lifecycle == NULL) || (params->ops == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (PLATFORM_TRUE ==
        platform_object_is_valid(&uart->device.object, PLATFORM_OBJECT_DEVICE)) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    result = platform_uart_validate_config(&params->config);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_device_init(&uart->device,
                                  params->name,
                                  PLATFORM_DEVICE_CLASS_UART,
                                  params->caps,
                                  params->lifecycle);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /**
     * 复制配置以解除 UART 对象对外部配置存储期的依赖。
     **/
    uart->config = params->config;
    uart->ops = params->ops;
    uart->implContext = params->implContext;
    uart->callback = params->callback;
    uart->callbackContext = params->callbackContext;

    return PLATFORM_ERR_OK;
}

/**
 * @brief 绑定或解绑 Platform UART 异步事件回调
 * @param[in] uart            : 已构造且未处于 STARTED 状态的 UART 对象
 * @param[in] callback        : 事件回调，NULL 表示解绑
 * @param[in] callbackContext : 回调上下文
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_set_callback(platform_uart_t *uart,
                                            platform_uart_callback_t callback,
                                            void *callbackContext)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /**
     * STARTED 状态可能有异步事件源，禁止替换或清除当前绑定。
     **/
    result = platform_uart_validate_constructed(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->device.object.state == PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    uart->callback = callback;
    uart->callbackContext = (callback == NULL) ? NULL : callbackContext;

    return PLATFORM_ERR_OK;
}

/**
 * @brief 校验并转发一次阻塞 UART 发送
 * @param[in] uart            : UART 对象
 * @param[in] data            : 发送缓冲区
 * @param[in] dataLength      : 发送长度
 * @param[in] timeoutMs       : 超时时间，单位毫秒
 * @param[out] writtenLength  : 实际完成长度
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_write(platform_uart_t *uart,
                                     const uint8_t *data,
                                     platform_size_t dataLength,
                                     uint32_t timeoutMs,
                                     platform_size_t *writtenLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_size_t completedLength = 0U;

    /**
     * 所有退出路径都使可用的完成长度保持明确的零值。
     **/
    if (writtenLength != NULL) {
        *writtenLength = 0U;
    }

    if ((data == NULL) || (dataLength == 0U) || (writtenLength == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_uart_validate_ready(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->ops->write == NULL) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    result = uart->ops->write(uart,
                              data,
                              dataLength,
                              platform_uart_resolve_timeout(uart, timeoutMs),
                              &completedLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (completedLength > dataLength) {
        return PLATFORM_ERR_OVERFLOW;
    }

    *writtenLength = completedLength;
    return PLATFORM_ERR_OK;
}

/**
 * @brief 校验并转发一次阻塞 UART 接收
 * @param[in] uart         : UART 对象
 * @param[out] buffer      : 接收缓冲区
 * @param[in] bufferSize   : 接收缓冲区容量
 * @param[in] timeoutMs    : 超时时间，单位毫秒
 * @param[out] readLength  : 实际完成长度
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_read(platform_uart_t *uart,
                                    uint8_t *buffer,
                                    platform_size_t bufferSize,
                                    uint32_t timeoutMs,
                                    platform_size_t *readLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_size_t completedLength = 0U;

    /**
     * 所有退出路径都使可用的完成长度保持明确的零值。
     **/
    if (readLength != NULL) {
        *readLength = 0U;
    }

    if ((buffer == NULL) || (bufferSize == 0U) || (readLength == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_uart_validate_ready(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->ops->read == NULL) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    result = uart->ops->read(uart,
                             buffer,
                             bufferSize,
                             platform_uart_resolve_timeout(uart, timeoutMs),
                             &completedLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (completedLength > bufferSize) {
        return PLATFORM_ERR_OVERFLOW;
    }

    *readLength = completedLength;
    return PLATFORM_ERR_OK;
}

/**
 * @brief 校验并转发一次异步 UART 发送
 * @param[in] uart       : UART 对象
 * @param[in] data       : 发送缓冲区
 * @param[in] dataLength : 发送长度
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_write_async(platform_uart_t *uart,
                                           const uint8_t *data,
                                           platform_size_t dataLength)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /**
     * 异步操作必须有结束事件接收者，否则 Buffer 所有权无法安全归还。
     **/
    if ((data == NULL) || (dataLength == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_uart_validate_ready(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->callback == NULL) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (uart->ops->writeAsync == NULL) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return uart->ops->writeAsync(uart, data, dataLength);
}

/**
 * @brief 校验并转发一次异步 UART 接收
 * @param[in] uart       : UART 对象
 * @param[out] buffer    : 接收缓冲区
 * @param[in] bufferSize : 接收缓冲区容量
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_read_async(platform_uart_t *uart,
                                          uint8_t *buffer,
                                          platform_size_t bufferSize)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /**
     * 异步接收不在 Platform 中创建或复制 Buffer。
     **/
    if ((buffer == NULL) || (bufferSize == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_uart_validate_ready(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->callback == NULL) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (uart->ops->readAsync == NULL) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return uart->ops->readAsync(uart, buffer, bufferSize);
}

/**
 * @brief 校验并转发一次 UART 异步传输取消
 * @param[in] uart      : UART 对象
 * @param[in] direction : 待取消的传输方向
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_cancel(platform_uart_t *uart,
                                      platform_uart_direction_t direction)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /**
     * BOTH 用于停止前同时取消 TX 和 RX，其他越界值全部拒绝。
     **/
    if (((int32_t)direction < (int32_t)PLATFORM_UART_DIRECTION_TX) ||
        (direction >= PLATFORM_UART_DIRECTION_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_uart_validate_ready(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->ops->cancel == NULL) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return uart->ops->cancel(uart, direction);
}

/**
 * @brief 校验并通知一次 UART 异步事件
 * @param[in] uart  : UART 对象
 * @param[in] event : UART 异步事件
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_notify_event(platform_uart_t *uart,
                                             const platform_uart_event_t *event)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (event == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    /**
     * Impl 必须在退出 STARTED 前关闭事件源并排空尾部事件，
     * 防止停止或反初始化后访问已失效的回调上下文。
     **/
    result = platform_uart_validate_ready(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->callback == NULL) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = platform_uart_validate_event(event);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /**
     * 校验通过后仅通知一次，回调上下文原样透传。
     **/
    uart->callback(uart, event, uart->callbackContext);

    return PLATFORM_ERR_OK;
}
//******************************** Functions *********************************//
