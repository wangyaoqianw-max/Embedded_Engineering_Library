/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_uart.h
 * @brief STM32F4 HAL UART Platform 构造入口
 * @author YaoQian Wang
 * @date 2026-08-29
 * @version V1.1
 *
 *****************************************************************************/

#ifndef IMPL_PLATFORM_UART_H
#define IMPL_PLATFORM_UART_H

//******************************** Includes *********************************//
#include "platform_uart.h"
#include "stm32f4xx_hal.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define IMPL_PLATFORM_UART_CONTEXT_INITIALIZER {0}
//******************************** Defines **********************************//

//******************************** Types ************************************//
typedef struct
{
    UART_HandleTypeDef *halUart;
    platform_uart_t *platformUart;
    uint8_t *rxBuffer;
    platform_size_t rxBufferSize;
    platform_size_t rxLastPosition;
    platform_bool_t rxActive;
    const uint8_t *txBuffer;
    platform_size_t txBufferSize;
    platform_bool_t txActive;
} impl_platform_uart_context_t;
//******************************** Types ************************************//

//******************************** Functions ********************************//
/**
 * @brief 构造并绑定一个 STM32F4 HAL UART Handle
 * @param[in,out] uart : Platform UART 对象
 * @param[in] name : Platform UART 名称
 * @param[in] caps : 设备能力标志
 * @param[in] config : UART 静态配置
 * @param[in] callback : 可选异步事件回调
 * @param[in] callbackContext : 可选回调上下文
 * @param[in,out] context : 调用者持有的 STM32F4 UART Impl 上下文
 * @return platform_error_t : 构造结果；不依赖 CubeMX 全局 huart 符号
 * @note context 与 halUart 在 UART 对象生命周期内必须持续有效。
 */
platform_error_t impl_platform_uart_construct(
    platform_uart_t *uart,
    const char *name,
    uint32_t caps,
    const platform_uart_config_t *config,
    platform_uart_callback_t callback,
    void *callbackContext,
    impl_platform_uart_context_t *context);
//******************************** Functions ********************************//

#endif
