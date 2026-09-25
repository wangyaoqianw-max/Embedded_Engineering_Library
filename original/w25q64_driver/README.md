# W25Q64 Platform Driver

## 资产简介
W25Q64JV SPI NOR Flash Raw Driver，提供初始化、JEDEC ID、状态读取、连续读取、Page Program、跨页写入和 4 KiB Sector Erase。

## 来源与版本
- 来源：`STM32F4_Bootloader_OTA_Test_Project`
- 源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 原路径：`03_Firmware/Application/OTA_APP/03_Platform/platform_bsp/w25q64/`
- 当前文件版本：V1.0

## 许可证
按本仓库 `original/` 的 MIT License 发布。源码旧版权头后续建议统一。

## 目录内容
- `platform_w25q64.c/.h`

## 依赖与适用环境
依赖五层架构 `platform_spi`、`platform_gpio` 和 `platform_time`。容量与命令集按 W25Q64JV 8 MiB、256 B Page、4 KiB Sector 设计。

## 使用方式
由 BSP 先构造 SPI bus/device 与 CS GPIO，再初始化 Flash 对象。写入前由上层负责是否需要擦除。

## 适用边界
Raw Driver 不负责文件系统、磨损均衡、镜像分区管理或掉电事务。

## 已知限制
仅覆盖当前项目需要的基础指令；未实现 Quad SPI、保护寄存器、Suspend/Resume、Chip Erase 等完整功能。

## 验证情况
来源项目完成 External Flash 阶段板测，并被后续固件存储、YMODEM、OTA 流程继续使用。迁移后需重新核对 SPI Mode、最高时钟、CS 时序和 JEDEC ID。

## 修改记录
- 2026-09-25：从 OTA 项目提取为独立存储驱动资产。

## 复用性评审结论
接口和器件职责清晰，适合 W25Q64JV 及兼容指令集场景复用，但不是通用 SPI NOR 抽象层。
