/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file impl_freertos_notify.c
 * @brief 实现基于 CMSIS-RTOS2 Thread Flags 的 Platform Notification Adapter。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#include "impl_freertos_common.h"

static platform_bool_t impl_freertos_notify_flags_are_valid(uint32_t flags)
{
    return (flags != 0U) && ((flags & ~PLATFORM_NOTIFY_VALID_MASK) == 0U);
}

static platform_error_t impl_freertos_notify_map_flags_result(uint32_t result)
{
    if ((result & osFlagsError) == 0U) {
        return PLATFORM_ERR_OK;
    }

    if (result == osFlagsErrorTimeout) {
        return PLATFORM_ERR_TIMEOUT;
    }

    if (result == osFlagsErrorResource) {
        return PLATFORM_ERR_NO_RESOURCE;
    }

    if (result == osFlagsErrorParameter) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (result == osFlagsErrorISR) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return PLATFORM_ERR_UNKNOWN;
}

platform_error_t platform_notify_set(platform_thread_t *thread, uint32_t flags)
{
    platform_error_t result;

    if (thread == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((thread->native == (void *)0) || !impl_freertos_notify_flags_are_valid(flags)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = impl_freertos_notify_map_flags_result(
        osThreadFlagsSet((osThreadId_t)thread->native, flags));
    return (result == PLATFORM_ERR_NO_RESOURCE) ? PLATFORM_ERR_INVALID_STATE : result;
}

platform_error_t platform_notify_set_from_isr(platform_thread_t *thread, uint32_t flags)
{
    return platform_notify_set(thread, flags);
}

platform_error_t platform_notify_wait(uint32_t flags,
                                      platform_bool_t waitAll,
                                      platform_bool_t clearOnExit,
                                      uint32_t timeoutMs,
                                      uint32_t *receivedFlags)
{
    uint32_t options;
    uint32_t result;
    platform_error_t error;

    if (receivedFlags == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (!impl_freertos_notify_flags_are_valid(flags)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    options = (waitAll != 0U) ? osFlagsWaitAll : osFlagsWaitAny;
    if (clearOnExit == 0U) {
        options |= osFlagsNoClear;
    }

    result = osThreadFlagsWait(flags,
                                options,
                                impl_freertos_timeout_to_ticks(timeoutMs));
    error = impl_freertos_notify_map_flags_result(result);
    if ((error == PLATFORM_ERR_NO_RESOURCE) && (timeoutMs == PLATFORM_OS_NO_WAIT)) {
        return PLATFORM_ERR_EMPTY;
    }

    if (error == PLATFORM_ERR_OK) {
        *receivedFlags = result;
    }

    return error;
}

platform_error_t platform_notify_clear(uint32_t flags, uint32_t *previousFlags)
{
    uint32_t result;
    platform_error_t error;

    if (previousFlags == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (!impl_freertos_notify_flags_are_valid(flags)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = osThreadFlagsClear(flags);
    error = impl_freertos_notify_map_flags_result(result);
    if (error == PLATFORM_ERR_OK) {
        *previousFlags = result;
    }

    return error;
}
