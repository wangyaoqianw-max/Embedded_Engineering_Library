# Embedded Engineering Library

这是一个公开展示的个人嵌入式代码资产库，用于保存开发和学习过程中发现、编写或改编的高复用性文件，并持续记录它们的复用价值、使用方式和适用边界。

## 仓库目标

- 沉淀可以跨项目复用的嵌入式代码资产。
- 区分原创、改编和第三方代码，保留清晰的来源与授权信息。
- 对每项资产的依赖、适用环境、限制和验证范围进行评审。
- 为后续项目提供可追溯的参考实现，而不是未经判断的代码堆积。

本仓库不保存密钥、个人隐私、公司内部代码或其他需要保密的材料。高保密内容应存放在独立的私有仓库中。

## 目录结构

```text
.
├─ original/       # 独立编写的原创资产
├─ adapted/        # 基于外部代码改编、裁剪、移植或封装的资产
└─ third_party/    # 原样保存或仅做配置级调整的第三方资产
```

每项资产必须放在独立目录中，并提供自己的 `README.md`。三个分区的具体准入规则见各自目录说明。

## 资产索引

| 资产 | 归属 | 技术类别 | 版本 | 用途 | 关键边界 | 评审状态 | 详细说明 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 基于五层架构 | 原创（AI 辅助） | 固件架构 / C 组件 | 组件 V1.3 | 分层架构、Platform Common/MCU/OS、STM32F4 Impl、Software I2C、FreeRTOS Adapter 和 SPSC Ring Buffer | IRQ Contract 仍有项目级语义；Software I2C 当前仍是 GPIO Backend；类型体系与并发语义仍需继续收敛 | Platform/Impl 已在 DMA UART 与 OTA 两个 STM32F4 项目中复用 | [查看说明](original/基于五层架构/README.md) |
| Common CRC | 原创 | 基础算法 / 数据校验 | V1.0 | CRC-8/SMBUS、CRC-16/XMODEM、CRC-32/ISO-HDLC | 非密码学完整性机制；当前为 bitwise 软件实现 | 标准向量 Host Test 通过 | [查看说明](original/common_crc/README.md) |
| UART Service | 原创 | 通信服务 / DMA / Ring Buffer | V1.0 | 异步 UART RX、SPSC 缓冲、事件唤醒、统计与同步 TX | 依赖 Platform UART/OS；按单 Producer / 单 Consumer 设计 | 已在 DMA UART 与 OTA 两个项目复用 | [查看说明](original/uart_service/README.md) |
| Diagnostics Solution | 原创 | 日志 / Crash Diagnostics | V1.0 | Service Log、Platform Log、EasyLogger/RTT Port、CmBacktrace、Cortex-M Fault Context | 第三方本体位于版本化 `third_party/`；EasyLogger Port 位于 `adapted/`；Fault 汇编当前为 Keil ARMASM | 日志已跨两项目复用；来源 S05A 三类 Fault 板测 PASS；Library 重构测试待执行 | [查看说明](original/diagnostics_solution/README.md) |
| W25Q64 Platform Driver | 原创 | 存储驱动 / SPI NOR | V1.0 | W25Q64JV 读写、Page Program、Sector Erase | 非通用 SPI NOR 层；不含文件系统和掉电事务 | 来源 OTA 工程完成板测并用于后续 OTA 数据链路 | [查看说明](original/w25q64_driver/README.md) |
| AT24C02 Platform Driver | 原创 | 存储驱动 / EEPROM | V1.0 | AT24C02 读取、分页写入、ACK Polling | 不含磨损均衡与上层事务一致性 | 来源 OTA 工程完成板测并用于 Metadata 持久化 | [查看说明](original/at24c02_driver/README.md) |
| A/B OTA Solution | 原创 | OTA / Bootloader 方案 | V1.0 | YMODEM、Firmware Image、A/B Storage、Header-last Sink、Metadata、Trial/Confirm/Rollback | 不含 MCU Flash 安装、Jump、Health Policy；CRC/Driver 作为外部通用依赖 | 来源工程软件验证 PASS、硬件 PARTIAL；Library 通用化 Host Test 待统一执行 | [查看说明](original/ab_ota_solution/README.md) |
| ST7789 + Graphics Display Solution | 原创（字体位图不收录） | 显示 / Graphics / LVGL 底层 | V1.0 | ST7789、Panel Profile、RGB565、Minimal Graphics；明确预留 Touch 扩展 | 不含 GUI Framework、Touch Driver、SPI DMA；原 Vendor-derived 8x16 字模因授权来源不明未收录 | ST7789/Graphics 已跨两项目复用；来源 Host Test 与硬件 Bring-up 有证据；Library Panel 重构待统一测试 | [查看说明](original/display_st7789_solution/README.md) |
| MPU6050 Driver | 原创 | 传感器 / IMU / I2C | V1.0 | WHO_AM_I、固定基础配置、14-byte Burst、Raw + g/dps 转换 | 当前量程/采样配置固定；不含 FIFO/DMP/中断；共享 I2C 生命周期由上层管理 | 来源阶段 Host/Keil/RTT/逻辑分析仪/物理合理性检查均有 PASS 记录 | [查看说明](original/mpu6050_driver/README.md) |
| DHT20 Driver | 原创 | 传感器 / 温湿度 / I2C | V1.0 | AC 33 00 测量、80 ms 等待、CRC-8、状态检查、RH/T 转换 | 固定同步读取；完整 Driver 板级协议 checklist 尚未形成完成证据 | 来源有 Host Test 与 DHT20/Software-I2C 连通性基线；完整板级协议验证未宣称 PASS | [查看说明](original/dht20_driver/README.md) |
| 嵌入式项目 C 代码设计规范 | 原创（AI 辅助整理） | 工程规范 / Code Review | V2.0 | 作为嵌入式 C 项目的规范起点和评审清单 | 需按项目裁剪；不替代 MISRA、CERT 或功能安全标准 | 已完成结构及仓库示例交叉检查 | [查看说明](original/嵌入式代码规范/README.md) |
| EasyLogger RTT + CMSIS-RTOS2 Port | 改编 | 日志平台适配 | 基于 EasyLogger 2.2.99 | EasyLogger → CMSIS-RTOS2/FreeRTOS + SEGGER RTT | 仍含 HAL Tick / RTOS 依赖，尚未完全平台化 | 来源 OTA 工程已实际使用；归类为改编而非第三方原版 | [查看说明](adapted/easylogger_rtt_cmsisrtos_port/README.md) |
| LVGL | 第三方 | GUI / 图形库 | v8.3 / 8.3.11 | 嵌入式图形界面开发与学习参考；为 v9.4 并存预留版本目录 | 8.3 为商家裁剪包；不同大版本 API/配置不可混用 | 当前收录 v8.3；v9.4 待后续引入 | [查看说明](third_party/LVGL/README.md) |
| CmBacktrace | 第三方 | Crash Diagnostics | 1.5.0 | Cortex-M Fault / Call Stack Backtrace | 项目配置与 Fault Adapter 不属于第三方本体 | OTA 工程 S05A 已验证该版本 | [查看说明](third_party/CmBacktrace/README.md) |
| EasyLogger | 第三方 | Logging Middleware | 2.2.99 | 日志等级、过滤、异步/缓冲输出、插件 | 当前 `elog_cfg.h` 是已使用配置快照；平台 Port 单独归入 adapted | DMA UART 与 OTA 工程均使用该版本 | [查看说明](third_party/EasyLogger/README.md) |
| SEGGER RTT | 第三方 | Debug Transport / RTT | 7.92 | J-Link 实时日志、CmBacktrace 输出、调试数据通道 | 非 MIT；遵守 SEGGER 源文件头再分发条件 | OTA/DMA UART 诊断链使用该版本 | [查看说明](third_party/SEGGER_RTT/README.md) |

## 分类规则

- **原创资产**：核心实现由仓库维护者独立编写。允许依赖第三方库，但必须列出依赖及许可证。
- **改编资产**：基于外部代码进行了实质修改、裁剪、移植或重新封装。必须记录原始来源、原许可证、修改内容以及与上游的差异。
- **第三方资产**：原样保存或仅进行配置级调整。默认只读，并保留上游版权和许可证。
- 无法确认来源或归属的内容，按第三方资产处理或暂不收录，不得按原创内容发布。

## 资产说明要求

每项资产的 `README.md` 至少应包含：

1. 资产简介
2. 来源与版本
3. 许可证
4. 目录内容
5. 依赖与适用环境
6. 使用方式
7. 适用边界
8. 已知限制
9. 验证情况
10. 修改记录
11. 复用性评审结论

评审不能只看代码能否编译，还应关注接口清晰度、平台耦合、资源占用、线程与中断环境、可移植性、授权义务和实际验证范围。

## 收录流程

1. 检查仓库中是否已有相同或功能重叠的资产。
2. 确认资产归属、原始来源、版本和许可证。
3. 判断它是否具备跨项目复用价值，而不是只适用于单个临时工程。
4. 创建独立资产目录并完成资产说明。
5. 执行与资产相称的验证，明确记录未验证项。
6. 更新本页资产索引。

## 许可证

`original/` 中未另行声明的原创资产采用 [MIT License](original/LICENSE)。`adapted/` 和 `third_party/` 中的内容遵循各资产自身的许可证，根目录的授权说明不覆盖这些第三方内容。
