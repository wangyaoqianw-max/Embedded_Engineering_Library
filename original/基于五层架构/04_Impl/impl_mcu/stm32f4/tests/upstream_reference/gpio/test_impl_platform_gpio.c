/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_impl_platform_gpio.c
 * @brief 验证 STM32 GPIO Impl Context 绑定和构造行为
 * @author YaoQian Wang
 * @date 2026-09-01
 * @version V1.0
 *
 *****************************************************************************/

#include <stddef.h>

#include "impl_platform_gpio.h"

#define TEST_ASSERT(condition)       \
    do {                             \
        if (!(condition)) {          \
            return __LINE__;         \
        }                            \
    } while (0)

static GPIO_TypeDef g_fakePort;
static uint32_t g_fakeHalInitCount;
static uint32_t g_fakeHalWriteCount;
static uint32_t g_fakeHalReadCount;
static uint32_t g_fakeHalDeinitCount;

enum
{
    FAKE_HAL_CALL_WRITE = 1U,
    FAKE_HAL_CALL_INIT
};

static uint32_t g_fakeHalCallSequence[4];
static GPIO_TypeDef *g_fakeHalLastPort;
static uint16_t g_fakeHalLastPin;
static GPIO_PinState g_fakeHalLastState;
static GPIO_PinState g_fakeHalReadState;
static GPIO_InitTypeDef g_fakeHalLastInit;

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
    g_fakeHalInitCount++;
    g_fakeHalCallSequence[g_fakeHalInitCount + g_fakeHalWriteCount - 1U] =
        FAKE_HAL_CALL_INIT;
    g_fakeHalLastPort = GPIOx;
    g_fakeHalLastPin = (uint16_t)GPIO_Init->Pin;
    g_fakeHalLastInit = *GPIO_Init;
}

void HAL_GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)
{
    g_fakeHalLastPort = GPIOx;
    g_fakeHalLastPin = (uint16_t)GPIO_Pin;
    g_fakeHalDeinitCount++;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void)GPIOx;
    (void)GPIO_Pin;
    g_fakeHalReadCount++;

    return g_fakeHalReadState;
}

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx,
                       uint16_t GPIO_Pin,
                       GPIO_PinState PinState)
{
    g_fakeHalWriteCount++;
    g_fakeHalCallSequence[g_fakeHalInitCount + g_fakeHalWriteCount - 1U] =
        FAKE_HAL_CALL_WRITE;
    g_fakeHalLastPort = GPIOx;
    g_fakeHalLastPin = GPIO_Pin;
    g_fakeHalLastState = PinState;
}

#include "../../04_Impl/impl_mcu/impl_platform_gpio.c"

static void reset_fake_hal(void)
{
    g_fakeHalInitCount = 0U;
    g_fakeHalWriteCount = 0U;
    g_fakeHalReadCount = 0U;
    g_fakeHalDeinitCount = 0U;
    g_fakeHalCallSequence[0] = 0U;
    g_fakeHalCallSequence[1] = 0U;
    g_fakeHalCallSequence[2] = 0U;
    g_fakeHalCallSequence[3] = 0U;
    g_fakeHalLastPort = NULL;
    g_fakeHalLastPin = 0U;
    g_fakeHalLastState = GPIO_PIN_RESET;
    g_fakeHalReadState = GPIO_PIN_RESET;
    g_fakeHalLastInit = (GPIO_InitTypeDef){0};
}

static int test_construct_rejects_invalid_contexts(void)
{
    platform_gpio_t gpio = PLATFORM_GPIO_INITIALIZER;
    impl_platform_gpio_context_t context = {
        &g_fakePort,
        GPIO_PIN_3
    };

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                impl_platform_gpio_construct(NULL, "test_gpio", &context));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                impl_platform_gpio_construct(&gpio, "test_gpio", NULL));

    context.port = NULL;
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));

    context.port = &g_fakePort;
    context.pin = 0U;
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));

    context.pin = (uint16_t)(GPIO_PIN_0 | GPIO_PIN_1);
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));
    TEST_ASSERT(0U == gpio.initialized);

    return 0;
}

static int test_construct_binds_valid_single_pin_without_hal_operation(void)
{
    platform_gpio_t gpio = PLATFORM_GPIO_INITIALIZER;
    impl_platform_gpio_context_t context = {
        &g_fakePort,
        GPIO_PIN_3
    };

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));
    TEST_ASSERT(1U == gpio.initialized);
    TEST_ASSERT(0U == gpio.configured);
    TEST_ASSERT(&context == gpio.implContext);
    TEST_ASSERT(0U == g_fakeHalInitCount);
    TEST_ASSERT(0U == g_fakeHalWriteCount);
    TEST_ASSERT(0U == g_fakeHalReadCount);
    TEST_ASSERT(0U == g_fakeHalDeinitCount);

    return 0;
}

static int test_construct_accepts_highest_single_pin_and_rejects_masks(void)
{
    platform_gpio_t gpio = PLATFORM_GPIO_INITIALIZER;
    impl_platform_gpio_context_t context = {
        &g_fakePort,
        GPIO_PIN_15
    };

    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));

    gpio = (platform_gpio_t)PLATFORM_GPIO_INITIALIZER;
    context.pin = GPIO_PIN_All;
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));

    return 0;
}

static int test_configure_maps_input_pull_modes(void)
{
    platform_gpio_t gpio = PLATFORM_GPIO_INITIALIZER;
    impl_platform_gpio_context_t context = {
        &g_fakePort,
        GPIO_PIN_3
    };
    platform_gpio_config_t config = {
        PLATFORM_GPIO_DIRECTION_INPUT,
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_OUTPUT_PUSH_PULL,
        PLATFORM_GPIO_LEVEL_HIGH
    };
    platform_gpio_pull_t pulls[] = {
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_PULL_UP,
        PLATFORM_GPIO_PULL_DOWN
    };
    uint32_t expectedPulls[] = {
        GPIO_NOPULL,
        GPIO_PULLUP,
        GPIO_PULLDOWN
    };
    uint32_t index;

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));

    for (index = 0U; index < 3U; index++) {
        config.pull = pulls[index];
        TEST_ASSERT(PLATFORM_ERR_OK ==
                    platform_gpio_configure(&gpio, &config));
        TEST_ASSERT(GPIO_MODE_INPUT == g_fakeHalLastInit.Mode);
        TEST_ASSERT(expectedPulls[index] == g_fakeHalLastInit.Pull);
        TEST_ASSERT(GPIO_SPEED_FREQ_LOW == g_fakeHalLastInit.Speed);
        TEST_ASSERT(0U == g_fakeHalLastInit.Alternate);
        TEST_ASSERT((uint32_t)context.pin == g_fakeHalLastInit.Pin);
        TEST_ASSERT(0U == g_fakeHalWriteCount);
    }

    return 0;
}

static int test_configure_maps_outputs_and_writes_initial_level_first(void)
{
    platform_gpio_t gpio = PLATFORM_GPIO_INITIALIZER;
    impl_platform_gpio_context_t context = {
        &g_fakePort,
        GPIO_PIN_3
    };
    platform_gpio_config_t config = {
        PLATFORM_GPIO_DIRECTION_OUTPUT,
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_OUTPUT_PUSH_PULL,
        PLATFORM_GPIO_LEVEL_LOW
    };

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_configure(&gpio, &config));
    TEST_ASSERT(1U == g_fakeHalWriteCount);
    TEST_ASSERT(1U == g_fakeHalInitCount);
    TEST_ASSERT(FAKE_HAL_CALL_WRITE == g_fakeHalCallSequence[0]);
    TEST_ASSERT(FAKE_HAL_CALL_INIT == g_fakeHalCallSequence[1]);
    TEST_ASSERT(&g_fakePort == g_fakeHalLastPort);
    TEST_ASSERT(context.pin == g_fakeHalLastPin);
    TEST_ASSERT(GPIO_PIN_RESET == g_fakeHalLastState);
    TEST_ASSERT(GPIO_MODE_OUTPUT_PP == g_fakeHalLastInit.Mode);
    TEST_ASSERT(GPIO_NOPULL == g_fakeHalLastInit.Pull);
    TEST_ASSERT(GPIO_SPEED_FREQ_LOW == g_fakeHalLastInit.Speed);
    TEST_ASSERT(0U == g_fakeHalLastInit.Alternate);

    reset_fake_hal();
    config.outputType = PLATFORM_GPIO_OUTPUT_OPEN_DRAIN;
    config.initialLevel = PLATFORM_GPIO_LEVEL_HIGH;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_configure(&gpio, &config));
    TEST_ASSERT(FAKE_HAL_CALL_WRITE == g_fakeHalCallSequence[0]);
    TEST_ASSERT(FAKE_HAL_CALL_INIT == g_fakeHalCallSequence[1]);
    TEST_ASSERT(GPIO_PIN_SET == g_fakeHalLastState);
    TEST_ASSERT(GPIO_MODE_OUTPUT_OD == g_fakeHalLastInit.Mode);

    return 0;
}

static int test_write_maps_logical_levels_and_forwards_context(void)
{
    platform_gpio_t gpio = PLATFORM_GPIO_INITIALIZER;
    impl_platform_gpio_context_t context = {
        &g_fakePort,
        GPIO_PIN_3
    };
    platform_gpio_config_t config = {
        PLATFORM_GPIO_DIRECTION_OUTPUT,
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_OUTPUT_PUSH_PULL,
        PLATFORM_GPIO_LEVEL_LOW
    };

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_configure(&gpio, &config));

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_gpio_write(&gpio, PLATFORM_GPIO_LEVEL_LOW));
    TEST_ASSERT(1U == g_fakeHalWriteCount);
    TEST_ASSERT(&g_fakePort == g_fakeHalLastPort);
    TEST_ASSERT(context.pin == g_fakeHalLastPin);
    TEST_ASSERT(GPIO_PIN_RESET == g_fakeHalLastState);

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_gpio_write(&gpio, PLATFORM_GPIO_LEVEL_HIGH));
    TEST_ASSERT(1U == g_fakeHalWriteCount);
    TEST_ASSERT(&g_fakePort == g_fakeHalLastPort);
    TEST_ASSERT(context.pin == g_fakeHalLastPin);
    TEST_ASSERT(GPIO_PIN_SET == g_fakeHalLastState);

    return 0;
}

static int test_read_maps_hal_levels_and_forwards_context(void)
{
    platform_gpio_t gpio = PLATFORM_GPIO_INITIALIZER;
    impl_platform_gpio_context_t context = {
        &g_fakePort,
        GPIO_PIN_3
    };
    platform_gpio_config_t config = {
        PLATFORM_GPIO_DIRECTION_INPUT,
        PLATFORM_GPIO_PULL_UP,
        PLATFORM_GPIO_OUTPUT_PUSH_PULL,
        PLATFORM_GPIO_LEVEL_LOW
    };
    platform_gpio_level_t level = PLATFORM_GPIO_LEVEL_HIGH;

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_configure(&gpio, &config));

    g_fakeHalReadState = GPIO_PIN_RESET;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_read(&gpio, &level));
    TEST_ASSERT(1U == g_fakeHalReadCount);
    TEST_ASSERT(&g_fakePort == g_fakeHalLastPort);
    TEST_ASSERT(context.pin == g_fakeHalLastPin);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_LOW == level);

    g_fakeHalReadState = GPIO_PIN_SET;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_read(&gpio, &level));
    TEST_ASSERT(2U == g_fakeHalReadCount);
    TEST_ASSERT(PLATFORM_GPIO_LEVEL_HIGH == level);

    return 0;
}

static int test_deinit_clears_platform_state_and_allows_reconfigure(void)
{
    platform_gpio_t gpio = PLATFORM_GPIO_INITIALIZER;
    impl_platform_gpio_context_t context = {
        &g_fakePort,
        GPIO_PIN_3
    };
    platform_gpio_config_t config = {
        PLATFORM_GPIO_DIRECTION_OUTPUT,
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_OUTPUT_OPEN_DRAIN,
        PLATFORM_GPIO_LEVEL_HIGH
    };

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_gpio_construct(&gpio, "test_gpio", &context));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_configure(&gpio, &config));

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_deinit(&gpio));
    TEST_ASSERT(1U == g_fakeHalDeinitCount);
    TEST_ASSERT(&g_fakePort == g_fakeHalLastPort);
    TEST_ASSERT(context.pin == g_fakeHalLastPin);
    TEST_ASSERT(1U == gpio.initialized);
    TEST_ASSERT(0U == gpio.configured);

    TEST_ASSERT(PLATFORM_ERR_OK == platform_gpio_configure(&gpio, &config));
    TEST_ASSERT(1U == gpio.configured);

    return 0;
}

int main(void)
{
    int result = test_construct_rejects_invalid_contexts();

    if (0 != result) {
        return result;
    }

    result = test_construct_binds_valid_single_pin_without_hal_operation();
    if (0 != result) {
        return result;
    }

    result = test_construct_accepts_highest_single_pin_and_rejects_masks();
    if (0 != result) {
        return result;
    }

    result = test_configure_maps_input_pull_modes();
    if (0 != result) {
        return result;
    }

    result = test_configure_maps_outputs_and_writes_initial_level_first();
    if (0 != result) {
        return result;
    }

    result = test_write_maps_logical_levels_and_forwards_context();
    if (0 != result) {
        return result;
    }

    result = test_read_maps_hal_levels_and_forwards_context();
    if (0 != result) {
        return result;
    }

    return test_deinit_clears_platform_state_and_allows_reconfigure();
}
