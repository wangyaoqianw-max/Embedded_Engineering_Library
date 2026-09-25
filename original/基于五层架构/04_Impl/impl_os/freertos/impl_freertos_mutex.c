/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file impl_freertos_mutex.c
 * @brief 实现基于 CMSIS-RTOS2 的 Platform Mutex Adapter。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#include "impl_freertos_common.h"

platform_error_t platform_mutex_create(platform_mutex_t *mutex, platform_mutex_type_t type)
{
    osMutexAttr_t attributes;

    if (mutex == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((mutex->native != (void *)0) || (type > PLATFORM_MUTEX_RECURSIVE)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    attributes.name = (const char *)0;
    attributes.attr_bits = (type == PLATFORM_MUTEX_RECURSIVE) ? osMutexRecursive : 0U;
    attributes.cb_mem = (void *)0;
    attributes.cb_size = 0U;
    mutex->native = osMutexNew(&attributes);

    return (mutex->native == (void *)0) ? PLATFORM_ERR_NO_MEMORY : PLATFORM_ERR_OK;
}

platform_error_t platform_mutex_lock(platform_mutex_t *mutex, uint32_t timeoutMs)
{
    platform_error_t result;

    if (mutex == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (mutex->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(
        osMutexAcquire((osMutexId_t)mutex->native,
                       impl_freertos_timeout_to_ticks(timeoutMs)));
    if ((result == PLATFORM_ERR_NO_RESOURCE) && (timeoutMs == PLATFORM_OS_NO_WAIT)) {
        return PLATFORM_ERR_BUSY;
    }

    return result;
}

platform_error_t platform_mutex_unlock(platform_mutex_t *mutex)
{
    if (mutex == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (mutex->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return impl_freertos_map_status(osMutexRelease((osMutexId_t)mutex->native));
}

platform_error_t platform_mutex_delete(platform_mutex_t *mutex)
{
    platform_error_t result;

    if (mutex == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (mutex->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(osMutexDelete((osMutexId_t)mutex->native));
    if (result == PLATFORM_ERR_OK) {
        mutex->native = (void *)0;
    }

    return result;
}
