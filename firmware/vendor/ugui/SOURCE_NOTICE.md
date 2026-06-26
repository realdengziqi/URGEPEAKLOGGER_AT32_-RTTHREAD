# uGUI 来源说明

- 原始库名称：uGUI / micro GUI
- 上游仓库：https://github.com/achimdoebler/UGUI
- 本次复制来源 commit：ce0bccb5b7d4877c42081419fccadf7aa5727303
- 许可证：见 `LICENSE.md`；源码再分发时必须保留上游版权声明。
- 复制日期：2026-06-20
- 本地修改：
  - `ugui_config.h` 已按本固件工程配置为 RGB565 颜色模式，并启用少量内置 ASCII 字体。
  - 新增 `system.h` 作为 RT-Thread 工程下的平台类型兼容头文件。
  - `ugui.c`、`ugui.h` 和 `LICENSE.md` 从上游复制，未故意修改功能逻辑。
