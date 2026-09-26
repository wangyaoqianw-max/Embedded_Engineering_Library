/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_st7789.c
 * @brief ST7789T3 Platform concrete device driver 实现
 * @author YaoQian Wang
 * @date 2026-09-06
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_st7789.h"

#include "platform_def.h"
#include "platform_object.h"
#include "platform_time.h"

#include <stddef.h>
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_ST7789_RESET_ASSERT_DELAY_MS   (100U)
#define PLATFORM_ST7789_RESET_RELEASE_DELAY_MS  (100U)
#define PLATFORM_ST7789_POST_RESET_DELAY_MS     (100U)
#define PLATFORM_ST7789_SLEEP_OUT_DELAY_MS      (120U)

#define PLATFORM_ST7789_CMD_SLEEP_OUT           (0x11U)
#define PLATFORM_ST7789_CMD_MADCTL              (0x36U)
#define PLATFORM_ST7789_CMD_DISPLAY_ON          (0x29U)
#define PLATFORM_ST7789_CMD_COLUMN_ADDRESS_SET  (0x2AU)
#define PLATFORM_ST7789_CMD_ROW_ADDRESS_SET     (0x2BU)
#define PLATFORM_ST7789_CMD_MEMORY_WRITE        (0x2CU)

#define PLATFORM_ST7789_RGB565_BYTES_PER_PIXEL  (2U)
#define PLATFORM_ST7789_PIXELS_PER_CHUNK        \
    (PLATFORM_ST7789_SCRATCH_BUFFER_SIZE / \
     PLATFORM_ST7789_RGB565_BYTES_PER_PIXEL)
//******************************** Defines *********************************//

//******************************** Types ***********************************//
typedef struct
{
    uint8_t command;
    const uint8_t *data;
    platform_size_t dataLength;
    uint16_t delayAfterMs;
} platform_st7789_init_entry_t;
//******************************** Types ***********************************//

//******************************** Constants ********************************//
static const uint8_t g_initB2[] = {0x0CU, 0x0CU, 0x00U, 0x33U, 0x33U};
static const uint8_t g_init35[] = {0x00U};
static const uint8_t g_init3A[] = {0x05U};
static const uint8_t g_initB7[] = {0x35U};
static const uint8_t g_initBB[] = {0x2DU};
static const uint8_t g_initC0[] = {0x2CU};
static const uint8_t g_initC2[] = {0x01U};
static const uint8_t g_initC3[] = {0x15U};
static const uint8_t g_initC4[] = {0x20U};
static const uint8_t g_initC6[] = {0x0FU};
static const uint8_t g_initD0[] = {0xA4U, 0xA1U};
static const uint8_t g_initD6[] = {0xA1U};
static const uint8_t g_initE0[] = {
    0x70U, 0x05U, 0x0AU, 0x0BU, 0x0AU, 0x27U, 0x2FU,
    0x44U, 0x47U, 0x37U, 0x14U, 0x14U, 0x29U, 0x2FU
};
static const uint8_t g_initE1[] = {
    0x70U, 0x07U, 0x0CU, 0x08U, 0x08U, 0x04U, 0x2FU,
    0x33U, 0x46U, 0x18U, 0x15U, 0x15U, 0x2BU, 0x2DU
};

static const platform_st7789_init_entry_t g_initSequence[] = {
    {PLATFORM_ST7789_CMD_SLEEP_OUT, NULL, 0U,
     PLATFORM_ST7789_SLEEP_OUT_DELAY_MS},
    {0xB2U, g_initB2, sizeof(g_initB2), 0U},
    {0x35U, g_init35, sizeof(g_init35), 0U},
    {PLATFORM_ST7789_CMD_MADCTL, NULL, 1U, 0U},
    {0x3AU, g_init3A, sizeof(g_init3A), 0U},
    {0xB7U, g_initB7, sizeof(g_initB7), 0U},
    {0xBBU, g_initBB, sizeof(g_initBB), 0U},
    {0xC0U, g_initC0, sizeof(g_initC0), 0U},
    {0xC2U, g_initC2, sizeof(g_initC2), 0U},
    {0xC3U, g_initC3, sizeof(g_initC3), 0U},
    {0xC4U, g_initC4, sizeof(g_initC4), 0U},
    {0xC6U, g_initC6, sizeof(g_initC6), 0U},
    {0xD0U, g_initD0, sizeof(g_initD0), 0U},
    {0xD6U, g_initD6, sizeof(g_initD6), 0U},
    {0xE0U, g_initE0, sizeof(g_initE0), 0U},
    {0xE1U, g_initE1, sizeof(g_initE1), 0U},
    {0x21U, NULL, 0U, 0U},
    {PLATFORM_ST7789_CMD_DISPLAY_ON, NULL, 0U, 0U}
};
//******************************** Constants ********************************//

//******************************** Private Functions *************************//
static platform_gpio_level_t platform_st7789_inactive_level(
    platform_gpio_level_t activeLevel)
{
    return (activeLevel == PLATFORM_GPIO_LEVEL_LOW) ?
           PLATFORM_GPIO_LEVEL_HIGH : PLATFORM_GPIO_LEVEL_LOW;
}

static platform_error_t platform_st7789_validate_constructed(
    const platform_st7789_t *display)
{
    if (display == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((display->cs.initialized != PLATFORM_TRUE) ||
        (display->dc.initialized != PLATFORM_TRUE) ||
        (display->reset.initialized != PLATFORM_TRUE) ||
        (display->backlight.initialized != PLATFORM_TRUE) ||
        (display->width == 0U) || (display->height == 0U)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t platform_st7789_validate_initialized(
    const platform_st7789_t *display)
{
    platform_error_t result = platform_st7789_validate_constructed(display);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if (display->initialized != PLATFORM_TRUE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }
    return PLATFORM_ERR_OK;
}

static platform_error_t platform_st7789_validate_spi_bus(
    const platform_spi_bus_t *spiBus)
{
    if ((platform_object_is_valid(&spiBus->device.object,
                                  PLATFORM_OBJECT_DEVICE) != PLATFORM_TRUE) ||
        (spiBus->device.dev_class != PLATFORM_DEVICE_CLASS_SPI)) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }
    if (spiBus->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }
    return PLATFORM_ERR_OK;
}

static platform_error_t platform_st7789_configure_output(
    platform_gpio_t *gpio,
    platform_gpio_level_t initialLevel)
{
    platform_gpio_config_t config = {
        PLATFORM_GPIO_DIRECTION_OUTPUT,
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_OUTPUT_PUSH_PULL,
        initialLevel
    };

    return platform_gpio_configure(gpio, &config);
}

static platform_error_t platform_st7789_hardware_reset(
    platform_st7789_t *display)
{
    platform_error_t result = platform_gpio_write(
        &display->reset,
        display->resetActiveLevel);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    result = platform_time_delay_ms(
        PLATFORM_ST7789_RESET_ASSERT_DELAY_MS);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    result = platform_gpio_write(
        &display->reset,
        platform_st7789_inactive_level(display->resetActiveLevel));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    result = platform_time_delay_ms(
        PLATFORM_ST7789_RESET_RELEASE_DELAY_MS);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    return platform_time_delay_ms(PLATFORM_ST7789_POST_RESET_DELAY_MS);
}

static platform_error_t platform_st7789_write_command_in_transaction(
    platform_st7789_t *display,
    uint8_t command,
    const uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = platform_gpio_write(
        &display->dc,
        PLATFORM_GPIO_LEVEL_LOW);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    result = platform_spi_write(&display->spiDevice, &command, 1U);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if (dataLength == 0U) {
        return PLATFORM_ERR_OK;
    }
    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = platform_gpio_write(&display->dc, PLATFORM_GPIO_LEVEL_HIGH);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    return platform_spi_write(&display->spiDevice, data, dataLength);
}

static platform_error_t platform_st7789_write_command(
    platform_st7789_t *display,
    uint8_t command,
    const uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t operationResult =
        platform_spi_transaction_begin(&display->spiDevice);
    platform_error_t endResult;

    if (operationResult != PLATFORM_ERR_OK) {
        return operationResult;
    }

    operationResult = platform_st7789_write_command_in_transaction(
        display,
        command,
        data,
        dataLength);
    endResult = platform_spi_transaction_end(&display->spiDevice);

    return (operationResult != PLATFORM_ERR_OK) ?
           operationResult : endResult;
}

static platform_error_t platform_st7789_controller_init(
    platform_st7789_t *display)
{
    platform_size_t index;

    for (index = 0U; index < ARRAY_SIZE(g_initSequence); index++) {
        const platform_st7789_init_entry_t *entry = &g_initSequence[index];
        const uint8_t *data = entry->data;
        platform_error_t result;

        if (entry->command == PLATFORM_ST7789_CMD_MADCTL) {
            data = &display->madctl;
        }

        result = platform_st7789_write_command(
            display,
            entry->command,
            data,
            entry->dataLength);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }
        if (entry->delayAfterMs > 0U) {
            result = platform_time_delay_ms(entry->delayAfterMs);
            if (result != PLATFORM_ERR_OK) {
                return result;
            }
        }
    }

    return PLATFORM_ERR_OK;
}

static void platform_st7789_save_first_error(
    platform_error_t candidate,
    platform_error_t *firstError)
{
    if ((*firstError == PLATFORM_ERR_OK) &&
        (candidate != PLATFORM_ERR_OK)) {
        *firstError = candidate;
    }
}

static void platform_st7789_rollback_init(
    platform_st7789_t *display,
    platform_bool_t csConfigured,
    platform_bool_t dcConfigured,
    platform_bool_t resetConfigured,
    platform_bool_t backlightConfigured)
{
    if (display->spiDevice.initialized == PLATFORM_TRUE) {
        (void)platform_spi_device_deinit(&display->spiDevice);
    }
    if (backlightConfigured == PLATFORM_TRUE) {
        (void)platform_gpio_write(
            &display->backlight,
            platform_st7789_inactive_level(display->backlightOnLevel));
        (void)platform_gpio_deinit(&display->backlight);
    }
    if (resetConfigured == PLATFORM_TRUE) {
        (void)platform_gpio_deinit(&display->reset);
    }
    if (dcConfigured == PLATFORM_TRUE) {
        (void)platform_gpio_deinit(&display->dc);
    }
    if (csConfigured == PLATFORM_TRUE) {
        (void)platform_gpio_deinit(&display->cs);
    }
    display->initialized = PLATFORM_FALSE;
}

static platform_error_t platform_st7789_validate_region(
    const platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height)
{
    platform_error_t result = platform_st7789_validate_initialized(display);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if ((width == 0U) || (height == 0U) ||
        (x >= display->width) || (y >= display->height) ||
        (width > (display->width - x)) ||
        (height > (display->height - y))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }
    return PLATFORM_ERR_OK;
}

static platform_error_t platform_st7789_prepare_region_write(
    platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height)
{
    uint16_t physicalStart;
    uint16_t physicalEnd;
    uint8_t addressData[4];
    platform_error_t result;

    physicalStart = (uint16_t)(x + display->xOffset);
    physicalEnd = (uint16_t)(physicalStart + width - 1U);
    addressData[0] = (uint8_t)(physicalStart >> 8U);
    addressData[1] = (uint8_t)physicalStart;
    addressData[2] = (uint8_t)(physicalEnd >> 8U);
    addressData[3] = (uint8_t)physicalEnd;
    result = platform_st7789_write_command_in_transaction(
        display,
        PLATFORM_ST7789_CMD_COLUMN_ADDRESS_SET,
        addressData,
        sizeof(addressData));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    physicalStart = (uint16_t)(y + display->yOffset);
    physicalEnd = (uint16_t)(physicalStart + height - 1U);
    addressData[0] = (uint8_t)(physicalStart >> 8U);
    addressData[1] = (uint8_t)physicalStart;
    addressData[2] = (uint8_t)(physicalEnd >> 8U);
    addressData[3] = (uint8_t)physicalEnd;
    result = platform_st7789_write_command_in_transaction(
        display,
        PLATFORM_ST7789_CMD_ROW_ADDRESS_SET,
        addressData,
        sizeof(addressData));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_st7789_write_command_in_transaction(
        display,
        PLATFORM_ST7789_CMD_MEMORY_WRITE,
        NULL,
        0U);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    return platform_gpio_write(&display->dc, PLATFORM_GPIO_LEVEL_HIGH);
}

static platform_error_t platform_st7789_finish_transaction(
    platform_st7789_t *display,
    platform_error_t operationResult)
{
    platform_error_t endResult =
        platform_spi_transaction_end(&display->spiDevice);

    return (operationResult != PLATFORM_ERR_OK) ?
           operationResult : endResult;
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_st7789_init(
    platform_st7789_t *display,
    platform_spi_bus_t *spiBus)
{
    platform_bool_t csConfigured = PLATFORM_FALSE;
    platform_bool_t dcConfigured = PLATFORM_FALSE;
    platform_bool_t resetConfigured = PLATFORM_FALSE;
    platform_bool_t backlightConfigured = PLATFORM_FALSE;
    platform_error_t result = PLATFORM_ERR_OK;

    if ((display == NULL) || (spiBus == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }
    result = platform_st7789_validate_constructed(display);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    if (display->initialized == PLATFORM_TRUE) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }
    result = platform_st7789_validate_spi_bus(spiBus);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_st7789_configure_output(
        &display->cs,
        platform_st7789_inactive_level(display->csActiveLevel));
    if (result != PLATFORM_ERR_OK) {
        goto cleanup;
    }
    csConfigured = PLATFORM_TRUE;

    result = platform_st7789_configure_output(
        &display->dc,
        PLATFORM_GPIO_LEVEL_HIGH);
    if (result != PLATFORM_ERR_OK) {
        goto cleanup;
    }
    dcConfigured = PLATFORM_TRUE;

    result = platform_st7789_configure_output(
        &display->reset,
        platform_st7789_inactive_level(display->resetActiveLevel));
    if (result != PLATFORM_ERR_OK) {
        goto cleanup;
    }
    resetConfigured = PLATFORM_TRUE;

    result = platform_st7789_configure_output(
        &display->backlight,
        platform_st7789_inactive_level(display->backlightOnLevel));
    if (result != PLATFORM_ERR_OK) {
        goto cleanup;
    }
    backlightConfigured = PLATFORM_TRUE;

    result = platform_spi_device_init(
        &display->spiDevice,
        "st7789",
        spiBus,
        &display->cs,
        display->csActiveLevel,
        &display->spiConfig);
    if (result != PLATFORM_ERR_OK) {
        goto cleanup;
    }

    result = platform_st7789_hardware_reset(display);
    if (result != PLATFORM_ERR_OK) {
        goto cleanup;
    }

    result = platform_st7789_controller_init(display);
    if (result != PLATFORM_ERR_OK) {
        goto cleanup;
    }

    display->initialized = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;

cleanup:
    platform_st7789_rollback_init(display,
                                  csConfigured,
                                  dcConfigured,
                                  resetConfigured,
                                  backlightConfigured);
    return result;
}

platform_error_t platform_st7789_deinit(platform_st7789_t *display)
{
    platform_error_t firstError =
        platform_st7789_validate_initialized(display);

    if (firstError != PLATFORM_ERR_OK) {
        return firstError;
    }
    firstError = PLATFORM_ERR_OK;

    platform_st7789_save_first_error(
        platform_gpio_write(
            &display->backlight,
            platform_st7789_inactive_level(display->backlightOnLevel)),
        &firstError);
    platform_st7789_save_first_error(
        platform_spi_device_deinit(&display->spiDevice),
        &firstError);
    platform_st7789_save_first_error(
        platform_gpio_deinit(&display->backlight),
        &firstError);
    platform_st7789_save_first_error(
        platform_gpio_deinit(&display->reset),
        &firstError);
    platform_st7789_save_first_error(
        platform_gpio_deinit(&display->dc),
        &firstError);
    platform_st7789_save_first_error(
        platform_gpio_deinit(&display->cs),
        &firstError);

    display->initialized = PLATFORM_FALSE;
    return firstError;
}

platform_error_t platform_st7789_backlight_on(platform_st7789_t *display)
{
    platform_error_t result = platform_st7789_validate_initialized(display);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    return platform_gpio_write(&display->backlight,
                               display->backlightOnLevel);
}

platform_error_t platform_st7789_backlight_off(platform_st7789_t *display)
{
    platform_error_t result = platform_st7789_validate_initialized(display);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    return platform_gpio_write(
        &display->backlight,
        platform_st7789_inactive_level(display->backlightOnLevel));
}

platform_error_t platform_st7789_draw_pixel(
    platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t color)
{
    return platform_st7789_fill_rect(display, x, y, 1U, 1U, color);
}

platform_error_t platform_st7789_fill(
    platform_st7789_t *display,
    uint16_t color)
{
    platform_error_t result = platform_st7789_validate_initialized(display);

    if (result != PLATFORM_ERR_OK) {
        return result;
    }
    return platform_st7789_fill_rect(
        display,
        0U,
        0U,
        display->width,
        display->height,
        color);
}

platform_error_t platform_st7789_fill_rect(
    platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint16_t color)
{
    platform_size_t pixelCount;
    platform_size_t remainingPixels;
    platform_size_t chunkPixels;
    platform_size_t index;
    platform_error_t operationResult =
        platform_st7789_validate_region(display, x, y, width, height);

    if (operationResult != PLATFORM_ERR_OK) {
        return operationResult;
    }

    for (index = 0U; index < PLATFORM_ST7789_PIXELS_PER_CHUNK; index++) {
        display->scratchBuffer[index * 2U] = (uint8_t)(color >> 8U);
        display->scratchBuffer[index * 2U + 1U] = (uint8_t)color;
    }

    operationResult = platform_spi_transaction_begin(&display->spiDevice);
    if (operationResult != PLATFORM_ERR_OK) {
        return operationResult;
    }
    operationResult = platform_st7789_prepare_region_write(
        display, x, y, width, height);

    pixelCount = (platform_size_t)width * (platform_size_t)height;
    remainingPixels = pixelCount;
    while ((operationResult == PLATFORM_ERR_OK) &&
           (remainingPixels > 0U)) {
        chunkPixels = remainingPixels;
        if (chunkPixels > PLATFORM_ST7789_PIXELS_PER_CHUNK) {
            chunkPixels = PLATFORM_ST7789_PIXELS_PER_CHUNK;
        }
        operationResult = platform_spi_write(
            &display->spiDevice,
            display->scratchBuffer,
            chunkPixels * PLATFORM_ST7789_RGB565_BYTES_PER_PIXEL);
        remainingPixels -= chunkPixels;
    }

    return platform_st7789_finish_transaction(display, operationResult);
}

platform_error_t platform_st7789_write_rgb565(
    platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint16_t *pixels,
    platform_size_t pixelCount)
{
    platform_size_t expectedPixelCount;
    platform_size_t pixelIndex = 0U;
    platform_size_t chunkPixels;
    platform_size_t chunkIndex;
    platform_error_t operationResult =
        platform_st7789_validate_region(display, x, y, width, height);

    if (operationResult != PLATFORM_ERR_OK) {
        return operationResult;
    }
    if (pixels == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }
    expectedPixelCount = (platform_size_t)width * (platform_size_t)height;
    if (pixelCount != expectedPixelCount) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    operationResult = platform_spi_transaction_begin(&display->spiDevice);
    if (operationResult != PLATFORM_ERR_OK) {
        return operationResult;
    }
    operationResult = platform_st7789_prepare_region_write(
        display, x, y, width, height);

    while ((operationResult == PLATFORM_ERR_OK) &&
           (pixelIndex < pixelCount)) {
        chunkPixels = pixelCount - pixelIndex;
        if (chunkPixels > PLATFORM_ST7789_PIXELS_PER_CHUNK) {
            chunkPixels = PLATFORM_ST7789_PIXELS_PER_CHUNK;
        }
        for (chunkIndex = 0U; chunkIndex < chunkPixels; chunkIndex++) {
            uint16_t pixel = pixels[pixelIndex + chunkIndex];

            display->scratchBuffer[chunkIndex * 2U] =
                (uint8_t)(pixel >> 8U);
            display->scratchBuffer[chunkIndex * 2U + 1U] = (uint8_t)pixel;
        }
        operationResult = platform_spi_write(
            &display->spiDevice,
            display->scratchBuffer,
            chunkPixels * PLATFORM_ST7789_RGB565_BYTES_PER_PIXEL);
        pixelIndex += chunkPixels;
    }

    return platform_st7789_finish_transaction(display, operationResult);
}
//******************************** Functions *********************************//
