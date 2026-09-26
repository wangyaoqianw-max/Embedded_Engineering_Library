# EasyLogger RTT + CMSIS-RTOS2 Port

## 资产简介

该资产保存基于 EasyLogger 官方 Port 模板改写的工程适配文件 `elog_port.c`。

它不是 EasyLogger 原版第三方文件，因此归入 `adapted/`，而不是 `third_party/EasyLogger/`。

## 来源

- 上游：Armink / EasyLogger Port 模板
- 上游许可证：MIT
- 直接来源工程：`STM32F4_Bootloader_OTA_Test_Project`
- 来源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`

## 主要改动

来源工程在上游 Port Hook 中加入：

- CMSIS-RTOS2 Mutex
- Kernel Tick / HAL Tick 时间戳
- FreeRTOS Task Name
- SEGGER RTT 输出
- RTOS 启动前后的兼容处理

## 依赖

- EasyLogger v2.2.99
- SEGGER RTT v7.92
- CMSIS-RTOS2
- FreeRTOS
- STM32 HAL Tick

## 边界

该 Port 仍含 `usart.h` 历史 include 和 STM32 HAL Tick 依赖，尚未完全平台化。项目优先通过 `original/diagnostics_solution/` 统一组织日志能力；后续可继续把该 Port 的时间/锁/线程名接口抽象化。
