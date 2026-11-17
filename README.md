# FC 27 Live Editor 简体中文补丁

适配本仓库记录的 **FCLiveEditor.DLL 27.2.0**。提供中文词表、中文字库及显示文本补丁源码。

当前版本：`v2026.11.17.1`。包含 3,961 条译文、151 处显示引用补丁和 8 处内联文本补丁，覆盖属性矩阵、合同、比赛风格、球员/球队/经理表头、快捷键及多级设置。

## 安装

1. 在 Releases 下载补丁压缩包并解压，退出游戏。
2. 执行安装脚本，传入实际游戏目录：

   ```powershell
   powershell -ExecutionPolicy Bypass -File .\install.ps1 -GameDirectory "你的游戏目录"
   ```

3. 通过已有的 DLL 加载方式加载 `FC27LEzh.dll`，重新启动游戏。已经使用本补丁旧版的用户可沿用原加载配置。

安装脚本核对原版 LE 文件哈希并备份现有补丁文件，然后复制下列四个文件。它不更改加载器配置，也不包含游戏本体、原版 Live Editor DLL、启动器或账号信息。

```text
FC27LEzh.dll
FC27LEzh_font.b85
FC27LEzh_ranges.bin
loc/eng_us/localize.json
```

补丁使用 `eng_us` 词表路径；如果 LE 选择了其他语言，需要先切回 English。字库与范围表必须成对使用。恢复安装前版本时，退出游戏，将 `FC27LEzh_backups` 中对应日期目录的文件按原路径复制回去。

## 适配与验证

原版 `FCLiveEditor.DLL` 的 SHA-256：

```text
0bc6b95bf636c4242cd109ffae9bca04d6542d4b2b88cb3d96cdb3e91da5d66d
```

- 静态核对 300 处表格字段注册，共 252 个唯一字段，表头及悬浮说明缺失均为 0。
- 将原 DLL 映射为普通数据，验证全部显示补丁、显式 UTF-8 长度及签名不匹配时拒绝修改。该测试不会执行原 DLL。
- 独立构建 ImGui 字体图集，检查译文与显示补丁所需的 820 个码点，均有字形。

本次发布经过静态与独立测试，**尚未进行游戏内逐页验证**。部分枚举仅能从 DLL 确认编号，显示为中文类别加原编号；教练组 0–3 同样保留原编号。无法据此宣称所有运行时文本均已覆盖。日志、脚本 API、内部数据库键和隐藏控件 ID 不做全局替换。

详见 [DLL 核对记录](audit/DLL核对记录.md) 和 [验证统计](audit/validation_summary.json)。升级 LE 后需重新核对 RVA 和指令，不能直接套用其他版本。

## 重建

使用 Windows x64、Visual Studio C++ 工具和 Python 3。在 x64 Native Tools Command Prompt 中运行：

```bat
build.bat
python make_localize2.py --output loc/eng_us/localize.json
test.bat "你的游戏目录"
python verify.py --game-dir "你的游戏目录"
```

`test.bat` 运行内存补丁测试；`verify.py` 核对版本、词表覆盖及发布文件哈希。

字库由霞鹜文楷 1.530 与 Font Awesome Free 6.7.2 合并生成，随包提供可直接使用的产物。重新生成需要 `fonttools`、对应字体和 ImGui 1.90.6 的 `binary_to_compressed_c`：

```powershell
python -m pip install fonttools
python make_ranges2.py --font LXGWWenKai-Regular.ttf --icons fa-solid-900.ttf --encoder binary_to_compressed_c.exe --localize loc/eng_us/localize.json --output-dir .
```

重新生成或修改文件后，原发布版的 `checksums.json` 将不再匹配，需重新验证并更新校验清单。

## 第三方来源

- [FC 27 Live Editor](https://github.com/xAranaktu/FC-27-Live-Editor)：独立上游项目，本仓库不是官方版本。
- [霞鹜文楷](https://github.com/lxgw/LxgwWenKai)：字体许可证见 `third_party/font_licenses/LXGW-WenKai-1.530.txt`。
- [Font Awesome Free](https://github.com/FortAwesome/Font-Awesome)：字体许可证见 `third_party/font_licenses/Font-Awesome-Free-6.7.2.txt`。
- [Dear ImGui](https://github.com/ocornut/imgui)：字体压缩工具和独立图集验证使用上游工具。

发布包不包含系统商业字体。第三方组件适用其各自许可证。
