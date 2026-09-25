# Protocol YMODEM

## 资产简介

Single-file YMODEM Receiver 资产，包含增量 Packet Parser、Receiver 状态机、文件 Sink 合同和独立 Transport 合同。Receiver 不再直接依赖 UART Service，可通过回调接入 UART、USB CDC 或其他可靠字节传输通道。

## 来源与版本

- 来源：`STM32F4_Bootloader_OTA_Test_Project`
- 源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 原路径：`03_Firmware/Application/OTA_APP/02_Service/service_ymodem/`
- 原始实现版本：V1.0
- Library 变更：2026-09-25 将 `service_uart_write()` 直接依赖替换为 `ymodem_transport_t`。

## 许可证

按本仓库 `original/` 的 MIT License 发布。部分迁移源码仍保留旧的 `All Rights Reserved` 文件头，独立发布前应统一授权表达。

## 目录内容

- `ymodem_def.h`：协议控制字节、Packet 大小和 Parser 事件。
- `ymodem_parser.c/.h`：SOH/STX Packet 增量解析和 CRC-16/XMODEM 校验。
- `ymodem_receiver.c/.h`：Block 0、数据块、EOT、CAN、超时和重试状态机。
- `ymodem_transport.h`：Receiver 向发送端写 ACK/NAK/CAN/C 的传输回调合同。
- `ymodem_sink.h`：文件 begin/write/end/abort 生命周期合同。
- `ymodem_config.h`：文件名长度、超时和重试次数等编译期策略。
- `tests/`：Parser 与 Receiver Host Test。

## 依赖与适用环境

- 依赖 `common_crc` 中的 CRC-16/XMODEM。
- 依赖五层架构 `platform_common` 的基础类型和错误码。
- 不依赖具体 UART、DMA、RTOS 或 Flash Driver。

## 使用方式

调用者实现 `ymodem_transport_t.write`，并提供一个 `ymodem_sink_t`。Receiver 只消费外部逐字节输入，并通过 Transport 发送协议控制字节。

## 适用边界

当前实现以单文件 YMODEM 为目标，严格解析当前工程验证过的 Block 0 元数据格式。Transport 回调本身不负责 RX；RX 字节由调用者主动喂给 Receiver。

## 已知限制

- 当前 Parser 内部固定持有 1029 Byte Packet Buffer。
- 配置以编译期宏为主，尚未改为每实例运行时配置。
- 未实现 Batch 多文件接收策略。
- Receiver 默认要求 Block 0 中包含文件大小、修改时间和权限字段，兼容范围以来源工程已验证格式为准。

## 验证情况

来源工程的 Parser/Receiver Host Test、Python Sender、Tera Term、STM32F411 + CH340 + USART1 + DMA/RingBuffer + Flash Sink 端到端验证均有 PASS 记录。Library 版本新增 Transport 解耦并同步改写 Host Test；当前会话环境无法拉取临时 Git 对象到本地执行 GCC，因此 Library 重构版本的 Host Test 标记为待再次执行。

## 修改记录

- 2026-09-25：从 OTA 项目提取；新增 `ymodem_transport_t`，移除 Receiver 对 `service_uart` 的直接依赖。

## 复用性评审结论

协议核心已与具体 UART/RTOS 解耦，适合作为串口 Bootloader、OTA 接收器或其他字节流传输场景的协议资产。下一步主要是补充独立构建入口与 Library 版本 Host Test 自动化。
