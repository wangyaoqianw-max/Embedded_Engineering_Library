/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_at24c02.c
 * @brief AT24C02 Platform Raw Driver 读路径与生命周期实现
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_at24c02.h"

#include "platform_def.h"
#include "platform_time.h"

#include <stddef.h>
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_AT24C02_WORD_ADDRESS_SIZE_BYTES    (1U)
#define PLATFORM_AT24C02_PAGE_WRITE_BUFFER_SIZE_BYTES \
    (PLATFORM_AT24C02_PAGE_SIZE_BYTES + \
     PLATFORM_AT24C02_WORD_ADDRESS_SIZE_BYTES)
#define PLATFORM_AT24C02_WRITE_POLL_INTERVAL_MS     (1U)
#define PLATFORM_AT24C02_WRITE_TIMEOUT_MS           (10U)
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
static platform_error_t platform_at24c02_validate_initialized(
    const platform_at24c02_t *eeprom);
static platform_error_t platform_at24c02_validate_range(
    uint32_t address,
    platform_size_t dataLength);
static void platform_at24c02_clear_binding(
    platform_at24c02_t *eeprom);
static platform_error_t platform_at24c02_page_write(
    platform_at24c02_t *eeprom,
    uint32_t address,
    const uint8_t *data,
    platform_size_t dataLength);
static platform_error_t platform_at24c02_wait_ready(
    platform_at24c02_t *eeprom);
//******************************** Declaring *********************************//

//******************************** Private Functions *************************//
static platform_error_t platform_at24c02_validate_initialized(
    const platform_at24c02_t *eeprom)
{
    if (eeprom == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (eeprom->initialized != PLATFORM_TRUE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

/* 校验完整 EEPROM 地址范围，避免通过 address + dataLength 产生溢出。 */
static platform_error_t platform_at24c02_validate_range(
    uint32_t address,
    platform_size_t dataLength)
{
    if (dataLength == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (address >= PLATFORM_AT24C02_TOTAL_SIZE_BYTES) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (dataLength > (PLATFORM_AT24C02_TOTAL_SIZE_BYTES - address)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

static void platform_at24c02_clear_binding(
    platform_at24c02_t *eeprom)
{
    eeprom->i2c = NULL;
    eeprom->deviceAddress = 0U;
    eeprom->initialized = PLATFORM_FALSE;
}

/* 组装一笔不跨 Page 的 [Word Address][Data...] 写事务。 */
static platform_error_t platform_at24c02_page_write(
    platform_at24c02_t *eeprom,
    uint32_t address,
    const uint8_t *data,
    platform_size_t dataLength)
{
    uint8_t txData[PLATFORM_AT24C02_PAGE_WRITE_BUFFER_SIZE_BYTES] = {0U};
    platform_size_t index = 0U;
    platform_size_t pageOffset = 0U;
    platform_size_t pageRemaining = 0U;
    platform_error_t result = platform_at24c02_validate_initialized(eeprom);

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_at24c02_validate_range(address, dataLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    pageOffset = address % PLATFORM_AT24C02_PAGE_SIZE_BYTES;
    pageRemaining = PLATFORM_AT24C02_PAGE_SIZE_BYTES - pageOffset;
    if ((dataLength > PLATFORM_AT24C02_PAGE_SIZE_BYTES) ||
        (dataLength > pageRemaining)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    txData[0] = (uint8_t)address;
    for (index = 0U; index < dataLength; index++) {
        txData[index + PLATFORM_AT24C02_WORD_ADDRESS_SIZE_BYTES] = data[index];
    }

    return platform_i2c_write(eeprom->i2c,
                              eeprom->deviceAddress,
                              txData,
                              (uint16_t)(dataLength +
                                         PLATFORM_AT24C02_WORD_ADDRESS_SIZE_BYTES));
}

/* 通过单次地址探测等待 EEPROM 内部写周期结束，最多等待 10 ms。 */
static platform_error_t platform_at24c02_wait_ready(
    platform_at24c02_t *eeprom)
{
    uint32_t elapsedMs = 0U;
    platform_error_t result = PLATFORM_ERR_OK;

    while (elapsedMs < PLATFORM_AT24C02_WRITE_TIMEOUT_MS) {
        result = platform_i2c_probe(eeprom->i2c, eeprom->deviceAddress);
        if (result == PLATFORM_ERR_OK) {
            return PLATFORM_ERR_OK;
        }

        if (result != PLATFORM_ERR_NOT_FOUND) {
            return result;
        }

        result = platform_time_delay_ms(
            PLATFORM_AT24C02_WRITE_POLL_INTERVAL_MS);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        elapsedMs += PLATFORM_AT24C02_WRITE_POLL_INTERVAL_MS;
    }

    return PLATFORM_ERR_TIMEOUT;
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_at24c02_init(
    platform_at24c02_t *eeprom,
    platform_i2c_t *i2c,
    uint8_t deviceAddress)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (eeprom == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (i2c == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (eeprom->initialized == PLATFORM_TRUE) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    if ((deviceAddress < PLATFORM_AT24C02_I2C_ADDRESS_MIN) ||
        (deviceAddress > PLATFORM_AT24C02_I2C_ADDRESS_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    eeprom->i2c = i2c;
    eeprom->deviceAddress = deviceAddress;
    eeprom->initialized = PLATFORM_FALSE;

    result = platform_i2c_probe(i2c, deviceAddress);
    if (result != PLATFORM_ERR_OK) {
        platform_at24c02_clear_binding(eeprom);
        return result;
    }

    eeprom->initialized = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

platform_error_t platform_at24c02_deinit(
    platform_at24c02_t *eeprom)
{
    platform_error_t result = platform_at24c02_validate_initialized(eeprom);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    platform_at24c02_clear_binding(eeprom);
    return PLATFORM_ERR_OK;
}

platform_error_t platform_at24c02_read(
    platform_at24c02_t *eeprom,
    uint32_t address,
    uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = platform_at24c02_validate_initialized(eeprom);
    uint8_t wordAddress = 0U;

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_at24c02_validate_range(address, dataLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    wordAddress = (uint8_t)address;
    return platform_i2c_write_read(eeprom->i2c,
                                   eeprom->deviceAddress,
                                   &wordAddress,
                                   PLATFORM_AT24C02_WORD_ADDRESS_SIZE_BYTES,
                                   data,
                                   (uint16_t)dataLength);
}

platform_error_t platform_at24c02_write(
    platform_at24c02_t *eeprom,
    uint32_t address,
    const uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = platform_at24c02_validate_initialized(eeprom);
    uint32_t currentAddress = address;
    const uint8_t *currentData = data;
    platform_size_t remaining = dataLength;

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_at24c02_validate_range(address, dataLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    while (remaining > 0U) {
        platform_size_t pageOffset =
            currentAddress % PLATFORM_AT24C02_PAGE_SIZE_BYTES;
        platform_size_t pageRemaining =
            PLATFORM_AT24C02_PAGE_SIZE_BYTES - pageOffset;
        platform_size_t chunkLength =
            (remaining < pageRemaining) ? remaining : pageRemaining;

        result = platform_at24c02_page_write(eeprom,
                                             currentAddress,
                                             currentData,
                                             chunkLength);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        result = platform_at24c02_wait_ready(eeprom);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        currentAddress += (uint32_t)chunkLength;
        currentData += chunkLength;
        remaining -= chunkLength;
    }

    return PLATFORM_ERR_OK;
}
//******************************** Functions *********************************//
