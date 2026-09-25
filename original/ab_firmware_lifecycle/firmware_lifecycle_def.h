/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_lifecycle_def.h
 * @brief A/B Firmware Lifecycle 公共数据合同
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef FIRMWARE_LIFECYCLE_DEF_H
#define FIRMWARE_LIFECYCLE_DEF_H

#include "firmware_storage.h"
#include "firmware_version.h"

typedef enum
{
    FIRMWARE_SLOT_STATE_EMPTY = 0,
    FIRMWARE_SLOT_STATE_VALID,
    FIRMWARE_SLOT_STATE_INVALID
} firmware_slot_state_t;

typedef enum
{
    FIRMWARE_UPGRADE_STATE_NONE = 0,
    FIRMWARE_UPGRADE_STATE_PENDING,
    FIRMWARE_UPGRADE_STATE_TRIAL,
    FIRMWARE_UPGRADE_STATE_ROLLBACK
} firmware_upgrade_state_t;

typedef struct
{
    uint32_t sequence;
    firmware_slot_t confirmedSlot;
    firmware_slot_t pendingSlot;
    firmware_slot_state_t slotAState;
    firmware_slot_state_t slotBState;
    firmware_upgrade_state_t upgradeState;
    firmware_version_t confirmedVersion;
} firmware_metadata_t;

#endif
