/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_image_sink.c
 * @brief YMODEM 到 Firmware Storage 的镜像接收适配器实现
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#include <stddef.h>
#include <string.h>

#include "firmware_image_sink.h"

static void firmware_image_sink_reset_runtime(firmware_image_sink_t *sink)
{
    firmware_storage_t *storage = sink->storage;
    firmware_slot_t slot = sink->slot;

    (void)memset(sink, 0, sizeof(*sink));
    sink->storage = storage;
    sink->slot = slot;
    sink->lastError = PLATFORM_ERR_OK;
}

static platform_error_t firmware_image_sink_fail(
    firmware_image_sink_t *sink,
    platform_error_t error)
{
    sink->failed = PLATFORM_TRUE;
    sink->lastError = error;
    return error;
}

static platform_error_t firmware_image_sink_prepare_payload(
    firmware_image_sink_t *sink)
{
    firmware_image_validation_t validation;
    uint32_t payloadCapacity;
    platform_error_t result;

    validation = firmware_image_validate_header(sink->headerBuffer, &sink->header);
    if (validation != FIRMWARE_IMAGE_VALIDATION_VALID) {
        return firmware_image_sink_fail(sink, PLATFORM_ERR_INVALID_PARAM);
    }

    result = firmware_storage_get_payload_capacity(sink->storage, &payloadCapacity);
    if (result != PLATFORM_ERR_OK) {
        return firmware_image_sink_fail(sink, result);
    }

    if ((sink->header.imageSize > payloadCapacity) ||
        (sink->expectedFileSize !=
         (FIRMWARE_IMAGE_HEADER_SIZE + sink->header.imageSize))) {
        return firmware_image_sink_fail(sink, PLATFORM_ERR_INVALID_PARAM);
    }

    result = firmware_storage_erase_slot(
        sink->storage,
        sink->slot,
        sink->header.imageSize);
    if (result != PLATFORM_ERR_OK) {
        return firmware_image_sink_fail(sink, result);
    }

    sink->headerValidated = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

static platform_error_t firmware_image_sink_begin(
    void *context,
    const char *filename,
    uint32_t fileSize)
{
    firmware_image_sink_t *sink = (firmware_image_sink_t *)context;
    uint32_t payloadCapacity;
    platform_error_t result;

    if ((sink == NULL) || (filename == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (sink->storage == NULL) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (sink->started != PLATFORM_FALSE) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if ((sink->slot != FIRMWARE_SLOT_A) && (sink->slot != FIRMWARE_SLOT_B)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = firmware_storage_get_payload_capacity(sink->storage, &payloadCapacity);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    firmware_image_sink_reset_runtime(sink);
    if ((fileSize < (FIRMWARE_IMAGE_HEADER_SIZE + 1U)) ||
        (fileSize > (FIRMWARE_IMAGE_HEADER_SIZE + payloadCapacity))) {
        return firmware_image_sink_fail(sink, PLATFORM_ERR_INVALID_PARAM);
    }

    sink->expectedFileSize = fileSize;
    sink->started = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

static platform_error_t firmware_image_sink_write(
    void *context,
    const uint8_t *data,
    uint32_t length)
{
    firmware_image_sink_t *sink = (firmware_image_sink_t *)context;
    uint32_t dataOffset = 0U;
    uint32_t remaining = length;
    platform_error_t result;

    if ((sink == NULL) || (data == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (length == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((sink->started == PLATFORM_FALSE) || (sink->failed != PLATFORM_FALSE)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if ((sink->receivedFileBytes > sink->expectedFileSize) ||
        (length > (sink->expectedFileSize - sink->receivedFileBytes))) {
        return firmware_image_sink_fail(sink, PLATFORM_ERR_INVALID_PARAM);
    }

    while (remaining != 0U) {
        if (sink->headerFillCount < FIRMWARE_IMAGE_HEADER_SIZE) {
            uint32_t copyLength =
                FIRMWARE_IMAGE_HEADER_SIZE - sink->headerFillCount;

            if (copyLength > remaining) {
                copyLength = remaining;
            }

            (void)memcpy(
                &sink->headerBuffer[sink->headerFillCount],
                &data[dataOffset],
                copyLength);
            sink->headerFillCount += copyLength;
            dataOffset += copyLength;
            remaining -= copyLength;

            if (sink->headerFillCount == FIRMWARE_IMAGE_HEADER_SIZE) {
                result = firmware_image_sink_prepare_payload(sink);
                if (result != PLATFORM_ERR_OK) {
                    return result;
                }
            }
        } else {
            uint32_t payloadLength;

            if ((sink->headerValidated == PLATFORM_FALSE) ||
                (sink->payloadWritten >= sink->header.imageSize)) {
                return firmware_image_sink_fail(
                    sink,
                    PLATFORM_ERR_INVALID_PARAM);
            }

            payloadLength = sink->header.imageSize - sink->payloadWritten;
            if (payloadLength > remaining) {
                payloadLength = remaining;
            }

            result = firmware_storage_write_payload(
                sink->storage,
                sink->slot,
                sink->payloadWritten,
                &data[dataOffset],
                payloadLength);
            if (result != PLATFORM_ERR_OK) {
                return firmware_image_sink_fail(sink, result);
            }

            sink->payloadWritten += payloadLength;
            dataOffset += payloadLength;
            remaining -= payloadLength;
        }
    }

    sink->receivedFileBytes += length;
    return PLATFORM_ERR_OK;
}

static platform_error_t firmware_image_sink_end(void *context)
{
    firmware_image_sink_t *sink = (firmware_image_sink_t *)context;
    platform_error_t result;

    if (sink == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((sink->started == PLATFORM_FALSE) || (sink->failed != PLATFORM_FALSE)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if ((sink->receivedFileBytes != sink->expectedFileSize) ||
        (sink->headerFillCount != FIRMWARE_IMAGE_HEADER_SIZE) ||
        (sink->headerValidated == PLATFORM_FALSE) ||
        (sink->payloadWritten != sink->header.imageSize)) {
        return firmware_image_sink_fail(sink, PLATFORM_ERR_INVALID_STATE);
    }

    result = firmware_storage_write_header(
        sink->storage,
        sink->slot,
        sink->headerBuffer);
    if (result != PLATFORM_ERR_OK) {
        return firmware_image_sink_fail(sink, result);
    }

    sink->headerCommitted = PLATFORM_TRUE;
    sink->started = PLATFORM_FALSE;
    return PLATFORM_ERR_OK;
}

static void firmware_image_sink_abort(void *context)
{
    firmware_image_sink_t *sink = (firmware_image_sink_t *)context;

    if (sink != NULL) {
        firmware_image_sink_reset_runtime(sink);
    }
}

platform_error_t firmware_image_sink_init(
    firmware_image_sink_t *sink,
    firmware_storage_t *storage)
{
    if ((sink == NULL) || (storage == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    (void)memset(sink, 0, sizeof(*sink));
    sink->storage = storage;
    sink->slot = FIRMWARE_SLOT_NONE;
    sink->lastError = PLATFORM_ERR_OK;
    return PLATFORM_ERR_OK;
}

platform_error_t firmware_image_sink_set_target_slot(
    firmware_image_sink_t *sink,
    firmware_slot_t slot)
{
    if (sink == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((slot != FIRMWARE_SLOT_A) && (slot != FIRMWARE_SLOT_B)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (sink->started != PLATFORM_FALSE) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    sink->slot = slot;
    return PLATFORM_ERR_OK;
}

platform_error_t firmware_image_sink_get_contract(
    firmware_image_sink_t *sink,
    ymodem_sink_t *contract)
{
    if ((sink == NULL) || (contract == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (sink->storage == NULL) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    contract->begin = firmware_image_sink_begin;
    contract->write = firmware_image_sink_write;
    contract->end = firmware_image_sink_end;
    contract->abort = firmware_image_sink_abort;
    contract->context = sink;
    return PLATFORM_ERR_OK;
}
