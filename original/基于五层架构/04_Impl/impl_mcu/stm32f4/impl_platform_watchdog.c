/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_watchdog.c
 * @brief STM32F4 HAL IWDG Platform Impl。
 * @author YaoQian Wang
 * @date 2026-09-19
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_watchdog.h"
#include "platform_def.h"

#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_iwdg.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define PLATFORM_WATCHDOG_PRESCALER_DIV      (256U)
#define PLATFORM_WATCHDOG_RELOAD_MAX         (0xFFFU)
#define PLATFORM_WATCHDOG_MIN_TIMEOUT_MS     (100U)
#define PLATFORM_WATCHDOG_MAX_TIMEOUT_MS     (32000U)
//******************************** Defines **********************************//

//******************************** Variables ********************************//
static IWDG_HandleTypeDef g_platformWatchdog;
static platform_bool_t g_platformWatchdogStarted = PLATFORM_FALSE;
//******************************** Variables ********************************//

//******************************** Private Functions *************************//
static platform_error_t platform_watchdog_calculate_reload(
    uint32_t timeoutMs,
    uint32_t *reload)
{
    uint64_t denominator;
    uint64_t ticks;

    if (reload == (uint32_t *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((timeoutMs < PLATFORM_WATCHDOG_MIN_TIMEOUT_MS) ||
        (timeoutMs > PLATFORM_WATCHDOG_MAX_TIMEOUT_MS)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    denominator = (uint64_t)PLATFORM_WATCHDOG_PRESCALER_DIV * 1000ULL;
    ticks = (((uint64_t)timeoutMs * (uint64_t)LSI_VALUE) +
             denominator - 1ULL) / denominator;
    if ((ticks == 0ULL) ||
        (ticks > ((uint64_t)PLATFORM_WATCHDOG_RELOAD_MAX + 1ULL))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    *reload = (uint32_t)(ticks - 1ULL);
    return PLATFORM_ERR_OK;
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_watchdog_start(uint32_t timeoutMs)
{
    uint32_t reload;
    platform_error_t result;

    if (g_platformWatchdogStarted == PLATFORM_TRUE) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    result = platform_watchdog_calculate_reload(timeoutMs, &reload);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    g_platformWatchdog.Instance = IWDG;
    g_platformWatchdog.Init.Prescaler = IWDG_PRESCALER_256;
    g_platformWatchdog.Init.Reload = reload;
    if (HAL_IWDG_Init(&g_platformWatchdog) != HAL_OK) {
        return PLATFORM_ERR_IO;
    }

    __HAL_DBGMCU_FREEZE_IWDG();
    g_platformWatchdogStarted = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

platform_error_t platform_watchdog_feed(void)
{
    if (g_platformWatchdogStarted != PLATFORM_TRUE) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return (HAL_IWDG_Refresh(&g_platformWatchdog) == HAL_OK) ?
           PLATFORM_ERR_OK : PLATFORM_ERR_IO;
}
//******************************** Functions *********************************//
