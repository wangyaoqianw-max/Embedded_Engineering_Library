/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_dht20.c
 * @brief Platform DHT20 同步测量接口实现
 * @author YaoQian Wang
 * @date 2026-09-04
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_dht20.h"

#include "platform_time.h"

#include <stddef.h>
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_DHT20_I2C_ADDRESS             (0x38U)
#define PLATFORM_DHT20_MEASURE_COMMAND_LENGTH  (3U)
#define PLATFORM_DHT20_FRAME_LENGTH            (7U)
#define PLATFORM_DHT20_CRC_DATA_LENGTH         (6U)
#define PLATFORM_DHT20_MEASURE_DELAY_MS        (80U)
#define PLATFORM_DHT20_STATUS_BUSY_MASK        (0x80U)
#define PLATFORM_DHT20_STATUS_CRC_FLAG_MASK    (0x10U)
#define PLATFORM_DHT20_STATUS_CAL_ENABLE_MASK  (0x08U)
#define PLATFORM_DHT20_CRC_INITIAL_VALUE       (0xFFU)
#define PLATFORM_DHT20_CRC_POLYNOMIAL          (0x31U)
#define PLATFORM_DHT20_RAW_SCALE               (1048576.0f)
//******************************** Defines *********************************//

//******************************** Private Functions ***********************//
/* 按 DHT20 协议计算前 6 字节的 CRC-8（初值 0xFF，多项式 0x31）。 */
static uint8_t platform_dht20_crc8(const uint8_t *data, uint32_t length)
{
    uint8_t crc = PLATFORM_DHT20_CRC_INITIAL_VALUE;
    uint32_t byteIndex = 0U;
    uint32_t bitIndex = 0U;

    for (byteIndex = 0U; byteIndex < length; byteIndex++) {
        crc ^= data[byteIndex];
        for (bitIndex = 0U; bitIndex < 8U; bitIndex++) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1U) ^
                                PLATFORM_DHT20_CRC_POLYNOMIAL);
            } else {
                crc <<= 1U;
            }
        }
    }

    return crc;
}

/* 对象和其非拥有型共享 I2C 引用都必须仍处于初始化状态。 */
static platform_error_t platform_dht20_validate_initialized(
    const platform_dht20_t *dht20)
{
    if (dht20 == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (dht20->initialized == 0U) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if ((dht20->i2c == NULL) ||
        (dht20->i2c->initialized == 0U)) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    return PLATFORM_ERR_OK;
}
//******************************** Private Functions ***********************//

//******************************** Functions *******************************//
platform_error_t platform_dht20_init(
    platform_dht20_t *dht20,
    platform_i2c_t *i2c)
{
    if ((dht20 == NULL) || (i2c == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (dht20->initialized != 0U) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    if (i2c->initialized == 0U) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    dht20->i2c = i2c;
    dht20->initialized = 1U;

    return PLATFORM_ERR_OK;
}

platform_error_t platform_dht20_read(
    platform_dht20_t *dht20,
    platform_dht20_measurement_t *measurement)
{
    static const uint8_t measureCommand[
        PLATFORM_DHT20_MEASURE_COMMAND_LENGTH] = {
        0xACU,
        0x33U,
        0x00U
    };
    uint8_t frame[PLATFORM_DHT20_FRAME_LENGTH] = {0};
    platform_dht20_measurement_t localMeasurement = {0};
    platform_error_t result = platform_dht20_validate_initialized(dht20);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (measurement == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = platform_i2c_write(dht20->i2c,
                                PLATFORM_DHT20_I2C_ADDRESS,
                                measureCommand,
                                PLATFORM_DHT20_MEASURE_COMMAND_LENGTH);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_time_delay_ms(PLATFORM_DHT20_MEASURE_DELAY_MS);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_i2c_read(dht20->i2c,
                               PLATFORM_DHT20_I2C_ADDRESS,
                               frame,
                               PLATFORM_DHT20_FRAME_LENGTH);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((frame[0] & PLATFORM_DHT20_STATUS_BUSY_MASK) != 0U) {
        return PLATFORM_ERR_BUSY;
    }

    if (platform_dht20_crc8(frame, PLATFORM_DHT20_CRC_DATA_LENGTH) !=
        frame[PLATFORM_DHT20_CRC_DATA_LENGTH]) {
        return PLATFORM_ERR_CHECKSUM;
    }

    if ((frame[0] & PLATFORM_DHT20_STATUS_CRC_FLAG_MASK) == 0U) {
        return PLATFORM_ERR_CHECKSUM;
    }

    if ((frame[0] & PLATFORM_DHT20_STATUS_CAL_ENABLE_MASK) == 0U) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    localMeasurement.status = frame[0];
    localMeasurement.rawHumidity =
        ((uint32_t)frame[1] << 12U) |
        ((uint32_t)frame[2] << 4U) |
        ((uint32_t)frame[3] >> 4U);
    localMeasurement.rawTemperature =
        (((uint32_t)frame[3] & 0x0FU) << 16U) |
        ((uint32_t)frame[4] << 8U) |
        (uint32_t)frame[5];
    localMeasurement.humidityPercent =
        ((float)localMeasurement.rawHumidity * 100.0f) /
        PLATFORM_DHT20_RAW_SCALE;
    localMeasurement.temperatureC =
        (((float)localMeasurement.rawTemperature * 200.0f) /
         PLATFORM_DHT20_RAW_SCALE) - 50.0f;

    *measurement = localMeasurement;

    return PLATFORM_ERR_OK;
}

platform_error_t platform_dht20_deinit(platform_dht20_t *dht20)
{
    if (dht20 == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (dht20->initialized == 0U) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    dht20->i2c = NULL;
    dht20->initialized = 0U;

    return PLATFORM_ERR_OK;
}
//******************************** Functions *******************************//
