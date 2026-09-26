# MPU6050 Driver

## 资产简介

该资产提供一个只依赖 `platform_i2c_t` 的 MPU6050 同步 Driver。设备对象不拥有 I2C Bus 生命周期，可以与 DHT20、Touch Controller 等设备共享同一条 I2C Bus。

## 来源与版本

- 来源仓库：`stm32f4_DMA_UART_ring_RTOS`
- 来源提交：`c27a13d2f68d278f4dc2e3768d2969eef1ffa4b2`
- 当前版本：V1.0
- 主要作者：YaoQian Wang

## 许可证

本资产归入 `original/`，按仓库 `original/LICENSE` 的 MIT License 发布。源码旧文件头仍含 `All Rights Reserved`，后续应统一版权头表达。

## 目录内容

```text
mpu6050_driver/
├─ platform_mpu6050.c
├─ platform_mpu6050.h
├─ tests/
│  └─ test_platform_mpu6050.c
└─ README.md
```

## 依赖与适用环境

依赖：

- 五层架构 Platform Common
- `platform_i2c_t`

不直接依赖 STM32 HAL、FreeRTOS、GPIO 或具体板卡。只要项目能提供兼容的 Platform I2C，就可以复用该 Driver。

## 使用方式

典型流程：

```c
platform_mpu6050_t mpu = PLATFORM_MPU6050_INITIALIZER;

platform_mpu6050_init(&mpu, &sensorI2c, 0x68U);
platform_mpu6050_read(&mpu, &measurement);
platform_mpu6050_deinit(&mpu);
```

初始化时：

1. 校验 7-bit Address，只接受 `0x68 / 0x69`。
2. 读取 `WHO_AM_I`。
3. 配置 `PWR_MGMT_1`。
4. 配置 DLPF。
5. 配置 Sample Rate Divider。
6. 配置 Gyroscope ±250 dps。
7. 配置 Accelerometer ±2 g。

读取使用从 `ACCEL_XOUT_H` 开始的一次 14-byte Burst，并输出 Raw、Acceleration g 和 Angular Velocity dps。

失败时不会修改调用者原有 measurement。

## 适用边界

当前配置固定为：

```text
Accel       ±2 g
Gyro        ±250 dps
DLPF        CONFIG = 0x03
SMPLRT_DIV  0x04
Clock       PLL with X-axis gyroscope reference
```

当前不提供 FIFO、DMP、中断、温度输出、运行时量程切换或采样率配置。

## 已知限制

- WHO_AM_I 固定按 MPU6050 语义处理。
- 当前 Scaling 与固定量程绑定。
- 不负责 I2C Bus Mutex。
- 不负责设备电源管理或中断引脚。
- 如果后续需要多配置，应增加 Config Contract，而不是继续扩展固定宏。

## 验证情况

来源阶段记录：

- Host regression PASS
- Keil production build PASS
- RTT target smoke PASS
- Logic Analyzer init sequence PASS
- Logic Analyzer burst read PASS
- Physical sanity check PASS
- Normal production rebuild PASS
- Architecture review PASS

对应 Host Test 已随资产保存。Library 本次未改 Driver 核心逻辑，因此保留来源验证证据；Library 尚未建立统一 Test Runner。

## 修改记录

- 2026-09-26：从 DMA UART 项目提取为独立 Library Driver，保留共享 Platform I2C 依赖与 Host Test。

## 复用性评审结论

该 Driver 的边界较干净：协议与数据转换位于设备层，I2C Bus 生命周期由调用者管理，没有 HAL/Board 直接耦合。适合作为后续传感器采集项目和工业控制样例的基础设备资产。
