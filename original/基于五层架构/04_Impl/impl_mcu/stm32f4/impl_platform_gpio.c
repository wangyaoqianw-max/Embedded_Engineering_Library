/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file impl_platform_gpio.c
 * @brief STM32 通用 GPIO 的 Platform Impl 构造与上下文绑定
 * @author YaoQian Wang
 * @date 2026-09-01
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "impl_platform_gpio.h"

#include "platform_def.h"

#include <stddef.h>
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define STM32_GPIO_DEFAULT_SPEED GPIO_SPEED_FREQ_LOW
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
static platform_bool_t stm32_gpio_is_single_pin(uint16_t pin);
static platform_error_t stm32_gpio_get_context(
    platform_gpio_t *gpio,
    impl_platform_gpio_context_t **context);
static platform_error_t stm32_gpio_map_mode(
    const platform_gpio_config_t *config,
    uint32_t *mode);
static platform_error_t stm32_gpio_map_pull(
    platform_gpio_pull_t pull,
    uint32_t *halPull);
static GPIO_PinState stm32_gpio_map_level(platform_gpio_level_t level);
static platform_error_t stm32_gpio_configure(
    platform_gpio_t *gpio,
    const platform_gpio_config_t *config);
static platform_error_t stm32_gpio_write(
    platform_gpio_t *gpio,
    platform_gpio_level_t level);
static platform_error_t stm32_gpio_read(
    platform_gpio_t *gpio,
    platform_gpio_level_t *level);
static platform_error_t stm32_gpio_deinit(platform_gpio_t *gpio);
//******************************** Declaring *********************************//

//******************************** Constants *********************************//
static const platform_gpio_ops_t g_stm32GpioOps = {
    stm32_gpio_configure,
    stm32_gpio_write,
    stm32_gpio_read,
    stm32_gpio_deinit
};
//******************************** Constants *********************************//

//******************************** Private Functions *************************//
static platform_bool_t stm32_gpio_is_single_pin(uint16_t pin)
{
    if (pin == 0U) {
        return PLATFORM_FALSE;
    }

    return (((uint16_t)(pin & (uint16_t)(pin - 1U))) == 0U) ?
           PLATFORM_TRUE : PLATFORM_FALSE;
}

static platform_error_t stm32_gpio_get_context(
    platform_gpio_t *gpio,
    impl_platform_gpio_context_t **context)
{
    if ((gpio == NULL) || (context == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (gpio->implContext == NULL) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    *context = (impl_platform_gpio_context_t *)gpio->implContext;
    if ((*context)->port == NULL) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (stm32_gpio_is_single_pin((*context)->pin) != PLATFORM_TRUE) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_gpio_map_mode(
    const platform_gpio_config_t *config,
    uint32_t *mode)
{
    if ((config == NULL) || (mode == NULL)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (config->direction == PLATFORM_GPIO_DIRECTION_INPUT) {
        *mode = GPIO_MODE_INPUT;
        return PLATFORM_ERR_OK;
    }

    if (config->direction != PLATFORM_GPIO_DIRECTION_OUTPUT) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (config->outputType == PLATFORM_GPIO_OUTPUT_PUSH_PULL) {
        *mode = GPIO_MODE_OUTPUT_PP;
        return PLATFORM_ERR_OK;
    }

    if (config->outputType == PLATFORM_GPIO_OUTPUT_OPEN_DRAIN) {
        *mode = GPIO_MODE_OUTPUT_OD;
        return PLATFORM_ERR_OK;
    }

    return PLATFORM_ERR_INVALID_PARAM;
}

static platform_error_t stm32_gpio_map_pull(
    platform_gpio_pull_t pull,
    uint32_t *halPull)
{
    if (halPull == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    switch (pull) {
        case PLATFORM_GPIO_PULL_NONE:
            *halPull = GPIO_NOPULL;
            return PLATFORM_ERR_OK;

        case PLATFORM_GPIO_PULL_UP:
            *halPull = GPIO_PULLUP;
            return PLATFORM_ERR_OK;

        case PLATFORM_GPIO_PULL_DOWN:
            *halPull = GPIO_PULLDOWN;
            return PLATFORM_ERR_OK;

        default:
            return PLATFORM_ERR_INVALID_PARAM;
    }
}

static GPIO_PinState stm32_gpio_map_level(platform_gpio_level_t level)
{
    return (level == PLATFORM_GPIO_LEVEL_HIGH) ?
           GPIO_PIN_SET : GPIO_PIN_RESET;
}

static platform_error_t stm32_gpio_configure(
    platform_gpio_t *gpio,
    const platform_gpio_config_t *config)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_gpio_context_t *context = NULL;
    GPIO_InitTypeDef halConfig = {0};

    if (config == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = stm32_gpio_get_context(gpio, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_gpio_map_mode(config, &halConfig.Mode);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = stm32_gpio_map_pull(config->pull, &halConfig.Pull);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    halConfig.Pin = context->pin;
    halConfig.Speed = STM32_GPIO_DEFAULT_SPEED;
    halConfig.Alternate = 0U;

    if (config->direction == PLATFORM_GPIO_DIRECTION_OUTPUT) {
        HAL_GPIO_WritePin(context->port,
                          context->pin,
                          stm32_gpio_map_level(config->initialLevel));
    }

    HAL_GPIO_Init(context->port, &halConfig);

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_gpio_write(
    platform_gpio_t *gpio,
    platform_gpio_level_t level)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_gpio_context_t *context = NULL;

    result = stm32_gpio_get_context(gpio, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    HAL_GPIO_WritePin(context->port,
                      context->pin,
                      stm32_gpio_map_level(level));

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_gpio_read(
    platform_gpio_t *gpio,
    platform_gpio_level_t *level)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_gpio_context_t *context = NULL;
    GPIO_PinState halLevel;

    if (level == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = stm32_gpio_get_context(gpio, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    halLevel = HAL_GPIO_ReadPin(context->port, context->pin);
    *level = (halLevel == GPIO_PIN_SET) ?
             PLATFORM_GPIO_LEVEL_HIGH : PLATFORM_GPIO_LEVEL_LOW;

    return PLATFORM_ERR_OK;
}

static platform_error_t stm32_gpio_deinit(platform_gpio_t *gpio)
{
    platform_error_t result = PLATFORM_ERR_OK;
    impl_platform_gpio_context_t *context = NULL;

    result = stm32_gpio_get_context(gpio, &context);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    HAL_GPIO_DeInit(context->port, context->pin);

    return PLATFORM_ERR_OK;
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t impl_platform_gpio_construct(
    platform_gpio_t *gpio,
    const char *name,
    impl_platform_gpio_context_t *context)
{
    platform_gpio_init_params_t params;

    if ((gpio == NULL) || (context == NULL) ||
        (context->port == NULL) ||
        (stm32_gpio_is_single_pin(context->pin) != PLATFORM_TRUE)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    params.name = name;
    params.ops = &g_stm32GpioOps;
    params.implContext = context;

    return platform_gpio_init(gpio, &params);
}
//******************************** Functions *********************************//
