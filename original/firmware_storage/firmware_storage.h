/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_storage.h
 * @brief Firmware Image Storage 通用后端抽象
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef FIRMWARE_STORAGE_H
#define FIRMWARE_STORAGE_H

#include "firmware_image.h"

#define FIRMWARE_STORAGE_INITIALIZER       {0}

typedef enum
{
    FIRMWARE_SLOT_A = 0,
    FIRMWARE_SLOT_B = 1,
    FIRMWARE_SLOT_NONE = 0xFF
} firmware_slot_t;

typedef struct
{
    platform_error_t (*erase_image)(
        void *context,
        firmware_slot_t slot,
        uint32_t payloadSize);
    platform_error_t (*read_header)(
        void *context,
        firmware_slot_t slot,
        uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE]);
    platform_error_t (*read_payload)(
        void *context,
        firmware_slot_t slot,
        uint32_t payloadOffset,
        uint8_t *data,
        uint32_t length);
    platform_error_t (*write_payload)(
        void *context,
        firmware_slot_t slot,
        uint32_t payloadOffset,
        const uint8_t *data,
        uint32_t length);
    platform_error_t (*write_header)(
        void *context,
        firmware_slot_t slot,
        const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE]);
    void *context;
    uint32_t payloadCapacity;
} firmware_storage_backend_t;

typedef struct
{
    firmware_storage_backend_t backend;
    platform_bool_t initialized;
} firmware_storage_t;

platform_error_t firmware_storage_init(
    firmware_storage_t *storage,
    const firmware_storage_backend_t *backend);

platform_error_t firmware_storage_get_payload_capacity(
    const firmware_storage_t *storage,
    uint32_t *payloadCapacity);

platform_error_t firmware_storage_read_header(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    firmware_image_header_t *header,
    firmware_image_validation_t *validation);

platform_error_t firmware_storage_validate_image(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    firmware_image_header_t *header,
    firmware_image_validation_t *validation);

platform_error_t firmware_storage_erase_slot(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    uint32_t payloadSize);

platform_error_t firmware_storage_write_payload(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    uint32_t payloadOffset,
    const uint8_t *data,
    uint32_t length);

platform_error_t firmware_storage_write_header(
    firmware_storage_t *storage,
    firmware_slot_t slot,
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE]);

#endif
