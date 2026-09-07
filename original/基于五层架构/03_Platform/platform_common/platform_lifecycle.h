/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_lifecycle.h
 * @brief platform层，对象生命周期入口
 * @author YaoQian Wang
 * @date 2026-06-25
 * @version V1.0
 * @note 
 * @warning 
 * @history 
 * 1. 2026-06-25 创建项目
 *
 *****************************************************************************/
#ifndef PLATFORM_LIFECYCLE_H
#define PLATFORM_LIFECYCLE_H
//******************************** Includes *********************************//
#include "platform_types.h"
#include "platform_error.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
/**
 * @brief Platform 对象的生命周期操作集合。
 * @note self 指向绑定对象但不转移所有权；调用顺序由对象状态机约束。
 */
typedef struct
{
    platform_error_t (*init)(void *self);
    platform_error_t (*start)(void *self);
    platform_error_t (*process)(void *self);
    platform_error_t (*stop)(void *self);
    platform_error_t (*deinit)(void *self);
} platform_lifecycle_ops_t;

//******************************** Declaring *********************************//
#endif
