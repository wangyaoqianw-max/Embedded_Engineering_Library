/*
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file cmb_user_cfg.h
 * @brief CmBacktrace 的 Application 工程配置。
 * @author YaoQian Wang
 * @date 2026-09-15
 * @version V1.0
 */

#ifndef CMB_USER_CFG_H
#define CMB_USER_CFG_H

#include "SEGGER_RTT.h"

/* CmBacktrace 要求保留此宏名，输出使用 Fault 路径的 raw RTT 接口。 */
#define cmb_println(...)                                                       \
    do {                                                                       \
        (void)SEGGER_RTT_printf(0U, __VA_ARGS__);                              \
        SEGGER_RTT_WriteString(0U, "\r\n");                                   \
    } while (0)

#define CMB_USING_OS_PLATFORM
#define CMB_OS_PLATFORM_TYPE          CMB_OS_PLATFORM_FREERTOS
#define CMB_CPU_PLATFORM_TYPE         CMB_CPU_ARM_CORTEX_M4
#define CMB_USING_DUMP_STACK_INFO
#define CMB_PRINT_LANGUAGE            CMB_PRINT_LANGUAGE_ENGLISH

#endif
