/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file firmware_version.h
 * @brief Firmware Version V1 公共接口
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H

//******************************** Includes *********************************//
#include "firmware_def.h"
//******************************** Includes *********************************//

//******************************** Declaring *******************************//
/**
 * @brief 比较两个 Firmware Version
 * @param[in] left : 左侧版本；为空时视为低于非空版本
 * @param[in] right : 右侧版本；为空时视为高于空版本
 * @return 大于 0 表示 left 较新；小于 0 表示 left 较旧；0 表示相同。
 * @note 只比较 major、minor、patch，不参与升级或回滚策略。
 */
int32_t firmware_version_compare(const firmware_version_t *left, const firmware_version_t *right);
/**
 * @brief 检查 Firmware Version 是否符合 V1 格式
 * @param[in] version : 待检查版本；为空时返回 0
 * @return 非零表示 reserved 为 0；0 表示格式无效。
 */
platform_bool_t firmware_version_is_valid(const firmware_version_t *version);
//******************************** Declaring *******************************//

#endif
