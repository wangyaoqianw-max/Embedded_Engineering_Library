/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_gpio.h
 * @brief STM32 通用 GPIO 的 Platform Impl 构造接口
 * @author YaoQian Wang
 * @date 2026-09-01
 * @version V1.0
 *
 *****************************************************************************/

#ifndef IMPL_PLATFORM_GPIO_H
#define IMPL_PLATFORM_GPIO_H

//******************************** Includes *********************************//
#include "platform_gpio.h"

#include "stm32f4xx_hal.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
/**
 * @brief STM32 GPIO Impl 的调用者持有上下文
 * @note context 只引用一个 GPIO Port 和一个物理 Pin，不由 Impl 分配或释放。
 */
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} impl_platform_gpio_context_t;

/**
 * @brief 构造并绑定通用 STM32 GPIO Platform 对象
 * @param[in,out] gpio    : 使用 PLATFORM_GPIO_INITIALIZER 清零的 GPIO 对象
 * @param[in] name        : Platform GPIO 名称，可为 NULL
 * @param[in] context     : 调用者持有的 Port + 单 Pin 上下文
 * @return platform_error_t : 构造结果；本函数不执行 HAL GPIO 操作
 */
platform_error_t impl_platform_gpio_construct(
    platform_gpio_t *gpio,
    const char *name,
    impl_platform_gpio_context_t *context);
//******************************** Declaring *********************************//

#endif
