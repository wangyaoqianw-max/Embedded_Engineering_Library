/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file spi.h
 * @brief Impl Platform SPI 主机测试使用的最小 HAL SPI 替身
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.0
 *
 *****************************************************************************/

#ifndef TEST_IMPL_PLATFORM_SPI_SPI_H
#define TEST_IMPL_PLATFORM_SPI_SPI_H

#include "platform_types.h"

typedef enum
{
    HAL_OK = 0U,
    HAL_ERROR,
    HAL_BUSY,
    HAL_TIMEOUT
} HAL_StatusTypeDef;

typedef enum
{
    HAL_SPI_STATE_RESET = 0U,
    HAL_SPI_STATE_READY,
    HAL_SPI_STATE_BUSY
} HAL_SPI_StateTypeDef;

typedef struct
{
    uint32_t Mode;
    uint32_t Direction;
    uint32_t DataSize;
    uint32_t CLKPolarity;
    uint32_t CLKPhase;
    uint32_t NSS;
    uint32_t BaudRatePrescaler;
    uint32_t FirstBit;
    uint32_t TIMode;
    uint32_t CRCCalculation;
    uint32_t CRCPolynomial;
} SPI_InitTypeDef;

typedef struct
{
    void *Instance;
    SPI_InitTypeDef Init;
    HAL_SPI_StateTypeDef State;
} SPI_HandleTypeDef;

#define SPI_MODE_MASTER             (0x00000004U)
#define SPI_DIRECTION_2LINES        (0x00000000U)
#define SPI_DATASIZE_8BIT           (0x00000000U)
#define SPI_DATASIZE_16BIT          (0x00000800U)
#define SPI_POLARITY_LOW            (0x00000000U)
#define SPI_POLARITY_HIGH           (0x00000002U)
#define SPI_PHASE_1EDGE             (0x00000000U)
#define SPI_PHASE_2EDGE             (0x00000001U)
#define SPI_NSS_SOFT                (0x00000200U)
#define SPI_BAUDRATEPRESCALER_2     (0x00000000U)
#define SPI_BAUDRATEPRESCALER_4     (0x00000008U)
#define SPI_BAUDRATEPRESCALER_8     (0x00000010U)
#define SPI_BAUDRATEPRESCALER_16    (0x00000018U)
#define SPI_BAUDRATEPRESCALER_32    (0x00000020U)
#define SPI_BAUDRATEPRESCALER_64    (0x00000028U)
#define SPI_BAUDRATEPRESCALER_128   (0x00000030U)
#define SPI_BAUDRATEPRESCALER_256   (0x00000038U)
#define SPI_FIRSTBIT_MSB            (0x00000000U)
#define SPI_FIRSTBIT_LSB            (0x00000080U)

extern SPI_HandleTypeDef hspi1;
extern HAL_StatusTypeDef g_fakeHalSpiTransmitResult;
extern SPI_HandleTypeDef *g_fakeHalSpiTransmitHandle;
extern uint8_t *g_fakeHalSpiTransmitData;
extern uint16_t g_fakeHalSpiTransmitLength;
extern uint32_t g_fakeHalSpiTransmitTimeoutMs;
extern uint32_t g_fakeHalSpiTransmitCount;
extern uint32_t g_fakePclk2Hz;

static inline HAL_SPI_StateTypeDef HAL_SPI_GetState(SPI_HandleTypeDef *halSpi)
{
    return halSpi->State;
}

static inline uint32_t HAL_RCC_GetPCLK2Freq(void)
{
    return g_fakePclk2Hz;
}

static inline HAL_StatusTypeDef HAL_SPI_Transmit(
    SPI_HandleTypeDef *halSpi,
    uint8_t *data,
    uint16_t dataLength,
    uint32_t timeoutMs)
{
    g_fakeHalSpiTransmitHandle = halSpi;
    g_fakeHalSpiTransmitData = data;
    g_fakeHalSpiTransmitLength = dataLength;
    g_fakeHalSpiTransmitTimeoutMs = timeoutMs;
    g_fakeHalSpiTransmitCount++;
    return g_fakeHalSpiTransmitResult;
}

#endif
