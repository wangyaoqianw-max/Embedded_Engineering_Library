/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_platform_dht20.c
 * @brief 验证 Platform DHT20 第一阶段公共合同
 * @author YaoQian Wang
 * @date 2026-09-04
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include <stddef.h>
#include <stdio.h>

#include "dht20/platform_dht20.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define TEST_ASSERT(condition)       \
    do {                             \
        if (!(condition)) {          \
            (void)printf("assertion failed at line %d\n", __LINE__); \
            return __LINE__;         \
        }                            \
    } while (0)

#define TEST_DHT20_ADDRESS              (0x38U)
#define TEST_DHT20_COMMAND_LENGTH       (3U)
#define TEST_DHT20_FRAME_LENGTH         (7U)
//******************************** Defines *********************************//

//******************************** Types ***********************************//
typedef struct
{
    platform_error_t writeResult;
    platform_error_t delayResult;
    platform_error_t readResult;
    uint8_t frame[TEST_DHT20_FRAME_LENGTH];
    uint8_t address;
    uint8_t command[TEST_DHT20_COMMAND_LENGTH];
    uint16_t writeLength;
    uint16_t readLength;
    uint32_t delayMs;
    uint32_t writeCallCount;
    uint32_t delayCallCount;
    uint32_t readCallCount;
    uint32_t writeReadCallCount;
    uint32_t i2cDeinitCallCount;
} test_recorder_t;
//******************************** Types ***********************************//

//******************************** Variables *******************************//
static test_recorder_t g_recorder;
//******************************** Variables *******************************//

//******************************** Private Functions ***********************//
static void test_reset_recorder(void)
{
    uint32_t index = 0U;
    uint8_t *bytes = (uint8_t *)&g_recorder;

    for (index = 0U; index < (uint32_t)sizeof(g_recorder); index++) {
        bytes[index] = 0U;
    }
}

static void test_set_valid_frame(void)
{
    g_recorder.frame[0] = 0x18U;
    g_recorder.frame[1] = 0x80U;
    g_recorder.frame[2] = 0x00U;
    g_recorder.frame[3] = 0x08U;
    g_recorder.frame[4] = 0x00U;
    g_recorder.frame[5] = 0x00U;
    g_recorder.frame[6] = 0xD4U;
}

static platform_i2c_t test_initialized_i2c(void)
{
    platform_i2c_t i2c = PLATFORM_I2C_INITIALIZER;

    i2c.initialized = 1U;
    return i2c;
}

static int test_measurement_unchanged(
    const platform_dht20_measurement_t *measurement)
{
    TEST_ASSERT(0xA5U == measurement->status);
    TEST_ASSERT(0x12345U == measurement->rawHumidity);
    TEST_ASSERT(0x54321U == measurement->rawTemperature);
    TEST_ASSERT(12.5f == measurement->humidityPercent);
    TEST_ASSERT(-7.5f == measurement->temperatureC);

    return 0;
}

static platform_dht20_measurement_t test_sentinel_measurement(void)
{
    platform_dht20_measurement_t measurement = {
        0xA5U,
        0x12345U,
        0x54321U,
        12.5f,
        -7.5f
    };

    return measurement;
}

platform_error_t platform_i2c_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length)
{
    uint16_t index = 0U;

    (void)i2c;
    g_recorder.writeCallCount++;
    g_recorder.address = address;
    g_recorder.writeLength = length;
    for ((index = 0U); (index < length) &&
         (index < TEST_DHT20_COMMAND_LENGTH); index++) {
        g_recorder.command[index] = data[index];
    }

    return g_recorder.writeResult;
}

platform_error_t platform_i2c_read(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t *data,
    uint16_t length)
{
    uint16_t index = 0U;

    (void)i2c;
    g_recorder.readCallCount++;
    g_recorder.address = address;
    g_recorder.readLength = length;
    for ((index = 0U); (index < length) &&
         (index < TEST_DHT20_FRAME_LENGTH); index++) {
        data[index] = g_recorder.frame[index];
    }

    return g_recorder.readResult;
}

platform_error_t platform_i2c_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength)
{
    (void)i2c;
    (void)address;
    (void)txData;
    (void)txLength;
    (void)rxData;
    (void)rxLength;
    g_recorder.writeReadCallCount++;
    return PLATFORM_ERR_UNKNOWN;
}

platform_error_t platform_i2c_deinit(platform_i2c_t *i2c)
{
    (void)i2c;
    g_recorder.i2cDeinitCallCount++;
    return PLATFORM_ERR_UNKNOWN;
}

platform_error_t platform_time_delay_ms(uint32_t delayMs)
{
    g_recorder.delayCallCount++;
    g_recorder.delayMs = delayMs;
    return g_recorder.delayResult;
}

static int test_init_validates_lifecycle_and_binds_shared_i2c(void)
{
    platform_dht20_t dht20 = PLATFORM_DHT20_INITIALIZER;
    platform_i2c_t i2c = test_initialized_i2c();
    platform_i2c_t uninitializedI2c = PLATFORM_I2C_INITIALIZER;

    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER == platform_dht20_init(NULL, &i2c));
    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER == platform_dht20_init(&dht20, NULL));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED ==
                platform_dht20_init(&dht20, &uninitializedI2c));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_dht20_init(&dht20, &i2c));
    TEST_ASSERT(&i2c == dht20.i2c);
    TEST_ASSERT(1U == dht20.initialized);
    TEST_ASSERT(PLATFORM_ERR_ALREADY_INITIALIZED ==
                platform_dht20_init(&dht20, &i2c));
    TEST_ASSERT(0U == g_recorder.writeCallCount);
    TEST_ASSERT(0U == g_recorder.readCallCount);

    return 0;
}

static int test_read_validates_required_state_and_output(void)
{
    platform_dht20_t dht20 = PLATFORM_DHT20_INITIALIZER;
    platform_dht20_measurement_t measurement = test_sentinel_measurement();
    platform_i2c_t i2c = test_initialized_i2c();

    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER == platform_dht20_read(NULL,
                                                                 &measurement));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED ==
                platform_dht20_read(&dht20, &measurement));
    dht20.initialized = 1U;
    TEST_ASSERT(PLATFORM_ERR_INVALID_STATE ==
                platform_dht20_read(&dht20, &measurement));
    dht20.initialized = 0U;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_dht20_init(&dht20, &i2c));
    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER == platform_dht20_read(&dht20, NULL));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    return 0;
}

static int test_read_uses_fixed_transaction_and_converts_measurement(void)
{
    platform_dht20_t dht20 = PLATFORM_DHT20_INITIALIZER;
    platform_dht20_measurement_t measurement = {0};
    platform_i2c_t i2c = test_initialized_i2c();

    test_set_valid_frame();
    TEST_ASSERT(PLATFORM_ERR_OK == platform_dht20_init(&dht20, &i2c));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_dht20_read(&dht20, &measurement));
    TEST_ASSERT(1U == g_recorder.writeCallCount);
    TEST_ASSERT(TEST_DHT20_ADDRESS == g_recorder.address);
    TEST_ASSERT(TEST_DHT20_COMMAND_LENGTH == g_recorder.writeLength);
    TEST_ASSERT(0xACU == g_recorder.command[0]);
    TEST_ASSERT(0x33U == g_recorder.command[1]);
    TEST_ASSERT(0x00U == g_recorder.command[2]);
    TEST_ASSERT(1U == g_recorder.delayCallCount);
    TEST_ASSERT(80U == g_recorder.delayMs);
    TEST_ASSERT(1U == g_recorder.readCallCount);
    TEST_ASSERT(TEST_DHT20_FRAME_LENGTH == g_recorder.readLength);
    TEST_ASSERT(0U == g_recorder.writeReadCallCount);
    TEST_ASSERT(0x18U == measurement.status);
    TEST_ASSERT(0x80000U == measurement.rawHumidity);
    TEST_ASSERT(0x80000U == measurement.rawTemperature);
    TEST_ASSERT(50.0f == measurement.humidityPercent);
    TEST_ASSERT(50.0f == measurement.temperatureC);

    return 0;
}

static int test_read_propagates_each_transport_error_atomically(void)
{
    platform_dht20_t dht20 = PLATFORM_DHT20_INITIALIZER;
    platform_dht20_measurement_t measurement = test_sentinel_measurement();
    platform_i2c_t i2c = test_initialized_i2c();

    TEST_ASSERT(PLATFORM_ERR_OK == platform_dht20_init(&dht20, &i2c));

    g_recorder.writeResult = PLATFORM_ERR_NOT_FOUND;
    TEST_ASSERT(PLATFORM_ERR_NOT_FOUND == platform_dht20_read(&dht20,
                                                              &measurement));
    TEST_ASSERT(0U == g_recorder.delayCallCount);
    TEST_ASSERT(0U == g_recorder.readCallCount);
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    g_recorder.writeResult = PLATFORM_ERR_OK;
    g_recorder.delayResult = PLATFORM_ERR_INVALID_STATE;
    TEST_ASSERT(PLATFORM_ERR_INVALID_STATE == platform_dht20_read(&dht20,
                                                                  &measurement));
    TEST_ASSERT(0U == g_recorder.readCallCount);
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    g_recorder.delayResult = PLATFORM_ERR_OK;
    g_recorder.readResult = PLATFORM_ERR_TIMEOUT;
    TEST_ASSERT(PLATFORM_ERR_TIMEOUT == platform_dht20_read(&dht20,
                                                            &measurement));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    return 0;
}

static int test_read_rejects_busy_crc_otp_and_calibration_atomically(void)
{
    platform_dht20_t dht20 = PLATFORM_DHT20_INITIALIZER;
    platform_dht20_measurement_t measurement = test_sentinel_measurement();
    platform_i2c_t i2c = test_initialized_i2c();

    TEST_ASSERT(PLATFORM_ERR_OK == platform_dht20_init(&dht20, &i2c));

    test_set_valid_frame();
    g_recorder.frame[0] |= 0x80U;
    g_recorder.frame[6] = 0x38U;
    TEST_ASSERT(PLATFORM_ERR_BUSY == platform_dht20_read(&dht20, &measurement));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    test_set_valid_frame();
    g_recorder.frame[6] = 0xD5U;
    TEST_ASSERT(PLATFORM_ERR_CHECKSUM ==
                platform_dht20_read(&dht20, &measurement));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    test_set_valid_frame();
    g_recorder.frame[0] &= (uint8_t)~0x10U;
    g_recorder.frame[6] = 0x51U;
    TEST_ASSERT(PLATFORM_ERR_CHECKSUM ==
                platform_dht20_read(&dht20, &measurement));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    test_set_valid_frame();
    g_recorder.frame[0] &= (uint8_t)~0x08U;
    g_recorder.frame[6] = 0x0EU;
    TEST_ASSERT(PLATFORM_ERR_INVALID_STATE ==
                platform_dht20_read(&dht20, &measurement));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    return 0;
}

static int test_deinit_clears_only_dht20_binding(void)
{
    platform_dht20_t dht20 = PLATFORM_DHT20_INITIALIZER;
    platform_i2c_t i2c = test_initialized_i2c();

    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER == platform_dht20_deinit(NULL));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED == platform_dht20_deinit(&dht20));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_dht20_init(&dht20, &i2c));
    i2c.initialized = 0U;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_dht20_deinit(&dht20));
    TEST_ASSERT(NULL == dht20.i2c);
    TEST_ASSERT(0U == dht20.initialized);
    TEST_ASSERT(0U == i2c.initialized);
    TEST_ASSERT(0U == g_recorder.i2cDeinitCallCount);

    return 0;
}
//******************************** Private Functions ***********************//

//******************************** Functions *******************************//
int main(void)
{
    int result = 0;

    test_reset_recorder();
    result = test_init_validates_lifecycle_and_binds_shared_i2c();
    if (0 != result) {
        return result;
    }

    test_reset_recorder();
    result = test_read_validates_required_state_and_output();
    if (0 != result) {
        return result;
    }

    test_reset_recorder();
    result = test_read_uses_fixed_transaction_and_converts_measurement();
    if (0 != result) {
        return result;
    }

    test_reset_recorder();
    result = test_read_propagates_each_transport_error_atomically();
    if (0 != result) {
        return result;
    }

    test_reset_recorder();
    result = test_read_rejects_busy_crc_otp_and_calibration_atomically();
    if (0 != result) {
        return result;
    }

    test_reset_recorder();
    return test_deinit_clears_only_dht20_binding();
}
//******************************** Functions *******************************//
