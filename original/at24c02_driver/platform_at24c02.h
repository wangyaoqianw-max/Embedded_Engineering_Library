/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_at24c02.h
 * @brief AT24C02 Platform Raw Driver 公共接口
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_AT24C02_H
#define PLATFORM_AT24C02_H

//******************************** Includes *********************************//
#include "platform_i2c.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_AT24C02_INITIALIZER          {0}
#define PLATFORM_AT24C02_TOTAL_SIZE_BYTES     (256U)
#define PLATFORM_AT24C02_PAGE_SIZE_BYTES      (8U)
#define PLATFORM_AT24C02_I2C_ADDRESS_MIN      (0x50U)
#define PLATFORM_AT24C02_I2C_ADDRESS_MAX      (0x57U)
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/**
 * @brief AT24C02 Raw Driver 运行时对象
 * @note i2c 指向由调用者拥有的共享 Software I2C 对象，Driver 不负责其生命周期。
 */
typedef struct
{
    platform_i2c_t *i2c;
    uint8_t deviceAddress;
    platform_bool_t initialized;
} platform_at24c02_t;

/**
 * @brief 初始化 AT24C02 Raw Driver 并执行非破坏性地址探测
 * @param[in,out] eeprom : 使用 PLATFORM_AT24C02_INITIALIZER 清零的对象存储
 * @param[in] i2c : 已初始化的 Software I2C 对象
 * @param[in] deviceAddress : AT24C02 7-bit 地址，范围为 0x50~0x57
 * @return PLATFORM_ERR_OK : 地址探测收到 ACK，初始化成功
 * @return 其他值 : 初始化或地址探测失败
 * @note 初始化成功前不会写入 EEPROM；Driver 不拥有 I2C Bus 生命周期。
 */
platform_error_t platform_at24c02_init(
    platform_at24c02_t *eeprom,
    platform_i2c_t *i2c,
    uint8_t deviceAddress);

/**
 * @brief 释放 AT24C02 Raw Driver 自身状态
 * @param[in,out] eeprom : 已初始化的 AT24C02 对象
 * @return PLATFORM_ERR_OK : 释放成功
 * @return 其他值 : 对象为空或尚未初始化
 * @note 不调用 platform_i2c_deinit()，共享 I2C Bus 由其所有者管理。
 */
platform_error_t platform_at24c02_deinit(
    platform_at24c02_t *eeprom);

/**
 * @brief 从 AT24C02 指定地址读取连续数据
 * @param[in] eeprom : 已初始化的 AT24C02 对象
 * @param[in] address : EEPROM 字节地址，范围为 0x00~0xFF
 * @param[out] data : 接收数据缓冲区
 * @param[in] dataLength : 读取字节数，必须位于 EEPROM 剩余容量内
 * @return PLATFORM_ERR_OK : 读取成功
 * @return 其他值 : 参数、状态或 I2C 事务错误
 * @note 接口为阻塞式 Random/Sequential Read，不按 8 Byte Page 拆分。
 */
platform_error_t platform_at24c02_read(
    platform_at24c02_t *eeprom,
    uint32_t address,
    uint8_t *data,
    platform_size_t dataLength);

/**
 * @brief 向 AT24C02 写入连续数据
 * @param[in,out] eeprom : 已初始化的 AT24C02 对象
 * @param[in] address : EEPROM 字节地址，范围为 0x00~0xFF
 * @param[in] data : 待写入数据缓冲区
 * @param[in] dataLength : 写入字节数，必须位于 EEPROM 剩余容量内
 * @return PLATFORM_ERR_OK : 写入成功且内部写周期已完成
 * @return 其他值 : 参数、状态或 I2C 事务错误
 * @note 写入按 8 Byte Page 自动拆分，并在每页后执行有界 ACK Polling。
 */
platform_error_t platform_at24c02_write(
    platform_at24c02_t *eeprom,
    uint32_t address,
    const uint8_t *data,
    platform_size_t dataLength);
//******************************** Declaring *********************************//

#endif
