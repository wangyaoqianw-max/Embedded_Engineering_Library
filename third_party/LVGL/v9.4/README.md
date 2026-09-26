# LVGL v9.4 — 9.4.0 源码裁剪包

## 资产简介

本目录保存 LVGL 9.4.0 的核心源码、演示和示例，供 STM32 工程移植及 GUI Guider 生成代码集成时使用。保留的 LVGL 源码文件未作内容修改；本次只按 v8.3 资产的目录形态裁掉上游仓库中的文档、测试、脚本和非目标平台工程文件。

## 来源与版本

- 直接来源：用户提供的 `lvgl-9.4.0` 源码目录。
- 上游项目：[LVGL](https://github.com/lvgl/lvgl)，对应标签：[v9.4.0](https://github.com/lvgl/lvgl/tree/v9.4.0)。
- 本地 `lv_version.h` 声明版本为 `9.4.0`。
- 原始下载地址、精确提交及与官方标签逐文件一致性：未验证。

## 许可证

LVGL 使用 MIT License，见 [LICENCE.txt](LICENCE.txt)。保留 [COPYRIGHTS.md](COPYRIGHTS.md) 及 `src/` 内第三方组件各自的许可文件；再分发时一并保留相关版权和许可声明。

## 目录内容

- `src/`：LVGL 核心、显示与输入抽象、绘制、控件、可选模块及随附组件源码。
- `demos/`、`examples/`：演示和 API 示例，供参考；正式产品按需选择，不要求全部编译。
- `lvgl.h`：公共聚合头文件。
- `lvgl_private.h`：内部头文件，部分演示和示例会引用。
- `lv_conf_template.h`：配置模板，不是已针对某块 STM32 板配置好的 `lv_conf.h`。
- `lv_version.h`：版本宏。
- `LICENCE.txt`、`COPYRIGHTS.md`：许可证和第三方来源说明。

未保留上游文档、测试、CI、代码生成脚本、通用构建入口、Zephyr/ESP/RT-Thread 等工程封装及 NemaGFX 外部库副本。`src/` 下的 LVGL 可选模块没有逐项删除。

## 依赖与适用环境

- LVGL 核心源码以 C 为主；启用特定可选组件时，可能需要额外源文件、C++ 支持或第三方依赖。
- STM32 工程需要自行接入目标屏幕的刷新/flush、输入设备、时基、内存配置及可选 RTOS 适配；DMA、缓存和颜色格式也要按 MCU 与屏幕确认。
- [NXP GUI Guider](https://www.nxp.com/design/design-center/software/development-software/gui-guider:GUI-GUIDER) v2.0.0/v2.0.1 对应 LVGL 9.4.0。其当前资料列出的器件和工程模板面向 NXP 平台；GUI Guider 生成代码移植到 STM32 的适配工作尚未验证。

## 使用方式

1. 将 `lv_conf_template.h` 复制到工程配置位置并命名为 `lv_conf.h`，把文件开头的 `#if 0` 改为 `#if 1`，再按 MCU、显示颜色深度、内存、字体和启用控件进行配置。
2. 将本目录加入头文件搜索路径，并在项目构建系统中纳入实际需要的 `src/` 源文件；不要默认把 `demos/` 或 `examples/` 全部编入产品。
3. 按 LVGL 9.4 接口实现显示、输入及时基接入，再集成 GUI Guider 输出的 UI 源文件。
4. GUI Guider 工程与 LVGL 源码版本保持一致；不要把 v8.3 生成代码或配置与本版本混用。

具体配置方式以 [LVGL v9.4 配置文档](https://lvgl.io/docs/open/9.4/details/integration/overview/configuration) 和所用 GUI Guider 版本为准。

## 适用边界

- 适合作为 LVGL v9.4.0 的 STM32 移植源码起点、GUI Guider 生成代码的 LVGL 依赖，以及离线学习参考。
- 不包含 STM32 HAL/BSP、显示屏或触摸驱动、工程启动文件、项目级 `lv_conf.h` 和可直接构建的 STM32 示例工程。
- NXP GUI Guider 的平台模板不等于 STM32 支持；生成代码在 STM32 上的编译、资源占用和运行效果必须在目标项目验证。
- v8.3 与 v9.4 的 API、配置和移植接口不能混用。

## 已知限制

- 用户提供的源码仅通过 `lv_version.h` 确认声明版本为 9.4.0；未与官方 v9.4.0 标签逐文件比对。
- 未按具体 STM32 型号、屏幕、工具链、RTOS 或 GUI Guider 工程配置裁剪可选功能。
- 尚未执行编译、测试或开发板验证；Flash、RAM、栈、刷新性能取决于项目配置和硬件。

## 验证情况

- 已核对 `lv_version.h` 的主、次、修订版本宏为 `9`、`4`、`0`。
- 裁剪前后对保留的 1,744 个源码、头文件、配置模板和许可文件计算 SHA-256，内容一致。
- 未执行 LVGL 构建、GUI Guider 代码集成或 STM32 实机验证。

## 修改记录

| 日期 | 修改内容 |
| --- | --- |
| 2026-09-26 | 按 v8.3 资产形态将用户提供的目录整理为 `v9.4/`；保留 `src/`、`demos/`、`examples/`、必要头文件和许可证，移除文档、测试、脚本及非目标平台工程文件；未修改保留的 LVGL 源码内容。 |

## 复用性评审结论

该源码包适合作为 LVGL 9.4 的移植起点，保留全部 `src/` 可减少 GUI Guider 输出使用可选控件或模块时缺文件的风险。实际复用前仍需审查 `lv_conf.h`、构建源文件清单及平台适配；STM32 与 GUI Guider 的组合目前没有本仓库验证证据。
