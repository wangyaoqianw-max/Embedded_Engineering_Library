/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_i2c.c
 * @brief Platform I2C 同步事务公共接口实现
 * @author YaoQian Wang
 * @date 2026-09-02
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_i2c.h"

#include "platform_def.h"
#include "project_config.h"

#include <stddef.h>
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_I2C_RECOVERY_CLOCK_COUNT       (9U)
#define PLATFORM_I2C_SCL_WAIT_STEP_US            (1U)
//******************************** Defines *********************************//

//******************************** Functions *********************************//
static platform_error_t platform_i2c_fail_transaction(
    platform_i2c_t *i2c,
    platform_error_t originalError);

static platform_error_t platform_i2c_sda_low(platform_i2c_t *i2c)
{
    return platform_gpio_write(i2c->sda, PLATFORM_GPIO_LEVEL_LOW);
}

static platform_error_t platform_i2c_sda_release(platform_i2c_t *i2c)
{
    return platform_gpio_write(i2c->sda, PLATFORM_GPIO_LEVEL_HIGH);
}

static platform_error_t platform_i2c_sda_read(
    platform_i2c_t *i2c,
    platform_gpio_level_t *level)
{
    return platform_gpio_read(i2c->sda, level);
}

static platform_error_t platform_i2c_scl_low(platform_i2c_t *i2c)
{
    return platform_gpio_write(i2c->scl, PLATFORM_GPIO_LEVEL_LOW);
}

static platform_error_t platform_i2c_scl_release(platform_i2c_t *i2c)
{
    return platform_gpio_write(i2c->scl, PLATFORM_GPIO_LEVEL_HIGH);
}

static platform_error_t platform_i2c_scl_read(
    platform_i2c_t *i2c,
    platform_gpio_level_t *level)
{
    return platform_gpio_read(i2c->scl, level);
}

/* 释放 SCL 后轮询物理电平，以支持从设备 clock stretching。 */
static platform_error_t platform_i2c_wait_scl_high(platform_i2c_t *i2c)
{
    platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_LOW;
    uint32_t waitedUs = 0U;
    platform_error_t result = platform_i2c_scl_release(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /* 开漏写 HIGH 只表示释放线路，必须读取物理电平确认 SCL 真正变高。 */
    while (waitedUs < PROJECT_SOFT_I2C_SCL_TIMEOUT_US) {
        result = platform_i2c_scl_read(i2c, &level);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        if (level == PLATFORM_GPIO_LEVEL_HIGH) {
            return PLATFORM_ERR_OK;
        }

        platform_delay_us(PLATFORM_I2C_SCL_WAIT_STEP_US);
        waitedUs += PLATFORM_I2C_SCL_WAIT_STEP_US;
    }

    return PLATFORM_ERR_TIMEOUT;
}

/* 仅当 SDA 和 SCL 均被上拉为高电平时，允许开始一笔新事务。 */
static platform_error_t platform_i2c_check_bus_idle(platform_i2c_t *i2c)
{
    platform_gpio_level_t sdaLevel = PLATFORM_GPIO_LEVEL_LOW;
    platform_error_t result = platform_i2c_wait_scl_high(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_sda_read(i2c, &sdaLevel);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if (sdaLevel != PLATFORM_GPIO_LEVEL_HIGH) {
        return PLATFORM_ERR_BUSY;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_start(platform_i2c_t *i2c)
{
    platform_error_t result = platform_i2c_sda_release(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    /* SCL 为 HIGH 时将 SDA 从释放态拉低，形成 START / Repeated START。 */
    result = platform_i2c_sda_low(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    return platform_i2c_scl_low(i2c);
}

static platform_error_t platform_i2c_stop(platform_i2c_t *i2c)
{
    platform_error_t result = platform_i2c_sda_low(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    /* SCL 为 HIGH 时释放 SDA，形成 STOP 并使总线回到 Idle。 */
    result = platform_i2c_sda_release(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_write_bit(
    platform_i2c_t *i2c,
    platform_gpio_level_t level)
{
    platform_error_t result = platform_i2c_scl_low(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (level == PLATFORM_GPIO_LEVEL_LOW) {
        result = platform_i2c_sda_low(i2c);
    } else {
        result = platform_i2c_sda_release(i2c);
    }
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    result = platform_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    result = platform_i2c_scl_low(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_read_bit(
    platform_i2c_t *i2c,
    platform_gpio_level_t *level)
{
    platform_error_t result = platform_i2c_scl_low(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_sda_release(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    result = platform_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    result = platform_i2c_sda_read(i2c, level);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_scl_low(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_wait_ack(
    platform_i2c_t *i2c,
    platform_bool_t *isAcknowledged)
{
    platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_HIGH;
    platform_error_t result = platform_i2c_sda_release(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    result = platform_i2c_wait_scl_high(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    result = platform_i2c_sda_read(i2c, &level);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_scl_low(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

    *isAcknowledged = (level == PLATFORM_GPIO_LEVEL_LOW) ?
        PLATFORM_TRUE : PLATFORM_FALSE;
    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_write_byte(
    platform_i2c_t *i2c,
    uint8_t data,
    platform_bool_t *isAcknowledged)
{
    uint8_t mask = 0x80U;
    platform_error_t result = PLATFORM_ERR_OK;

    while (mask != 0U) {
        platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_LOW;

        if ((data & mask) != 0U) {
            level = PLATFORM_GPIO_LEVEL_HIGH;
        }

        result = platform_i2c_write_bit(i2c, level);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        mask >>= 1U;
    }

    return platform_i2c_wait_ack(i2c, isAcknowledged);
}

static platform_error_t platform_i2c_send_ack(
    platform_i2c_t *i2c,
    platform_bool_t acknowledge)
{
    platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_HIGH;

    if (acknowledge != PLATFORM_FALSE) {
        level = PLATFORM_GPIO_LEVEL_LOW;
    }

    return platform_i2c_write_bit(i2c, level);
}

static platform_error_t platform_i2c_read_byte(
    platform_i2c_t *i2c,
    uint8_t *data,
    platform_bool_t acknowledge)
{
    uint8_t value = 0U;
    uint8_t bitIndex = 0U;
    platform_error_t result = PLATFORM_ERR_OK;

    for (bitIndex = 0U; bitIndex < 8U; bitIndex++) {
        platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_LOW;

        result = platform_i2c_read_bit(i2c, &level);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        value <<= 1U;
        if (level == PLATFORM_GPIO_LEVEL_HIGH) {
            value |= 1U;
        }
    }

    result = platform_i2c_send_ack(i2c, acknowledge);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    *data = value;
    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_send_address(
    platform_i2c_t *i2c,
    uint8_t address,
    platform_bool_t isRead)
{
    platform_bool_t isAcknowledged = PLATFORM_FALSE;
    uint8_t addressByte = (uint8_t)(address << 1U);
    platform_error_t result = PLATFORM_ERR_OK;

    if (isRead != PLATFORM_FALSE) {
        addressByte |= 1U;
    }

    result = platform_i2c_write_byte(i2c,
                                     addressByte,
                                     &isAcknowledged);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if (isAcknowledged == PLATFORM_FALSE) {
        return PLATFORM_ERR_NOT_FOUND;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_send_data(
    platform_i2c_t *i2c,
    const uint8_t *data,
    uint16_t length)
{
    platform_bool_t isAcknowledged = PLATFORM_FALSE;
    uint16_t index = 0U;
    platform_error_t result = PLATFORM_ERR_OK;

    for (index = 0U; index < length; index++) {
        result = platform_i2c_write_byte(i2c, data[index], &isAcknowledged);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        if (isAcknowledged == PLATFORM_FALSE) {
            return PLATFORM_ERR_IO;
        }
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_receive_data(
    platform_i2c_t *i2c,
    uint8_t *data,
    uint16_t length)
{
    uint16_t index = 0U;
    platform_error_t result = PLATFORM_ERR_OK;

    for (index = 0U; index < length; index++) {
        platform_bool_t acknowledge = PLATFORM_TRUE;

        if (index == (uint16_t)(length - 1U)) {
            acknowledge = PLATFORM_FALSE;
        }

        result = platform_i2c_read_byte(i2c, &data[index], acknowledge);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
    }

    return PLATFORM_ERR_OK;
}

/* 事务入口先校验总线空闲；START 失败时仍恢复线路释放状态。 */
static platform_error_t platform_i2c_begin_transaction(platform_i2c_t *i2c)
{
    platform_error_t result = platform_i2c_check_bus_idle(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_start(i2c);
    if (result != PLATFORM_ERR_OK) {
        return platform_i2c_fail_transaction(i2c, result);
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_release_bus(platform_i2c_t *i2c)
{
    platform_error_t firstError = platform_i2c_wait_scl_high(i2c);
    platform_error_t result = platform_i2c_sda_release(i2c);

    if ((firstError == PLATFORM_ERR_OK) && (result != PLATFORM_ERR_OK)) {
        firstError = result;
    }

    return firstError;
}

/* 事务出口优先发送 STOP；失败后继续释放两条开漏线路。 */
static platform_error_t platform_i2c_end_transaction(platform_i2c_t *i2c)
{
    platform_error_t cleanupResult = platform_i2c_stop(i2c);

    if (cleanupResult != PLATFORM_ERR_OK) {
        (void)platform_i2c_release_bus(i2c);
    }

    return cleanupResult;
}

static platform_error_t platform_i2c_fail_transaction(
    platform_i2c_t *i2c,
    platform_error_t originalError)
{
    /* cleanup 失败不得覆盖最先发生、对调用者最有价值的错误。 */
    (void)platform_i2c_end_transaction(i2c);
    return originalError;
}

/* 初始化时至多提供 9 个 SCL 脉冲释放被从机占用的 SDA，再生成 STOP。 */
static platform_error_t platform_i2c_bus_recover(platform_i2c_t *i2c)
{
    platform_gpio_level_t sdaLevel = PLATFORM_GPIO_LEVEL_LOW;
    uint32_t clockIndex = 0U;
    platform_error_t result = platform_i2c_sda_release(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /* 初始化阶段最多提供 9 个恢复时钟，SDA 提前释放即可停止。 */
    for (clockIndex = 0U;
         clockIndex < PLATFORM_I2C_RECOVERY_CLOCK_COUNT;
         clockIndex++) {
        result = platform_i2c_scl_low(i2c);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

        result = platform_i2c_wait_scl_high(i2c);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        platform_delay_us(PROJECT_SOFT_I2C_HALF_PERIOD_US);

        result = platform_i2c_sda_read(i2c, &sdaLevel);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        if (sdaLevel == PLATFORM_GPIO_LEVEL_HIGH) {
            break;
        }
    }

    result = platform_i2c_stop(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_i2c_check_bus_idle(i2c);
}

static void platform_i2c_clear_binding(platform_i2c_t *i2c)
{
    i2c->name = NULL;
    i2c->scl = NULL;
    i2c->sda = NULL;
    i2c->initialized = PLATFORM_FALSE;
}

static platform_error_t platform_i2c_validate_initialized(
    const platform_i2c_t *i2c)
{
    if (i2c == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (i2c->initialized == 0U) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_i2c_validate_address(uint8_t address)
{
    if (address > 0x7FU) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_i2c_init(
    platform_i2c_t *i2c,
    const char *name,
    platform_gpio_t *scl,
    platform_gpio_t *sda)
{
    const platform_gpio_config_t gpioConfig = {
        PLATFORM_GPIO_DIRECTION_OUTPUT,
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_OUTPUT_OPEN_DRAIN,
        PLATFORM_GPIO_LEVEL_HIGH
    };
    platform_gpio_level_t sdaLevel = PLATFORM_GPIO_LEVEL_LOW;
    platform_error_t result = PLATFORM_ERR_OK;

    if (i2c == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((scl == NULL) || (sda == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (i2c->initialized != 0U) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    i2c->name = name;
    i2c->scl = scl;
    i2c->sda = sda;

    result = platform_gpio_configure(scl, &gpioConfig);
    if (result != PLATFORM_ERR_OK) {
        platform_i2c_clear_binding(i2c);
        return result;
    }

    result = platform_gpio_configure(sda, &gpioConfig);
    if (result != PLATFORM_ERR_OK) {
        (void)platform_gpio_deinit(scl);
        platform_i2c_clear_binding(i2c);
        return result;
    }

    result = platform_i2c_sda_release(i2c);
    if (result == PLATFORM_ERR_OK) {
        result = platform_i2c_wait_scl_high(i2c);
    }
    if (result == PLATFORM_ERR_OK) {
        result = platform_i2c_sda_read(i2c, &sdaLevel);
    }
    if ((result == PLATFORM_ERR_OK) &&
        (sdaLevel == PLATFORM_GPIO_LEVEL_LOW)) {
        result = platform_i2c_bus_recover(i2c);
    }
    if (result != PLATFORM_ERR_OK) {
        (void)platform_gpio_deinit(sda);
        (void)platform_gpio_deinit(scl);
        platform_i2c_clear_binding(i2c);
        return result;
    }

    i2c->initialized = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

platform_error_t platform_i2c_probe(
    platform_i2c_t *i2c,
    uint8_t address)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_begin_transaction(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_send_address(i2c, address, PLATFORM_FALSE);
    if (result != PLATFORM_ERR_OK) {
        return platform_i2c_fail_transaction(i2c, result);
    }

    return platform_i2c_end_transaction(i2c);
}

platform_error_t platform_i2c_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (data == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (length == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_i2c_begin_transaction(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_send_address(i2c, address, PLATFORM_FALSE);
    if (result == PLATFORM_ERR_OK) {
        result = platform_i2c_send_data(i2c, data, length);
    }
    if (result != PLATFORM_ERR_OK) {
        return platform_i2c_fail_transaction(i2c, result);
    }

    return platform_i2c_end_transaction(i2c);
}

platform_error_t platform_i2c_read(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t *data,
    uint16_t length)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (data == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (length == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_i2c_begin_transaction(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_send_address(i2c, address, PLATFORM_TRUE);
    if (result == PLATFORM_ERR_OK) {
        result = platform_i2c_receive_data(i2c, data, length);
    }
    if (result != PLATFORM_ERR_OK) {
        return platform_i2c_fail_transaction(i2c, result);
    }

    return platform_i2c_end_transaction(i2c);
}

platform_error_t platform_i2c_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (txData == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (txLength == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (rxData == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (rxLength == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_i2c_begin_transaction(i2c);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_send_address(i2c, address, PLATFORM_FALSE);
    if (result == PLATFORM_ERR_OK) {
        result = platform_i2c_send_data(i2c, txData, txLength);
    }
    if (result != PLATFORM_ERR_OK) {
        return platform_i2c_fail_transaction(i2c, result);
    }

    result = platform_i2c_start(i2c);
    if (result != PLATFORM_ERR_OK) {
        return platform_i2c_fail_transaction(i2c, result);
    }

    result = platform_i2c_send_address(i2c, address, PLATFORM_TRUE);
    if (result == PLATFORM_ERR_OK) {
        result = platform_i2c_receive_data(i2c, rxData, rxLength);
    }
    if (result != PLATFORM_ERR_OK) {
        return platform_i2c_fail_transaction(i2c, result);
    }

    return platform_i2c_end_transaction(i2c);
}

platform_error_t platform_i2c_deinit(platform_i2c_t *i2c)
{
    platform_error_t result = platform_i2c_validate_initialized(i2c);
    platform_error_t firstError = PLATFORM_ERR_OK;

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    firstError = platform_i2c_release_bus(i2c);

    result = platform_gpio_deinit(i2c->sda);
    if ((result != PLATFORM_ERR_OK) && (firstError == PLATFORM_ERR_OK)) {
        firstError = result;
    }

    result = platform_gpio_deinit(i2c->scl);
    if ((result != PLATFORM_ERR_OK) && (firstError == PLATFORM_ERR_OK)) {
        firstError = result;
    }

    platform_i2c_clear_binding(i2c);

    return firstError;
}
//******************************** Functions *********************************//
