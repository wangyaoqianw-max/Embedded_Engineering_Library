/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_metadata_at24c02_backend.h
 * @brief AT24C02 到 Firmware Metadata Backend 的适配器
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef FIRMWARE_METADATA_AT24C02_BACKEND_H
#define FIRMWARE_METADATA_AT24C02_BACKEND_H

#include "firmware_metadata_store.h"
#include "platform_at24c02.h"

typedef struct
{
    platform_at24c02_t *eeprom;
} firmware_metadata_at24c02_backend_t;

platform_error_t firmware_metadata_at24c02_backend_init(
    firmware_metadata_at24c02_backend_t *adapter,
    platform_at24c02_t *eeprom,
    firmware_metadata_backend_t *backend);

#endif
