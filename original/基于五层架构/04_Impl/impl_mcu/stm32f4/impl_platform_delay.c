/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_delay.c
 * @brief STM32 Cortex-M4 Platform 微秒延时实现
 * @author YaoQian Wang
 * @date 2026-09-02
 * @version V1.1
 *
 *****************************************************************************/

#include "impl_platform_delay.h"
#include "platform_def.h"

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#define IMPL_PLATFORM_DELAY_US_PER_SECOND      (1000000U)
#define IMPL_PLATFORM_DELAY_MAX_CYCLE_COUNT    (0x7FFFFFFFU)

static void impl_platform_delay_enable_cycle_counter(void);
static void impl_platform_delay_wait_cycles(uint32_t cycles);

void platform_delay_ms(uint32_t ms)
{
    if (ms == 0U) {
        return;
    }

    HAL_Delay(ms);
}

static void impl_platform_delay_enable_cycle_counter(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U) {
        DWT->CYCCNT = 0U;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}

static void impl_platform_delay_wait_cycles(uint32_t cycles)
{
    uint32_t start = DWT->CYCCNT;

    while ((uint32_t)(DWT->CYCCNT - start) < cycles) {
    }
}

void impl_platform_delay_us(uint32_t us)
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

void platform_delay_us(uint32_t us)
{
    impl_platform_delay_us(us);
}
