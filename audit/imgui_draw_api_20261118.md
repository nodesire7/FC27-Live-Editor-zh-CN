# FC27 LE 原生 ImGui 二维码绘制 API 静态定位

目标：`FCLiveEditor.DLL`，SHA256：`0bc6b95bf636c4242cd109ffae9bca04d6542d4b2b88cb3d96cdb3e91da5d66d`。只读反汇编，未修改源码。

| 函数 | RVA | 调用 ABI |
|---|---|---|
| ImGui::GetWindowDrawList | `0x26bf60` | No arguments; RAX returns current window DrawList. |
| ImGui::GetCursorScreenPos | `0x26ab30` | RCX is hidden struct-return pointer; RAX=RCX. Explicit out-pointer wrapper avoids return ABI mismatch. |
| ImGui::Dummy | `0x2a5f60` | RCX points to two consecutive float32 values. |
| ImDrawList::AddRectFilled | `0x286660` | RCX=list, RDX=min*, R8=max*, R9D=color; rounding at entry RSP+0x28; flags at entry RSP+0x30. Always pass rounding=0.0f, flags=0 for QR modules. |

```cpp
struct Vec2 { float x, y; };
using GetDrawListFn = void* (__fastcall*)();
using GetCursorFn = Vec2* (__fastcall*)(Vec2* out);
using DummyFn = void (__fastcall*)(const Vec2* size);
using RectFillFn = void (__fastcall*)(void* list, const Vec2* a, const Vec2* b,
                                       unsigned int color, float rounding, int flags);
```

`GetCursorScreenPos` 的原始 ImVec2 返回值使用 RCX 隐藏返回缓冲区；显式 out 参数可准确匹配当前二进制。若直接声明返回仅含两个 float 的普通 POD，编译器可能选择 RAX 返回而产生 ABI 错配。

`AddRectFilled` 已由不透明度检查、圆角分支、6 索引/4 顶点分配与四角顶点写入确认。二维码使用 `rounding=0.0f, flags=0`，白色 `0xFFFFFFFF`、黑色 `0xFF000000`。

先读取屏幕光标，绘制含四模块留白的白底及黑色模块，再调用 `Dummy` 占据完整正方形。采用整数像素边界。调用必须位于原 ImGui 帧内及成功 Begin 内；Begin/End配对。

全局 context 指针槽为 `base+0xA50E20`；窗口指针在 context+0x4140。JSON 保存各函数16字节签名、完整相关指令与字段偏移。仅已静态识别，仍需主任务游戏内验证实际显示。

## 补充：居中

`ImGui::SetNextWindowPos` RVA `0x27BB60`，ABI `void(const Vec2* position, int condition, const Vec2* pivot)`，分别使用 RCX/EDX/R8。在 Begin 前传入视口中心与 pivot=(0.5f,0.5f)即可居中。函数写入明确的 NextWindowData PosVal/PosPivotVal/PosCond，见 JSON 指令。
