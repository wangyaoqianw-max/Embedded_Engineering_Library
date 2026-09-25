/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_image_sink.h
 * @brief YMODEM 到 Firmware Storage 的镜像接收适配器
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef FIRMWARE_IMAGE_SINK_H
#define FIRMWARE_IMAGE_SINK_H

#include "firmware_storage.h"
#include "ymodem_sink.h"

#define FIRMWARE_IMAGE_SINK_INITIALIZER       {0}

typedef struct
{
    firmware_storage_t *storage;
    firmware_slot_t slot;
    uint32_t expectedFileSize;
    uint32_t receivedFileBytes;
    uint8_t headerBuffer[FIRMWARE_IMAGE_HEADER_SIZE];
    uint32_t headerFillCount;
    firmware_image_header_t header;
    uint32_t payloadWritten;
    platform_bool_t headerValidated;
    platform_bool_t headerCommitted;
    platform_bool_t started;
    platform_bool_t failed;
    platform_error_t lastError;
} firmware_image_sink_t;

platform_error_t firmware_image_sink_init(
    firmware_image_sink_t *sink,
    firmware_storage_t *storage);

platform_error_t firmware_image_sink_set_target_slot(
    firmware_image_sink_t *sink,
    firmware_slot_t slot);

platform_error_t firmware_image_sink_get_contract(
    firmware_image_sink_t *sink,
    ymodem_sink_t *contract);

#endif
