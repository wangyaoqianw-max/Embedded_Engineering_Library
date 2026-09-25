/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_spi.h
 * @brief Platform SPI Bus、Device 与同步事务公共接口
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_SPI_H
#define PLATFORM_SPI_H

//******************************** Includes *********************************//
#include "platform_spi_types.h"

#include "platform_device.h"
#include "platform_gpio.h"
#include "platform_lifecycle.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_SPI_BUS_INITIALIZER    {0}
#define PLATFORM_SPI_DEVICE_INITIALIZER {0}
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/**
 * @brief SPI Bus 数据操作表，由 Impl 层注入
 * @note applyConfig 确保当前 Bus 配置满足目标 Device；Phase 1 允许只校验不重配置。
 */
typedef struct
{
    platform_error_t (*applyConfig)(
        platform_spi_bus_t *bus,
        const platform_spi_device_config_t *config);
    platform_error_t (*write)(
        platform_spi_bus_t *bus,
        const uint8_t *data,
        platform_size_t dataLength);
    platform_error_t (*read)(
        platform_spi_bus_t *bus,
        uint8_t *data,
        platform_size_t dataLength);
} platform_spi_bus_ops_t;

/*Platform SPI Bus 设备对象*/
struct platform_spi_bus
{
    platform_device_t device;
    const platform_spi_bus_ops_t *ops;
    void *implContext;
    platform_spi_device_t *activeDevice;
};

/*Platform SPI 从设备轻量描述符*/
struct platform_spi_device
{
    const char *name;
    platform_spi_bus_t *bus;
    platform_gpio_t *cs;
    platform_gpio_level_t csActiveLevel;
    platform_spi_device_config_t config;
    platform_bool_t initialized;
};

/**
 * @brief Platform SPI Bus 对象构造参数
 * @note lifecycle、ops 和 implContext 均为非拥有型引用，必须在 Bus 使用期间保持有效。
 */
typedef struct
{
    const char *name;
    uint32_t caps;
    const platform_lifecycle_ops_t *lifecycle;
    const platform_spi_bus_ops_t *ops;
    void *implContext;
} platform_spi_bus_init_params_t;

/**
 * @brief 构造 Platform SPI Bus 对象
 * @param[in,out] bus : 使用 PLATFORM_SPI_BUS_INITIALIZER 清零的 Bus 对象
 * @param[in] params : 名称、生命周期、Ops 和实现上下文
 * @return platform_error_t : 函数执行状态
 * @note 本函数只构造抽象对象，不初始化或配置 SPI 硬件。
 */
platform_error_t platform_spi_bus_init(
    platform_spi_bus_t *bus,
    const platform_spi_bus_init_params_t *params);

/**
 * @brief 通过 Platform facade 初始化 SPI Bus 生命周期
 * @param[in,out] bus : 已构造、处于 CREATED 状态的 SPI Bus
 * @return platform_error_t : 生命周期初始化结果
 */
platform_error_t platform_spi_bus_lifecycle_init(platform_spi_bus_t *bus);

/**
 * @brief 通过 Platform facade 启动 SPI Bus
 * @param[in,out] bus : 已完成生命周期初始化或已停止的 SPI Bus
 * @return platform_error_t : 生命周期启动结果
 */
platform_error_t platform_spi_bus_lifecycle_start(platform_spi_bus_t *bus);

/**
 * @brief 通过 Platform facade 停止 SPI Bus
 * @param[in,out] bus : 已启动且没有活动事务的 SPI Bus
 * @return platform_error_t : 生命周期停止结果
 */
platform_error_t platform_spi_bus_lifecycle_stop(platform_spi_bus_t *bus);

/**
 * @brief 通过 Platform facade 反初始化 SPI Bus
 * @param[in,out] bus : 已初始化但未启动，或已停止且没有活动事务的 SPI Bus
 * @return platform_error_t : 生命周期反初始化结果
 */
platform_error_t platform_spi_bus_lifecycle_deinit(platform_spi_bus_t *bus);

/**
 * @brief 初始化挂接到 SPI Bus 的从设备描述符
 * @param[in,out] device : 使用 PLATFORM_SPI_DEVICE_INITIALIZER 清零的设备描述符
 * @param[in] name : 设备名称，不得为 NULL
 * @param[in] bus : 已构造的 SPI Bus，非拥有型引用
 * @param[in] cs : 可选软件片选 GPIO，NULL 表示无片选操作
 * @param[in] csActiveLevel : 片选有效电平
 * @param[in] config : SPI Mode、位序、数据位和设备最大时钟
 * @return platform_error_t : 函数执行状态
 * @note 有 CS 时，本函数会先将其设置为无效电平。
 */
platform_error_t platform_spi_device_init(
    platform_spi_device_t *device,
    const char *name,
    platform_spi_bus_t *bus,
    platform_gpio_t *cs,
    platform_gpio_level_t csActiveLevel,
    const platform_spi_device_config_t *config);

/**
 * @brief 解除 SPI Device 描述符绑定
 * @param[in,out] device : 已初始化且未持有事务的设备描述符
 * @return platform_error_t : 函数执行状态
 * @note 有 CS 时会尝试恢复无效电平；即使 GPIO 写失败也会清除软件绑定。
 */
platform_error_t platform_spi_device_deinit(platform_spi_device_t *device);

/**
 * @brief 开始一笔显式 SPI 事务
 * @param[in,out] device : 已初始化且 Bus 已 STARTED 的 SPI Device
 * @return platform_error_t : 函数执行状态
 * @note 成功后调用者持有 Bus，必须调用 platform_spi_transaction_end() 释放。
 */
platform_error_t platform_spi_transaction_begin(platform_spi_device_t *device);

/**
 * @brief 在当前显式事务中阻塞发送字节流
 * @param[in,out] device : 当前 Bus activeDevice
 * @param[in] data : 发送缓冲区，函数返回前保持有效
 * @param[in] dataLength : 发送字节数，必须大于 0
 * @return platform_error_t : 函数执行状态
 * @note 发送失败不会隐式结束事务。
 */
platform_error_t platform_spi_write(
    platform_spi_device_t *device,
    const uint8_t *data,
    platform_size_t dataLength);

/**
 * @brief 在当前显式事务中阻塞接收字节流
 * @param[in,out] device : 当前 Bus activeDevice
 * @param[out] data : 接收缓冲区，函数返回前保持有效
 * @param[in] dataLength : 接收字节数，必须大于 0
 * @return platform_error_t : 函数执行状态
 * @note 接收失败不会隐式结束事务。
 */
platform_error_t platform_spi_read(
    platform_spi_device_t *device,
    uint8_t *data,
    platform_size_t dataLength);

/**
 * @brief 结束一笔显式 SPI 事务
 * @param[in,out] device : 当前 Bus activeDevice
 * @return platform_error_t : 函数执行状态
 * @note 即使 CS 恢复失败，也会清除 Bus 的软件事务所有权。
 */
platform_error_t platform_spi_transaction_end(platform_spi_device_t *device);
//******************************** Declaring *********************************//

#endif
