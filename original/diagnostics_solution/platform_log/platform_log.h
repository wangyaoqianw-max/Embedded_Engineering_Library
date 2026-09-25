/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_log.h
 * @brief platform层，日志集中管理头文件
 * @author YaoQian Wang
 * @date 2026-05-09
 * @version V1.0
 * @note 只定义接口，不做具体实现
 * @warning 
 * @history
 * 1. 2026-05-09 创建项目
 *
 *****************************************************************************/
#ifndef PLATFORM_LOG_H
#define PLATFORM_LOG_H

//******************************** Includes *********************************//
#include <stdbool.h>
#include <stdint.h>
#include "platform_def.h"
#include "platform_error.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/*对日志等级进行枚举定义*/
typedef enum
{
    PLATFORM_LOG_LEVEL_ASSERT = 0,
    PLATFORM_LOG_LEVEL_ERROR,
    PLATFORM_LOG_LEVEL_WARN,
    PLATFORM_LOG_LEVEL_INFO,
    PLATFORM_LOG_LEVEL_DEBUG,
    PLATFORM_LOG_LEVEL_VERBOSE,
    PLATFORM_LOG_LEVEL_MAX
} platform_log_level_t;

typedef void (*platform_log_output_fn_t)(
    uint8_t level,
    const char *tag,
    const char *file,
    const char *func,
    long line,
    const char *format,
    ...);

#define platform_log_e(...)                                                   \
    platform_log_get_output_fn()((uint8_t)PLATFORM_LOG_LEVEL_ERROR, LOG_TAG,  \
                               __FILE__, __FUNCTION__, (long)__LINE__,      \
                               __VA_ARGS__)
#define platform_log_w(...)                                                   \
    platform_log_get_output_fn()((uint8_t)PLATFORM_LOG_LEVEL_WARN, LOG_TAG,   \
                               __FILE__, __FUNCTION__, (long)__LINE__,      \
                               __VA_ARGS__)
#define platform_log_i(...)                                                   \
    platform_log_get_output_fn()((uint8_t)PLATFORM_LOG_LEVEL_INFO, LOG_TAG,   \
                               __FILE__, __FUNCTION__, (long)__LINE__,      \
                               __VA_ARGS__)
#define platform_log_d(...)                                                   \
    platform_log_get_output_fn()((uint8_t)PLATFORM_LOG_LEVEL_DEBUG, LOG_TAG,  \
                               __FILE__, __FUNCTION__, (long)__LINE__,      \
                               __VA_ARGS__)
#define platform_log_v(...)                                                   \
    platform_log_get_output_fn()((uint8_t)PLATFORM_LOG_LEVEL_VERBOSE, LOG_TAG,\
                               __FILE__, __FUNCTION__, (long)__LINE__,      \
                               __VA_ARGS__)
//******************************** Defines *********************************//

//******************************** Function *********************************//
/**
 * @brief  对日志进行初始化
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败
 */
platform_error_t platform_log_init(void);

/**
 * @brief  设置可以输出的日志的级别
 * @param  level  代表要设置的日志级别
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败
 */
platform_error_t platform_log_set_level(platform_log_level_t level);

/**
 * @brief  打开/关闭日志输出
 * @param  enable  代表日志的开关指令
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败
 */
platform_error_t platform_log_enable_output(bool enable);

/**
 * @brief  获取当前日志输出后端
 * @return 当前 Impl 绑定的输出函数，初始化前返回 no-op 后端
 */
platform_log_output_fn_t platform_log_get_output_fn(void);
//******************************** Function *********************************//

#endif
