/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_uart.c
 * @brief STM32F4 HAL UART Platform 阻塞/DMA 实现
 * @author YaoQian Wang
 * @date 2026-08-29
 * @version V1.1
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "impl_platform_uart.h"

#include "platform_device.h"
#include "platform_object.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define STM32_UART_HAL_MAX_TRANSFER_SIZE ((platform_size_t)0xFFFFU)
#define STM32_UART_HAL_RX_LINE_ERROR_MASK \
    (HAL_UART_ERROR_PE | HAL_UART_ERROR_NE | HAL_UART_ERROR_FE | HAL_UART_ERROR_ORE)
//******************************** Defines *********************************//

//******************************** Defines **********************************//
#define STM32_UART_IMPL_MAX_CONTEXTS   (4U)
//******************************** Defines **********************************//

//******************************** Variables ********************************//
static impl_platform_uart_context_t *g_stm32UartContexts[STM32_UART_IMPL_MAX_CONTEXTS] = {0};
//******************************** Variables ********************************//

//******************************** Private Functions *************************//
static platform_error_t stm32_uart_map_hal_status(HAL_StatusTypeDef halStatus);
static platform_error_t stm32_uart_get_context(platform_uart_t *uart,
                                               impl_platform_uart_context_t **context);
static platform_error_t stm32_uart_validate_object(const platform_uart_t *uart);
static platform_error_t stm32_uart_apply_config(UART_HandleTypeDef *halUart,
                                                const platform_uart_config_t *config);
static platform_error_t stm32_uart_map_word_length(const platform_uart_config_t *config,
                                                   uint32_t *wordLength);
static platform_error_t stm32_uart_map_stop_bits(platform_uart_stop_bits_t stopBits,
                                                 uint32_t *halStopBits);
static platform_error_t stm32_uart_map_parity(platform_uart_parity_t parity,
                                              uint32_t *halParity);
static platform_error_t stm32_uart_lifecycle_init(void *self);
static platform_error_t stm32_uart_lifecycle_start(void *self);
static platform_error_t stm32_uart_lifecycle_process(void *self);
static platform_error_t stm32_uart_lifecycle_stop(void *self);
static platform_error_t stm32_uart_lifecycle_deinit(void *self);
static platform_error_t stm32_uart_write(platform_uart_t *uart,
                                         const uint8_t *data,
                                         platform_size_t dataLength,
                                         uint32_t timeoutMs,
                                         platform_size_t *writtenLength);
static platform_error_t stm32_uart_write_async(platform_uart_t *uart,
                                               const uint8_t *data,
                                               platform_size_t dataLength);
static platform_error_t stm32_uart_read(platform_uart_t *uart,
                                        uint8_t *buffer,
                                        platform_size_t bufferSize,
                                        uint32_t timeoutMs,
                                        platform_size_t *readLength);
static platform_error_t stm32_uart_read_async(platform_uart_t *uart,
                                              uint8_t *buffer,
                                              platform_size_t bufferSize);
static platform_error_t stm32_uart_cancel(platform_uart_t *uart,
                                          platform_uart_direction_t direction);
static void stm32_uart_clear_rx_session(impl_platform_uart_context_t *context);
static void stm32_uart_clear_tx_transaction(impl_platform_uart_context_t *context);
static void stm32_uart_emit_canceled(platform_uart_t *uart,
                                     platform_uart_direction_t direction);
static platform_error_t stm32_uart_cancel_tx(platform_uart_t *uart,
                                             impl_platform_uart_context_t *context);
static platform_error_t stm32_uart_cancel_rx(platform_uart_t *uart,
                                             impl_platform_uart_context_t *context);
static void stm32_uart_emit_rx_data(impl_platform_uart_context_t *context,
                                    platform_size_t startPosition,
                                    platform_size_t dataLength);
static void stm32_uart_process_rx_position(impl_platform_uart_context_t *context,
                                           platform_size_t position);
static platform_error_t stm32_uart_map_hal_error(uint32_t errorCode);
static impl_platform_uart_context_t *stm32_uart_find_context(
    UART_HandleTypeDef *halUart);
static platform_error_t stm32_uart_register_context(
    impl_platform_uart_context_t *context);
//******************************** Private Functions *************************//

//******************************** Constants *********************************//
static const platform_lifecycle_ops_t g_stm32UartLifecycleOps = {
    stm32_uart_lifecycle_init,
    stm32_uart_lifecycle_start,
    stm32_uart_lifecycle_process,
    stm32_uart_lifecycle_stop,
    stm32_uart_lifecycle_deinit
};

static const platform_uart_ops_t g_stm32UartOps = {
    stm32_uart_write,
    stm32_uart_read,
    stm32_uart_write_async,
    stm32_uart_read_async,
    stm32_uart_cancel
};
//******************************** Constants *********************************//

//******************************** Private Functions *************************//
static platform_error_t stm32_uart_map_hal_status(HAL_StatusTypeDef halStatus)
{
    switch (halStatus) {
        case HAL_OK:
            return PLATFORM_ERR_OK;

        case HAL_BUSY:
            return PLATFORM_ERR_BUSY;

        case HAL_TIMEOUT:
            return PLATFORM_ERR_TIMEOUT;

        case HAL_ERROR:
            return PLATFORM_ERR_IO;

        default:
            return PLATFORM_ERR_UNKNOWN;
    }
}

static impl_platform_uart_context_t *stm32_uart_find_context(
    UART_HandleTypeDef *halUart)
{
    uint32_t index;

    if (halUart == NULL) {
        return NULL;
    }

    for (index = 0U; index < STM32_UART_IMPL_MAX_CONTEXTS; index++) {
        if ((g_stm32UartContexts[index] != NULL) &&
            (g_stm32UartContexts[index]->halUart == halUart)) {
            return g_stm32UartContexts[index];
        }
    }

    return NULL;
}

static platform_error_t stm32_uart_register_context(
    impl_platform_uart_context_t *context)
{
    uint32_t index;

    if ((context == NULL) || (context->halUart == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    for (index = 0U; index < STM32_UART_IMPL_MAX_CONTEXTS; index++) {
        if (g_stm32UartContexts[index] == context) {
            return PLATFORM_ERR_OK;
        }

        if ((g_stm32UartContexts[index] != NULL) &&
            (g_stm32UartContexts[index]->halUart == context->halUart)) {
            return PLATFORM_ERR_ALREADY_INITIALIZED;
        }
    }

    for (index = 0U; index < STM32_UART_IMPL_MAX_CONTEXTS; index++) {
        if (g_stm32UartContexts[index] == NULL) {
            g_stm32UartContexts[index] = context;
            return PLATFORM_ERR_OK;
        }
    }

    return PLATFORM_ERR_NO_RESOURCE;
}

/* 在 DMA 已停止或初始化前失效 RX 缓冲区引用，阻止旧会话继续被回调使用。 */
static void stm32_uart_clear_rx_session(impl_platform_uart_context_t *context)
{
    if (context != NULL) {
        context->rxBuffer = NULL;
        context->rxBufferSize = 0U;
        context->rxLastPosition = 0U;
        context->rxActive = PLATFORM_FALSE;
    }
}

/* 在完成、取消或停止后失效 TX 缓冲区引用，结束发送事务所有权。 */
static void stm32_uart_clear_tx_transaction(impl_platform_uart_context_t *context)
{
    if (context != NULL) {
        context->txBuffer = NULL;
        context->txBufferSize = 0U;
        context->txActive = PLATFORM_FALSE;
    }
}

static void stm32_uart_emit_canceled(platform_uart_t *uart,
                                     platform_uart_direction_t direction)
{
    platform_uart_event_t event;

    event.type = PLATFORM_UART_EVENT_CANCELED;
    event.direction = direction;
    event.data = NULL;
    event.dataLength = 0U;
    event.error = PLATFORM_ERR_CANCELED;
    (void)platform_uart_notify_event(uart, &event);
}

static platform_error_t stm32_uart_cancel_tx(platform_uart_t *uart,
                                             impl_platform_uart_context_t *context)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (context->txActive != PLATFORM_TRUE) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = stm32_uart_map_hal_status(HAL_UART_AbortTransmit(context->halUart));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    stm32_uart_clear_tx_transaction(context);
    stm32_uart_emit_canceled(uart, PLATFORM_UART_DIRECTION_TX);

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_uart_cancel_rx(platform_uart_t *uart,
                                             impl_platform_uart_context_t *context)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (context->rxActive != PLATFORM_TRUE) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = stm32_uart_map_hal_status(HAL_UART_AbortReceive(context->halUart));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    stm32_uart_clear_rx_session(context);
    stm32_uart_emit_canceled(uart, PLATFORM_UART_DIRECTION_RX);

    return PLATFORM_ERR_OK;
}

static void stm32_uart_emit_rx_data(impl_platform_uart_context_t *context,
                                    platform_size_t startPosition,
                                    platform_size_t dataLength)
{
    platform_uart_event_t event;

    if ((context == NULL) || (context->platformUart == NULL) ||
        (context->rxBuffer == NULL) || (dataLength == 0U) ||
        (startPosition >= context->rxBufferSize) ||
        (dataLength > (context->rxBufferSize - startPosition))) {
        return;
    }

    event.type = PLATFORM_UART_EVENT_RX_DATA;
    event.direction = PLATFORM_UART_DIRECTION_RX;
    event.data = &context->rxBuffer[startPosition];
    event.dataLength = dataLength;
    event.error = PLATFORM_ERR_OK;
    (void)platform_uart_notify_event(context->platformUart, &event);
}

static void stm32_uart_process_rx_position(impl_platform_uart_context_t *context,
                                           platform_size_t position)
{
    if ((context == NULL) || (context->rxActive != PLATFORM_TRUE) ||
        (context->rxBuffer == NULL) || (context->rxBufferSize == 0U) ||
        (position > context->rxBufferSize)) {
        return;
    }

    if (position > context->rxLastPosition) {
        stm32_uart_emit_rx_data(context,
                                context->rxLastPosition,
                                position - context->rxLastPosition);
    } else if (position < context->rxLastPosition) {
        stm32_uart_emit_rx_data(context,
                                context->rxLastPosition,
                                context->rxBufferSize - context->rxLastPosition);
        stm32_uart_emit_rx_data(context, 0U, position);
    } else {
        return;
    }

    if (position == context->rxBufferSize) {
        context->rxLastPosition = 0U;
    } else {
        context->rxLastPosition = position;
    }
}

static platform_error_t stm32_uart_map_hal_error(uint32_t errorCode)
{
    if (0U != (errorCode & HAL_UART_ERROR_ORE)) {
        return PLATFORM_ERR_OVERFLOW;
    }

    if (0U != (errorCode & (HAL_UART_ERROR_DMA | STM32_UART_HAL_RX_LINE_ERROR_MASK))) {
        return PLATFORM_ERR_IO;
    }

    return PLATFORM_ERR_IO;
}

static platform_error_t stm32_uart_get_context(platform_uart_t *uart,
                                               impl_platform_uart_context_t **context)
{
    if ((uart == NULL) || (context == NULL) || (uart->implContext == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    *context = (impl_platform_uart_context_t *)uart->implContext;
    if ((*context)->halUart == NULL) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_uart_validate_object(const platform_uart_t *uart)
{
    if (uart == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((platform_object_is_valid(&uart->device.object, PLATFORM_OBJECT_DEVICE) !=
         PLATFORM_TRUE) ||
        (uart->device.dev_class != PLATFORM_DEVICE_CLASS_UART)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_uart_apply_config(UART_HandleTypeDef *halUart,
                                                const platform_uart_config_t *config)
{
    platform_error_t result = PLATFORM_ERR_OK;
    uint32_t wordLength = 0U;
    uint32_t halStopBits = 0U;
    uint32_t halParity = 0U;

    if ((halUart == NULL) || (config == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = stm32_uart_map_word_length(config, &wordLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_uart_map_stop_bits(config->stopBits, &halStopBits);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_uart_map_parity(config->parity, &halParity);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (config->flowControl != PLATFORM_UART_FLOW_CONTROL_NONE) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    halUart->Init.BaudRate = config->baudRate;
    halUart->Init.WordLength = wordLength;
    halUart->Init.StopBits = halStopBits;
    halUart->Init.Parity = halParity;
    halUart->Init.Mode = UART_MODE_TX_RX;
    halUart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    halUart->Init.OverSampling = UART_OVERSAMPLING_16;

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_uart_map_word_length(const platform_uart_config_t *config,
                                                   uint32_t *wordLength)
{
    if ((config == NULL) || (wordLength == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    switch (config->dataBits) {
        case PLATFORM_UART_DATA_BITS_7:
            if (config->parity == PLATFORM_UART_PARITY_NONE) {
                return PLATFORM_ERR_NOT_SUPPORTED;
            }
            *wordLength = UART_WORDLENGTH_8B;
            return PLATFORM_ERR_OK;

        case PLATFORM_UART_DATA_BITS_8:
            if (config->parity == PLATFORM_UART_PARITY_NONE) {
                *wordLength = UART_WORDLENGTH_8B;
            } else {
                *wordLength = UART_WORDLENGTH_9B;
            }
            return PLATFORM_ERR_OK;

        case PLATFORM_UART_DATA_BITS_9:
            /* HAL 在 9-bit 无校验模式按 uint16_t 元素访问 Buffer，
             * 与 Platform UART 的 uint8_t 字节流契约不兼容。 */
            return PLATFORM_ERR_NOT_SUPPORTED;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }
}

static platform_error_t stm32_uart_map_stop_bits(platform_uart_stop_bits_t stopBits,
                                                 uint32_t *halStopBits)
{
    if (halStopBits == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    switch (stopBits) {
        case PLATFORM_UART_STOP_BITS_1:
            *halStopBits = UART_STOPBITS_1;
            return PLATFORM_ERR_OK;

        case PLATFORM_UART_STOP_BITS_2:
            *halStopBits = UART_STOPBITS_2;
            return PLATFORM_ERR_OK;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }
}

static platform_error_t stm32_uart_map_parity(platform_uart_parity_t parity,
                                              uint32_t *halParity)
{
    if (halParity == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    switch (parity) {
        case PLATFORM_UART_PARITY_NONE:
            *halParity = UART_PARITY_NONE;
            return PLATFORM_ERR_OK;

        case PLATFORM_UART_PARITY_EVEN:
            *halParity = UART_PARITY_EVEN;
            return PLATFORM_ERR_OK;

        case PLATFORM_UART_PARITY_ODD:
            *halParity = UART_PARITY_ODD;
            return PLATFORM_ERR_OK;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }
}

/* 配置并初始化 HAL UART，只有成功后才发布 Platform 的 INITIALIZED 状态。 */
static platform_error_t stm32_uart_lifecycle_init(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_uart_context_t *context = NULL;
    platform_uart_t *uart = (platform_uart_t *)self;

    result = stm32_uart_validate_object(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->device.object.state != PLATFORM_OBJECT_CREATED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = stm32_uart_get_context(uart, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    stm32_uart_clear_rx_session(context);
    stm32_uart_clear_tx_transaction(context);

    result = stm32_uart_apply_config(context->halUart, &uart->config);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_uart_map_hal_status(HAL_UART_Init(context->halUart));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    context->platformUart = uart;

    result = platform_object_set_state(&uart->device.object, PLATFORM_OBJECT_INITIALIZED);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_device_set_power_state(&uart->device, PLATFORM_DEVICE_POWER_IDLE);
}

/* 不启动 DMA；仅将已初始化 UART 发布为可接受读写请求的 ACTIVE 状态。 */
static platform_error_t stm32_uart_lifecycle_start(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_uart_t *uart = (platform_uart_t *)self;

    result = stm32_uart_validate_object(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((uart->device.object.state != PLATFORM_OBJECT_INITIALIZED) &&
        (uart->device.object.state != PLATFORM_OBJECT_STOPPED)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = platform_object_set_state(&uart->device.object, PLATFORM_OBJECT_STARTED);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_device_set_power_state(&uart->device, PLATFORM_DEVICE_POWER_ACTIVE);
}

static platform_error_t stm32_uart_lifecycle_process(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_uart_t *uart = (platform_uart_t *)self;

    result = stm32_uart_validate_object(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return PLATFORM_ERR_OK;
}

/* 同步终止活动 DMA 后清除会话引用，再将对象转换为 STOPPED/IDLE。 */
static platform_error_t stm32_uart_lifecycle_stop(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_uart_context_t *context = NULL;
    platform_uart_t *uart = (platform_uart_t *)self;

    result = stm32_uart_validate_object(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = stm32_uart_get_context(uart, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (context->txActive == PLATFORM_TRUE) {
        result = stm32_uart_map_hal_status(HAL_UART_AbortTransmit(context->halUart));
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        stm32_uart_clear_tx_transaction(context);
    }

    if (context->rxActive == PLATFORM_TRUE) {
        result = stm32_uart_map_hal_status(HAL_UART_AbortReceive(context->halUart));
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        stm32_uart_clear_rx_session(context);
    }

    result = platform_object_set_state(&uart->device.object, PLATFORM_OBJECT_STOPPED);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_device_set_power_state(&uart->device, PLATFORM_DEVICE_POWER_IDLE);
}

/* 仅在 STOPPED 后反初始化 HAL，避免 DMA 或回调访问已释放的 UART 资源。 */
static platform_error_t stm32_uart_lifecycle_deinit(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_uart_context_t *context = NULL;
    platform_uart_t *uart = (platform_uart_t *)self;

    result = stm32_uart_validate_object(uart);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (uart->device.object.state != PLATFORM_OBJECT_STOPPED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = stm32_uart_get_context(uart, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_uart_map_hal_status(HAL_UART_DeInit(context->halUart));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    stm32_uart_clear_rx_session(context);
    stm32_uart_clear_tx_transaction(context);
    context->platformUart = NULL;

    result = platform_device_set_power_state(&uart->device, PLATFORM_DEVICE_POWER_OFF);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_object_set_state(&uart->device.object, PLATFORM_OBJECT_CREATED);
}

static platform_error_t stm32_uart_write(platform_uart_t *uart,
                                         const uint8_t *data,
                                         platform_size_t dataLength,
                                         uint32_t timeoutMs,
                                         platform_size_t *writtenLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_uart_context_t *context = NULL;

    if (writtenLength != NULL) {
        *writtenLength = 0U;
    }

    if ((data == NULL) || (dataLength == 0U) || (writtenLength == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (dataLength > STM32_UART_HAL_MAX_TRANSFER_SIZE) {
        return PLATFORM_ERR_OVERFLOW;
    }

    result = stm32_uart_get_context(uart, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_uart_map_hal_status(
        HAL_UART_Transmit(context->halUart, (uint8_t *)data, (uint16_t)dataLength, timeoutMs));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *writtenLength = dataLength;
    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_uart_read(platform_uart_t *uart,
                                        uint8_t *buffer,
                                        platform_size_t bufferSize,
                                        uint32_t timeoutMs,
                                        platform_size_t *readLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_uart_context_t *context = NULL;

    if (readLength != NULL) {
        *readLength = 0U;
    }

    if ((buffer == NULL) || (bufferSize == 0U) || (readLength == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (bufferSize > STM32_UART_HAL_MAX_TRANSFER_SIZE) {
        return PLATFORM_ERR_OVERFLOW;
    }

    result = stm32_uart_get_context(uart, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_uart_map_hal_status(
        HAL_UART_Receive(context->halUart, buffer, (uint16_t)bufferSize, timeoutMs));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *readLength = bufferSize;
    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_uart_write_async(platform_uart_t *uart,
                                               const uint8_t *data,
                                               platform_size_t dataLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_uart_context_t *context = NULL;

    if ((data == NULL) || (dataLength == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (dataLength > STM32_UART_HAL_MAX_TRANSFER_SIZE) {
        return PLATFORM_ERR_OVERFLOW;
    }

    result = stm32_uart_get_context(uart, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (context->txActive == PLATFORM_TRUE) {
        return PLATFORM_ERR_BUSY;
    }

    context->txBuffer = data;
    context->txBufferSize = dataLength;
    context->txActive = PLATFORM_TRUE;
    result = stm32_uart_map_hal_status(HAL_UART_Transmit_DMA(
        context->halUart, (uint8_t *)data, (uint16_t)dataLength));
    if (result != PLATFORM_ERR_OK) {
        stm32_uart_clear_tx_transaction(context);
        return result;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_uart_read_async(platform_uart_t *uart,
                                              uint8_t *buffer,
                                              platform_size_t bufferSize)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_uart_context_t *context = NULL;

    if ((buffer == NULL) || (bufferSize == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (bufferSize > STM32_UART_HAL_MAX_TRANSFER_SIZE) {
        return PLATFORM_ERR_OVERFLOW;
    }

    result = stm32_uart_get_context(uart, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (context->rxActive == PLATFORM_TRUE) {
        return PLATFORM_ERR_BUSY;
    }

    context->rxBuffer = buffer;
    context->rxBufferSize = bufferSize;
    context->rxLastPosition = 0U;
    context->rxActive = PLATFORM_TRUE;
    result = stm32_uart_map_hal_status(HAL_UARTEx_ReceiveToIdle_DMA(
        context->halUart, buffer, (uint16_t)bufferSize));
    if (result != PLATFORM_ERR_OK) {
        stm32_uart_clear_rx_session(context);
        return result;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_uart_cancel(platform_uart_t *uart,
                                          platform_uart_direction_t direction)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_uart_context_t *context = NULL;

    result = stm32_uart_get_context(uart, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (direction == PLATFORM_UART_DIRECTION_TX) {
        return stm32_uart_cancel_tx(uart, context);
    }

    if (direction == PLATFORM_UART_DIRECTION_BOTH) {
        platform_bool_t canceled = PLATFORM_FALSE;

        if (context->txActive == PLATFORM_TRUE) {
            result = stm32_uart_cancel_tx(uart, context);
            if (result != PLATFORM_ERR_OK) {
                return result;
            }

            canceled = PLATFORM_TRUE;
        }

        if (context->rxActive == PLATFORM_TRUE) {
            result = stm32_uart_cancel_rx(uart, context);
            if (result != PLATFORM_ERR_OK) {
                return result;
            }

            canceled = PLATFORM_TRUE;
        }

        if (canceled != PLATFORM_TRUE) {
            return PLATFORM_ERR_INVALID_STATE;
        }

        return PLATFORM_ERR_OK;
    }

    if (direction != PLATFORM_UART_DIRECTION_RX) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return stm32_uart_cancel_rx(uart, context);
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t impl_platform_uart_construct(
    platform_uart_t *uart,
    const char *name,
    uint32_t caps,
    const platform_uart_config_t *config,
    platform_uart_callback_t callback,
    void *callbackContext,
    impl_platform_uart_context_t *context)
{
    platform_uart_init_params_t params;
    platform_error_t result;

    if ((uart == NULL) || (config == NULL) || (context == NULL) ||
        (context->halUart == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    context->platformUart = NULL;
    context->rxBuffer = NULL;
    context->rxBufferSize = 0U;
    context->rxLastPosition = 0U;
    context->rxActive = PLATFORM_FALSE;
    context->txBuffer = NULL;
    context->txBufferSize = 0U;
    context->txActive = PLATFORM_FALSE;

    result = stm32_uart_register_context(context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    params.name = name;
    params.caps = caps;
    params.config = *config;
    params.lifecycle = &g_stm32UartLifecycleOps;
    params.ops = &g_stm32UartOps;
    params.implContext = context;
    params.callback = callback;
    params.callbackContext = callbackContext;

    return platform_uart_init(uart, &params);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
    impl_platform_uart_context_t *context = stm32_uart_find_context(huart);

    if (context != NULL) {
        stm32_uart_process_rx_position(context, (platform_size_t)size);
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    impl_platform_uart_context_t *context = stm32_uart_find_context(huart);
    const uint8_t *data = NULL;
    platform_size_t dataLength = 0U;
    platform_uart_event_t event;

    if ((context == NULL) ||
        (context->txActive != PLATFORM_TRUE) ||
        (context->platformUart == NULL)) {
        return;
    }

    data = context->txBuffer;
    dataLength = context->txBufferSize;
    stm32_uart_clear_tx_transaction(context);
    event.type = PLATFORM_UART_EVENT_TX_COMPLETE;
    event.direction = PLATFORM_UART_DIRECTION_TX;
    event.data = data;
    event.dataLength = dataLength;
    event.error = PLATFORM_ERR_OK;
    (void)platform_uart_notify_event(context->platformUart, &event);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    impl_platform_uart_context_t *context = stm32_uart_find_context(huart);
    platform_uart_event_t event;
    platform_error_t error;

    if ((context == NULL) || (context->platformUart == NULL)) {
        return;
    }

    if ((context->rxActive == PLATFORM_TRUE) &&
        (context->txActive == PLATFORM_TRUE) &&
        (0U == (huart->ErrorCode & STM32_UART_HAL_RX_LINE_ERROR_MASK)) &&
        (0U != (huart->ErrorCode & HAL_UART_ERROR_DMA))) {
        error = stm32_uart_map_hal_error(huart->ErrorCode);
        stm32_uart_clear_rx_session(context);
        stm32_uart_clear_tx_transaction(context);
        event.type = PLATFORM_UART_EVENT_ERROR;
        event.direction = PLATFORM_UART_DIRECTION_BOTH;
        event.data = NULL;
        event.dataLength = 0U;
        event.error = error;
        (void)platform_uart_notify_event(context->platformUart, &event);
        return;
    }

    if (context->rxActive == PLATFORM_TRUE) {
        error = stm32_uart_map_hal_error(huart->ErrorCode);
        stm32_uart_clear_rx_session(context);
        event.type = PLATFORM_UART_EVENT_ERROR;
        event.direction = PLATFORM_UART_DIRECTION_RX;
        event.data = NULL;
        event.dataLength = 0U;
        event.error = error;
        (void)platform_uart_notify_event(context->platformUart, &event);
        return;
    }

    if (context->txActive == PLATFORM_TRUE) {
        error = stm32_uart_map_hal_error(huart->ErrorCode);
        stm32_uart_clear_tx_transaction(context);
        event.type = PLATFORM_UART_EVENT_ERROR;
        event.direction = PLATFORM_UART_DIRECTION_TX;
        event.data = NULL;
        event.dataLength = 0U;
        event.error = error;
        (void)platform_uart_notify_event(context->platformUart, &event);
    }
}
//******************************** Functions *********************************//
