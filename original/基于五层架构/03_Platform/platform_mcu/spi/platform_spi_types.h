/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_spi_types.h
 * @brief Platform SPI 公共数据类型
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_SPI_TYPES_H
#define PLATFORM_SPI_TYPES_H

//******************************** Includes *********************************//
#include "platform_types.h"
//******************************** Includes *********************************//

//******************************** Declaring *********************************//
typedef struct platform_spi_bus platform_spi_bus_t;
typedef struct platform_spi_device platform_spi_device_t;

/*SPI 时钟模式*/
typedef enum
{
    PLATFORM_SPI_MODE_0 = 0,
    PLATFORM_SPI_MODE_1,
    PLATFORM_SPI_MODE_2,
    PLATFORM_SPI_MODE_3,
    PLATFORM_SPI_MODE_MAX
} platform_spi_mode_t;

/*SPI 位传输顺序*/
typedef enum
{
    PLATFORM_SPI_BIT_ORDER_MSB_FIRST = 0,
    PLATFORM_SPI_BIT_ORDER_LSB_FIRST,
    PLATFORM_SPI_BIT_ORDER_MAX
} platform_spi_bit_order_t;

/*SPI 从设备静态配置*/
typedef struct
{
    platform_spi_mode_t mode;
    platform_spi_bit_order_t bitOrder;
    uint8_t dataBits;
    uint32_t maxClockHz;
} platform_spi_device_config_t;
//******************************** Declaring *********************************//

#endif
