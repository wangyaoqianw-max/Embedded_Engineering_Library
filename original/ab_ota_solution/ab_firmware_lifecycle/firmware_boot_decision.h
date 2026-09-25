/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file firmware_boot_decision.h
 * @brief A/B Firmware Lifecycle 启动动作决策接口
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef FIRMWARE_BOOT_DECISION_H
#define FIRMWARE_BOOT_DECISION_H

#include "firmware_lifecycle_def.h"

typedef enum
{
    FIRMWARE_BOOT_ACTION_BOOT_CURRENT = 0,
    FIRMWARE_BOOT_ACTION_INSTALL_PENDING,
    FIRMWARE_BOOT_ACTION_BEGIN_ROLLBACK,
    FIRMWARE_BOOT_ACTION_CONTINUE_ROLLBACK
} firmware_boot_action_t;

platform_error_t firmware_boot_decision_get_action(
    const firmware_metadata_t *metadata,
    firmware_boot_action_t *action);

#endif
