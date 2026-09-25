/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_w25q64.h
 * @brief W25Q64JV Platform Raw Driver V1 公共接口
 * @author YaoQian Wang
 * @date 2026-09-12
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_W25Q64_H
#define PLATFORM_W25Q64_H

//******************************** Includes *********************************//
#include "platform_gpio.h"
#include "platform_spi.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_W25Q64_INITIALIZER          {0}
#define PLATFORM_W25Q64_TOTAL_SIZE_BYTES     (8U * 1024U * 1024U)
#define PLATFORM_W25Q64_PAGE_SIZE_BYTES      (256U)
#define PLATFORM_W25Q64_SECTOR_SIZE_BYTES    (4096U)
#define PLATFORM_W25Q64_ADDRESS_MAX          (0x7FFFFFU)
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
/**
 * @brief W25Q64JV JEDEC ID
 */
typedef struct
{
    uint8_t manufacturerId; /* 厂商 ID，W25Q64JV 预期为 0xEF */
    uint8_t memoryType;     /* 存储器类型，W25Q64JV 预期为 0x40 */
    uint8_t capacityId;     /* 容量 ID，W25Q64JV 预期为 0x17 */
} platform_w25q64_jedec_id_t;

/**
 * @brief W25Q64JV Raw Driver 运行时对象
 * @note spiDevice、cs 和 spiConfig 由 BSP 构造；initialized 表示 Driver 是否已初始化。
 */
typedef struct
{
    platform_spi_device_t spiDevice; /* 挂接到共享 SPI Bus 的设备对象 */
    platform_gpio_t cs;               /* Flash 软件片选 GPIO 对象 */
    platform_spi_device_config_t spiConfig; /* Flash 专用 SPI 配置 */
    platform_gpio_level_t csActiveLevel;    /* Flash 片选有效电平 */
    uint8_t manufacturerId;                 /* 已识别的厂商 ID */
    uint8_t memoryType;                     /* 已识别的存储器类型 */
    uint8_t capacityId;                     /* 已识别的容量 ID */
    platform_bool_t initialized;            /* Driver 初始化状态 */
} platform_w25q64_t;

/**
 * @brief 初始化 W25Q64JV Raw Driver
 * @param[in,out] flash : 已完成 BSP 构造的 W25Q64JV 对象
 * @param[in] spiBus : 已初始化并启动的 Storage SPI Bus
 * @return PLATFORM_ERR_OK : 初始化成功
 * @return 其他值 : 初始化失败
 * @note 初始化成功后 Driver 接管 Flash CS GPIO 和 SPI Device；不会停止共享 SPI Bus
 */
platform_error_t platform_w25q64_init(
    platform_w25q64_t *flash,
    platform_spi_bus_t *spiBus);

/**
 * @brief 反初始化 W25Q64JV Raw Driver
 * @param[in,out] flash : 已初始化的 W25Q64JV 对象
 * @return PLATFORM_ERR_OK : 反初始化成功
 * @return 其他值 : 反初始化失败
 * @note 仅释放 Flash CS GPIO 和 SPI Device，不会停止共享 SPI Bus
 */
platform_error_t platform_w25q64_deinit(platform_w25q64_t *flash);

/**
 * @brief 读取 W25Q64JV JEDEC ID
 * @param[in] flash : 已初始化的 W25Q64JV 对象
 * @param[out] jedecId : JEDEC ID 输出对象
 * @return PLATFORM_ERR_OK : 读取成功
 * @return 其他值 : 读取失败
 */
platform_error_t platform_w25q64_read_jedec_id(
    platform_w25q64_t *flash,
    platform_w25q64_jedec_id_t *jedecId);

/**
 * @brief 读取 W25Q64JV Status Register-1
 * @param[in] flash : 已初始化的 W25Q64JV 对象
 * @param[out] status : Status Register-1 输出值
 * @return PLATFORM_ERR_OK : 读取成功
 * @return 其他值 : 读取失败
 */
platform_error_t platform_w25q64_read_status1(
    platform_w25q64_t *flash,
    uint8_t *status);

/**
 * @brief 读取 W25Q64JV 连续地址数据
 * @param[in] flash : 已初始化的 W25Q64JV 对象
 * @param[in] address : 起始地址，范围为 0x000000 至 0x7FFFFF
 * @param[out] data : 数据输出缓冲区
 * @param[in] dataLength : 读取字节数
 * @return PLATFORM_ERR_OK : 读取成功
 * @return 其他值 : 读取失败
 * @note 接口为阻塞式读取，不改变 Flash 内容
 */
platform_error_t platform_w25q64_read(
    platform_w25q64_t *flash,
    uint32_t address,
    uint8_t *data,
    platform_size_t dataLength);

/**
 * @brief 对 W25Q64JV 单页执行 Page Program
 * @param[in,out] flash : 已初始化的 W25Q64JV 对象
 * @param[in] address : 起始地址，数据不得跨越 256 Byte Page
 * @param[in] data : 待写入数据缓冲区
 * @param[in] dataLength : 写入字节数，范围为 1 至 256 Byte
 * @return PLATFORM_ERR_OK : 编程完成且 Flash 已恢复 Ready
 * @return 其他值 : 编程失败或参数不满足单页约束
 * @note 本接口不会自动擦除 Flash，也不会跨 Page 执行写入。
 */
platform_error_t platform_w25q64_page_program(
    platform_w25q64_t *flash,
    uint32_t address,
    const uint8_t *data,
    platform_size_t dataLength);

/**
 * @brief 按 Page 边界拆分执行连续写入
 * @param[in,out] flash : 已初始化的 W25Q64JV 对象
 * @param[in] address : 起始地址
 * @param[in] data : 待写入数据缓冲区
 * @param[in] dataLength : 写入字节数，必须完全位于 Flash 地址范围内
 * @return PLATFORM_ERR_OK : 所有 Page 编程均完成
 * @return 其他值 : 参数无效或任一 Page 编程失败
 * @note 本接口只负责 Page 拆分，不自动擦除，也不预读检查目标区域。
 */
platform_error_t platform_w25q64_write(
    platform_w25q64_t *flash,
    uint32_t address,
    const uint8_t *data,
    platform_size_t dataLength);

/**
 * @brief 擦除一个对齐的 4 KiB Sector
 * @param[in,out] flash : 已初始化的 W25Q64JV 对象
 * @param[in] sectorAddress : 4 KiB 对齐的 Sector 起始地址
 * @return PLATFORM_ERR_OK : 擦除完成且 Flash 已恢复 Ready
 * @return 其他值 : 擦除失败或地址未按 Sector 对齐
 * @note 本接口只使用 0x20 Sector Erase，不提供 Chip Erase，也不会自动对齐地址。
 */
platform_error_t platform_w25q64_sector_erase(
    platform_w25q64_t *flash,
    uint32_t sectorAddress);
//******************************** Declaring *********************************//

#endif
