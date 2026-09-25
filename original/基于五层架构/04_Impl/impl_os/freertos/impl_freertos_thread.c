/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_freertos_thread.c
 * @brief 实现基于 CMSIS-RTOS2 的 Platform Thread Adapter。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#include "impl_freertos_common.h"

static platform_bool_t impl_freertos_thread_priority_is_valid(platform_thread_priority_t priority)
{
    return (priority <= PLATFORM_THREAD_PRIORITY_HIGH);
}

static osPriority_t impl_freertos_thread_priority_to_cmsis(platform_thread_priority_t priority)
{
    static const osPriority_t priorities[] = {
        osPriorityLow,
        osPriorityBelowNormal,
        osPriorityNormal,
        osPriorityAboveNormal,
        osPriorityHigh
    };

    return priorities[priority];
}

platform_error_t platform_thread_create(platform_thread_t *thread,
                                        const platform_thread_config_t *config)
{
    osThreadAttr_t attributes;

    if ((thread == (void *)0) || (config == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((thread->native != (void *)0) ||
        (config->entry == (void *)0) ||
        (config->stackSizeBytes == 0U) ||
        !impl_freertos_thread_priority_is_valid(config->priority)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    attributes.name = config->name;
    attributes.attr_bits = 0U;
    attributes.cb_mem = (void *)0;
    attributes.cb_size = 0U;
    attributes.stack_mem = (void *)0;
    attributes.stack_size = config->stackSizeBytes;
    attributes.priority = impl_freertos_thread_priority_to_cmsis(config->priority);
    attributes.tz_module = 0U;
    attributes.reserved = 0U;

    thread->native = osThreadNew(config->entry, config->argument, &attributes);
    if (thread->native == (void *)0) {
        return PLATFORM_ERR_NO_MEMORY;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_thread_get_current(platform_thread_t *thread)
{
    if (thread == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    thread->native = osThreadGetId();
    if (thread->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_thread_get_stack_space(
    const platform_thread_t *thread,
    uint32_t *freeStackBytes)
{
    if ((thread == (void *)0) || (freeStackBytes == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (thread->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    *freeStackBytes = osThreadGetStackSpace(
        (osThreadId_t)thread->native);
    return PLATFORM_ERR_OK;
}

platform_error_t platform_thread_set_priority(platform_thread_t *thread,
                                              platform_thread_priority_t priority)
{
    if (thread == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((thread->native == (void *)0) || !impl_freertos_thread_priority_is_valid(priority)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return impl_freertos_map_status(
        osThreadSetPriority((osThreadId_t)thread->native,
                            impl_freertos_thread_priority_to_cmsis(priority)));
}

platform_error_t platform_thread_get_priority(const platform_thread_t *thread,
                                              platform_thread_priority_t *priority)
{
    osPriority_t cmsisPriority;

    if ((thread == (void *)0) || (priority == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (thread->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    cmsisPriority = osThreadGetPriority((osThreadId_t)thread->native);
    if (cmsisPriority == osPriorityError) {
        return PLATFORM_ERR_UNKNOWN;
    }

    if (cmsisPriority <= osPriorityLow) {
        *priority = PLATFORM_THREAD_PRIORITY_LOW;
    } else if (cmsisPriority <= osPriorityBelowNormal) {
        *priority = PLATFORM_THREAD_PRIORITY_BELOW_NORMAL;
    } else if (cmsisPriority <= osPriorityNormal) {
        *priority = PLATFORM_THREAD_PRIORITY_NORMAL;
    } else if (cmsisPriority <= osPriorityAboveNormal) {
        *priority = PLATFORM_THREAD_PRIORITY_ABOVE_NORMAL;
    } else {
        *priority = PLATFORM_THREAD_PRIORITY_HIGH;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_thread_suspend(platform_thread_t *thread)
{
    if (thread == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (thread->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return impl_freertos_map_status(osThreadSuspend((osThreadId_t)thread->native));
}

platform_error_t platform_thread_resume(platform_thread_t *thread)
{
    if (thread == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (thread->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return impl_freertos_map_status(osThreadResume((osThreadId_t)thread->native));
}

platform_error_t platform_thread_terminate(platform_thread_t *thread)
{
    osThreadId_t targetThread;
    platform_error_t result;

    if (thread == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (thread->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    targetThread = (osThreadId_t)thread->native;
    if (targetThread == osThreadGetId()) {
        thread->native = (void *)0;
    }

    result = impl_freertos_map_status(osThreadTerminate(targetThread));
    if (result == PLATFORM_ERR_OK) {
        thread->native = (void *)0;
    }

    return result;
}

platform_error_t platform_thread_yield(void)
{
    return impl_freertos_map_status(osThreadYield());
}
