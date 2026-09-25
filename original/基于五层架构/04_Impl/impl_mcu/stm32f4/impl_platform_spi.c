/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_spi.c
 * @brief STM32F4 HAL SPI Platform 固定配置校验与阻塞收发实现
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.1
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "impl_platform_spi.h"

#include "platform_device.h"
#include "platform_object.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/* 单个 HAL blocking transfer 的 Size 参数是 16-bit；超出部分在 Impl 内拆分，不是 Platform 语义上限。 */
#define STM32_SPI_HAL_MAX_TRANSFER_SIZE ((platform_size_t)0xFFFFU)
#define STM32_SPI_BLOCKING_TIMEOUT_MS   (1000U)
//******************************** Defines *********************************//

//******************************** Private Functions *************************//
static platform_error_t stm32_spi_map_hal_status(HAL_StatusTypeDef halStatus);
static platform_error_t stm32_spi_get_context(
    platform_spi_bus_t *bus,
    impl_platform_spi_context_t **context);
static platform_error_t stm32_spi_validate_bus(const platform_spi_bus_t *bus);
static platform_error_t stm32_spi_get_mode(
    const SPI_HandleTypeDef *halSpi,
    platform_spi_mode_t *mode);
static platform_error_t stm32_spi_get_bit_order(
    const SPI_HandleTypeDef *halSpi,
    platform_spi_bit_order_t *bitOrder);
static platform_error_t stm32_spi_get_clock_divisor(
    uint32_t halPrescaler,
    uint32_t *divisor);
static platform_error_t stm32_spi_get_peripheral_clock_hz(
    const SPI_HandleTypeDef *halSpi,
    uint32_t *clockHz);
static platform_error_t stm32_spi_get_actual_clock_hz(
    const SPI_HandleTypeDef *halSpi,
    uint32_t *actualClockHz);
static platform_error_t stm32_spi_apply_config(
    platform_spi_bus_t *bus,
    const platform_spi_device_config_t *config);
static platform_size_t stm32_spi_get_transfer_chunk_length(
    platform_size_t remainingLength);
static platform_error_t stm32_spi_write(
    platform_spi_bus_t *bus,
    const uint8_t *data,
    platform_size_t dataLength);
static platform_error_t stm32_spi_read(
    platform_spi_bus_t *bus,
    uint8_t *data,
    platform_size_t dataLength);
static platform_error_t stm32_spi_bind(
    platform_spi_bus_t *bus,
    const char *name,
    uint32_t caps,
    impl_platform_spi_context_t *context);
static platform_error_t stm32_spi_lifecycle_init(void *self);
static platform_error_t stm32_spi_lifecycle_start(void *self);
static platform_error_t stm32_spi_lifecycle_process(void *self);
static platform_error_t stm32_spi_lifecycle_stop(void *self);
static platform_error_t stm32_spi_lifecycle_deinit(void *self);
//******************************** Private Functions *************************//

//******************************** Constants *********************************//
static const platform_lifecycle_ops_t g_stm32SpiLifecycleOps = {
    stm32_spi_lifecycle_init,
    stm32_spi_lifecycle_start,
    stm32_spi_lifecycle_process,
    stm32_spi_lifecycle_stop,
    stm32_spi_lifecycle_deinit
};

static const platform_spi_bus_ops_t g_stm32SpiOps = {
    stm32_spi_apply_config,
    stm32_spi_write,
    stm32_spi_read
};
//******************************** Constants *********************************//

//******************************** Private Functions *************************//
static platform_error_t stm32_spi_map_hal_status(HAL_StatusTypeDef halStatus)
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

static platform_error_t stm32_spi_get_context(
    platform_spi_bus_t *bus,
    impl_platform_spi_context_t **context)
{
    if ((bus == NULL) || (context == NULL) || (bus->implContext == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    *context = (impl_platform_spi_context_t *)bus->implContext;
    if ((*context)->halSpi == NULL) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_validate_bus(const platform_spi_bus_t *bus)
{
    if (bus == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((platform_object_is_valid(&bus->device.object,
                                  PLATFORM_OBJECT_DEVICE) != PLATFORM_TRUE) ||
        (bus->device.dev_class != PLATFORM_DEVICE_CLASS_SPI)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_get_mode(
    const SPI_HandleTypeDef *halSpi,
    platform_spi_mode_t *mode)
{
    if ((halSpi == NULL) || (mode == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((halSpi->Init.CLKPolarity == SPI_POLARITY_LOW) &&
        (halSpi->Init.CLKPhase == SPI_PHASE_1EDGE)) {
        *mode = PLATFORM_SPI_MODE_0;
    } else if ((halSpi->Init.CLKPolarity == SPI_POLARITY_LOW) &&
               (halSpi->Init.CLKPhase == SPI_PHASE_2EDGE)) {
        *mode = PLATFORM_SPI_MODE_1;
    } else if ((halSpi->Init.CLKPolarity == SPI_POLARITY_HIGH) &&
               (halSpi->Init.CLKPhase == SPI_PHASE_1EDGE)) {
        *mode = PLATFORM_SPI_MODE_2;
    } else if ((halSpi->Init.CLKPolarity == SPI_POLARITY_HIGH) &&
               (halSpi->Init.CLKPhase == SPI_PHASE_2EDGE)) {
        *mode = PLATFORM_SPI_MODE_3;
    } else {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_get_bit_order(
    const SPI_HandleTypeDef *halSpi,
    platform_spi_bit_order_t *bitOrder)
{
    if ((halSpi == NULL) || (bitOrder == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (halSpi->Init.FirstBit == SPI_FIRSTBIT_MSB) {
        *bitOrder = PLATFORM_SPI_BIT_ORDER_MSB_FIRST;
    } else if (halSpi->Init.FirstBit == SPI_FIRSTBIT_LSB) {
        *bitOrder = PLATFORM_SPI_BIT_ORDER_LSB_FIRST;
    } else {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_get_clock_divisor(
    uint32_t halPrescaler,
    uint32_t *divisor)
{
    if (divisor == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    switch (halPrescaler) {
        case SPI_BAUDRATEPRESCALER_2:
            *divisor = 2U;
            break;

        case SPI_BAUDRATEPRESCALER_4:
            *divisor = 4U;
            break;

        case SPI_BAUDRATEPRESCALER_8:
            *divisor = 8U;
            break;

        case SPI_BAUDRATEPRESCALER_16:
            *divisor = 16U;
            break;

        case SPI_BAUDRATEPRESCALER_32:
            *divisor = 32U;
            break;

        case SPI_BAUDRATEPRESCALER_64:
            *divisor = 64U;
            break;

        case SPI_BAUDRATEPRESCALER_128:
            *divisor = 128U;
            break;

        case SPI_BAUDRATEPRESCALER_256:
            *divisor = 256U;
            break;

        default:
            return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_get_actual_clock_hz(
    const SPI_HandleTypeDef *halSpi,
    uint32_t *actualClockHz)
{
    platform_error_t result = PLATFORM_ERR_OK;
    uint32_t divisor = 0U;
    uint32_t peripheralClockHz = 0U;

    if ((halSpi == NULL) || (actualClockHz == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = stm32_spi_get_clock_divisor(halSpi->Init.BaudRatePrescaler,
                                         &divisor);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_spi_get_peripheral_clock_hz(halSpi,
                                               &peripheralClockHz);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *actualClockHz = peripheralClockHz / divisor;
    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_get_peripheral_clock_hz(
    const SPI_HandleTypeDef *halSpi,
    uint32_t *clockHz)
{
    if ((halSpi == NULL) || (clockHz == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (halSpi->Instance == SPI1) {
        *clockHz = HAL_RCC_GetPCLK2Freq();
    } else if (halSpi->Instance == SPI2) {
        *clockHz = HAL_RCC_GetPCLK1Freq();
    } else {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return (*clockHz == 0U) ?
           PLATFORM_ERR_NOT_INITIALIZED : PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_apply_config(
    platform_spi_bus_t *bus,
    const platform_spi_device_config_t *config)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_spi_context_t *context = NULL;
    platform_spi_mode_t currentMode = PLATFORM_SPI_MODE_0;
    platform_spi_bit_order_t currentBitOrder =
        PLATFORM_SPI_BIT_ORDER_MSB_FIRST;
    uint32_t actualClockHz = 0U;

    if (config == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = stm32_spi_get_context(bus, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_spi_get_mode(context->halSpi, &currentMode);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_spi_get_bit_order(context->halSpi, &currentBitOrder);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_spi_get_actual_clock_hz(context->halSpi, &actualClockHz);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((currentMode != config->mode) ||
        (currentBitOrder != config->bitOrder) ||
        (context->halSpi->Init.DataSize != SPI_DATASIZE_8BIT) ||
        (config->dataBits != 8U) ||
        (actualClockHz > config->maxClockHz)) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return PLATFORM_ERR_OK;
}

static platform_size_t stm32_spi_get_transfer_chunk_length(
    platform_size_t remainingLength)
{
    return (remainingLength > STM32_SPI_HAL_MAX_TRANSFER_SIZE) ?
           STM32_SPI_HAL_MAX_TRANSFER_SIZE : remainingLength;
}

static platform_error_t stm32_spi_write(
    platform_spi_bus_t *bus,
    const uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_spi_context_t *context = NULL;
    const uint8_t *currentData = NULL;
    platform_size_t chunkLength = 0U;
    platform_size_t remaining = 0U;

    if ((data == NULL) || (dataLength == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = stm32_spi_get_context(bus, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /* 同一 Platform transaction 内拆成多个 HAL chunk，CS 仍由上层保持全程有效。 */
    currentData = data;
    remaining = dataLength;

    while (remaining > 0U) {
        chunkLength = stm32_spi_get_transfer_chunk_length(remaining);

        result = stm32_spi_map_hal_status(HAL_SPI_Transmit(
            context->halSpi,
            (uint8_t *)currentData,
            (uint16_t)chunkLength,
            STM32_SPI_BLOCKING_TIMEOUT_MS));
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        currentData += chunkLength;
        remaining -= chunkLength;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_read(
    platform_spi_bus_t *bus,
    uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_spi_context_t *context = NULL;
    uint8_t *currentData = NULL;
    platform_size_t chunkLength = 0U;
    platform_size_t remaining = 0U;

    if ((data == NULL) || (dataLength == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = stm32_spi_get_context(bus, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /* 与 stm32_spi_write() 保持对称：长度语义不依赖 HAL 单次传输上限。 */
    currentData = data;
    remaining = dataLength;

    while (remaining > 0U) {
        chunkLength = stm32_spi_get_transfer_chunk_length(remaining);

        result = stm32_spi_map_hal_status(HAL_SPI_Receive(
            context->halSpi,
            currentData,
            (uint16_t)chunkLength,
            STM32_SPI_BLOCKING_TIMEOUT_MS));
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        currentData += chunkLength;
        remaining -= chunkLength;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_lifecycle_init(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_spi_context_t *context = NULL;
    platform_spi_bus_t *bus = (platform_spi_bus_t *)self;

    result = stm32_spi_validate_bus(bus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (bus->device.object.state != PLATFORM_OBJECT_CREATED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = stm32_spi_get_context(bus, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((context->halSpi->Instance == NULL) ||
        (HAL_SPI_GetState(context->halSpi) == HAL_SPI_STATE_RESET)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    bus->activeDevice = NULL;
    result = platform_object_set_state(&bus->device.object,
                                       PLATFORM_OBJECT_INITIALIZED);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_device_set_power_state(&bus->device,
                                           PLATFORM_DEVICE_POWER_IDLE);
}

static platform_error_t stm32_spi_lifecycle_start(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_spi_bus_t *bus = (platform_spi_bus_t *)self;

    result = stm32_spi_validate_bus(bus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((bus->device.object.state != PLATFORM_OBJECT_INITIALIZED) &&
        (bus->device.object.state != PLATFORM_OBJECT_STOPPED)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = platform_object_set_state(&bus->device.object,
                                       PLATFORM_OBJECT_STARTED);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_device_set_power_state(&bus->device,
                                           PLATFORM_DEVICE_POWER_ACTIVE);
}

static platform_error_t stm32_spi_lifecycle_process(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_spi_bus_t *bus = (platform_spi_bus_t *)self;

    result = stm32_spi_validate_bus(bus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (bus->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_spi_lifecycle_stop(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_spi_bus_t *bus = (platform_spi_bus_t *)self;

    result = stm32_spi_validate_bus(bus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (bus->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (bus->activeDevice != NULL) {
        return PLATFORM_ERR_BUSY;
    }

    result = platform_object_set_state(&bus->device.object,
                                       PLATFORM_OBJECT_STOPPED);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_device_set_power_state(&bus->device,
                                           PLATFORM_DEVICE_POWER_IDLE);
}

static platform_error_t stm32_spi_lifecycle_deinit(void *self)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_spi_context_t *context = NULL;
    platform_spi_bus_t *bus = (platform_spi_bus_t *)self;

    result = stm32_spi_validate_bus(bus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((bus->device.object.state != PLATFORM_OBJECT_INITIALIZED) &&
        (bus->device.object.state != PLATFORM_OBJECT_STOPPED)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (bus->activeDevice != NULL) {
        return PLATFORM_ERR_BUSY;
    }

    result = stm32_spi_get_context(bus, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_device_set_power_state(&bus->device,
                                             PLATFORM_DEVICE_POWER_OFF);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_object_set_state(&bus->device.object,
                                     PLATFORM_OBJECT_CREATED);
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t impl_platform_spi_construct(
    platform_spi_bus_t *bus,
    const char *name,
    uint32_t caps,
    impl_platform_spi_context_t *context)
{
    if ((context == NULL) || (context->halSpi == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return stm32_spi_bind(bus,
                          name,
                          caps,
                          context);
}

static platform_error_t stm32_spi_bind(
    platform_spi_bus_t *bus,
    const char *name,
    uint32_t caps,
    impl_platform_spi_context_t *context)
{
    platform_spi_bus_init_params_t params;

    if ((bus == NULL) || (name == NULL) || (context == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    params.name = name;
    params.caps = caps;
    params.lifecycle = &g_stm32SpiLifecycleOps;
    params.ops = &g_stm32SpiOps;
    params.implContext = context;

    return platform_spi_bus_init(bus, &params);
}
//******************************** Functions *********************************//
