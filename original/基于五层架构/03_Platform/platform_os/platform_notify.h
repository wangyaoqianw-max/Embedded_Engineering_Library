/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_notify.h
 * @brief 定义基于 Thread Flag 的 Task 与 ISR 通知接口。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_NOTIFY_H
#define PLATFORM_NOTIFY_H

#include "platform_os_types.h"

/** @brief 在 Task Context 设置目标线程的有效通知位。 */
platform_error_t platform_notify_set(platform_thread_t *thread,
                                     uint32_t flags);
/** @brief 在 ISR Context 设置目标线程的有效通知位；不得阻塞。 */
platform_error_t platform_notify_set_from_isr(platform_thread_t *thread, uint32_t flags);
/** @brief 等待当前线程的任一或全部通知位；timeoutMs 单位为毫秒。 */
platform_error_t platform_notify_wait(uint32_t flags, platform_bool_t waitAll, platform_bool_t clearOnExit,
                                      uint32_t timeoutMs, uint32_t *receivedFlags);
/** @brief 清除当前线程的通知位并返回清除前结果。 */
platform_error_t platform_notify_clear(uint32_t flags, uint32_t *previousFlags);

#endif
