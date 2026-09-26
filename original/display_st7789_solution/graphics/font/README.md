# Font Asset Boundary

本目录只保留 `platform_font_t` 字体描述接口，不收录来源工程中的 `platform_font_ascii_8x16.c/.h` 位图数据。

原因：来源工程设计文档明确把 Vendor LCD 参考文件中的 ASCII 字模作为字模资源来源，但当前仓库没有可独立确认的原始字模许可证。为避免把来源不明确的位图数据重新标记为原创，本方案要求项目自行提供字体资源。

推荐扩展方向：

- 项目自有 ASCII / 数字点阵。
- 明确许可证的开源字体转换资源。
- 外部 W25Qxx 字库。
- UTF-8 / 中文字库 Service。
- LVGL 自带或生成字体。

`platform_graphics.c` 当前仍要求 8×16、0x20~0x7E 的单色 ASCII Font Contract；后续若需要多字号/UTF-8，应再抽象 Graphics Font Backend。
