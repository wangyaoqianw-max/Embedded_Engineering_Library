/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_device.h
 * @brief platform层，定义设备类型
 * @author YaoQian Wang
 * @date 2026-06-25
 * @version V1.0
 * @note 
 * @warning 
 * @history 
 * 1. 2026-06-25 创建项目
 *
 *****************************************************************************/
#ifndef PLATFORM_DEVICE_H
#define PLATFORM_DEVICE_H
//******************************** Includes *********************************//
#include "platform_types.h"
#include "platform_error.h"
#include "platform_object.h"
#include "platform_lifecycle.h"
#include "platform_def.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//

//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/*平台设备分类，描述设备的功能类别*/
typedef enum
{
    PLATFORM_DEVICE_CLASS_UNKNOWN = 0, //未知设备

    PLATFORM_DEVICE_CLASS_DISPLAY,     //显示设备，如oled
    PLATFORM_DEVICE_CLASS_TOUCH,       //触摸输入设备，如电容触摸屏
    PLATFORM_DEVICE_CLASS_IMU,         //惯性测量设备，如陀螺仪
    PLATFORM_DEVICE_CLASS_TEMP_HUMI,   //温湿度传感器设备
    PLATFORM_DEVICE_CLASS_STORAGE,     //数据存储设备，如flash
    PLATFORM_DEVICE_CLASS_BACKLIGHT,   //背光控制设备
    PLATFORM_DEVICE_CLASS_MOTOR,       //电机及执行机构设备
    PLATFORM_DEVICE_CLASS_CPU,         //处理器相关设备
    PLATFORM_DEVICE_CLASS_POWER,       //电源管理设备，如电源开关
    PLATFORM_DEVICE_CLASS_UART,        //通用异步收发设备
    PLATFORM_DEVICE_CLASS_SPI,         //通用同步串行总线控制器

    PLATFORM_DEVICE_CLASS_MAX
} platform_device_class_t;

/*平台设备能力标志，每个枚举值表示一个独立能力*/
/* bit 4 reserved */
/* bit 6 reserved */
typedef enum
{
    PLATFORM_DEVICE_CAP_NONE                = 0U,      
    PLATFORM_DEVICE_CAP_SLEEP               = 1U << 0,  //支持进入低功耗休眠状态
    PLATFORM_DEVICE_CAP_DEEP_SLEEP          = 1U << 1,  //支持进入深度睡眠状态
    PLATFORM_DEVICE_CAP_POWER_OFF           = 1U << 2,  //支持软件控制设备完全断电
    PLATFORM_DEVICE_CAP_WAKEUP_SRC          = 1U << 3,  //可以作为系统或设备唤醒源

    PLATFORM_DEVICE_CAP_NEED_REINIT         = 1U << 5,  //唤醒或重新上电后需要重新初始化

    PLATFORM_DEVICE_CAP_HIGH_POWER          = 1U << 7,  //高功耗设备
    PLATFORM_DEVICE_CAP_CLOCK_SCALABLE      = 1U << 8,  //支持动态调整工作时钟或运行效率
    PLATFORM_DEVICE_CAP_SAMPLE_SCALABLE     = 1U << 9,  //支持动态调整采样率
    PLATFORM_DEVICE_CAP_BRIGHTNESS_SCALABLE = 1U << 10, //支持动态调整亮度
} platform_device_cap_t;

/*设备电源以及运行状态*/
typedef enum{
    PLATFORM_DEVICE_POWER_OFF,       //处于关闭或者断电状态
    PLATFORM_DEVICE_POWER_SLEEP,     //处于低功耗睡眠状态
    PLATFORM_DEVICE_POWER_IDLE,      //设备已就绪，但当前未执行主要工作
    PLATFORM_DEVICE_POWER_ACTIVE,    //设备处于正常工作状态
    PLATFORM_DEVICE_POWER_ERROR,     //设备出现异常，当前状态不可正常使用

    PLATFORM_DEVICE_POWER_STATE_MAX,
}platform_device_power_state_t;

/*平台设备基础对象*/
typedef struct 
{
    platform_object_t object;                   //平台对象基础信息
    platform_device_class_t dev_class;          //设备功能分类
    uint32_t caps;                              //设备能力位掩码
    platform_device_power_state_t power_state;  //设备电源状态
    const platform_lifecycle_ops_t *lifecycle;  //生命周期操作接口
}platform_device_t;

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
platform_error_t platform_device_init(platform_device_t *p_dev,
                                      const char *p_name,
                                      platform_device_class_t dev_class,
                                      uint32_t caps,
                                      const platform_lifecycle_ops_t *p_lifecycle);

/**
 * @brief 修改设备电源及运行状态
 *
 * @param[in,out] p_dev       : 指向设备对象本体的指针
 * @param[in]     power_state : 新的设备电源及运行状态
 *
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_device_set_power_state(
                                platform_device_t *p_dev,
                                platform_device_power_state_t power_state);

//******************************** Declaring *********************************//
#endif
