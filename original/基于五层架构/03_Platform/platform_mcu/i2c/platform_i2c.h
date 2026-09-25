/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_i2c.h
 * @brief Platform I2C 同步事务公共接口
 * @author YaoQian Wang
 * @date 2026-09-02
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_I2C_H
#define PLATFORM_I2C_H

//******************************** Includes *********************************//
#include "platform_error.h"
#include "platform_gpio.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/*首次初始化前使用此宏初始化 I2C 对象存储*/
#define PLATFORM_I2C_INITIALIZER {0}
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/*Platform I2C 轻量同步总线对象，GPIO 存储由调用者拥有*/
typedef struct
{
    const char *name;
    platform_gpio_t *scl;
    platform_gpio_t *sda;
    platform_bool_t initialized;
} platform_i2c_t;

/**
 * @brief 绑定 Platform I2C 对象与 SCL/SDA GPIO
 * @param[in,out] i2c : 使用 PLATFORM_I2C_INITIALIZER 清零的 I2C 对象
 * @param[in] name : I2C 总线名称，可为 NULL
 * @param[in] scl : 调用者拥有的 SCL GPIO 对象
 * @param[in] sda : 调用者拥有的 SDA GPIO 对象
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_i2c_init(
    platform_i2c_t *i2c,
    const char *name,
    platform_gpio_t *scl,
    platform_gpio_t *sda);

/**
 * @brief 探测 7-bit 地址从设备是否响应
 * @param[in,out] i2c : 已初始化 I2C 对象
 * @param[in] address : 7-bit 从设备地址
 * @return PLATFORM_ERR_OK : 地址阶段收到 ACK
 * @return PLATFORM_ERR_NOT_FOUND : 地址阶段收到 NACK
 * @return 其他值 : 总线或事务错误
 * @note 本函数只执行一次 START、SLA+W、ACK/NACK 和 STOP，不发送数据字节。
 */
platform_error_t platform_i2c_probe(
    platform_i2c_t *i2c,
    uint8_t address);

/**
 * @brief 向 7-bit 地址从设备写入数据
 * @param[in,out] i2c : 已初始化 I2C 对象
 * @param[in] address : 7-bit 从设备地址
 * @param[in] data : 待发送数据
 * @param[in] length : 待发送字节数，必须大于 0
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_i2c_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length);

/**
 * @brief 从 7-bit 地址从设备读取数据
 * @param[in,out] i2c : 已初始化 I2C 对象
 * @param[in] address : 7-bit 从设备地址
 * @param[out] data : 接收数据缓冲区
 * @param[in] length : 接收字节数，必须大于 0
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_i2c_read(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t *data,
    uint16_t length);

/**
 * @brief 先写入数据再发起 Repeated START 读取数据
 * @param[in,out] i2c : 已初始化 I2C 对象
 * @param[in] address : 7-bit 从设备地址
 * @param[in] txData : 待发送数据
 * @param[in] txLength : 待发送字节数，必须大于 0
 * @param[out] rxData : 接收数据缓冲区
 * @param[in] rxLength : 接收字节数，必须大于 0
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_i2c_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength);

/**
 * @brief 解除 I2C 对象绑定
 * @param[in,out] i2c : 已初始化 I2C 对象
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_i2c_deinit(platform_i2c_t *i2c);
//******************************** Declaring *********************************//

#endif
