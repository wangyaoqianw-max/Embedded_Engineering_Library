# Platform OS

本目录提供 OS 抽象接口：Thread、Mutex、Semaphore、Queue、Notify、Event Flags、Timer 与 Time。

这些头文件用于隔离上层 Service 与具体 RTOS API。当前主要验证后端为 CMSIS-RTOS2/FreeRTOS，对象采用 opaque native handle。

## 适用边界
接口语义以当前项目需求为基准，不等价于完整 CMSIS-RTOS2 封装。ISR 可调用能力、超时语义和资源释放规则在迁移时必须逐项核对。
