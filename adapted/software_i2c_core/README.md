# Software I2C Core

## 资产简介

该资产是一个与 MCU、HAL、RTOS 解耦的软件 I2C Bit-bang Core。

它不直接访问 GPIO，不调用 STM32 HAL，也不依赖 Platform 层；同时不占用目标工程的 `uint8_t/uint16_t/uint32_t` 名称空间。所有底层动作通过 Port Callback 注入，因此可以由 STM32F4 Impl、其他 MCU Impl 或 Host Test Fake Port 复用。

## 来源与版本

参考并改编自 RT-Thread Software I2C Bit Ops：

- Upstream: `RT-Thread/rt-thread`
- 参考文件：
  - `components/drivers/i2c/dev_i2c_bit_ops.c`
  - `components/drivers/include/drivers/dev_i2c_bit_ops.h`
- 参考提交：`05badce954fc77fc2eb3a41ca785d3a35627ffde`
- 上游版权：RT-Thread Development Team
- 上游许可证：Apache License 2.0

本资产不是 RT-Thread 文件的原样副本。它保留了 RT-Thread 的 bit-ops/port-callback 思路，并结合本仓库已有 Software I2C 的 Clock Stretch、Bus Recovery 和错误清理语义进行了重新组织。

## 许可证

Apache License 2.0。详见本目录 `LICENSE`。

源码保留 RT-Thread 上游版权与本仓库修改版权声明。

## 目录内容

```text
software_i2c_core/
├─ software_i2c_core.h
├─ software_i2c_core.c
├─ LICENSE
└─ README.md
```

## Port Contract

Core 只要求：

```c
set_sda()
set_scl()
get_sda()
get_scl()
delay_us()
```

其中写 HIGH 的语义是“释放开漏线路”，不是主动推挽输出高电平。

Port Callback 的返回值只表达底层动作是否执行成功；Core 自己负责协议状态和错误分类。

## 已有能力

- 7-bit Address
- START / STOP
- Repeated START
- ACK / NACK
- Multi-byte Write / Read
- Write-Read
- Clock Stretching 等待
- SCL Timeout
- 初始化阶段 9-clock Bus Recovery
- 事务失败后的 Best-effort STOP / Release
- Address NACK 与 Data NACK 区分

## 与 RT-Thread 上游的主要差异

移除了：

- `rt_i2c_bus_device`
- `rt_i2c_msg`
- `rt_tick_get()`
- RT-Thread Log
- RT-Thread Device Registration
- `rt_*` 类型系统
- 10-bit Address 与 RT-Thread Flag 体系

改为：

- C99 `stdbool.h` + 资产私有 `software_i2c_u8_t/u16_t/u32_t`，并做编译期位宽检查
- 以微秒为单位的 SCL Timeout
- 独立同步 API：`probe/write/read/write_read`
- 端口操作失败显式映射为 Core I/O Error
- 保留并强化初始化 Bus Recovery

## 在五层工程中的位置

Library 中该资产归类为：

```text
adapted/software_i2c_core/
```

集成到实际 Firmware 工程时，建议作为：

```text
05_Vendors/software_i2c_core/
```

由 `04_Impl` 提供 MCU Port Adapter。Platform / Service 不应直接依赖该 Core。

## 适用边界

- 当前仅支持单 Master、7-bit Address。
- 当前为同步阻塞实现。
- 不包含 Mutex，多 Task 共用 Bus 时由更高层处理串行化。
- 时序精度由注入的 `delay_us` 和 GPIO 实现决定。
- Fast-mode 是否满足真实波形要求必须在目标 MCU 上用逻辑分析仪验证。

## 验证情况

上游 RT-Thread 实现属于长期维护的软件 I2C 方案，但本资产已经做了实质改编，不能继承上游测试结论。

当前版本需要重新执行：

- Host Test
- 编译器 Warning 检查
- STM32F4 Build
- Logic Analyzer START/STOP/ACK/Repeated START
- Clock Stretch / Bus Recovery 定向测试

## 修改记录

- 2026-09-27：首次建立独立 Core；移除 RT-Thread Runtime 依赖，并引入本仓库现有 Software I2C 的 Timeout/Recovery/Cleanup 语义。

## 复用性评审结论

该 Core 的价值在于把 I2C 协议时序与具体 GPIO/HAL 完全分开。对五层架构而言，它应作为 Vendor/Adapted 算法组件，由 Impl 适配，而不是成为 Platform 的具体实现。
