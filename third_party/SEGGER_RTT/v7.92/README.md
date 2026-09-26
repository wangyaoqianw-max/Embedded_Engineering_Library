# SEGGER RTT v7.92

## 来源与版本

- 产品：SEGGER RTT
- 版本：7.92
- 本仓库直接来源：`STM32F4_Bootloader_OTA_Test_Project`
- 来源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`

源码文件头明确标注 `RTT version: 7.92`。

## 收录内容

```text
SEGGER_RTT.c
SEGGER_RTT.h
SEGGER_RTT_Conf.h
SEGGER_RTT_printf.c
SEGGER_RTT_ASM_ARMv7M.S
```

## 许可证与再分发

该源码不是 MIT。SEGGER 在每个源文件头中给出了再分发条件和免责声明；再分发时必须保留原版权声明、条件和免责声明。

本目录保留原始文件头，不改变其授权条件。正式使用或再次分发前应重新核对对应 SEGGER 版本的许可条款。

## 配置说明

`SEGGER_RTT_Conf.h` 是来源工程实际使用的配置快照。新项目可按 MCU Cache、Buffer 数量、Buffer Size、Lock 策略和工具链需求调整，但应避免修改 RTT 协议核心源码。
