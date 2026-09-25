/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_delay.c
 * @brief STM32 Cortex-M4 Platform 微秒延时实现
 * @author YaoQian Wang
 * @date 2026-09-02
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_def.h"

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define IMPL_PLATFORM_DELAY_US_PER_SECOND      (1000000U)
#define IMPL_PLATFORM_DELAY_MAX_CYCLE_COUNT    (0x7FFFFFFFU)
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
static void impl_platform_delay_enable_cycle_counter(void);
static void impl_platform_delay_wait_cycles(uint32_t cycles);
//******************************** Declaring *********************************//

//******************************** Functions *********************************//
void platform_delay_ms(uint32_t ms)
{
    if (ms == 0U) {
        return;
    }

    HAL_Delay(ms);
}

/**
 * @brief 懒初始化 Cortex-M DWT 周期计数器
 * @note DWT 细节仅保留在 Impl 层，Platform 层只依赖微秒延时契约。
 */
static void impl_platform_delay_enable_cycle_counter(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U) {
        DWT->CYCCNT = 0U;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

/**
 * @brief 等待指定数量的 DWT 周期
 * @param[in] cycles : 等待周期数
 * @note 无符号差值比较能够处理 CYCCNT 在短延时期间回绕。
 */
static void impl_platform_delay_wait_cycles(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
    }
}

void platform_delay_us(uint32_t us)
{
    uint32_t cyclesPerUs = 0U;
    uint32_t maximumDelayUs = 0U;

    if (us == 0U) {
        return;
    }

    impl_platform_delay_enable_cycle_counter();

    cyclesPerUs = SystemCoreClock / IMPL_PLATFORM_DELAY_US_PER_SECOND;
    if (cyclesPerUs == 0U) {
        return;
    }

    maximumDelayUs = IMPL_PLATFORM_DELAY_MAX_CYCLE_COUNT / cyclesPerUs;
    while (us > maximumDelayUs) {
        impl_platform_delay_wait_cycles(IMPL_PLATFORM_DELAY_MAX_CYCLE_COUNT);
        us -= maximumDelayUs;
    }

    impl_platform_delay_wait_cycles(us * cyclesPerUs);
}
//******************************** Functions *********************************//
