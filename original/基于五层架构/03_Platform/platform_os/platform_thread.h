/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_thread.h
 * @brief 定义 Platform Thread 的配置与 Task Context 操作接口。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_THREAD_H
#define PLATFORM_THREAD_H

#include "platform_os_types.h"

typedef enum {
    PLATFORM_THREAD_PRIORITY_LOW = 0,
    PLATFORM_THREAD_PRIORITY_BELOW_NORMAL,
    PLATFORM_THREAD_PRIORITY_NORMAL,
    PLATFORM_THREAD_PRIORITY_ABOVE_NORMAL,
    PLATFORM_THREAD_PRIORITY_HIGH
} platform_thread_priority_t;

typedef void (*platform_thread_entry_t)(void *argument);

/**
 * @brief 创建线程所需的静态配置。
 * @note name、entry 和 argument 只被引用；stackSizeBytes 的单位为 byte。
 */
typedef struct {
    const char *name;
    platform_thread_entry_t entry;
    void *argument;
    uint32_t stackSizeBytes;
    platform_thread_priority_t priority;
} platform_thread_config_t;

/** @brief 创建线程；thread 必须为未创建对象，stackSizeBytes 的单位为 byte。 */
platform_error_t platform_thread_create(platform_thread_t *thread,
                                        const platform_thread_config_t *config);
/** @brief 获取当前执行线程的 opaque handle。 */
platform_error_t platform_thread_get_current(platform_thread_t *thread);
/** @brief 获取线程剩余未使用 Stack 空间，单位为 byte。 */
platform_error_t platform_thread_get_stack_space(
    const platform_thread_t *thread,
    uint32_t *freeStackBytes);
/** @brief 设置线程的 Platform 优先级。 */
platform_error_t platform_thread_set_priority(platform_thread_t *thread, platform_thread_priority_t priority);
/** @brief 获取线程的 Platform 优先级。 */
platform_error_t platform_thread_get_priority(const platform_thread_t *thread, platform_thread_priority_t *priority);
/** @brief 挂起目标线程；仅允许 Task Context 调用。 */
platform_error_t platform_thread_suspend(platform_thread_t *thread);
/** @brief 恢复目标线程；仅允许 Task Context 调用。 */
platform_error_t platform_thread_resume(platform_thread_t *thread);
/** @brief 终止目标线程；成功后清空 opaque handle。 */
platform_error_t platform_thread_terminate(platform_thread_t *thread);
/** @brief 让出当前线程的处理器时间片。 */
platform_error_t platform_thread_yield(void);

#endif
