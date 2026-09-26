/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_graphics.h
 * @brief ST7789T3 最小字符绘制接口
 * @author YaoQian Wang
 * @date 2026-09-06
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_GRAPHICS_H
#define PLATFORM_GRAPHICS_H

//******************************** Includes *********************************//
#include "platform_font.h"
#include "platform_st7789.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
/**
 * @brief 在 ST7789 逻辑坐标中不透明绘制一个 8x16 可打印 ASCII 字符
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @param[in] x : 字形左上角逻辑 X 坐标
 * @param[in] y : 字形左上角逻辑 Y 坐标
 * @param[in] character : 可打印 ASCII 字符，范围为 0x20 到 0x7E
 * @param[in] font : 8x16 可打印 ASCII 字体描述
 * @param[in] foreground : 前景 RGB565 颜色
 * @param[in] background : 背景 RGB565 颜色
 * @return platform_error_t : 绘制结果
 * @note 一个字形扩展为 128 个行优先像素，并执行一次区域写入。
 */
platform_error_t platform_graphics_draw_char(
    platform_st7789_t *display, uint16_t x, uint16_t y,
    char_t character, const platform_font_t *font,
    uint16_t foreground, uint16_t background);

/**
 * @brief 从指定逻辑坐标开始不透明绘制可打印 ASCII 字符串
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @param[in] x : 首字符左上角逻辑 X 坐标
 * @param[in] y : 首字符左上角逻辑 Y 坐标
 * @param[in] text : 以空字符结尾的可打印 ASCII 字符串
 * @param[in] font : 8x16 可打印 ASCII 字体描述
 * @param[in] foreground : 前景 RGB565 颜色
 * @param[in] background : 背景 RGB565 颜色
 * @return platform_error_t : 绘制结果
 * @note 写入前验证完整字符串及区域；字符间无额外间距，不自动换行。
 */
platform_error_t platform_graphics_draw_string(
    platform_st7789_t *display, uint16_t x, uint16_t y,
    const char_t *text, const platform_font_t *font,
    uint16_t foreground, uint16_t background);
//******************************** Declaring *********************************//

#endif
