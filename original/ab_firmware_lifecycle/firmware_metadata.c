/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file firmware_metadata.c
 * @brief A/B Firmware Metadata V1/V2 固定格式实现
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include <stddef.h>
#include <string.h>

#include "crc.h"
#include "firmware_metadata.h"
#include "firmware_version.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define FIRMWARE_METADATA_OFFSET_MAGIC             (0x00U)
#define FIRMWARE_METADATA_OFFSET_FORMAT_VERSION    (0x04U)
#define FIRMWARE_METADATA_OFFSET_SIZE              (0x06U)
#define FIRMWARE_METADATA_OFFSET_SEQUENCE          (0x08U)
#define FIRMWARE_METADATA_OFFSET_V2_RESERVED       (0x0CU)
#define FIRMWARE_METADATA_OFFSET_CONFIRMED_SLOT    (0x0DU)
#define FIRMWARE_METADATA_OFFSET_SLOT_A_STATE      (0x0EU)
#define FIRMWARE_METADATA_OFFSET_SLOT_B_STATE      (0x0FU)
#define FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION (0x10U)
#define FIRMWARE_METADATA_OFFSET_PENDING_SLOT      (0x18U)
#define FIRMWARE_METADATA_OFFSET_UPGRADE_STATE     (0x19U)
#define FIRMWARE_METADATA_OFFSET_CRC32             (0x78U)
#define FIRMWARE_METADATA_OFFSET_COMMIT_MARKER     (0x7CU)
#define FIRMWARE_METADATA_BODY_SIZE                (0x78U)
#define FIRMWARE_METADATA_V2_RESERVED_OFFSET       (0x1AU)
#define FIRMWARE_METADATA_V1_RESERVED_OFFSET       (0x18U)
//******************************** Defines **********************************//

//******************************** Declaring *******************************//
static uint16_t firmware_metadata_read_u16_le(const uint8_t *data);
static uint32_t firmware_metadata_read_u32_le(const uint8_t *data);
static void firmware_metadata_write_u16_le(uint8_t *data, uint16_t value);
static void firmware_metadata_write_u32_le(uint8_t *data, uint32_t value);
static platform_bool_t firmware_metadata_slot_is_valid(firmware_slot_t slot);
static platform_bool_t firmware_metadata_slot_state_is_valid(firmware_slot_state_t state);
static platform_bool_t firmware_metadata_upgrade_state_is_valid(
    firmware_upgrade_state_t state);
static platform_bool_t firmware_metadata_reserved_is_zero(
    const uint8_t raw[FIRMWARE_METADATA_COPY_SIZE],
    uint16_t formatVersion);
static platform_bool_t firmware_metadata_fields_are_valid(
    const firmware_metadata_t *metadata);
static void firmware_metadata_decode_fields(
    const uint8_t raw[FIRMWARE_METADATA_COPY_SIZE],
    uint16_t formatVersion,
    firmware_metadata_t *metadata);
//******************************** Declaring *******************************//

//******************************** Private Functions ************************//
static uint16_t firmware_metadata_read_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

static uint32_t firmware_metadata_read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static void firmware_metadata_write_u16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
}

static void firmware_metadata_write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static platform_bool_t firmware_metadata_slot_is_valid(firmware_slot_t slot)
{
    return ((slot == FIRMWARE_SLOT_A) || (slot == FIRMWARE_SLOT_B) ||
            (slot == FIRMWARE_SLOT_NONE)) ? (platform_bool_t)1U : (platform_bool_t)0U;
}

static platform_bool_t firmware_metadata_slot_state_is_valid(firmware_slot_state_t state)
{
    return (state <= FIRMWARE_SLOT_STATE_INVALID) ? (platform_bool_t)1U : (platform_bool_t)0U;
}

static platform_bool_t firmware_metadata_upgrade_state_is_valid(firmware_upgrade_state_t state)
{
    return (state <= FIRMWARE_UPGRADE_STATE_ROLLBACK) ? (platform_bool_t)1U : (platform_bool_t)0U;
}

/**
 * @brief 检查 Metadata 保留区是否保持全零
 * @note 保留区属于 Binary Contract；即使 CRC 正确，非零值也不能被当前版本接受。
 */
static platform_bool_t firmware_metadata_reserved_is_zero(
    const uint8_t raw[FIRMWARE_METADATA_COPY_SIZE],
    uint16_t formatVersion)
{
    uint32_t index;

    if ((formatVersion == FIRMWARE_METADATA_FORMAT_VERSION_V2) &&
        (raw[FIRMWARE_METADATA_OFFSET_V2_RESERVED] != 0U)) {
        return (platform_bool_t)0U;
    }

    index = (formatVersion == FIRMWARE_METADATA_FORMAT_VERSION_V1) ?
            FIRMWARE_METADATA_V1_RESERVED_OFFSET : FIRMWARE_METADATA_V2_RESERVED_OFFSET;
    for (; index < FIRMWARE_METADATA_OFFSET_CRC32; index++) {
        if (raw[index] != 0U) {
            return (platform_bool_t)0U;
        }
    }

    return (platform_bool_t)1U;
}

/**
 * @brief 聚合校验 Metadata 的可表示字段
 * @note 该检查被编码和已提交副本解码共用，避免两条路径接受不同的 Slot 或 Version 范围。
 */
static platform_bool_t firmware_metadata_fields_are_valid(const firmware_metadata_t *metadata)
{
    if (metadata == NULL) {
        return (platform_bool_t)0U;
    }

    if ((firmware_metadata_slot_is_valid(metadata->confirmedSlot) == 0U) ||
        (firmware_metadata_slot_is_valid(metadata->pendingSlot) == 0U) ||
        (firmware_metadata_slot_state_is_valid(metadata->slotAState) == 0U) ||
        (firmware_metadata_slot_state_is_valid(metadata->slotBState) == 0U) ||
        (firmware_metadata_upgrade_state_is_valid(metadata->upgradeState) == 0U) ||
        (firmware_version_is_valid(&metadata->confirmedVersion) == 0U)) {
        return (platform_bool_t)0U;
    }

    if (((metadata->upgradeState == FIRMWARE_UPGRADE_STATE_NONE) &&
         (metadata->pendingSlot != FIRMWARE_SLOT_NONE)) ||
        ((metadata->upgradeState != FIRMWARE_UPGRADE_STATE_NONE) &&
         (metadata->pendingSlot == FIRMWARE_SLOT_NONE))) {
        return (platform_bool_t)0U;
    }

    if (((metadata->confirmedSlot == FIRMWARE_SLOT_A) &&
         (metadata->slotAState != FIRMWARE_SLOT_STATE_VALID)) ||
        ((metadata->confirmedSlot == FIRMWARE_SLOT_B) &&
         (metadata->slotBState != FIRMWARE_SLOT_STATE_VALID))) {
        return (platform_bool_t)0U;
    }

    if ((metadata->upgradeState != FIRMWARE_UPGRADE_STATE_NONE) &&
        ((metadata->confirmedSlot == FIRMWARE_SLOT_NONE) ||
         (metadata->pendingSlot == FIRMWARE_SLOT_NONE) ||
         (metadata->confirmedSlot == metadata->pendingSlot))) {
        return (platform_bool_t)0U;
    }

    if (((metadata->pendingSlot == FIRMWARE_SLOT_A) &&
         (metadata->slotAState != FIRMWARE_SLOT_STATE_VALID)) ||
        ((metadata->pendingSlot == FIRMWARE_SLOT_B) &&
         (metadata->slotBState != FIRMWARE_SLOT_STATE_VALID))) {
        return (platform_bool_t)0U;
    }

    return (platform_bool_t)1U;
}

/**
 * @brief 将已完成格式前置检查的 raw Body 解码为内存表达
 * @note 调用者必须先验证 magic、format、size、commit marker 与 reserved；本函数不做 I/O 或 CRC。
 */
static void firmware_metadata_decode_fields(
    const uint8_t raw[FIRMWARE_METADATA_COPY_SIZE],
    uint16_t formatVersion,
    firmware_metadata_t *metadata)
{
    metadata->sequence = firmware_metadata_read_u32_le(&raw[FIRMWARE_METADATA_OFFSET_SEQUENCE]);
    metadata->confirmedSlot = (firmware_slot_t)raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_SLOT];
    metadata->slotAState = (firmware_slot_state_t)raw[FIRMWARE_METADATA_OFFSET_SLOT_A_STATE];
    metadata->slotBState = (firmware_slot_state_t)raw[FIRMWARE_METADATA_OFFSET_SLOT_B_STATE];
    metadata->confirmedVersion.major = firmware_metadata_read_u16_le(
        &raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION]);
    metadata->confirmedVersion.minor = firmware_metadata_read_u16_le(
        &raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION + 2U]);
    metadata->confirmedVersion.patch = firmware_metadata_read_u16_le(
        &raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION + 4U]);
    metadata->confirmedVersion.reserved = firmware_metadata_read_u16_le(
        &raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION + 6U]);
    metadata->pendingSlot = FIRMWARE_SLOT_NONE;
    metadata->upgradeState = FIRMWARE_UPGRADE_STATE_NONE;

    if (formatVersion == FIRMWARE_METADATA_FORMAT_VERSION_V2) {
        metadata->pendingSlot = (firmware_slot_t)raw[FIRMWARE_METADATA_OFFSET_PENDING_SLOT];
        metadata->upgradeState = (firmware_upgrade_state_t)raw[FIRMWARE_METADATA_OFFSET_UPGRADE_STATE];
    }
}
//******************************** Public Functions *************************//
platform_error_t firmware_metadata_encode_uncommitted(
    const firmware_metadata_t *metadata,
    uint8_t raw[FIRMWARE_METADATA_COPY_SIZE])
{
    uint32_t metadataCrc32;

    if ((metadata == NULL) || (raw == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (firmware_metadata_fields_are_valid(metadata) == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memset(raw, 0, FIRMWARE_METADATA_COPY_SIZE);
    firmware_metadata_write_u32_le(&raw[FIRMWARE_METADATA_OFFSET_MAGIC], FIRMWARE_METADATA_MAGIC);
    firmware_metadata_write_u16_le(&raw[FIRMWARE_METADATA_OFFSET_FORMAT_VERSION],
                                   FIRMWARE_METADATA_FORMAT_VERSION_V2);
    firmware_metadata_write_u16_le(&raw[FIRMWARE_METADATA_OFFSET_SIZE], FIRMWARE_METADATA_COPY_SIZE);
    firmware_metadata_write_u32_le(&raw[FIRMWARE_METADATA_OFFSET_SEQUENCE], metadata->sequence);
    raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_SLOT] = (uint8_t)metadata->confirmedSlot;
    raw[FIRMWARE_METADATA_OFFSET_SLOT_A_STATE] = (uint8_t)metadata->slotAState;
    raw[FIRMWARE_METADATA_OFFSET_SLOT_B_STATE] = (uint8_t)metadata->slotBState;
    firmware_metadata_write_u16_le(&raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION], metadata->confirmedVersion.major);
    firmware_metadata_write_u16_le(
        &raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION + 2U],
        metadata->confirmedVersion.minor);
    firmware_metadata_write_u16_le(
        &raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION + 4U],
        metadata->confirmedVersion.patch);
    firmware_metadata_write_u16_le(&raw[FIRMWARE_METADATA_OFFSET_CONFIRMED_VERSION + 6U], 0U);
    raw[FIRMWARE_METADATA_OFFSET_PENDING_SLOT] = (uint8_t)metadata->pendingSlot;
    raw[FIRMWARE_METADATA_OFFSET_UPGRADE_STATE] = (uint8_t)metadata->upgradeState;
    metadataCrc32 = crc32_iso_hdlc_calculate(raw, FIRMWARE_METADATA_BODY_SIZE);
    firmware_metadata_write_u32_le(&raw[FIRMWARE_METADATA_OFFSET_CRC32], metadataCrc32);
    firmware_metadata_write_u32_le(&raw[FIRMWARE_METADATA_OFFSET_COMMIT_MARKER], FIRMWARE_METADATA_INVALID_MARKER);

    return PLATFORM_ERR_OK;
}

platform_error_t firmware_metadata_decode_committed(
    const uint8_t raw[FIRMWARE_METADATA_COPY_SIZE],
    firmware_metadata_t *metadata)
{
    firmware_metadata_t decodedMetadata;
    uint16_t formatVersion;
    uint32_t expectedCrc32;

    if ((raw == NULL) || (metadata == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (firmware_metadata_read_u32_le(&raw[FIRMWARE_METADATA_OFFSET_MAGIC]) != FIRMWARE_METADATA_MAGIC) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    formatVersion = firmware_metadata_read_u16_le(&raw[FIRMWARE_METADATA_OFFSET_FORMAT_VERSION]);
    if ((formatVersion != FIRMWARE_METADATA_FORMAT_VERSION_V1) &&
        (formatVersion != FIRMWARE_METADATA_FORMAT_VERSION_V2)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (firmware_metadata_read_u16_le(&raw[FIRMWARE_METADATA_OFFSET_SIZE]) != FIRMWARE_METADATA_COPY_SIZE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (firmware_metadata_read_u32_le(&raw[FIRMWARE_METADATA_OFFSET_COMMIT_MARKER]) !=
        FIRMWARE_METADATA_COMMIT_MARKER) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (firmware_metadata_reserved_is_zero(
            raw, formatVersion) == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    firmware_metadata_decode_fields(
        raw, formatVersion, &decodedMetadata);
    if (firmware_metadata_fields_are_valid(&decodedMetadata) == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    expectedCrc32 = crc32_iso_hdlc_calculate(raw, FIRMWARE_METADATA_BODY_SIZE);
    if (firmware_metadata_read_u32_le(&raw[FIRMWARE_METADATA_OFFSET_CRC32]) != expectedCrc32) {
        return PLATFORM_ERR_CHECKSUM;
    }

    *metadata = decodedMetadata;
    return PLATFORM_ERR_OK;
}

platform_bool_t firmware_metadata_sequence_is_newer(uint32_t candidate, uint32_t reference)
{
    uint32_t difference = candidate - reference;

    return ((difference != 0U) && (difference < 0x80000000UL)) ?
           (platform_bool_t)1U : (platform_bool_t)0U;
}

platform_error_t firmware_metadata_select_latest(
    const uint8_t copyA[FIRMWARE_METADATA_COPY_SIZE],
    const uint8_t copyB[FIRMWARE_METADATA_COPY_SIZE],
    firmware_metadata_t *metadata,
    firmware_metadata_copy_id_t *selectedCopy)
{
    firmware_metadata_t metadataA;
    firmware_metadata_t metadataB;
    platform_error_t resultA;
    platform_error_t resultB;

    if ((copyA == NULL) || (copyB == NULL) || (metadata == NULL) || (selectedCopy == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    resultA = firmware_metadata_decode_committed(copyA, &metadataA);
    resultB = firmware_metadata_decode_committed(copyB, &metadataB);

    if ((resultA != PLATFORM_ERR_OK) && (resultB != PLATFORM_ERR_OK)) {
        return PLATFORM_ERR_NOT_FOUND;
    }

    if ((resultA == PLATFORM_ERR_OK) && (resultB != PLATFORM_ERR_OK)) {
        *metadata = metadataA;
        *selectedCopy = FIRMWARE_METADATA_COPY_A;
        return PLATFORM_ERR_OK;
    }

    if ((resultA != PLATFORM_ERR_OK) && (resultB == PLATFORM_ERR_OK)) {
        *metadata = metadataB;
        *selectedCopy = FIRMWARE_METADATA_COPY_B;
        return PLATFORM_ERR_OK;
    }

    if (firmware_metadata_sequence_is_newer(metadataB.sequence, metadataA.sequence) != 0U) {
        *metadata = metadataB;
        *selectedCopy = FIRMWARE_METADATA_COPY_B;
    } else {
        *metadata = metadataA;
        *selectedCopy = FIRMWARE_METADATA_COPY_A;
    }

    return PLATFORM_ERR_OK;
}
