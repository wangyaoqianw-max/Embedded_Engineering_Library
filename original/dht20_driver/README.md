# DHT20 Driver

## 资产简介

该资产提供基于 `platform_i2c_t` 的 DHT20 同步温湿度 Driver。Driver 不拥有共享 I2C Bus，也不创建 Task、Mutex 或采集周期；这些属于上层 Acquisition / Application。

## 来源与版本

- 来源仓库：`stm32f4_DMA_UART_ring_RTOS`
- 来源提交：`c27a13d2f68d278f4dc2e3768d2969eef1ffa4b2`
- 当前版本：V1.0
- 主要作者：YaoQian Wang

## 许可证

本资产归入 `original/`，按仓库 `original/LICENSE` 的 MIT License 发布。源码旧文件头仍含 `All Rights Reserved`，后续应统一版权头表达。

## 目录内容

```text
dht20_driver/
├─ platform_dht20.c
├─ platform_dht20.h
├─ tests/
│  └─ test_platform_dht20.c
└─ README.md
```

## 依赖与适用环境

依赖：

- 五层架构 Platform Common
- Platform I2C
- Platform Time

不直接依赖 STM32 HAL、FreeRTOS 或具体 Board。

## 使用方式

```c
platform_dht20_t dht20 = PLATFORM_DHT20_INITIALIZER;

platform_dht20_init(&dht20, &sensorI2c);
platform_dht20_read(&dht20, &measurement);
platform_dht20_deinit(&dht20);
```

单次读取：

```text
Write 0x38: AC 33 00
        ↓
delay >= 80 ms
        ↓
Read 7 bytes
        ↓
Busy / CRC / Calibration check
        ↓
Humidity / Temperature parse
```

CRC 参数：

```text
init = 0xFF
poly = 0x31
data = frame[0..5]
expected = frame[6]
```

只有完整事务、状态、CRC 与解析全部成功后才更新调用者 measurement。

## 适用边界

当前为同步阻塞式单次测量接口，固定设备地址 `0x38`。产品采集周期不属于 Driver，应由上层 Task/Service 控制。

## 已知限制

- 固定同步等待 80 ms。
- 没有 non-blocking state machine。
- 没有上电初始化/软复位流程。
- 不负责 I2C Bus Mutex。
- 建议上层避免过于频繁测量造成传感器自热。

## 验证情况

来源工程记录的共享硬件基线：

```text
DHT20 0x38              PASS
MPU6050 shared Soft I2C PASS
PB6 SCL / PB7 SDA       PASS
Software I2C            PASS
RTT observation         PASS
```

来源 Host Test 覆盖：

- 生命周期
- AC 33 00 固定事务
- 80 ms wait
- raw parse / float conversion
- transport error propagation
- Busy
- CRC mismatch
- CRC flag
- Calibration status
- failure output atomicity
- deinit ownership

但来源 Phase 6 文档中的完整 DHT20 Target Protocol Checklist 仍未勾选完成，因此本资产不宣称“完整 DHT20 Driver 板级协议验证 PASS”。

## 修改记录

- 2026-09-26：从 DMA UART 项目提取为独立 Library Driver，保留共享 Platform I2C 与 Platform Time 依赖以及 Host Test。

## 复用性评审结论

该 Driver 的协议边界清楚，状态/CRC/解析逻辑集中在设备层，并保持失败输出原子性。适合作为后续环境采集和工业仪表项目的基础设备资产；如需要高并发或非阻塞采集，应在 Service 层或后续版本扩展。
