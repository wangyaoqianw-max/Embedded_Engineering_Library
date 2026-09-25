/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_mcu_irq.h
 * @brief Platform MCU 通用 IRQ 控制接口
 * @author YaoQian Wang
 * @date 2026-09-17
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_MCU_IRQ_H
#define PLATFORM_MCU_IRQ_H

//******************************** Includes *********************************//
#include "platform_error.h"
#include "platform_types.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
/* 这些 IRQ 会调用 FreeRTOS FromISR 接口，优先级不得高于该安全边界。 */
#define PLATFORM_MCU_IRQ_FREERTOS_SAFE_PRIORITY    (5U)
//******************************** Defines **********************************//

//******************************** Types ************************************//
typedef enum
{
    PLATFORM_MCU_IRQ_KEY_EXTI0 = 0,
    PLATFORM_MCU_IRQ_USART1,
    PLATFORM_MCU_IRQ_DMA2_STREAM2,
    PLATFORM_MCU_IRQ_DMA2_STREAM7,
    PLATFORM_MCU_IRQ_MAX
} platform_mcu_irq_id_t;
//******************************** Types ************************************//

//******************************** Functions ********************************//
/**
 * @brief 使能指定 MCU IRQ
 * @param[in] irq : Platform IRQ 标识
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_mcu_irq_enable(platform_mcu_irq_id_t irq);

/**
 * @brief 禁用指定 MCU IRQ
 * @param[in] irq : Platform IRQ 标识
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_mcu_irq_disable(platform_mcu_irq_id_t irq);

/**
 * @brief 设置指定 MCU IRQ 优先级
 * @param[in] irq : Platform IRQ 标识
 * @param[in] priority : CMSIS 优先级编码，取值范围为 5~15
 * @return platform_error_t : 操作结果
 * @note 可能调用 FreeRTOS FromISR 接口的 IRQ 不得设置为高于安全边界的优先级。
 */
platform_error_t platform_mcu_irq_set_priority(platform_mcu_irq_id_t irq,
                                                uint32_t priority);

/**
 * @brief 清除指定 MCU IRQ 的挂起状态
 * @param[in] irq : Platform IRQ 标识
 * @return platform_error_t : 操作结果
 */
platform_error_t platform_mcu_irq_clear_pending(platform_mcu_irq_id_t irq);
//******************************** Functions ********************************//

#endif
