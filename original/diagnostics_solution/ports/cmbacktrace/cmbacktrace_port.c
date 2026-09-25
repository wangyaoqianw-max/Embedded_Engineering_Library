/*
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file cmbacktrace_port.c
 * @brief CmBacktrace 工程适配实现。
 * @author YaoQian Wang
 * @date 2026-09-15
 * @version V1.0
 */

#include "cmbacktrace_port.h"
#include "diagnostics_config.h"

#include "cm_backtrace.h"

void cmbacktrace_port_init(void)
{
    cm_backtrace_init(DIAGNOSTICS_CMB_FIRMWARE_NAME,
                     DIAGNOSTICS_CMB_HARDWARE_NAME,
                     DIAGNOSTICS_CMB_FIRMWARE_VERSION);
}
