# STM32F4 Impl Tests

## Upstream Reference

本目录保留来源工程已经执行过的 GPIO、SPI、UART 和 DWT Delay Host Test 及其最小 Fake Header，路径位于：

```text
tests/upstream_reference/
├─ gpio/
├─ spi/
├─ uart/
└─ delay/
```

这些文件是“来源验证证据”，不是 Library 当前 API 的直接 Runner。

原因是 Library 在沉淀时对 SPI/UART 做了接口重构：

- SPI：`impl_platform_spi1_construct()/spi2_construct()` → `impl_platform_spi_construct(..., context)`
- UART：`impl_platform_uart_usart1_construct()` → `impl_platform_uart_construct(..., context)`
- CubeMX 全局 `hspi1/hspi2/huart1` 不再由 Impl 直接引用

因此原 SPI/UART Test Fixture 仍记录旧 API，不能把它们直接视为 Library 重构版 PASS。

## 后续统一 Runner 应覆盖

1. GPIO 单 Pin Context、输入/输出、Pull、初始电平、Read/Write/DeInit。
2. SPI Handle 注入、Lifecycle、Mode/BitOrder/DataSize/Clock 校验、HAL Status 映射、65535 Byte 分块边界。
3. UART 多 Context Registry、DMA TX、Receive-To-Idle、Circular position wrap、Cancel、Error Callback。
4. DWT 0 us、Cycle Counter Enable、CYCCNT Wrap 安全边界。
5. IRQ Enable/Disable/Priority/Pending。
6. Reset 调用 CMSIS SystemReset。
7. Watchdog timeout/reload 边界与未启动 Feed。
8. `platform_delay_ms()` 的 HAL Tick 转发。

## 验证状态

- 来源工程旧接口 Host Test：PASS。
- Library GPIO/IRQ/Reset/Watchdog/Delay 迁移：静态边界检查待统一构建。
- Library SPI/UART Handle 注入重构：需要新的 Host Test Runner 后才能标记 PASS。
