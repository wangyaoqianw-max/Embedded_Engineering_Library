/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_object.h
 * @brief platform层，定义对象基类
 * @author YaoQian Wang
 * @date 2026-06-25
 * @version V1.0
 * @note 
 * @warning 
 * @history 
 * 1. 2026-06-25 创建项目
 *
 *****************************************************************************/
#ifndef PLATFORM_OBJECT_H
#define PLATFORM_OBJECT_H
//******************************** Includes *********************************//
#include "platform_types.h"
#include "platform_error.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/*
    “POBJ”，用来做对象合法性检查
    0x50 = 'P'
    0x4f = 'O'
    0x42 = 'B'
    0x4a = 'J'
*/
#define PLATFORM_OBJECT_MAGIC 0x504F424Au
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/*对象类型*/
typedef enum
{
    PLATFORM_OBJECT_DEVICE,     //设备对象
    PLATFORM_OBJECT_SERVICE,    //服务对象
    PLATFORM_OBJECT_MANAGER,    //管理器对象
    PLATFORM_OBJECT_APP,        //应用对象
    PLATFORM_OBJECT_TYPE_MAX    //边界值
}platform_object_type_t;

/*软件对象状态*/
typedef enum
{
    PLATFORM_OBJECT_CREATED,    //已创建
    PLATFORM_OBJECT_INITIALIZED,//已初始化
    PLATFORM_OBJECT_STARTED,    //已启动
    PLATFORM_OBJECT_STOPPED,    //已停止
    PLATFORM_OBJECT_ERROR,      //错误
    PLATFORM_OBJECT_STATE_MAX   //边界值
}platform_object_state_t;

/*系统对象结构体*/
typedef struct 
{
    uint32 magic;                      //对象身份证
    const char *name;                  //对象名称
    platform_object_type_t type;       //对象高层类型
    platform_object_state_t state;     //对象状态

    void *parent;                      //父对象指针
    void *user_data;                   //用户扩展指针
    uint32 flags;                      //标志位，给后续扩展使用
}platform_object_t;

/**
 * @brief 初始化平台对象的公共基础字段
 *
 * Steps:
 *  1. 检查对象指针是否有效
 *  2. 写入对象的magic，名称，类型和初始状态
 *  3. 清除标志位和数据
 *
 * @param[in] obj    : 指向对象本体的指针
 * @param[in] name   : 对象的名称
 * @param[in] type   : 对象的高层类型
 * @param[in] parent : 指向父对象的指针
 *
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_object_init(platform_object_t *obj,
                                      const char *name,
                                      platform_object_type_t type,
                                      void *parent);

/**
 * @brief 更新平台生命周期状态记录
 *
 * @param[in] obj   : 指向对象本体的指针
 * @param[in] state : 新的对象生命周期状态
 *
 * @return platform_error_t : 函数执行状态
 */
platform_error_t platform_object_set_state(platform_object_t *obj,
                                           platform_object_state_t state);

/**
 * @brief 检查平台对象指针是否匹配期望的对象类型
 *
 * @param[in] obj   : 指向对象本体的指针
 * @param[in] type  : 期望的平台对象高层类型
 *
 * @return platform_bool_t : 如果对象指针有效返回true
 */
platform_bool_t platform_object_is_valid(const platform_object_t *obj,
                                         platform_object_type_t type);

//******************************** Declaring *********************************//

#endif
