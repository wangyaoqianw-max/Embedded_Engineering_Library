# A/B OTA Solution

## 资产简介

本目录整合一套可跨项目复用的 A/B 分区 OTA 方案。它把固件传输、镜像格式、外部存储、YMODEM 到固件存储的桥接、Metadata 双副本事务以及 Trial / Confirm / Rollback 生命周期集中在一个方案文件夹内，同时保持内部模块独立。

本资产不是完整 STM32 工程，也不包含 Bootloader 跳转、Internal Flash 具体擦写、RTOS Task、UI、看门狗或项目级 Health 策略。

## 目录结构

```text
ab_ota_solution/
├─ protocol_ymodem/
├─ firmware_image/
├─ firmware_storage/
├─ ymodem_firmware_sink/
├─ ab_firmware_lifecycle/
└─ README.md
```

各子目录仍保留自己的 README、源码和 Host Test，便于单独复用和评审。

## 方案数据链

```text
UART / USB CDC / other byte stream
        │
        ▼
ymodem_transport_t
        │
        ▼
Protocol YMODEM
        │
        ▼
ymodem_sink_t
        │
        ▼
YMODEM Firmware Sink
        │
        ▼
Firmware Storage
        │
        ▼
firmware_storage_backend_t
        │
        ▼
External Flash / Internal Flash / File Backend
```

镜像接收完成后：

```text
Candidate Image VALID
        │
        ▼
request_upgrade()
        │
        ▼
PENDING
        │
        ▼
Bootloader installs candidate
        │
        ▼
TRIAL
   ┌────┴────┐
   │         │
Confirm   next boot without confirm
   │         │
   ▼         ▼
 NONE     ROLLBACK
             │
             ▼
        restore confirmed
             │
             ▼
            NONE
```

## 子模块职责

### protocol_ymodem

负责 Single-file YMODEM Parser / Receiver。通过 Transport 回调发送 ACK / NAK / CAN / C，通过 Sink 回调输出文件数据，不直接依赖 UART Service、RTOS 或 Flash。

### firmware_image

定义 64 Byte Firmware Image Header V1、Version、Header CRC32 和 Payload CRC32 数据合同。它只描述“固件镜像是什么”，不决定 Slot 地址和目标容量。

### firmware_storage

提供 A/B 逻辑 Slot 和通用 Storage Backend。它负责 Header / Payload 读写、容量检查和流式 Payload CRC 验证，不直接依赖 W25Q64。

### ymodem_firmware_sink

连接 YMODEM 和 Firmware Storage，执行 Header-first validation、Payload 写入和 Header-last commit，避免未完整接收的 Payload 被误认为有效镜像。

### ab_firmware_lifecycle

负责 Metadata V2、双副本原子事务、PENDING / TRIAL / ROLLBACK / NONE 状态机、Strict Confirm 和 Boot Decision。核心不依赖 AT24C02；AT24C02 通过可选 Adapter 接入。

## 外部依赖

本方案复用 Library 中其他通用资产，而不在本目录重复保存：

- `original/common_crc/`
- `original/基于五层架构/03_Platform/platform_common/`
- 可选：`original/uart_service/`
- 可选：`original/w25q64_driver/`
- 可选：`original/at24c02_driver/`

因此本目录保存的是 OTA 方案核心，不把 UART、SPI NOR、EEPROM 等通用 Driver 再复制一份。

## 典型集成顺序

1. 接入 `firmware_image` 和 `common_crc`，固定镜像格式。
2. 实现 `firmware_storage_backend_t`，建立 A/B Slot。
3. 接入 `protocol_ymodem` 的 Transport。
4. 使用 `ymodem_firmware_sink` 将接收数据写入目标 Slot。
5. 下载完成并验证 Image 后，将目标 Slot 标记为 VALID。
6. 调用 `firmware_lifecycle_request_upgrade()` 提交 PENDING。
7. Bootloader 根据 `firmware_boot_decision_get_action()` 决定安装或恢复。
8. Candidate 安装完成后提交 TRIAL。
9. Application 达到项目 Health 条件后执行 Strict Confirm。
10. 未确认的 Trial 在下一次启动进入 Rollback，并从 confirmed Slot 恢复。

## 项目层仍需实现

本方案有意不包含以下内容：

- STM32 Internal Flash 擦除和写入。
- MSP / Reset_Handler / VTOR 检查与跳转。
- 具体 Bootloader 主循环。
- Candidate 安装器。
- confirmed image 恢复器。
- Watchdog、Reset Cause 和 Fault Handler。
- 自动确认时间窗、任务心跳等 Health Policy。
- 蓝牙、USB、网络等 OTA Transport 编排。
- 产品 UI 和用户交互。

这些能力应在项目层或后续独立资产中实现。

## 验证边界

本方案的核心来源于 `STM32F4_Bootloader_OTA_Test_Project`。来源项目完成了 YMODEM、Firmware Image、W25Q64 Firmware Storage、Metadata、Trial / Confirm / Rollback 等 Host/Contract/Build 验证，并取得部分真实 STM32F411 板级闭环证据。

来源 S10 最终记录中 Software Verification 为 PASS，Hardware Verification 为 PARTIAL / DEFERRED FOLLOW-UP；本目录保留这一证据边界，不将其表述为完整硬件验证 PASS。

Library 通用化后，各子模块已经保存对应 Host Test，但当前尚未建立统一 CI / Host Test Runner，因此重构后的整体方案仍需要在 Library 内再次完成统一编译和自动测试。

## 许可证

本目录属于 `original/`，采用本仓库 `original/LICENSE` 的 MIT License。部分早期迁移源码文件头仍保留旧的 `All Rights Reserved` 表达，后续应统一源码版权头。

## 修改记录

- 2026-09-25：将此前独立的 YMODEM、Firmware Image、Firmware Storage、YMODEM Firmware Sink、A/B Firmware Lifecycle 五项资产整合到单一 `ab_ota_solution/` 目录，不复制通用 Driver 和 CRC 依赖。

## 复用性评审结论

该目录已经形成完整的 A/B OTA 核心方案骨架：传输协议、镜像格式、存储抽象、接收桥接和升级生命周期处于同一个方案边界内，同时通过 Backend / Transport / Sink 接口保持模块可替换。后续项目可以整体引用本目录，也可以只抽取其中一个子模块。
