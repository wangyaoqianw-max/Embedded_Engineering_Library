/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_boot_decision.c
 * @brief A/B Firmware Lifecycle 启动动作决策实现
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#include <stddef.h>

#include "firmware_boot_decision.h"\n#include "platform_def.h"

static platform_bool_t firmware_boot_decision_slot_is_ab(firmware_slot_t slot)
{
    return ((slot == FIRMWARE_SLOT_A) || (slot == FIRMWARE_SLOT_B)) ?
           PLATFORM_TRUE : PLATFORM_FALSE;
}

platform_error_t firmware_boot_decision_get_action(
    const firmware_metadata_t *metadata,
    firmware_boot_action_t *action)
{
    if ((metadata == NULL) || (action == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    switch (metadata->upgradeState) {
        case FIRMWARE_UPGRADE_STATE_NONE:
            if (metadata->pendingSlot != FIRMWARE_SLOT_NONE) {
                return PLATFORM_ERR_INVALID_STATE;
            }
            *action = FIRMWARE_BOOT_ACTION_BOOT_CURRENT;
            return PLATFORM_ERR_OK;

        case FIRMWARE_UPGRADE_STATE_PENDING:
            if ((firmware_boot_decision_slot_is_ab(metadata->pendingSlot) == PLATFORM_FALSE) ||
                (metadata->pendingSlot == metadata->confirmedSlot)) {
                return PLATFORM_ERR_INVALID_STATE;
            }
            *action = FIRMWARE_BOOT_ACTION_INSTALL_PENDING;
            return PLATFORM_ERR_OK;

        case FIRMWARE_UPGRADE_STATE_TRIAL:
            if ((firmware_boot_decision_slot_is_ab(metadata->confirmedSlot) == PLATFORM_FALSE) ||
                (firmware_boot_decision_slot_is_ab(metadata->pendingSlot) == PLATFORM_FALSE) ||
                (metadata->confirmedSlot == metadata->pendingSlot)) {
                return PLATFORM_ERR_INVALID_STATE;
            }
            *action = FIRMWARE_BOOT_ACTION_BEGIN_ROLLBACK;
            return PLATFORM_ERR_OK;

        case FIRMWARE_UPGRADE_STATE_ROLLBACK:
            if ((firmware_boot_decision_slot_is_ab(metadata->confirmedSlot) == PLATFORM_FALSE) ||
                (firmware_boot_decision_slot_is_ab(metadata->pendingSlot) == PLATFORM_FALSE) ||
                (metadata->confirmedSlot == metadata->pendingSlot)) {
                return PLATFORM_ERR_INVALID_STATE;
            }
            *action = FIRMWARE_BOOT_ACTION_CONTINUE_ROLLBACK;
            return PLATFORM_ERR_OK;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }
}
