/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_i2c.c
 * @brief Platform I2C 同步事务公共契约实现
 * @author YaoQian Wang
 * @date 2026-09-27
 * @version V2.0
 *
 *****************************************************************************/

#include "platform_i2c.h"

#include <stddef.h>

static platform_error_t platform_i2c_validate_initialized(
    const platform_i2c_t *i2c)
{
    if (i2c == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((i2c->initialized != PLATFORM_TRUE) ||
        (i2c->ops == NULL) ||
        (i2c->implContext == NULL)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_validate_address(uint8_t address)
{
    return (address <= 0x7FU) ?
        PLATFORM_ERR_OK : PLATFORM_ERR_INVALID_PARAM;
}

static void platform_i2c_clear_binding(platform_i2c_t *i2c)
{
    i2c->name = NULL;
    i2c->ops = NULL;
    i2c->implContext = NULL;
    i2c->initialized = PLATFORM_FALSE;
}

platform_error_t platform_i2c_init(
    platform_i2c_t *i2c,
    const platform_i2c_init_params_t *params)
{
    platform_error_t result;

    if ((i2c == NULL) || (params == NULL) ||
        (params->ops == NULL) || (params->implContext == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (i2c->initialized == PLATFORM_TRUE) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    if ((params->ops->probe == NULL) ||
        (params->ops->write == NULL) ||
        (params->ops->read == NULL) ||
        (params->ops->writeRead == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    i2c->name = params->name;
    i2c->ops = params->ops;
    i2c->implContext = params->implContext;
    i2c->initialized = PLATFORM_FALSE;

    if (i2c->ops->init != NULL) {
        result = i2c->ops->init(i2c);
        if (result != PLATFORM_ERR_OK) {
            platform_i2c_clear_binding(i2c);
            return result;
        }
    }

    i2c->initialized = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

platform_error_t platform_i2c_probe(
    platform_i2c_t *i2c,
    uint8_t address)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return i2c->ops->probe(i2c, address);
}

platform_error_t platform_i2c_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((data == NULL) || (length == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return i2c->ops->write(i2c, address, data, length);
}

platform_error_t platform_i2c_read(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t *data,
    uint16_t length)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((data == NULL) || (length == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return i2c->ops->read(i2c, address, data, length);
}

platform_error_t platform_i2c_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((txData == NULL) || (txLength == 0U) ||
        (rxData == NULL) || (rxLength == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return i2c->ops->writeRead(
        i2c,
        address,
        txData,
        txLength,
        rxData,
        rxLength);
}

platform_error_t platform_i2c_deinit(platform_i2c_t *i2c)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (i2c->ops->deinit != NULL) {
        result = i2c->ops->deinit(i2c);
    } else {
        result = PLATFORM_ERR_OK;
    }

    platform_i2c_clear_binding(i2c);
    return result;
}
