/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_timer.h
 * @brief 定义运行于 RTOS Timer Task 的 Software Timer 接口。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_TIMER_H
#define PLATFORM_TIMER_H

#include "platform_os_types.h"

/* Callback 运行在 RTOS Timer Task Context，绝非 ISR Context。 */
typedef enum {
    PLATFORM_TIMER_ONCE = 0,
    PLATFORM_TIMER_PERIODIC
} platform_timer_type_t;

typedef void (*platform_timer_callback_t)(void *argument);

/**
 * @brief 创建软件定时器所需的静态配置。
 * @note callback 在 RTOS Timer Task 中执行，argument 的有效期由调用者保证。
 */
typedef struct {
    const char *name;
    platform_timer_type_t type;
    platform_timer_callback_t callback;
    void *argument;
} platform_timer_config_t;

/** @brief 创建 one-shot 或 periodic Timer；callback 不得为 NULL。 */
platform_error_t platform_timer_create(platform_timer_t *timer,
                                       const platform_timer_config_t *config);
/** @brief 以毫秒周期启动 Timer；periodMs 必须非零。 */
platform_error_t platform_timer_start(platform_timer_t *timer, uint32_t periodMs);
/** @brief 停止已创建的 Timer。 */
platform_error_t platform_timer_stop(platform_timer_t *timer);
/** @brief 查询 Timer 是否正在运行。 */
platform_error_t platform_timer_is_running(const platform_timer_t *timer, platform_bool_t *running);
/** @brief 删除 Timer；成功后清空 opaque handle。 */
platform_error_t platform_timer_delete(platform_timer_t *timer);

#endif
