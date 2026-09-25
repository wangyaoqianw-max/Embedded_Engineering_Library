/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_mcu_irq.c
 * @brief STM32 CMSIS NVIC Platform MCU IRQ Impl
 * @author YaoQian Wang
 * @date 2026-09-17
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_mcu_irq.h"

#include "platform_def.h"
#include "stm32f4xx_hal.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define STM32_NVIC_PRIORITY_MAX    (15U)
//******************************** Defines **********************************//

//******************************** Declaring *********************************//
static platform_error_t impl_platform_mcu_irq_map(
    platform_mcu_irq_id_t irq,
    IRQn_Type *irqn);
//******************************** Declaring *********************************//

//******************************** Private Functions *************************//
static platform_error_t impl_platform_mcu_irq_map(
    platform_mcu_irq_id_t irq,
    IRQn_Type *irqn)
{
    if (irqn == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    switch (irq) {
        case PLATFORM_MCU_IRQ_KEY_EXTI0:
            *irqn = EXTI0_IRQn;
            return PLATFORM_ERR_OK;

        case PLATFORM_MCU_IRQ_USART1:
            *irqn = USART1_IRQn;
            return PLATFORM_ERR_OK;

        case PLATFORM_MCU_IRQ_DMA2_STREAM2:
            *irqn = DMA2_Stream2_IRQn;
            return PLATFORM_ERR_OK;

        case PLATFORM_MCU_IRQ_DMA2_STREAM7:
            *irqn = DMA2_Stream7_IRQn;
            return PLATFORM_ERR_OK;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_mcu_irq_enable(platform_mcu_irq_id_t irq)
{
    platform_error_t result;
    IRQn_Type irqn;

    result = impl_platform_mcu_irq_map(irq, &irqn);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    NVIC_EnableIRQ(irqn);
    return PLATFORM_ERR_OK;
}

platform_error_t platform_mcu_irq_disable(platform_mcu_irq_id_t irq)
{
    platform_error_t result;
    IRQn_Type irqn;

    result = impl_platform_mcu_irq_map(irq, &irqn);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    NVIC_DisableIRQ(irqn);
    return PLATFORM_ERR_OK;
}

platform_error_t platform_mcu_irq_set_priority(platform_mcu_irq_id_t irq,
                                                uint32_t priority)
{
    platform_error_t result;
    IRQn_Type irqn;

    result = impl_platform_mcu_irq_map(irq, &irqn);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((priority < PLATFORM_MCU_IRQ_FREERTOS_SAFE_PRIORITY) ||
        (priority > STM32_NVIC_PRIORITY_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    NVIC_SetPriority(irqn, priority);
    return PLATFORM_ERR_OK;
}

platform_error_t platform_mcu_irq_clear_pending(platform_mcu_irq_id_t irq)
{
    platform_error_t result;
    IRQn_Type irqn;

    result = impl_platform_mcu_irq_map(irq, &irqn);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    NVIC_ClearPendingIRQ(irqn);
    return PLATFORM_ERR_OK;
}
//******************************** Functions *********************************//
