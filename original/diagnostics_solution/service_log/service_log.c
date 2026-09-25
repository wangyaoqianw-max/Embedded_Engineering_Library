/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file service_log.c
 * @brief Service Log 初始化策略和 Platform Log 转发实现。
 * @author YaoQian Wang
 * @date 2026-08-31
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "service_log.h"
#include "service_log_config.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define LOG_TAG "service_log"
//******************************** Defines *********************************//

//******************************** Variables ********************************//
static platform_bool_t g_serviceLogInitialized = PLATFORM_FALSE;
//******************************** Variables ********************************//

//******************************** Functions *********************************//
static platform_log_level_t service_log_convert_level(service_log_level_t level)
{
    switch (level) {
        case SERVICE_LOG_LEVEL_ERROR:
            return PLATFORM_LOG_LEVEL_ERROR;

        case SERVICE_LOG_LEVEL_WARN:
            return PLATFORM_LOG_LEVEL_WARN;

        case SERVICE_LOG_LEVEL_INFO:
            return PLATFORM_LOG_LEVEL_INFO;

        case SERVICE_LOG_LEVEL_DEBUG:
            return PLATFORM_LOG_LEVEL_DEBUG;

        case SERVICE_LOG_LEVEL_VERBOSE:
            return PLATFORM_LOG_LEVEL_VERBOSE;

        default:
            return PLATFORM_LOG_LEVEL_ERROR;
    }
}

platform_error_t service_log_set_level(service_log_level_t level)
{
    if (level >= SERVICE_LOG_LEVEL_MAX) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return platform_log_set_level(service_log_convert_level(level));
}

platform_error_t service_log_enable_output(platform_bool_t enable)
{
    return platform_log_enable_output(enable);
}

platform_error_t service_log_init(void)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (g_serviceLogInitialized == PLATFORM_TRUE) {
        return PLATFORM_ERR_OK;
    }

    result = platform_log_init();
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = service_log_set_level(DIAGNOSTICS_LOG_DEFAULT_LEVEL);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = service_log_enable_output(DIAGNOSTICS_LOG_DEFAULT_OUTPUT_ENABLE);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    g_serviceLogInitialized = PLATFORM_TRUE;
    SERVICE_LOG_I("log service initialized");

    return PLATFORM_ERR_OK;
}
//******************************** Functions *********************************//
