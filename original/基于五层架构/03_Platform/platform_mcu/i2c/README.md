# Platform Software I2C

## 资产简介

该模块提供基于 Platform GPIO 的同步 Software I2C，实现 7-bit Address、Write、Read、Repeated START Write-Read、ACK/NACK、Clock Stretching 等待和总线恢复。

## V1.1 变化

来源工程原实现直接依赖：

```text
project_config.h
PROJECT_SOFT_I2C_HALF_PERIOD_US
PROJECT_SOFT_I2C_SCL_TIMEOUT_US
```

Library V1.1 改为实例级配置：

```c
typedef struct
{
    uint32_t halfPeriodUs;
    uint32_t sclTimeoutUs;
} platform_i2c_config_t;
```

并新增：

```c
platform_i2c_init_with_config(...)
```

原 `platform_i2c_init()` 继续保留，默认使用：

```text
halfPeriodUs = 5
sclTimeoutUs = 100
```

因此旧项目可以无修改继续使用，同时新项目可以为不同 Software I2C Bus 设置不同 Timing。

## 典型使用

```c
platform_i2c_config_t config = {
    .halfPeriodUs = 5U,
    .sclTimeoutUs = 100U
};

platform_i2c_init_with_config(
    &i2c,
    "sensor_i2c",
    &scl,
    &sda,
    &config);
```

## 当前 Backend

当前 `platform_i2c_t` 的实现本质上是 GPIO Bit-bang Software I2C。

它依赖：

- Platform GPIO
- `platform_delay_us()`

它不是 STM32 HAL Hardware I2C Wrapper。

长期演进可以进一步形成：

```text
I2C API
├─ Software GPIO Backend
└─ STM32 HAL Hardware I2C Backend
```

当前版本暂不重构到多 Backend，以避免扩大变更面。

## 已有能力

- START / STOP
- 7-bit Address
- Address ACK/NACK
- Multi-byte Write
- Multi-byte Read
- Repeated START
- Final-byte NACK
- Clock Stretching wait
- 9-clock bus recovery
- Transaction failure cleanup
- Probe
- Deinit

## 验证

来源工程 Phase 3 记录：

- Host Test PASS
- Coding Standard Review PASS
- Keil Full Rebuild PASS
- DWT us delay target integration PASS
- Serial Assistant smoke PASS
- RTT smoke PASS
- Logic Analyzer START / STOP PASS
- Logic Analyzer Address / ACK PASS
- Logic Analyzer Repeated START PASS
- Logic Analyzer Read / Write transaction PASS

Library V1.1 已更新 Host Test 源码，新增实例级 Timing 配置测试，但当前仓库尚未建立统一 Runner，因此 Library 重构版仍需重新执行。

## 已知限制

- GPIO 必须正确支持 Open Drain / release-high 语义。
- 微秒 Timing 依赖底层 delay 精度。
- 当前没有 Mutex；多 Task 共享同一 Bus 时需要由上层串行化或以后增加 Bus Lock。
- 当前 API 名称仍是 `platform_i2c`，但实现仅为 Software I2C；后续如果加入 Hardware I2C，需要进一步抽象 Backend。
