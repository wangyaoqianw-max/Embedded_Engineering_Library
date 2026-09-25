/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_semaphore.h
 * @brief 定义 Binary 与 Counting Semaphore 的 Platform 接口。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_SEMAPHORE_H
#define PLATFORM_SEMAPHORE_H

#include "platform_os_types.h"

/** @brief 创建 Semaphore；maxCount 必须非零且 initialCount 不大于 maxCount。 */
platform_error_t platform_semaphore_create(platform_semaphore_t *semaphore,
                                           uint32_t maxCount,
                                           uint32_t initialCount);
/** @brief 在 Task Context 获取一个 token；timeoutMs 单位为毫秒。 */
platform_error_t platform_semaphore_take(platform_semaphore_t *semaphore, uint32_t timeoutMs);
/** @brief 在 Task Context 释放一个 token。 */
platform_error_t platform_semaphore_give(platform_semaphore_t *semaphore);
/** @brief 在 ISR Context 释放一个 token；不得阻塞。 */
platform_error_t platform_semaphore_give_from_isr(platform_semaphore_t *semaphore);
/** @brief 删除 Semaphore；成功后清空 opaque handle。 */
platform_error_t platform_semaphore_delete(platform_semaphore_t *semaphore);

#endif
