# 嵌入式 C 项目代码规范 V2.0

## 1. 规范目标与适用范围

### 1.1 目标

本规范用于约束项目自研嵌入式 C 代码的设计、编写、修改、Review 与维护。

规则优先级：

```text
正确性
  ↓
安全性
  ↓
硬件与实时性
  ↓
架构一致性
  ↓
可维护性
  ↓
可移植性
  ↓
代码风格
```

不得为了满足形式规范而破坏正确实现、稳定 API 或硬件行为。

### 1.2 适用范围

主要适用于：

- App
- Service
- Platform
- Impl
- Board / BSP / Driver
- Bootloader
- 自研 Middleware
- Utils / Common
- Test / Debug

主要针对 `.c`、`.h` 文件。

### 1.3 规则等级

- `[必须]`：原则上不得违反。
- `[推荐]`：默认遵守，存在明确工程理由时可以例外。
- `[可选]`：根据项目规模和实际需要采用。

### 1.4 非自研代码

以下代码原则上保留其原有规范：

- HAL / LL
- CMSIS
- RTOS Kernel
- Vendor SDK
- 第三方库
- 自动生成代码

不得仅为了统一风格进行大规模重命名、格式化或结构调整。

------

# 2. 文件、模块与分层规范

## 2.1 文件职责

一个源文件原则上对应一个明确模块或职责。

公共模块通常采用：

```text
module.h
module.c
```

拆分主要依据：

- 职责；
- 依赖；
- 复用；
- 测试；
- 硬件边界。

不以固定行数机械拆分文件。

## 2.2 `.c` 文件顺序

推荐：

```text
文件头
Includes
Macros / Constants
Private Types
File-static Variables
Private Function Declarations
Private Functions
Public Functions
```

## 2.3 `.h` 文件顺序

推荐：

```text
文件头
Include Guard
Includes
Public Macros / Constants
Public Types
Public APIs
Include Guard End
```

## 2.4 Header Guard

所有自研头文件必须使用 Header Guard。

```c
#ifndef PLATFORM_UART_H
#define PLATFORM_UART_H

...

#endif
```

## 2.5 Include

推荐顺序：

```text
自身头文件

项目内部头文件

RTOS / Middleware

HAL / CMSIS / Vendor

标准库
```

不得依赖间接 Include。

公共头文件应尽量能够独立 Include。

## 2.6 私有符号

仅供当前 `.c` 使用的函数、变量和常量使用 `static` 限制作用域。

## 2.7 分层依赖

推荐总体依赖：

```text
App
 ↓
Service
 ↓
Platform
 ↓
Impl / Board / BSP
 ↓
HAL / CMSIS / RTOS
 ↓
Hardware
```

上层不得无明确理由绕过抽象直接访问底层实现。

------

# 3. 模块数据模型与横切关注点

## 3.1 数据模型基本思想

对于具有明显生命周期和运行状态的模块，推荐将数据分为：

```text
Config
Context
Data
```

分别描述：

```text
Config   → 模块应该怎样工作
Context  → 模块现在怎样运行
Data     → 模块当前有什么结果
```

不要求简单模块机械建立三套结构体。

------

## 3.2 Config —— 静态设置

`xxx_config_t` 用于保存：

- 工作模式；
- 初始化参数；
- 阈值；
- Timeout；
- 周期；
- 功能开关；
- 静态硬件配置引用。

例如：

```c
typedef struct {
    platform_u32_t baudRate;
    platform_u32_t timeoutMs;
    platform_bool_t dmaEnabled;
} platform_uart_config_t;
```

初始化完成后长期不变化的 Config 应优先按只读数据处理。

Config 不应包含：

- 当前状态；
- DMA 位置；
- 实时计数器；
- 当前采样值；
- 临时错误。

------

## 3.3 Context —— 运行上下文

`xxx_context_t` 用于保存模块维持运行所需的内部状态。

典型内容：

- State；
- 生命周期；
- Index；
- Counter；
- FSM Context；
- DMA State；
- Buffer Position；
- Last Error；
- Lock；
- Handle；
- 内部工作变量。

例如：

```c
typedef struct {
    platform_device_state_t state;
    platform_size_t rxWriteIndex;
    platform_size_t rxReadIndex;
    platform_error_t lastError;
} platform_uart_context_t;
```

Context 默认属于模块内部实现。

上层不应直接修改。

------

## 3.4 Data —— 当前数据

`xxx_data_t` 用于表示模块当前产生或对外提供的数据。

例如：

```c
typedef struct {
    platform_i32_t temperature;
    platform_u32_t humidity;
    platform_u32_t timestampMs;
} temperature_sensor_data_t;
```

Data 主要用于描述：

- 测量结果；
- 当前计算结果；
- 当前设备数据；
- 最新数据快照。

Data 不应混入内部状态机、锁、DMA Handle 等运行机制。

------

## 3.5 State 与 Data

例如：

```text
READY / BUSY / ERROR
```

属于 Context 中的 State。

```text
temperature = 25
```

属于 Data。

两者应保持概念区分。

------

## 3.6 Config 动态修改

如果 Config 允许运行时修改，必须明确：

- 立即生效；
- 下次 Start 生效；
- 重新 Init 生效；
- 停止设备后才能修改。

必要时通过统一 Setter 完成：

```c
platform_error_t module_set_config(...);
```

不得让调用者直接修改配置成员而破坏当前 Context。

------

## 3.7 Data Snapshot

如果 Data 会被 ISR、DMA 或其他 Task 异步更新，应保证读取的一致性。

可根据场景使用：

- Lock；
- Critical Section；
- Double Buffer；
- Snapshot Copy；
- Queue。

避免读取到部分旧数据与部分新数据组成的不一致状态。

------

## 3.8 横切关注点

以下能力属于 Cross-cutting Concerns（横切关注点）：

- Logging
- Error Handling
- Trace
- Statistics
- Synchronization
- Assert
- Watchdog
- Debug Hook
- Performance Measurement

这些能力应尽量通过统一组件处理，而不是由每个模块各自实现。

------

## 3.9 面向切面设计思想

项目借鉴 AOP 的职责分离思想，但不实现复杂运行时 AOP 框架。

原则：

> 与核心业务无关、但横跨多个模块的公共机制，应集中设计。

例如：

```text
Logging       → 统一 Log 模块
Error Mapping → Impl / Layer Boundary
Synchronization → Resource Owner
Trace         → Trace / Hook
Watchdog      → Watchdog Manager
Debug         → Debug Infrastructure
```

------

## 3.10 横切逻辑不得隐藏关键行为

不得为了抽象而让宏或 Hook 隐式执行：

- Lock；
- Retry；
- Reset；
- Return；
- Feed Watchdog；
- 状态机跳转。

嵌入式项目必须保证：

> 执行路径可见，时序可以分析。

因此 AOP 在本项目中主要是一种职责分离思想，而不是隐藏控制流的机制。

------

# 4. 命名规范

统一命名：

| 对象              | 规则                    | 示例                          |
| ----------------- | ----------------------- | ----------------------------- |
| 文件              | `snake_case`            | `platform_uart.c`             |
| 函数              | `snake_case`            | `platform_uart_init()`        |
| 类型              | `lower_snake_case_t`    | `platform_device_t`           |
| 局部变量          | `lowerCamelCase`        | `dataLength`                  |
| 参数              | `lowerCamelCase`        | `timeoutMs`                   |
| 成员              | `lowerCamelCase`        | `powerState`                  |
| 全局/文件可变变量 | `g_` + `lowerCamelCase` | `g_deviceCount`               |
| 宏                | `UPPER_SNAKE_CASE`      | `UART_RX_BUFFER_SIZE`         |
| 枚举成员          | `UPPER_SNAKE_CASE`      | `PLATFORM_DEVICE_STATE_READY` |

## 4.1 类型名称

推荐：

```text
<module>_<object>_<semantic>_t
```

例如：

```c
platform_device_t
platform_device_config_t
platform_device_power_state_t
platform_uart_t
ring_buffer_result_t
```

避免：

```c
state_t
config_t
device_t
```

等过于宽泛的公共类型名。

## 4.2 公共函数

使用模块前缀：

```c
platform_device_init();
platform_device_deinit();

platform_device_start();
platform_device_stop();
```

## 4.3 Boolean

优先：

```text
is
has
can
should
need
enable
```

例如：

```c
isReady
hasError
canWrite
```

## 4.4 单位

存在歧义时明确单位：

```c
timeoutMs
delayUs
frequencyHz
voltageMv
```

## 4.5 长度语义

严格区分：

```text
Size
Length
Count
Index
Offset
Capacity
```

------

# 5. 代码格式规范

- 4 空格缩进；
- 禁止 TAB；
- 推荐不超过 120 列。

函数：

```c
void function(void)
{
}
```

控制语句：

```c
if (condition) {
}
```

所有控制语句必须使用大括号。

一行只写一条语句。

指针统一：

```c
platform_u8_t *buffer;
```

推荐：

- 一个变量一条声明；
- 靠近首次使用位置；
- 能初始化时立即初始化。

不使用 Yoda Condition。

条件中不隐藏赋值、自增或重要副作用。

复杂条件应拆分或使用 early return。

具有独立工程含义的 Magic Number 应使用具名常量。

------

# 6. 注释与文档规范

## 6.1 注释内容

注释优先说明：

- 为什么；
- 约束；
- 硬件行为；
- 时序；
- 所有权；
- 并发关系；
- 特殊算法。

不要逐行翻译代码。

## 6.2 注释语言

默认中文。

允许：

1. 中文；
2. 中英双语；
3. 纯英文。

中英双语时中文在前。

同一模块保持一致。

## 6.3 文件头

自研 `.c/.h` 文件应至少包含：

```text
Copyright
@file
@brief
@author
@date
@version
```

`@note`、`@warning` 按需使用。

Git 已承担版本历史时，不要求维护详细 `@history`。

## 6.4 公共 API

公共 API 的 Doxygen 主要维护在 `.h`。

应根据需要说明：

- 参数方向；
- NULL 语义；
- 单位；
- Buffer 长度；
- 生命周期；
- 所有权；
- 返回值；
- 副作用。

`.c` 不重复相同文档。

## 6.5 私有函数

简单私有函数无需机械增加 Doxygen。

## 6.6 TODO

统一：

```text
TODO
FIXME
NOTE
WARNING
```

不长期保留已失效 TODO 和注释掉的旧代码。

------

# 7. 类型、变量、常量与宏规范

## 7.1 基础类型体系

项目允许通过基础类型适配层隔离标准库依赖。

底层可以提供：

```c
uint8
uint16
uint32
uint64

int8
int16
int32
int64
```

Platform 公共层推荐：

```c
platform_u8_t
platform_u16_t
platform_u32_t
platform_u64_t

platform_i8_t
platform_i16_t
platform_i32_t
platform_i64_t

platform_size_t
platform_addr_t
platform_bool_t
```

## 7.2 标准名称

不得自行重新定义：

```text
uint8_t
uint32_t
size_t
float_t
double_t
```

等标准命名。

## 7.3 固定宽度

协议、寄存器、Flash、序列化数据使用宽度明确的类型。

自定义固定宽度类型时必须验证 ABI。

## 7.4 Boolean

可以定义：

```c
typedef uint8 platform_bool_t;
```

并统一：

```c
PLATFORM_FALSE
PLATFORM_TRUE
```

## 7.5 Size / Address

Buffer 长度和 Index 优先：

```c
platform_size_t
```

通用地址表示使用：

```c
platform_addr_t
```

不得使用 `platform_u32_t` 作为通用指针容器。

## 7.6 Enum / Struct

Enum 不直接作为稳定协议或 Flash 二进制格式。

Struct 原始布局不直接作为稳定协议、文件或持久化格式。

## 7.7 `const`

只读输入使用 `const`。

禁止无理由移除 `const`。

## 7.8 `volatile`

仅用于 MMIO 或异步可见性等场景。

`volatile` 不等于：

- Atomic；
- Mutex；
- Thread Safe；
- Cache Coherent。

## 7.9 常量

推荐：

```text
enum         → 状态、类别、错误
static const → 普通常量
#define      → 配置、位掩码、寄存器、预处理
```

## 7.10 宏

优先函数或 `static inline`。

多语句宏使用：

```c
do {
    ...
} while (0)
```

------

# 8. 函数与接口设计规范

## 8.1 职责

一个函数原则上完成一个主要职责。

普通函数建议：

- 约 50 行以内；
- 嵌套尽量不超过 4 层。

仅作为复杂度提醒。

## 8.2 参数

属于同一个逻辑配置的大量参数可组合为：

```c
xxx_config_t
```

但不强行组合无关参数。

## 8.3 输入检查

公共接口必须检查：

- NULL；
- Length；
- Range；
- Index；
- Enum；
- State；
- Address；
- Alignment。

## 8.4 Buffer

明确：

```text
bufferSize
dataLength
readLength
writtenLength
```

输入 Buffer 使用 `const`。

## 8.5 返回值

可能失败的 API 使用统一错误类型。

Boolean 仅表达真假。

## 8.6 生命周期

推荐：

```text
init
start
stop
deinit
```

重复初始化和重复释放行为必须明确。

## 8.7 所有权

接口应能回答：

```text
谁创建？
谁持有？
是否复制？
是否保存？
谁释放？
何时失效？
```

## 8.8 Blocking

阻塞接口必须明确：

- 是否阻塞；
- Timeout；
- 单位。

同步与异步语义必须稳定。

## 8.9 Platform API

不得泄漏：

```text
HAL Handle
RTOS Handle
IRQn_Type
具体 MCU Register Type
```

------

# 9. 控制流规范

优先 early return 处理：

- 参数错误；
- 状态错误；
- 不支持；
- 资源错误。

`switch` 适用于：

- 状态机；
- Command；
- Event；
- Device Type。

每个 `case` 必须明确结束。

Fallthrough 必须显式说明。

循环必须具有退出条件。

注意：

- Array boundary；
- Unsigned underflow；
- Hardware timeout。

Retry 必须：

- 仅用于可恢复错误；
- 有上限；
- 不掩盖永久错误。

允许 `goto cleanup` 处理 C 资源释放。

复杂生命周期优先使用 State Machine，而不是大量 Boolean Flag。

------

# 10. 指针、数组与内存规范

指针必须保证：

- 有效；
- 生命周期正确；
- 所有权明确。

Buffer 必须保证边界明确。

检查：

```text
offset + length
address + length
```

时应避免整数溢出。

数组传参时显式传长度。

`ARRAY_SIZE()` 只能用于真正数组。

默认避免：

- VLA；
- 大型局部数组；
- 深层递归。

动态内存：

- 不绝对禁止；
- 默认优先静态或预分配；
- 必须检查失败；
- 必须有释放路径；
- ISR 默认禁止普通动态分配。

`memcpy` 必须验证边界。

重叠内存使用 `memmove` 或明确算法。

DMA Buffer 必须保证：

- 生命周期；
- 所有权；
- 对齐；
- DMA 可访问性；
- Cache 一致性。

字节流解析不得依赖：

- 未对齐访问；
- CPU Endianness；
- Struct Padding。

------

# 11. 错误处理与返回值规范

Platform 统一使用：

```c
platform_error_t
```

成功：

```c
PLATFORM_OK
```

错误码按调用者真正需要区分的语义设计，例如：

```text
INVALID_PARAM
INVALID_STATE
NOT_READY
BUSY
TIMEOUT
NO_MEMORY
NO_SPACE
NOT_SUPPORTED
OVERFLOW
DATA
CRC
HW
INTERNAL
```

Impl 将 HAL / RTOS / Vendor Error 映射为 Platform Error。

可能失败的 API 必须检查返回值。

确实故意忽略可显式：

```c
(void)function();
```

不得机械忽略。

错误应正确传播，不无意义吞掉根因。

Partial Success 必须明确实际完成量。

错误路径必须正确释放资源。

Assert 不处理正常运行时错误。

------

# 12. 中断、DMA 与并发规范

## 12.1 ISR

ISR 优先：

```text
识别原因
清状态
保存最少数据
通知 Task
退出
```

默认禁止：

- 阻塞；
- 普通 Mutex；
- 动态内存；
- 大量日志；
- 长循环；
- 完整协议解析。

## 12.2 Shared State

跨 Task / ISR / DMA / Callback 的状态必须明确：

```text
谁写？
谁读？
怎样同步？
```

## 12.3 同步方式

根据情况使用：

- Mutex；
- Critical Section；
- Atomic；
- Queue；
- Event；
- Single Owner。

复杂状态优先单一 Owner Context。

## 12.4 Critical Section

必须：

- 短；
- 成对；
- 错误路径恢复。

## 12.5 DMA

DMA 必须明确：

```text
Buffer
Direction
Length
Ownership
Completion
Error
Overflow
```

TX DMA 完成前不得修改 Buffer。

RX DMA 不得读取未完成写入区域。

Circular DMA 必须处理：

```text
Write Position
Read Position
Wrap
Overflow
```

UART DMA + IDLE 不得假设 IDLE 等于完整协议帧。

## 12.6 Deinit

对象释放前必须先停止仍可能访问它的：

- DMA；
- IRQ；
- Timer；
- Callback；
- Async Event。

------

# 13. RTOS 使用规范

Task 应具有明确职责。

不默认：

```text
一个模块 = 一个 Task
```

无工作时 Task 优先 Block。

固定周期任务优先使用绝对周期调度方式。

Priority 根据：

- Deadline；
- 响应时间；
- 数据速率；
- 阻塞关系；

设计。

OS Object 创建必须检查失败。

Mutex 用于资源互斥。

Semaphore 更多用于事件或计数。

Queue Full 必须定义行为。

Queue 传指针必须明确所有权和生命周期。

Task Stack 应评估并在 Debug 阶段监测。

Watchdog 推荐：

```text
Task Heartbeat
      ↓
Watchdog Manager
      ↓
Hardware Watchdog
```

RTOS abstraction 按真实可移植需求建立，不全量机械包装。

------

# 14. 硬件与寄存器规范

具体：

- MCU；
- HAL；
- LL；
- CMSIS；
- IRQ；
- DMA；
- Register；
- GPIO；
- Clock；

尽量限制在 Impl / Board / BSP。

不得为了项目风格修改厂商 API 和符号。

HAL、LL、Register 可根据实际需求选择。

直接寄存器访问必须确认：

- R/W；
- W1C；
- Read-to-clear；
- Reserved Bits；
- Reset Value；
- Access Order；
- Side Effect。

初始化顺序必须依据硬件要求。

IRQ Enable 前相关 Context 必须有效。

Board 层负责：

- Pin Mapping；
- Active High / Low；
- Pull；
- 安全初始电平。

硬件 Delay 和时序应有：

- Datasheet；
- Reference Manual；
- Errata；
- Schematic；

等依据。

存在 Ready/Busy 时优先使用状态 + Timeout。

Flash 擦写必须严格校验：

- Address；
- Partition；
- Alignment；
- Erase Unit；
- Program Unit；
- Protected Area。

Errata workaround 必须注明依据。

------

# 15. 可移植性规范

总体目标：

> 平台变化时修改范围可控。

可移植层级：

```text
App / Service
    高可移植

Platform
    API 稳定

Impl / Board
    平台相关

HAL / Vendor
    完全平台相关
```

不得假设：

```text
pointer == 32 bit
```

协议和持久化格式不得依赖：

- Endianness；
- Alignment；
- Padding；
- Enum Width。

Compiler Extension 尽量限制在适配层。

条件编译主要用于：

- Platform；
- Compiler；
- Hardware Revision；
- Feature。

业务代码避免大量 MCU 条件编译。

可移植接口最重要的是：

> 不同 Impl 行为一致。

不得为了理论可移植性建立大量无价值包装。

------

# 16. 日志、调试与断言规范

项目统一日志：

```text
ERROR
WARN
INFO
DEBUG
```

按需增加 TRACE / FATAL。

日志应提供有价值的诊断上下文。

避免同一错误逐层重复打印。

以下场景严格控制日志：

- ISR；
- DMA Callback；
- 高频 Timer；
- Control Loop；
- 高优先级 Task。

程序行为不能依赖：

- Debug Log 是否启用；
- 日志参数副作用。

Release 可保留：

- ERROR；
- WARN；
- 必要 INFO。

Assert 仅用于内部不变量。

Assert 表达式禁止包含必要副作用。

Fault Handler 应：

- 最小依赖；
- 不依赖复杂 RTOS Log；
- 尽量保存关键现场。

------

# 17. 第三方与自动生成代码规范

第三方代码保持：

```text
原目录
原命名
原类型
原格式
原 API
```

项目通过 Adapter 完成：

- Type Mapping；
- Error Mapping；
- API Adaptation。

CMSIS、HAL、LL、Startup 保持原厂风格。

Generated Code 优先仅修改允许的 USER CODE 区域。

业务逻辑尽量移出 Generated 文件。

只有：

- Bug；
- Security；
- Compatibility；
- MCU Adaptation；
- Resource / Performance；
- Required Feature；

等实际需求才修改第三方源码。

第三方修改必须：

- 最小；
- 集中；
- 可追踪。

记录依赖版本、来源和 License。

------

# 18. Code Review 规则

Review 顺序：

```text
1. 功能正确性
2. 内存和数据安全
3. 硬件与实时性
4. 错误处理
5. 资源释放
6. ISR / DMA / 并发
7. API 与架构边界
8. 数据模型职责
9. 可移植性
10. 可维护性
11. 命名 / 格式 / 注释
```

重点检查：

```text
输入是否验证？
Buffer 是否越界？
Pointer 生命周期是否正确？
返回值是否检查？
错误路径是否释放资源？
Timeout 是否存在？
Retry 是否有边界？
ISR 是否阻塞？
DMA 所有权是否明确？
Task 是否存在竞态？
Config / Context / Data 是否混乱？
Platform 是否泄漏 HAL？
横切逻辑是否重复散落？
硬件行为是否有官方依据？
日志与 Assert 是否合理？
是否存在新的 Compiler Warning？
```

风格问题不得掩盖功能、安全和架构问题。

大规模格式修改应独立提交。

------

# 19. 静态检查、格式化与质量工具

能够机器检查的问题优先交给工具。

人工 Review 重点关注：

- 架构；
- 数据模型；
- 硬件；
- 实时性；
- 所有权；
- 状态机；
- 并发。

推荐工具：

```text
clang-format
clang-tidy
cppcheck
Compiler Warning
```

按项目需要增加：

```text
MISRA Checker
PC-lint
Coverity
```

## 19.1 clang-format

统一：

```text
4 Spaces
No TAB
120 Columns
Function Brace New Line
Control Brace Same Line
Pointer Right Alignment
Indented Case Labels
```

只默认格式化自研代码。

排除：

- Vendor；
- ThirdParty；
- Generated。

## 19.2 Warning

自研代码启用较高 Warning Level。

禁止仅通过 Cast 消除 Warning。

## 19.3 Static Analysis

逐步启用静态规则。

Suppression 必须：

- 范围尽量小；
- 有明确理由。

## 19.4 C 标准

Compiler 和 Static Analysis Tool 使用相同 C 标准。

项目最终应明确采用：

```text
C99
或
C11
```

## 19.5 CI

推荐：

```text
Format Check
     ↓
Compile
     ↓
Compiler Warning
     ↓
Static Analysis
     ↓
Unit Test
```

条件允许增加：

```text
Host Test
Integration Test
Firmware Size Check
```

## 19.6 Firmware Size

关注：

- Flash；
- RAM；
- `.data`；
- `.bss`；
- Heap；
- Stack。

Bootloader 等固定分区模块不得超过预留区域。

正式构建建议保存 `.map`。

------

# 20. 项目设计总原则

项目代码设计最终遵循以下原则。

### 分层

```text
App
 ↓
Service
 ↓
Platform
 ↓
Impl / Board
 ↓
HAL / RTOS
 ↓
Hardware
```

### 模块数据模型

```text
              Module
                │
      ┌─────────┼─────────┐
      │         │         │
    Config    Context    Data
      │         │         │
   静态设置   运行上下文   当前数据
```

### 横切关注点

```text
Logging
Error Handling
Trace
Synchronization
Watchdog
Debug
Statistics
```

由公共机制统一提供。

### 核心原则

```text
模块职责明确
        +
数据职责明确
        +
依赖方向明确
        +
横切机制集中
        +
硬件实现隔离
        +
执行路径可见
```

目标不是追求复杂设计模式，而是：

> 在保证嵌入式系统正确性、实时性和资源可控的前提下，使模块边界、数据职责、运行状态和平台依赖保持清晰、稳定、可维护。