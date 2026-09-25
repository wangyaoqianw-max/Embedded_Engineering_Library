/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_gpio_types.h
 * @brief Platform GPIO 公共数据类型
 * @author YaoQian Wang
 * @date 2026-09-01
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_GPIO_TYPES_H
#define PLATFORM_GPIO_TYPES_H

//******************************** Includes *********************************//
#include "platform_types.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
typedef struct platform_gpio platform_gpio_t;

/*GPIO 逻辑电平*/
typedef enum
{
    PLATFORM_GPIO_LEVEL_LOW = 0,
    PLATFORM_GPIO_LEVEL_HIGH,
    PLATFORM_GPIO_LEVEL_MAX
} platform_gpio_level_t;

/*GPIO 方向*/
typedef enum
{
    PLATFORM_GPIO_DIRECTION_INPUT = 0,
    PLATFORM_GPIO_DIRECTION_OUTPUT,
    PLATFORM_GPIO_DIRECTION_MAX
} platform_gpio_direction_t;

/*GPIO 上下拉*/
typedef enum
{
    PLATFORM_GPIO_PULL_NONE = 0,
    PLATFORM_GPIO_PULL_UP,
    PLATFORM_GPIO_PULL_DOWN,
    PLATFORM_GPIO_PULL_MAX
} platform_gpio_pull_t;

/*GPIO 输出类型*/
typedef enum
{
    PLATFORM_GPIO_OUTPUT_PUSH_PULL = 0,
    PLATFORM_GPIO_OUTPUT_OPEN_DRAIN,
    PLATFORM_GPIO_OUTPUT_MAX
} platform_gpio_output_type_t;

/*GPIO 静态配置*/
typedef struct
{
    platform_gpio_direction_t direction;
    platform_gpio_pull_t pull;
    platform_gpio_output_type_t outputType;
    platform_gpio_level_t initialLevel;
} platform_gpio_config_t;
//******************************** Declaring *********************************//

#endif
