/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file diagnostics_config.h
 * @brief Cortex-M Fault 与 CmBacktrace 方案配置
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef DIAGNOSTICS_CONFIG_H
#define DIAGNOSTICS_CONFIG_H

#ifndef DIAGNOSTICS_CMB_FIRMWARE_NAME
#define DIAGNOSTICS_CMB_FIRMWARE_NAME          "APP"
#endif

#ifndef DIAGNOSTICS_CMB_HARDWARE_NAME
#define DIAGNOSTICS_CMB_HARDWARE_NAME          "Cortex-M"
#endif

#ifndef DIAGNOSTICS_CMB_FIRMWARE_VERSION
#define DIAGNOSTICS_CMB_FIRMWARE_VERSION       "V1.0"
#endif

#ifndef DIAG_FAULT_TEST_ENABLE
#define DIAG_FAULT_TEST_ENABLE                 (0U)
#endif

#ifndef DIAG_FAULT_TEST_TYPE
#define DIAG_FAULT_TEST_TYPE                   DIAG_FAULT_INVALID_ADDRESS
#endif

#ifndef DIAG_FAULT_TEST_DELAY_MS
#define DIAG_FAULT_TEST_DELAY_MS               (1000U)
#endif

#define DIAG_FAULT_INVALID_ADDRESS_VALUE       (0xFFFFFFF0U)
#define DIAG_FAULT_WRITE_PATTERN               (0xDEADBEEFU)
#define DIAG_FAULT_UNDEFINED_OPCODE            (0xDEADU)

#endif
