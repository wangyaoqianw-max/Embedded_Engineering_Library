# Platform I2C

## 资产定位

本目录只定义 MCU 无关的同步 I2C 能力契约。

Platform 不实现 Software I2C 时序，也不直接依赖 STM32 HAL。具体 Backend 由 `platform_i2c_ops_t` 注入。

```text
Sensor / EEPROM Driver
        │
        ▼
Platform I2C Contract
        │
        ▼
platform_i2c_ops_t
        │
        ▼
04_Impl
   ├─ STM32F4 Software I2C Adapter
   │        │
   │        ▼
   │   Software I2C Core
   │   (adapted / Vendor)
   │
   └─ STM32 HAL Hardware I2C Adapter
       （后续按实际项目需要实现）
```

## Platform 职责

Platform 只负责：

- I2C 总线对象与 Backend 绑定。
- 7-bit 地址公共参数检查。
- Write / Read / Write-Read 公共参数检查。
- Backend Ops 分发。
- 初始化状态与解除绑定。

Platform 不负责：

- GPIO。
- START / STOP。
- ACK / NACK。
- Clock Stretch。
- Bus Recovery。
- STM32 HAL。
- Software I2C Timing。

## Software I2C

Software I2C 协议核心不再放在 Platform 或 Impl 中。

Library 中的改编资产位于：

```text
adapted/software_i2c_core/
```

实际五层 Firmware 工程中建议将它作为：

```text
05_Vendors/software_i2c_core/
```

STM32F4 Adapter 位于：

```text
04_Impl/impl_mcu/stm32f4/impl_platform_i2c_soft.c/.h
```

Adapter 负责：

- 配置 STM32 GPIO Open-Drain。
- 将 GPIO Port/Pin 映射到 Software I2C Port Callback。
- 注入 DWT 微秒延时。
- 将 Software I2C Error 映射为 `platform_error_t`。
- 将 `platform_i2c_ops_t` 映射到 Software I2C Core。

因此 Vendor/Core 不依赖 Platform，Impl 也不会通过 Platform GPIO 再绕回 Impl。

## 使用示例

```c
platform_i2c_t i2c = PLATFORM_I2C_INITIALIZER;
impl_platform_i2c_soft_context_t context =
    IMPL_PLATFORM_I2C_SOFT_CONTEXT_INITIALIZER;

const impl_platform_i2c_soft_config_t config = {
    .sclPort = GPIOB,
    .sclPin = GPIO_PIN_6,
    .sdaPort = GPIOB,
    .sdaPin = GPIO_PIN_7,
    .halfPeriodUs = 5U,
    .sclTimeoutUs = 100U
};

impl_platform_i2c_soft_construct(
    &i2c,
    "sensor_i2c",
    &config,
    &context);
```

调用者必须在构造前开启对应 GPIO Port Clock。

DHT20、MPU6050、AT24C02 等上层驱动继续只依赖：

```text
platform_i2c_probe()
platform_i2c_write()
platform_i2c_read()
platform_i2c_write_read()
```

## Hardware I2C

后续若项目需要 STM32 Hardware I2C，应新增独立 Backend，例如：

```text
04_Impl/impl_mcu/stm32f4/
├─ impl_platform_i2c_soft.c
└─ impl_platform_i2c_hal.c
```

二者都实现 `platform_i2c_ops_t`。

不应在 Platform 层增加 `if (software/hardware)` 分支。

## 验证边界

Platform Contract Host Test 已改为 Fake Backend，只验证 Platform 参数检查、Ops 分发和生命周期绑定。

Software I2C Core 与 STM32F4 Adapter 属于新的重构边界，需要重新执行 Host Test、Keil Build 和逻辑分析仪验证。
