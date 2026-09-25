/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_storage.c
 * @brief Firmware Image Storage 通用后端抽象实现
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#include <stddef.h>

#include "crc.h"
#include "firmware_storage.h"

#define FIRMWARE_STORAGE_CRC_BUFFER_SIZE       (256U)

static platform_bool_t firmware_storage_slot_is_valid(firmware_slot_t slot)
{
    return ((slot == FIRMWARE_SLOT_A) || (slot == FIRMWARE_SLOT_B)) ?
           PLATFORM_TRUE : PLATFORM_FALSE;
}

static platform_error_t firmware_storage_check_ready(
    const firmware_storage_t *storage)
{
    if (storage == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((storage->initialized == PLATFORM_FALSE) ||
        (storage->backend.erase_image == NULL) ||
        (storage->backend.read_header == NULL) ||
        (storage->backend.read_payload == NULL) ||
        (storage->backend.write_payload == NULL) ||
        (storage->backend.write_header == NULL) ||
        (storage->backend.payloadCapacity == 0U)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t firmware_storage_init(
    firmware_storage_t *storage,
    const firmware_storage_backend_t *backend)
{
    if ((storage == NULL) || (backend == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((backend->erase_image == NULL) ||
        (backend->read_header == NULL) ||
        (backend->read_payload == NULL) ||
        (backend->write_payload == NULL) ||
        (backend->write_header == NULL) ||
        (backend->payloadCapacity == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    storage->backend = *backend;
    storage->initialized = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

platform_error_t firmware_storage_get_payload_capacity(
    const firmware_storage_t *storage,
    uint32_t *payloadCapacity)
{
    platform_error_t result;

    if (payloadCapacity == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = firmware_storage_check_ready(storage);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *payloadCapacity = storage->backend.payloadCapacity;
    return PLATFORM_ERR_OK;
}

platform_error_t firmware_storage_read_header(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    firmware_image_header_t *header,
    firmware_image_validation_t *validation)
{
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE];
    platform_error_t result;

    if ((header == NULL) || (validation == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    *validation = FIRMWARE_IMAGE_VALIDATION_UNKNOWN;
    result = firmware_storage_check_ready(storage);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (firmware_storage_slot_is_valid(slot) == PLATFORM_FALSE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = storage->backend.read_header(
        storage->backend.context,
        slot,
        rawHeader);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *validation = firmware_image_validate_header(rawHeader, header);
    if ((*validation == FIRMWARE_IMAGE_VALIDATION_VALID) &&
        (header->imageSize > storage->backend.payloadCapacity)) {
        *validation = FIRMWARE_IMAGE_VALIDATION_INVALID_SIZE;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t firmware_storage_validate_image(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    firmware_image_header_t *header,
    firmware_image_validation_t *validation)
{
    crc32_iso_hdlc_context_t crcContext;
    uint8_t buffer[FIRMWARE_STORAGE_CRC_BUFFER_SIZE];
    uint32_t remaining;
    uint32_t offset = 0U;
    uint32_t readLength;
    platform_error_t result;

    result = firmware_storage_read_header(storage, slot, header, validation);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (*validation != FIRMWARE_IMAGE_VALIDATION_VALID) {
        return PLATFORM_ERR_OK;
    }

    remaining = header->imageSize;
    crc32_iso_hdlc_init(&crcContext);

    while (remaining != 0U) {
        readLength = (remaining > FIRMWARE_STORAGE_CRC_BUFFER_SIZE) ?
                     FIRMWARE_STORAGE_CRC_BUFFER_SIZE : remaining;

        result = storage->backend.read_payload(
            storage->backend.context,
            slot,
            offset,
            buffer,
            readLength);
        if (result != PLATFORM_ERR_OK) {
            *validation = FIRMWARE_IMAGE_VALIDATION_UNKNOWN;
            return result;
        }

        crc32_iso_hdlc_update(&crcContext, buffer, readLength);
        offset += readLength;
        remaining -= readLength;
    }

    if (crc32_iso_hdlc_finalize(&crcContext) != header->payloadCrc32) {
        *validation = FIRMWARE_IMAGE_VALIDATION_INVALID_PAYLOAD_CRC;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t firmware_storage_erase_slot(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    uint32_t payloadSize)
{
    platform_error_t result = firmware_storage_check_ready(storage);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (firmware_storage_slot_is_valid(slot) == PLATFORM_FALSE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((payloadSize == 0U) ||
        (payloadSize > storage->backend.payloadCapacity)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return storage->backend.erase_image(
        storage->backend.context,
        slot,
        payloadSize);
}

platform_error_t firmware_storage_write_payload(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    uint32_t payloadOffset,
    const uint8_t *data,
    uint32_t length)
{
    platform_error_t result;

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (length == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = firmware_storage_check_ready(storage);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (firmware_storage_slot_is_valid(slot) == PLATFORM_FALSE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((payloadOffset > storage->backend.payloadCapacity) ||
        (length > (storage->backend.payloadCapacity - payloadOffset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return storage->backend.write_payload(
        storage->backend.context,
        slot,
        payloadOffset,
        data,
        length);
}

platform_error_t firmware_storage_write_header(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE])
{
    platform_error_t result;

    if (rawHeader == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = firmware_storage_check_ready(storage);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (firmware_storage_slot_is_valid(slot) == PLATFORM_FALSE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return storage->backend.write_header(
        storage->backend.context,
        slot,
        rawHeader);
}
