/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_i2c.h
 * @brief Platform I2C 同步事务公共契约
 * @author YaoQian Wang
 * @date 2026-09-27
 * @version V2.0
 *
 *****************************************************************************/

#ifndef PLATFORM_I2C_H
#define PLATFORM_I2C_H

#include "platform_error.h"
#include "platform_types.h"

#define PLATFORM_I2C_INITIALIZER {0}

typedef struct platform_i2c platform_i2c_t;

/**
 * @brief I2C Backend 操作表
 * @note Platform 只定义同步 I2C 能力；START/STOP、GPIO Bit-bang、HAL I2C 等实现细节均属于 Backend。
 */
typedef struct
{
    platform_error_t (*init)(platform_i2c_t *i2c);
    platform_error_t (*probe)(platform_i2c_t *i2c, uint8_t address);
    platform_error_t (*write)(
        platform_i2c_t *i2c,
        uint8_t address,
        const uint8_t *data,
        uint16_t length);
    platform_error_t (*read)(
        platform_i2c_t *i2c,
        uint8_t address,
        uint8_t *data,
        uint16_t length);
    platform_error_t (*writeRead)(
        platform_i2c_t *i2c,
        uint8_t address,
        const uint8_t *txData,
        uint16_t txLength,
        uint8_t *rxData,
        uint16_t rxLength);
    platform_error_t (*deinit)(platform_i2c_t *i2c);
} platform_i2c_ops_t;

/**
 * @brief Platform I2C 总线对象
 * @note ops 与 implContext 均为非拥有型引用，必须在 I2C 对象使用期间保持有效。
 */
struct platform_i2c
{
    const char *name;
    const platform_i2c_ops_t *ops;
    void *implContext;
    platform_bool_t initialized;
};

typedef struct
{
    const char *name;
    const platform_i2c_ops_t *ops;
    void *implContext;
} platform_i2c_init_params_t;

/**
 * @brief 绑定并初始化一个 I2C Backend
 */
platform_error_t platform_i2c_init(
    platform_i2c_t *i2c,
    const platform_i2c_init_params_t *params);

/**
 * @brief 探测 7-bit 地址从设备是否响应
 */
platform_error_t platform_i2c_probe(
    platform_i2c_t *i2c,
    uint8_t address);

/**
 * @brief 向 7-bit 地址从设备写入数据
 */
platform_error_t platform_i2c_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length);

/**
 * @brief 从 7-bit 地址从设备读取数据
 */
platform_error_t platform_i2c_read(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t *data,
    uint16_t length);

/**
 * @brief 先写后读，Backend 必须保证中间使用 Repeated START 或等价硬件语义
 */
platform_error_t platform_i2c_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength);

/**
 * @brief 反初始化 Backend 并解除 Platform I2C 绑定
 * @note 即使 Backend 清理返回错误，Platform 绑定仍会被清除。
 */
platform_error_t platform_i2c_deinit(platform_i2c_t *i2c);

#endif
