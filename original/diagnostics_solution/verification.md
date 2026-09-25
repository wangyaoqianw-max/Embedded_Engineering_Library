# Diagnostics Solution Verification Reference

## 来源

主要证据来自 `STM32F4_Bootloader_OTA_Test_Project` 的 S05A Debug & Crash Diagnostics 阶段，来源提交 `77e170c0fcec249484bde7031ec144c856c6f4c4`。

## 来源环境

```text
Target        STM32F411CE
Interface     SWD
GDB port      2331
Compiler      Keil / ARMCC
GDB           arm-none-eabi-gdb
Server        SEGGER J-Link GDB Server V7.92
Probe         J-Link
```

## 已验证结果

| 项目 | 来源结果 |
| --- | --- |
| GDB breakpoint / continue / step / backtrace | PASS |
| GDB runtime snapshot halt | PASS |
| GDB runtime snapshot resume | PASS |
| EasyLogger / RTT normal output | PASS |
| CmBacktrace build/link integration | PASS |
| Invalid Address Fault capture | PASS |
| Undefined Instruction Fault capture | PASS |
| Divide By Zero Fault capture | PASS |
| GDB / RTT Fault PC and register comparison | PASS |
| Fault loop halt semantics | PASS |
| Recovery to normal firmware | PASS |

典型来源现场：

```text
Invalid Address       CFSR = 0x00000400
Undefined Instruction CFSR = 0x00010000
Divide By Zero        CFSR = 0x02000000
```

来源验证还确认：项目自有 Fault Adapter 是 Cortex-M 四个 Fault Vector 的唯一 Owner，CmBacktrace 上游自带 Fault 汇编保留为 Vendor Source，但不参与项目编译。

## Library 重构差异

Library 版本相对来源工程修改了：

- `project_log_config.h` → `service_log_config.h`
- 日志默认策略宏改为 `DIAGNOSTICS_LOG_*`
- CmBacktrace 固定的 `OTA_APP / STM32F411CE / V1.0` 改为 `diagnostics_config.h` 配置宏
- 第三方 EasyLogger / CmBacktrace / SEGGER RTT 源码不进入本目录

核心 Fault Context、Handler 路径和 EasyLogger Port 逻辑未做功能性重写。

## 当前 Library 验证状态

已保存：

- `service_log_host_test.c`
- `platform_log_host_test.c`
- Host Test 所需最小 RTT / RTOS fake 头

尚未在 Library 独立构建环境重新执行，因此不得把 Library 重构版本标记为新的 PASS。后续应补统一 Host Test Runner，并在实际目标板重新执行至少一次受控 Fault 回归。
