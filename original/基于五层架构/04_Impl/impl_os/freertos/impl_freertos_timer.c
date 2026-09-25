/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_freertos_timer.c
 * @brief 实现基于 CMSIS-RTOS2 的 Platform Software Timer Adapter。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *
 * @note CMSIS-RTOS2 的 osTimerNew() 负责保存 callback 与 argument 的生命周期。
 *****************************************************************************/

#include "impl_freertos_common.h"

static osTimerType_t impl_freertos_timer_type_to_cmsis(platform_timer_type_t type)
{
    if (type == PLATFORM_TIMER_PERIODIC) {
        return osTimerPeriodic;
    }

    return osTimerOnce;
}

platform_error_t platform_timer_create(platform_timer_t *timer,
                                       const platform_timer_config_t *config)
{
    osTimerAttr_t attributes;

    if ((timer == (void *)0) || (config == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((timer->native != (void *)0) ||
        (config->callback == (void *)0) ||
        (config->type > PLATFORM_TIMER_PERIODIC)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    attributes.name = config->name;
    attributes.attr_bits = 0U;
    attributes.cb_mem = (void *)0;
    attributes.cb_size = 0U;

    timer->native = osTimerNew(config->callback,
                               impl_freertos_timer_type_to_cmsis(config->type),
                               config->argument,
                               &attributes);
    if (timer->native == (void *)0) {
        return PLATFORM_ERR_NO_MEMORY;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_timer_start(platform_timer_t *timer, uint32_t periodMs)
{
    if (timer == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (timer->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (periodMs == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return impl_freertos_map_status(
        osTimerStart((osTimerId_t)timer->native,
                     impl_freertos_timeout_to_ticks(periodMs)));
}

platform_error_t platform_timer_stop(platform_timer_t *timer)
{
    if (timer == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (timer->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return impl_freertos_map_status(osTimerStop((osTimerId_t)timer->native));
}

platform_error_t platform_timer_is_running(const platform_timer_t *timer,
                                            platform_bool_t *running)
{
    if ((timer == (void *)0) || (running == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (timer->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    *running = (osTimerIsRunning((osTimerId_t)timer->native) != 0U) ? 1U : 0U;
    return PLATFORM_ERR_OK;
}

platform_error_t platform_timer_delete(platform_timer_t *timer)
{
    platform_error_t result;

    if (timer == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (timer->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(osTimerDelete((osTimerId_t)timer->native));
    if (result == PLATFORM_ERR_OK) {
        timer->native = (void *)0;
    }

    return result;
}
