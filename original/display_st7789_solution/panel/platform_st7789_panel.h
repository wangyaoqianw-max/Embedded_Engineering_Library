/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_st7789_panel.h
 * @brief ST7789 面板静态参数配置接口
 * @author YaoQian Wang
 * @date 2026-09-26
 * @version V1.0
 *****************************************************************************/

#ifndef PLATFORM_ST7789_PANEL_H
#define PLATFORM_ST7789_PANEL_H

#include "platform_st7789.h"

typedef struct
{
    uint16_t width;
    uint16_t height;
    uint16_t xOffset;
    uint16_t yOffset;
    uint8_t madctl;
    platform_spi_device_config_t spiConfig;
    platform_gpio_level_t csActiveLevel;
    platform_gpio_level_t resetActiveLevel;
    platform_gpio_level_t backlightOnLevel;
} platform_st7789_panel_config_t;

platform_error_t platform_st7789_apply_panel_config(
    platform_st7789_t *display,
    const platform_st7789_panel_config_t *config);

#endif
