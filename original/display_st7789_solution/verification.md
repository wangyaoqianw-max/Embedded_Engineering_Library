# Display ST7789 Solution Verification

## 来源证据

来源工程：`stm32f4_DMA_UART_ring_RTOS`

来源提交：

`c27a13d2f68d278f4dc2e3768d2969eef1ffa4b2`

已保存的来源回归测试：

- `tests/upstream_reference/test_platform_st7789.c`
- `tests/upstream_reference/test_platform_graphics.c`

来源设计冻结文档记录了 P169H002-CTP 的最小 Bring-up 结果，包括：

```text
ST7789T3
240 x 280
RGB565
SPI Mode 3
12.5 MHz baseline
X Offset = 0
Y Offset = 20
Backlight HIGH = ON
```

ST7789 / Graphics Core 随后在 `STM32F4_Bootloader_OTA_Test_Project` 中继续复用，相关源码与前一工程 SHA 一致。

## 来源 Host Test 覆盖

ST7789 Test 涵盖：

- BSP/面板静态配置
- GPIO 安全初始状态
- Reset Delay
- Controller Init Sequence
- SPI Transaction
- RGB565 Region Write
- Backlight
- Deinit
- Init Failure Rollback
- Region Boundary

Graphics Test 涵盖：

- Font Descriptor
- Glyph 展开
- draw_char
- draw_string
- Region Validation
- Write Failure Propagation

## Library 重构差异

Library 相对来源工程做了以下结构修改：

- 删除项目专用 `platform_bsp_st7789.c/.h`
- 删除 `project_config.h` 面板参数依赖
- 新增 `platform_st7789_panel_config_t`
- 新增 P169H002 Panel Profile
- 不收录来源不明的 Vendor-derived ASCII 位图数据
- 新增 Touch Extension Design

因此来源 Host Test 只能作为 Upstream Reference，不能直接声明 Library 当前版本已重新 PASS。

## 当前 Library 验证状态

已新增：

- `tests/panel_config_host_test.c`
- 来源 ST7789/Graphics Test Reference

待后续统一 Runner 执行：

1. Panel Profile Host Test
2. ST7789 Driver Host Test（改为 Generic Panel Config Fixture）
3. Graphics Host Test（使用 Library 自建测试字体）
4. STM32F4 SPI Impl + ST7789 集成编译
5. P169H002 Target Smoke Test
6. 若增加 Touch，再独立验证 Touch Controller 与坐标映射

当前不得将 Library 重构版标记为新的完整 PASS。
