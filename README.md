# FC 27 Live Editor 简体中文补丁

版本 **v2026.11.18.4**，适配 Live Editor 27.2.0。汉化作者 **A-FEI-**。可在 [GBTGame 论坛](https://bbs.gbtgame.me/) 免费获取。

[下载运行包](https://github.com/nodesire7/FC27-Live-Editor-zh-CN/releases/tag/v2026.11.18.4) · [汉化进度与全部词条](catalog/README.md) · [剩余编号](catalog/pending.md) · [开发代码](CODE/)

## 安装

退出游戏，将运行包内 **Game** 文件夹的内容复制到含有 FC27.exe 的目录。已有补丁可先备份同名文件。保留随包的字体与组件许可证。使用现有汉化 DLL 加载配置，词表使用 English (eng_us) 路径。游戏目录只需以下文件。

| 文件 | 用途 |
|---|---|
| FC27LEzh.dll | 游戏内字体、文本与署名窗口 |
| FC27LEzh_font.b85 | 中文与图标字体 |
| FC27LEzh_ranges.bin | 字体范围 |
| loc/eng_us/localize.json | 全部 4,316 条显示文本 |

## 进度

- 词表 4,316 条。内容覆盖配置键、比赛风格与特性组名称、阵型位置偏移提示。
- 已识别编辑字段 268 / 268 具备短名及提示，表格字段 252 / 252 具备表头及提示。156 个显示引用和 8 个内联文本位置已列入清单。
- 349 个选项仍以编号显示。游戏动态内容总量尚未确定，详见[完整进度](catalog/README.md)。
- 修正游戏弹窗的绘制时机，将其移到 LE 启用汉化字体且处理输入的对话框阶段，支持按钮、窗口关闭与 Esc。原 DLL 的显示位置和补丁已做静态验证，实际游戏中的点击仍待复核。

## 开发

所有源码、构建、安装及验证脚本位于 [CODE](CODE/)。从仓库根目录可运行 `python CODE/verify.py` 和 `python CODE/make_localize2.py`。在 x64 Native Tools 命令行执行 `CODE\build.bat` 重建汉化 DLL，`CODE\test.bat "游戏目录"` 运行静态补丁与关闭行为测试。

组件许可证见 [third_party](third_party/)。汉化字体包含霞鹜文楷与 Font Awesome。

所支持原版 FCLiveEditor.DLL 的 SHA-256 为 `0bc6b95bf636c4242cd109ffae9bca04d6542d4b2b88cb3d96cdb3e91da5d66d`。
