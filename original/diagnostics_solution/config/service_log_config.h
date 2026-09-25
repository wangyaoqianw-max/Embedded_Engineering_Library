/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * @file service_log_config.h
 * @brief Diagnostics Solution 的 Service Log 默认策略
 * @author YaoQian Wang
 * @date 2026-09-25
 * @version V1.0
 *****************************************************************************/

#ifndef SERVICE_LOG_CONFIG_H
#define SERVICE_LOG_CONFIG_H

#include "service_log.h"

#ifndef DIAGNOSTICS_LOG_DEFAULT_LEVEL
#define DIAGNOSTICS_LOG_DEFAULT_LEVEL          SERVICE_LOG_LEVEL_INFO
#endif

#ifndef DIAGNOSTICS_LOG_DEFAULT_OUTPUT_ENABLE
#define DIAGNOSTICS_LOG_DEFAULT_OUTPUT_ENABLE  PLATFORM_TRUE
#endif

#endif
