# A/B Firmware Lifecycle

## 资产简介

本资产管理 A/B Firmware 的持久化生命周期，覆盖 Metadata 固定格式、双副本原子提交、PENDING / TRIAL / ROLLBACK / NONE 状态迁移、严格 Confirm，以及供 Bootloader 使用的纯启动动作决策。

它只描述“升级状态如何安全变化”，不实现 MCU Internal Flash 安装、VTOR/MSP 跳转、看门狗、RTOS Task 或产品级 Health 策略。

## 来源与版本

主要来源于 `STM32F4_Bootloader_OTA_Test_Project`：

- 源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- Application：
  - `service_firmware/firmware_metadata.c/.h`
  - `service_firmware/firmware_lifecycle.c/.h`
- Bootloader：
  - `boot_metadata_commit.c/.h`
  - `boot_main.c` 中的升级状态决策
  - `boot_prevalidate.c/.h` 中的 destructive-operation 前置验证思想
- 测试依据：
  - `04_Test/Host/S10_Trial_Confirm_Rollback/`
  - `04_Test/Reports/Stages/S10_Trial_Confirm_Rollback/verification.md`

Library 版本将 Application 与 Bootloader 中重复的 Metadata 语义收敛为一份数据合同，并把 AT24C02 直接访问拆为 Backend。

## 许可证

本资产按本仓库 `original/` 的 MIT License 发布。部分从来源工程迁移的源码文件仍保留旧的 `All Rights Reserved` 文件头，后续独立发布时应统一授权表达。

## 目录内容

```text
ab_firmware_lifecycle/
├─ firmware_lifecycle_def.h
├─ firmware_metadata.c/.h
├─ firmware_metadata_store.c/.h
├─ firmware_lifecycle.c/.h
├─ firmware_boot_decision.c/.h
├─ adapters/
│  └─ at24c02/
│     └─ firmware_metadata_at24c02_backend.c/.h
└─ tests/
   ├─ metadata_format_host_test.c
   ├─ metadata_store_host_test.c
   └─ firmware_lifecycle_host_test.c
```

## 核心状态机

```text
NONE
  │
  │ request_upgrade()
  ▼
PENDING
  │
  │ Bootloader 安装 Candidate 成功
  │ mark_trial()
  ▼
TRIAL
  ├─────────────── Application Health 达标
  │                confirm()
  │                       │
  │                       ▼
  │                     NONE
  │
  │ 下一次启动仍为 TRIAL
  │ begin_rollback()
  ▼
ROLLBACK
  │
  │ confirmed image 恢复完成
  │ complete_rollback()
  ▼
NONE
```

Boot Decision 只返回动作，不执行硬件操作：

| Metadata State | Boot Action |
| --- | --- |
| NONE | BOOT_CURRENT |
| PENDING | INSTALL_PENDING |
| TRIAL | BEGIN_ROLLBACK |
| ROLLBACK | CONTINUE_ROLLBACK |

## Metadata 持久化

Metadata 继续使用来源工程验证过的 128 Byte V2 Binary Contract，并兼容读取 V1。持久化采用 A/B 双副本和 32-bit sequence。

`firmware_metadata_store_commit()` 的顺序固定为：

1. 选择旧副本之外的目标 Copy。
2. 先写 INVALID marker。
3. 写 Body + CRC。
4. 回读并比较 Body + CRC。
5. 最后单独写 COMMIT marker。
6. 回读 marker。
7. 重新加载双副本并验证新 Copy 成为 latest。

因此在 Body 或最终 marker 写入失败时，旧 Copy 仍然保留为有效恢复点。

## Backend 边界

核心只依赖：

```c
read(context, copy, offset, data, length)
write(context, copy, offset, data, length)
```

它不知道 EEPROM 的物理地址、页大小和 I2C 总线。

当前额外提供一个可选 `AT24C02` Adapter：

- Copy A → EEPROM `0x00`
- Copy B → EEPROM `0x80`

该 Adapter 依赖 Library 中已有的 `at24c02_driver`；核心模块本身不依赖具体 EEPROM。

## Strict Confirm

`firmware_lifecycle_confirm()` 仅接受 TRIAL 状态，并在提交 Confirm 前重新：

1. 加载最新 Metadata。
2. 检查 confirmed/pending Slot 不变量。
3. 通过 `firmware_storage_validate_image()` 完整验证 pending image。
4. 检查 Header Version。
5. 将 pending Slot 提升为 confirmed Slot。
6. 更新 confirmedVersion。
7. 清空 pendingSlot。
8. 将状态提交为 NONE。

如果 Image CRC、Header 或状态检查失败，不改变持久化 Metadata。

## 依赖与适用环境

核心依赖：

- `firmware_image`
- `firmware_storage`
- `common_crc`
- 五层架构的 `platform_common`

AT24C02 Adapter 额外依赖：

- `at24c02_driver`

不依赖 STM32 HAL、FreeRTOS、W25Q64、Bootloader Jump 或 Internal Flash Driver。

## 使用方式

典型 OTA/Application：

```text
Candidate 下载并完整验证
→ 将目标 Slot 标记为 VALID
→ firmware_lifecycle_request_upgrade()
→ 重启
```

典型 Bootloader：

```text
firmware_boot_decision_get_action()

INSTALL_PENDING:
  验证并安装 Candidate
  → firmware_lifecycle_mark_trial()

BEGIN_ROLLBACK:
  验证 confirmed image
  → firmware_lifecycle_begin_rollback()
  → 执行恢复
  → firmware_lifecycle_complete_rollback()

CONTINUE_ROLLBACK:
  重新验证 confirmed image
  → 从头执行恢复
  → firmware_lifecycle_complete_rollback()
```

Application 在 Trial Firmware 达到项目定义的 Health 条件后调用 `firmware_lifecycle_confirm()`。

## 适用边界

- 当前固定为两个逻辑 Slot：A/B。
- Lifecycle 不负责 Candidate 下载。
- Lifecycle 不负责把外部镜像复制到 Internal Flash。
- Lifecycle 不检查 MSP/Reset_Handler；这是 Bootloader/目标 MCU 的安全安装边界。
- Lifecycle 不定义“运行多久算健康”；Health 条件由项目决定。
- Boot Decision 不自动执行任何硬件动作。
- confirmed image 在 Rollback 前仍应由 Bootloader 做完整 CRC、版本、向量等破坏性操作前检查。

## 已知限制

- Metadata 仍是当前项目定义的 V2 格式，并非行业标准。
- confirmedVersion 只记录 major/minor/patch 基准。
- 当前没有 Trial 次数、最大失败次数或多 Slot 扩展字段。
- Backend API 假定单次 read/write 操作的失败能被可靠上报；介质本身的写原子性仍由具体 Driver 决定。
- Library 重构版本尚未建立独立 CI 构建入口。

## 验证情况

来源项目 S10 的 Host Tests、静态合同、Python 回归和 Application/Bootloader Clean Build 已通过；来源工程的 Trial/Confirm/Rollback 功能行为获得项目验收。

来源报告同时明确：连续 Bootloader RTT/GDB 中间链和部分破坏性硬件用例未形成完整证据，因此 S10 Hardware Verification 为 `PARTIAL / DEFERRED FOLLOW-UP`。本资产不会将该状态描述为完整硬件验证 PASS。

Library 版本新增：

- Metadata 格式与不变量 Host Test。
- 双副本提交失败后旧 Copy 保持有效的 Host Test。
- PENDING → TRIAL → ROLLBACK → NONE、Boot Decision 和 strict Confirm Host Test。

当前会话环境无法直接拉取未发布 Git 对象到本地执行 GCC，因此这些 Library 重构后的测试源码已保存，但实际执行状态仍为待再次验证。

## 修改记录

- 2026-09-25：从 OTA Application 与 Bootloader 提取 A/B Lifecycle；统一 Metadata 合同；新增通用 Metadata Backend、AT24C02 Adapter、Boot Decision 和 Library Host Tests。

## 复用性评审结论

该资产已经把项目级 OTA/Bootloader 编排收敛为“持久化状态机 + 原子 Metadata 事务 + 纯启动决策”。它与具体 MCU、RTOS 和 Flash 安装实现解耦，适合继续用于 A/B OTA、独立 Bootloader 和后台升级项目。
