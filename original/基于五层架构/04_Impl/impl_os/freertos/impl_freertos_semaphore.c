/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file impl_freertos_semaphore.c
 * @brief 实现基于 CMSIS-RTOS2 的 Platform Semaphore Adapter。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#include "impl_freertos_common.h"

platform_error_t platform_semaphore_create(platform_semaphore_t *semaphore,
                                           uint32_t maxCount,
                                           uint32_t initialCount)
{
    if (semaphore == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((semaphore->native != (void *)0) ||
        (maxCount == 0U) ||
        (initialCount > maxCount)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    semaphore->native = osSemaphoreNew(maxCount, initialCount, (const osSemaphoreAttr_t *)0);
    return (semaphore->native == (void *)0) ? PLATFORM_ERR_NO_MEMORY : PLATFORM_ERR_OK;
}

platform_error_t platform_semaphore_take(platform_semaphore_t *semaphore, uint32_t timeoutMs)
{
    platform_error_t result;

    if (semaphore == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (semaphore->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(
        osSemaphoreAcquire((osSemaphoreId_t)semaphore->native,
                           impl_freertos_timeout_to_ticks(timeoutMs)));
    if ((result == PLATFORM_ERR_NO_RESOURCE) && (timeoutMs == PLATFORM_OS_NO_WAIT)) {
        return PLATFORM_ERR_EMPTY;
    }

    return result;
}

platform_error_t platform_semaphore_give(platform_semaphore_t *semaphore)
{
    platform_error_t result;

    if (semaphore == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (semaphore->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(osSemaphoreRelease((osSemaphoreId_t)semaphore->native));
    return (result == PLATFORM_ERR_NO_RESOURCE) ? PLATFORM_ERR_FULL : result;
}

platform_error_t platform_semaphore_give_from_isr(platform_semaphore_t *semaphore)
{
    return platform_semaphore_give(semaphore);
}

platform_error_t platform_semaphore_delete(platform_semaphore_t *semaphore)
{
    platform_error_t result;

    if (semaphore == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (semaphore->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(osSemaphoreDelete((osSemaphoreId_t)semaphore->native));
    if (result == PLATFORM_ERR_OK) {
        semaphore->native = (void *)0;
    }

    return result;
}
