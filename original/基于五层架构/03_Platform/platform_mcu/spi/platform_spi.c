/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_spi.c
 * @brief Platform SPI Bus、Device 与同步事务公共接口实现
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_spi.h"

#include "platform_def.h"
//******************************** Includes *********************************//

//******************************** Private Functions *************************//
static platform_error_t platform_spi_validate_bus(
    const platform_spi_bus_t *bus)
{
    if (bus == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((platform_object_is_valid(&bus->device.object,
                                  PLATFORM_OBJECT_DEVICE) != PLATFORM_TRUE) ||
        (bus->device.dev_class != PLATFORM_DEVICE_CLASS_SPI)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_spi_validate_device(
    const platform_spi_device_t *device)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (device == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (device->initialized != PLATFORM_TRUE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    result = platform_spi_validate_bus(device->bus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_spi_validate_config(
    const platform_spi_device_config_t *config)
{
    if (config == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    switch (config->mode) {
        case PLATFORM_SPI_MODE_0:
        case PLATFORM_SPI_MODE_1:
        case PLATFORM_SPI_MODE_2:
        case PLATFORM_SPI_MODE_3:
            break;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }

    switch (config->bitOrder) {
        case PLATFORM_SPI_BIT_ORDER_MSB_FIRST:
        case PLATFORM_SPI_BIT_ORDER_LSB_FIRST:
            break;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }

    if (config->maxClockHz == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (config->dataBits != 8U) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return PLATFORM_ERR_OK;
}

static platform_gpio_level_t platform_spi_get_cs_inactive_level(
    const platform_spi_device_t *device)
{
    return (device->csActiveLevel == PLATFORM_GPIO_LEVEL_LOW) ?
           PLATFORM_GPIO_LEVEL_HIGH : PLATFORM_GPIO_LEVEL_LOW;
}

static void platform_spi_clear_device(platform_spi_device_t *device)
{
    device->name = NULL;
    device->bus = NULL;
    device->cs = NULL;
    device->csActiveLevel = PLATFORM_GPIO_LEVEL_LOW;
    device->config.mode = PLATFORM_SPI_MODE_0;
    device->config.bitOrder = PLATFORM_SPI_BIT_ORDER_MSB_FIRST;
    device->config.dataBits = 0U;
    device->config.maxClockHz = 0U;
    device->initialized = PLATFORM_FALSE;
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_spi_bus_init(
    platform_spi_bus_t *bus,
    const platform_spi_bus_init_params_t *params)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if ((bus == NULL) || (params == NULL) || (params->name == NULL) ||
        (params->lifecycle == NULL) || (params->ops == NULL) ||
        (params->ops->applyConfig == NULL) || (params->ops->write == NULL) ||
        (params->ops->read == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (platform_object_is_valid(&bus->device.object,
                                 PLATFORM_OBJECT_DEVICE) == PLATFORM_TRUE) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    result = platform_device_init(&bus->device,
                                  params->name,
                                  PLATFORM_DEVICE_CLASS_SPI,
                                  params->caps,
                                  params->lifecycle);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    bus->ops = params->ops;
    bus->implContext = params->implContext;
    bus->activeDevice = NULL;

    return PLATFORM_ERR_OK;
}

platform_error_t platform_spi_bus_lifecycle_init(platform_spi_bus_t *bus)
{
    platform_error_t result = platform_spi_validate_bus(bus);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if ((bus->device.lifecycle == NULL) ||
        (bus->device.lifecycle->init == NULL)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return bus->device.lifecycle->init(bus);
}

platform_error_t platform_spi_bus_lifecycle_start(platform_spi_bus_t *bus)
{
    platform_error_t result = platform_spi_validate_bus(bus);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if ((bus->device.lifecycle == NULL) ||
        (bus->device.lifecycle->start == NULL)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return bus->device.lifecycle->start(bus);
}

platform_error_t platform_spi_bus_lifecycle_stop(platform_spi_bus_t *bus)
{
    platform_error_t result = platform_spi_validate_bus(bus);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if ((bus->device.lifecycle == NULL) ||
        (bus->device.lifecycle->stop == NULL)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return bus->device.lifecycle->stop(bus);
}

platform_error_t platform_spi_bus_lifecycle_deinit(platform_spi_bus_t *bus)
{
    platform_error_t result = platform_spi_validate_bus(bus);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if ((bus->device.lifecycle == NULL) ||
        (bus->device.lifecycle->deinit == NULL)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return bus->device.lifecycle->deinit(bus);
}

platform_error_t platform_spi_device_init(
    platform_spi_device_t *device,
    const char *name,
    platform_spi_bus_t *bus,
    platform_gpio_t *cs,
    platform_gpio_level_t csActiveLevel,
    const platform_spi_device_config_t *config)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if ((device == NULL) || (bus == NULL) || (config == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (device->initialized == PLATFORM_TRUE) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    if (name == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_spi_validate_bus(bus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_spi_validate_config(config);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    switch (csActiveLevel) {
        case PLATFORM_GPIO_LEVEL_LOW:
        case PLATFORM_GPIO_LEVEL_HIGH:
            break;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }

    if (cs != NULL) {
        platform_gpio_level_t inactiveLevel =
            (csActiveLevel == PLATFORM_GPIO_LEVEL_LOW) ?
            PLATFORM_GPIO_LEVEL_HIGH : PLATFORM_GPIO_LEVEL_LOW;

        result = platform_gpio_write(cs, inactiveLevel);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
    }

    device->name = name;
    device->bus = bus;
    device->cs = cs;
    device->csActiveLevel = csActiveLevel;
    device->config = *config;
    device->initialized = PLATFORM_TRUE;

    return PLATFORM_ERR_OK;
}

platform_error_t platform_spi_device_deinit(platform_spi_device_t *device)
{
    platform_error_t result = platform_spi_validate_device(device);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (device->bus->activeDevice == device) {
        return PLATFORM_ERR_BUSY;
    }

    if (device->cs != NULL) {
        result = platform_gpio_write(device->cs,
                                     platform_spi_get_cs_inactive_level(device));
    }

    platform_spi_clear_device(device);
    return result;
}

platform_error_t platform_spi_transaction_begin(platform_spi_device_t *device)
{
    platform_error_t result = platform_spi_validate_device(device);
    platform_spi_bus_t *bus = NULL;

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    bus = device->bus;
    if (bus->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (bus->activeDevice != NULL) {
        return PLATFORM_ERR_BUSY;
    }

    result = bus->ops->applyConfig(bus, &device->config);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (device->cs != NULL) {
        result = platform_gpio_write(device->cs, device->csActiveLevel);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
    }

    bus->activeDevice = device;
    return PLATFORM_ERR_OK;
}

platform_error_t platform_spi_write(
    platform_spi_device_t *device,
    const uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_spi_bus_t *bus = NULL;

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (dataLength == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_spi_validate_device(device);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    bus = device->bus;
    if (bus->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (bus->activeDevice != device) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return bus->ops->write(bus, data, dataLength);
}

platform_error_t platform_spi_read(
    platform_spi_device_t *device,
    uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_spi_bus_t *bus = NULL;

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (dataLength == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_spi_validate_device(device);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    bus = device->bus;
    if (bus->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (bus->activeDevice != device) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return bus->ops->read(bus, data, dataLength);
}

platform_error_t platform_spi_transaction_end(platform_spi_device_t *device)
{
    platform_error_t result = platform_spi_validate_device(device);
    platform_spi_bus_t *bus = NULL;

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    bus = device->bus;
    if (bus->activeDevice != device) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (device->cs != NULL) {
        result = platform_gpio_write(device->cs,
                                     platform_spi_get_cs_inactive_level(device));
    }

    bus->activeDevice = NULL;
    return result;
}
//******************************** Functions *********************************//
