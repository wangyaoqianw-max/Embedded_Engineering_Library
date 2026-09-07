/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_error.h
 * @brief platform层，定义上层通用错误类型
 * @author YaoQian Wang
 * @date 2026-06-24
 * @version V1.0
 * @note 
 * @warning 
 * @history
 * 1. 2026-06-24 创建项目
 *
 *****************************************************************************/
#ifndef PLATFORM_ERROR_H
#define PLATFORM_ERROR_H
//******************************** Includes *********************************//
#include "platform_types.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
/**
 * @brief 平台公共错误码
 *
 * @note PLATFORM_ERR_OK 表示成功，其余值均表示失败。
 * @note 错误码发布后不应修改已有数值；新增错误码应添加在
 *       PLATFORM_ERR_RESERVED 之前。
 */
typedef enum
{
    PLATFORM_ERR_OK = 0,               //通用成功值
 
    PLATFORM_ERR_UNKNOWN,              //未知错误，在无法归类的时候使用
    PLATFORM_ERR_TIMEOUT,              //超时错误，操作在规定时间内没有完成
    PLATFORM_ERR_INVALID_PARAM,        //参数的数值、范围、长度或者格式无效
    PLATFORM_ERR_NULL_POINTER,         //必要的指针参数为空
    PLATFORM_ERR_INVALID_STATE,        //当前模块不允许执行该操作

    PLATFORM_ERR_NO_MEMORY,            //内存或内存池空间不足
    PLATFORM_ERR_NO_RESOURCE,          //句柄、通道等非内存资源不足
    PLATFORM_ERR_BUSY,                 //资源暂时忙线
    PLATFORM_ERR_FULL,                 //队列、缓冲区或存储空间已满
    PLATFORM_ERR_EMPTY,                //队列、缓冲区或容器中空的

    PLATFORM_ERR_NOT_SUPPORTED,        //当前平台不支持该操作
    PLATFORM_ERR_NOT_INITIALIZED,      //模块尚未初始化
    PLATFORM_ERR_ALREADY_INITIALIZED,  //模块已经完成初始化
    PLATFORM_ERR_NOT_FOUND,            //未找到指定的设备、对象或数据

    PLATFORM_ERR_IO,                   //外设、总线、存储等输入输出操作失败
    PLATFORM_ERR_OVERFLOW,             //数值、长度或数据范围发生溢出
    PLATFORM_ERR_CHECKSUM,             //CRC或校验和验证失败
    PLATFORM_ERR_PERMISSION,           //当前调用者没有执行该操作的权限
    PLATFORM_ERR_CANCELED,             //操作在完成前被取消

    PLATFORM_ERR_RESERVED = 0x7FFFFFFF //保留值，正常接口不应该返回
} platform_error_t;

/*错误判断宏*/
#define PLATFORM_IS_ERR(err)     ((err)!=PLATFORM_ERR_OK)
#define PLATFORM_IS_OK(err)      ((err)==PLATFORM_ERR_OK)

//******************************** Declaring *********************************//

#endif
