/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_st7789.h
 * @brief ST7789T3 Platform concrete device driver 公共接口
 * @author YaoQian Wang
 * @date 2026-09-06
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_ST7789_H
#define PLATFORM_ST7789_H

//******************************** Includes *********************************//
#include "platform_gpio.h"
#include "platform_spi.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_ST7789_INITIALIZER           {0}
#define PLATFORM_ST7789_SCRATCH_BUFFER_SIZE   (256U)

#define PLATFORM_ST7789_COLOR_BLACK           (0x0000U)
#define PLATFORM_ST7789_COLOR_WHITE           (0xFFFFU)
#define PLATFORM_ST7789_COLOR_RED             (0xF800U)
#define PLATFORM_ST7789_COLOR_GREEN           (0x07E0U)
#define PLATFORM_ST7789_COLOR_BLUE            (0x001FU)
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
typedef struct
{
    platform_spi_device_t spiDevice;
    platform_gpio_t cs;
    platform_gpio_t dc;
    platform_gpio_t reset;
    platform_gpio_t backlight;
    platform_spi_device_config_t spiConfig;
    platform_gpio_level_t csActiveLevel;
    platform_gpio_level_t resetActiveLevel;
    platform_gpio_level_t backlightOnLevel;
    uint16_t width;
    uint16_t height;
    uint16_t xOffset;
    uint16_t yOffset;
    uint8_t madctl;
    uint8_t scratchBuffer[PLATFORM_ST7789_SCRATCH_BUFFER_SIZE];
    platform_bool_t initialized;
} platform_st7789_t;

/**
 * @brief 初始化已完成 BSP 静态绑定的 ST7789 显示设备
 * @param[in,out] display : ST7789 对象
 * @param[in,out] spiBus : 已启动的共享 SPI Bus，非拥有型引用
 * @return platform_error_t : 初始化结果
 * @note 仅允许在 Task Context 调用；成功后背光保持关闭。
 */
platform_error_t platform_st7789_init(
    platform_st7789_t *display,
    platform_spi_bus_t *spiBus);

/**
 * @brief 反初始化 ST7789 拥有的 SPI Device 与 GPIO 资源
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @return platform_error_t : 反初始化结果
 * @note 不停止或反初始化共享 SPI Bus。
 */
platform_error_t platform_st7789_deinit(platform_st7789_t *display);

/**
 * @brief 打开 ST7789 背光
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_st7789_backlight_on(platform_st7789_t *display);

/**
 * @brief 关闭 ST7789 背光
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_st7789_backlight_off(platform_st7789_t *display);

/**
 * @brief 绘制一个逻辑坐标像素
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @param[in] x : 逻辑 X 坐标
 * @param[in] y : 逻辑 Y 坐标
 * @param[in] color : RGB565 颜色
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_st7789_draw_pixel(
    platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t color);

/**
 * @brief 用单一 RGB565 颜色填充完整逻辑屏幕
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @param[in] color : RGB565 颜色
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_st7789_fill(
    platform_st7789_t *display,
    uint16_t color);

/**
 * @brief 用单一 RGB565 颜色填充一个逻辑区域
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @param[in] x : 逻辑区域左上角 X
 * @param[in] y : 逻辑区域左上角 Y
 * @param[in] width : 区域宽度
 * @param[in] height : 区域高度
 * @param[in] color : RGB565 颜色
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_st7789_fill_rect(
    platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    uint16_t color);

/**
 * @brief 将 row-major RGB565 像素写入一个逻辑区域
 * @param[in,out] display : 已初始化的 ST7789 对象
 * @param[in] x : 逻辑区域左上角 X
 * @param[in] y : 逻辑区域左上角 Y
 * @param[in] width : 区域宽度
 * @param[in] height : 区域高度
 * @param[in] pixels : RGB565 像素数组
 * @param[in] pixelCount : 像素数组元素数，必须等于 width * height
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_st7789_write_rgb565(
    platform_st7789_t *display,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    const uint16_t *pixels,
    platform_size_t pixelCount);
//******************************** Declaring *********************************//

#endif
