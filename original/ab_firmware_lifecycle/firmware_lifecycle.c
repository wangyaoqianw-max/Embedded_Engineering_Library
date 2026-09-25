/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_lifecycle.c
 * @brief A/B Firmware Lifecycle 状态迁移实现
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#include <stddef.h>

#include "firmware_lifecycle.h"\n#include "platform_def.h"
#include "firmware_version.h"

static platform_bool_t firmware_lifecycle_slot_is_ab(firmware_slot_t slot)
{
    return ((slot == FIRMWARE_SLOT_A) || (slot == FIRMWARE_SLOT_B)) ?
           PLATFORM_TRUE : PLATFORM_FALSE;
}

static firmware_slot_state_t firmware_lifecycle_get_slot_state(
    const firmware_metadata_t *metadata,
    firmware_slot_t slot)
{
    if (slot == FIRMWARE_SLOT_A) {
        return metadata->slotAState;
    }

    if (slot == FIRMWARE_SLOT_B) {
        return metadata->slotBState;
    }

    return FIRMWARE_SLOT_STATE_INVALID;
}

static platform_error_t firmware_lifecycle_load(
    firmware_metadata_store_t *store,
    firmware_metadata_t *metadata)
{
    firmware_metadata_copy_id_t selectedCopy;

    return firmware_metadata_store_load(
        store,
        metadata,
        &selectedCopy);
}

static platform_error_t firmware_lifecycle_commit(
    firmware_metadata_store_t *store,
    const firmware_metadata_t *metadata)
{
    firmware_metadata_t committed;
    firmware_metadata_copy_id_t committedCopy;

    return firmware_metadata_store_commit(
        store,
        metadata,
        &committed,
        &committedCopy);
}

platform_error_t firmware_lifecycle_request_upgrade(
    firmware_metadata_store_t *metadataStore,
    firmware_slot_t targetSlot)
{
    firmware_metadata_t metadata;
    platform_error_t result;

    if (metadataStore == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (firmware_lifecycle_slot_is_ab(targetSlot) == PLATFORM_FALSE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = firmware_lifecycle_load(metadataStore, &metadata);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((metadata.upgradeState != FIRMWARE_UPGRADE_STATE_NONE) ||
        (metadata.pendingSlot != FIRMWARE_SLOT_NONE) ||
        (metadata.confirmedSlot == targetSlot) ||
        (firmware_lifecycle_get_slot_state(&metadata, targetSlot) !=
         FIRMWARE_SLOT_STATE_VALID)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    metadata.pendingSlot = targetSlot;
    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_PENDING;
    return firmware_lifecycle_commit(metadataStore, &metadata);
}

platform_error_t firmware_lifecycle_mark_trial(
    firmware_metadata_store_t *metadataStore,
    firmware_slot_t installedSlot)
{
    firmware_metadata_t metadata;
    platform_error_t result;

    if (metadataStore == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (firmware_lifecycle_slot_is_ab(installedSlot) == PLATFORM_FALSE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = firmware_lifecycle_load(metadataStore, &metadata);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((metadata.upgradeState != FIRMWARE_UPGRADE_STATE_PENDING) ||
        (metadata.pendingSlot != installedSlot) ||
        (metadata.confirmedSlot == installedSlot) ||
        (firmware_lifecycle_get_slot_state(&metadata, installedSlot) !=
         FIRMWARE_SLOT_STATE_VALID)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_TRIAL;
    return firmware_lifecycle_commit(metadataStore, &metadata);
}

platform_error_t firmware_lifecycle_begin_rollback(
    firmware_metadata_store_t *metadataStore)
{
    firmware_metadata_t metadata;
    platform_error_t result;

    if (metadataStore == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = firmware_lifecycle_load(metadataStore, &metadata);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((metadata.upgradeState != FIRMWARE_UPGRADE_STATE_TRIAL) ||
        (firmware_lifecycle_slot_is_ab(metadata.confirmedSlot) == PLATFORM_FALSE) ||
        (firmware_lifecycle_slot_is_ab(metadata.pendingSlot) == PLATFORM_FALSE) ||
        (metadata.confirmedSlot == metadata.pendingSlot)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_ROLLBACK;
    return firmware_lifecycle_commit(metadataStore, &metadata);
}

platform_error_t firmware_lifecycle_complete_rollback(
    firmware_metadata_store_t *metadataStore)
{
    firmware_metadata_t metadata;
    platform_error_t result;

    if (metadataStore == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = firmware_lifecycle_load(metadataStore, &metadata);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((metadata.upgradeState != FIRMWARE_UPGRADE_STATE_ROLLBACK) ||
        (firmware_lifecycle_slot_is_ab(metadata.confirmedSlot) == PLATFORM_FALSE) ||
        (firmware_lifecycle_slot_is_ab(metadata.pendingSlot) == PLATFORM_FALSE) ||
        (metadata.confirmedSlot == metadata.pendingSlot)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    metadata.pendingSlot = FIRMWARE_SLOT_NONE;
    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_NONE;
    return firmware_lifecycle_commit(metadataStore, &metadata);
}

platform_error_t firmware_lifecycle_confirm(
    firmware_metadata_store_t *metadataStore,
    firmware_storage_t *firmwareStorage)
{
    firmware_metadata_t metadata;
    firmware_image_header_t header;
    firmware_image_validation_t validation;
    platform_error_t result;

    if ((metadataStore == NULL) || (firmwareStorage == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = firmware_lifecycle_load(metadataStore, &metadata);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((metadata.upgradeState != FIRMWARE_UPGRADE_STATE_TRIAL) ||
        (firmware_lifecycle_slot_is_ab(metadata.confirmedSlot) == PLATFORM_FALSE) ||
        (firmware_lifecycle_slot_is_ab(metadata.pendingSlot) == PLATFORM_FALSE) ||
        (metadata.confirmedSlot == metadata.pendingSlot) ||
        (firmware_lifecycle_get_slot_state(&metadata, metadata.pendingSlot) !=
         FIRMWARE_SLOT_STATE_VALID)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    validation = FIRMWARE_IMAGE_VALIDATION_UNKNOWN;
    result = firmware_storage_validate_image(
        firmwareStorage,
        metadata.pendingSlot,
        &header,
        &validation);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (validation == FIRMWARE_IMAGE_VALIDATION_INVALID_PAYLOAD_CRC) {
        return PLATFORM_ERR_CHECKSUM;
    }

    if (validation == FIRMWARE_IMAGE_VALIDATION_INVALID_SIZE) {
        return PLATFORM_ERR_OVERFLOW;
    }

    if ((validation != FIRMWARE_IMAGE_VALIDATION_VALID) ||
        (firmware_version_is_valid(&header.version) == PLATFORM_FALSE)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    metadata.confirmedSlot = metadata.pendingSlot;
    metadata.confirmedVersion = header.version;
    metadata.pendingSlot = FIRMWARE_SLOT_NONE;
    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_NONE;

    return firmware_lifecycle_commit(metadataStore, &metadata);
}
