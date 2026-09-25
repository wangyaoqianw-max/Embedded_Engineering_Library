/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file stm32f4xx_hal.h
 * @brief GPIO STM32 Impl Host Test 使用的最小 HAL 替身
 * @author YaoQian Wang
 * @date 2026-09-01
 * @version V1.0
 *
 *****************************************************************************/

#ifndef TEST_IMPL_PLATFORM_GPIO_STM32F4XX_HAL_H
#define TEST_IMPL_PLATFORM_GPIO_STM32F4XX_HAL_H

#include "platform_types.h"

typedef struct
{
    uint32_t marker;
} GPIO_TypeDef;

typedef struct
{
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
    uint32_t Alternate;
} GPIO_InitTypeDef;

typedef enum
{
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET
} GPIO_PinState;

#define GPIO_PIN_0 ((uint16_t)0x0001U)
#define GPIO_PIN_1 ((uint16_t)0x0002U)
#define GPIO_PIN_2 ((uint16_t)0x0004U)
#define GPIO_PIN_3 ((uint16_t)0x0008U)
#define GPIO_PIN_15 ((uint16_t)0x8000U)
#define GPIO_PIN_All ((uint16_t)0xFFFFU)

#define GPIO_MODE_INPUT     0x00000000U
#define GPIO_MODE_OUTPUT_PP 0x00000001U
#define GPIO_MODE_OUTPUT_OD 0x00000011U

#define GPIO_NOPULL   0x00000000U
#define GPIO_PULLUP   0x00000001U
#define GPIO_PULLDOWN 0x00000002U

#define GPIO_SPEED_FREQ_LOW 0x00000000U

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void HAL_GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx,
                       uint16_t GPIO_Pin,
                       GPIO_PinState PinState);

#endif
