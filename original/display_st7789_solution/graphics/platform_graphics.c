/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_graphics.c
 * @brief ST7789T3 最小字符绘制实现
 * @author YaoQian Wang
 * @date 2026-09-06
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_graphics.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_GRAPHICS_ASCII_WIDTH             (8U)
#define PLATFORM_GRAPHICS_ASCII_HEIGHT            (16U)
#define PLATFORM_GRAPHICS_ASCII_FIRST             (0x20U)
#define PLATFORM_GRAPHICS_ASCII_LAST              (0x7EU)
#define PLATFORM_GRAPHICS_ASCII_BYTES_PER_GLYPH   (16U)
#define PLATFORM_GRAPHICS_GLYPH_PIXEL_COUNT       (128U)
//******************************** Defines *********************************//

//******************************** Private Functions *************************//
static platform_bool_t platform_graphics_is_ascii_8x16(
    const platform_font_t *font)
{
    return (font->width == PLATFORM_GRAPHICS_ASCII_WIDTH) &&
           (font->height == PLATFORM_GRAPHICS_ASCII_HEIGHT) &&
           (font->firstChar == PLATFORM_GRAPHICS_ASCII_FIRST) &&
           (font->lastChar == PLATFORM_GRAPHICS_ASCII_LAST) &&
           (font->bytesPerGlyph == PLATFORM_GRAPHICS_ASCII_BYTES_PER_GLYPH) &&
           (font->glyphData != NULL);
}

static platform_error_t platform_graphics_validate_common(
    const platform_st7789_t *display, const platform_font_t *font)
{
    if ((display == NULL) || (font == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }
    if (display->initialized == PLATFORM_FALSE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }
    if (platform_graphics_is_ascii_8x16(font) == PLATFORM_FALSE) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }
    return PLATFORM_ERR_OK;
}

static platform_error_t platform_graphics_validate_region(
    const platform_st7789_t *display, uint16_t x, uint16_t y,
    platform_size_t characterCount)
{
    if ((x >= display->width) || (y >= display->height)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }
    if ((display->height - y) < (uint16_t)PLATFORM_GRAPHICS_ASCII_HEIGHT) {
        return PLATFORM_ERR_INVALID_PARAM;
    }
    if (characterCount > ((display->width - x) / PLATFORM_GRAPHICS_ASCII_WIDTH)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }
    return PLATFORM_ERR_OK;
}

static platform_error_t platform_graphics_write_glyph(
    platform_st7789_t *display, uint16_t x, uint16_t y,
    uint8_t character, const platform_font_t *font,
    uint16_t foreground, uint16_t background)
{
    uint16_t pixels[PLATFORM_GRAPHICS_GLYPH_PIXEL_COUNT];
    const uint8_t *glyph = font->glyphData +
        ((platform_size_t)(character - font->firstChar) * font->bytesPerGlyph);
    platform_size_t row;
    platform_size_t column;

    for (row = 0U; row < PLATFORM_GRAPHICS_ASCII_HEIGHT; row++) {
        for (column = 0U; column < PLATFORM_GRAPHICS_ASCII_WIDTH; column++) {
            pixels[row * PLATFORM_GRAPHICS_ASCII_WIDTH + column] =
                ((glyph[row] & ((uint8_t)1U << column)) != 0U) ?
                foreground : background;
        }
    }
    return platform_st7789_write_rgb565(display, x, y,
                                        PLATFORM_GRAPHICS_ASCII_WIDTH,
                                        PLATFORM_GRAPHICS_ASCII_HEIGHT,
                                        pixels,
                                        PLATFORM_GRAPHICS_GLYPH_PIXEL_COUNT);
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_graphics_draw_char(
    platform_st7789_t *display, uint16_t x, uint16_t y,
    char_t character, const platform_font_t *font,
    uint16_t foreground, uint16_t background)
{
    platform_error_t error;
    uint8_t ascii = (uint8_t)character;

    error = platform_graphics_validate_common(display, font);
    if (error != PLATFORM_ERR_OK) {
        return error;
    }
    if ((ascii < font->firstChar) || (ascii > font->lastChar)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }
    error = platform_graphics_validate_region(display, x, y, 1U);
    if (error != PLATFORM_ERR_OK) {
        return error;
    }
    return platform_graphics_write_glyph(display, x, y, ascii, font,
                                         foreground, background);
}

platform_error_t platform_graphics_draw_string(
    platform_st7789_t *display, uint16_t x, uint16_t y,
    const char_t *text, const platform_font_t *font,
    uint16_t foreground, uint16_t background)
{
    platform_error_t error;
    platform_size_t length = 0U;
    platform_size_t index;

    if (text == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }
    error = platform_graphics_validate_common(display, font);
    if (error != PLATFORM_ERR_OK) {
        return error;
    }
    while (text[length] != '\0') {
        uint8_t ascii = (uint8_t)text[length];

        if ((ascii < font->firstChar) || (ascii > font->lastChar)) {
            return PLATFORM_ERR_INVALID_PARAM;
        }
        length++;
    }
    error = platform_graphics_validate_region(display, x, y, length);
    if (error != PLATFORM_ERR_OK) {
        return error;
    }
    for (index = 0U; index < length; index++) {
        error = platform_graphics_write_glyph(
            display, (uint16_t)(x + index * PLATFORM_GRAPHICS_ASCII_WIDTH), y,
            (uint8_t)text[index], font, foreground, background);
        if (error != PLATFORM_ERR_OK) {
            return error;
        }
    }
    return PLATFORM_ERR_OK;
}
//******************************** Functions *********************************//
