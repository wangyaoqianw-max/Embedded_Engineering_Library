# 第三方资产

本目录保存原样收录、版本化归档或仅进行配置级调整的第三方代码资产。第三方源码默认只读，项目级 Port/Adapter 应放在 `adapted/` 或 `original/`，不要混入第三方本体。

## 当前资产

| 资产 | 当前版本 | 直接来源 | 许可证/条件 |
| --- | --- | --- | --- |
| LVGL | v8.3 / 8.3.11 | 开发板商家裁剪包 | MIT |
| CmBacktrace | v1.5.0 | 已验证 OTA Vendor Snapshot | MIT |
| EasyLogger | v2.2.99 | 已验证 OTA Vendor Snapshot | MIT |
| SEGGER RTT | v7.92 | 已验证 OTA Vendor Snapshot | SEGGER 源文件头再分发条件 |

## 版本目录规则

推荐结构：

```text
third_party/
├─ LVGL/
│  ├─ README.md
│  ├─ v8.3/
│  └─ v9.4/
├─ CmBacktrace/
│  └─ v1.5.0/
├─ EasyLogger/
│  └─ v2.2.99/
└─ SEGGER_RTT/
   └─ v7.92/
```

升级第三方库时优先新增版本目录，不直接覆盖已被项目验证过的旧版本。每项资产 README 应记录上游来源、精确版本、许可证、直接来源快照、配置差异和验证边界。

收录第三方资产不改变其版权归属，也不表示仓库维护者为其完整性、安全性或适用性提供保证。
