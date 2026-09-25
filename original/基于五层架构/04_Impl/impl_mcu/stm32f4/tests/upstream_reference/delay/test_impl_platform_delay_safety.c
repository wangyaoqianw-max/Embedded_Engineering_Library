/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_impl_platform_delay_safety.c
 * @brief 验证 DWT 延时单段等待不会跨越半个计数器回绕周期
 * @author YaoQian Wang
 * @date 2026-09-02
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_def.h"
#include "stm32f4xx.h"
//******************************** Includes *********************************//

//******************************** Variables *********************************//
static CoreDebug_Type g_fakeCoreDebug;
static DWT_Type g_fakeDwt;

CoreDebug_Type *CoreDebug = &g_fakeCoreDebug;
DWT_Type *DWT = &g_fakeDwt;
uint32_t SystemCoreClock = 100000000U;
//******************************** Variables *********************************//

#include "../../04_Impl/impl_mcu/impl_platform_delay.c"

_Static_assert(IMPL_PLATFORM_DELAY_MAX_CYCLE_COUNT <= 0x7FFFFFFFU,
               "DWT 单段等待必须小于半个 32 位计数器周期");

//******************************** Functions *********************************//
int main(void)
{
    return 0;
}
//******************************** Functions *********************************//
