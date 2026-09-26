/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_platform_i2c.c
 * @brief 验证 Platform Software I2C 同步事务合同
 * @author YaoQian Wang
 * @date 2026-09-02
 * @version V1.1
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include <stddef.h>
#include <string.h>

#include "platform_i2c.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define TEST_ASSERT(condition)       \
    do {                             \
        if (!(condition)) {          \
            return __LINE__;         \
        }                            \
    } while (0)

#define TEST_MAX_EVENTS              (4096U)
#define TEST_MAX_READ_VALUES         (512U)
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
typedef enum
{
    TEST_LINE_NONE = 0,
    TEST_LINE_SCL,
    TEST_LINE_SDA
} test_line_t;

typedef enum
{
    TEST_EVENT_CONFIGURE = 0,
    TEST_EVENT_WRITE,
    TEST_EVENT_READ,
    TEST_EVENT_DEINIT,
    TEST_EVENT_DELAY
} test_event_type_t;

typedef struct
{
    test_event_type_t type;
    test_line_t line;
    uint32_t value;
    platform_error_t result;
} test_event_t;

typedef struct
{
    test_event_t events[TEST_MAX_EVENTS];
    uint32_t eventCount;
    platform_gpio_level_t sclOutputLevel;
    platform_gpio_level_t sdaOutputLevel;
    platform_gpio_level_t sclRiseSdaLevels[TEST_MAX_EVENTS];
    uint32_t sclRiseCount;
    uint32_t startConditionCount;
    uint32_t stopConditionCount;
} test_recorder_t;

typedef struct
{
    test_line_t line;
    test_recorder_t *recorder;
    platform_gpio_level_t defaultReadLevel;
    platform_gpio_level_t readValues[TEST_MAX_READ_VALUES];
    uint32_t readValueCount;
    uint32_t readValueIndex;
    uint32_t configureCallCount;
    uint32_t writeCallCount;
    uint32_t readCallCount;
    uint32_t deinitCallCount;
    uint32_t failConfigureCall;
    uint32_t failWriteCall;
    uint32_t failReadCall;
    uint32_t failDeinitCall;
    platform_error_t configureError;
    platform_error_t writeError;
    platform_error_t readError;
    platform_error_t deinitError;
    platform_gpio_config_t configuredValue;
} test_gpio_context_t;

typedef struct
{
    platform_i2c_t i2c;
    platform_gpio_t scl;
    platform_gpio_t sda;
    test_gpio_context_t sclContext;
    test_gpio_context_t sdaContext;
    test_recorder_t recorder;
} test_fixture_t;
//******************************** Declaring *********************************//

//******************************** Variables *********************************//
static test_recorder_t *g_delayRecorder = NULL;
//******************************** Variables *********************************//

//******************************** Functions *********************************//
static void record_event(
    test_recorder_t *recorder,
    test_event_type_t type,
    test_line_t line,
    uint32_t value,
    platform_error_t result)
{
    if (recorder->eventCount < TEST_MAX_EVENTS) {
        test_event_t *event = &recorder->events[recorder->eventCount];

        event->type = type;
        event->line = line;
        event->value = value;
        event->result = result;
        recorder->eventCount++;
    }
}

static platform_error_t fake_gpio_configure(
    platform_gpio_t *gpio,
    const platform_gpio_config_t *config)
{
    test_gpio_context_t *context = (test_gpio_context_t *)gpio->implContext;
    platform_error_t result = PLATFORM_ERR_OK;

    context->configureCallCount++;
    if (context->configureCallCount == context->failConfigureCall) {
        result = context->configureError;
    }

    record_event(context->recorder,
                 TEST_EVENT_CONFIGURE,
                 context->line,
                 0U,
                 result);
    if (result == PLATFORM_ERR_OK) {
        context->configuredValue = *config;
    }

    return result;
}

static platform_error_t fake_gpio_write(
    platform_gpio_t *gpio,
    platform_gpio_level_t level)
{
    test_gpio_context_t *context = (test_gpio_context_t *)gpio->implContext;
    platform_error_t result = PLATFORM_ERR_OK;
    platform_gpio_level_t previousLevel = PLATFORM_GPIO_LEVEL_HIGH;

    context->writeCallCount++;
    if (context->writeCallCount == context->failWriteCall) {
        result = context->writeError;
    }

    if (context->line == TEST_LINE_SCL) {
        previousLevel = context->recorder->sclOutputLevel;
    } else {
        previousLevel = context->recorder->sdaOutputLevel;
    }

    record_event(context->recorder,
                 TEST_EVENT_WRITE,
                 context->line,
                 (uint32_t)level,
                 result);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if (context->line == TEST_LINE_SCL) {
        context->recorder->sclOutputLevel = level;
        if ((previousLevel == PLATFORM_GPIO_LEVEL_LOW) &&
            (level == PLATFORM_GPIO_LEVEL_HIGH) &&
            (context->recorder->sclRiseCount < TEST_MAX_EVENTS)) {
            uint32_t index = context->recorder->sclRiseCount;

            context->recorder->sclRiseSdaLevels[index] =
                context->recorder->sdaOutputLevel;
            context->recorder->sclRiseCount++;
        }
    } else {
        context->recorder->sdaOutputLevel = level;
        if ((previousLevel == PLATFORM_GPIO_LEVEL_HIGH) &&
            (level == PLATFORM_GPIO_LEVEL_LOW) &&
            (context->recorder->sclOutputLevel == PLATFORM_GPIO_LEVEL_HIGH)) {
            context->recorder->startConditionCount++;
        }
        if ((previousLevel == PLATFORM_GPIO_LEVEL_LOW) &&
            (level == PLATFORM_GPIO_LEVEL_HIGH) &&
            (context->recorder->sclOutputLevel == PLATFORM_GPIO_LEVEL_HIGH)) {
            context->recorder->stopConditionCount++;
        }
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t fake_gpio_read(
    platform_gpio_t *gpio,
    platform_gpio_level_t *level)
{
    test_gpio_context_t *context = (test_gpio_context_t *)gpio->implContext;
    platform_error_t result = PLATFORM_ERR_OK;

    context->readCallCount++;
    if (context->readCallCount == context->failReadCall) {
        result = context->readError;
    }

    if (context->readValueIndex < context->readValueCount) {
        *level = context->readValues[context->readValueIndex];
        context->readValueIndex++;
    } else {
        *level = context->defaultReadLevel;
    }

    record_event(context->recorder,
                 TEST_EVENT_READ,
                 context->line,
                 (uint32_t)(*level),
                 result);

    return result;
}

static platform_error_t fake_gpio_deinit(platform_gpio_t *gpio)
{
    test_gpio_context_t *context = (test_gpio_context_t *)gpio->implContext;
    platform_error_t result = PLATFORM_ERR_OK;

    context->deinitCallCount++;
    if (context->deinitCallCount == context->failDeinitCall) {
        result = context->deinitError;
    }

    record_event(context->recorder,
                 TEST_EVENT_DEINIT,
                 context->line,
                 0U,
                 result);

    return result;
}

static const platform_gpio_ops_t g_fakeGpioOps = {
    fake_gpio_configure,
    fake_gpio_write,
    fake_gpio_read,
    fake_gpio_deinit
};

void platform_delay_us(uint32_t us)
{
    if (g_delayRecorder != NULL) {
        record_event(g_delayRecorder,
                     TEST_EVENT_DELAY,
                     TEST_LINE_NONE,
                     us,
                     PLATFORM_ERR_OK);
    }
}

static void script_read_values(
    test_gpio_context_t *context,
    const platform_gpio_level_t *values,
    uint32_t count)
{
    uint32_t index = 0U;

    context->readValueCount = count;
    context->readValueIndex = 0U;
    for (index = 0U; index < count; index++) {
        context->readValues[index] = values[index];
    }
}

static int initialize_fixture(test_fixture_t *fixture)
{
    platform_gpio_init_params_t sclParams = {0};
    platform_gpio_init_params_t sdaParams = {0};

    (void)memset(fixture, 0, sizeof(*fixture));
    fixture->recorder.sclOutputLevel = PLATFORM_GPIO_LEVEL_HIGH;
    fixture->recorder.sdaOutputLevel = PLATFORM_GPIO_LEVEL_HIGH;
    fixture->sclContext.line = TEST_LINE_SCL;
    fixture->sclContext.recorder = &fixture->recorder;
    fixture->sclContext.defaultReadLevel = PLATFORM_GPIO_LEVEL_HIGH;
    fixture->sdaContext.line = TEST_LINE_SDA;
    fixture->sdaContext.recorder = &fixture->recorder;
    fixture->sdaContext.defaultReadLevel = PLATFORM_GPIO_LEVEL_HIGH;
    sclParams.name = "test_scl";
    sclParams.ops = &g_fakeGpioOps;
    sclParams.implContext = &fixture->sclContext;
    sdaParams.name = "test_sda";
    sdaParams.ops = &g_fakeGpioOps;
    sdaParams.implContext = &fixture->sdaContext;
    g_delayRecorder = &fixture->recorder;

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_gpio_init(&fixture->scl, &sclParams));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_gpio_init(&fixture->sda, &sdaParams));

    return 0;
}

static uint32_t count_events(
    const test_recorder_t *recorder,
    test_event_type_t type,
    test_line_t line,
    uint32_t value)
{
    uint32_t count = 0U;
    uint32_t index = 0U;

    for (index = 0U; index < recorder->eventCount; index++) {
        const test_event_t *event = &recorder->events[index];

        if ((event->type == type) &&
            (event->line == line) &&
            (event->value == value)) {
            count++;
        }
    }

    return count;
}

static const test_event_t *get_write_event(
    const test_recorder_t *recorder,
    uint32_t writeIndex)
{
    uint32_t currentWriteIndex = 0U;
    uint32_t eventIndex = 0U;

    for (eventIndex = 0U; eventIndex < recorder->eventCount; eventIndex++) {
        const test_event_t *event = &recorder->events[eventIndex];

        if (event->type == TEST_EVENT_WRITE) {
            if (currentWriteIndex == writeIndex) {
                return event;
            }
            currentWriteIndex++;
        }
    }

    return NULL;
}

static int assert_clocked_byte(
    const test_recorder_t *recorder,
    uint32_t firstRiseIndex,
    uint8_t expectedByte)
{
    uint32_t bitIndex = 0U;

    TEST_ASSERT((firstRiseIndex + 8U) <= recorder->sclRiseCount);
    for (bitIndex = 0U; bitIndex < 8U; bitIndex++) {
        platform_gpio_level_t expectedLevel = PLATFORM_GPIO_LEVEL_LOW;

        if ((expectedByte & (uint8_t)(0x80U >> bitIndex)) != 0U) {
            expectedLevel = PLATFORM_GPIO_LEVEL_HIGH;
        }
        TEST_ASSERT(expectedLevel ==
                    recorder->sclRiseSdaLevels[firstRiseIndex + bitIndex]);
    }

    return 0;
}

static int test_init_rejects_null_required_objects(void)
{
    platform_i2c_t i2c = PLATFORM_I2C_INITIALIZER;
    platform_gpio_t scl = PLATFORM_GPIO_INITIALIZER;
    platform_gpio_t sda = PLATFORM_GPIO_INITIALIZER;

    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_init(NULL, "test_i2c", &scl, &sda));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_init(&i2c, "test_i2c", NULL, &sda));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_init(&i2c, "test_i2c", &scl, NULL));

    return 0;
}

static int test_init_configures_open_drain_output_and_releases_bus(void)
{
    test_fixture_t fixture;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    TEST_ASSERT(1U == fixture.sclContext.configureCallCount);
    TEST_ASSERT(1U == fixture.sdaContext.configureCallCount);
    TEST_ASSERT(PLATFORM_GPIO_DIRECTION_OUTPUT ==
                fixture.sclContext.configuredValue.direction);
    TEST_ASSERT(PLATFORM_GPIO_DIRECTION_OUTPUT ==
                fixture.sdaContext.configuredValue.direction);
    TEST_ASSERT(PLATFORM_GPIO_OUTPUT_OPEN_DRAIN ==
                fixture.sclContext.configuredValue.outputType);
    TEST_ASSERT(PLATFORM_GPIO_OUTPUT_OPEN_DRAIN ==
                fixture.sdaContext.configuredValue.outputType);
    TEST_ASSERT(PLATFORM_GPIO_PULL_NONE ==
                fixture.sclContext.configuredValue.pull);
    TEST_ASSERT(PLATFORM_GPIO_PULL_NONE ==
                fixture.sdaContext.configuredValue.pull);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.sclContext.configuredValue.initialLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.sdaContext.configuredValue.initialLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sclOutputLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sdaOutputLevel);
    TEST_ASSERT(fixture.i2c.initialized != 0U);

    return 0;
}

static int test_init_recovers_sda_stuck_low_and_generates_stop(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_HIGH
    };
    test_fixture_t fixture;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    TEST_ASSERT(2U == count_events(&fixture.recorder,
                                  TEST_EVENT_WRITE,
                                  TEST_LINE_SCL,
                                  PLATFORM_GPIO_LEVEL_LOW));
    TEST_ASSERT(count_events(&fixture.recorder,
                             TEST_EVENT_WRITE,
                             TEST_LINE_SCL,
                             PLATFORM_GPIO_LEVEL_LOW) <= 9U);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sclOutputLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sdaOutputLevel);
    TEST_ASSERT(fixture.i2c.initialized != 0U);

    return 0;
}

static int test_init_fails_when_scl_cannot_become_high(void)
{
    test_fixture_t fixture;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    fixture.sclContext.defaultReadLevel = PLATFORM_GPIO_LEVEL_LOW;

    TEST_ASSERT(PLATFORM_ERR_TIMEOUT ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    TEST_ASSERT(0U == fixture.i2c.initialized);
    TEST_ASSERT(count_events(&fixture.recorder,
                             TEST_EVENT_DELAY,
                             TEST_LINE_NONE,
                             1U) > 0U);

    return 0;
}

static int test_transaction_rejects_non_idle_bus_without_recovery(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW
    };
    test_fixture_t fixture;
    uint8_t data = 0x55U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));

    fixture.recorder.sclRiseCount = 0U;
    TEST_ASSERT(PLATFORM_ERR_BUSY ==
                platform_i2c_write(&fixture.i2c, 0x38U, &data, 1U));
    TEST_ASSERT(0U == fixture.recorder.sclRiseCount);
    TEST_ASSERT(0U == count_events(&fixture.recorder,
                                  TEST_EVENT_WRITE,
                                  TEST_LINE_SCL,
                                  PLATFORM_GPIO_LEVEL_LOW));

    return 0;
}

static int test_write_generates_start_msb_bytes_ack_clocks_and_stop(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW
    };
    const test_event_t *event = NULL;
    test_fixture_t fixture;
    uint8_t data = 0xA5U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.recorder.eventCount = 0U;
    fixture.recorder.sclRiseCount = 0U;
    fixture.recorder.startConditionCount = 0U;
    fixture.recorder.stopConditionCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_write(&fixture.i2c, 0x38U, &data, 1U));
    event = get_write_event(&fixture.recorder, 0U);
    TEST_ASSERT((event != NULL) &&
                (event->line == TEST_LINE_SCL) &&
                (event->value == PLATFORM_GPIO_LEVEL_HIGH));
    event = get_write_event(&fixture.recorder, 1U);
    TEST_ASSERT((event != NULL) &&
                (event->line == TEST_LINE_SDA) &&
                (event->value == PLATFORM_GPIO_LEVEL_HIGH));
    event = get_write_event(&fixture.recorder, 2U);
    TEST_ASSERT((event != NULL) &&
                (event->line == TEST_LINE_SCL) &&
                (event->value == PLATFORM_GPIO_LEVEL_HIGH));
    event = get_write_event(&fixture.recorder, 3U);
    TEST_ASSERT((event != NULL) &&
                (event->line == TEST_LINE_SDA) &&
                (event->value == PLATFORM_GPIO_LEVEL_LOW));
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 0U, 0x70U));
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.recorder.sclRiseSdaLevels[8U]);
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 9U, 0xA5U));
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.recorder.sclRiseSdaLevels[17U]);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_LOW ==
                fixture.recorder.sclRiseSdaLevels[18U]);
    TEST_ASSERT(19U == fixture.recorder.sclRiseCount);
    TEST_ASSERT(1U == fixture.recorder.startConditionCount);
    TEST_ASSERT(1U == fixture.recorder.stopConditionCount);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sclOutputLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sdaOutputLevel);

    return 0;
}

static int test_address_nack_is_detected_on_ninth_clock(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_HIGH
    };
    test_fixture_t fixture;
    uint8_t data = 0x00U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.recorder.sclRiseCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_NOT_FOUND ==
                platform_i2c_write(&fixture.i2c, 0x38U, &data, 1U));
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 0U, 0x70U));
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.recorder.sclRiseSdaLevels[8U]);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_LOW ==
                fixture.recorder.sclRiseSdaLevels[9U]);

    return 0;
}

static int test_read_reconstructs_byte_and_sends_final_nack(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW
    };
    test_fixture_t fixture;
    uint8_t data = 0U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.recorder.sclRiseCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_read(&fixture.i2c, 0x38U, &data, 1U));
    TEST_ASSERT(0xA6U == data);
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 0U, 0x71U));
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.recorder.sclRiseSdaLevels[17U]);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_LOW ==
                fixture.recorder.sclRiseSdaLevels[18U]);
    TEST_ASSERT(19U == fixture.recorder.sclRiseCount);
    TEST_ASSERT(1U == fixture.sclContext.configureCallCount);
    TEST_ASSERT(1U == fixture.sdaContext.configureCallCount);

    return 0;
}

static int test_multi_byte_read_sends_intermediate_ack_and_final_nack(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH
    };
    test_fixture_t fixture;
    uint8_t data[2] = {0U, 0U};
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.recorder.sclRiseCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_read(&fixture.i2c, 0x38U, data, 2U));
    TEST_ASSERT(0x3CU == data[0]);
    TEST_ASSERT(0xA5U == data[1]);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_LOW ==
                fixture.recorder.sclRiseSdaLevels[17U]);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.recorder.sclRiseSdaLevels[26U]);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_LOW ==
                fixture.recorder.sclRiseSdaLevels[27U]);
    TEST_ASSERT(28U == fixture.recorder.sclRiseCount);

    return 0;
}

static int test_write_read_uses_repeated_start_and_both_address_bits(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW
    };
    test_fixture_t fixture;
    uint8_t txData = 0xF3U;
    uint8_t rxData = 0U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.recorder.sclRiseCount = 0U;
    fixture.recorder.startConditionCount = 0U;
    fixture.recorder.stopConditionCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_write_read(&fixture.i2c,
                                        0x38U,
                                        &txData,
                                        1U,
                                        &rxData,
                                        1U));
    TEST_ASSERT(0x5AU == rxData);
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 0U, 0x70U));
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 9U, 0xF3U));
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.recorder.sclRiseSdaLevels[18U]);
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 19U, 0x71U));
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.recorder.sclRiseSdaLevels[36U]);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_LOW ==
                fixture.recorder.sclRiseSdaLevels[37U]);
    TEST_ASSERT(38U == fixture.recorder.sclRiseCount);
    TEST_ASSERT(2U == fixture.recorder.startConditionCount);
    TEST_ASSERT(1U == fixture.recorder.stopConditionCount);

    return 0;
}

static int test_data_nack_maps_to_io_and_stops_transaction(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_HIGH
    };
    test_fixture_t fixture;
    uint8_t data = 0x24U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.recorder.startConditionCount = 0U;
    fixture.recorder.stopConditionCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_IO ==
                platform_i2c_write(&fixture.i2c, 0x38U, &data, 1U));
    TEST_ASSERT(1U == fixture.recorder.startConditionCount);
    TEST_ASSERT(1U == fixture.recorder.stopConditionCount);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sclOutputLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sdaOutputLevel);

    return 0;
}

static int test_multi_byte_write_clocks_every_data_byte(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW
    };
    test_fixture_t fixture;
    const uint8_t data[] = {0x12U, 0x80U};
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.recorder.sclRiseCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_write(&fixture.i2c,
                                   0x38U,
                                   data,
                                   sizeof(data) / sizeof(data[0])));
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 9U, 0x12U));
    TEST_ASSERT(0 == assert_clocked_byte(&fixture.recorder, 18U, 0x80U));
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH ==
                fixture.recorder.sclRiseSdaLevels[26U]);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_LOW ==
                fixture.recorder.sclRiseSdaLevels[27U]);
    TEST_ASSERT(28U == fixture.recorder.sclRiseCount);

    return 0;
}

static int test_transaction_scl_timeout_does_not_attempt_recovery(void)
{
    test_fixture_t fixture;
    uint8_t data = 0x55U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    fixture.sclContext.defaultReadLevel = PLATFORM_GPIO_LEVEL_LOW;
    fixture.recorder.sclRiseCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_TIMEOUT ==
                platform_i2c_write(&fixture.i2c, 0x38U, &data, 1U));
    TEST_ASSERT(0U == fixture.recorder.sclRiseCount);
    TEST_ASSERT(0U == count_events(&fixture.recorder,
                                  TEST_EVENT_WRITE,
                                  TEST_LINE_SCL,
                                  PLATFORM_GPIO_LEVEL_LOW));

    return 0;
}

static int test_gpio_failure_is_preserved_after_best_effort_stop(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH
    };
    test_fixture_t fixture;
    uint8_t data = 0x55U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.sdaContext.failWriteCall = 4U;
    fixture.sdaContext.writeError = PLATFORM_ERR_CHECKSUM;
    fixture.sclContext.failWriteCall = 6U;
    fixture.sclContext.writeError = PLATFORM_ERR_IO;

    TEST_ASSERT(PLATFORM_ERR_CHECKSUM ==
                platform_i2c_write(&fixture.i2c, 0x38U, &data, 1U));
    TEST_ASSERT(fixture.sclContext.writeCallCount > 6U);
    TEST_ASSERT(fixture.sclContext.readCallCount > 3U);
    TEST_ASSERT(fixture.sdaContext.writeCallCount > 5U);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sclOutputLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sdaOutputLevel);

    return 0;
}

static int test_initial_start_failure_preserves_error_and_releases_bus(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH
    };
    test_fixture_t fixture;
    uint8_t data = 0x55U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.sclContext.failWriteCall = 4U;
    fixture.sclContext.writeError = PLATFORM_ERR_CHECKSUM;
    fixture.recorder.stopConditionCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_CHECKSUM ==
                platform_i2c_write(&fixture.i2c, 0x38U, &data, 1U));
    TEST_ASSERT(fixture.sclContext.writeCallCount > 4U);
    TEST_ASSERT(1U == fixture.recorder.stopConditionCount);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sclOutputLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sdaOutputLevel);

    return 0;
}

static int test_stop_failure_is_returned_after_best_effort_bus_release(void)
{
    const platform_gpio_level_t sdaReads[] = {
        PLATFORM_GPIO_LEVEL_HIGH,
        PLATFORM_GPIO_LEVEL_LOW,
        PLATFORM_GPIO_LEVEL_LOW
    };
    test_fixture_t fixture;
    uint8_t data = 0x55U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    script_read_values(&fixture.sdaContext,
                       sdaReads,
                       sizeof(sdaReads) / sizeof(sdaReads[0]));
    fixture.sdaContext.failWriteCall = 23U;
    fixture.sdaContext.writeError = PLATFORM_ERR_CHECKSUM;

    TEST_ASSERT(PLATFORM_ERR_CHECKSUM ==
                platform_i2c_write(&fixture.i2c, 0x38U, &data, 1U));
    TEST_ASSERT(fixture.sdaContext.writeCallCount > 23U);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sclOutputLevel);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == fixture.recorder.sdaOutputLevel);

    return 0;
}

static int test_init_recovery_failure_returns_busy_after_nine_clocks(void)
{
    test_fixture_t fixture;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    fixture.sdaContext.defaultReadLevel = PLATFORM_GPIO_LEVEL_LOW;

    TEST_ASSERT(PLATFORM_ERR_BUSY ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    TEST_ASSERT(9U == count_events(&fixture.recorder,
                                  TEST_EVENT_WRITE,
                                  TEST_LINE_SCL,
                                  PLATFORM_GPIO_LEVEL_LOW));
    TEST_ASSERT(0U == fixture.i2c.initialized);

    return 0;
}

static int test_deinit_releases_lines_and_deconfigures_sda_then_scl(void)
{
    const test_event_t *event = NULL;
    test_fixture_t fixture;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    fixture.recorder.eventCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_OK == platform_i2c_deinit(&fixture.i2c));
    TEST_ASSERT(5U == fixture.recorder.eventCount);
    event = &fixture.recorder.events[0U];
    TEST_ASSERT((event->type == TEST_EVENT_WRITE) &&
                (event->line == TEST_LINE_SCL) &&
                (event->value == PLATFORM_GPIO_LEVEL_HIGH));
    event = &fixture.recorder.events[1U];
    TEST_ASSERT((event->type == TEST_EVENT_READ) &&
                (event->line == TEST_LINE_SCL) &&
                (event->value == PLATFORM_GPIO_LEVEL_HIGH));
    event = &fixture.recorder.events[2U];
    TEST_ASSERT((event->type == TEST_EVENT_WRITE) &&
                (event->line == TEST_LINE_SDA) &&
                (event->value == PLATFORM_GPIO_LEVEL_HIGH));
    event = &fixture.recorder.events[3U];
    TEST_ASSERT((event->type == TEST_EVENT_DEINIT) &&
                (event->line == TEST_LINE_SDA));
    event = &fixture.recorder.events[4U];
    TEST_ASSERT((event->type == TEST_EVENT_DEINIT) &&
                (event->line == TEST_LINE_SCL));
    TEST_ASSERT(0U == fixture.i2c.initialized);
    TEST_ASSERT(0U == fixture.scl.configured);
    TEST_ASSERT(0U == fixture.sda.configured);

    return 0;
}

static int test_deinit_waits_for_scl_and_continues_cleanup_after_timeout(void)
{
    test_fixture_t fixture;
    uint32_t sdaWriteCountBeforeDeinit = 0U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    fixture.sclContext.defaultReadLevel = PLATFORM_GPIO_LEVEL_LOW;
    sdaWriteCountBeforeDeinit = fixture.sdaContext.writeCallCount;
    fixture.recorder.eventCount = 0U;

    TEST_ASSERT(PLATFORM_ERR_TIMEOUT == platform_i2c_deinit(&fixture.i2c));
    TEST_ASSERT(PLATFORM_I2C_DEFAULT_SCL_TIMEOUT_US ==
                count_events(&fixture.recorder,
                             TEST_EVENT_DELAY,
                             TEST_LINE_NONE,
                             1U));
    TEST_ASSERT(fixture.sdaContext.writeCallCount > sdaWriteCountBeforeDeinit);
    TEST_ASSERT(1U == fixture.sdaContext.deinitCallCount);
    TEST_ASSERT(1U == fixture.sclContext.deinitCallCount);
    TEST_ASSERT(0U == fixture.i2c.initialized);

    return 0;
}

static int test_contract_validation_remains_stable(void)
{
    test_fixture_t fixture;
    uint8_t txData = 0U;
    uint8_t rxData = 0U;
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    TEST_ASSERT(PLATFORM_ERR_ALREADY_INITIALIZED ==
                platform_i2c_init(&fixture.i2c,
                                  "test_i2c",
                                  &fixture.scl,
                                  &fixture.sda));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_write(&fixture.i2c, 0x80U, &txData, 1U));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_write(&fixture.i2c, 0x38U, NULL, 1U));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_write(&fixture.i2c, 0x38U, &txData, 0U));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_read(&fixture.i2c, 0x38U, NULL, 1U));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_read(&fixture.i2c, 0x38U, &rxData, 0U));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_write_read(&fixture.i2c,
                                        0x38U,
                                        &txData,
                                        0U,
                                        &rxData,
                                        1U));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_write_read(&fixture.i2c,
                                        0x38U,
                                        &txData,
                                        1U,
                                        &rxData,
                                        0U));

    return 0;
}

static int test_operations_reject_uninitialized_object(void)
{
    platform_i2c_t i2c = PLATFORM_I2C_INITIALIZER;
    uint8_t data = 0U;

    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED ==
                platform_i2c_write(&i2c, 0x38U, &data, 1U));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED ==
                platform_i2c_read(&i2c, 0x38U, &data, 1U));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED ==
                platform_i2c_write_read(&i2c,
                                        0x38U,
                                        &data,
                                        1U,
                                        &data,
                                        1U));
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED == platform_i2c_deinit(&i2c));

    return 0;
}

static int test_instance_timing_config_is_stored_per_bus(void)
{
    test_fixture_t fixture;
    const platform_i2c_config_t config = {
        7U,
        23U
    };
    int result = initialize_fixture(&fixture);

    if (result != 0) {
        return result;
    }

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_init_with_config(&fixture.i2c,
                                              "custom_i2c",
                                              &fixture.scl,
                                              &fixture.sda,
                                              &config));
    TEST_ASSERT(7U == fixture.i2c.halfPeriodUs);
    TEST_ASSERT(23U == fixture.i2c.sclTimeoutUs);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_i2c_deinit(&fixture.i2c));

    return 0;
}

int main(void)
{
    int result = test_instance_timing_config_is_stored_per_bus();

    if (result != 0) {
        return result;
    }
    result = test_init_rejects_null_required_objects();

    if (result != 0) {
        return result;
    }
    result = test_init_configures_open_drain_output_and_releases_bus();
    if (result != 0) {
        return result;
    }
    result = test_init_recovers_sda_stuck_low_and_generates_stop();
    if (result != 0) {
        return result;
    }
    result = test_init_fails_when_scl_cannot_become_high();
    if (result != 0) {
        return result;
    }
    result = test_transaction_rejects_non_idle_bus_without_recovery();
    if (result != 0) {
        return result;
    }
    result = test_write_generates_start_msb_bytes_ack_clocks_and_stop();
    if (result != 0) {
        return result;
    }
    result = test_address_nack_is_detected_on_ninth_clock();
    if (result != 0) {
        return result;
    }
    result = test_read_reconstructs_byte_and_sends_final_nack();
    if (result != 0) {
        return result;
    }
    result = test_multi_byte_read_sends_intermediate_ack_and_final_nack();
    if (result != 0) {
        return result;
    }
    result = test_write_read_uses_repeated_start_and_both_address_bits();
    if (result != 0) {
        return result;
    }
    result = test_data_nack_maps_to_io_and_stops_transaction();
    if (result != 0) {
        return result;
    }
    result = test_multi_byte_write_clocks_every_data_byte();
    if (result != 0) {
        return result;
    }
    result = test_transaction_scl_timeout_does_not_attempt_recovery();
    if (result != 0) {
        return result;
    }
    result = test_deinit_waits_for_scl_and_continues_cleanup_after_timeout();
    if (result != 0) {
        return result;
    }
    result = test_gpio_failure_is_preserved_after_best_effort_stop();
    if (result != 0) {
        return result;
    }
    result = test_initial_start_failure_preserves_error_and_releases_bus();
    if (result != 0) {
        return result;
    }
    result = test_stop_failure_is_returned_after_best_effort_bus_release();
    if (result != 0) {
        return result;
    }
    result = test_init_recovery_failure_returns_busy_after_nine_clocks();
    if (result != 0) {
        return result;
    }
    result = test_deinit_releases_lines_and_deconfigures_sda_then_scl();
    if (result != 0) {
        return result;
    }
    result = test_contract_validation_remains_stable();
    if (result != 0) {
        return result;
    }

    return test_operations_reject_uninitialized_object();
}
//******************************** Functions *********************************//
