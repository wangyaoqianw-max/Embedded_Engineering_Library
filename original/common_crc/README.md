# Common CRC

## 资产简介
纯 C CRC 计算组件，提供 CRC-8/SMBUS、CRC-16/XMODEM、CRC-32/ISO-HDLC 的流式与一次性计算接口。

## 来源与版本
- 来源：`STM32F4_Bootloader_OTA_Test_Project`
- 源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 原路径：`03_Firmware/Application/OTA_APP/02_Service/service_common/crc/`
- 当前文件版本：V1.0

## 许可证
按本仓库 `original/` 的 MIT License 发布。源码文件头仍保留旧的 `All Rights Reserved`，后续独立发布时建议统一版权声明。

## 目录内容
- `crc.c/.h`：CRC 实现与接口。
- `tests/crc_host_test.c`：主机端标准向量测试。

## 依赖与适用环境
仅依赖基础整数类型定义；适用于裸机、RTOS 与主机测试环境。

## 使用方式
将 `crc.c/.h` 加入工程，按算法调用 init/update/finalize 或 calculate 接口。

## 适用边界
适合协议校验、固件完整性快速检查和存储数据校验；CRC 不提供密码学真实性与抗篡改能力。

## 已知限制
当前为 bitwise 软件实现，未针对查表、硬件 CRC 外设或 SIMD 优化。

## 验证情况
来源工程的 Host Test 对字符串标准向量同时验证一次性与分段计算路径。

## 修改记录
- 2026-09-25：从 OTA 项目提取为独立复用资产。

## 复用性评审结论
硬件与项目耦合低，可直接作为通用基础组件复用。
