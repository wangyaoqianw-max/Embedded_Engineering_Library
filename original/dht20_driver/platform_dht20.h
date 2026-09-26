/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_dht20.h
 * @brief Platform DHT20 轻量设备对象和公共接口
 * @author YaoQian Wang
 * @date 2026-09-04
 * @version V1.0
 *
 *****************************************************************************/

#ifndef PLATFORM_DHT20_H
#define PLATFORM_DHT20_H

//******************************** Includes *********************************//
#include "platform_i2c.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
/*首次构造前使用此宏初始化 DHT20 对象存储*/
#define PLATFORM_DHT20_INITIALIZER {0}
//******************************** Defines *********************************//

//******************************** Declaring *******************************//
/*Platform DHT20 轻量对象；I2C 总线由调用者拥有。*/
typedef struct
{
    platform_i2c_t *i2c;
    platform_bool_t initialized;
} platform_dht20_t;

/*单次 DHT20 测量结果。*/
typedef struct
{
    uint8_t status;
    uint32_t rawHumidity;
    uint32_t rawTemperature;
    float humidityPercent;
    float temperatureC;
} platform_dht20_measurement_t;

/**
 * @brief 将 DHT20 对象绑定到已初始化的共享 I2C 总线
 * @param[in,out] dht20 : 使用 PLATFORM_DHT20_INITIALIZER 清零的对象
 * @param[in,out] i2c : 已初始化且由调用者拥有的共享 I2C 对象
 * @return platform_error_t : 初始化结果
 * @note 本函数不发送探测、测量或校准事务。
 * @warning DHT20 不拥有 i2c 生命周期，调用者必须保证引用有效。
 */
platform_error_t platform_dht20_init(
    platform_dht20_t *dht20,
    platform_i2c_t *i2c);

/**
 * @brief 同步触发并读取一次 DHT20 温湿度测量
 * @param[in,out] dht20 : 已初始化的 DHT20 对象
 * @param[out] measurement : 成功时接收完整测量结果
 * @return platform_error_t : 测量结果或底层 I2C 错误
 * @note 本函数在 Task Context 阻塞至少 80 ms。
 * @note 失败时不会修改 measurement 原有内容。
 * @warning 不得从 ISR 或 HAL Callback 调用。
 */
platform_error_t platform_dht20_read(
    platform_dht20_t *dht20,
    platform_dht20_measurement_t *measurement);

/**
 * @brief 解除 DHT20 对共享 I2C 总线的引用
 * @param[in,out] dht20 : 已初始化的 DHT20 对象
 * @return platform_error_t : 反初始化结果
 * @note 本函数不反初始化或释放共享 I2C 总线。
 * @warning 调用者仍负责共享 I2C 总线的最终生命周期。
 */
platform_error_t platform_dht20_deinit(platform_dht20_t *dht20);
//******************************** Declaring *******************************//

#endif
