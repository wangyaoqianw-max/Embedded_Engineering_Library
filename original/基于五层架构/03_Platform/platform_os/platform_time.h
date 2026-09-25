/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_time.h
 * @brief 定义 Platform OS 的毫秒时间与延时接口。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_TIME_H
#define PLATFORM_TIME_H

#include "platform_os_types.h"

/** @brief 在 Task Context 延时；delayMs 为零时立即返回。 */
platform_error_t platform_time_delay_ms(uint32_t delayMs);
/** @brief 读取可自然回绕的单调毫秒计数。 */
platform_error_t platform_time_get_ms(uint32_t *timeMs);

#endif
