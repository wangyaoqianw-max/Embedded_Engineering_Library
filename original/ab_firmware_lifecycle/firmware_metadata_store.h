/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_metadata_store.h
 * @brief Firmware Metadata 双副本原子持久化抽象
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef FIRMWARE_METADATA_STORE_H
#define FIRMWARE_METADATA_STORE_H

#include "firmware_metadata.h"

#define FIRMWARE_METADATA_STORE_INITIALIZER   {0}

typedef struct
{
    platform_error_t (*read)(
        void *context,
        firmware_metadata_copy_id_t copy,
        uint32_t offset,
        uint8_t *data,
        uint32_t length);

    platform_error_t (*write)(
        void *context,
        firmware_metadata_copy_id_t copy,
        uint32_t offset,
        const uint8_t *data,
        uint32_t length);

    void *context;
} firmware_metadata_backend_t;

typedef struct
{
    firmware_metadata_backend_t backend;
    platform_bool_t initialized;
} firmware_metadata_store_t;

platform_error_t firmware_metadata_store_init(
    firmware_metadata_store_t *store,
    const firmware_metadata_backend_t *backend);

platform_error_t firmware_metadata_store_load(
    firmware_metadata_store_t *store,
    firmware_metadata_t *metadata,
    firmware_metadata_copy_id_t *selectedCopy);

platform_error_t firmware_metadata_store_commit(
    firmware_metadata_store_t *store,
    const firmware_metadata_t *metadata,
    firmware_metadata_t *committedMetadata,
    firmware_metadata_copy_id_t *committedCopy);

#endif
