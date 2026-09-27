# Software I2C Backend

## 资产简介

本目录实现 `platform_i2c_ops_t` 的 GPIO Bit-bang Backend。

它由原 `03_Platform/platform_mcu/i2c/platform_i2c.c` 中的软件 I2C 实现迁移而来，使 Platform I2C 不再绑定某一种底层实现。

## 实现能力

- 7-bit Address
- START / STOP
- ACK / NACK
- Multi-byte Write / Read
- Repeated START Write-Read
- Clock Stretching 等待
- 9-clock Bus Recovery
- 事务失败后的 Best-effort Cleanup
- 实例级 Half Period / SCL Timeout

## 依赖

- `platform_i2c.h`
- `platform_gpio.h`
- `platform_delay_us()`

SCL/SDA GPIO 由调用者创建并持有，Backend 仅保存非拥有型引用。

## 使用方式

```c
platform_i2c_t i2c = PLATFORM_I2C_INITIALIZER;
impl_platform_i2c_soft_context_t context = {0};

impl_platform_i2c_soft_construct(
    &i2c,
    "sensor_i2c",
    &scl,
    &sda,
    &context);
```

## 边界

该 Backend 只解决 Software I2C。Hardware I2C 不应在本目录继续增加条件分支，而应实现新的 Backend。

当前没有 Bus Mutex；多 Task 共用同一 I2C Bus 时仍由上层串行化或后续独立增加同步策略。

## 验证

原 Software I2C Host Test 已调整为通过 `impl_platform_i2c_soft_construct*` 构造。Library V2.0 重构后的实际 Host Test / Target Test 结果应以重新执行后的记录为准。
