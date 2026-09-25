# FreeRTOS / CMSIS-RTOS2 Adapter

本目录实现 Platform OS 到 CMSIS-RTOS2 的适配，覆盖 Thread、Mutex、Semaphore、Queue、Notify、Event Flags、Timer 和 Time。

## 依赖
- CMSIS-RTOS2 `cmsis_os2.h`
- 对应 Platform OS 头文件
- 底层通常为 FreeRTOS

## 使用边界
该实现依赖 CMSIS-RTOS2 语义，不是直接调用原生 FreeRTOS API 的适配层。不同 CMSIS wrapper、FreeRTOS 配置和 tick 频率下需要重新验证。

## 验证
实现已随 DMA UART 和 OTA 工程运行；OTA 版本补充了 Event Flags。迁移后至少应验证线程创建/优先级、超时、通知、队列、定时器以及 ISR/Task Context 限制。
