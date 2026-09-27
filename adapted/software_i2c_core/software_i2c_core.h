/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 * Copyright (c) 2026 YaoQian Wang
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file is an adapted implementation inspired by RT-Thread
 * components/drivers/i2c/dev_i2c_bit_ops.c and dev_i2c_bit_ops.h.
 * The RT-Thread device model, logging, tick API and rt_* types were removed.
 */

#ifndef SOFTWARE_I2C_CORE_H
#define SOFTWARE_I2C_CORE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 使用资产私有整数类型，避免依赖目标工程可能自定义的 stdint 名称。
 * 编译期检查确保本 Core 对 8/16/32-bit 宽度的假设成立。
 */
typedef unsigned char  software_i2c_u8_t;
typedef unsigned short software_i2c_u16_t;
typedef unsigned int   software_i2c_u32_t;

typedef char software_i2c_u8_width_check[
    (sizeof(software_i2c_u8_t) == 1U) ? 1 : -1];
typedef char software_i2c_u16_width_check[
    (sizeof(software_i2c_u16_t) == 2U) ? 1 : -1];
typedef char software_i2c_u32_width_check[
    (sizeof(software_i2c_u32_t) == 4U) ? 1 : -1];

#define SOFTWARE_I2C_INITIALIZER {0}

typedef enum
{
    SOFTWARE_I2C_OK = 0,
    SOFTWARE_I2C_ERR_INVALID_PARAM,
    SOFTWARE_I2C_ERR_TIMEOUT,
    SOFTWARE_I2C_ERR_BUSY,
    SOFTWARE_I2C_ERR_ADDRESS_NACK,
    SOFTWARE_I2C_ERR_DATA_NACK,
    SOFTWARE_I2C_ERR_IO
} software_i2c_status_t;

/*
 * true 表示端口操作成功。
 * 写 HIGH 表示释放开漏线路，而不是主动推挽输出高电平。
 */
typedef struct
{
    void *context;
    bool (*set_sda)(void *context, bool releaseHigh);
    bool (*set_scl)(void *context, bool releaseHigh);
    bool (*get_sda)(void *context, bool *isHigh);
    bool (*get_scl)(void *context, bool *isHigh);
    void (*delay_us)(void *context, software_i2c_u32_t us);
} software_i2c_port_t;

typedef struct
{
    const software_i2c_port_t *port;
    software_i2c_u32_t halfPeriodUs;
    software_i2c_u32_t sclTimeoutUs;
    bool initialized;
} software_i2c_t;

software_i2c_status_t software_i2c_init(
    software_i2c_t *i2c,
    const software_i2c_port_t *port,
    software_i2c_u32_t halfPeriodUs,
    software_i2c_u32_t sclTimeoutUs);

software_i2c_status_t software_i2c_probe(
    software_i2c_t *i2c,
    software_i2c_u8_t address);

software_i2c_status_t software_i2c_write(
    software_i2c_t *i2c,
    software_i2c_u8_t address,
    const software_i2c_u8_t *data,
    software_i2c_u16_t length);

software_i2c_status_t software_i2c_read(
    software_i2c_t *i2c,
    software_i2c_u8_t address,
    software_i2c_u8_t *data,
    software_i2c_u16_t length);

software_i2c_status_t software_i2c_write_read(
    software_i2c_t *i2c,
    software_i2c_u8_t address,
    const software_i2c_u8_t *txData,
    software_i2c_u16_t txLength,
    software_i2c_u8_t *rxData,
    software_i2c_u16_t rxLength);

software_i2c_status_t software_i2c_deinit(software_i2c_t *i2c);

#ifdef __cplusplus
}
#endif

#endif
