/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_impl_platform_delay.c
 * @brief 验证 Platform 微秒延时实现能够提供既有公共符号
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

//******************************** Functions *********************************//
/**
 * @brief 验证既有 Platform 微秒延时声明具有可链接的实现
 * @return 成功返回 0
 */
int main(void)
{
    platform_delay_us(0U);

    return 0;
}
//******************************** Functions *********************************//
