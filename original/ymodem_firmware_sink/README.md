# YMODEM Firmware Sink

## 资产简介

连接 `protocol_ymodem` 与 `firmware_storage` 的适配资产。它把 YMODEM 接收到的完整 `.img` 文件拆成 64 Byte Header 和 Payload，并执行“先验证 Header、再擦除/写 Payload、最后提交 Header”的 Header-last 流程。

## 来源与版本

- 来源：`STM32F4_Bootloader_OTA_Test_Project`
- 源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 原实现：`service_firmware/ota_firmware_sink.c/.h`
- Library 变更：重命名为 `firmware_image_sink`，去除项目文件名缓存和固定 Slot Capacity，改由 `firmware_storage` 提供容量与 Backend。

## 许可证

本资产按本仓库 `original/` 的 MIT License 发布。

## 目录内容

- `firmware_image_sink.c/.h`：YMODEM Sink 合同到 Firmware Storage 的桥接。
- `tests/firmware_image_sink_host_test.c`：Header 分片、Payload 写入和 Header-last 基础 Host Test。

## 依赖与适用环境

依赖 `protocol_ymodem`、`firmware_image`、`firmware_storage` 和 `platform_common`。

## 使用方式

初始化 Storage 后创建 `firmware_image_sink_t`，指定目标 Slot，通过 `firmware_image_sink_get_contract()` 获取 `ymodem_sink_t` 并交给 YMODEM Receiver。

## 适用边界

本资产是 YMODEM 与 Firmware Image Storage 的桥接层，不负责 UART、Flash Driver、OTA 状态机或 Bootloader 安装。

## 已知限制

- 当前只接受 Library Firmware Image V1 格式。
- 仍以 A/B Slot 为目标。
- 失败时不会提交 Header，但 Backend 的 Payload 擦写可能已经发生；真正的掉电事务语义仍由上层 Metadata/Lifecycle 设计保证。

## 验证情况

来源工程同类 Flash Sink 已通过 Host Test，并最终完成 Tera Term → STM32 → Slot B → Image VALID 的板级闭环。Library 版本改用通用 Storage Backend，并附带新的 Host Test；当前会话环境未执行重构后的 GCC 测试。

## 修改记录

- 2026-09-25：从 Production OTA Sink 提取并通用化为 YMODEM Firmware Sink。

## 复用性评审结论

该适配层保持单一职责，可把协议接收和存储实现分别替换，适合作为 OTA 数据入口的可组合组件。
