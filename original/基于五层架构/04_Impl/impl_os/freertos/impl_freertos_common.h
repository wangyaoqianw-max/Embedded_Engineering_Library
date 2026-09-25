/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_freertos_common.h
 * @brief 定义 FreeRTOS CMSIS-RTOS2 Adapter 的内部公共辅助接口。
 * @author YaoQian Wang
 * @date 2026-08-30
 * @version V1.0
 *****************************************************************************/

#ifndef IMPL_FREERTOS_COMMON_H
#define IMPL_FREERTOS_COMMON_H

#include "platform_os.h"

#include "cmsis_os2.h"

/**
 * @brief 将 Platform 毫秒超时转换为 CMSIS tick。
 * @param[in] timeoutMs 超时毫秒；支持 PLATFORM_OS_NO_WAIT 和 PLATFORM_OS_WAIT_FOREVER。
 * @return 转换后的 CMSIS tick。
 */
uint32_t impl_freertos_timeout_to_ticks(uint32_t timeoutMs);

/**
 * @brief 将通用 CMSIS 状态转换为 Platform 错误码。
 * @param[in] status CMSIS-RTOS2 返回状态。
 * @return 对应的 Platform 错误码。
 */
platform_error_t impl_freertos_map_status(osStatus_t status);

#endif
