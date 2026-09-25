# Diagnostics Solution

## 资产简介

本目录整合一套面向 Cortex-M 的日志与崩溃诊断方案，覆盖 Service Log、Platform Log、EasyLogger 适配、SEGGER RTT 输出、CmBacktrace 工程适配，以及 HardFault / MemManage / BusFault / UsageFault 现场捕获。

目标是把“正常运行日志”和“异常现场诊断”放在同一方案边界内，同时不复制第三方中间件源码。

## 来源与版本

主要来源于两个实际 STM32F4 工程：

- `stm32f4_DMA_UART_ring_RTOS`：提交 `c27a13d2f68d278f4dc2e3768d2969eef1ffa4b2`
- `STM32F4_Bootloader_OTA_Test_Project`：提交 `77e170c0fcec249484bde7031ec144c856c6f4c4`

其中 `service_log`、`platform_log` 和 `easylogger_port` 在两个工程中内容一致，属于已经发生真实跨项目复用的资产。Fault / CmBacktrace 集成来自 OTA 项目的 S05A Debug & Crash Diagnostics 阶段。

## 目录结构

```text
diagnostics_solution/
├─ config/
│  ├─ service_log_config.h
│  └─ diagnostics_config.h
├─ service_log/
│  ├─ service_log.c
│  └─ service_log.h
├─ platform_log/
│  └─ platform_log.h
├─ ports/
│  ├─ easylogger/
│  │  ├─ easylogger_port.c
│  │  └─ easylogger_port.h
│  └─ cmbacktrace/
│     ├─ cmbacktrace_port.c
│     └─ cmbacktrace_port.h
├─ fault/
│  ├─ diagnostics_fault.c
│  ├─ diagnostics_fault.h
│  ├─ cmbacktrace_fault_handlers.S
│  └─ diagnostics_fault_trigger.S
├─ tests/
│  ├─ service_log_host_test.c
│  ├─ platform_log_host_test.c
│  └─ fakes/
└─ verification.md
```

## 日志链路

```text
Service / APP
    │
    ▼
service_log
    │
    ▼
platform_log
    │
    ▼
EasyLogger Port
    │
    ▼
EasyLogger
    │
    ▼
SEGGER RTT
```

`platform_log_get_output_fn()` 在初始化前提供 no-op 后端，因此上层在日志系统尚未完成初始化时调用日志宏不会直接访问未初始化的 EasyLogger。

## Fault 诊断链路

```text
HardFault / MemManage / BusFault / UsageFault
                   │
                   ▼
        cmbacktrace_fault_handlers.S
                   │
                   ▼
          diagnostics_fault_handler
                   │
          ┌────────┴────────┐
          ▼                 ▼
Cortex-M Fault Context   CmBacktrace
          │                 │
          └────────┬────────┘
                   ▼
               SEGGER RTT
```

保存的现场包括 EXC_RETURN、stacked SP、MSP、PSP、CFSR、HFSR、MMFAR、BFAR，以及自动压栈的 R0-R3、R12、LR、PC、xPSR。

## 配置入口

`service_log_config.h` 负责：

- 默认日志等级
- 默认日志输出开关

`diagnostics_config.h` 负责：

- CmBacktrace Firmware Name
- Hardware Name
- Firmware Version
- 受控 Fault Test 开关
- Fault Test 类型与参数

迁移到新项目时优先修改配置，而不是修改 Service 或 Port 实现。

## 第三方依赖

本目录只保存项目自有适配代码，不复制下列第三方源码：

### EasyLogger

来源工程使用 EasyLogger 2.2.99。其源码头声明 MIT License，Copyright 2015-2019 Armink。

### CmBacktrace

来源工程中的 CmBacktrace 带独立 MIT License，Copyright 2016-2020 Armink。

### SEGGER RTT

来源工程使用 RTT 7.92。SEGGER RTT 源码允许在保留版权、条件和免责声明的前提下重新分发，但本资产不复制 RTT 源码，项目应从合法来源自行接入。

因此本目录仍属于 `original/`：保存的是项目自有 Service、Platform Contract、Port 与 Fault Adapter，而不是第三方本体。

## 使用方式

典型初始化顺序：

```text
Platform / RTOS ready
    ↓
service_log_init()
    ↓
cmbacktrace_port_init()
    ↓
diagnostics_fault_init()
    ↓
Application start
```

工程需要自行提供 EasyLogger、SEGGER RTT、CmBacktrace 和对应 include path。

Fault Handler 的向量归属必须唯一；如果 startup 文件、HAL 模板或 CmBacktrace 自带汇编也定义了同名 Handler，需要只保留一个 Owner。

## 适用边界

- Fault 现场捕获面向 Cortex-M。
- 当前 Fault Handler 汇编为 Keil ARMASM 语法，直接适用于来源工程 ARMCC 工具链。
- `easylogger_port.c` 与当前 CMSIS-RTOS / FreeRTOS 配置存在实现耦合，尤其异步输出模式需要重新核对线程、信号量和栈配置。
- Fault Handler 中直接使用 CmBacktrace 与 RTT，因此这部分是“诊断方案适配层”，不是无依赖的纯算法模块。
- 本方案不替代离线 ELF/AXF 符号、GDB、Map 文件和版本管理；它负责保留并输出 MCU 现场。

## 已知限制

- `rtt_elog_port.c/.h` 在来源项目中只是空的预留文件，因此本次没有搬入 Library。
- 当前 CmBacktrace Port 仍使用编译期字符串配置，不包含运行时 Build ID。
- Fault Context 未保存 FPU 扩展栈帧。
- 当前没有统一 GCC/ArmClang Fault Handler 汇编版本。
- Library 尚未提供统一 Host Test Runner 或 CI。

## 验证情况

来源工程的日志资产已经在两个 STM32F4 工程中复用。

OTA 项目 S05A 于 2026-09-15 完成：

- GDB 自动化合同测试 PASS
- CmBacktrace Integration Contract Test PASS
- Keil Full Rebuild PASS
- Normal RTT Logging PASS
- Invalid Address Fault Board Test PASS
- Undefined Instruction Fault Board Test PASS
- Divide By Zero Fault Board Test PASS
- CmBacktrace RTT Fault Output PASS
- GDB 与 RTT Fault PC / Context 对照 PASS
- Fault 后恢复正常固件 PASS

详细证据边界见 `verification.md`。

Library 本次把项目配置改为通用配置头，但当前会话环境未实际重新执行 Library 版本 Host Test，因此重构后的测试状态仍标记为待执行。

## 修改记录

- 2026-09-25：从 DMA UART 与 OTA 工程提取日志和 Crash Diagnostics；去除项目级日志配置名和固定 CmBacktrace 产品信息；保留第三方依赖为外部依赖。

## 复用性评审结论

该资产已经覆盖嵌入式工程中两类高频诊断需求：正常日志与异常现场。日志核心已发生真实跨项目复用，Fault/CmBacktrace 链路也有真实 STM32F411 板级验证，因此比单一设备驱动更适合作为基础设施资产长期维护。
