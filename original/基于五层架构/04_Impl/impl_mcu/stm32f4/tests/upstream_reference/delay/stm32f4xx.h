/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file stm32f4xx.h
 * @brief Platform 微秒延时 Host Test 的最小 CMSIS 替身
 * @author YaoQian Wang
 * @date 2026-09-02
 * @version V1.0
 *
 *****************************************************************************/

#ifndef TEST_IMPL_PLATFORM_DELAY_STM32F4XX_H
#define TEST_IMPL_PLATFORM_DELAY_STM32F4XX_H

//******************************** Includes *********************************//
#include "platform_types.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
typedef struct
{
    uint32_t DEMCR;
} CoreDebug_Type;

typedef struct
{
    uint32_t CTRL;
    uint32_t CYCCNT;
} DWT_Type;

extern CoreDebug_Type *CoreDebug;
extern DWT_Type *DWT;
extern uint32_t SystemCoreClock;
//******************************** Declaring *********************************//

//******************************** Defines *********************************//
#define CoreDebug_DEMCR_TRCENA_Msk    (0x01000000U)
#define DWT_CTRL_CYCCNTENA_Msk        (0x00000001U)
//******************************** Defines *********************************//

#endif
