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


## 编码说明

来源快照中的 `Languages/zh-CN/cmb_zh_CN.h` 使用非 UTF-8 文本编码。通过当前 GitHub 连接器导入 Library 时，该文件被文本归一化为 UTF-8：

- 读取后的文本内容与来源工程一致；
- Git Blob SHA 与来源工程不同；
- 其余 CmBacktrace 第三方本体文件保持来源快照内容；
- 同目录还保留上游提供的 `cmb_zh_CN_UTF8.h`。

因此本目录可作为当前项目复用基线，但若未来要求“逐字节完全等同于上游/来源仓库”的归档，应通过支持原始二进制字节的导入链路重新导入该单个文件。
