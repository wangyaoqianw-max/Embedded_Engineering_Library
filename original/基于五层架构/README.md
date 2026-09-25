# 基于五层架构的嵌入式固件骨架

## 资产简介

本资产提供一套面向 STM32 等嵌入式项目的分层固件骨架，采用 `APP → Service → Platform → Impl → HAL / Hardware` 的依赖方向。当前包含架构说明、SPSC 字节环形缓冲区、Platform 公共对象模型、GPIO/UART/SPI/I2C 等 MCU 抽象接口、OS 抽象接口、CMSIS-RTOS2/FreeRTOS Adapter、STM32F4 MCU Impl 和 Board 基础类型定义。

它适合作为新项目的结构参考和模块起点，不是可直接下载运行的完整固件工程。

## 来源与版本

- 主要作者：YaoQian Wang。
- 编写方式：维护者设计和编写，其中 Ring Buffer 由 AI 辅助编写。
- 第三方代码来源：未使用已识别的具体第三方源码作为改编基础。
- 当前版本：各代码文件标记为 `V1.0`，资产整体尚未建立统一版本号。

AI 辅助不改变本资产在仓库中的原创分类；如后续引入外部代码，应单独记录来源、版本和许可证。

## 许可证

本资产采用 [MIT License](../LICENSE)。部分源码文件头仍保留 `All Rights Reserved` 字样，独立分发前建议统一版权头与本许可证声明，避免授权表达不一致。

## 目录内容

```text
基于五层架构/
├─ 01_APP/                         # 应用层预留目录
├─ 02_Service/
│  └─ ring_buffer/                 # SPSC 字节环形缓冲区
├─ 03_Platform/
│  └─ platform_common/             # 公共类型、错误码、对象与生命周期接口
├─ 04_Impl/
│  └─ impl_board/board_types.h     # 板级基础类型
└─ 软件架构说明.md                 # 分层职责与依赖原则
```

当前 `01_APP/` 仍为空；`04_Impl/` 已加入 CMSIS-RTOS2/FreeRTOS Adapter，并新增 STM32F4 GPIO/SPI/UART/Delay/IRQ/Reset/Watchdog Impl；仍不是完整 BSP 或可直接运行工程。

## 依赖与适用环境

- C99 或兼容的 C 编译器。
- 编译 Ring Buffer 时需要同时加入以下头文件路径：
  - `02_Service/ring_buffer/`
  - `03_Platform/platform_common/`
  - `04_Impl/impl_board/`
- `platform_def.h` 声明的 `platform_delay_ms()` 和 `platform_delay_us()` 需要由具体 Impl 层实现。
- Board 类型当前假设 `char/short/int/long long` 分别满足预期位宽；移植到新编译器时必须验证 ABI。

## 使用方式

1. 先阅读 `软件架构说明.md`，根据项目规模决定保留哪些层级。
2. 为目标 MCU 和工具链审查或替换 `board_types.h`。
3. 将 Platform 公共层作为上层接口的基础类型和对象模型。
4. 可直接复用 `04_Impl/impl_mcu/stm32f4/` 作为 STM32F4 HAL Backend 起点，或为其他 MCU 实现等价 Impl。
5. 使用 Ring Buffer 前由调用者提供后备存储；可用容量为 `storageSize - 1`。
6. 在目标工程中补充单元测试、并发模型验证和硬件集成测试。

## 适用边界

- 适合用于分层架构学习、接口原型和新工程骨架设计。
- 不应把空目录和接口声明理解为已经完成的框架实现。
- Ring Buffer 当前采用 Partial Write：空间不足时写入可容纳部分并返回 `PLATFORM_ERR_OVERFLOW`。
- Ring Buffer 的 `volatile` 索引不能提供一般 C 线程模型所需的原子性和内存顺序保证。当前仅按受控的单 Producer / 单 Consumer 场景设计；用于多核、跨 Task 或 Task/ISR 并发前，需要结合目标平台增加原子操作、内存屏障或临界区，并重新验证。
- `platform_size_t` 固定映射为 32 位无符号类型，不适合作为所有主机环境中通用的指针宽度或 `size_t` 替代品。
- 本资产没有提供构建系统、完整示例工程、启动代码和目标板配置。STM32F4 Impl 依赖项目自行提供 HAL/CMSIS 与 CubeMX/手工初始化后的 Handle。

## 已知限制

- `platform_types.h` 重新定义了 `uint8_t`、`int32_t`、`float_t`、`double_t` 等标准名称，可能与标准头文件冲突，也与仓库中的《嵌入式项目 C 代码设计规范 V2.0》不一致。形成通用资产前应改用 `<stdint.h>` 或仅保留带 `platform_` 前缀的类型。
- `platform_object_set_state()` 未检查状态是否超出 `PLATFORM_OBJECT_STATE_MAX`。
- Device 和 Service 的枚举参数检查只显式验证上界；负值或非枚举值的可移植校验语义尚未明确。
- `platform_common/README.md` 和 `impl_board/README.md` 当前为空。
- 架构整体仍未统一验证 MCU ABI、大小端、性能和 Flash/RAM 占用；但 STM32F4 Impl 与 FreeRTOS Adapter 的多个子模块已经在两个真实 STM32F4 工程中使用。

## 验证情况

- 使用 MinGW GCC 6.3.0，以 C99、`-Wall -Wextra -Wpedantic -Werror` 对 4 个 `.c` 文件完成语法编译检查：通过，无警告。
- 使用仓库外测试程序检查 Ring Buffer 初始化、容量、写入、读取、回绕、部分写入、空读、零长度写入和重置，共 13 项断言：通过。
- 早期架构骨架本身未建立统一 CI/覆盖率；后续新增 STM32F4 Impl 来源工程已有 Host Test、Keil Build 与板级运行证据，Library Handle 注入重构后仍需重新执行统一测试。

## 修改记录

| 日期 | 修改内容 |
| --- | --- |
| 2026-09-25 | 新增 STM32F4 MCU Impl：GPIO/SPI/UART/Delay/IRQ/Reset/Watchdog；SPI/UART 改为 HAL Handle 注入，不依赖 CubeMX 全局符号。 |
| 2026-09-25 | 从两个实际 STM32F4 项目回收 Platform MCU、Platform OS 与 CMSIS-RTOS2/FreeRTOS Adapter；核心通信和 OS 抽象开始形成跨项目基线。 |
| 2026-09-07 | 收录架构说明、Platform 公共层、Board 基础类型和 AI 辅助编写的 Ring Buffer；完成首次资产评审。 |

## 复用性评审结论

架构说明具有较好的项目启动和职责划分参考价值，Ring Buffer 在明确单线程或受控 SPSC 条件后具备独立复用潜力。Platform 公共层目前更适合作为设计草案，不建议未经调整直接作为跨平台基础库：标准类型重定义、状态输入校验和并发语义需要先收敛，并在目标编译器与硬件上重新验证。


## V1.2 新增模块说明

- `04_Impl/impl_mcu/stm32f4/`：STM32F4 GPIO、SPI、UART、Delay、IRQ、Reset、Watchdog Backend。
- SPI/UART 不再依赖 `hspi1/hspi2/huart1`，由调用者注入 HAL Handle。
- UART 通过轻量 Context Registry 将 HAL 全局 Callback 路由回对应 Platform UART。
- 详细边界见 `04_Impl/impl_mcu/stm32f4/README.md`。

## V1.1 新增模块说明

- `03_Platform/platform_mcu/`：GPIO、UART、SPI、Software I2C、IRQ、Reset、Watchdog 抽象。
- `03_Platform/platform_os/`：Thread、Mutex、Semaphore、Queue、Notify、Event Flags、Timer、Time。
- `04_Impl/impl_os/freertos/`：基于 CMSIS-RTOS2 的 FreeRTOS Adapter。
- 当前 Software I2C 仍依赖项目级 `project_config.h` 时序宏，属于已识别的待解耦项。
