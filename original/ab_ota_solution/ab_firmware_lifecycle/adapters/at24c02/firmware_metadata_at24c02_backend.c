/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_metadata_at24c02_backend.c
 * @brief AT24C02 到 Firmware Metadata Backend 的适配器实现
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#include <stddef.h>

#include "firmware_metadata_at24c02_backend.h"

#define FIRMWARE_METADATA_AT24C02_COPY_A_ADDRESS   (0x00U)
#define FIRMWARE_METADATA_AT24C02_COPY_B_ADDRESS   (0x80U)

static platform_error_t firmware_metadata_at24c02_base(
    firmware_metadata_copy_id_t copy,
    uint32_t *baseAddress)
{
    if (baseAddress == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (copy == FIRMWARE_METADATA_COPY_A) {
        *baseAddress = FIRMWARE_METADATA_AT24C02_COPY_A_ADDRESS;
        return PLATFORM_ERR_OK;
    }

    if (copy == FIRMWARE_METADATA_COPY_B) {
        *baseAddress = FIRMWARE_METADATA_AT24C02_COPY_B_ADDRESS;
        return PLATFORM_ERR_OK;
    }

    return PLATFORM_ERR_INVALID_PARAM;
}

static platform_error_t firmware_metadata_at24c02_read(
    void *context,
    firmware_metadata_copy_id_t copy,
    uint32_t offset,
    uint8_t *data,
    uint32_t length)
{
    firmware_metadata_at24c02_backend_t *adapter =
        (firmware_metadata_at24c02_backend_t *)context;
    uint32_t baseAddress;
    platform_error_t result;

    if ((adapter == NULL) || (adapter->eeprom == NULL) || (data == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((offset > FIRMWARE_METADATA_COPY_SIZE) ||
        (length > (FIRMWARE_METADATA_COPY_SIZE - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = firmware_metadata_at24c02_base(copy, &baseAddress);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_at24c02_read(
        adapter->eeprom,
        baseAddress + offset,
        data,
        length);
}

static platform_error_t firmware_metadata_at24c02_write(
    void *context,
    firmware_metadata_copy_id_t copy,
    uint32_t offset,
    const uint8_t *data,
    uint32_t length)
{
    firmware_metadata_at24c02_backend_t *adapter =
        (firmware_metadata_at24c02_backend_t *)context;
    uint32_t baseAddress;
    platform_error_t result;

    if ((adapter == NULL) || (adapter->eeprom == NULL) || (data == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((offset > FIRMWARE_METADATA_COPY_SIZE) ||
        (length > (FIRMWARE_METADATA_COPY_SIZE - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = firmware_metadata_at24c02_base(copy, &baseAddress);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_at24c02_write(
        adapter->eeprom,
        baseAddress + offset,
        data,
        length);
}

platform_error_t firmware_metadata_at24c02_backend_init(
    firmware_metadata_at24c02_backend_t *adapter,
    platform_at24c02_t *eeprom,
    firmware_metadata_backend_t *backend)
{
    if ((adapter == NULL) || (eeprom == NULL) || (backend == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    adapter->eeprom = eeprom;
    backend->read = firmware_metadata_at24c02_read;
    backend->write = firmware_metadata_at24c02_write;
    backend->context = adapter;
    return PLATFORM_ERR_OK;
}
