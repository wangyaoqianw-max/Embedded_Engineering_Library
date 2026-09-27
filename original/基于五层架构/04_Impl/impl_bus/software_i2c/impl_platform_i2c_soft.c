/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_i2c_soft.c
 * @brief 基于 Platform GPIO 的 Software I2C Backend 实现
 * @author YaoQian Wang
 * @date 2026-09-27
 * @version V1.0
 *
 *****************************************************************************/

#include "impl_platform_i2c_soft.h"

#include "platform_def.h"

#include <stddef.h>

#define IMPL_I2C_RECOVERY_CLOCK_COUNT (9U)
#define IMPL_I2C_SCL_WAIT_STEP_US      (1U)

static platform_error_t impl_i2c_fail_transaction(
    platform_i2c_t *i2c,
    platform_error_t originalError);

static platform_error_t impl_i2c_get_context(
    platform_i2c_t *i2c,
    impl_platform_i2c_soft_context_t **context)
{
    if ((i2c == NULL) || (context == NULL) || (i2c->implContext == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    *context = (impl_platform_i2c_soft_context_t *)i2c->implContext;
    if (((*context)->scl == NULL) || ((*context)->sda == NULL) ||
        ((*context)->halfPeriodUs == 0U) ||
        ((*context)->sclTimeoutUs == 0U)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_sda_low(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_error_t result = impl_i2c_get_context(i2c, &context);

    return (result == PLATFORM_ERR_OK) ?
        platform_gpio_write(context->sda, PLATFORM_GPIO_LEVEL_LOW) : result;
}

static platform_error_t impl_i2c_sda_release(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_error_t result = impl_i2c_get_context(i2c, &context);

    return (result == PLATFORM_ERR_OK) ?
        platform_gpio_write(context->sda, PLATFORM_GPIO_LEVEL_HIGH) : result;
}

static platform_error_t impl_i2c_sda_read(
    platform_i2c_t *i2c,
    platform_gpio_level_t *level)
{
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_error_t result = impl_i2c_get_context(i2c, &context);

    return (result == PLATFORM_ERR_OK) ?
        platform_gpio_read(context->sda, level) : result;
}

static platform_error_t impl_i2c_scl_low(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_error_t result = impl_i2c_get_context(i2c, &context);

    return (result == PLATFORM_ERR_OK) ?
        platform_gpio_write(context->scl, PLATFORM_GPIO_LEVEL_LOW) : result;
}

static platform_error_t impl_i2c_scl_release(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_error_t result = impl_i2c_get_context(i2c, &context);

    return (result == PLATFORM_ERR_OK) ?
        platform_gpio_write(context->scl, PLATFORM_GPIO_LEVEL_HIGH) : result;
}

static platform_error_t impl_i2c_scl_read(
    platform_i2c_t *i2c,
    platform_gpio_level_t *level)
{
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_error_t result = impl_i2c_get_context(i2c, &context);

    return (result == PLATFORM_ERR_OK) ?
        platform_gpio_read(context->scl, level) : result;
}

static void impl_i2c_delay_half_period(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context =
        (impl_platform_i2c_soft_context_t *)i2c->implContext;

    platform_delay_us(context->halfPeriodUs);
}

static platform_error_t impl_i2c_wait_scl_high(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_LOW;
    uint32_t waitedUs = 0U;
    platform_error_t result = impl_i2c_get_context(i2c, &context);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_scl_release(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    while (waitedUs < context->sclTimeoutUs) {
        result = impl_i2c_scl_read(i2c, &level);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        if (level == PLATFORM_GPIO_LEVEL_HIGH) {
            return PLATFORM_ERR_OK;
        }

        platform_delay_us(IMPL_I2C_SCL_WAIT_STEP_US);
        waitedUs += IMPL_I2C_SCL_WAIT_STEP_US;
    }

    return PLATFORM_ERR_TIMEOUT;
}

static platform_error_t impl_i2c_check_bus_idle(platform_i2c_t *i2c)
{
    platform_gpio_level_t sdaLevel = PLATFORM_GPIO_LEVEL_LOW;
    platform_error_t result = impl_i2c_wait_scl_high(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_sda_read(i2c, &sdaLevel);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return (sdaLevel == PLATFORM_GPIO_LEVEL_HIGH) ?
        PLATFORM_ERR_OK : PLATFORM_ERR_BUSY;
}

static platform_error_t impl_i2c_start(platform_i2c_t *i2c)
{
    platform_error_t result = impl_i2c_sda_release(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    result = impl_i2c_sda_low(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    return impl_i2c_scl_low(i2c);
}

static platform_error_t impl_i2c_stop(platform_i2c_t *i2c)
{
    platform_error_t result = impl_i2c_sda_low(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    result = impl_i2c_sda_release(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_write_bit(
    platform_i2c_t *i2c,
    platform_gpio_level_t level)
{
    platform_error_t result = impl_i2c_scl_low(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = (level == PLATFORM_GPIO_LEVEL_LOW) ?
        impl_i2c_sda_low(i2c) : impl_i2c_sda_release(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    result = impl_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    result = impl_i2c_scl_low(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_read_bit(
    platform_i2c_t *i2c,
    platform_gpio_level_t *level)
{
    platform_error_t result = impl_i2c_scl_low(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_sda_release(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    result = impl_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    result = impl_i2c_sda_read(i2c, level);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_scl_low(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_wait_ack(
    platform_i2c_t *i2c,
    platform_bool_t *isAcknowledged)
{
    platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_HIGH;
    platform_error_t result = impl_i2c_sda_release(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    result = impl_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    result = impl_i2c_sda_read(i2c, &level);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_scl_low(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    impl_i2c_delay_half_period(i2c);

    *isAcknowledged = (level == PLATFORM_GPIO_LEVEL_LOW) ?
        PLATFORM_TRUE : PLATFORM_FALSE;
    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_write_byte(
    platform_i2c_t *i2c,
    uint8_t data,
    platform_bool_t *isAcknowledged)
{
    uint8_t mask = 0x80U;
    platform_error_t result;

    while (mask != 0U) {
        platform_gpio_level_t level =
            ((data & mask) != 0U) ?
            PLATFORM_GPIO_LEVEL_HIGH : PLATFORM_GPIO_LEVEL_LOW;

        result = impl_i2c_write_bit(i2c, level);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        mask >>= 1U;
    }

    return impl_i2c_wait_ack(i2c, isAcknowledged);
}

static platform_error_t impl_i2c_send_ack(
    platform_i2c_t *i2c,
    platform_bool_t acknowledge)
{
    return impl_i2c_write_bit(
        i2c,
        (acknowledge != PLATFORM_FALSE) ?
        PLATFORM_GPIO_LEVEL_LOW : PLATFORM_GPIO_LEVEL_HIGH);
}

static platform_error_t impl_i2c_read_byte(
    platform_i2c_t *i2c,
    uint8_t *data,
    platform_bool_t acknowledge)
{
    uint8_t value = 0U;
    uint8_t bitIndex;
    platform_error_t result;

    for (bitIndex = 0U; bitIndex < 8U; bitIndex++) {
        platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_LOW;

        result = impl_i2c_read_bit(i2c, &level);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        value <<= 1U;
        if (level == PLATFORM_GPIO_LEVEL_HIGH) {
            value |= 1U;
        }
    }

    result = impl_i2c_send_ack(i2c, acknowledge);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *data = value;
    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_send_address(
    platform_i2c_t *i2c,
    uint8_t address,
    platform_bool_t isRead)
{
    platform_bool_t isAcknowledged = PLATFORM_FALSE;
    uint8_t addressByte = (uint8_t)(address << 1U);
    platform_error_t result;

    if (isRead != PLATFORM_FALSE) {
        addressByte |= 1U;
    }

    result = impl_i2c_write_byte(i2c, addressByte, &isAcknowledged);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return (isAcknowledged != PLATFORM_FALSE) ?
        PLATFORM_ERR_OK : PLATFORM_ERR_NOT_FOUND;
}

static platform_error_t impl_i2c_send_data(
    platform_i2c_t *i2c,
    const uint8_t *data,
    uint16_t length)
{
    platform_bool_t isAcknowledged = PLATFORM_FALSE;
    uint16_t index;
    platform_error_t result;

    for (index = 0U; index < length; index++) {
        result = impl_i2c_write_byte(i2c, data[index], &isAcknowledged);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        if (isAcknowledged == PLATFORM_FALSE) {
            return PLATFORM_ERR_IO;
        }
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_receive_data(
    platform_i2c_t *i2c,
    uint8_t *data,
    uint16_t length)
{
    uint16_t index;
    platform_error_t result;

    for (index = 0U; index < length; index++) {
        platform_bool_t acknowledge =
            (index == (uint16_t)(length - 1U)) ?
            PLATFORM_FALSE : PLATFORM_TRUE;

        result = impl_i2c_read_byte(i2c, &data[index], acknowledge);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_begin_transaction(platform_i2c_t *i2c)
{
    platform_error_t result = impl_i2c_check_bus_idle(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_start(i2c);
    if (result != PLATFORM_ERR_OK) {
        return impl_i2c_fail_transaction(i2c, result);
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t impl_i2c_release_bus(platform_i2c_t *i2c)
{
    platform_error_t firstError = impl_i2c_wait_scl_high(i2c);
    platform_error_t result = impl_i2c_sda_release(i2c);

    if ((firstError == PLATFORM_ERR_OK) && (result != PLATFORM_ERR_OK)) {
        firstError = result;
    }

    return firstError;
}

static platform_error_t impl_i2c_end_transaction(platform_i2c_t *i2c)
{
    platform_error_t cleanupResult = impl_i2c_stop(i2c);

    if (cleanupResult != PLATFORM_ERR_OK) {
        (void)impl_i2c_release_bus(i2c);
    }

    return cleanupResult;
}

static platform_error_t impl_i2c_fail_transaction(
    platform_i2c_t *i2c,
    platform_error_t originalError)
{
    (void)impl_i2c_end_transaction(i2c);
    return originalError;
}

static platform_error_t impl_i2c_bus_recover(platform_i2c_t *i2c)
{
    platform_gpio_level_t sdaLevel = PLATFORM_GPIO_LEVEL_LOW;
    uint32_t clockIndex;
    platform_error_t result = impl_i2c_sda_release(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    for (clockIndex = 0U;
         clockIndex < IMPL_I2C_RECOVERY_CLOCK_COUNT;
         clockIndex++) {
        result = impl_i2c_scl_low(i2c);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        impl_i2c_delay_half_period(i2c);

        result = impl_i2c_wait_scl_high(i2c);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        impl_i2c_delay_half_period(i2c);

        result = impl_i2c_sda_read(i2c, &sdaLevel);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        if (sdaLevel == PLATFORM_GPIO_LEVEL_HIGH) {
            break;
        }
    }

    result = impl_i2c_stop(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return impl_i2c_check_bus_idle(i2c);
}

static platform_error_t impl_i2c_soft_init(platform_i2c_t *i2c)
{
    const platform_gpio_config_t gpioConfig = {
        PLATFORM_GPIO_DIRECTION_OUTPUT,
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_OUTPUT_OPEN_DRAIN,
        PLATFORM_GPIO_LEVEL_HIGH
    };
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_gpio_level_t sdaLevel = PLATFORM_GPIO_LEVEL_LOW;
    platform_error_t result = impl_i2c_get_context(i2c, &context);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_gpio_configure(context->scl, &gpioConfig);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_gpio_configure(context->sda, &gpioConfig);
    if (result != PLATFORM_ERR_OK) {
        (void)platform_gpio_deinit(context->scl);
        return result;
    }

    result = impl_i2c_sda_release(i2c);
    if (result == PLATFORM_ERR_OK) {
        result = impl_i2c_wait_scl_high(i2c);
    }
    if (result == PLATFORM_ERR_OK) {
        result = impl_i2c_sda_read(i2c, &sdaLevel);
    }
    if ((result == PLATFORM_ERR_OK) &&
        (sdaLevel == PLATFORM_GPIO_LEVEL_LOW)) {
        result = impl_i2c_bus_recover(i2c);
    }
    if (result != PLATFORM_ERR_OK) {
        (void)platform_gpio_deinit(context->sda);
        (void)platform_gpio_deinit(context->scl);
    }

    return result;
}

static platform_error_t impl_i2c_soft_probe(
    platform_i2c_t *i2c,
    uint8_t address)
{
    platform_error_t result = impl_i2c_begin_transaction(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_send_address(i2c, address, PLATFORM_FALSE);
    if (result != PLATFORM_ERR_OK) {
        return impl_i2c_fail_transaction(i2c, result);
    }

    return impl_i2c_end_transaction(i2c);
}

static platform_error_t impl_i2c_soft_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length)
{
    platform_error_t result = impl_i2c_begin_transaction(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_send_address(i2c, address, PLATFORM_FALSE);
    if (result == PLATFORM_ERR_OK) {
        result = impl_i2c_send_data(i2c, data, length);
    }
    if (result != PLATFORM_ERR_OK) {
        return impl_i2c_fail_transaction(i2c, result);
    }

    return impl_i2c_end_transaction(i2c);
}

static platform_error_t impl_i2c_soft_read(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t *data,
    uint16_t length)
{
    platform_error_t result = impl_i2c_begin_transaction(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_send_address(i2c, address, PLATFORM_TRUE);
    if (result == PLATFORM_ERR_OK) {
        result = impl_i2c_receive_data(i2c, data, length);
    }
    if (result != PLATFORM_ERR_OK) {
        return impl_i2c_fail_transaction(i2c, result);
    }

    return impl_i2c_end_transaction(i2c);
}

static platform_error_t impl_i2c_soft_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength)
{
    platform_error_t result = impl_i2c_begin_transaction(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = impl_i2c_send_address(i2c, address, PLATFORM_FALSE);
    if (result == PLATFORM_ERR_OK) {
        result = impl_i2c_send_data(i2c, txData, txLength);
    }
    if (result != PLATFORM_ERR_OK) {
        return impl_i2c_fail_transaction(i2c, result);
    }

    result = impl_i2c_start(i2c);
    if (result != PLATFORM_ERR_OK) {
        return impl_i2c_fail_transaction(i2c, result);
    }

    result = impl_i2c_send_address(i2c, address, PLATFORM_TRUE);
    if (result == PLATFORM_ERR_OK) {
        result = impl_i2c_receive_data(i2c, rxData, rxLength);
    }
    if (result != PLATFORM_ERR_OK) {
        return impl_i2c_fail_transaction(i2c, result);
    }

    return impl_i2c_end_transaction(i2c);
}

static platform_error_t impl_i2c_soft_deinit(platform_i2c_t *i2c)
{
    impl_platform_i2c_soft_context_t *context = NULL;
    platform_error_t result = impl_i2c_get_context(i2c, &context);
    platform_error_t firstError;

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    firstError = impl_i2c_release_bus(i2c);

    result = platform_gpio_deinit(context->sda);
    if ((result != PLATFORM_ERR_OK) && (firstError == PLATFORM_ERR_OK)) {
        firstError = result;
    }

    result = platform_gpio_deinit(context->scl);
    if ((result != PLATFORM_ERR_OK) && (firstError == PLATFORM_ERR_OK)) {
        firstError = result;
    }

    return firstError;
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
    platform_gpio_t *scl,
    platform_gpio_t *sda,
    impl_platform_i2c_soft_context_t *context)
{
    const impl_platform_i2c_soft_config_t config = {
        IMPL_PLATFORM_I2C_SOFT_DEFAULT_HALF_PERIOD_US,
        IMPL_PLATFORM_I2C_SOFT_DEFAULT_SCL_TIMEOUT_US
    };

    return impl_platform_i2c_soft_construct_with_config(
        i2c,
        name,
        scl,
        sda,
        &config,
        context);
}

platform_error_t impl_platform_i2c_soft_construct_with_config(
    platform_i2c_t *i2c,
    const char *name,
    platform_gpio_t *scl,
    platform_gpio_t *sda,
    const impl_platform_i2c_soft_config_t *config,
    impl_platform_i2c_soft_context_t *context)
{
    platform_i2c_init_params_t params;

    if ((i2c == NULL) || (scl == NULL) || (sda == NULL) ||
        (config == NULL) || (context == NULL) ||
        (config->halfPeriodUs == 0U) ||
        (config->sclTimeoutUs == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    context->scl = scl;
    context->sda = sda;
    context->halfPeriodUs = config->halfPeriodUs;
    context->sclTimeoutUs = config->sclTimeoutUs;

    params.name = name;
    params.ops = &g_implI2cSoftOps;
    params.implContext = context;

    return platform_i2c_init(i2c, &params);
}
