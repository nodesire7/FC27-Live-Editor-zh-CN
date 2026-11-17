# GBTGame FC 27 启动器与 Live Editor 简体中文补丁

版本 **v2026.11.18.3**，适配 Live Editor 27.2.0。汉化作者 **A-FEI-**。可在 [GBTGame 论坛](https://bbs.gbtgame.me/) 免费获取。

[下载运行包](https://github.com/nodesire7/FC27-Live-Editor-zh-CN/releases/tag/v2026.11.18.3) · [汉化进度与全部词条](catalog/README.md) · [剩余编号](catalog/pending.md) · [开发代码](CODE/)

## 安装

退出游戏，将运行包内 **Game** 文件夹的内容复制到含有 FC27.exe 的目录。已有补丁可先备份同名文件。保留随包的字体与组件许可证。使用现有汉化 DLL 加载配置，词表使用 English (eng_us) 路径。游戏目录只需以下文件。

| 文件 | 用途 |
|---|---|
| GBTGame.FC27.exe | 独立中文界面、启动选择与设置 |
| FC27LEzh.dll | 游戏内字体、文本与署名窗口 |
| FC27LEzh_font.b85 | 中文与图标字体 |
| FC27LEzh_ranges.bin | 字体范围 |
| loc/eng_us/localize.json | 全部 4,316 条显示文本 |

## 启动与设置

启动器使用论坛官方 Logo 和无系统标题栏的窗口。拖动顶部可移动窗口，右上角 × 可关闭。运行 GBTGame.FC27.exe，默认游戏入口以游戏目录为工作目录启动 FC27.exe。LE 模式可选择本机已有的 LE 快捷方式或程序。“记住启动模式”保存到当前用户的 LocalAppData/GBTGame/FC27。LE 模式首次使用需要有效的外部入口；本版没有独立实现 LE 加载后端。

“编辑器设置”和“游戏功能”读取 C:/FC 27 Live Editor 的 le_config.json 和 le_misc_features.json（如系统盘不同则跟随系统盘）。可修改显示、日志、目录、快捷键现有值及比赛、生涯等选项。保存前检查外部修改并备份原文件。自定义程序注入及进程目标选项未集成。

## 进度

- 词表 4,316 条。本版新增 49 条，调整 45 条，覆盖配置键、比赛风格与特性组名称、阵型位置偏移提示。
- 已识别编辑字段 268 / 268 具备短名及提示，表格字段 252 / 252 具备表头及提示。156 个显示引用和 8 个内联文本位置已列入清单。
- 349 个选项仍以编号显示。游戏动态内容总量尚未确定，详见[完整进度](catalog/README.md)。
- 修正游戏弹窗的绘制时机，将其移到 LE 启用汉化字体且处理输入的对话框阶段，支持按钮、窗口关闭与 Esc。原 DLL 的显示位置和补丁已做静态验证，实际游戏中的点击仍待复核。

## 开发

所有源码、构建、安装及验证脚本位于 [CODE](CODE/)。从仓库根目录可运行 `python CODE/verify.py` 和 `python CODE/make_localize2.py`。在 x64 Native Tools 命令行执行 `CODE\build.bat` 重建汉化 DLL，`CODE\test.bat "游戏目录"` 运行静态补丁与关闭行为测试。

启动器的 Win32 / DX11 窗口来自 [上游 MIT 源码](https://github.com/xAranaktu/FIFA-Live-Editor-Launcher/tree/283ebb9ebd0c5958d769ae4d3d89b8bcfd9c54df)。检出该提交，然后在 x64 Native Tools 中执行 `CODE\launcher\build.bat "上游源码目录"`，生成仓库根目录的 GBTGame.FC27.exe。使用该提交内的 Dear ImGui 和 nlohmann/json。组件许可证见 [third_party](third_party/)。汉化字体包含霞鹜文楷与 Font Awesome。

所支持原版 FCLiveEditor.DLL 的 SHA-256 为 `0bc6b95bf636c4242cd109ffae9bca04d6542d4b2b88cb3d96cdb3e91da5d66d`。
