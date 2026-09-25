# AT24C02 Platform Driver

## 资产简介
AT24C02 I2C EEPROM Raw Driver，提供地址探测、连续读取、自动 Page 拆分写入和写周期 ACK Polling。

## 来源与版本
- 来源：`STM32F4_Bootloader_OTA_Test_Project`
- 源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 原路径：`03_Firmware/Application/OTA_APP/03_Platform/platform_bsp/at24c02/`
- 当前文件版本：V1.0

## 许可证
按本仓库 `original/` 的 MIT License 发布。源码旧版权头后续建议统一。

## 目录内容
- `platform_at24c02.c/.h`

## 依赖与适用环境
依赖五层架构 `platform_i2c` 与 `platform_time`。当前容量 256 B、Page 8 B、7-bit 地址范围 0x50~0x57。

## 使用方式
调用者持有并初始化 I2C bus；Driver 不拥有 I2C 生命周期。初始化完成后使用 read/write 访问 EEPROM。

## 适用边界
适合少量配置、状态和元数据持久化，不适合作为大容量高频日志介质。

## 已知限制
未提供磨损均衡、双副本事务或掉电一致性策略，这些应由上层存储服务实现。

## 验证情况
来源项目完成 EEPROM 阶段板测，并在 Firmware Metadata 持久化流程中继续使用。

## 修改记录
- 2026-09-25：从 OTA 项目提取为独立存储驱动资产。

## 复用性评审结论
器件与接口耦合明确，可作为 AT24C02 类 EEPROM 的基础驱动复用。
