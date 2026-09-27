# Platform MCU

本目录保存与具体 MCU HAL 解耦的 GPIO、UART、SPI、I2C、IRQ、Reset、Watchdog 公共接口。当前版本来自两个 STM32F4 实际项目中使用过的 Platform 层。

## 依赖与边界
- 依赖同资产中的 `platform_common`。
- GPIO/UART/SPI/I2C 通过 Ops/对象模型由 Impl 层注入具体实现。
- I2C V2.0 只保留 MCU 无关 Contract；Software I2C 协议核心归入 `adapted/software_i2c_core/`，STM32F4 适配位于 `04_Impl/impl_mcu/stm32f4/impl_platform_i2c_soft.*`。后续可按需增加 STM32 HAL Hardware I2C Backend。
- IRQ、Reset、Watchdog 当前主要是抽象接口，具体行为由 Impl 层提供。

## 验证
相关接口已在 DMA UART 与 OTA 项目中实际使用；不同子模块的验证深度不同，不能仅凭收录认定在所有 MCU/HAL 上已验证。
