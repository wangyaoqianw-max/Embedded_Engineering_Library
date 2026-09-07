/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_service.c
 * @brief platform层，实现平台服务类型的具体内容
 * @author YaoQian Wang
 * @date 2026-08-27
 * @version V1.0
 * @note
 * @warning
 * @history
 * 1. 2026-08-27 创建项目
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_service.h"
//******************************** Includes *********************************//

//******************************** Functions *********************************//
/**
 * @brief 初始化服务对象的公共基础字段
 *
 * @param[out] p_svc          : 指向服务对象本体的指针
 * @param[in] p_name          : 服务的名称
 * @param[in] service_class   : 服务功能的细分类型
 * @param[in] p_lifecycle     : 生命周期管理的指针
 *
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_service_init(platform_service_t *service,
                                       const char *name,
                                       platform_service_class_t serviceClass,
                                       const platform_lifecycle_ops_t *lifecycle)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /* 参数校验。 */
    if (service == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (serviceClass >= PLATFORM_SERVICE_CLASS_MAX) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    /* 初始化基类。 */
    result = platform_object_init(&service->object, name, PLATFORM_OBJECT_SERVICE, NULL);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /* 初始化服务字段。 */
    service->service_class = serviceClass;
    service->lifecycle = lifecycle;

    return PLATFORM_ERR_OK;
}
//******************************** Functions *********************************//
