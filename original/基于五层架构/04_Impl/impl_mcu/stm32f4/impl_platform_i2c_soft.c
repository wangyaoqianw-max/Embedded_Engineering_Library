/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_i2c_soft.c
 * @brief STM32F4 Software I2C Platform Adapter
 * @author YaoQian Wang
 * @date 2026-09-27
 * @version V1.0
 *
 *****************************************************************************/

#include "impl_platform_i2c_soft.h"

#include "impl_platform_delay.h"
#include "platform_def.h"

#include <stddef.h>

static platform_bool_t impl_i2c_soft_is_single_pin(uint16_t pin)
{
    if (pin == 0U) {
        return PLATFORM_FALSE;
    }

    return ((pin & (uint16_t)(pin - 1U)) == 0U) ?
        PLATFORM_TRUE : PLATFORM_FALSE;
}

static bool impl_i2c_soft_set_sda(void *opaque, bool releaseHigh)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)opaque;

    HAL_GPIO_WritePin(
        context->config.sdaPort,
        context->config.sdaPin,
        releaseHigh ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return true;
}

static bool impl_i2c_soft_set_scl(void *opaque, bool releaseHigh)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)opaque;

    HAL_GPIO_WritePin(
        context->config.sclPort,
        context->config.sclPin,
        releaseHigh ? GPIO_PIN_SET : GPIO_PIN_RESET);
    return true;
}

static bool impl_i2c_soft_get_sda(void *opaque, bool *isHigh)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)opaque;

    if (isHigh == NULL) {
        return false;
    }

    *isHigh = HAL_GPIO_ReadPin(
        context->config.sdaPort,
        context->config.sdaPin) == GPIO_PIN_SET;
    return true;
}

static bool impl_i2c_soft_get_scl(void *opaque, bool *isHigh)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)opaque;

    if (isHigh == NULL) {
        return false;
    }

    *isHigh = HAL_GPIO_ReadPin(
        context->config.sclPort,
        context->config.sclPin) == GPIO_PIN_SET;
    return true;
}

static void impl_i2c_soft_delay_us(void *opaque, uint32_t us)
{
    (void)opaque;
    impl_platform_delay_us(us);
}

static platform_error_t impl_i2c_soft_map_status(
    software_i2c_status_t status)
{
    switch (status) {
        case SOFTWARE_I2C_OK:
            return PLATFORM_ERR_OK;

        case SOFTWARE_I2C_ERR_INVALID_PARAM:
            return PLATFORM_ERR_INVALID_PARAM;

        case SOFTWARE_I2C_ERR_TIMEOUT:
            return PLATFORM_ERR_TIMEOUT;

        case SOFTWARE_I2C_ERR_BUSY:
            return PLATFORM_ERR_BUSY;

        case SOFTWARE_I2C_ERR_ADDRESS_NACK:
            return PLATFORM_ERR_NOT_FOUND;

        case SOFTWARE_I2C_ERR_DATA_NACK:
        case SOFTWARE_I2C_ERR_IO:
            return PLATFORM_ERR_IO;

        default:
            return PLATFORM_ERR_UNKNOWN;
    }
}

static platform_error_t impl_i2c_soft_init(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context;
    GPIO_InitTypeDef gpioInit = {0};
    software_i2c_status_t status;

    if ((i2c == NULL) || (i2c->implContext == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    context = (impl_platform_i2c_soft_context_t *)i2c->implContext;

    /*
     * 先把输出寄存器置 HIGH，再切换为开漏输出，降低初始化瞬间误拉低总线的风险。
     * GPIO Port Clock 必须由目标工程在调用本函数前开启。
     */
    HAL_GPIO_WritePin(
        context->config.sclPort,
        context->config.sclPin,
        GPIO_PIN_SET);
    HAL_GPIO_WritePin(
        context->config.sdaPort,
        context->config.sdaPin,
        GPIO_PIN_SET);

    gpioInit.Mode = GPIO_MODE_OUTPUT_OD;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_LOW;

    gpioInit.Pin = context->config.sclPin;
    HAL_GPIO_Init(context->config.sclPort, &gpioInit);

    gpioInit.Pin = context->config.sdaPin;
    HAL_GPIO_Init(context->config.sdaPort, &gpioInit);

    status = software_i2c_init(
        &context->core,
        &context->port,
        context->config.halfPeriodUs,
        context->config.sclTimeoutUs);

    if (status != SOFTWARE_I2C_OK) {
        HAL_GPIO_DeInit(
            context->config.sdaPort,
            context->config.sdaPin);
        HAL_GPIO_DeInit(
            context->config.sclPort,
            context->config.sclPin);
    }

    return impl_i2c_soft_map_status(status);
}

static platform_error_t impl_i2c_soft_probe(
    platform_i2c_t *i2c,
    uint8_t address)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)i2c->implContext;

    return impl_i2c_soft_map_status(
        software_i2c_probe(&context->core, address));
}

static platform_error_t impl_i2c_soft_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)i2c->implContext;

    return impl_i2c_soft_map_status(
        software_i2c_write(&context->core, address, data, length));
}

static platform_error_t impl_i2c_soft_read(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t *data,
    uint16_t length)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)i2c->implContext;

    return impl_i2c_soft_map_status(
        software_i2c_read(&context->core, address, data, length));
}

static platform_error_t impl_i2c_soft_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)i2c->implContext;

    return impl_i2c_soft_map_status(
        software_i2c_write_read(
            &context->core,
            address,
            txData,
            txLength,
            rxData,
            rxLength));
}

static platform_error_t impl_i2c_soft_deinit(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)i2c->implContext;
    software_i2c_status_t status =
        software_i2c_deinit(&context->core);

    HAL_GPIO_DeInit(
        context->config.sdaPort,
        context->config.sdaPin);
    HAL_GPIO_DeInit(
        context->config.sclPort,
        context->config.sclPin);

    return impl_i2c_soft_map_status(status);
}

static const platform_i2c_ops_t g_implI2cSoftOps = {
    impl_i2c_soft_init,
    impl_i2c_soft_probe,
    impl_i2c_soft_write,
    impl_i2c_soft_read,
    impl_i2c_soft_write_read,
    impl_i2c_soft_deinit
};

platform_error_t impl_platform_i2c_soft_construct(
    platform_i2c_t *i2c,
    const char *name,
    const impl_platform_i2c_soft_config_t *config,
    impl_platform_i2c_soft_context_t *context)
{
    platform_i2c_init_params_t params;

    if ((i2c == NULL) || (config == NULL) || (context == NULL) ||
        (config->sclPort == NULL) || (config->sdaPort == NULL) ||
        (impl_i2c_soft_is_single_pin(config->sclPin) != PLATFORM_TRUE) ||
        (impl_i2c_soft_is_single_pin(config->sdaPin) != PLATFORM_TRUE) ||
        (config->halfPeriodUs == 0U) ||
        (config->sclTimeoutUs == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    context->config = *config;
    context->port.context = context;
    context->port.set_sda = impl_i2c_soft_set_sda;
    context->port.set_scl = impl_i2c_soft_set_scl;
    context->port.get_sda = impl_i2c_soft_get_sda;
    context->port.get_scl = impl_i2c_soft_get_scl;
    context->port.delay_us = impl_i2c_soft_delay_us;
    context->core = (software_i2c_t)SOFTWARE_I2C_INITIALIZER;

    params.name = name;
    params.ops = &g_implI2cSoftOps;
    params.implContext = context;

    return platform_i2c_init(i2c, &params);
}
