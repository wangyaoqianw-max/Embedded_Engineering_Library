/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_mpu6050.c
 * @brief Platform MPU6050 Phase 7 同步六轴读取实现
 * @author YaoQian Wang
 * @date 2026-09-04
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_mpu6050.h"

#include "platform_def.h"

#include <stddef.h>
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_MPU6050_ADDRESS_LOW              (0x68U)
#define PLATFORM_MPU6050_ADDRESS_HIGH             (0x69U)
#define PLATFORM_MPU6050_WHO_AM_I_VALUE           (0x68U)

#define PLATFORM_MPU6050_REGISTER_SMPLRT_DIV      (0x19U)
#define PLATFORM_MPU6050_REGISTER_CONFIG          (0x1AU)
#define PLATFORM_MPU6050_REGISTER_GYRO_CONFIG     (0x1BU)
#define PLATFORM_MPU6050_REGISTER_ACCEL_CONFIG    (0x1CU)
#define PLATFORM_MPU6050_REGISTER_ACCEL_XOUT_H    (0x3BU)
#define PLATFORM_MPU6050_REGISTER_PWR_MGMT_1      (0x6BU)
#define PLATFORM_MPU6050_REGISTER_WHO_AM_I        (0x75U)

#define PLATFORM_MPU6050_PWR_MGMT_1_VALUE         (0x01U)
#define PLATFORM_MPU6050_CONFIG_VALUE             (0x03U)
#define PLATFORM_MPU6050_SMPLRT_DIV_VALUE         (0x04U)
#define PLATFORM_MPU6050_GYRO_CONFIG_VALUE        (0x00U)
#define PLATFORM_MPU6050_ACCEL_CONFIG_VALUE       (0x00U)

#define PLATFORM_MPU6050_REGISTER_WRITE_LENGTH    (2U)
#define PLATFORM_MPU6050_FRAME_LENGTH             (14U)
#define PLATFORM_MPU6050_ACCEL_SCALE              (16384.0f)
#define PLATFORM_MPU6050_GYRO_SCALE               (131.0f)
//******************************** Defines *********************************//

//******************************** Private Functions ***********************//
static platform_error_t platform_mpu6050_validate_address(uint8_t address)
{
    if ((address != PLATFORM_MPU6050_ADDRESS_LOW) &&
        (address != PLATFORM_MPU6050_ADDRESS_HIGH)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_mpu6050_validate_initialized(
    const platform_mpu6050_t *mpu6050)
{
    if (mpu6050 == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (mpu6050->initialized == PLATFORM_FALSE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if ((mpu6050->i2c == NULL) ||
        (mpu6050->i2c->initialized == PLATFORM_FALSE)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return PLATFORM_ERR_OK;
}

/* 将寄存器地址和值作为一个连续 I2C 写帧发送，避免调用者重复组包。 */
static platform_error_t platform_mpu6050_write_register(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t registerAddress,
    uint8_t value)
{
    const uint8_t data[PLATFORM_MPU6050_REGISTER_WRITE_LENGTH] = {
        registerAddress,
        value
    };

    return platform_i2c_write(i2c,
                              address,
                              data,
                              PLATFORM_MPU6050_REGISTER_WRITE_LENGTH);
}

/* 将传感器高字节在前的补码轴数据拼接为有符号 16 位原始值。 */
static int16_t platform_mpu6050_parse_axis(uint8_t high, uint8_t low)
{
    return (int16_t)(((uint16_t)high << 8U) | (uint16_t)low);
}
//******************************** Private Functions ***********************//

//******************************** Functions *******************************//
platform_error_t platform_mpu6050_init(
    platform_mpu6050_t *mpu6050,
    platform_i2c_t *i2c,
    uint8_t address)
{
    uint8_t whoAmIRegister = PLATFORM_MPU6050_REGISTER_WHO_AM_I;
    uint8_t whoAmI = 0U;
    platform_error_t result = PLATFORM_ERR_OK;

    if ((mpu6050 == NULL) || (i2c == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (mpu6050->initialized != PLATFORM_FALSE) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    result = platform_mpu6050_validate_address(address);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (i2c->initialized == PLATFORM_FALSE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    result = platform_i2c_write_read(i2c,
                                     address,
                                     &whoAmIRegister,
                                     1U,
                                     &whoAmI,
                                     1U);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (whoAmI != PLATFORM_MPU6050_WHO_AM_I_VALUE) {
        return PLATFORM_ERR_NOT_FOUND;
    }

    result = platform_mpu6050_write_register(
        i2c,
        address,
        PLATFORM_MPU6050_REGISTER_PWR_MGMT_1,
        PLATFORM_MPU6050_PWR_MGMT_1_VALUE);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_mpu6050_write_register(
        i2c,
        address,
        PLATFORM_MPU6050_REGISTER_CONFIG,
        PLATFORM_MPU6050_CONFIG_VALUE);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_mpu6050_write_register(
        i2c,
        address,
        PLATFORM_MPU6050_REGISTER_SMPLRT_DIV,
        PLATFORM_MPU6050_SMPLRT_DIV_VALUE);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_mpu6050_write_register(
        i2c,
        address,
        PLATFORM_MPU6050_REGISTER_GYRO_CONFIG,
        PLATFORM_MPU6050_GYRO_CONFIG_VALUE);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_mpu6050_write_register(
        i2c,
        address,
        PLATFORM_MPU6050_REGISTER_ACCEL_CONFIG,
        PLATFORM_MPU6050_ACCEL_CONFIG_VALUE);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    mpu6050->i2c = i2c;
    mpu6050->address = address;
    mpu6050->initialized = PLATFORM_TRUE;

    return PLATFORM_ERR_OK;
}

platform_error_t platform_mpu6050_read(
    platform_mpu6050_t *mpu6050,
    platform_mpu6050_measurement_t *measurement)
{
    uint8_t dataRegister = PLATFORM_MPU6050_REGISTER_ACCEL_XOUT_H;
    uint8_t frame[PLATFORM_MPU6050_FRAME_LENGTH] = {0};
    platform_mpu6050_measurement_t localMeasurement = {0};
    platform_error_t result = platform_mpu6050_validate_initialized(mpu6050);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (measurement == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = platform_i2c_write_read(mpu6050->i2c,
                                     mpu6050->address,
                                     &dataRegister,
                                     1U,
                                     frame,
                                     PLATFORM_MPU6050_FRAME_LENGTH);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    localMeasurement.accelXRaw = platform_mpu6050_parse_axis(frame[0],
                                                              frame[1]);
    localMeasurement.accelYRaw = platform_mpu6050_parse_axis(frame[2],
                                                              frame[3]);
    localMeasurement.accelZRaw = platform_mpu6050_parse_axis(frame[4],
                                                              frame[5]);
    localMeasurement.gyroXRaw = platform_mpu6050_parse_axis(frame[8],
                                                             frame[9]);
    localMeasurement.gyroYRaw = platform_mpu6050_parse_axis(frame[10],
                                                             frame[11]);
    localMeasurement.gyroZRaw = platform_mpu6050_parse_axis(frame[12],
                                                             frame[13]);

    localMeasurement.accelXG =
        (float)localMeasurement.accelXRaw / PLATFORM_MPU6050_ACCEL_SCALE;
    localMeasurement.accelYG =
        (float)localMeasurement.accelYRaw / PLATFORM_MPU6050_ACCEL_SCALE;
    localMeasurement.accelZG =
        (float)localMeasurement.accelZRaw / PLATFORM_MPU6050_ACCEL_SCALE;
    localMeasurement.gyroXDps =
        (float)localMeasurement.gyroXRaw / PLATFORM_MPU6050_GYRO_SCALE;
    localMeasurement.gyroYDps =
        (float)localMeasurement.gyroYRaw / PLATFORM_MPU6050_GYRO_SCALE;
    localMeasurement.gyroZDps =
        (float)localMeasurement.gyroZRaw / PLATFORM_MPU6050_GYRO_SCALE;

    *measurement = localMeasurement;

    return PLATFORM_ERR_OK;
}

platform_error_t platform_mpu6050_deinit(platform_mpu6050_t *mpu6050)
{
    if (mpu6050 == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (mpu6050->initialized == PLATFORM_FALSE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    mpu6050->i2c = NULL;
    mpu6050->address = 0U;
    mpu6050->initialized = PLATFORM_FALSE;

    return PLATFORM_ERR_OK;
}
//******************************** Functions *******************************//
