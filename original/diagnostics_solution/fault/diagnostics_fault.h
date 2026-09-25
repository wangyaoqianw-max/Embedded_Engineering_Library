/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file diagnostics_fault.h
 * @brief Cortex-M Fault 受控测试与现场上下文接口。
 * @author YaoQian Wang
 * @date 2026-09-15
 * @version V1.0
 ******************************************************************************/

#ifndef DIAGNOSTICS_FAULT_H
#define DIAGNOSTICS_FAULT_H

#include "diagnostics_config.h"

#include <stdint.h>

/**
 * @brief 受控 Fault 测试类型。
 */
typedef enum {
    DIAG_FAULT_NONE = 0,
    DIAG_FAULT_INVALID_ADDRESS,
    DIAG_FAULT_UNDEFINED_INSTRUCTION,
    DIAG_FAULT_DIV_BY_ZERO
} diagnostics_fault_type_t;

/**
 * @brief 保存 Cortex-M 自动压栈帧和系统 Fault 寄存器快照。
 */
typedef struct {
    uint32_t exceptionReturn;
    uint32_t stackedSp;
    uint32_t msp;
    uint32_t psp;
    uint32_t cfsr;
    uint32_t hfsr;
    uint32_t mmfar;
    uint32_t bfar;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} diagnostics_fault_context_t;

/**
 * @brief 供 Fault Handler 写入、供调试器读取的最新现场快照。
 */
extern volatile diagnostics_fault_context_t g_diagnostics_fault_context;

/**
 * @brief 初始化 Fault 测试所需的 Cortex-M 系统异常使能。
 */
void diagnostics_fault_init(void);

/**
 * @brief 触发一个受控 Fault 测试。
 * @param[in] type Fault 类型。
 */
void diagnostics_fault_trigger(diagnostics_fault_type_t type);

/**
 * @brief 执行 ARMCC 可识别的未定义 Thumb 指令。
 */
void diagnostics_fault_trigger_undefined(void);

/**
 * @brief 保存 Fault 现场并输出最小 RTT 诊断信息。
 * @param[in] exceptionReturn Fault 入口的 EXC_RETURN 值。
 * @param[in] stackedSp Cortex-M 自动压栈帧地址。
 */
void diagnostics_fault_handler(uint32_t exceptionReturn, uint32_t stackedSp);

#endif
