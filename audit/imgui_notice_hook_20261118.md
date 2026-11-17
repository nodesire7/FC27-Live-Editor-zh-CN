# LE 内置 ImGui 提示弹窗接入审计

目标 SHA-256：`0bc6b95bf636c4242cd109ffae9bca04d6542d4b2b88cb3d96cdb3e91da5d66d`

安全接入：将 RVA `0x340E9D` 的 `E8 FE 57 F3 FF` 替换为无参 void wrapper 调用。wrapper 在原 Render（`0x2766A0`）前绘制弹窗，随后无条件调用原 Render 一次。无需 detour 函数入口。

同一 Present 调用链已执行 DX12/Win32 NewFrame、ImGui NewFrame、LE 自身 GUI。接入继承原渲染线程及当前上下文（全局槽 `0xA50E20`）；不可从 DLL 工作线程绘制。

初始化完成旗标 `0xA51A5D`：原 Present 结束路径在计数到 75 时调用 UIInitDoneNotify 并置 1。接入位于该路径前，因此第 76 个成功帧起可显示。可额外检查 context 非空与 context+0x40A8 的 WithinFrameScope。此旗标只表示 UI 初始化，不表示球员数据库加载。

## 函数 / ABI

| API | RVA | ABI |
|---|---|---|
| Render | 0x2766a0 | `void()` |
| NewFrame | 0x2744e0 | `void()` |
| GetCurrentContext | 0x26aa80 | `void*()` |
| GetIO | 0x26b180 | `void*()` |
| GetFontSize | 0x26abb0 | `float()` |
| Begin | 0x2583f0 | `bool(const char* name, bool* open, int flags)` |
| End | 0x268170 | `void()` |
| TextUnformatted | 0x2b12b0 | `void(const char* text, const char* text_end)` |
| Text | 0x2b0cb0 | `void(const char* format, ...)` |
| Button | 0x29fe00 | `bool(const char* label, const Vec2* size)` |
| OpenPopup | 0x275500 | `void(const char* id, int popup_flags)` |
| BeginPopupModal | 0x25c9c0 | `bool(const char* name, bool* open, int window_flags)` |
| EndPopup | 0x269530 | `void()` |
| CloseCurrentPopup | 0x25dde0 | `void()` |
| SameLine | 0x27a2b0 | `void(float offset_from_start_x, float spacing)` |
| SetNextWindowPos | 0x27bb60 | `void(const Vec2* pos, int cond, const Vec2* pivot)` |
| SetNextWindowSize | 0x27bbd0 | `void(const Vec2* size, int cond)` |
| GetCursorScreenPos | 0x26ab30 | `Vec2* (Vec2* result)` |
| GetWindowDrawList | 0x26bf60 | `void*()` |
| Dummy | 0x2a5f60 | `void(const Vec2* size)` |
| AddRectFilled | 0x286660 | `void(void* drawlist,const Vec2* min,const Vec2* max,unsigned color,float rounding,int flags)` |

所有入口 16 字节签名保存在 JSON 的 apis，供固定版本匹配。

`0x2B0CB0` 是可变参数 Text(format, ...)，不是 TextWrapped。任意 UTF-8 内容使用 `0x2B12B0` TextUnformatted(text, nullptr)，防止百分号被解释为格式符。

居中：GetIO 返回对象 +8/+12 为 DisplaySize，Win32 NewFrame 从 GetClientRect 宽高写入，有指令交叉证据；SetNextWindowPos 使用一半 DisplaySize 和 pivot(0.5,0.5)。

本轮只读静态验证；未安装 hook、未改变源码、未实测新弹窗。全 .text 仅发现一处直调 Render，未证明 DX11 分支存在。
