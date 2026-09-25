/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_mcu_reset.h
 * @brief Platform MCU 复位控制公共接口
 * @author YaoQian Wang
 * @date 2026-09-17
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_MCU_RESET_H
#define PLATFORM_MCU_RESET_H

//******************************** Declaring *******************************//
/**
 * @brief 请求 MCU 系统复位
 * @note 本接口由 APP/Impl 执行层调用；Service 业务层不得直接调用。
 */
void platform_mcu_reset(void);
//******************************** Declaring *******************************//

#endif
