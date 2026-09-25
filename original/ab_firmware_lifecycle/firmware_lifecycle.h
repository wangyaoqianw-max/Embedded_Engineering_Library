/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_lifecycle.h
 * @brief A/B Firmware Lifecycle 状态迁移接口
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef FIRMWARE_LIFECYCLE_H
#define FIRMWARE_LIFECYCLE_H

#include "firmware_metadata_store.h"
#include "firmware_storage.h"

platform_error_t firmware_lifecycle_request_upgrade(
    firmware_metadata_store_t *metadataStore,
    firmware_slot_t targetSlot);

platform_error_t firmware_lifecycle_mark_trial(
    firmware_metadata_store_t *metadataStore,
    firmware_slot_t installedSlot);

platform_error_t firmware_lifecycle_begin_rollback(
    firmware_metadata_store_t *metadataStore);

platform_error_t firmware_lifecycle_complete_rollback(
    firmware_metadata_store_t *metadataStore);

platform_error_t firmware_lifecycle_confirm(
    firmware_metadata_store_t *metadataStore,
    firmware_storage_t *firmwareStorage);

#endif
