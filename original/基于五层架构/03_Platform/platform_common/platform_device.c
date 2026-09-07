/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_device.c
 * @brief platform层，平台设备的具体实现
 * @author YaoQian Wang
 * @date 2026-06-25
 * @version V1.0
 * @note
 * @warning
 * @history
 * 1. 2026-06-25 创建项目
 *
 *****************************************************************************/
//******************************** Includes *********************************//
#include "platform_device.h"
//******************************** Includes *********************************//

//******************************** Functions *********************************//
/**
 * @brief 初始化设备对象的公共基础字段
 *
 * @param[in] p_dev       : 指向设备对象本体的指针
 * @param[in] name        : 设备对象的名称
 * @param[in] dev_class   : 对象的细分类型
 * @param[in] caps        : 设备能力标志
 * @param[in] p_lifecycle : 生命周期管理的指针
 *
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_device_init(platform_device_t *device,
                                      const char *name,
                                      platform_device_class_t deviceClass,
                                      uint32_t capabilities,
                                      const platform_lifecycle_ops_t *lifecycle)
{
    platform_error_t result = PLATFORM_ERR_OK;

    /* 参数校验。 */
    if (device == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (deviceClass >= PLATFORM_DEVICE_CLASS_MAX) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    /* 初始化基类。 */
    result = platform_object_init(&device->object, name, PLATFORM_OBJECT_DEVICE, NULL);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    /* 初始化设备字段。 */
    device->dev_class = deviceClass;
    device->caps = capabilities;
    device->power_state = PLATFORM_DEVICE_POWER_OFF;
    device->lifecycle = lifecycle;

    return PLATFORM_ERR_OK;
}

/**
 * @brief 修改设备的电源及运行状态
 *
 * @param[in] p_dev       : 指向设备对象本体的指针
 * @param[in] power_state : 电源及运行状态
 *
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_device_set_power_state(platform_device_t *device,
                                                 platform_device_power_state_t powerState)
{
    if (device == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (platform_object_is_valid(&device->object, PLATFORM_OBJECT_DEVICE) != PLATFORM_TRUE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (powerState >= PLATFORM_DEVICE_POWER_STATE_MAX) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    device->power_state = powerState;

    return PLATFORM_ERR_OK;
}
//******************************** Functions *********************************//

