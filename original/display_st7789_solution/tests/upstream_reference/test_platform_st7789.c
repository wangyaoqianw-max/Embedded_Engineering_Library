/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_platform_st7789.c
 * @brief 验证 ST7789 Platform Driver 与 BSP 静态配置合同
 * @author YaoQian Wang
 * @date 2026-09-06
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_bsp_st7789.h"
#include "platform_device.h"
#include "platform_time.h"

#include <stddef.h>
#include <string.h>
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define TEST_ASSERT(condition)       \
    do {                             \
        if (!(condition)) {          \
            return __LINE__;         \
        }                            \
    } while (0)

#define FAKE_EVENT_CAPACITY          (1024U)
#define FAKE_CAPTURE_BYTES           (16U)
//******************************** Defines *********************************//

//******************************** Types ***********************************//
typedef enum
{
    FAKE_GPIO_CS = 0,
    FAKE_GPIO_DC,
    FAKE_GPIO_RESET,
    FAKE_GPIO_BACKLIGHT,
    FAKE_GPIO_COUNT
} fake_gpio_role_t;

typedef enum
{
    FAKE_EVENT_GPIO_WRITE = 0,
    FAKE_EVENT_SPI_WRITE,
    FAKE_EVENT_DELAY
} fake_event_type_t;

typedef struct
{
    fake_event_type_t type;
    uint32_t id;
    uint32_t value;
    platform_size_t length;
    uint8_t data[FAKE_CAPTURE_BYTES];
    platform_gpio_level_t dcLevel;
    platform_bool_t transactionActive;
} fake_event_t;

typedef struct
{
    fake_gpio_role_t role;
    platform_error_t configureResult;
    platform_error_t writeResult;
    platform_error_t deinitResult;
    platform_gpio_config_t configuredValue;
    platform_gpio_level_t currentLevel;
    uint32_t configureCount;
    uint32_t writeCount;
    uint32_t deinitCount;
    uint32_t failWriteCall;
    platform_error_t failWriteResult;
} fake_gpio_context_t;

typedef struct
{
    platform_error_t applyResult;
    platform_error_t writeResult;
    uint32_t applyCount;
    uint32_t writeCount;
    uint32_t failWriteCall;
    platform_error_t failWriteResult;
} fake_spi_context_t;

typedef struct
{
    uint8_t command;
    uint8_t data[14];
    platform_size_t dataLength;
} expected_command_t;
//******************************** Types ***********************************//

//******************************** Variables ********************************//
static fake_gpio_context_t g_gpioContexts[FAKE_GPIO_COUNT];
static fake_spi_context_t g_spiContext;
static fake_event_t g_events[FAKE_EVENT_CAPACITY];
static platform_size_t g_eventCount;
static platform_spi_bus_t *g_observedBus;
static uint32_t g_delayCount;
static uint32_t g_failDelayCall;
static platform_error_t g_failDelayResult;
static uint32_t g_gpioConstructCount;
//******************************** Variables ********************************//

//******************************** Private Functions *************************//
static void fake_record_event(const fake_event_t *event)
{
    if (g_eventCount < FAKE_EVENT_CAPACITY) {
        g_events[g_eventCount] = *event;
        g_eventCount++;
    }
}

static void fake_reset_all(void)
{
    uint32_t index;

    (void)memset(g_gpioContexts, 0, sizeof(g_gpioContexts));
    (void)memset(&g_spiContext, 0, sizeof(g_spiContext));
    (void)memset(g_events, 0, sizeof(g_events));
    g_eventCount = 0U;
    g_observedBus = NULL;
    g_delayCount = 0U;
    g_failDelayCall = 0U;
    g_failDelayResult = PLATFORM_ERR_OK;
    g_gpioConstructCount = 0U;
    g_spiContext.applyResult = PLATFORM_ERR_OK;
    g_spiContext.writeResult = PLATFORM_ERR_OK;

    for (index = 0U; index < FAKE_GPIO_COUNT; index++) {
        g_gpioContexts[index].role = (fake_gpio_role_t)index;
        g_gpioContexts[index].configureResult = PLATFORM_ERR_OK;
        g_gpioContexts[index].writeResult = PLATFORM_ERR_OK;
        g_gpioContexts[index].deinitResult = PLATFORM_ERR_OK;
        g_gpioContexts[index].failWriteResult = PLATFORM_ERR_OK;
    }
}

static void fake_clear_events(void)
{
    (void)memset(g_events, 0, sizeof(g_events));
    g_eventCount = 0U;
}

static const fake_event_t *fake_spi_event_at(platform_size_t spiEventIndex)
{
    platform_size_t eventIndex;
    platform_size_t currentSpiEvent = 0U;

    for (eventIndex = 0U; eventIndex < g_eventCount; eventIndex++) {
        if (g_events[eventIndex].type == FAKE_EVENT_SPI_WRITE) {
            if (currentSpiEvent == spiEventIndex) {
                return &g_events[eventIndex];
            }
            currentSpiEvent++;
        }
    }
    return NULL;
}

static platform_error_t fake_gpio_configure(
    platform_gpio_t *gpio,
    const platform_gpio_config_t *config)
{
    fake_gpio_context_t *context =
        (fake_gpio_context_t *)gpio->implContext;

    context->configureCount++;
    context->configuredValue = *config;
    if (context->configureResult == PLATFORM_ERR_OK) {
        context->currentLevel = config->initialLevel;
    }
    return context->configureResult;
}

static platform_error_t fake_gpio_write(
    platform_gpio_t *gpio,
    platform_gpio_level_t level)
{
    fake_gpio_context_t *context =
        (fake_gpio_context_t *)gpio->implContext;
    fake_event_t event = {0};
    platform_error_t result = context->writeResult;

    context->writeCount++;
    event.type = FAKE_EVENT_GPIO_WRITE;
    event.id = (uint32_t)context->role;
    event.value = (uint32_t)level;
    event.transactionActive =
        ((g_observedBus != NULL) &&
         (g_observedBus->activeDevice != NULL)) ? PLATFORM_TRUE : PLATFORM_FALSE;
    fake_record_event(&event);

    if ((context->failWriteCall != 0U) &&
        (context->writeCount == context->failWriteCall)) {
        result = context->failWriteResult;
    }
    if (result == PLATFORM_ERR_OK) {
        context->currentLevel = level;
    }
    return result;
}

static platform_error_t fake_gpio_read(
    platform_gpio_t *gpio,
    platform_gpio_level_t *level)
{
    fake_gpio_context_t *context =
        (fake_gpio_context_t *)gpio->implContext;

    *level = context->currentLevel;
    return PLATFORM_ERR_OK;
}

static platform_error_t fake_gpio_deinit(platform_gpio_t *gpio)
{
    fake_gpio_context_t *context =
        (fake_gpio_context_t *)gpio->implContext;

    context->deinitCount++;
    return context->deinitResult;
}

static const platform_gpio_ops_t g_fakeGpioOps = {
    fake_gpio_configure,
    fake_gpio_write,
    fake_gpio_read,
    fake_gpio_deinit
};

static platform_error_t fake_construct_gpio(
    platform_gpio_t *gpio,
    fake_gpio_role_t role,
    const char *name)
{
    platform_gpio_init_params_t params = {
        name,
        &g_fakeGpioOps,
        &g_gpioContexts[role]
    };

    g_gpioConstructCount++;
    return platform_gpio_init(gpio, &params);
}

static platform_error_t fake_lifecycle(void *self)
{
    (void)self;
    return PLATFORM_ERR_OK;
}

static platform_error_t fake_spi_apply_config(
    platform_spi_bus_t *bus,
    const platform_spi_device_config_t *config)
{
    (void)bus;
    (void)config;
    g_spiContext.applyCount++;
    return g_spiContext.applyResult;
}

static platform_error_t fake_spi_write(
    platform_spi_bus_t *bus,
    const uint8_t *data,
    platform_size_t dataLength)
{
    fake_event_t event = {0};
    platform_size_t index;
    platform_size_t captureLength = dataLength;
    platform_error_t result = g_spiContext.writeResult;

    (void)bus;
    g_spiContext.writeCount++;
    event.type = FAKE_EVENT_SPI_WRITE;
    event.id = g_spiContext.writeCount;
    event.length = dataLength;
    event.dcLevel = g_gpioContexts[FAKE_GPIO_DC].currentLevel;
    event.transactionActive =
        ((g_observedBus != NULL) &&
         (g_observedBus->activeDevice != NULL)) ? PLATFORM_TRUE : PLATFORM_FALSE;
    if (captureLength > FAKE_CAPTURE_BYTES) {
        captureLength = FAKE_CAPTURE_BYTES;
    }
    for (index = 0U; index < captureLength; index++) {
        event.data[index] = data[index];
    }
    fake_record_event(&event);

    if ((g_spiContext.failWriteCall != 0U) &&
        (g_spiContext.writeCount == g_spiContext.failWriteCall)) {
        result = g_spiContext.failWriteResult;
    }
    return result;
}

static const platform_lifecycle_ops_t g_fakeLifecycleOps = {
    fake_lifecycle,
    fake_lifecycle,
    fake_lifecycle,
    fake_lifecycle,
    fake_lifecycle
};

static const platform_spi_bus_ops_t g_fakeSpiOps = {
    fake_spi_apply_config,
    fake_spi_write
};

static platform_error_t prepare_constructed_display(
    platform_st7789_t *display)
{
    fake_reset_all();
    return platform_bsp_st7789_construct_display(display);
}

static platform_error_t prepare_started_bus(platform_spi_bus_t *bus)
{
    platform_spi_bus_init_params_t params = {
        "st7789-test-bus",
        PLATFORM_DEVICE_CAP_NONE,
        &g_fakeLifecycleOps,
        &g_fakeSpiOps,
        &g_spiContext
    };
    platform_error_t result = platform_spi_bus_init(bus, &params);

    if (result == PLATFORM_ERR_OK) {
        bus->device.object.state = PLATFORM_OBJECT_STARTED;
        g_observedBus = bus;
    }
    return result;
}

static platform_error_t prepare_initialized_display(
    platform_st7789_t *display,
    platform_spi_bus_t *bus)
{
    platform_error_t result = prepare_constructed_display(display);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    result = prepare_started_bus(bus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    return platform_st7789_init(display, bus);
}

static int test_bsp_construct_binds_frozen_panel_config_without_io(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    uint32_t index;

    TEST_ASSERT(prepare_constructed_display(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_gpioConstructCount == 4U);
    TEST_ASSERT(display.width == 240U);
    TEST_ASSERT(display.height == 280U);
    TEST_ASSERT(display.xOffset == 0U);
    TEST_ASSERT(display.yOffset == 20U);
    TEST_ASSERT(display.madctl == 0x00U);
    TEST_ASSERT(display.spiConfig.mode == PLATFORM_SPI_MODE_3);
    TEST_ASSERT(display.spiConfig.bitOrder == PLATFORM_SPI_BIT_ORDER_MSB_FIRST);
    TEST_ASSERT(display.spiConfig.dataBits == 8U);
    TEST_ASSERT(display.spiConfig.maxClockHz == 12500000U);
    TEST_ASSERT(display.csActiveLevel == PLATFORM_GPIO_LEVEL_LOW);
    TEST_ASSERT(display.resetActiveLevel == PLATFORM_GPIO_LEVEL_LOW);
    TEST_ASSERT(display.backlightOnLevel == PLATFORM_GPIO_LEVEL_HIGH);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);
    TEST_ASSERT(display.spiDevice.initialized == PLATFORM_FALSE);
    TEST_ASSERT(g_eventCount == 0U);
    TEST_ASSERT(g_delayCount == 0U);
    TEST_ASSERT(g_spiContext.writeCount == 0U);
    for (index = 0U; index < FAKE_GPIO_COUNT; index++) {
        TEST_ASSERT(((platform_gpio_t *[]){&display.cs, &display.dc,
                                           &display.reset,
                                           &display.backlight})[index]->initialized == PLATFORM_TRUE);
        TEST_ASSERT(g_gpioContexts[index].configureCount == 0U);
        TEST_ASSERT(g_gpioContexts[index].writeCount == 0U);
        TEST_ASSERT(g_gpioContexts[index].deinitCount == 0U);
    }

    return 0;
}

static int test_bsp_construct_rejects_null(void)
{
    fake_reset_all();
    TEST_ASSERT(platform_bsp_st7789_construct_display(NULL) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(g_gpioConstructCount == 0U);
    return 0;
}

static int test_init_configures_safe_gpio_levels_and_resets_controller(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_size_t eventIndex;
    uint32_t resetWriteIndex = 0U;
    uint32_t resetLevels[2] = {0U, 0U};
    uint32_t delayValues[3] = {0U, 0U, 0U};
    uint32_t delayIndex = 0U;

    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    TEST_ASSERT(display.initialized == PLATFORM_TRUE);
    TEST_ASSERT(display.spiDevice.initialized == PLATFORM_TRUE);
    TEST_ASSERT(bus.activeDevice == NULL);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_CS].configuredValue.initialLevel == PLATFORM_GPIO_LEVEL_HIGH);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_DC].configuredValue.initialLevel == PLATFORM_GPIO_LEVEL_HIGH);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_RESET].configuredValue.initialLevel == PLATFORM_GPIO_LEVEL_HIGH);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_BACKLIGHT].configuredValue.initialLevel == PLATFORM_GPIO_LEVEL_LOW);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_BACKLIGHT].currentLevel == PLATFORM_GPIO_LEVEL_LOW);

    for (eventIndex = 0U; eventIndex < g_eventCount; eventIndex++) {
        if ((g_events[eventIndex].type == FAKE_EVENT_GPIO_WRITE) &&
            (g_events[eventIndex].id == (uint32_t)FAKE_GPIO_RESET) &&
            (resetWriteIndex < 2U)) {
            resetLevels[resetWriteIndex] = g_events[eventIndex].value;
            resetWriteIndex++;
        }
        if ((g_events[eventIndex].type == FAKE_EVENT_DELAY) &&
            (delayIndex < 3U)) {
            delayValues[delayIndex] = g_events[eventIndex].value;
            delayIndex++;
        }
    }
    TEST_ASSERT(resetWriteIndex == 2U);
    TEST_ASSERT(resetLevels[0] == (uint32_t)PLATFORM_GPIO_LEVEL_LOW);
    TEST_ASSERT(resetLevels[1] == (uint32_t)PLATFORM_GPIO_LEVEL_HIGH);
    TEST_ASSERT(delayIndex == 3U);
    TEST_ASSERT(delayValues[0] == 100U);
    TEST_ASSERT(delayValues[1] == 100U);
    TEST_ASSERT(delayValues[2] == 100U);

    return 0;
}

static int test_backlight_and_deinit_release_only_owned_resources(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    uint32_t index;

    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    TEST_ASSERT(platform_st7789_backlight_on(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_BACKLIGHT].currentLevel == PLATFORM_GPIO_LEVEL_HIGH);
    TEST_ASSERT(platform_st7789_backlight_off(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_BACKLIGHT].currentLevel == PLATFORM_GPIO_LEVEL_LOW);
    TEST_ASSERT(platform_st7789_deinit(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);
    TEST_ASSERT(display.spiDevice.initialized == PLATFORM_FALSE);
    TEST_ASSERT(bus.device.object.state == PLATFORM_OBJECT_STARTED);
    TEST_ASSERT(bus.activeDevice == NULL);
    for (index = 0U; index < FAKE_GPIO_COUNT; index++) {
        TEST_ASSERT(g_gpioContexts[index].deinitCount == 1U);
    }
    TEST_ASSERT(platform_st7789_backlight_on(&display) == PLATFORM_ERR_NOT_INITIALIZED);
    TEST_ASSERT(platform_st7789_deinit(&display) == PLATFORM_ERR_NOT_INITIALIZED);

    return 0;
}

static int test_init_validates_state_and_rolls_back_early_gpio_failure(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;

    TEST_ASSERT(platform_st7789_init(NULL, &bus) == PLATFORM_ERR_NULL_POINTER);
    TEST_ASSERT(platform_st7789_init(&display, NULL) == PLATFORM_ERR_NULL_POINTER);
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_NOT_INITIALIZED);
    TEST_ASSERT(platform_st7789_deinit(NULL) == PLATFORM_ERR_NULL_POINTER);
    TEST_ASSERT(platform_st7789_backlight_on(NULL) == PLATFORM_ERR_NULL_POINTER);
    TEST_ASSERT(platform_st7789_backlight_off(NULL) == PLATFORM_ERR_NULL_POINTER);

    TEST_ASSERT(prepare_constructed_display(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(platform_st7789_init(&display, &bus) ==
                PLATFORM_ERR_NOT_INITIALIZED);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_CS].configureCount == 0U);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_CS].writeCount == 0U);
    TEST_ASSERT(g_delayCount == 0U);

    TEST_ASSERT(prepare_started_bus(&bus) == PLATFORM_ERR_OK);
    bus.device.object.state = PLATFORM_OBJECT_INITIALIZED;
    TEST_ASSERT(platform_st7789_init(&display, &bus) ==
                PLATFORM_ERR_INVALID_STATE);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_CS].configureCount == 0U);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_CS].writeCount == 0U);
    TEST_ASSERT(g_delayCount == 0U);

    bus.device.object.state = PLATFORM_OBJECT_STARTED;
    g_gpioContexts[FAKE_GPIO_DC].configureResult = PLATFORM_ERR_IO;
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_IO);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);
    TEST_ASSERT(display.spiDevice.initialized == PLATFORM_FALSE);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_CS].deinitCount == 1U);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_DC].deinitCount == 0U);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_RESET].deinitCount == 0U);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_BACKLIGHT].deinitCount == 0U);

    fake_reset_all();
    display = (platform_st7789_t)PLATFORM_ST7789_INITIALIZER;
    bus = (platform_spi_bus_t)PLATFORM_SPI_BUS_INITIALIZER;
    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_ALREADY_INITIALIZED);

    return 0;
}

static int test_init_uses_verified_table_sequence_and_delay_contract(void)
{
    static const expected_command_t expected[] = {
        {0x11U, {0}, 0U},
        {0xB2U, {0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U}, 5U},
        {0x35U, {0x00U}, 1U},
        {0x36U, {0x00U}, 1U},
        {0x3AU, {0x05U}, 1U},
        {0xB7U, {0x35U}, 1U},
        {0xBBU, {0x2DU}, 1U},
        {0xC0U, {0x2CU}, 1U},
        {0xC2U, {0x01U}, 1U},
        {0xC3U, {0x15U}, 1U},
        {0xC4U, {0x20U}, 1U},
        {0xC6U, {0x0FU}, 1U},
        {0xD0U, {0xA4U, 0xA1U}, 2U},
        {0xD6U, {0xA1U}, 1U},
        {0xE0U, {0x70U, 0x05U, 0x0AU, 0x0BU, 0x0AU, 0x27U, 0x2FU,
                 0x44U, 0x47U, 0x37U, 0x14U, 0x14U, 0x29U, 0x2FU}, 14U},
        {0xE1U, {0x70U, 0x07U, 0x0CU, 0x08U, 0x08U, 0x04U, 0x2FU,
                 0x33U, 0x46U, 0x18U, 0x15U, 0x15U, 0x2BU, 0x2DU}, 14U},
        {0x21U, {0}, 0U},
        {0x29U, {0}, 0U}
    };
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_size_t eventIndex = 0U;
    platform_size_t expectedIndex;
    platform_size_t dataIndex;
    uint32_t commandCount = 0U;
    platform_bool_t sleepOutDelaySeen = PLATFORM_FALSE;

    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_delayCount == 4U);
    TEST_ASSERT(g_spiContext.applyCount == 18U);

    for (expectedIndex = 0U;
         expectedIndex < (sizeof(expected) / sizeof(expected[0]));
         expectedIndex++) {
        while ((eventIndex < g_eventCount) &&
               (g_events[eventIndex].type != FAKE_EVENT_SPI_WRITE)) {
            eventIndex++;
        }
        TEST_ASSERT(eventIndex < g_eventCount);
        TEST_ASSERT(g_events[eventIndex].dcLevel == PLATFORM_GPIO_LEVEL_LOW);
        TEST_ASSERT(g_events[eventIndex].length == 1U);
        TEST_ASSERT(expected[expectedIndex].command ==
                    g_events[eventIndex].data[0]);
        TEST_ASSERT(g_events[eventIndex].data[0] != 0x2CU);
        commandCount++;
        eventIndex++;

        if (expected[expectedIndex].dataLength > 0U) {
            while ((eventIndex < g_eventCount) &&
                   (g_events[eventIndex].type != FAKE_EVENT_SPI_WRITE)) {
                eventIndex++;
            }
            TEST_ASSERT(eventIndex < g_eventCount);
            TEST_ASSERT(g_events[eventIndex].dcLevel == PLATFORM_GPIO_LEVEL_HIGH);
            TEST_ASSERT(expected[expectedIndex].dataLength ==
                        g_events[eventIndex].length);
            for (dataIndex = 0U;
                 dataIndex < expected[expectedIndex].dataLength;
                 dataIndex++) {
                TEST_ASSERT(expected[expectedIndex].data[dataIndex] ==
                            g_events[eventIndex].data[dataIndex]);
            }
            eventIndex++;
        }
    }

    while ((eventIndex < g_eventCount) &&
           (g_events[eventIndex].type != FAKE_EVENT_SPI_WRITE)) {
        eventIndex++;
    }
    TEST_ASSERT(eventIndex == g_eventCount);
    TEST_ASSERT(commandCount == 18U);
    for (eventIndex = 0U; eventIndex < g_eventCount; eventIndex++) {
        if ((g_events[eventIndex].type == FAKE_EVENT_DELAY) &&
            (g_events[eventIndex].value == 120U)) {
            TEST_ASSERT(g_events[eventIndex].transactionActive == PLATFORM_FALSE);
            sleepOutDelaySeen = PLATFORM_TRUE;
        }
    }
    TEST_ASSERT(sleepOutDelaySeen == PLATFORM_TRUE);
    TEST_ASSERT(bus.activeDevice == NULL);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_BACKLIGHT].currentLevel == PLATFORM_GPIO_LEVEL_LOW);

    return 0;
}

static int test_init_rollback_preserves_root_failure(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    uint32_t index;
    platform_size_t eventIndex;
    platform_bool_t failedDelaySeen = PLATFORM_FALSE;

    TEST_ASSERT(prepare_constructed_display(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(prepare_started_bus(&bus) == PLATFORM_ERR_OK);
    g_failDelayCall = 4U;
    g_failDelayResult = PLATFORM_ERR_TIMEOUT;
    g_gpioContexts[FAKE_GPIO_BACKLIGHT].deinitResult = PLATFORM_ERR_IO;
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_TIMEOUT);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);
    TEST_ASSERT(display.spiDevice.initialized == PLATFORM_FALSE);
    TEST_ASSERT(bus.activeDevice == NULL);
    for (eventIndex = 0U; eventIndex < g_eventCount; eventIndex++) {
        if ((g_events[eventIndex].type == FAKE_EVENT_DELAY) &&
            (g_events[eventIndex].value == 120U)) {
            TEST_ASSERT(g_events[eventIndex].transactionActive == PLATFORM_FALSE);
            failedDelaySeen = PLATFORM_TRUE;
        }
    }
    TEST_ASSERT(failedDelaySeen == PLATFORM_TRUE);
    for (index = 0U; index < FAKE_GPIO_COUNT; index++) {
        TEST_ASSERT(g_gpioContexts[index].deinitCount == 1U);
    }

    return 0;
}

static int test_init_command_failure_matrix_releases_transaction(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;

    TEST_ASSERT(prepare_constructed_display(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(prepare_started_bus(&bus) == PLATFORM_ERR_OK);
    g_spiContext.applyResult = PLATFORM_ERR_IO;
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_IO);
    TEST_ASSERT(bus.activeDevice == NULL);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);

    display = (platform_st7789_t)PLATFORM_ST7789_INITIALIZER;
    bus = (platform_spi_bus_t)PLATFORM_SPI_BUS_INITIALIZER;
    TEST_ASSERT(prepare_constructed_display(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(prepare_started_bus(&bus) == PLATFORM_ERR_OK);
    g_gpioContexts[FAKE_GPIO_CS].failWriteCall = 2U;
    g_gpioContexts[FAKE_GPIO_CS].failWriteResult = PLATFORM_ERR_TIMEOUT;
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_TIMEOUT);
    TEST_ASSERT(bus.activeDevice == NULL);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);

    display = (platform_st7789_t)PLATFORM_ST7789_INITIALIZER;
    bus = (platform_spi_bus_t)PLATFORM_SPI_BUS_INITIALIZER;
    TEST_ASSERT(prepare_constructed_display(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(prepare_started_bus(&bus) == PLATFORM_ERR_OK);
    g_gpioContexts[FAKE_GPIO_DC].failWriteCall = 1U;
    g_gpioContexts[FAKE_GPIO_DC].failWriteResult = PLATFORM_ERR_IO;
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_IO);
    TEST_ASSERT(bus.activeDevice == NULL);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);

    display = (platform_st7789_t)PLATFORM_ST7789_INITIALIZER;
    bus = (platform_spi_bus_t)PLATFORM_SPI_BUS_INITIALIZER;
    TEST_ASSERT(prepare_constructed_display(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(prepare_started_bus(&bus) == PLATFORM_ERR_OK);
    g_spiContext.failWriteCall = 1U;
    g_spiContext.failWriteResult = PLATFORM_ERR_TIMEOUT;
    g_gpioContexts[FAKE_GPIO_CS].failWriteCall = 3U;
    g_gpioContexts[FAKE_GPIO_CS].failWriteResult = PLATFORM_ERR_IO;
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_TIMEOUT);
    TEST_ASSERT(bus.activeDevice == NULL);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);

    display = (platform_st7789_t)PLATFORM_ST7789_INITIALIZER;
    bus = (platform_spi_bus_t)PLATFORM_SPI_BUS_INITIALIZER;
    TEST_ASSERT(prepare_constructed_display(&display) == PLATFORM_ERR_OK);
    TEST_ASSERT(prepare_started_bus(&bus) == PLATFORM_ERR_OK);
    g_gpioContexts[FAKE_GPIO_CS].failWriteCall = 3U;
    g_gpioContexts[FAKE_GPIO_CS].failWriteResult = PLATFORM_ERR_IO;
    TEST_ASSERT(platform_st7789_init(&display, &bus) == PLATFORM_ERR_IO);
    TEST_ASSERT(bus.activeDevice == NULL);
    TEST_ASSERT(display.initialized == PLATFORM_FALSE);

    return 0;
}

static int test_write_rgb565_uses_one_offset_region_transaction(void)
{
    static const uint16_t pixels[] = {
        0xF800U, 0x07E0U, 0x001FU,
        0xFFFFU, 0x0000U, 0x1234U,
        0x5678U, 0x9ABCU, 0xDEF0U,
        0x1111U, 0x2222U, 0x3333U
    };
    static const uint8_t expectedPixels[] = {
        0xF8U, 0x00U, 0x07U, 0xE0U, 0x00U, 0x1FU,
        0xFFU, 0xFFU, 0x00U, 0x00U, 0x12U, 0x34U,
        0x56U, 0x78U, 0x9AU, 0xBCU
    };
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    const fake_event_t *event;
    uint32_t applyCount;
    uint32_t csWriteCount;
    platform_size_t index;

    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    fake_clear_events();
    applyCount = g_spiContext.applyCount;
    csWriteCount = g_gpioContexts[FAKE_GPIO_CS].writeCount;

    TEST_ASSERT(platform_st7789_write_rgb565(
        &display, 1U, 2U, 3U, 4U, pixels,
        sizeof(pixels) / sizeof(pixels[0])) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_spiContext.applyCount == applyCount + 1U);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_CS].writeCount ==
                csWriteCount + 2U);
    TEST_ASSERT(bus.activeDevice == NULL);

    event = fake_spi_event_at(0U);
    TEST_ASSERT((event != NULL) && (event->length == 1U) &&
                (event->dcLevel == PLATFORM_GPIO_LEVEL_LOW) &&
                (event->data[0] == 0x2AU));
    event = fake_spi_event_at(1U);
    TEST_ASSERT((event != NULL) && (event->length == 4U) &&
                (event->dcLevel == PLATFORM_GPIO_LEVEL_HIGH));
    TEST_ASSERT((event->data[0] == 0x00U) && (event->data[1] == 0x01U) &&
                (event->data[2] == 0x00U) && (event->data[3] == 0x03U));
    event = fake_spi_event_at(2U);
    TEST_ASSERT((event != NULL) && (event->data[0] == 0x2BU));
    event = fake_spi_event_at(3U);
    TEST_ASSERT((event != NULL) && (event->length == 4U));
    TEST_ASSERT((event->data[0] == 0x00U) && (event->data[1] == 0x16U) &&
                (event->data[2] == 0x00U) && (event->data[3] == 0x19U));
    event = fake_spi_event_at(4U);
    TEST_ASSERT((event != NULL) && (event->data[0] == 0x2CU));
    event = fake_spi_event_at(5U);
    TEST_ASSERT((event != NULL) &&
                (event->length == sizeof(pixels)));
    for (index = 0U; index < sizeof(expectedPixels); index++) {
        TEST_ASSERT(event->data[index] == expectedPixels[index]);
    }
    TEST_ASSERT(fake_spi_event_at(7U) == NULL);
    return 0;
}

static int test_region_apis_reject_invalid_input_without_bus_traffic(void)
{
    uint16_t pixel = 0U;
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_st7789_t uninitialized = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    uint32_t applyCount;
    uint32_t writeCount;

    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    fake_clear_events();
    applyCount = g_spiContext.applyCount;
    writeCount = g_spiContext.writeCount;

    TEST_ASSERT(platform_st7789_draw_pixel(NULL, 0U, 0U, pixel) == PLATFORM_ERR_NULL_POINTER);
    TEST_ASSERT(platform_st7789_fill(NULL, pixel) == PLATFORM_ERR_NULL_POINTER);
    TEST_ASSERT(platform_st7789_fill_rect(&display, 0U, 0U, 0U, 1U, pixel) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(platform_st7789_fill_rect(&display, 0U, 0U, 1U, 0U, pixel) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(platform_st7789_fill_rect(&display, 240U, 0U, 1U, 1U, pixel) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(platform_st7789_fill_rect(&display, 0U, 280U, 1U, 1U, pixel) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(platform_st7789_fill_rect(&display, 239U, 0U, 2U, 1U, pixel) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(platform_st7789_fill_rect(&display, 0U, 279U, 1U, 2U, pixel) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(platform_st7789_write_rgb565(
        &display, 0U, 0U, 1U, 1U, NULL, 1U) == PLATFORM_ERR_NULL_POINTER);
    TEST_ASSERT(platform_st7789_write_rgb565(
        &display, 0U, 0U, 1U, 1U, &pixel, 0U) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(platform_st7789_fill_rect(&uninitialized,
                                          0U, 0U, 1U, 1U, pixel) == PLATFORM_ERR_NOT_INITIALIZED);
    TEST_ASSERT(applyCount == g_spiContext.applyCount);
    TEST_ASSERT(writeCount == g_spiContext.writeCount);
    TEST_ASSERT(g_eventCount == 0U);
    return 0;
}

static int test_write_rgb565_chunks_pixels_to_fixed_scratch_buffer(void)
{
    uint16_t pixels[130];
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    const fake_event_t *event;
    platform_size_t index;

    for (index = 0U; index < 130U; index++) {
        pixels[index] = (uint16_t)(0x1000U + index);
    }
    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    fake_clear_events();
    TEST_ASSERT(platform_st7789_write_rgb565(
        &display, 0U, 0U, 130U, 1U, pixels, 130U) == PLATFORM_ERR_OK);
    event = fake_spi_event_at(5U);
    TEST_ASSERT((event != NULL) && (event->length == 256U));
    TEST_ASSERT((event->data[0] == 0x10U) && (event->data[1] == 0x00U));
    event = fake_spi_event_at(6U);
    TEST_ASSERT((event != NULL) && (event->length == 4U));
    TEST_ASSERT((event->data[0] == 0x10U) && (event->data[1] == 0x80U) &&
                (event->data[2] == 0x10U) && (event->data[3] == 0x81U));
    TEST_ASSERT(fake_spi_event_at(7U) == NULL);
    return 0;
}

static int test_pixel_write_preserves_root_error_and_releases_transaction(void)
{
    uint16_t pixels[130] = {0U};
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;

    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    fake_clear_events();
    g_spiContext.failWriteCall = g_spiContext.writeCount + 7U;
    g_spiContext.failWriteResult = PLATFORM_ERR_TIMEOUT;
    g_gpioContexts[FAKE_GPIO_CS].failWriteCall =
        g_gpioContexts[FAKE_GPIO_CS].writeCount + 2U;
    g_gpioContexts[FAKE_GPIO_CS].failWriteResult = PLATFORM_ERR_IO;

    TEST_ASSERT(platform_st7789_write_rgb565(
        &display, 0U, 0U, 130U, 1U, pixels, 130U) == PLATFORM_ERR_TIMEOUT);
    TEST_ASSERT(bus.activeDevice == NULL);
    return 0;
}

static int test_fill_streams_full_screen_in_one_transaction(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    const fake_event_t *event;
    platform_size_t spiEventIndex;
    platform_size_t pixelBytes = 0U;
    platform_size_t pixelWriteCount = 0U;
    uint32_t applyCount;
    uint32_t csWriteCount;

    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    fake_clear_events();
    applyCount = g_spiContext.applyCount;
    csWriteCount = g_gpioContexts[FAKE_GPIO_CS].writeCount;
    TEST_ASSERT(platform_st7789_fill(&display, 0x1234U) == PLATFORM_ERR_OK);

    TEST_ASSERT(g_spiContext.applyCount == applyCount + 1U);
    TEST_ASSERT(g_gpioContexts[FAKE_GPIO_CS].writeCount ==
                csWriteCount + 2U);
    TEST_ASSERT(bus.activeDevice == NULL);
    for (spiEventIndex = 5U;
         (event = fake_spi_event_at(spiEventIndex)) != NULL;
         spiEventIndex++) {
        TEST_ASSERT(event->length <= PLATFORM_ST7789_SCRATCH_BUFFER_SIZE);
        TEST_ASSERT((event->length & 1U) == 0U);
        TEST_ASSERT((event->data[0] == 0x12U) && (event->data[1] == 0x34U));
        pixelBytes += event->length;
        pixelWriteCount++;
    }
    TEST_ASSERT(pixelBytes == 134400U);
    TEST_ASSERT(pixelWriteCount == 525U);
    return 0;
}

static int test_draw_pixel_maps_all_four_logical_corners(void)
{
    static const uint16_t logicalCoordinates[][2] = {
        {0U, 0U}, {239U, 0U}, {0U, 279U}, {239U, 279U}
    };
    static const uint16_t physicalCoordinates[][2] = {
        {0U, 20U}, {239U, 20U}, {0U, 299U}, {239U, 299U}
    };
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_size_t corner;

    TEST_ASSERT(prepare_initialized_display(&display, &bus) == PLATFORM_ERR_OK);
    for (corner = 0U; corner < 4U; corner++) {
        const fake_event_t *xData;
        const fake_event_t *yData;

        fake_clear_events();
        TEST_ASSERT(platform_st7789_draw_pixel(
            &display,
            logicalCoordinates[corner][0],
            logicalCoordinates[corner][1],
            0xABCDU) == PLATFORM_ERR_OK);
        xData = fake_spi_event_at(1U);
        yData = fake_spi_event_at(3U);
        TEST_ASSERT((xData != NULL) && (yData != NULL));
        TEST_ASSERT(xData->data[0] ==
                    (uint8_t)(physicalCoordinates[corner][0] >> 8U));
        TEST_ASSERT(xData->data[1] ==
                    (uint8_t)physicalCoordinates[corner][0]);
        TEST_ASSERT(xData->data[2] == xData->data[0]);
        TEST_ASSERT(xData->data[3] == xData->data[1]);
        TEST_ASSERT(yData->data[0] ==
                    (uint8_t)(physicalCoordinates[corner][1] >> 8U));
        TEST_ASSERT(yData->data[1] ==
                    (uint8_t)physicalCoordinates[corner][1]);
        TEST_ASSERT(yData->data[2] == yData->data[0]);
        TEST_ASSERT(yData->data[3] == yData->data[1]);
    }
    return 0;
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_bsp_gpio_construct_lcd_cs(platform_gpio_t *gpio)
{
    return fake_construct_gpio(gpio, FAKE_GPIO_CS, "lcd-cs");
}

platform_error_t platform_bsp_gpio_construct_lcd_dc(platform_gpio_t *gpio)
{
    return fake_construct_gpio(gpio, FAKE_GPIO_DC, "lcd-dc");
}

platform_error_t platform_bsp_gpio_construct_lcd_reset(platform_gpio_t *gpio)
{
    return fake_construct_gpio(gpio, FAKE_GPIO_RESET, "lcd-reset");
}

platform_error_t platform_bsp_gpio_construct_lcd_backlight(
    platform_gpio_t *gpio)
{
    return fake_construct_gpio(gpio, FAKE_GPIO_BACKLIGHT, "lcd-backlight");
}

platform_error_t platform_time_delay_ms(uint32_t delayMs)
{
    fake_event_t event = {0};

    g_delayCount++;
    event.type = FAKE_EVENT_DELAY;
    event.id = g_delayCount;
    event.value = delayMs;
    event.transactionActive =
        ((g_observedBus != NULL) &&
         (g_observedBus->activeDevice != NULL)) ? PLATFORM_TRUE : PLATFORM_FALSE;
    fake_record_event(&event);

    if ((g_failDelayCall != 0U) && (g_delayCount == g_failDelayCall)) {
        return g_failDelayResult;
    }
    return PLATFORM_ERR_OK;
}

int main(void)
{
    int result = test_bsp_construct_binds_frozen_panel_config_without_io();

    if (result != 0) {
        return result;
    }
    result = test_bsp_construct_rejects_null();
    if (result != 0) {
        return result;
    }
    result = test_init_configures_safe_gpio_levels_and_resets_controller();
    if (result != 0) {
        return result;
    }
    result = test_backlight_and_deinit_release_only_owned_resources();
    if (result != 0) {
        return result;
    }
    result = test_init_validates_state_and_rolls_back_early_gpio_failure();
    if (result != 0) {
        return result;
    }
    result = test_init_uses_verified_table_sequence_and_delay_contract();
    if (result != 0) {
        return result;
    }
    result = test_init_rollback_preserves_root_failure();
    if (result != 0) {
        return result;
    }
    result = test_init_command_failure_matrix_releases_transaction();
    if (result != 0) {
        return result;
    }
    result = test_write_rgb565_uses_one_offset_region_transaction();
    if (result != 0) {
        return result;
    }
    result = test_region_apis_reject_invalid_input_without_bus_traffic();
    if (result != 0) {
        return result;
    }
    result = test_write_rgb565_chunks_pixels_to_fixed_scratch_buffer();
    if (result != 0) {
        return result;
    }
    result = test_pixel_write_preserves_root_error_and_releases_transaction();
    if (result != 0) {
        return result;
    }
    result = test_fill_streams_full_screen_in_one_transaction();
    if (result != 0) {
        return result;
    }
    return test_draw_pixel_maps_all_four_logical_corners();
}
//******************************** Functions *********************************//
