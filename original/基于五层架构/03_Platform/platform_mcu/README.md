# Platform MCU

本目录保存与具体 MCU HAL 解耦的 GPIO、UART、SPI、I2C、IRQ、Reset、Watchdog 公共接口。当前版本来自两个 STM32F4 实际项目中使用过的 Platform 层。

## 依赖与边界
- 依赖同资产中的 `platform_common`。
- GPIO/UART/SPI 通过 Ops/对象模型由 Impl 层注入具体实现。
- 当前 `platform_i2c.c` 是 GPIO bit-bang Software I2C，并仍引用项目级 `project_config.h` 中的时序参数，因此尚未完全参数化；迁移新项目时必须提供对应配置或先完成配置解耦。
- IRQ、Reset、Watchdog 当前主要是抽象接口，具体行为由 Impl 层提供。

## 验证
相关接口已在 DMA UART 与 OTA 项目中实际使用；不同子模块的验证深度不同，不能仅凭收录认定在所有 MCU/HAL 上已验证。
