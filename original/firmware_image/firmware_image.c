/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file firmware_image.c
 * @brief Firmware Image Header V1 固定格式实现
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include <stddef.h>
#include <string.h>

#include "crc.h"
#include "firmware_image.h"
#include "firmware_version.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define FIRMWARE_IMAGE_OFFSET_MAGIC              (0x00U)
#define FIRMWARE_IMAGE_OFFSET_FORMAT_VERSION     (0x04U)
#define FIRMWARE_IMAGE_OFFSET_HEADER_SIZE        (0x06U)
#define FIRMWARE_IMAGE_OFFSET_VERSION_MAJOR      (0x08U)
#define FIRMWARE_IMAGE_OFFSET_VERSION_MINOR      (0x0AU)
#define FIRMWARE_IMAGE_OFFSET_VERSION_PATCH      (0x0CU)
#define FIRMWARE_IMAGE_OFFSET_VERSION_RESERVED   (0x0EU)
#define FIRMWARE_IMAGE_OFFSET_IMAGE_SIZE         (0x10U)
#define FIRMWARE_IMAGE_OFFSET_PAYLOAD_CRC32      (0x14U)
#define FIRMWARE_IMAGE_OFFSET_RESERVED           (0x18U)
#define FIRMWARE_IMAGE_RESERVED_SIZE              (36U)
#define FIRMWARE_IMAGE_OFFSET_HEADER_CRC32       (0x3CU)
//******************************** Defines **********************************//

static uint16_t firmware_image_read_u16_le(const uint8_t *data)
{
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

static uint32_t firmware_image_read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static void firmware_image_write_u16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
}

static void firmware_image_write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static platform_bool_t firmware_image_is_erased(const uint8_t *rawHeader)
{
    uint32_t index;

    for (index = 0U; index < FIRMWARE_IMAGE_HEADER_SIZE; index++) {
        if (rawHeader[index] != 0xFFU) {
            return (platform_bool_t)0U;
        }
    }

    return (platform_bool_t)1U;
}

static platform_bool_t firmware_image_reserved_is_zero(const uint8_t *rawHeader)
{
    uint32_t index;

    for (index = 0U; index < FIRMWARE_IMAGE_RESERVED_SIZE; index++) {
        if (rawHeader[FIRMWARE_IMAGE_OFFSET_RESERVED + index] != 0U) {
            return (platform_bool_t)0U;
        }
    }

    return (platform_bool_t)1U;
}

platform_error_t firmware_image_decode_header(
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE],
    firmware_image_header_t *header)
{
    if ((rawHeader == NULL) || (header == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    header->magic = firmware_image_read_u32_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_MAGIC]);
    header->formatVersion = firmware_image_read_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_FORMAT_VERSION]);
    header->headerSize = firmware_image_read_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_HEADER_SIZE]);
    header->version.major = firmware_image_read_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_VERSION_MAJOR]);
    header->version.minor = firmware_image_read_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_VERSION_MINOR]);
    header->version.patch = firmware_image_read_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_VERSION_PATCH]);
    header->version.reserved = firmware_image_read_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_VERSION_RESERVED]);
    header->imageSize = firmware_image_read_u32_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_IMAGE_SIZE]);
    header->payloadCrc32 = firmware_image_read_u32_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_PAYLOAD_CRC32]);
    (void)memcpy(header->reserved, &rawHeader[FIRMWARE_IMAGE_OFFSET_RESERVED], FIRMWARE_IMAGE_RESERVED_SIZE);
    header->headerCrc32 = firmware_image_read_u32_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_HEADER_CRC32]);

    return PLATFORM_ERR_OK;
}

platform_error_t firmware_image_encode_header(
    const firmware_image_header_t *header,
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE])
{
    uint32_t headerCrc32;

    if ((header == NULL) || (rawHeader == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((firmware_version_is_valid(&header->version) == 0U) ||
        (header->imageSize == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memset(rawHeader, 0, FIRMWARE_IMAGE_HEADER_SIZE);
    firmware_image_write_u32_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_MAGIC], FIRMWARE_IMAGE_MAGIC);
    firmware_image_write_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_FORMAT_VERSION], FIRMWARE_IMAGE_FORMAT_VERSION);
    firmware_image_write_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_HEADER_SIZE], FIRMWARE_IMAGE_HEADER_SIZE);
    firmware_image_write_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_VERSION_MAJOR], header->version.major);
    firmware_image_write_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_VERSION_MINOR], header->version.minor);
    firmware_image_write_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_VERSION_PATCH], header->version.patch);
    firmware_image_write_u16_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_VERSION_RESERVED], 0U);
    firmware_image_write_u32_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_IMAGE_SIZE], header->imageSize);
    firmware_image_write_u32_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_PAYLOAD_CRC32], header->payloadCrc32);
    headerCrc32 = crc32_iso_hdlc_calculate(rawHeader, FIRMWARE_IMAGE_OFFSET_HEADER_CRC32);
    firmware_image_write_u32_le(&rawHeader[FIRMWARE_IMAGE_OFFSET_HEADER_CRC32], headerCrc32);

    return PLATFORM_ERR_OK;
}

firmware_image_validation_t firmware_image_validate_header(
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE],
    firmware_image_header_t *decodedHeader)
{
    firmware_image_header_t header;
    uint32_t headerCrc32;

    if (rawHeader == NULL) {
        return FIRMWARE_IMAGE_VALIDATION_UNKNOWN;
    }

    if (firmware_image_is_erased(rawHeader) != 0U) {
        return FIRMWARE_IMAGE_VALIDATION_EMPTY;
    }

    if (firmware_image_decode_header(rawHeader, &header) != PLATFORM_ERR_OK) {
        return FIRMWARE_IMAGE_VALIDATION_UNKNOWN;
    }

    if (header.magic != FIRMWARE_IMAGE_MAGIC) {
        return FIRMWARE_IMAGE_VALIDATION_INVALID_MAGIC;
    }

    if (header.formatVersion != FIRMWARE_IMAGE_FORMAT_VERSION) {
        return FIRMWARE_IMAGE_VALIDATION_INVALID_FORMAT_VERSION;
    }

    if (header.headerSize != FIRMWARE_IMAGE_HEADER_SIZE) {
        return FIRMWARE_IMAGE_VALIDATION_INVALID_HEADER_SIZE;
    }

    if (firmware_image_reserved_is_zero(rawHeader) == 0U) {
        return FIRMWARE_IMAGE_VALIDATION_INVALID_RESERVED;
    }

    if (firmware_version_is_valid(&header.version) == 0U) {
        return FIRMWARE_IMAGE_VALIDATION_INVALID_VERSION;
    }

    if (header.imageSize == 0U) {
        return FIRMWARE_IMAGE_VALIDATION_INVALID_SIZE;
    }

    headerCrc32 = crc32_iso_hdlc_calculate(rawHeader, FIRMWARE_IMAGE_OFFSET_HEADER_CRC32);
    if (header.headerCrc32 != headerCrc32) {
        return FIRMWARE_IMAGE_VALIDATION_INVALID_HEADER_CRC;
    }

    if (decodedHeader != NULL) {
        *decodedHeader = header;
    }

    return FIRMWARE_IMAGE_VALIDATION_VALID;
}
