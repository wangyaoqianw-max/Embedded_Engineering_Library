# ST7789 + Graphics Display Solution

## 资产简介

本目录沉淀一套基于 ST7789 的轻量显示方案，覆盖 ST7789 Controller Driver、面板参数 Profile、RGB565 区域写入、背光控制和最小 ASCII Graphics/Text 能力。

方案目标不是提供完整 GUI Framework，而是提供一个可直接作为 LVGL、项目自研 UI 或简单状态显示底层的 Display 基线。

本方案明确支持后续扩展触摸功能，但触摸不会耦合进 ST7789 Driver。详细设计见 `docs/touch_extension.md`。

## 来源与版本

主要来源：

- `stm32f4_DMA_UART_ring_RTOS`
  - 提交：`c27a13d2f68d278f4dc2e3768d2969eef1ffa4b2`
- `STM32F4_Bootloader_OTA_Test_Project`
  - 提交：`77e170c0fcec249484bde7031ec144c856c6f4c4`

两个工程中的 ST7789 Driver 与 Graphics Core 为相同实现，属于已经发生真实跨项目复用的资产。

来源工程已验证的面板基线：

```text
Module      P169H002-CTP
Controller  ST7789T3
Resolution  240 x 280
Pixel       RGB565
SPI         Mode 3 / 8 bit / MSB First
X Offset    0
Y Offset    20
```

Library 版本移除了原来的项目级 `platform_bsp_st7789.c` 与 `project_config.h` 绑定，改用通用 Panel Config + Profile。

## 目录结构

```text
display_st7789_solution/
├─ st7789/
│  ├─ platform_st7789.c
│  └─ platform_st7789.h
├─ panel/
│  ├─ platform_st7789_panel.c/.h
│  └─ p169h002_st7789_profile.c/.h
├─ graphics/
│  ├─ platform_graphics.c/.h
│  └─ font/
│     ├─ platform_font.h
│     └─ README.md
├─ docs/
│  └─ touch_extension.md
├─ tests/
│  ├─ panel_config_host_test.c
│  └─ upstream_reference/
├─ verification.md
└─ README.md
```

## 显示数据链

```text
APP / GUI / LVGL
       │
       ▼
Graphics / Display Adapter
       │
       ▼
ST7789 Driver
       │
       ├─ Platform SPI Device
       ├─ Platform GPIO
       └─ Platform Time
              │
              ▼
      STM32F4 / other Impl
```

ST7789 Driver 不直接依赖：

- STM32 HAL
- CubeMX `hspiX`
- FreeRTOS
- 某个固定 GPIO
- 某个固定 Board

## ST7789 能力

当前公共能力：

```text
init / deinit
backlight on / off
draw pixel
fill full screen
fill rectangle
write RGB565 region
```

区域写入使用：

```text
CASET
→ RASET
→ RAMWR
→ RGB565 pixel stream
```

整个 Region 保持在同一 SPI Transaction 中。

RGB565 公共数据合同使用 `uint16_t`，Driver 内部负责转换成 ST7789 Wire 需要的 high-byte-first 字节流。

## Panel Profile

通用 Driver 不再写死：

```text
240 x 280
Y_OFFSET = 20
SPI Mode 3
12.5 MHz
Backlight polarity
```

这些信息通过 `platform_st7789_panel_config_t` 注入。

示例：

```c
platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;

platform_st7789_apply_panel_config(
    &display,
    &g_p169h002St7789PanelConfig);
```

随后由具体项目分别构造：

```text
display.cs
display.dc
display.reset
display.backlight
SPI Bus
```

这样同一个 ST7789 Driver 可以复用到不同板卡和不同 ST7789 面板。

## Graphics

当前 Graphics Core 只提供最小文字能力：

```text
draw_char()
draw_string()
```

当前合同限定：

- 8 x 16
- printable ASCII 0x20 ~ 0x7E
- monochrome bitmap
- opaque foreground/background rendering
- 不自动换行
- 不做 clipping

这是轻量显示能力，不是通用 GUI Framework。

## 字体资源边界

来源工程设计文档明确记录过 Vendor LCD 参考文件中的 ASCII 字模被作为字模资源来源。

由于当前无法独立确认那份位图资源的原始授权，本 Library **不把来源工程中的 8x16 位图数据收入 `original/`**。

本目录只保留 `platform_font_t` 字体合同。

项目可自行提供：

- 自有 ASCII 点阵
- 明确开源许可证的字体
- 外部 Flash 字库
- 中文 / UTF-8 字体
- LVGL Font

详见 `graphics/font/README.md`。

## 触摸扩展能力

本方案明确可以继续扩展触摸屏功能。

来源 P169H002-CTP 资料包含：

```text
TP_SCL
TP_SDA
TP_TRST
TP_TINT
```

因此后续可以在当前目录中增加：

```text
touch/
├─ platform_touch.h
├─ touch_controller_xxx.c/.h
└─ touch_service.c/.h
```

但架构边界固定为：

```text
ST7789 = Display Output
Touch Controller = Input Device
```

二者通过 GUI / LVGL 层组合，不互相直接调用。

未来使用 LVGL 时推荐：

```text
LVGL flush_cb
    ↓
platform_st7789_write_rgb565()

LVGL indev read_cb
    ↓
Touch Driver / Touch Service
```

详细扩展方案见：

`docs/touch_extension.md`

## 与 LVGL 的关系

本方案适合作为 LVGL Display Port 的底层：

```text
LVGL
 ↓
Display Flush Adapter
 ↓
platform_st7789_write_rgb565()
 ↓
ST7789
```

当前 Graphics 模块在引入 LVGL 后可以：

1. 保留作为 Boot / Debug / Recovery 最小显示能力。
2. 或仅用于无 LVGL 的小型项目。

不建议让 LVGL Widget 直接访问 ST7789 寄存器或 SPI。

## 外部依赖

依赖 Library：

- 五层架构 `platform_common`
- Platform GPIO
- Platform SPI
- Platform Time

典型 STM32F4 项目可组合：

- `original/基于五层架构/04_Impl/impl_mcu/stm32f4/`
- `original/基于五层架构/04_Impl/impl_os/freertos/`

本方案不复制 HAL/CMSIS/FreeRTOS。

## 资源模型

当前设计不使用 Full Framebuffer。

240 x 280 RGB565 Full Framebuffer 约为：

```text
240 × 280 × 2 = 134400 bytes
```

Driver 使用固定小型 Scratch Buffer，并对 Pixel Stream 分块输出。

这对 SRAM 较小的 MCU 更合适。

## 适用边界

- 当前 Controller 为 ST7789/ST7789T3。
- Init Table 基于来源工程验证过的 ST7789T3 面板序列。
- Runtime Rotation 尚未抽象。
- Graphics 当前仅支持 8x16 ASCII Contract。
- 当前 SPI Pixel 输出为同步 Platform SPI Write。
- 不包含 SPI DMA Display Pipeline。
- 不包含 Display Task / Queue / UI State。
- 不包含触摸实现，只明确触摸扩展接口方向。
- Touch Controller 型号必须根据实际硬件确认后再实现。

## 验证情况

来源工程具有：

- ST7789 Host Test
- Graphics Host Test
- P169H002 最小硬件 Bring-up 记录
- RTOS Display Integration 设计与实际项目集成基础

ST7789/Graphics Core 随后被复用到 OTA Application 中，源码 SHA 保持一致。

Library 新增 `platform_st7789_panel_config_t` 与 P169H002 Profile，并移除了项目级 BSP 构造耦合。

当前 Library 尚未统一重新运行完整 Display Host Test，因此 Library 重构后的验证状态仍为待执行，详细见 `verification.md`。

## 已知限制

- ST7789 Init Sequence 目前仍偏向已验证的 P169H002/ST7789T3。
- Panel Config 解耦了 Geometry/Timing，但 Init Register Table 尚未完全 Profile 化。
- Graphics 与 ST7789 类型直接关联，还不是 Generic Surface API。
- 字体位图未随资产收录。
- Touch 只保留设计扩展点，尚未实现。
- 没有统一 Host Test Runner/CI。
- 源码旧文件头仍存在 `All Rights Reserved` 与 Library MIT 表达不一致的问题。

## 修改记录

- 2026-09-26：从两个 STM32F4 项目沉淀 ST7789 + Graphics；移除项目 BSP/Project Config 耦合；增加 Panel Profile；明确 Touch 与 LVGL 扩展路径；因字模来源授权不明，不收录原 8x16 位图数据。

## 复用性评审结论

该资产已经具备较清晰的“Controller Driver + Panel Profile + Minimal Graphics”边界，并且与具体 MCU 和 Board 解耦。它适合作为后续 LVGL 项目、工业仪表 UI、Boot/Recovery Screen 的显示基础。

触摸可以在同一方案目录下继续扩展，但应作为独立 Input Device 能力接入，而不是修改 ST7789 Driver 的职责。
