/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file usart.h
 * @brief Impl Platform UART 主机测试使用的最小 HAL UART 替身
 * @author YaoQian Wang
 * @date 2026-08-29
 * @version V1.0
 *
 *****************************************************************************/

#ifndef TEST_IMPL_PLATFORM_UART_USART_H
#define TEST_IMPL_PLATFORM_UART_USART_H

typedef enum
{
    HAL_OK = 0U,
    HAL_ERROR,
    HAL_BUSY,
    HAL_TIMEOUT
} HAL_StatusTypeDef;

typedef struct
{
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t Mode;
    uint32_t HwFlowCtl;
    uint32_t OverSampling;
} UART_InitTypeDef;

typedef struct
{
    void *Instance;
    UART_InitTypeDef Init;
    uint32_t ErrorCode;
} UART_HandleTypeDef;

#define UART_WORDLENGTH_8B  (0x00000000U)
#define UART_WORDLENGTH_9B  (0x00001000U)
#define UART_STOPBITS_1     (0x00000000U)
#define UART_STOPBITS_2     (0x00002000U)
#define UART_PARITY_NONE    (0x00000000U)
#define UART_PARITY_EVEN    (0x00000400U)
#define UART_PARITY_ODD     (0x00000600U)
#define UART_MODE_TX_RX     (0x0000000CU)
#define UART_HWCONTROL_NONE (0x00000000U)
#define UART_OVERSAMPLING_16 (0x00000000U)

extern UART_HandleTypeDef huart1;
extern HAL_StatusTypeDef g_fakeHalReceiveToIdleResult;
extern HAL_StatusTypeDef g_fakeHalAbortReceiveResult;
extern HAL_StatusTypeDef g_fakeHalAbortTransmitResult;
extern HAL_StatusTypeDef g_fakeHalTransmitDmaResult;
extern UART_HandleTypeDef *g_fakeHalReceiveToIdleUart;
extern uint8_t *g_fakeHalReceiveToIdleBuffer;
extern uint16_t g_fakeHalReceiveToIdleSize;
extern uint32_t g_fakeHalReceiveToIdleCallCount;
extern uint32_t g_fakeHalAbortReceiveCallCount;
extern uint32_t g_fakeHalAbortTransmitCallCount;
extern UART_HandleTypeDef *g_fakeHalTransmitDmaUart;
extern uint8_t *g_fakeHalTransmitDmaBuffer;
extern uint16_t g_fakeHalTransmitDmaSize;
extern uint32_t g_fakeHalTransmitDmaCallCount;
extern uint16_t g_fakeHalReceiveToIdleCallbackPosition;

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);

static inline HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *halUart)
{
    (void)halUart;
    return HAL_OK;
}

static inline HAL_StatusTypeDef HAL_UART_DeInit(UART_HandleTypeDef *halUart)
{
    (void)halUart;
    return HAL_OK;
}

static inline HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *halUart,
                                                   uint8_t *data,
                                                   uint16_t dataLength,
                                                   uint32_t timeoutMs)
{
    (void)halUart;
    (void)data;
    (void)dataLength;
    (void)timeoutMs;
    return HAL_OK;
}

static inline HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *halUart,
                                                  uint8_t *buffer,
                                                  uint16_t bufferSize,
                                                  uint32_t timeoutMs)
{
    (void)halUart;
    (void)buffer;
    (void)bufferSize;
    (void)timeoutMs;
    return HAL_OK;
}

static inline HAL_StatusTypeDef HAL_UARTEx_ReceiveToIdle_DMA(
    UART_HandleTypeDef *halUart,
    uint8_t *buffer,
    uint16_t bufferSize)
{
    g_fakeHalReceiveToIdleUart = halUart;
    g_fakeHalReceiveToIdleBuffer = buffer;
    g_fakeHalReceiveToIdleSize = bufferSize;
    g_fakeHalReceiveToIdleCallCount++;

    if (0U != g_fakeHalReceiveToIdleCallbackPosition) {
        HAL_UARTEx_RxEventCallback(halUart, g_fakeHalReceiveToIdleCallbackPosition);
    }

    return g_fakeHalReceiveToIdleResult;
}

static inline HAL_StatusTypeDef HAL_UART_AbortReceive(UART_HandleTypeDef *halUart)
{
    (void)halUart;
    g_fakeHalAbortReceiveCallCount++;

    return g_fakeHalAbortReceiveResult;
}

static inline HAL_StatusTypeDef HAL_UART_AbortTransmit(UART_HandleTypeDef *halUart)
{
    (void)halUart;
    g_fakeHalAbortTransmitCallCount++;

    return g_fakeHalAbortTransmitResult;
}

static inline HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef *halUart,
                                                       uint8_t *data,
                                                       uint16_t dataLength)
{
    g_fakeHalTransmitDmaUart = halUart;
    g_fakeHalTransmitDmaBuffer = data;
    g_fakeHalTransmitDmaSize = dataLength;
    g_fakeHalTransmitDmaCallCount++;

    return g_fakeHalTransmitDmaResult;
}

#define HAL_UART_ERROR_NONE (0x00000000U)
#define HAL_UART_ERROR_PE   (0x00000001U)
#define HAL_UART_ERROR_NE   (0x00000002U)
#define HAL_UART_ERROR_FE   (0x00000004U)
#define HAL_UART_ERROR_ORE  (0x00000008U)
#define HAL_UART_ERROR_DMA  (0x00000010U)

#endif
