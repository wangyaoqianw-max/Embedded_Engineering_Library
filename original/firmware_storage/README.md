# Firmware Storage

## 资产简介

面向 A/B Firmware Image 的存储编排核心。通过 `firmware_storage_backend_t` 抽象擦除、Header 读写和 Payload 读写，不再直接持有 W25Q64、AT24C02 或具体地址布局。

## 来源与版本

- 设计来源：`STM32F4_Bootloader_OTA_Test_Project`
- 源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 原实现：`service_firmware/firmware_storage.c/.h`
- Library 版本：V1.0 Backend 抽象版。
- 本次只提取 Image Storage；原工程 Metadata 双副本持久化逻辑未收入本资产。

## 许可证

本资产按本仓库 `original/` 的 MIT License 发布。

## 目录内容

- `firmware_storage.c/.h`：Backend 合同、A/B Slot 选择、容量检查、Header/Payload 读写和流式 CRC 验证。
- `tests/firmware_storage_host_test.c`：内存 Backend Host Test。

## 依赖与适用环境

依赖 `firmware_image`、`common_crc` 和 `platform_common`。Backend 可以由 W25Q64、QSPI NOR、内部 Flash、文件或测试内存实现。

## 使用方式

调用者提供 `firmware_storage_backend_t`，其中声明 Payload Capacity，并实现 erase/read_header/read_payload/write_payload/write_header。Storage Core 负责参数、Slot 和镜像验证逻辑。

## 适用边界

当前核心仍保留 A/B 两个逻辑 Slot，这是固件升级存储策略的一部分；但不包含 Slot 物理基址、Sector 大小或具体 Flash Driver。

## 已知限制

- 仅定义 A/B 两个逻辑 Slot。
- 不负责 Metadata、confirmed/pending/trial/rollback 状态。
- Backend 自身必须保证其擦写粒度、对齐、掉电行为和硬件时序正确。
- 核心只在完成 Payload 后由上层决定何时提交 Header。

## 验证情况

来源工程的 W25Q64 Firmware Storage 和最终 Slot B Image Validation 已通过 Host/板级数据链验证。本资产重构为 Backend 回调后附带新的内存 Backend Host Test；当前会话环境未实际运行该重构测试。

## 修改记录

- 2026-09-25：拆除 W25Q64/AT24C02 直接依赖，建立 Storage Backend 合同；Metadata 逻辑暂不迁入。

## 复用性评审结论

Storage Core 已从“W25Q64 + AT24C02 项目服务”收敛为可替换存储后端的镜像存储编排组件，可与不同 MCU/Flash 组合复用。
