# CmBacktrace

本目录保存常用 CmBacktrace 第三方源码快照，并按版本并存。

| 版本 | 来源 | 许可证 | 说明 |
| --- | --- | --- | --- |
| v1.5.0 | Armink / OTA 工程已使用快照 | MIT | 当前 Diagnostics 基线版本 |

当前项目级配置与输出适配不放在第三方源码目录中：

- `original/diagnostics_solution/config/cmb_user_cfg.h`
- `original/diagnostics_solution/ports/cmbacktrace/`

这样升级 CmBacktrace 本体时不会覆盖项目适配。
