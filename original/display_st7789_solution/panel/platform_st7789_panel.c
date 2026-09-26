/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file platform_st7789_panel.c
 * @brief ST7789 面板静态参数配置实现
 * @author YaoQian Wang
 * @date 2026-09-26
 * @version V1.0
 *****************************************************************************/

#include <stddef.h>

#include "platform_def.h"
#include "platform_st7789_panel.h"

platform_error_t platform_st7789_apply_panel_config(
    platform_st7789_t *display,
    const platform_st7789_panel_config_t *config)
{
    if ((display == NULL) || (config == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((display->initialized == PLATFORM_TRUE) ||
        (config->width == 0U) ||
        (config->height == 0U) ||
        (config->spiConfig.dataBits != 8U) ||
        (config->spiConfig.maxClockHz == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    display->width = config->width;
    display->height = config->height;
    display->xOffset = config->xOffset;
    display->yOffset = config->yOffset;
    display->madctl = config->madctl;
    display->spiConfig = config->spiConfig;
    display->csActiveLevel = config->csActiveLevel;
    display->resetActiveLevel = config->resetActiveLevel;
    display->backlightOnLevel = config->backlightOnLevel;

    return PLATFORM_ERR_OK;
}
