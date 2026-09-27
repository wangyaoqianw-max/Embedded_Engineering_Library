/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 * Copyright (c) 2026 YaoQian Wang
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Adapted from the design of RT-Thread dev_i2c_bit_ops.c.
 * This implementation is intentionally independent from RT-Thread and any MCU HAL.
 */

#include "software_i2c_core.h"

#include <stddef.h>

#define SOFTWARE_I2C_RECOVERY_CLOCK_COUNT (9U)
#define SOFTWARE_I2C_SCL_WAIT_STEP_US      (1U)

static software_i2c_status_t software_i2c_fail_transaction(
    software_i2c_t *i2c,
    software_i2c_status_t originalError);

static software_i2c_status_t software_i2c_validate(
    const software_i2c_t *i2c)
{
    if ((i2c == NULL) || (i2c->port == NULL)) {
        return SOFTWARE_I2C_ERR_INVALID_PARAM;
    }

    if (!i2c->initialized) {
        return SOFTWARE_I2C_ERR_INVALID_PARAM;
    }

    return SOFTWARE_I2C_OK;
}

static bool software_i2c_set_sda(
    software_i2c_t *i2c,
    bool releaseHigh)
{
    return i2c->port->set_sda(i2c->port->context, releaseHigh);
}

static bool software_i2c_set_scl(
    software_i2c_t *i2c,
    bool releaseHigh)
{
    return i2c->port->set_scl(i2c->port->context, releaseHigh);
}

static bool software_i2c_get_sda(
    software_i2c_t *i2c,
    bool *isHigh)
{
    return i2c->port->get_sda(i2c->port->context, isHigh);
}

static bool software_i2c_get_scl(
    software_i2c_t *i2c,
    bool *isHigh)
{
    return i2c->port->get_scl(i2c->port->context, isHigh);
}

static void software_i2c_delay_half_period(software_i2c_t *i2c)
{
    i2c->port->delay_us(i2c->port->context, i2c->halfPeriodUs);
}

static software_i2c_status_t software_i2c_wait_scl_high(
    software_i2c_t *i2c)
{
    bool isHigh = false;
    software_i2c_u32_t waitedUs = 0U;

    if (!software_i2c_set_scl(i2c, true)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    while (waitedUs < i2c->sclTimeoutUs) {
        if (!software_i2c_get_scl(i2c, &isHigh)) {
            return SOFTWARE_I2C_ERR_IO;
        }

        if (isHigh) {
            return SOFTWARE_I2C_OK;
        }

        i2c->port->delay_us(
            i2c->port->context,
            SOFTWARE_I2C_SCL_WAIT_STEP_US);
        waitedUs += SOFTWARE_I2C_SCL_WAIT_STEP_US;
    }

    return SOFTWARE_I2C_ERR_TIMEOUT;
}

static software_i2c_status_t software_i2c_check_bus_idle(
    software_i2c_t *i2c)
{
    bool sdaHigh = false;
    software_i2c_status_t result = software_i2c_wait_scl_high(i2c);

    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    if (!software_i2c_get_sda(i2c, &sdaHigh)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    return sdaHigh ? SOFTWARE_I2C_OK : SOFTWARE_I2C_ERR_BUSY;
}

static software_i2c_status_t software_i2c_start(software_i2c_t *i2c)
{
    software_i2c_status_t result;

    if (!software_i2c_set_sda(i2c, true)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    result = software_i2c_wait_scl_high(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }
    software_i2c_delay_half_period(i2c);

    if (!software_i2c_set_sda(i2c, false)) {
        return SOFTWARE_I2C_ERR_IO;
    }
    software_i2c_delay_half_period(i2c);

    if (!software_i2c_set_scl(i2c, false)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_stop(software_i2c_t *i2c)
{
    software_i2c_status_t result;

    if (!software_i2c_set_sda(i2c, false)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    result = software_i2c_wait_scl_high(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }
    software_i2c_delay_half_period(i2c);

    if (!software_i2c_set_sda(i2c, true)) {
        return SOFTWARE_I2C_ERR_IO;
    }
    software_i2c_delay_half_period(i2c);

    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_write_bit(
    software_i2c_t *i2c,
    bool high)
{
    software_i2c_status_t result;

    if (!software_i2c_set_scl(i2c, false)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    if (!software_i2c_set_sda(i2c, high)) {
        return SOFTWARE_I2C_ERR_IO;
    }
    software_i2c_delay_half_period(i2c);

    result = software_i2c_wait_scl_high(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }
    software_i2c_delay_half_period(i2c);

    if (!software_i2c_set_scl(i2c, false)) {
        return SOFTWARE_I2C_ERR_IO;
    }
    software_i2c_delay_half_period(i2c);

    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_read_bit(
    software_i2c_t *i2c,
    bool *high)
{
    software_i2c_status_t result;

    if (!software_i2c_set_scl(i2c, false)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    if (!software_i2c_set_sda(i2c, true)) {
        return SOFTWARE_I2C_ERR_IO;
    }
    software_i2c_delay_half_period(i2c);

    result = software_i2c_wait_scl_high(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }
    software_i2c_delay_half_period(i2c);

    if (!software_i2c_get_sda(i2c, high)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    if (!software_i2c_set_scl(i2c, false)) {
        return SOFTWARE_I2C_ERR_IO;
    }
    software_i2c_delay_half_period(i2c);

    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_wait_ack(
    software_i2c_t *i2c,
    bool *acknowledged)
{
    bool sdaHigh = true;
    software_i2c_status_t result;

    if (!software_i2c_set_sda(i2c, true)) {
        return SOFTWARE_I2C_ERR_IO;
    }
    software_i2c_delay_half_period(i2c);

    result = software_i2c_wait_scl_high(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }
    software_i2c_delay_half_period(i2c);

    if (!software_i2c_get_sda(i2c, &sdaHigh)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    if (!software_i2c_set_scl(i2c, false)) {
        return SOFTWARE_I2C_ERR_IO;
    }
    software_i2c_delay_half_period(i2c);

    *acknowledged = !sdaHigh;
    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_write_byte(
    software_i2c_t *i2c,
    software_i2c_u8_t data,
    bool *acknowledged)
{
    software_i2c_u8_t mask = 0x80U;

    while (mask != 0U) {
        software_i2c_status_t result =
            software_i2c_write_bit(i2c, (data & mask) != 0U);

        if (result != SOFTWARE_I2C_OK) {
            return result;
        }

        mask >>= 1U;
    }

    return software_i2c_wait_ack(i2c, acknowledged);
}

static software_i2c_status_t software_i2c_send_ack(
    software_i2c_t *i2c,
    bool acknowledge)
{
    return software_i2c_write_bit(i2c, !acknowledge);
}

static software_i2c_status_t software_i2c_read_byte(
    software_i2c_t *i2c,
    software_i2c_u8_t *data,
    bool acknowledge)
{
    software_i2c_u8_t value = 0U;
    software_i2c_u8_t bitIndex;

    for (bitIndex = 0U; bitIndex < 8U; bitIndex++) {
        bool bitHigh = false;
        software_i2c_status_t result =
            software_i2c_read_bit(i2c, &bitHigh);

        if (result != SOFTWARE_I2C_OK) {
            return result;
        }

        value <<= 1U;
        if (bitHigh) {
            value |= 1U;
        }
    }

    {
        software_i2c_status_t result =
            software_i2c_send_ack(i2c, acknowledge);

        if (result != SOFTWARE_I2C_OK) {
            return result;
        }
    }

    *data = value;
    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_send_address(
    software_i2c_t *i2c,
    software_i2c_u8_t address,
    bool read)
{
    bool acknowledged = false;
    software_i2c_u8_t addressByte = (software_i2c_u8_t)(address << 1U);
    software_i2c_status_t result;

    if (read) {
        addressByte |= 1U;
    }

    result = software_i2c_write_byte(i2c, addressByte, &acknowledged);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    return acknowledged ?
        SOFTWARE_I2C_OK : SOFTWARE_I2C_ERR_ADDRESS_NACK;
}

static software_i2c_status_t software_i2c_send_data(
    software_i2c_t *i2c,
    const software_i2c_u8_t *data,
    software_i2c_u16_t length)
{
    software_i2c_u16_t index;

    for (index = 0U; index < length; index++) {
        bool acknowledged = false;
        software_i2c_status_t result =
            software_i2c_write_byte(i2c, data[index], &acknowledged);

        if (result != SOFTWARE_I2C_OK) {
            return result;
        }

        if (!acknowledged) {
            return SOFTWARE_I2C_ERR_DATA_NACK;
        }
    }

    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_receive_data(
    software_i2c_t *i2c,
    software_i2c_u8_t *data,
    software_i2c_u16_t length)
{
    software_i2c_u16_t index;

    for (index = 0U; index < length; index++) {
        bool acknowledge = index < (software_i2c_u16_t)(length - 1U);
        software_i2c_status_t result =
            software_i2c_read_byte(i2c, &data[index], acknowledge);

        if (result != SOFTWARE_I2C_OK) {
            return result;
        }
    }

    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_begin_transaction(
    software_i2c_t *i2c)
{
    software_i2c_status_t result = software_i2c_check_bus_idle(i2c);

    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    result = software_i2c_start(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return software_i2c_fail_transaction(i2c, result);
    }

    return SOFTWARE_I2C_OK;
}

static software_i2c_status_t software_i2c_release_bus(software_i2c_t *i2c)
{
    software_i2c_status_t firstError = software_i2c_wait_scl_high(i2c);

    if (!software_i2c_set_sda(i2c, true) &&
        (firstError == SOFTWARE_I2C_OK)) {
        firstError = SOFTWARE_I2C_ERR_IO;
    }

    return firstError;
}

static software_i2c_status_t software_i2c_end_transaction(
    software_i2c_t *i2c)
{
    software_i2c_status_t result = software_i2c_stop(i2c);

    if (result != SOFTWARE_I2C_OK) {
        (void)software_i2c_release_bus(i2c);
    }

    return result;
}

static software_i2c_status_t software_i2c_fail_transaction(
    software_i2c_t *i2c,
    software_i2c_status_t originalError)
{
    (void)software_i2c_end_transaction(i2c);
    return originalError;
}

static software_i2c_status_t software_i2c_recover_bus(software_i2c_t *i2c)
{
    bool sdaHigh = false;
    software_i2c_u32_t clockIndex;

    if (!software_i2c_set_sda(i2c, true)) {
        return SOFTWARE_I2C_ERR_IO;
    }

    for (clockIndex = 0U;
         clockIndex < SOFTWARE_I2C_RECOVERY_CLOCK_COUNT;
         clockIndex++) {
        software_i2c_status_t result;

        if (!software_i2c_set_scl(i2c, false)) {
            return SOFTWARE_I2C_ERR_IO;
        }
        software_i2c_delay_half_period(i2c);

        result = software_i2c_wait_scl_high(i2c);
        if (result != SOFTWARE_I2C_OK) {
            return result;
        }
        software_i2c_delay_half_period(i2c);

        if (!software_i2c_get_sda(i2c, &sdaHigh)) {
            return SOFTWARE_I2C_ERR_IO;
        }

        if (sdaHigh) {
            break;
        }
    }

    {
        software_i2c_status_t result = software_i2c_stop(i2c);

        if (result != SOFTWARE_I2C_OK) {
            return result;
        }
    }

    return software_i2c_check_bus_idle(i2c);
}

software_i2c_status_t software_i2c_init(
    software_i2c_t *i2c,
    const software_i2c_port_t *port,
    software_i2c_u32_t halfPeriodUs,
    software_i2c_u32_t sclTimeoutUs)
{
    bool sdaHigh = false;
    software_i2c_status_t result;

    if ((i2c == NULL) || (port == NULL) ||
        (port->set_sda == NULL) || (port->set_scl == NULL) ||
        (port->get_sda == NULL) || (port->get_scl == NULL) ||
        (port->delay_us == NULL) ||
        (halfPeriodUs == 0U) || (sclTimeoutUs == 0U)) {
        return SOFTWARE_I2C_ERR_INVALID_PARAM;
    }

    if (i2c->initialized) {
        return SOFTWARE_I2C_ERR_BUSY;
    }

    i2c->port = port;
    i2c->halfPeriodUs = halfPeriodUs;
    i2c->sclTimeoutUs = sclTimeoutUs;

    if (!software_i2c_set_sda(i2c, true)) {
        result = SOFTWARE_I2C_ERR_IO;
        goto fail;
    }

    result = software_i2c_wait_scl_high(i2c);
    if (result != SOFTWARE_I2C_OK) {
        goto fail;
    }

    if (!software_i2c_get_sda(i2c, &sdaHigh)) {
        result = SOFTWARE_I2C_ERR_IO;
        goto fail;
    }

    if (!sdaHigh) {
        result = software_i2c_recover_bus(i2c);
        if (result != SOFTWARE_I2C_OK) {
            goto fail;
        }
    }

    i2c->initialized = true;
    return SOFTWARE_I2C_OK;

fail:
    i2c->port = NULL;
    i2c->halfPeriodUs = 0U;
    i2c->sclTimeoutUs = 0U;
    i2c->initialized = false;
    return result;
}

software_i2c_status_t software_i2c_probe(
    software_i2c_t *i2c,
    software_i2c_u8_t address)
{
    software_i2c_status_t result = software_i2c_validate(i2c);

    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    if (address > 0x7FU) {
        return SOFTWARE_I2C_ERR_INVALID_PARAM;
    }

    result = software_i2c_begin_transaction(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    result = software_i2c_send_address(i2c, address, false);
    if (result != SOFTWARE_I2C_OK) {
        return software_i2c_fail_transaction(i2c, result);
    }

    return software_i2c_end_transaction(i2c);
}

software_i2c_status_t software_i2c_write(
    software_i2c_t *i2c,
    software_i2c_u8_t address,
    const software_i2c_u8_t *data,
    software_i2c_u16_t length)
{
    software_i2c_status_t result = software_i2c_validate(i2c);

    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    if ((address > 0x7FU) || (data == NULL) || (length == 0U)) {
        return SOFTWARE_I2C_ERR_INVALID_PARAM;
    }

    result = software_i2c_begin_transaction(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    result = software_i2c_send_address(i2c, address, false);
    if (result == SOFTWARE_I2C_OK) {
        result = software_i2c_send_data(i2c, data, length);
    }

    if (result != SOFTWARE_I2C_OK) {
        return software_i2c_fail_transaction(i2c, result);
    }

    return software_i2c_end_transaction(i2c);
}

software_i2c_status_t software_i2c_read(
    software_i2c_t *i2c,
    software_i2c_u8_t address,
    software_i2c_u8_t *data,
    software_i2c_u16_t length)
{
    software_i2c_status_t result = software_i2c_validate(i2c);

    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    if ((address > 0x7FU) || (data == NULL) || (length == 0U)) {
        return SOFTWARE_I2C_ERR_INVALID_PARAM;
    }

    result = software_i2c_begin_transaction(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    result = software_i2c_send_address(i2c, address, true);
    if (result == SOFTWARE_I2C_OK) {
        result = software_i2c_receive_data(i2c, data, length);
    }

    if (result != SOFTWARE_I2C_OK) {
        return software_i2c_fail_transaction(i2c, result);
    }

    return software_i2c_end_transaction(i2c);
}

software_i2c_status_t software_i2c_write_read(
    software_i2c_t *i2c,
    software_i2c_u8_t address,
    const software_i2c_u8_t *txData,
    software_i2c_u16_t txLength,
    software_i2c_u8_t *rxData,
    software_i2c_u16_t rxLength)
{
    software_i2c_status_t result = software_i2c_validate(i2c);

    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    if ((address > 0x7FU) ||
        (txData == NULL) || (txLength == 0U) ||
        (rxData == NULL) || (rxLength == 0U)) {
        return SOFTWARE_I2C_ERR_INVALID_PARAM;
    }

    result = software_i2c_begin_transaction(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    result = software_i2c_send_address(i2c, address, false);
    if (result == SOFTWARE_I2C_OK) {
        result = software_i2c_send_data(i2c, txData, txLength);
    }
    if (result != SOFTWARE_I2C_OK) {
        return software_i2c_fail_transaction(i2c, result);
    }

    result = software_i2c_start(i2c);
    if (result != SOFTWARE_I2C_OK) {
        return software_i2c_fail_transaction(i2c, result);
    }

    result = software_i2c_send_address(i2c, address, true);
    if (result == SOFTWARE_I2C_OK) {
        result = software_i2c_receive_data(i2c, rxData, rxLength);
    }
    if (result != SOFTWARE_I2C_OK) {
        return software_i2c_fail_transaction(i2c, result);
    }

    return software_i2c_end_transaction(i2c);
}

software_i2c_status_t software_i2c_deinit(software_i2c_t *i2c)
{
    software_i2c_status_t result = software_i2c_validate(i2c);

    if (result != SOFTWARE_I2C_OK) {
        return result;
    }

    result = software_i2c_release_bus(i2c);
    i2c->port = NULL;
    i2c->halfPeriodUs = 0U;
    i2c->sclTimeoutUs = 0U;
    i2c->initialized = false;

    return result;
}
