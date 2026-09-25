/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_metadata_store.c
 * @brief Firmware Metadata 双副本原子持久化实现
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#include <stddef.h>
#include <string.h>

#include "firmware_metadata_store.h"\n#include "platform_def.h"
#include "firmware_version.h"

#define FIRMWARE_METADATA_BODY_AND_CRC_SIZE   (0x7CU)
#define FIRMWARE_METADATA_MARKER_OFFSET       (0x7CU)
#define FIRMWARE_METADATA_MARKER_SIZE         (4U)

static void firmware_metadata_store_write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static platform_bool_t firmware_metadata_store_ready(
    const firmware_metadata_store_t *store)
{
    return ((store != NULL) &&
            (store->initialized == PLATFORM_TRUE) &&
            (store->backend.read != NULL) &&
            (store->backend.write != NULL)) ?
           PLATFORM_TRUE : PLATFORM_FALSE;
}

static platform_error_t firmware_metadata_store_read_copy(
    firmware_metadata_store_t *store,
    firmware_metadata_copy_id_t copy,
    uint8_t raw[FIRMWARE_METADATA_COPY_SIZE])
{
    return store->backend.read(
        store->backend.context,
        copy,
        0U,
        raw,
        FIRMWARE_METADATA_COPY_SIZE);
}

static platform_bool_t firmware_metadata_store_matches(
    const firmware_metadata_t *actual,
    const firmware_metadata_t *expected)
{
    if ((actual == NULL) || (expected == NULL)) {
        return PLATFORM_FALSE;
    }

    return ((actual->sequence == expected->sequence) &&
            (actual->confirmedSlot == expected->confirmedSlot) &&
            (actual->pendingSlot == expected->pendingSlot) &&
            (actual->slotAState == expected->slotAState) &&
            (actual->slotBState == expected->slotBState) &&
            (actual->upgradeState == expected->upgradeState) &&
            (firmware_version_compare(&actual->confirmedVersion,
                                      &expected->confirmedVersion) == 0)) ?
           PLATFORM_TRUE : PLATFORM_FALSE;
}

platform_error_t firmware_metadata_store_init(
    firmware_metadata_store_t *store,
    const firmware_metadata_backend_t *backend)
{
    if ((store == NULL) || (backend == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((backend->read == NULL) || (backend->write == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    store->backend = *backend;
    store->initialized = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

platform_error_t firmware_metadata_store_load(
    firmware_metadata_store_t *store,
    firmware_metadata_t *metadata,
    firmware_metadata_copy_id_t *selectedCopy)
{
    uint8_t copyA[FIRMWARE_METADATA_COPY_SIZE];
    uint8_t copyB[FIRMWARE_METADATA_COPY_SIZE];
    platform_error_t resultA;
    platform_error_t resultB;

    if ((metadata == NULL) || (selectedCopy == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (firmware_metadata_store_ready(store) == PLATFORM_FALSE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    (void)memset(copyA, 0xFF, sizeof(copyA));
    (void)memset(copyB, 0xFF, sizeof(copyB));

    resultA = firmware_metadata_store_read_copy(
        store, FIRMWARE_METADATA_COPY_A, copyA);
    resultB = firmware_metadata_store_read_copy(
        store, FIRMWARE_METADATA_COPY_B, copyB);

    if ((resultA != PLATFORM_ERR_OK) && (resultB != PLATFORM_ERR_OK)) {
        return PLATFORM_ERR_IO;
    }

    return firmware_metadata_select_latest(
        copyA,
        copyB,
        metadata,
        selectedCopy);
}

platform_error_t firmware_metadata_store_commit(
    firmware_metadata_store_t *store,
    const firmware_metadata_t *metadata,
    firmware_metadata_t *committedMetadata,
    firmware_metadata_copy_id_t *committedCopy)
{
    firmware_metadata_t latest;
    firmware_metadata_t target;
    firmware_metadata_t verified;
    firmware_metadata_copy_id_t selectedCopy = FIRMWARE_METADATA_COPY_NONE;
    firmware_metadata_copy_id_t targetCopy;
    firmware_metadata_copy_id_t verifiedCopy = FIRMWARE_METADATA_COPY_NONE;
    uint8_t raw[FIRMWARE_METADATA_COPY_SIZE];
    uint8_t verify[FIRMWARE_METADATA_COPY_SIZE];
    uint8_t invalidMarker[FIRMWARE_METADATA_MARKER_SIZE];
    uint8_t commitMarker[FIRMWARE_METADATA_MARKER_SIZE];
    platform_error_t result;

    if ((metadata == NULL) || (committedMetadata == NULL) ||
        (committedCopy == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (firmware_metadata_store_ready(store) == PLATFORM_FALSE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    target = *metadata;
    result = firmware_metadata_store_load(store, &latest, &selectedCopy);
    if (result == PLATFORM_ERR_OK) {
        target.sequence = latest.sequence + 1U;
        targetCopy = (selectedCopy == FIRMWARE_METADATA_COPY_A) ?
                     FIRMWARE_METADATA_COPY_B : FIRMWARE_METADATA_COPY_A;
    } else if (result == PLATFORM_ERR_NOT_FOUND) {
        targetCopy = FIRMWARE_METADATA_COPY_A;
    } else {
        return result;
    }

    result = firmware_metadata_encode_uncommitted(&target, raw);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    firmware_metadata_store_write_u32_le(
        invalidMarker,
        FIRMWARE_METADATA_INVALID_MARKER);
    firmware_metadata_store_write_u32_le(
        commitMarker,
        FIRMWARE_METADATA_COMMIT_MARKER);

    result = store->backend.write(
        store->backend.context,
        targetCopy,
        FIRMWARE_METADATA_MARKER_OFFSET,
        invalidMarker,
        sizeof(invalidMarker));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = store->backend.write(
        store->backend.context,
        targetCopy,
        0U,
        raw,
        FIRMWARE_METADATA_BODY_AND_CRC_SIZE);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = store->backend.read(
        store->backend.context,
        targetCopy,
        0U,
        verify,
        FIRMWARE_METADATA_BODY_AND_CRC_SIZE);
    if ((result != PLATFORM_ERR_OK) ||
        (memcmp(verify, raw, FIRMWARE_METADATA_BODY_AND_CRC_SIZE) != 0)) {
        return (result != PLATFORM_ERR_OK) ? result : PLATFORM_ERR_IO;
    }

    result = store->backend.write(
        store->backend.context,
        targetCopy,
        FIRMWARE_METADATA_MARKER_OFFSET,
        commitMarker,
        sizeof(commitMarker));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = store->backend.read(
        store->backend.context,
        targetCopy,
        FIRMWARE_METADATA_MARKER_OFFSET,
        verify,
        FIRMWARE_METADATA_MARKER_SIZE);
    if ((result != PLATFORM_ERR_OK) ||
        (memcmp(verify, commitMarker, FIRMWARE_METADATA_MARKER_SIZE) != 0)) {
        return (result != PLATFORM_ERR_OK) ? result : PLATFORM_ERR_IO;
    }

    result = firmware_metadata_store_load(
        store,
        &verified,
        &verifiedCopy);
    if ((result != PLATFORM_ERR_OK) ||
        (verifiedCopy != targetCopy) ||
        (firmware_metadata_store_matches(&verified, &target) == PLATFORM_FALSE)) {
        return (result != PLATFORM_ERR_OK) ? result : PLATFORM_ERR_IO;
    }

    *committedMetadata = verified;
    *committedCopy = verifiedCopy;
    return PLATFORM_ERR_OK;
}
