/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_types.h
 * @brief platform层，定义上层通用基础类型
 * @author YaoQian Wang
 * @date 2026-06-24
 * @version V1.0
 * @note 
 * @warning 不依赖任何标准库
 * @history
 * 1. 2026-06-24 创建项目
 *
 *****************************************************************************/
#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H
//******************************** Includes *********************************//
#include "board_types.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
/*基础类型*/
typedef int8       int8_t;
typedef uint8      uint8_t;
typedef int16      int16_t;
typedef uint16     uint16_t;
typedef int32      int32_t;
typedef uint32     uint32_t;
typedef int64      int64_t;
typedef uint64     uint64_t;

/*浮点类型*/
typedef float32    float_t;
typedef float64    double_t;

/*字符类型*/
typedef char       char_t;
typedef uint8      uchar_t;

/*布尔类型*/
typedef uint8      platform_bool_t;

/*长度与索引类型*/
typedef uint32     platform_size_t;

//******************************** Declaring *********************************//

#endif
