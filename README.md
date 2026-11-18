# FC 27 Live Editor 简体中文补丁

版本 **v2026.11.18.4**，适配 Live Editor 27.2.0。汉化作者 **A-FEI-**。可在 [GBTGame 论坛](https://bbs.gbtgame.me/) 免费获取。

[下载简洁版 Windows 安装包（EXE，20.7 MB）](https://github.com/nodesire7/FC27-Live-Editor-zh-CN/raw/61caab9cab852ade6e2b87516fc3a36bee343917/downloads/%E3%80%90LE%E7%AE%80%E4%BD%93%E4%B8%AD%E6%96%87%E6%B1%89%E5%8C%96%E5%8D%95%E7%8B%AC%E8%A1%A5%E4%B8%81%E3%80%91GBTGame-FC27-LE-27.2.0-zh-CN-Compact-Setup.exe) · [汉化进度与全部词条](catalog/README.md) · [剩余编号](catalog/pending.md) · [开发代码](CODE/)

## 安装

适配游戏版本 **1.0.142.20665**。安装包包含 Live Editor 27.2.0 核心、配套偏移数据及简体中文资源，需要已有可用的 LE 加载环境。

1. 退出游戏，双击下载的 `【LE简体中文汉化单独补丁】GBTGame-FC27-LE-27.2.0-zh-CN-Compact-Setup.exe`。
2. 确认窗口中的游戏目录。若路径为空或不正确，点击「浏览」，选择包含 `FC27.exe` 的文件夹。
3. 点击「安装补丁」，看到「安装完成」后点击「完成并关闭」。
4. 沿用原来的方式启动 LE，在 LE 语言设置中选择 **English (eng_us)**，即可使用简体中文词表。

安装程序覆盖同名文件前，会将旧文件按原目录结构保存到游戏目录下的 `FC27LEzh_backups`。如需恢复旧文件，先退出游戏，再将对应备份中的内容复制回游戏目录。

## 安装内容

| 文件 | 用途 |
|---|---|
| FCLiveEditor.DLL | Live Editor 27.2.0 核心 |
| le_offsets.json | 对应版本的偏移数据 |
| FC27LEzh.dll | 游戏内字体、文本与署名窗口 |
| FC27LEzh_font.b85 | 中文与图标字体 |
| FC27LEzh_ranges.bin | 字体范围 |
| loc/eng_us/localize.json | 全部 4,316 条显示文本 |
| FC27LEzh_licenses/ | 霞鹜文楷与 Font Awesome 许可证 |

安装包 SHA-256 为 `30db90768ecc18266542f90b0a5f1b67714ceafd782c6e226f4db5ff71a4a17c`。

## 进度

- 词表 4,316 条。内容覆盖配置键、比赛风格与特性组名称、阵型位置偏移提示。
- 已识别编辑字段 268 / 268 具备短名及提示，表格字段 252 / 252 具备表头及提示。156 个显示引用和 8 个内联文本位置已列入清单。
- 349 个选项仍以编号显示。游戏动态内容总量尚未确定，详见[完整进度](catalog/README.md)。
- 修正游戏弹窗的绘制时机，将其移到 LE 启用汉化字体且处理输入的对话框阶段，支持按钮、窗口关闭与 Esc。原 DLL 的显示位置和补丁已做静态验证，实际游戏中的点击仍待复核。

## 开发

所有源码、构建、安装及验证脚本位于 [CODE](CODE/)。从仓库根目录可运行 `python CODE/verify.py` 和 `python CODE/make_localize2.py`。在 x64 Native Tools 命令行执行 `CODE\build.bat` 重建汉化 DLL，`CODE\test.bat "游戏目录"` 运行静态补丁与关闭行为测试。

组件许可证见 [third_party](third_party/)。汉化字体包含霞鹜文楷与 Font Awesome。

所支持原版 FCLiveEditor.DLL 的 SHA-256 为 `0bc6b95bf636c4242cd109ffae9bca04d6542d4b2b88cb3d96cdb3e91da5d66d`。
