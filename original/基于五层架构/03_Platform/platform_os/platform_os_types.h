/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_os_types.h
 * @brief 定义 Platform OS 的公共对象、超时和通知类型。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_OS_TYPES_H
#define PLATFORM_OS_TYPES_H

#include "platform_error.h"

/* 所有 Platform OS 对象在创建前或删除后均使用此初始值。 */
#define PLATFORM_OS_OBJECT_INITIALIZER    { (void *)0 }
#define PLATFORM_OS_NO_WAIT               (0U)
#define PLATFORM_OS_WAIT_FOREVER          (0xFFFFFFFFU)
#define PLATFORM_NOTIFY_VALID_MASK        (0x7FFFFFFFU)

/* native 仅由 Impl 使用，Platform 公共层不解释其具体类型。 */
/** @brief 线程的不透明 Platform 句柄；native 仅由 Impl 创建和释放。 */
typedef struct {
    void *native;
} platform_thread_t;

/** @brief Event Flags 的不透明 Platform 句柄；native 仅由 Impl 创建和释放。 */
typedef struct {
    void *native;
} platform_event_flags_t;

/** @brief 互斥锁的不透明 Platform 句柄；native 仅由 Impl 创建和释放。 */
typedef struct {
    void *native;
} platform_mutex_t;

/** @brief 信号量的不透明 Platform 句柄；native 仅由 Impl 创建和释放。 */
typedef struct {
    void *native;
} platform_semaphore_t;

/** @brief 消息队列的不透明 Platform 句柄；native 仅由 Impl 创建和释放。 */
typedef struct {
    void *native;
} platform_queue_t;

/** @brief 软件定时器的不透明 Platform 句柄；native 仅由 Impl 创建和释放。 */
typedef struct {
    void *native;
} platform_timer_t;

#endif
