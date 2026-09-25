/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_uart.h
 * @brief Platform UART 抽象对象和公共接口
 * @author YaoQian Wang
 * @date 2026-08-28
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_UART_H
#define PLATFORM_UART_H

//******************************** Includes *********************************//
#include "platform_uart_types.h"

#include "platform_device.h"
#include "platform_lifecycle.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/*首次构造前使用此宏初始化 UART 对象存储*/
#define PLATFORM_UART_INITIALIZER {0}
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/*UART 数据操作表，由 Impl 层注入*/
typedef struct
{
    platform_error_t (*write)(platform_uart_t *uart,
                              const uint8_t *data,
                              platform_size_t dataLength,
                              uint32_t timeoutMs,
                              platform_size_t *writtenLength);
    platform_error_t (*read)(platform_uart_t *uart,
                             uint8_t *buffer,
                             platform_size_t bufferSize,
                             uint32_t timeoutMs,
                             platform_size_t *readLength);
    platform_error_t (*writeAsync)(platform_uart_t *uart,
                                   const uint8_t *data,
                                   platform_size_t dataLength);
    platform_error_t (*readAsync)(platform_uart_t *uart,
                                  uint8_t *buffer,
                                  platform_size_t bufferSize);
    platform_error_t (*cancel)(platform_uart_t *uart,
                               platform_uart_direction_t direction);
} platform_uart_ops_t;

/*Platform UART 设备对象*/
struct platform_uart
{
    platform_device_t device;
    platform_uart_config_t config;
    const platform_uart_ops_t *ops;
    void *implContext;
    platform_uart_callback_t callback;
    void *callbackContext;
};

/**
 * @brief Platform UART 对象构造参数
 * @note lifecycle 和 ops 建议使用 static const，且必须至少有效至 deinit 完成
 * @note implContext 和 callbackContext 指向的对象必须至少有效至 deinit 完成
 */
typedef struct
{
    const char *name;
    uint32_t caps;
    platform_uart_config_t config;
    const platform_lifecycle_ops_t *lifecycle;
    const platform_uart_ops_t *ops;
    void *implContext;
    platform_uart_callback_t callback;
    void *callbackContext;
} platform_uart_init_params_t;

/**
 * @brief 构造 Platform UART 对象
 * @param[in,out] uart : 已使用 PLATFORM_UART_INITIALIZER 零初始化的 UART 对象
 * @param[in] params  : 配置、生命周期、Ops 和上下文
 * @return platform_error_t : 函数执行状态
 * @note 本函数只构造抽象对象，不初始化具体硬件
 * @note 同一 UART 对象只允许构造一次，重复调用返回已初始化错误
 */
platform_error_t platform_uart_init(platform_uart_t *uart,
                                    const platform_uart_init_params_t *params);

/**
 * @brief 绑定或解绑 Platform UART 异步事件回调
 * @param[in] uart            : 已构造且未处于 STARTED 状态的 UART 对象
 * @param[in] callback        : 事件回调，NULL 表示解绑
 * @param[in] callbackContext : 回调上下文，解绑时忽略并清空
 * @return platform_error_t : 函数执行状态
 * @note 仅允许在 CREATED、INITIALIZED、STOPPED 或 ERROR 状态调用
 */
platform_error_t platform_uart_set_callback(platform_uart_t *uart,
                                            platform_uart_callback_t callback,
                                            void *callbackContext);

/**
 * @brief 阻塞发送 UART 数据
 * @param[in] uart            : 已进入 STARTED 状态的 UART 对象
 * @param[in] data            : 发送缓冲区，函数返回前必须保持有效
 * @param[in] dataLength      : 发送字节数
 * @param[in] timeoutMs       : 超时时间，单位毫秒
 * @param[out] writtenLength  : 成功时为实际完成字节数，失败时为 0
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_write(platform_uart_t *uart,
                                     const uint8_t *data,
                                     platform_size_t dataLength,
                                     uint32_t timeoutMs,
                                     platform_size_t *writtenLength);

/**
 * @brief 阻塞接收 UART 数据
 * @param[in] uart         : 已进入 STARTED 状态的 UART 对象
 * @param[out] buffer      : 接收缓冲区，函数返回前必须保持有效
 * @param[in] bufferSize   : 接收缓冲区容量
 * @param[in] timeoutMs    : 超时时间，单位毫秒
 * @param[out] readLength  : 成功时为实际完成字节数，失败时为 0
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_read(platform_uart_t *uart,
                                    uint8_t *buffer,
                                    platform_size_t bufferSize,
                                    uint32_t timeoutMs,
                                    platform_size_t *readLength);

/**
 * @brief 启动异步 UART 发送
 * @param[in] uart       : 已进入 STARTED 状态的 UART 对象
 * @param[in] data       : 发送缓冲区，结束事件前必须有效且不得修改
 * @param[in] dataLength : 发送字节数
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_write_async(platform_uart_t *uart,
                                           const uint8_t *data,
                                           platform_size_t dataLength);

/**
 * @brief 启动异步 UART 接收
 * @param[in] uart       : 已进入 STARTED 状态的 UART 对象
 * @param[out] buffer    : 接收缓冲区，结束事件前必须保持有效
 * @param[in] bufferSize : 接收缓冲区容量
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_read_async(platform_uart_t *uart,
                                          uint8_t *buffer,
                                          platform_size_t bufferSize);

/**
 * @brief 取消指定方向的异步 UART 传输
 * @param[in] uart      : 已进入 STARTED 状态的 UART 对象
 * @param[in] direction : 待取消的传输方向
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_uart_cancel(platform_uart_t *uart,
                                      platform_uart_direction_t direction);

/**
 * @brief 由 Impl 向上层通知 UART 异步事件
 * @param[in] uart  : 产生事件的 UART 对象
 * @param[in] event : 事件数据，仅在回调期间有效
 * @return platform_error_t : 函数执行状态
 * @note 仅允许在 STARTED 状态通知事件，Impl 须在停止前关闭并排空事件源
 * @warning 用户回调可能在中断上下文执行，不得阻塞
 */
platform_error_t platform_uart_notify_event(platform_uart_t *uart,
                                             const platform_uart_event_t *event);
//******************************** Declaring *********************************//

#endif
