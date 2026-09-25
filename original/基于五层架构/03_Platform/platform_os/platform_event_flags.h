/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_event_flags.h
 * @brief 定义 Platform Event Flags 的事件同步接口。
 * @author YaoQian Wang
 * @date 2026-09-18
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_EVENT_FLAGS_H
#define PLATFORM_EVENT_FLAGS_H

#include "platform_os_types.h"

#define PLATFORM_EVENT_FLAGS_VALID_MASK    (0x7FFFFFFFU)

/**
 * @brief 创建 Event Flags 对象。
 * @param[out] eventFlags : 输出的 Event Flags 不透明句柄
 * @return platform_error_t : 创建结果
 */
platform_error_t platform_event_flags_create(
    platform_event_flags_t *eventFlags);

/**
 * @brief 设置一个或多个事件位。
 * @param[in,out] eventFlags : Event Flags 不透明句柄
 * @param[in] flags : 待设置的事件位
 * @return platform_error_t : 设置结果
 */
platform_error_t platform_event_flags_set(
    platform_event_flags_t *eventFlags,
    uint32_t flags);

/**
 * @brief 等待一个或多个事件位。
 * @param[in] eventFlags : Event Flags 不透明句柄
 * @param[in] flags : 待等待的事件位
 * @param[in] waitAll : PLATFORM_TRUE 等待全部事件位，否则等待任一事件位
 * @param[in] clearOnExit : 返回前清除已匹配的事件位
 * @param[in] timeoutMs : 超时时间，单位为毫秒
 * @param[out] receivedFlags : 返回本次匹配到的事件位
 * @return platform_error_t : 等待结果
 */
platform_error_t platform_event_flags_wait(
    platform_event_flags_t *eventFlags,
    uint32_t flags,
    platform_bool_t waitAll,
    platform_bool_t clearOnExit,
    uint32_t timeoutMs,
    uint32_t *receivedFlags);

/**
 * @brief 删除 Event Flags 对象并清空句柄。
 * @param[in,out] eventFlags : Event Flags 不透明句柄
 * @return platform_error_t : 删除结果
 */
platform_error_t platform_event_flags_delete(
    platform_event_flags_t *eventFlags);

#endif
