/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_def.h
 * @brief 通用 Firmware Image Header V1 固定数据合同
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.1
 *****************************************************************************/

#ifndef FIRMWARE_DEF_H
#define FIRMWARE_DEF_H

#include "platform_error.h"

#define FIRMWARE_IMAGE_HEADER_SIZE           (64U)
#define FIRMWARE_IMAGE_MAGIC                 (0x4D495746UL)
#define FIRMWARE_IMAGE_FORMAT_VERSION        (1U)

typedef struct
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint16_t reserved;
} firmware_version_t;

typedef struct
{
    uint32_t magic;
    uint16_t formatVersion;
    uint16_t headerSize;
    firmware_version_t version;
    uint32_t imageSize;
    uint32_t payloadCrc32;
    uint8_t reserved[36];
    uint32_t headerCrc32;
} firmware_image_header_t;

typedef enum
{
    FIRMWARE_IMAGE_VALIDATION_UNKNOWN = 0,
    FIRMWARE_IMAGE_VALIDATION_EMPTY,
    FIRMWARE_IMAGE_VALIDATION_VALID,
    FIRMWARE_IMAGE_VALIDATION_INVALID_MAGIC,
    FIRMWARE_IMAGE_VALIDATION_INVALID_FORMAT_VERSION,
    FIRMWARE_IMAGE_VALIDATION_INVALID_HEADER_SIZE,
    FIRMWARE_IMAGE_VALIDATION_INVALID_RESERVED,
    FIRMWARE_IMAGE_VALIDATION_INVALID_VERSION,
    FIRMWARE_IMAGE_VALIDATION_INVALID_SIZE,
    FIRMWARE_IMAGE_VALIDATION_INVALID_HEADER_CRC,
    FIRMWARE_IMAGE_VALIDATION_INVALID_PAYLOAD_CRC
} firmware_image_validation_t;

#endif
