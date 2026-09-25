/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_spi.h
 * @brief STM32F4 HAL SPI Platform Bus 构造入口
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.1
 *
 *****************************************************************************/

#ifndef IMPL_PLATFORM_SPI_H
#define IMPL_PLATFORM_SPI_H

//******************************** Includes *********************************//
#include "platform_spi.h"
#include "stm32f4xx_hal.h"
//******************************** Includes *********************************//

//******************************** Types ************************************//
typedef struct
{
    SPI_HandleTypeDef *halSpi;
} impl_platform_spi_context_t;
//******************************** Types ************************************//

//******************************** Functions ********************************//
/**
 * @brief 构造并绑定 STM32F4 HAL SPI Handle
 * @param[in,out] bus : 使用 PLATFORM_SPI_BUS_INITIALIZER 清零的 Bus 对象
 * @param[in] name : Platform SPI Bus 名称
 * @param[in] caps : 设备能力标志
 * @param[in] context : 调用者持有的 HAL Handle 上下文
 * @return platform_error_t : 构造结果；本函数不初始化 CubeMX 外设
 */
platform_error_t impl_platform_spi_construct(
    platform_spi_bus_t *bus,
    const char *name,
    uint32_t caps,
    impl_platform_spi_context_t *context);
//******************************** Functions ********************************//

#endif
