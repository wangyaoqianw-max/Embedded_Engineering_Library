/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_mutex.h
 * @brief 定义 Task Context 使用的 Platform Mutex 接口。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_MUTEX_H
#define PLATFORM_MUTEX_H

#include "platform_os_types.h"

typedef enum {
    PLATFORM_MUTEX_NORMAL = 0,
    PLATFORM_MUTEX_RECURSIVE
} platform_mutex_type_t;

/** @brief 创建 Mutex；mutex 必须为未创建对象。 */
platform_error_t platform_mutex_create(platform_mutex_t *mutex,
                                       platform_mutex_type_t type);
/** @brief 在 Task Context 获取 Mutex；timeoutMs 单位为毫秒。 */
platform_error_t platform_mutex_lock(platform_mutex_t *mutex, uint32_t timeoutMs);
/** @brief 在 Task Context 释放 Mutex。 */
platform_error_t platform_mutex_unlock(platform_mutex_t *mutex);
/** @brief 删除 Mutex；成功后清空 opaque handle。 */
platform_error_t platform_mutex_delete(platform_mutex_t *mutex);

#endif
