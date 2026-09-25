/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_freertos_event_flags.c
 * @brief 实现基于 CMSIS-RTOS2 Event Flags 的 Platform Adapter。
 * @author YaoQian Wang
 * @date 2026-09-18
 * @version V1.0
 *
 *****************************************************************************/

#include "impl_freertos_common.h"

static platform_bool_t impl_freertos_event_flags_are_valid(uint32_t flags)
{
    return (flags != 0U) &&
           ((flags & ~PLATFORM_EVENT_FLAGS_VALID_MASK) == 0U);
}

static platform_error_t impl_freertos_event_flags_map_result(uint32_t result)
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

platform_error_t platform_event_flags_create(
    platform_event_flags_t *eventFlags)
{
    if (eventFlags == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (eventFlags->native != (void *)0) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    eventFlags->native = osEventFlagsNew((const osEventFlagsAttr_t *)0);
    if (eventFlags->native == (void *)0) {
        return PLATFORM_ERR_NO_MEMORY;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_event_flags_set(
    platform_event_flags_t *eventFlags,
    uint32_t flags)
{
    uint32_t result;

    if (eventFlags == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((eventFlags->native == (void *)0) ||
        !impl_freertos_event_flags_are_valid(flags)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = osEventFlagsSet(
        (osEventFlagsId_t)eventFlags->native,
        flags);
    return impl_freertos_event_flags_map_result(result);
}

platform_error_t platform_event_flags_wait(
    platform_event_flags_t *eventFlags,
    uint32_t flags,
    platform_bool_t waitAll,
    platform_bool_t clearOnExit,
    uint32_t timeoutMs,
    uint32_t *receivedFlags)
{
    uint32_t options;
    uint32_t result;
    platform_error_t error;

    if ((eventFlags == (void *)0) || (receivedFlags == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((eventFlags->native == (void *)0) ||
        !impl_freertos_event_flags_are_valid(flags)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    options = (waitAll != 0U) ? osFlagsWaitAll : osFlagsWaitAny;
    if (clearOnExit == 0U) {
        options |= osFlagsNoClear;
    }

    result = osEventFlagsWait(
        (osEventFlagsId_t)eventFlags->native,
        flags,
        options,
        impl_freertos_timeout_to_ticks(timeoutMs));
    error = impl_freertos_event_flags_map_result(result);
    /* 无等待且无匹配事件时，FreeRTOS 返回 Resource，Platform 统一映射为超时。 */
    if ((error == PLATFORM_ERR_NO_RESOURCE) &&
        (timeoutMs == PLATFORM_OS_NO_WAIT)) {
        return PLATFORM_ERR_TIMEOUT;
    }
    if (error == PLATFORM_ERR_OK) {
        *receivedFlags = result;
    }

    return error;
}

platform_error_t platform_event_flags_delete(
    platform_event_flags_t *eventFlags)
{
    platform_error_t result;

    if (eventFlags == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (eventFlags->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(
        osEventFlagsDelete((osEventFlagsId_t)eventFlags->native));
    if (result == PLATFORM_ERR_OK) {
        eventFlags->native = (void *)0;
    }

    return result;
}
