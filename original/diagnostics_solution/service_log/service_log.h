/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file service_log.h
 * @brief Service 层统一普通日志接口。
 * @author YaoQian Wang
 * @date 2026-08-31
 * @version V1.0
 *
 *****************************************************************************/

#ifndef SERVICE_LOG_H
#define SERVICE_LOG_H

//******************************** Includes *********************************//
#include "platform_error.h"
#include "platform_log.h"
//******************************** Includes *********************************//

//******************************** Types ***********************************//
typedef enum
{
    SERVICE_LOG_LEVEL_ERROR = 0,
    SERVICE_LOG_LEVEL_WARN,
    SERVICE_LOG_LEVEL_INFO,
    SERVICE_LOG_LEVEL_DEBUG,
    SERVICE_LOG_LEVEL_VERBOSE,
    SERVICE_LOG_LEVEL_MAX
} service_log_level_t;
//******************************** Types ***********************************//

//******************************** Defines *********************************//
#define SERVICE_LOG_E(...) platform_log_e(__VA_ARGS__)
#define SERVICE_LOG_W(...) platform_log_w(__VA_ARGS__)
#define SERVICE_LOG_I(...) platform_log_i(__VA_ARGS__)
#define SERVICE_LOG_D(...) platform_log_d(__VA_ARGS__)
#define SERVICE_LOG_V(...) platform_log_v(__VA_ARGS__)
//******************************** Defines *********************************//

//******************************** Declaring *******************************//
/**
 * @brief 初始化 Service Log 并应用一次项目默认策略。
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败。
 */
platform_error_t service_log_init(void);

/**
 * @brief 设置 Service Log 的全局等级。
 * @param[in] level Service Log 等级。
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败。
 */
platform_error_t service_log_set_level(service_log_level_t level);

/**
 * @brief 打开或关闭 Service Log 的全局输出。
 * @param[in] enable 使用 PLATFORM_TRUE 表示启用输出，使用 PLATFORM_FALSE 表示关闭输出。
 * @return PLATFORM_ERR_OK 表示成功，其他 platform_error_t 表示失败。
 */
platform_error_t service_log_enable_output(platform_bool_t enable);
//******************************** Declaring *******************************//

#endif
