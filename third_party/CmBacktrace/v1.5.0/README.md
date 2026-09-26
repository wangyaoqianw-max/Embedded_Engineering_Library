# CmBacktrace v1.5.0

## 来源与版本

- 上游：Armink / CmBacktrace
- 版本：`CMB_SW_VERSION = "1.5.0"`
- 本仓库直接来源：`STM32F4_Bootloader_OTA_Test_Project`
- 来源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 许可证：MIT，见 `LICENSE`

本目录保存来源工程中已经实际使用的第三方源码快照，包括语言资源、核心源码、默认配置头和 Keil Fault Handler。

## 集成边界

项目级 `cmb_user_cfg.h` 不属于第三方本体，位于：

`original/diagnostics_solution/config/cmb_user_cfg.h`

项目自有 Fault Adapter 与 CmBacktrace Port 位于：

`original/diagnostics_solution/`

因此本目录应尽量保持只读，升级版本时以新增版本目录的方式处理。
