# UART Service

## 资产简介
面向异步 UART/DMA 接收场景的 Service 层组件。使用 SPSC Ring Buffer 缓冲 RX 数据，并提供事件唤醒、数据丢失状态、统计信息与同步 TX transaction 管理。

## 来源与版本
- 首次成熟来源：`stm32f4_DMA_UART_ring_RTOS`，提交 `c27a13d2f68d278f4dc2e3768d2969eef1ffa4b2`
- 再次复用来源：`STM32F4_Bootloader_OTA_Test_Project`，提交 `77e170c0fcec249484bde7031ec144c856c6f4c4`
- 两个项目中的 `service_uart.c/.h` 当前内容一致。
- 当前文件版本：V1.0

## 许可证
按本仓库 `original/` 的 MIT License 发布。源码文件头旧版权声明后续建议统一。

## 目录内容
- `service_uart.c`
- `service_uart.h`

## 依赖与适用环境
依赖五层架构中的 `ring_buffer`、`platform_uart` 和 `platform_os`。目标 UART Impl 需要提供异步 RX/TX、事件回调与取消能力。

## 使用方式
调用者提供 UART 对象、DMA RX 缓冲、Ring Buffer 存储和 owner thread；完成 init 后启动 RX session，Consumer Task 通过 wait/read 消费数据。

## 适用边界
设计目标是单 UART RX callback Producer + 单 Consumer Task。不是多 Producer/Multi Consumer 通用消息队列。

## 已知限制
并发安全依赖当前 SPSC Ring Buffer 和目标 Platform/RTOS 实现；统计字段为 best-effort 快照，不保证跨字段事务一致性。

## 验证情况
已在两个独立 STM32F4 工程中复用；来源项目覆盖 DMA UART、RTOS 和 YMODEM 数据通路。迁移到新 MCU/RTOS 后仍需重新执行板级异步收发、溢出和超时测试。

## 修改记录
- 2026-09-25：以 OTA 工程中的当前版本提取；确认与 DMA UART 工程版本一致。

## 复用性评审结论
已经发生真实跨项目复用，适合作为高优先级通信基础资产。
