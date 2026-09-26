/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file p169h002_st7789_profile.c
 * @brief P169H002 240x280 ST7789T3 已验证面板参数
 * @author YaoQian Wang
 * @date 2026-09-26
 * @version V1.0
 *****************************************************************************/

#include "p169h002_st7789_profile.h"

const platform_st7789_panel_config_t g_p169h002St7789PanelConfig = {
    240U,
    280U,
    0U,
    20U,
    0x00U,
    {
        PLATFORM_SPI_MODE_3,
        PLATFORM_SPI_BIT_ORDER_MSB_FIRST,
        8U,
        12500000U
    },
    PLATFORM_GPIO_LEVEL_LOW,
    PLATFORM_GPIO_LEVEL_LOW,
    PLATFORM_GPIO_LEVEL_HIGH
};
