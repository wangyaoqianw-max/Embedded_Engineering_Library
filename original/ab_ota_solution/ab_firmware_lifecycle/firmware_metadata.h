/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_metadata.h
 * @brief Firmware Metadata V2 固定格式公共接口
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef FIRMWARE_METADATA_H
#define FIRMWARE_METADATA_H

#include "firmware_lifecycle_def.h"

#define FIRMWARE_METADATA_COPY_SIZE           (128U)
#define FIRMWARE_METADATA_MAGIC               (0x444D5746UL)
#define FIRMWARE_METADATA_FORMAT_VERSION_V1   (1U)
#define FIRMWARE_METADATA_FORMAT_VERSION_V2   (2U)
#define FIRMWARE_METADATA_FORMAT_VERSION      FIRMWARE_METADATA_FORMAT_VERSION_V2
#define FIRMWARE_METADATA_COMMIT_MARKER       (0x54494D43UL)
#define FIRMWARE_METADATA_INVALID_MARKER      (0xFFFFFFFFUL)

typedef enum
{
    FIRMWARE_METADATA_COPY_NONE = 0,
    FIRMWARE_METADATA_COPY_A,
    FIRMWARE_METADATA_COPY_B
} firmware_metadata_copy_id_t;

platform_error_t firmware_metadata_encode_uncommitted(
    const firmware_metadata_t *metadata,
    uint8_t raw[FIRMWARE_METADATA_COPY_SIZE]);

platform_error_t firmware_metadata_decode_committed(
    const uint8_t raw[FIRMWARE_METADATA_COPY_SIZE],
    firmware_metadata_t *metadata);

platform_bool_t firmware_metadata_sequence_is_newer(
    uint32_t candidate,
    uint32_t reference);

platform_error_t firmware_metadata_select_latest(
    const uint8_t copyA[FIRMWARE_METADATA_COPY_SIZE],
    const uint8_t copyB[FIRMWARE_METADATA_COPY_SIZE],
    firmware_metadata_t *metadata,
    firmware_metadata_copy_id_t *selectedCopy);

#endif
