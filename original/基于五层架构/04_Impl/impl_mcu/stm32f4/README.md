# STM32F4 Platform Impl

## 资产简介

本目录为“五层架构”中的 Platform MCU 接口提供 STM32F4 实现，当前覆盖 GPIO、SPI、UART、DWT 微秒延时、IRQ、System Reset 和 Independent Watchdog。

目标是让上层 Platform / Service 不直接依赖 STM32 HAL，同时避免 Library 绑定某个 CubeMX 工程的 `hspi1`、`hspi2`、`huart1` 全局符号。

## 来源与版本

主要来源：

- `stm32f4_DMA_UART_ring_RTOS`：`c27a13d2f68d278f4dc2e3768d2969eef1ffa4b2`
- `STM32F4_Bootloader_OTA_Test_Project`：`77e170c0fcec249484bde7031ec144c856c6f4c4`

GPIO、UART、Delay 等实现已经在两个项目中重复使用；OTA 项目又补充了 IRQ、Reset 和 Watchdog。

Library 版本在 2026-09-25 对 SPI/UART 构造入口做了去 CubeMX 全局变量改造。

## 目录内容

```text
stm32f4/
├─ impl_platform_gpio.c/.h
├─ impl_platform_spi.c/.h
├─ impl_platform_uart.c/.h
├─ impl_platform_delay.c
├─ impl_platform_mcu_irq.c
├─ impl_platform_mcu_reset.c
├─ impl_platform_watchdog.c
├─ tests/
└─ README.md
```

## GPIO

GPIO 使用调用者持有的：

```c
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} impl_platform_gpio_context_t;
```

Impl 不拥有 Port/Pin，不保存板级宏，也不决定具体引脚用途。

## SPI

来源工程原本提供：

```text
impl_platform_spi1_construct()
impl_platform_spi2_construct()
→ hspi1 / hspi2
```

Library 改为：

```c
impl_platform_spi_context_t context = {
    .halSpi = &hspi2
};

impl_platform_spi_construct(
    &bus,
    "storage_spi",
    PLATFORM_DEVICE_CAP_NONE,
    &context);
```

因此 CubeMX Handle 由项目注入。Impl 继续负责：

- HAL 状态到 `platform_error_t` 映射
- SPI Mode / Bit Order / Data Size 检查
- APB Clock 与 Prescaler 推导
- Blocking read/write
- 超过 HAL 16-bit Size 上限时分块传输
- Platform Lifecycle 状态映射

当前时钟推导明确支持 SPI1(APB2) 与 SPI2(APB1)。扩展 SPI3 等实例时需要补充映射。

## UART

来源工程原本固定绑定 `huart1`。Library 版改为调用者提供：

```c
impl_platform_uart_context_t context =
    IMPL_PLATFORM_UART_CONTEXT_INITIALIZER;

context.halUart = &huart1;

impl_platform_uart_construct(
    &uart,
    "console",
    PLATFORM_DEVICE_CAP_NONE,
    &config,
    callback,
    callbackContext,
    &context);
```

UART Impl 保留：

- Blocking TX/RX
- DMA TX
- Receive-To-Idle DMA RX
- Circular RX position 差分
- TX/RX cancel
- HAL error → Platform error
- TX Complete / RX Event / Error Callback → Platform Event

因为 STM32 HAL 的 UART Callback 是全局入口，Library 使用最多 4 个 Context 的轻量 Registry，根据 `UART_HandleTypeDef *` 找回对应 Platform UART。

## Delay

`platform_delay_us()` 使用 Cortex-M DWT Cycle Counter，并限制单次等待不超过半个 32-bit 计数器周期。

Library 版同时补齐 `platform_delay_ms()`，默认转发到 STM32 HAL `HAL_Delay()`。

## IRQ / Reset / Watchdog

### IRQ

当前实现把已有 Platform IRQ ID 映射到：

- EXTI0
- USART1
- DMA2 Stream2
- DMA2 Stream7

并限制 FreeRTOS FromISR 相关中断优先级不能高于当前安全边界。

这一接口仍带有参考项目语义，是当前资产的已知技术债；未来应把 IRQ 身份与板级业务名称解耦。

### Reset

`platform_mcu_reset()` 直接封装 CMSIS `NVIC_SystemReset()`。

### Watchdog

当前为 STM32F4 HAL IWDG 实现：

- Prescaler = 256
- 基于 `LSI_VALUE` 计算 Reload
- 支持当前约 100 ms ~ 32 s 目标窗口
- 启动后不可停止
- Feed Policy 不属于 Platform，由 Application / Health 层决定

## 外部依赖

本目录不复制 ST 的 HAL/CMSIS 源码。项目需要自行提供：

- STM32F4 CMSIS Device
- STM32F4 HAL
- 对应 CubeMX / 手工初始化的 HAL Handle

因此 ST Vendor 代码仍留在具体项目或其正常依赖中。

## 适用边界

- 当前实现针对 STM32F4 HAL/CMSIS。
- GPIO/SPI/UART 的物理实例由项目层提供。
- SPI 配置策略仍是“验证 CubeMX 固定硬件配置”，不是运行时重配所有寄存器。
- UART Context Registry 当前最多 4 个实例。
- IRQ ID 合同仍含参考项目业务含义。
- Watchdog 超时精度依赖实际 LSI 频率；`LSI_VALUE` 是估算值。
- `platform_delay_ms()` 依赖 HAL Tick 正常运行。

## 验证情况

来源工程已有 Host Tests：

- GPIO Impl：Context、模式、Pull、读写和 DeInit。
- SPI Impl：Lifecycle、固定配置、Prescaler/Clock、HAL Status、分块传输。
- UART Impl：DMA TX、Receive-To-Idle RX、Cancel、Error Callback、Full-duplex transaction。
- Delay：公共符号与 DWT 最大单段等待安全边界。

同时这些实现已经随两个 STM32F4 项目完成真实编译和运行。Watchdog 在 OTA S10 中有独立 Contract / Build 验证。

Library 版对 SPI/UART 构造入口进行了去全局 Handle 重构，目前尚未在 Library 独立 Runner 中重新执行全部 Host Tests，因此不能把重构版测试状态标记为新的 PASS。

## 已知限制

- 尚未提供统一 Host Test Runner / CI。
- IRQ Contract 需要在架构 V2 中进一步去板级语义。
- UART Registry 没有动态注销接口，Context 应采用静态/长期生命周期。
- SPI 目前仅识别 SPI1 / SPI2 的 APB Clock。
- DWT Delay 不适用于没有 DWT CYCCNT 的 Cortex-M 内核。
- 源码文件头仍有旧 `All Rights Reserved` 表达，后续应与 Library MIT 授权统一。

## 修改记录

- 2026-09-25：从两个 STM32F4 项目沉淀 MCU Impl；SPI/UART 改为 HAL Handle 注入；补齐 millisecond delay；收录 IRQ/Reset/Watchdog。

## 复用性评审结论

这一资产补齐了此前“五层架构只有 Platform Contract、缺少具体 MCU Backend”的主要空白。GPIO/SPI/UART 已经具备明确的 Handle 注入边界，适合直接用于后续 STM32F4 工程；IRQ 是当前最明显的后续收敛项。
