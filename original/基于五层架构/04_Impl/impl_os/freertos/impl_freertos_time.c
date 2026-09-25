/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_freertos_time.c
 * @brief 实现 CMSIS-RTOS2 Time / Delay Adapter。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#include "impl_freertos_common.h"

uint32_t impl_freertos_timeout_to_ticks(uint32_t timeoutMs)
{
    uint64_t ticks;
    uint32_t frequency;

    if (timeoutMs == PLATFORM_OS_WAIT_FOREVER) {
        return osWaitForever;
    }

    if (timeoutMs == PLATFORM_OS_NO_WAIT) {
        return 0U;
    }

    frequency = osKernelGetTickFreq();
    if (frequency == 0U) {
        return 0U;
    }

    /* 使用 64-bit 中间值并向上取整，保证非零毫秒不会变成零 tick。 */
    ticks = (((uint64_t)timeoutMs * frequency) + 999U) / 1000U;
    return (ticks > 0xFFFFFFFEULL) ? 0xFFFFFFFEU : (uint32_t)ticks;
}

platform_error_t impl_freertos_map_status(osStatus_t status)
{
    switch (status) {
        case osOK:
            return PLATFORM_ERR_OK;
        case osErrorTimeout:
            return PLATFORM_ERR_TIMEOUT;
        case osErrorParameter:
            return PLATFORM_ERR_INVALID_PARAM;
        case osErrorNoMemory:
            return PLATFORM_ERR_NO_MEMORY;
        case osErrorISR:
            return PLATFORM_ERR_INVALID_STATE;
        case osErrorResource:
            return PLATFORM_ERR_NO_RESOURCE;
        default:
            return PLATFORM_ERR_UNKNOWN;
    }
}

platform_error_t platform_time_delay_ms(uint32_t delayMs)
{
    if (delayMs == 0U) {
        return PLATFORM_ERR_OK;
    }

    if (delayMs == PLATFORM_OS_WAIT_FOREVER) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return impl_freertos_map_status(osDelay(impl_freertos_timeout_to_ticks(delayMs)));
}

platform_error_t platform_time_get_ms(uint32_t *timeMs)
{
    uint32_t frequency;

    if (timeMs == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    frequency = osKernelGetTickFreq();
    if (frequency == 0U) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    *timeMs = (uint32_t)(((uint64_t)osKernelGetTickCount() * 1000U) / frequency);
    return PLATFORM_ERR_OK;
}
