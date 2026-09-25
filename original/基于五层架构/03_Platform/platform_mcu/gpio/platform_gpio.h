/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_gpio.h
 * @brief Platform GPIO 轻量对象和公共接口
 * @author YaoQian Wang
 * @date 2026-09-01
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_GPIO_H
#define PLATFORM_GPIO_H

//******************************** Includes *********************************//
#include "platform_gpio_types.h"
#include "platform_error.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/*首次构造前使用此宏初始化 GPIO 对象存储*/
#define PLATFORM_GPIO_INITIALIZER {0}
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/**
 * @brief Platform GPIO 数据操作表，由 Impl 层注入
 */
typedef struct
{
    platform_error_t (*configure)(platform_gpio_t *gpio,
                                  const platform_gpio_config_t *config);
    platform_error_t (*write)(platform_gpio_t *gpio,
                              platform_gpio_level_t level);
    platform_error_t (*read)(platform_gpio_t *gpio,
                             platform_gpio_level_t *level);
    platform_error_t (*deinit)(platform_gpio_t *gpio);
} platform_gpio_ops_t;

/*Platform GPIO 轻量资源对象*/
struct platform_gpio
{
    const char *name;
    platform_gpio_config_t config;
    const platform_gpio_ops_t *ops;
    void *implContext;
    platform_bool_t initialized;
    platform_bool_t configured;
};

/**
 * @brief Platform GPIO 对象构造参数
 * @note ops 和 implContext 指向的对象必须至少有效至对象不再使用。
 */
typedef struct
{
    const char *name;
    const platform_gpio_ops_t *ops;
    void *implContext;
} platform_gpio_init_params_t;

/**
 * @brief 构造 Platform GPIO 对象
 * @param[in,out] gpio   : 使用 PLATFORM_GPIO_INITIALIZER 零初始化的 GPIO 对象
 * @param[in] params     : 名称、Ops 和实现上下文
 * @return platform_error_t : 函数执行状态
 * @note 本函数只构造抽象对象，不初始化具体 GPIO 硬件。
 * @note 同一 GPIO 对象只允许构造一次，重复调用返回已初始化错误。
 */
platform_error_t platform_gpio_init(
    platform_gpio_t *gpio,
    const platform_gpio_init_params_t *params);

/**
 * @brief 应用 GPIO 硬件配置
 * @param[in,out] gpio       : 已构造的 GPIO 对象
 * @param[in] config         : GPIO 配置
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_gpio_configure(
    platform_gpio_t *gpio,
    const platform_gpio_config_t *config);

/**
 * @brief 写入 GPIO 逻辑电平
 * @param[in,out] gpio       : 已配置为输出的 GPIO 对象
 * @param[in] level          : 待写入的逻辑电平
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_gpio_write(
    platform_gpio_t *gpio,
    platform_gpio_level_t level);

/**
 * @brief 读取 GPIO 实际逻辑电平
 * @param[in] gpio            : 已完成配置的 GPIO 对象
 * @param[out] level          : 读取到的逻辑电平
 * @return platform_error_t  : 函数执行状态
 */
platform_error_t platform_gpio_read(
    platform_gpio_t *gpio,
    platform_gpio_level_t *level);

/**
 * @brief 反配置 GPIO 硬件
 * @param[in,out] gpio       : 已完成配置的 GPIO 对象
 * @return platform_error_t : 函数执行状态
 * @note 本函数不销毁 Platform GPIO 对象；成功后仍保持 initialized 状态。
 */
platform_error_t platform_gpio_deinit(platform_gpio_t *gpio);
//******************************** Declaring *********************************//

#endif
