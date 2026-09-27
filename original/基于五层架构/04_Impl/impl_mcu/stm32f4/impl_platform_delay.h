/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_delay.h
 * @brief STM32 Cortex-M4 Impl 内部微秒延时接口
 * @author YaoQian Wang
 * @date 2026-09-27
 * @version V1.0
 *
 *****************************************************************************/

#ifndef IMPL_PLATFORM_DELAY_H
#define IMPL_PLATFORM_DELAY_H

#include "platform_types.h"

/*
 * 供同一 Impl 层模块复用，避免 Impl 反向调用 Platform delay wrapper。
 */
void impl_platform_delay_us(uint32_t us);

#endif
