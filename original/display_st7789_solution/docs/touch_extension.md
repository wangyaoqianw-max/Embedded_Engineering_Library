# Touch Extension Design

## 目标

当前 `display_st7789_solution` 只实现显示输出，但方案明确预留触摸输入扩展能力。

ST7789 只负责 LCD Controller，不应承担 Touch Controller 逻辑。显示与触摸在架构上是两个并列设备能力。

## 已知硬件条件

来源 P169H002-CTP 规格资料列出了独立触摸信号：

```text
TP_SCL   Touch clock
TP_SDA   Touch bidirectional data
TP_TRST  Touch reset
TP_TINT  Touch interrupt
```

来源开发板接入文档还记录过一组板级网络：

```text
PA8  -> TP_SCL
PB4  -> TP_SDA
PA15 -> TP_RST
PB2  -> TP_INT
```

这些映射属于来源开发板，不属于本通用方案的固定配置。

当前资料能够确认“模组具备独立触摸接口”，但本 Library 不据此假设具体 Touch Controller 型号。来源参考工程出现过 CST816 代码，只能作为候选参考，不能在未核对实际硬件前直接作为本板控制器结论。

## 推荐架构

未来扩展触摸时保持：

```text
                    GUI / LVGL
                  /            \
                 ▼              ▼
        Display Adapter      Input Adapter
             │                   │
             ▼                   ▼
        ST7789 Display       Touch Service
             │                   │
       SPI + GPIO            Touch Driver
                                 │
                           I2C + RST + INT
```

禁止：

```text
platform_st7789.c
    └─ 直接读取 Touch I2C
```

ST7789 与 Touch Controller 应互相不知道对方存在。

## 推荐未来目录

```text
display_st7789_solution/
├─ st7789/
├─ graphics/
├─ panel/
├─ touch/
│  ├─ platform_touch.h
│  ├─ touch_controller_xxx.c/.h
│  └─ touch_service.c/.h
└─ adapters/
   └─ lvgl/
      ├─ lvgl_display_adapter.c
      └─ lvgl_input_adapter.c
```

其中 `touch/` 当前只是演进方向，不应在未确认控制器和协议前添加伪实现。

## 推荐 Touch Platform Contract

未来可以抽象：

```c
typedef struct
{
    uint16_t x;
    uint16_t y;
    uint8_t pressed;
} platform_touch_point_t;
```

基础能力：

```text
init
deinit
read point
interrupt/poll notification
coordinate transform
optional gesture
```

坐标转换应由 Touch/GUI Adapter 完成，不要污染 ST7789 Controller Driver。

## 与 LVGL 的关系

未来使用 LVGL 时：

```text
LVGL Display flush_cb
    ↓
platform_st7789_write_rgb565()

LVGL indev read_cb
    ↓
Touch Service / Touch Driver
```

这样显示和触摸都能独立替换。例如以后换 LCD Controller，只改 Display Adapter；换触摸 IC，只改 Input Adapter。

## 扩展触摸前必须确认

1. 实际 Touch Controller 型号。
2. I2C 地址与寄存器协议。
3. TP_INT 极性/触发方式。
4. TP_RST 时序。
5. 屏幕旋转与 Touch 坐标对应关系。
6. 多点触控能力。
7. 中断模式还是轮询模式。
8. LVGL Input Device 接口需求。

确认这些事实后再把 Touch Driver 加入本方案。
