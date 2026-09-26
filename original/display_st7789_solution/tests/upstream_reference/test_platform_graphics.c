/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_platform_graphics.c
 * @brief 验证最小字符绘制与 ASCII 8x16 字体合同
 * @author YaoQian Wang
 * @date 2026-09-06
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_graphics.h"
#include "platform_font_ascii_8x16.h"

#include <stddef.h>
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define TEST_ASSERT(condition)       \
    do {                             \
        if (!(condition)) {          \
            return __LINE__;         \
        }                            \
    } while (0)
//******************************** Defines *********************************//

//******************************** Variables ********************************//
static uint32_t g_writeCount;
static platform_error_t g_writeResult;
static uint16_t g_writeX[2];
static uint16_t g_writeY[2];
static uint16_t g_lastPixels[128];
static uint16_t g_lastWidth;
static uint16_t g_lastHeight;
static platform_size_t g_lastPixelCount;
//******************************** Variables ********************************//

//******************************** Private Functions *************************//
static void reset_write_fake(void)
{
    g_writeCount = 0U;
    g_writeResult = PLATFORM_ERR_OK;
    g_lastWidth = 0U;
    g_lastHeight = 0U;
    g_lastPixelCount = 0U;
}

static int test_ascii_font_descriptor_and_literal_rows(void)
{
    const uint8_t *glyphs = g_platformFontAscii8x16.glyphData;

    TEST_ASSERT(g_platformFontAscii8x16.width == 8U);
    TEST_ASSERT(g_platformFontAscii8x16.height == 16U);
    TEST_ASSERT(g_platformFontAscii8x16.firstChar == 0x20U);
    TEST_ASSERT(g_platformFontAscii8x16.lastChar == 0x7EU);
    TEST_ASSERT(g_platformFontAscii8x16.bytesPerGlyph == 16U);
    TEST_ASSERT(glyphs[0U] == 0x00U);
    TEST_ASSERT(glyphs[(0x41U - 0x20U) * 16U + 3U] == 0x08U);
    TEST_ASSERT(glyphs[(0x41U - 0x20U) * 16U + 13U] == 0xE7U);
    TEST_ASSERT(glyphs[(0x7EU - 0x20U) * 16U] == 0x0CU);
    return 0;
}

static int test_draw_char_expands_lsb_first_and_writes_one_region(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;

    display.width = 240U;
    display.height = 280U;
    display.initialized = PLATFORM_TRUE;
    reset_write_fake();
    TEST_ASSERT(platform_graphics_draw_char(
        &display, 3U, 4U, 'A', &g_platformFontAscii8x16,
        0x1234U, 0xABCDU) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_writeCount == 1U);
    TEST_ASSERT(g_writeX[0] == 3U);
    TEST_ASSERT(g_writeY[0] == 4U);
    TEST_ASSERT(g_lastWidth == 8U);
    TEST_ASSERT(g_lastHeight == 16U);
    TEST_ASSERT(g_lastPixelCount == 128U);
    TEST_ASSERT(g_lastPixels[3U * 8U] == 0xABCDU);
    TEST_ASSERT(g_lastPixels[3U * 8U + 1U] == 0xABCDU);
    TEST_ASSERT(g_lastPixels[3U * 8U + 2U] == 0xABCDU);
    TEST_ASSERT(g_lastPixels[3U * 8U + 3U] == 0x1234U);
    TEST_ASSERT(g_lastPixels[3U * 8U + 4U] == 0xABCDU);
    TEST_ASSERT(g_lastPixels[3U * 8U + 5U] == 0xABCDU);
    TEST_ASSERT(g_lastPixels[3U * 8U + 6U] == 0xABCDU);
    TEST_ASSERT(g_lastPixels[3U * 8U + 7U] == 0xABCDU);
    return 0;
}

static int test_draw_char_accepts_printable_limits_and_rejects_neighbors(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;

    display.width = 240U;
    display.height = 280U;
    display.initialized = PLATFORM_TRUE;
    reset_write_fake();
    TEST_ASSERT(platform_graphics_draw_char(
        &display, 0U, 0U, (char_t)0x20U, &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_OK);
    TEST_ASSERT(platform_graphics_draw_char(
        &display, 8U, 0U, (char_t)0x7EU, &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_writeCount == 2U);
    TEST_ASSERT(platform_graphics_draw_char(
        &display, 0U, 0U, (char_t)0x1FU, &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(platform_graphics_draw_char(
        &display, 0U, 0U, (char_t)0x7FU, &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(g_writeCount == 2U);
    return 0;
}

static int test_draw_string_advances_by_eight(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;

    display.width = 240U;
    display.height = 280U;
    display.initialized = PLATFORM_TRUE;
    reset_write_fake();
    TEST_ASSERT(platform_graphics_draw_string(
        &display, 10U, 11U, "AB", &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_writeCount == 2U);
    TEST_ASSERT(g_writeX[0] == 10U);
    TEST_ASSERT(g_writeX[1] == 18U);
    TEST_ASSERT(g_writeY[0] == 11U);
    TEST_ASSERT(g_writeY[1] == 11U);
    return 0;
}

static int test_draw_string_validates_whole_region_before_writes(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;

    display.width = 15U;
    display.height = 16U;
    display.initialized = PLATFORM_TRUE;
    reset_write_fake();
    TEST_ASSERT(platform_graphics_draw_string(
        &display, 0U, 0U, "AB", &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(g_writeCount == 0U);
    display.width = 16U;
    display.height = 15U;
    TEST_ASSERT(platform_graphics_draw_string(
        &display, 0U, 0U, "A", &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_INVALID_PARAM);
    TEST_ASSERT(g_writeCount == 0U);
    return 0;
}

static int test_draw_string_stops_after_first_write_failure(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;

    display.width = 240U;
    display.height = 280U;
    display.initialized = PLATFORM_TRUE;
    reset_write_fake();
    g_writeResult = PLATFORM_ERR_IO;
    TEST_ASSERT(platform_graphics_draw_string(
        &display, 0U, 0U, "AB", &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_IO);
    TEST_ASSERT(g_writeCount == 1U);
    return 0;
}

static int test_draw_string_empty_is_noop_after_validation(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;

    display.width = 240U;
    display.height = 280U;
    display.initialized = PLATFORM_TRUE;
    reset_write_fake();
    TEST_ASSERT(platform_graphics_draw_string(
        &display, 0U, 0U, "", &g_platformFontAscii8x16,
        1U, 0U) == PLATFORM_ERR_OK);
    TEST_ASSERT(g_writeCount == 0U);
    return 0;
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_st7789_write_rgb565(
    platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint16_t *pixels,
    platform_size_t pixelCount)
{
    platform_size_t index;

    (void)display;
    g_writeX[g_writeCount] = x;
    g_writeY[g_writeCount] = y;
    g_writeCount++;
    g_lastWidth = width;
    g_lastHeight = height;
    g_lastPixelCount = pixelCount;
    for (index = 0U; index < pixelCount; index++) {
        g_lastPixels[index] = pixels[index];
    }
    return g_writeResult;
}

int main(void)
{
    int result = test_ascii_font_descriptor_and_literal_rows();

    if (result != 0) {
        return result;
    }
    result = test_draw_char_expands_lsb_first_and_writes_one_region();
    if (result != 0) {
        return result;
    }
    result = test_draw_char_accepts_printable_limits_and_rejects_neighbors();
    if (result != 0) {
        return result;
    }
    result = test_draw_string_advances_by_eight();
    if (result != 0) {
        return result;
    }
    result = test_draw_string_validates_whole_region_before_writes();
    if (result != 0) {
        return result;
    }
    result = test_draw_string_stops_after_first_write_failure();
    if (result != 0) {
        return result;
    }
    return test_draw_string_empty_is_noop_after_validation();
}
//******************************** Functions *********************************//
