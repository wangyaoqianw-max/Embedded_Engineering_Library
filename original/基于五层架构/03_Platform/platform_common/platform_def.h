/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_def.h
 * @brief platform层，定义上层通用公共宏
 * @author YaoQian Wang
 * @date 2026-06-24
 * @version V1.0
 * @note 
 * @warning 
 * @history
 * 1. 2026-06-24 创建项目
 *
 *****************************************************************************/
#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H
//******************************** Includes *********************************//
#include "platform_types.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/*平台公共布尔值*/
#define PLATFORM_TRUE   1
#define PLATFORM_FALSE  0

/*NULL定义*/
#ifndef NULL
#define NULL     ((void *)0)
#endif

/*字节对齐宏*/
#define PLATFORM_ALIGN_SIZE  (4u)
#define PLATFORM_ALIGN(n)    (((n)+PLATFORM_ALIGN_SIZE-1u)&~(PLATFORM_ALIGN_SIZE-1u))

/*容器大小宏*/
#ifndef ARRAY_SIZE  
#define ARRAY_SIZE(arr)    (sizeof(arr)/sizeof(arr[0]))     
#endif

/*延时宏*/
#define PLATFORM_DELAY_MS(ms)    platform_delay_ms(ms)
#define PLATFORM_DELAY_US(us)    platform_delay_us(us)

//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/*延时函数声明（由impl层具体实现）*/
void platform_delay_ms(uint32_t ms);
void platform_delay_us(uint32_t us);

//******************************** Declaring *********************************//

#endif
