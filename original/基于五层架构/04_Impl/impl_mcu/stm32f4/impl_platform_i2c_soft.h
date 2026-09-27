/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_i2c_soft.h
 * @brief STM32F4 Software I2C Platform Adapter
 * @author YaoQian Wang
 * @date 2026-09-27
 * @version V1.0
 *
 *****************************************************************************/

#ifndef IMPL_PLATFORM_I2C_SOFT_H
#define IMPL_PLATFORM_I2C_SOFT_H

#include "platform_i2c.h"
#include "software_i2c_core.h"

#include "stm32f4xx_hal.h"

#define IMPL_PLATFORM_I2C_SOFT_DEFAULT_HALF_PERIOD_US (5U)
#define IMPL_PLATFORM_I2C_SOFT_DEFAULT_SCL_TIMEOUT_US (100U)

typedef struct
{
    GPIO_TypeDef *sclPort;
    uint16_t sclPin;
    GPIO_TypeDef *sdaPort;
    uint16_t sdaPin;
    uint32_t halfPeriodUs;
    uint32_t sclTimeoutUs;
} impl_platform_i2c_soft_config_t;

typedef struct
{
    impl_platform_i2c_soft_config_t config;
    software_i2c_port_t port;
    software_i2c_t core;
} impl_platform_i2c_soft_context_t;

#define IMPL_PLATFORM_I2C_SOFT_CONTEXT_INITIALIZER {0}

platform_error_t impl_platform_i2c_soft_construct(
    platform_i2c_t *i2c,
    const char *name,
    const impl_platform_i2c_soft_config_t *config,
    impl_platform_i2c_soft_context_t *context);

#endif
