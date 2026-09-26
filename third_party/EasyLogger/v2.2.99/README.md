# EasyLogger v2.2.99

## 来源与版本

- 上游：Armink / EasyLogger
- 版本：`ELOG_SW_VERSION = "2.2.99"`
- 本仓库直接来源：`STM32F4_Bootloader_OTA_Test_Project`
- 来源提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`
- 许可证：MIT，见 `LICENSE`

## 收录内容

保留：

- `inc/`
- `src/`
- `plugins/file/`
- `plugins/flash/`
- 来源工程使用的 `elog_cfg.h` 配置快照

没有把来源工程已经实质改写的 `port/elog_port.c` 放在第三方目录，因为它包含 CMSIS-RTOS2、FreeRTOS 和 SEGGER RTT 项目适配代码。该文件归入：

`adapted/easylogger_rtt_cmsisrtos_port/`

## 配置说明

当前 `elog_cfg.h` 是“已验证工程配置快照”，启用了异步输出并使用较大的 Line/Async Buffer。它不是所有项目的推荐默认值。新项目复用时应审查 RAM、线程模型和日志吞吐需求。

第三方源码本体原则上只读；修改平台输出应优先修改 adapted Port 或 Diagnostics Adapter。
