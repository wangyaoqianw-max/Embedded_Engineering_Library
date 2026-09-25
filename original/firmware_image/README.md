# Firmware Image

## 资产简介

定义并实现固定 64 Byte、little-endian 的 Firmware Image Header V1，以及三段式 Firmware Version 比较、Header 编解码和 Header CRC 校验。

## 来源与版本

- 来源：`STM32F4_Bootloader_OTA_Test_Project`
- 源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 原路径：`03_Firmware/Application/OTA_APP/02_Service/service_firmware/firmware_def.* / firmware_image.* / firmware_version.*`
- 原始实现版本：V1.0
- Library 变更：移除 Slot A/B 地址和容量定义；Image Core 不再决定目标存储容量。

## 许可证

按本仓库 `original/` 的 MIT License 发布。部分迁移源码仍保留旧版权头，后续应统一。

## 目录内容

- `firmware_def.h`：Header V1、版本和验证结果数据合同。
- `firmware_image.c/.h`：固定偏移编解码与 Header 校验。
- `firmware_version.c/.h`：版本合法性和 major/minor/patch 比较。
- `tests/firmware_image_host_test.c`：格式与版本 Host Test。

## 依赖与适用环境

依赖 `common_crc` 的 CRC-32/ISO-HDLC，以及 `platform_common` 基础类型。

## 使用方式

Packer 或 MCU 侧先计算 Payload CRC32，再填充版本、imageSize 和 payloadCrc32，调用 `firmware_image_encode_header()` 生成固定 Header。读取端先校验 Header，再由存储层校验 Payload CRC。

## 适用边界

Image Core 只描述“固件文件是什么”，不描述固件应该写入哪个 Slot、哪个 Flash 地址，也不决定设备最大固件容量。

## 已知限制

- V1 Header 只有 CRC32，没有签名、HMAC、Hash 或加密字段。
- imageSize 只要求非零；目标容量约束由 Firmware Storage 或产品策略负责。
- 当前格式固定 little-endian 和 64 Byte Header。

## 验证情况

来源工程的 Firmware Format Host Test 和后续 YMODEM/OTA 流程均已使用该格式。Library 版本新增去 Slot 容量耦合的 Host Test，但当前会话环境未实际运行重构后的 GCC 测试。

## 修改记录

- 2026-09-25：从 OTA 工程提取；将 Image Format 与 A/B Slot 布局解耦。

## 复用性评审结论

与硬件、RTOS 和具体存储介质耦合低，可作为固件打包、Bootloader、OTA 和离线验证工具共同使用的数据合同。
