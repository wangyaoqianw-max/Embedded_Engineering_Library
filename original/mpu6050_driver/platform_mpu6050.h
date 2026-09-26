/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_mpu6050.h
 * @brief Platform MPU6050 轻量设备对象和公共接口
 * @author YaoQian Wang
 * @date 2026-09-04
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_MPU6050_H
#define PLATFORM_MPU6050_H

//******************************** Includes *********************************//
#include "platform_i2c.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/*首次初始化前使用此宏初始化 MPU6050 对象存储。*/
#define PLATFORM_MPU6050_INITIALIZER {0}
//******************************** Defines *********************************//

//******************************** Declaring *******************************//
/*Platform MPU6050 轻量对象；I2C 总线由调用者拥有。*/
typedef struct
{
    platform_i2c_t *i2c;
    uint8_t address;
    platform_bool_t initialized;
} platform_mpu6050_t;

/*单次 MPU6050 六轴测量结果。*/
typedef struct
{
    int16_t accelXRaw;
    int16_t accelYRaw;
    int16_t accelZRaw;

    int16_t gyroXRaw;
    int16_t gyroYRaw;
    int16_t gyroZRaw;

    float accelXG;
    float accelYG;
    float accelZG;

    float gyroXDps;
    float gyroYDps;
    float gyroZDps;
} platform_mpu6050_measurement_t;

/**
 * @brief 验证并配置 MPU6050，然后绑定共享 I2C 总线
 * @param[in,out] mpu6050 : 使用 PLATFORM_MPU6050_INITIALIZER 清零的对象
 * @param[in,out] i2c : 已初始化且由调用者拥有的共享 I2C 对象
 * @param[in] address : MPU6050 7-bit 地址，只允许 0x68 或 0x69
 * @return platform_error_t : 初始化结果或底层 I2C 错误
 * @note 成功后设备已唤醒并应用 Phase 7 固定配置。
 * @warning MPU6050 不拥有 i2c 生命周期，调用者必须保证引用有效。
 */
platform_error_t platform_mpu6050_init(
    platform_mpu6050_t *mpu6050,
    platform_i2c_t *i2c,
    uint8_t address);

/**
 * @brief 同步读取一次 MPU6050 六轴快照
 * @param[in,out] mpu6050 : 已初始化的 MPU6050 对象
 * @param[out] measurement : 成功时接收完整六轴测量结果
 * @return platform_error_t : 读取结果或底层 I2C 错误
 * @note 失败时不会修改 measurement 原有内容。
 * @warning 不得从 ISR 或 HAL Callback 调用。
 */
platform_error_t platform_mpu6050_read(
    platform_mpu6050_t *mpu6050,
    platform_mpu6050_measurement_t *measurement);

/**
 * @brief 解除 MPU6050 对共享 I2C 总线的引用
 * @param[in,out] mpu6050 : 已初始化的 MPU6050 对象
 * @return platform_error_t : 反初始化结果
 * @note 本函数不反初始化共享 I2C，也不向芯片发送 reset 或 sleep。
 * @warning 调用者仍负责共享 I2C 总线的最终生命周期。
 */
platform_error_t platform_mpu6050_deinit(platform_mpu6050_t *mpu6050);
//******************************** Declaring *******************************//

#endif
