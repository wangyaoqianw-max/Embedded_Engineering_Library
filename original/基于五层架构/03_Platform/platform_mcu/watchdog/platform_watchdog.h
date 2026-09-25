/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_watchdog.h
 * @brief Platform MCU Watchdog 能力公共接口。
 * @author YaoQian Wang
 * @date 2026-09-19
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_WATCHDOG_H
#define PLATFORM_WATCHDOG_H

//******************************** Includes *********************************//
#include "platform_error.h"
//******************************** Includes *********************************//

//******************************** Functions ********************************//
/**
 * @brief 启动不可停止的 MCU Independent Watchdog。
 * @param[in] timeoutMs : 目标超时时间，单位为毫秒
 * @return platform_error_t : 启动结果
 * @note 本接口只表达 MCU 能力，不包含 Trial、OTA 或 Health 语义。
 */
platform_error_t platform_watchdog_start(uint32_t timeoutMs);

/**
 * @brief 刷新已启动的 MCU Independent Watchdog。
 * @return platform_error_t : 刷新结果
 * @note Feed policy 由 Application Health/Task 决定，Platform 不主动调度 Feed。
 */
platform_error_t platform_watchdog_feed(void);
//******************************** Functions ********************************//

#endif
