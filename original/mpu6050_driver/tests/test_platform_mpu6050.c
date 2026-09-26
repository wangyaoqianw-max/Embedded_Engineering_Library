/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_platform_mpu6050.c
 * @brief 验证 Platform MPU6050 Phase 7 公共合同
 * @author YaoQian Wang
 * @date 2026-09-04
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include <stddef.h>
#include <stdio.h>

#include "mpu6050/platform_mpu6050.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define TEST_ASSERT(condition)       \
    do {                             \
        if (!(condition)) {          \
            (void)printf("assertion failed at line %d\n", __LINE__); \
            return __LINE__;         \
        }                            \
    } while (0)

#define TEST_MPU6050_ADDRESS_LOW        (0x68U)
#define TEST_MPU6050_ADDRESS_HIGH       (0x69U)
#define TEST_MPU6050_REGISTER_WHO_AM_I  (0x75U)
#define TEST_MPU6050_REGISTER_DATA      (0x3BU)
#define TEST_MPU6050_FRAME_LENGTH       (14U)
#define TEST_MPU6050_CONFIG_COUNT       (5U)
//******************************** Defines *********************************//

//******************************** Types ***********************************//
typedef struct
{
    platform_error_t writeReadResult;
    platform_error_t writeResults[TEST_MPU6050_CONFIG_COUNT];
    uint8_t whoAmI;
    uint8_t frame[TEST_MPU6050_FRAME_LENGTH];
    uint8_t writeAddress[TEST_MPU6050_CONFIG_COUNT];
    uint8_t writeRegister[TEST_MPU6050_CONFIG_COUNT];
    uint8_t writeValue[TEST_MPU6050_CONFIG_COUNT];
    uint8_t writeReadAddress;
    uint8_t writeReadRegister;
    uint16_t writeLengths[TEST_MPU6050_CONFIG_COUNT];
    uint16_t writeReadTxLength;
    uint16_t writeReadRxLength;
    uint32_t writeCallCount;
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

    g_recorder.whoAmI = 0x68U;
}

static platform_i2c_t test_initialized_i2c(void)
{
    platform_i2c_t i2c = PLATFORM_I2C_INITIALIZER;

    i2c.initialized = 1U;
    return i2c;
}

static platform_mpu6050_measurement_t test_sentinel_measurement(void)
{
    platform_mpu6050_measurement_t measurement = {
        101,
        -102,
        103,
        -201,
        202,
        -203,
        1.25f,
        -2.5f,
        3.75f,
        -4.25f,
        5.5f,
        -6.75f
    };

    return measurement;
}

static int test_measurement_unchanged(
    const platform_mpu6050_measurement_t *measurement)
{
    TEST_ASSERT(101 == measurement->accelXRaw);
    TEST_ASSERT(-102 == measurement->accelYRaw);
    TEST_ASSERT(103 == measurement->accelZRaw);
    TEST_ASSERT(-201 == measurement->gyroXRaw);
    TEST_ASSERT(202 == measurement->gyroYRaw);
    TEST_ASSERT(-203 == measurement->gyroZRaw);
    TEST_ASSERT(1.25f == measurement->accelXG);
    TEST_ASSERT(-2.5f == measurement->accelYG);
    TEST_ASSERT(3.75f == measurement->accelZG);
    TEST_ASSERT(-4.25f == measurement->gyroXDps);
    TEST_ASSERT(5.5f == measurement->gyroYDps);
    TEST_ASSERT(-6.75f == measurement->gyroZDps);

    return 0;
}

static void test_set_axis_data(void)
{
    static const uint8_t frame[TEST_MPU6050_FRAME_LENGTH] = {
        0x40U, 0x00U,
        0xC0U, 0x00U,
        0x20U, 0x00U,
        0x12U, 0x34U,
        0x00U, 0x83U,
        0xFFU, 0x7DU,
        0x01U, 0x06U
    };
    uint32_t index = 0U;

    for (index = 0U; index < TEST_MPU6050_FRAME_LENGTH; index++) {
        g_recorder.frame[index] = frame[index];
    }
}

platform_error_t platform_i2c_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length)
{
    uint32_t index = g_recorder.writeCallCount;

    (void)i2c;
    if (index < TEST_MPU6050_CONFIG_COUNT) {
        g_recorder.writeAddress[index] = address;
        g_recorder.writeLengths[index] = length;
        if ((data != NULL) && (length >= 2U)) {
            g_recorder.writeRegister[index] = data[0];
            g_recorder.writeValue[index] = data[1];
        }
    }
    g_recorder.writeCallCount++;

    if (index < TEST_MPU6050_CONFIG_COUNT) {
        return g_recorder.writeResults[index];
    }

    return PLATFORM_ERR_UNKNOWN;
}

platform_error_t platform_i2c_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength)
{
    uint32_t index = 0U;

    (void)i2c;
    g_recorder.writeReadCallCount++;
    g_recorder.writeReadAddress = address;
    g_recorder.writeReadTxLength = txLength;
    g_recorder.writeReadRxLength = rxLength;
    if ((txData != NULL) && (txLength > 0U)) {
        g_recorder.writeReadRegister = txData[0];
    }

    if (g_recorder.writeReadResult != PLATFORM_ERR_OK) {
        return g_recorder.writeReadResult;
    }

    if ((rxData != NULL) && (rxLength == 1U)) {
        rxData[0] = g_recorder.whoAmI;
    } else if (rxData != NULL) {
        for ((index = 0U); (index < rxLength) &&
             (index < TEST_MPU6050_FRAME_LENGTH); index++) {
            rxData[index] = g_recorder.frame[index];
        }
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_i2c_deinit(platform_i2c_t *i2c)
{
    (void)i2c;
    g_recorder.i2cDeinitCallCount++;
    return PLATFORM_ERR_UNKNOWN;
}

static int test_init_validates_arguments_and_lifecycle(void)
{
    platform_mpu6050_t mpu6050 = PLATFORM_MPU6050_INITIALIZER;
    platform_i2c_t i2c = test_initialized_i2c();
    platform_i2c_t uninitializedI2c = PLATFORM_I2C_INITIALIZER;

    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER ==
                platform_mpu6050_init(NULL, &i2c, TEST_MPU6050_ADDRESS_LOW));
    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER ==
                platform_mpu6050_init(&mpu6050, NULL,
                                      TEST_MPU6050_ADDRESS_LOW));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_mpu6050_init(&mpu6050, &i2c, 0x67U));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_mpu6050_init(&mpu6050, &i2c, 0x6AU));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED ==
                platform_mpu6050_init(&mpu6050, &uninitializedI2c,
                                      TEST_MPU6050_ADDRESS_LOW));
    TEST_ASSERT(0U == g_recorder.writeReadCallCount);
    TEST_ASSERT(0U == g_recorder.writeCallCount);

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_LOW));
    TEST_ASSERT(PLATFORM_ERR_ALREADY_INITIALIZED ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_LOW));

    return 0;
}

static int test_init_accepts_ad0_high_and_applies_fixed_sequence(void)
{
    static const uint8_t expectedRegisters[TEST_MPU6050_CONFIG_COUNT] = {
        0x6BU, 0x1AU, 0x19U, 0x1BU, 0x1CU
    };
    static const uint8_t expectedValues[TEST_MPU6050_CONFIG_COUNT] = {
        0x01U, 0x03U, 0x04U, 0x00U, 0x00U
    };
    platform_mpu6050_t mpu6050 = PLATFORM_MPU6050_INITIALIZER;
    platform_i2c_t i2c = test_initialized_i2c();
    uint32_t index = 0U;

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_HIGH));
    TEST_ASSERT(&i2c == mpu6050.i2c);
    TEST_ASSERT(TEST_MPU6050_ADDRESS_HIGH == mpu6050.address);
    TEST_ASSERT(1U == mpu6050.initialized);
    TEST_ASSERT(1U == g_recorder.writeReadCallCount);
    TEST_ASSERT(TEST_MPU6050_ADDRESS_HIGH == g_recorder.writeReadAddress);
    TEST_ASSERT(TEST_MPU6050_REGISTER_WHO_AM_I ==
                g_recorder.writeReadRegister);
    TEST_ASSERT(1U == g_recorder.writeReadTxLength);
    TEST_ASSERT(1U == g_recorder.writeReadRxLength);
    TEST_ASSERT(TEST_MPU6050_CONFIG_COUNT == g_recorder.writeCallCount);

    for (index = 0U; index < TEST_MPU6050_CONFIG_COUNT; index++) {
        TEST_ASSERT(TEST_MPU6050_ADDRESS_HIGH ==
                    g_recorder.writeAddress[index]);
        TEST_ASSERT(2U == g_recorder.writeLengths[index]);
        TEST_ASSERT(expectedRegisters[index] ==
                    g_recorder.writeRegister[index]);
        TEST_ASSERT(expectedValues[index] == g_recorder.writeValue[index]);
    }

    return 0;
}

static int test_init_preserves_errors_and_commits_only_after_success(void)
{
    platform_mpu6050_t mpu6050 = PLATFORM_MPU6050_INITIALIZER;
    platform_i2c_t oldI2c = PLATFORM_I2C_INITIALIZER;
    platform_i2c_t i2c = test_initialized_i2c();
    uint32_t failureIndex = 0U;

    mpu6050.i2c = &oldI2c;
    mpu6050.address = 0x55U;
    g_recorder.writeReadResult = PLATFORM_ERR_TIMEOUT;
    TEST_ASSERT(PLATFORM_ERR_TIMEOUT ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_LOW));
    TEST_ASSERT(&oldI2c == mpu6050.i2c);
    TEST_ASSERT(0x55U == mpu6050.address);
    TEST_ASSERT(0U == mpu6050.initialized);

    test_reset_recorder();
    g_recorder.whoAmI = 0x69U;
    TEST_ASSERT(PLATFORM_ERR_NOT_FOUND ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_HIGH));
    TEST_ASSERT(0U == g_recorder.writeCallCount);
    TEST_ASSERT(&oldI2c == mpu6050.i2c);
    TEST_ASSERT(0x55U == mpu6050.address);
    TEST_ASSERT(0U == mpu6050.initialized);

    for (failureIndex = 0U;
         failureIndex < TEST_MPU6050_CONFIG_COUNT;
         failureIndex++) {
        test_reset_recorder();
        g_recorder.writeResults[failureIndex] = PLATFORM_ERR_IO;
        TEST_ASSERT(PLATFORM_ERR_IO ==
                    platform_mpu6050_init(&mpu6050, &i2c,
                                          TEST_MPU6050_ADDRESS_LOW));
        TEST_ASSERT((failureIndex + 1U) == g_recorder.writeCallCount);
        TEST_ASSERT(&oldI2c == mpu6050.i2c);
        TEST_ASSERT(0x55U == mpu6050.address);
        TEST_ASSERT(0U == mpu6050.initialized);
    }

    return 0;
}

static int test_read_validates_state_and_preserves_output(void)
{
    platform_mpu6050_t mpu6050 = PLATFORM_MPU6050_INITIALIZER;
    platform_mpu6050_measurement_t measurement = test_sentinel_measurement();
    platform_i2c_t i2c = test_initialized_i2c();

    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER ==
                platform_mpu6050_read(NULL, &measurement));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED ==
                platform_mpu6050_read(&mpu6050, &measurement));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_LOW));
    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER ==
                platform_mpu6050_read(&mpu6050, NULL));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    i2c.initialized = 0U;
    TEST_ASSERT(PLATFORM_ERR_INVALID_STATE ==
                platform_mpu6050_read(&mpu6050, &measurement));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    return 0;
}

static int test_read_uses_one_burst_and_converts_signed_axes(void)
{
    platform_mpu6050_t mpu6050 = PLATFORM_MPU6050_INITIALIZER;
    platform_mpu6050_measurement_t measurement = {0};
    platform_i2c_t i2c = test_initialized_i2c();

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_LOW));
    test_reset_recorder();
    test_set_axis_data();

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_mpu6050_read(&mpu6050, &measurement));
    TEST_ASSERT(1U == g_recorder.writeReadCallCount);
    TEST_ASSERT(0U == g_recorder.writeCallCount);
    TEST_ASSERT(TEST_MPU6050_ADDRESS_LOW == g_recorder.writeReadAddress);
    TEST_ASSERT(TEST_MPU6050_REGISTER_DATA == g_recorder.writeReadRegister);
    TEST_ASSERT(1U == g_recorder.writeReadTxLength);
    TEST_ASSERT(TEST_MPU6050_FRAME_LENGTH == g_recorder.writeReadRxLength);
    TEST_ASSERT(16384 == measurement.accelXRaw);
    TEST_ASSERT(-16384 == measurement.accelYRaw);
    TEST_ASSERT(8192 == measurement.accelZRaw);
    TEST_ASSERT(131 == measurement.gyroXRaw);
    TEST_ASSERT(-131 == measurement.gyroYRaw);
    TEST_ASSERT(262 == measurement.gyroZRaw);
    TEST_ASSERT(1.0f == measurement.accelXG);
    TEST_ASSERT(-1.0f == measurement.accelYG);
    TEST_ASSERT(0.5f == measurement.accelZG);
    TEST_ASSERT(1.0f == measurement.gyroXDps);
    TEST_ASSERT(-1.0f == measurement.gyroYDps);
    TEST_ASSERT(2.0f == measurement.gyroZDps);

    return 0;
}

static int test_failed_read_propagates_error_atomically(void)
{
    platform_mpu6050_t mpu6050 = PLATFORM_MPU6050_INITIALIZER;
    platform_mpu6050_measurement_t measurement = test_sentinel_measurement();
    platform_i2c_t i2c = test_initialized_i2c();

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_LOW));
    test_reset_recorder();
    g_recorder.writeReadResult = PLATFORM_ERR_TIMEOUT;

    TEST_ASSERT(PLATFORM_ERR_TIMEOUT ==
                platform_mpu6050_read(&mpu6050, &measurement));
    TEST_ASSERT(0 == test_measurement_unchanged(&measurement));

    return 0;
}

static int test_deinit_clears_only_mpu6050_binding(void)
{
    platform_mpu6050_t mpu6050 = PLATFORM_MPU6050_INITIALIZER;
    platform_i2c_t i2c = test_initialized_i2c();

    TEST_ASSERT(PLATFORM_ERR_NULL_POINTER == platform_mpu6050_deinit(NULL));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED ==
                platform_mpu6050_deinit(&mpu6050));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_mpu6050_init(&mpu6050, &i2c,
                                      TEST_MPU6050_ADDRESS_LOW));
    i2c.initialized = 0U;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_mpu6050_deinit(&mpu6050));
    TEST_ASSERT(NULL == mpu6050.i2c);
    TEST_ASSERT(0U == mpu6050.address);
    TEST_ASSERT(0U == mpu6050.initialized);
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
    result = test_init_validates_arguments_and_lifecycle();
    if (result != 0) {
        return result;
    }

    test_reset_recorder();
    result = test_init_accepts_ad0_high_and_applies_fixed_sequence();
    if (result != 0) {
        return result;
    }

    test_reset_recorder();
    result = test_init_preserves_errors_and_commits_only_after_success();
    if (result != 0) {
        return result;
    }

    test_reset_recorder();
    result = test_read_validates_state_and_preserves_output();
    if (result != 0) {
        return result;
    }

    test_reset_recorder();
    result = test_read_uses_one_burst_and_converts_signed_axes();
    if (result != 0) {
        return result;
    }

    test_reset_recorder();
    result = test_failed_read_propagates_error_atomically();
    if (result != 0) {
        return result;
    }

    test_reset_recorder();
    return test_deinit_clears_only_mpu6050_binding();
}
//******************************** Functions *******************************//
