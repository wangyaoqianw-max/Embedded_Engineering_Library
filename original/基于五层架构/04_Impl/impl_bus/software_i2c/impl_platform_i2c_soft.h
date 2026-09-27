/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_i2c_soft.h
 * @brief 基于 Platform GPIO 的 Software I2C Backend
 * @author YaoQian Wang
 * @date 2026-09-27
 * @version V1.0
 *
 *****************************************************************************/

#ifndef IMPL_PLATFORM_I2C_SOFT_H
#define IMPL_PLATFORM_I2C_SOFT_H

#include "platform_gpio.h"
#include "platform_i2c.h"

#define IMPL_PLATFORM_I2C_SOFT_DEFAULT_HALF_PERIOD_US (5U)
#define IMPL_PLATFORM_I2C_SOFT_DEFAULT_SCL_TIMEOUT_US (100U)

typedef struct
{
    uint32_t halfPeriodUs;
    uint32_t sclTimeoutUs;
} impl_platform_i2c_soft_config_t;

typedef struct
{
    platform_gpio_t *scl;
    platform_gpio_t *sda;
    uint32_t halfPeriodUs;
    uint32_t sclTimeoutUs;
} impl_platform_i2c_soft_context_t;

/**
 * @brief 使用来源工程已验证的默认时序构造 Software I2C
 */
platform_error_t impl_platform_i2c_soft_construct(
    platform_i2c_t *i2c,
    const char *name,
    platform_gpio_t *scl,
    platform_gpio_t *sda,
    impl_platform_i2c_soft_context_t *context);

/**
 * @brief 使用实例级时序构造 Software I2C
 */
platform_error_t impl_platform_i2c_soft_construct_with_config(
    platform_i2c_t *i2c,
    const char *name,
    platform_gpio_t *scl,
    platform_gpio_t *sda,
    const impl_platform_i2c_soft_config_t *config,
    impl_platform_i2c_soft_context_t *context);

#endif
