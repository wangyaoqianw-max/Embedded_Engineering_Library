/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_mcu_reset.c
 * @brief STM32 CMSIS MCU 复位 Platform Impl
 * @author YaoQian Wang
 * @date 2026-09-17
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_mcu_reset.h"

#include "stm32f4xx_hal.h"
//******************************** Includes *********************************//

//******************************** Public Functions *************************//
void platform_mcu_reset(void)
{
    NVIC_SystemReset();
}
//******************************** Public Functions *************************//
