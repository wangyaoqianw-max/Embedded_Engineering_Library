/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file impl_freertos_queue.c
 * @brief 实现基于 CMSIS-RTOS2 Message Queue 的 Platform Queue Adapter。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#include "impl_freertos_common.h"

platform_error_t platform_queue_create(platform_queue_t *queue,
                                       platform_size_t itemCount,
                                       platform_size_t itemSize)
{
    if (queue == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((queue->native != (void *)0) || (itemCount == 0U) || (itemSize == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    queue->native = osMessageQueueNew(itemCount, itemSize, (const osMessageQueueAttr_t *)0);
    return (queue->native == (void *)0) ? PLATFORM_ERR_NO_MEMORY : PLATFORM_ERR_OK;
}

platform_error_t platform_queue_send(platform_queue_t *queue, const void *item, uint32_t timeoutMs)
{
    platform_error_t result;

    if ((queue == (void *)0) || (item == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (queue->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(
        osMessageQueuePut((osMessageQueueId_t)queue->native,
                          item,
                          0U,
                          impl_freertos_timeout_to_ticks(timeoutMs)));
    if ((result == PLATFORM_ERR_NO_RESOURCE) && (timeoutMs == PLATFORM_OS_NO_WAIT)) {
        return PLATFORM_ERR_FULL;
    }

    return result;
}

platform_error_t platform_queue_send_from_isr(platform_queue_t *queue, const void *item)
{
    return platform_queue_send(queue, item, PLATFORM_OS_NO_WAIT);
}

platform_error_t platform_queue_receive(platform_queue_t *queue, void *item, uint32_t timeoutMs)
{
    platform_error_t result;

    if ((queue == (void *)0) || (item == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (queue->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(
        osMessageQueueGet((osMessageQueueId_t)queue->native,
                          item,
                          (uint8_t *)0,
                          impl_freertos_timeout_to_ticks(timeoutMs)));
    if ((result == PLATFORM_ERR_NO_RESOURCE) && (timeoutMs == PLATFORM_OS_NO_WAIT)) {
        return PLATFORM_ERR_EMPTY;
    }

    return result;
}

platform_error_t platform_queue_get_count(const platform_queue_t *queue, platform_size_t *count)
{
    if ((queue == (void *)0) || (count == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (queue->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    *count = osMessageQueueGetCount((osMessageQueueId_t)queue->native);
    return PLATFORM_ERR_OK;
}

platform_error_t platform_queue_get_space(const platform_queue_t *queue, platform_size_t *space)
{
    if ((queue == (void *)0) || (space == (void *)0)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (queue->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    *space = osMessageQueueGetSpace((osMessageQueueId_t)queue->native);
    return PLATFORM_ERR_OK;
}

platform_error_t platform_queue_delete(platform_queue_t *queue)
{
    platform_error_t result;

    if (queue == (void *)0) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (queue->native == (void *)0) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    result = impl_freertos_map_status(osMessageQueueDelete((osMessageQueueId_t)queue->native));
    if (result == PLATFORM_ERR_OK) {
        queue->native = (void *)0;
    }

    return result;
}
