# Platform I2C

## 资产定位

本目录只定义 MCU 无关的同步 I2C 能力契约，不再实现 GPIO Bit-bang 时序。

Platform 对上提供稳定接口：

```text
platform_i2c_probe()
platform_i2c_write()
platform_i2c_read()
platform_i2c_write_read()
platform_i2c_deinit()
```

具体如何完成 I2C 事务，由 Backend 通过 `platform_i2c_ops_t` 实现。

## V2.0 架构

```text
Sensor / EEPROM Driver
        │
        ▼
Platform I2C Contract
        │
        ▼
platform_i2c_ops_t
        │
   ┌────┴─────────────┐
   ▼                  ▼
Software I2C       Hardware I2C
GPIO Bit-bang      STM32 HAL 等
当前已实现          后续按需增加
```

当前 Software I2C Backend 位于：

```text
04_Impl/impl_bus/software_i2c/
```

## Platform 职责

Platform 仅负责：

- I2C 总线对象与 Backend 绑定。
- 7-bit 地址公共参数检查。
- Write / Read / Write-Read 公共参数检查。
- Backend Ops 分发。
- 初始化状态与解除绑定。

Platform 不负责：

- START / STOP。
- ACK / NACK。
- GPIO Open Drain。
- Clock Stretching。
- Bit Timing。
- 9-clock Bus Recovery。
- STM32 HAL / LL 调用。

上述内容均属于具体 Backend。

## Software I2C 迁移

V1.x：

```c
platform_i2c_init(&i2c, "sensor_i2c", &scl, &sda);
```

V2.0：

```c
impl_platform_i2c_soft_context_t context = {0};

impl_platform_i2c_soft_construct(
    &i2c,
    "sensor_i2c",
    &scl,
    &sda,
    &context);
```

自定义时序：

```c
const impl_platform_i2c_soft_config_t config = {
    .halfPeriodUs = 5U,
    .sclTimeoutUs = 100U
};

impl_platform_i2c_soft_construct_with_config(
    &i2c,
    "sensor_i2c",
    &scl,
    &sda,
    &config,
    &context);
```

DHT20、MPU6050、AT24C02 等上层驱动继续只依赖 `platform_i2c_t` 和 `platform_i2c_write/read/write_read`，无需知道底层是 Software I2C 还是 Hardware I2C。

## 后续扩展

如实际项目需要 STM32 Hardware I2C，再新增：

```text
04_Impl/impl_mcu/stm32f4/impl_platform_i2c_hal.c/.h
```

并实现同一组 `platform_i2c_ops_t`，不修改传感器和 EEPROM 驱动。

## 验证边界

原 Software I2C Host Test 已迁移到新的 Backend 构造入口。V2.0 重构后需要重新执行 Host Test 与目标板集成测试后，才能把新版本标记为完整 PASS。
