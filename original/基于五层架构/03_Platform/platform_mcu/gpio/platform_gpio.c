/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_gpio.c
 * @brief Platform GPIO 抽象接口实现
 * @author YaoQian Wang
 * @date 2026-09-01
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_gpio.h"

#include <stddef.h>
//******************************** Includes *********************************//

//******************************** Functions *********************************//
/**
 * @brief 校验 GPIO 配置的公共枚举范围
 * @param[in] config : GPIO 配置
 * @return 配置校验结果
 */
static platform_error_t platform_gpio_validate_config(
    const platform_gpio_config_t *config)
{
    if (config == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (((int32_t)config->direction < (int32_t)PLATFORM_GPIO_DIRECTION_INPUT) ||
        (config->direction >= PLATFORM_GPIO_DIRECTION_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (((int32_t)config->pull < (int32_t)PLATFORM_GPIO_PULL_NONE) ||
        (config->pull >= PLATFORM_GPIO_PULL_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (((int32_t)config->outputType < (int32_t)PLATFORM_GPIO_OUTPUT_PUSH_PULL) ||
        (config->outputType >= PLATFORM_GPIO_OUTPUT_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (((int32_t)config->initialLevel < (int32_t)PLATFORM_GPIO_LEVEL_LOW) ||
        (config->initialLevel >= PLATFORM_GPIO_LEVEL_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_gpio_init(
    platform_gpio_t *gpio,
    const platform_gpio_init_params_t *params)
{
    if (gpio == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (params == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (gpio->initialized != 0U) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    if (params->ops == NULL) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    gpio->name = params->name;
    gpio->ops = params->ops;
    gpio->implContext = params->implContext;
    gpio->initialized = 1U;
    gpio->configured = 0U;

    return PLATFORM_ERR_OK;
}

platform_error_t platform_gpio_configure(
    platform_gpio_t *gpio,
    const platform_gpio_config_t *config)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (gpio == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (gpio->initialized == 0U) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    result = platform_gpio_validate_config(config);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((gpio->ops == NULL) || (gpio->ops->configure == NULL)) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    result = gpio->ops->configure(gpio, config);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    gpio->config = *config;
    gpio->configured = 1U;

    return PLATFORM_ERR_OK;
}

platform_error_t platform_gpio_write(
    platform_gpio_t *gpio,
    platform_gpio_level_t level)
{
    if (gpio == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (gpio->initialized == 0U) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (gpio->configured == 0U) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (gpio->config.direction != PLATFORM_GPIO_DIRECTION_OUTPUT) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (((int32_t)level < (int32_t)PLATFORM_GPIO_LEVEL_LOW) ||
        (level >= PLATFORM_GPIO_LEVEL_MAX)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((gpio->ops == NULL) || (gpio->ops->write == NULL)) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return gpio->ops->write(gpio, level);
}

platform_error_t platform_gpio_read(
    platform_gpio_t *gpio,
    platform_gpio_level_t *level)
{
    if (gpio == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (gpio->initialized == 0U) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (gpio->configured == 0U) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if (level == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if ((gpio->ops == NULL) || (gpio->ops->read == NULL)) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    return gpio->ops->read(gpio, level);
}

platform_error_t platform_gpio_deinit(platform_gpio_t *gpio)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if (gpio == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (gpio->initialized == 0U) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    if (gpio->configured == 0U) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    if ((gpio->ops == NULL) || (gpio->ops->deinit == NULL)) {
        return PLATFORM_ERR_NOT_SUPPORTED;
    }

    result = gpio->ops->deinit(gpio);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    gpio->configured = 0U;

    return PLATFORM_ERR_OK;
}
//******************************** Functions *********************************//
