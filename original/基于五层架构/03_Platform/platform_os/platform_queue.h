/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_queue.h
 * @brief 定义固定大小、值拷贝语义的 Platform Queue 接口。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_QUEUE_H
#define PLATFORM_QUEUE_H

#include "platform_os_types.h"

/** @brief 创建固定容量和固定 itemSize 的 Queue。 */
platform_error_t platform_queue_create(platform_queue_t *queue,
                                       platform_size_t itemCount,
                                       platform_size_t itemSize);
/** @brief 在 Task Context 复制一个 item 到 Queue；timeoutMs 单位为毫秒。 */
platform_error_t platform_queue_send(platform_queue_t *queue, const void *item, uint32_t timeoutMs);
/** @brief 在 ISR Context 复制一个 item 到 Queue；不得阻塞。 */
platform_error_t platform_queue_send_from_isr(platform_queue_t *queue, const void *item);
/** @brief 在 Task Context 从 Queue 接收一个 item；timeoutMs 单位为毫秒。 */
platform_error_t platform_queue_receive(platform_queue_t *queue, void *item, uint32_t timeoutMs);
/** @brief 获取 Queue 当前已存 item 数量。 */
platform_error_t platform_queue_get_count(const platform_queue_t *queue, platform_size_t *count);
/** @brief 获取 Queue 剩余 item 容量。 */
platform_error_t platform_queue_get_space(const platform_queue_t *queue, platform_size_t *space);
/** @brief 删除 Queue；成功后清空 opaque handle。 */
platform_error_t platform_queue_delete(platform_queue_t *queue);

#endif
