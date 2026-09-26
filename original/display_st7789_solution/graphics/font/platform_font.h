/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_font.h
 * @brief Platform Graphics 点阵字体描述类型
 * @author YaoQian Wang
 * @date 2026-09-06
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_FONT_H
#define PLATFORM_FONT_H

//******************************** Includes *********************************//
#include "platform_types.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
/**
 * @brief 行优先、每行一字节的单色点阵字体描述
 * @note glyphData 中每个字形占 bytesPerGlyph 字节，行内 bit0 对应左侧像素。
 */
typedef struct
{
    uint8_t width;
    uint8_t height;
    uint8_t firstChar;
    uint8_t lastChar;
    const uint8_t *glyphData;
    platform_size_t bytesPerGlyph;
} platform_font_t;
//******************************** Declaring *********************************//

#endif
