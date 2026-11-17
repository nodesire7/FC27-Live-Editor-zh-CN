// FC27LEzh.dll replaces the primary ImGui font used by the FC 27 LE 27.2.0 overlay.
// LE still merges its bundled supplemental fonts into the primary font.
// External assets: little-endian uint16 glyph ranges and ImGui base85/stb font data.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#pragma warning(disable: 4996)

static HANDLE  g_log = INVALID_HANDLE_VALUE;
static SRWLOCK g_log_lock = SRWLOCK_INIT;
static wchar_t g_dir[MAX_PATH] = {};

static void Log(const char* fmt, ...)
{
    char body[1024];
    va_list ap; va_start(ap, fmt);
    _vsnprintf_s(body, sizeof(body), _TRUNCATE, fmt, ap);
    va_end(ap);

    SYSTEMTIME st; GetLocalTime(&st);
    char line[1200];
    _snprintf_s(line, sizeof(line), _TRUNCATE, "[%02d:%02d:%02d.%03d] %s\r\n",
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, body);
    if (g_log == INVALID_HANDLE_VALUE) { OutputDebugStringA(line); return; }
    AcquireSRWLockExclusive(&g_log_lock);
    DWORD written = 0; WriteFile(g_log, line, (DWORD)strlen(line), &written, nullptr);
    ReleaseSRWLockExclusive(&g_log_lock);
}

static void* ReadWholeFile(const wchar_t* path, size_t* outSize)
{
    HANDLE h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h == INVALID_HANDLE_VALUE) return nullptr;
    LARGE_INTEGER sz; GetFileSizeEx(h, &sz);
    void* buf = malloc((size_t)sz.QuadPart + 1);
    if (!buf) { CloseHandle(h); return nullptr; }
    DWORD rd = 0;
    BOOL ok = ReadFile(h, buf, (DWORD)sz.QuadPart, &rd, nullptr);
    CloseHandle(h);
    if (!ok) { free(buf); return nullptr; }
    ((char*)buf)[rd] = 0;
    if (outSize) *outSize = rd;
    return buf;
}

// allocate `size` bytes within +-2GB of `near`
static void* AllocNear(void* nearAddr, size_t size)
{
    uintptr_t base = (uintptr_t)nearAddr & ~(uintptr_t)0xFFFF;
    for (uintptr_t delta = 0x100000; delta < 0x60000000ULL; delta += 0x100000)
    {
        for (int dir = 0; dir < 2; ++dir)
        {
            uintptr_t hint = dir == 0 ? base + delta : base - delta;
            if (hint < 0x10000) continue;
            void* p = VirtualAlloc((void*)hint, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (!p) continue;
            int64_t d = (int64_t)((uintptr_t)p - (uintptr_t)nearAddr);
            if (d > -(int64_t)0x70000000 && d < (int64_t)0x70000000) return p;
            VirtualFree(p, 0, MEM_RELEASE);
        }
    }
    return nullptr;
}

static bool PatchFont(HMODULE le)
{
    // Primary AddFontFromMemoryCompressedTTF call in the overlay font setup.
    // Verify the whole sequence before replacing any of the three operands.
    constexpr uintptr_t siteRva = 0x46E917;
    constexpr uint8_t expected[] = {
        0x48, 0x8D, 0x05, 0x82, 0xC6, 0x4A, 0x00,
        0x48, 0x89, 0x44, 0x24, 0x28, 0x48, 0x8D, 0x44, 0x24, 0x40,
        0x48, 0x89, 0x44, 0x24, 0x20, 0xF3, 0x0F, 0x10, 0x5C, 0x24, 0x54,
        0x41, 0xB8, 0x18, 0xC3, 0x01, 0x00,
        0x48, 0x8D, 0x15, 0xD0, 0x36, 0x1E, 0x00,
        0x49, 0x8B, 0x4F, 0x30, 0xE8, 0x87, 0x66, 0xE1, 0xFF
    };

    auto* site = reinterpret_cast<uint8_t*>(le) + siteRva;
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(site, &region, sizeof(region)) || region.State != MEM_COMMIT ||
        region.AllocationBase != le ||
        reinterpret_cast<uintptr_t>(site) + sizeof(expected) >
            reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize ||
        memcmp(site, expected, sizeof(expected)) != 0)
    {
        Log("primary font call signature mismatch");
        return false;
    }

    size_t rangesSize = 0, fontSize = 0;
    wchar_t rangesPath[MAX_PATH], fontPath[MAX_PATH];
    _snwprintf_s(rangesPath, _countof(rangesPath), _TRUNCATE, L"%s\\FC27LEzh_ranges.bin", g_dir);
    _snwprintf_s(fontPath, _countof(fontPath), _TRUNCATE, L"%s\\FC27LEzh_font.b85", g_dir);

    auto* ranges = static_cast<uint16_t*>(ReadWholeFile(rangesPath, &rangesSize));
    auto* font = static_cast<uint8_t*>(ReadWholeFile(fontPath, &fontSize));
    if (!ranges || rangesSize < 6 || rangesSize % 4 != 2 ||
        ranges[rangesSize / 2 - 1] != 0 || !font || fontSize < 20 || fontSize % 5 ||
        fontSize / 5 * 4 > INT32_MAX)
    {
        Log("primary font assets missing or malformed");
        free(ranges);
        free(font);
        return false;
    }

    const int32_t decodedSize = static_cast<int32_t>(fontSize / 5 * 4);
    auto* block = static_cast<uint8_t*>(AllocNear(le, rangesSize + decodedSize));
    if (!block)
    {
        Log("primary font allocation failed");
        free(ranges);
        free(font);
        return false;
    }

    auto* compressed = block + rangesSize;
    memcpy(block, ranges, rangesSize);
    for (size_t i = 0; i < fontSize; i += 5)
    {
        uint32_t word = 0;
        for (int j = 4; j >= 0; --j)
        {
            const uint8_t ch = font[i + j];
            if (ch < '#' || ch > 'x' || ch == 92)
            {
                Log("primary font contains invalid base85 at %zu", i + j);
                VirtualFree(block, 0, MEM_RELEASE);
                free(ranges);
                free(font);
                return false;
            }
            word = word * 85 + ch - (ch >= 92 ? 36 : 35);
        }
        memcpy(compressed + i / 5 * 4, &word, sizeof(word));
    }
    free(ranges);
    free(font);

    const auto rangeDelta = reinterpret_cast<intptr_t>(block) - reinterpret_cast<intptr_t>(site + 7);
    const auto fontDelta = reinterpret_cast<intptr_t>(compressed) - reinterpret_cast<intptr_t>(site + 41);
    if (rangeDelta < INT32_MIN || rangeDelta > INT32_MAX ||
        fontDelta < INT32_MIN || fontDelta > INT32_MAX)
    {
        Log("primary font operands out of range");
        VirtualFree(block, 0, MEM_RELEASE);
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(site, sizeof(expected), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        Log("primary font call protection change failed: %lu", GetLastError());
        VirtualFree(block, 0, MEM_RELEASE);
        return false;
    }
    const auto rangeDisp = static_cast<int32_t>(rangeDelta);
    const auto fontDisp = static_cast<int32_t>(fontDelta);
    memcpy(site + 3, &rangeDisp, sizeof(rangeDisp));
    memcpy(site + 30, &decodedSize, sizeof(decodedSize));
    memcpy(site + 37, &fontDisp, sizeof(fontDisp));
    DWORD ignored = 0;
    VirtualProtect(site, sizeof(expected), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), site, sizeof(expected));
    Log("primary font call PATCHED, ranges=%zu bytes, compressed=%d bytes", rangesSize, decodedSize);
    return true;
}

struct Label {
    uintptr_t instruction;
    uintptr_t original;
    uint8_t operand;
    const char* replacement;
    // Some C++ strings carry an explicit byte length alongside the pointer.
    uintptr_t lengthImmediate = 0;
    uint32_t expectedLength = 0;
    bool includeTerminator = false;
};

template <size_t Count>
static void PatchLabels(HMODULE le, const char* group, const Label (&labels)[Count])
{
    auto* module = reinterpret_cast<uint8_t*>(le);
    uintptr_t begin = labels[0].instruction;
    uintptr_t end = begin + 7;
    for (const Label& label : labels)
    {
        if (label.instruction < begin) begin = label.instruction;
        if (label.instruction + 7 > end) end = label.instruction + 7;
        if (label.lengthImmediate)
        {
            if (label.lengthImmediate < begin) begin = label.lengthImmediate;
            if (label.lengthImmediate + 4 > end) end = label.lengthImmediate + 4;
        }
    }
    auto* first = module + begin;
    const size_t span = end - begin;
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(first, &region, sizeof(region)) || region.State != MEM_COMMIT ||
        region.AllocationBase != le ||
        reinterpret_cast<uintptr_t>(first) + span >
            reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize)
    {
        Log("%s label code region unavailable", group);
        return;
    }

    size_t bytes = 0;
    for (const Label& label : labels)
    {
        const uint8_t* site = module + label.instruction;
        int32_t disp;
        memcpy(&disp, site + 3, sizeof(disp));
        if (site[0] != 0x48 || site[1] != 0x8D || site[2] != label.operand ||
            static_cast<intptr_t>(label.instruction + 7) + disp != label.original)
        {
            Log("%s label signature mismatch at RVA 0x%llX", group,
                static_cast<unsigned long long>(label.instruction));
            return;
        }
        bytes += strlen(label.replacement) + 1;
        if (label.lengthImmediate)
        {
            uint32_t length;
            memcpy(&length, module + label.lengthImmediate, sizeof(length));
            if (length != label.expectedLength)
            {
                Log("%s label length signature mismatch at RVA 0x%llX", group,
                    static_cast<unsigned long long>(label.lengthImmediate));
                return;
            }
        }
    }

    auto* block = static_cast<char*>(AllocNear(le, bytes));
    if (!block)
    {
        Log("%s label allocation failed", group);
        return;
    }

    int32_t displacements[_countof(labels)]{};
    char* next = block;
    for (size_t index = 0; index < _countof(labels); ++index)
    {
        const Label& label = labels[index];
        const auto delta = reinterpret_cast<intptr_t>(next) -
                           reinterpret_cast<intptr_t>(module + label.instruction + 7);
        if (delta < INT32_MIN || delta > INT32_MAX)
        {
            VirtualFree(block, 0, MEM_RELEASE);
            return;
        }
        displacements[index] = static_cast<int32_t>(delta);
        const size_t length = strlen(label.replacement) + 1;
        memcpy(next, label.replacement, length);
        next += length;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(first, span, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        Log("%s label protection change failed: %lu", group, GetLastError());
        VirtualFree(block, 0, MEM_RELEASE);
        return;
    }

    for (size_t index = 0; index < _countof(labels); ++index)
    {
        memcpy(module + labels[index].instruction + 3, &displacements[index], sizeof(int32_t));
        const Label& label = labels[index];
        if (label.lengthImmediate)
        {
            const uint32_t length = static_cast<uint32_t>(strlen(label.replacement)) +
                                    (label.includeTerminator ? 1 : 0);
            memcpy(module + label.lengthImmediate, &length, sizeof(length));
        }
    }

    DWORD ignored = 0;
    VirtualProtect(first, span, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), first, span);
    Log("%s labels PATCHED: %zu", group, _countof(labels));
}

// These settings labels go directly to ImGui, bypassing LE's translation map.
static void PatchSettingsLabels(HMODULE le)
{
    static constexpr char background[] = "{}档背景颜色";
    static constexpr char text[] = "{} 档文字颜色";
    static constexpr char threshold[] = "{} 档能力值最低值";
    static_assert(sizeof(background) - 1 == 0x11);
    static_assert(sizeof(text) - 1 == 0x12);
    static_assert(sizeof(threshold) - 1 == 0x18);

    constexpr Label labels[] = {
        {0x4AB68A, 0x91DC20, 0x05, background},
        {0x4AB6D7, 0x91DC38, 0x05, text},
        {0x4AB724, 0x91DC50, 0x05, threshold},
        {0x4ABA7E, 0x63099C, 0x0D, "界面###ui"},
        {0x4ABA94, 0x91DC70, 0x0D, "能力值颜色###attr_colors"},
        {0x4ABAAF, 0x91DC7C, 0x15, "顶级"},
        {0x4ABAC2, 0x91DC88, 0x15, "优秀"},
        {0x4ABADB, 0x91DC94, 0x15, "良好"},
        {0x4ABAF4, 0x91DCA0, 0x15, "普通"},
        {0x4ABB0D, 0x91DCA8, 0x15, "较差"},
        {0x4ABB48, 0x6321F0, 0x0D, "字号###font_size"},
        {0x4ABC12, 0x91DCC8, 0x0D, "新建快捷键###Create New Hotkey"},
        {0x4ABC59, 0x91DCE0, 0x0D, "点击新建快捷键"},
        {0x4ABC74, 0x6309D0, 0x0D, "其他###other"},
        {0x4ABE6E, 0x91DBF8, 0x0D, "点击编辑快捷键"},
    };

    PatchLabels(le, "settings", labels);
}

static void PatchPlayerEditorLabels(HMODULE le)
{
    // Literal C strings passed to ImGui controls or their tooltips.
    constexpr Label bulkEdit[] = {
        {0x41B494, 0x64E260, 0x0D, "批量创建 100 名球员，随机生成姓名、外观和能力值。"},
        {0x41C40C, 0x64E300, 0x15, "按字段名搜索"},
    };
    constexpr Label tiers[] = {
        {0x421637, 0x64E540, 0x0D, "默认值与白金档相同"},
        {0x42174F, 0x64E5C8, 0x0D, "潜力范围"},
        {0x42182B, 0x64E690, 0x0D, "主要能力值提升"},
        {0x4218FF, 0x64E7C8, 0x0D, "次要能力值提升"},
        {0x4219D3, 0x64E858, 0x0D, "门将主要能力值提升"},
        {0x421AA7, 0x64E8E8, 0x0D, "门将次要能力值提升"},
        {0x421B5A, 0x64E960, 0x0D, "分配给球员的核心比赛风格数量，因球员位置而异。"},
        {0x421B7B, 0x64E9C0, 0x0D, "核心比赛风格数量"},
        {0x421C2E, 0x64EA30, 0x0D, "分配给球员的附加比赛风格和特性数量，因球员位置而异。"},
        {0x421C4F, 0x64EAA0, 0x0D, "附加比赛风格数量"},
        {0x421D27, 0x64EB38, 0x0D, "逆足始终五星###Always Best Weak Foot"},
        {0x421D8B, 0x64EB78, 0x0D, "花式动作始终五星###Always Best Skillmoves"},
    };
    PatchLabels(le, "bulk player editor", bulkEdit);
    PatchLabels(le, "player tiers", tiers);
}

static void PatchDialogLabels(HMODULE le)
{
    constexpr Label fileDialog[] = {
        {0x2458EC, 0x636828, 0x0D, "新建目录"},
        {0x245B53, 0x636858, 0x0D, "取消###Cancel"},
        {0x245CA8, 0x636860, 0x0D, "返回当前目录"},
        {0x245CE0, 0x636880, 0x0D, "驱动器###Devices"},
        {0x245D02, 0x636880, 0x0D, "打开驱动器列表"},
        {0x251C35, 0x636A18, 0x0D, "确认###Confirm"},
        {0x251C71, 0x636858, 0x0D, "取消###Cancel"},
        {0x252522, 0x636920, 0x0D, "取消###Cancel##validationdialog"},
        {0x253211, 0x636940, 0x0D, "文件名"},
        {0x253523, 0x6366E0, 0x0D, "清除###R##BtnImGuiFileDialogSearchField"},
        {0x25357E, 0x636708, 0x0D, "清除搜索条件"},
        {0x25359A, 0x636718, 0x0D, "搜索"},
        {0x253691, 0x636908, 0x0D, "确定###OK##validationdialog"},
    };
    constexpr Label playerDetails[] = {
        {0x3FD45F, 0x64D620, 0x0D, "球员 ID "},
        {0x40A99F, 0x64DAC4, 0x0D, "名称 "},
        {0x41185E, 0x64DD58, 0x0D, "新值 "},
        {0x421726, 0x64E590, 0x0D, "球员可能达到的最低和最高潜力。"},
        {0x42180A, 0x64E610, 0x0D, "常用能力值因场上位置而异。提高这些能力值通常会提高总评。"},
        {0x4219B2, 0x64E828, 0x0D, "提高门将的主要能力值。"},
        {0x421A86, 0x64E8B8, 0x0D, "提高门将的次要能力值。"},
        {0x421D02, 0x64EB10, 0x0D, "所有球员的逆足能力均设为五星。"},
        {0x421D66, 0x64EB50, 0x0D, "所有球员的花式动作能力均设为五星。"},
        {0x4228A0, 0x64EBC0, 0x0D, "将球员导出至预设文件 "},
        {0x4228F1, 0x64EBF8, 0x0D, "输入新预设名称"},
    };
    constexpr Label hotkeyEditor[] = {
        {0x425997, 0x64EDB8, 0x0D, "按键 %d "},
        {0x425AB3, 0x64EDE8, 0x0D, "数值 "},
        {0x425B41, 0x64EDD0, 0x0D, "添加按键###Add key"},
        {0x425B70, 0x64EDD8, 0x0D, "全部重置###Reset all"},
        {0x425BDC, 0x62A000, 0x0D, "删除###DELETE"},
        {0x425C4A, 0x6396E0, 0x0D, "保存###Save"},
        {0x4281FC, 0x64EDF0, 0x0D, "脚本"},
        {0x42906B, 0x64EE90, 0x0D, "操作类型"},
        {0x4290B9, 0x64EEB8, 0x0D, "脚本"},
        {0x4294F0, 0x64EEC8, 0x0D, "名称"},
        {0x429566, 0x64EEE8, 0x0D, "快捷键的唯一名称"},
        {0x42957C, 0x64EF00, 0x0D, "说明"},
        {0x4295DB, 0x64EF28, 0x0D, "快捷键说明"},
        {0x429619, 0x64EDB8, 0x0D, "按键 %d "},
        {0x42972F, 0x64EDD0, 0x0D, "添加按键###Add key"},
        {0x42975F, 0x64EDD8, 0x0D, "全部重置###Reset all"},
        {0x429825, 0x6396E0, 0x0D, "保存###Save"},
    };
    constexpr Label tooltips[] = {
        {0x436918, 0x64FDA0, 0x0D, "随机生成 %s 的能力值"},
        {0x4526EA, 0x650DE8, 0x0D, "在编辑器中打开球员"},
        {0x4542B5, 0x650D68, 0x0D, "在编辑器中打开经理"},
        {0x48EE5F, 0x91C818, 0x0D, "右键导出此目录的文件"},
    };
    PatchLabels(le, "file dialog", fileDialog);
    PatchLabels(le, "player details", playerDetails);
    PatchLabels(le, "hotkey editor", hotkeyEditor);
    PatchLabels(le, "tooltips", tooltips);
}

static void PatchRemainingUiLabels(HMODULE le)
{
    constexpr Label general[] = {
        {0x22464A, 0x6330C0, 0x0D, "第 %d 行有错误"},
        {0x2A334A, 0x639660, 0x0D, "当前颜色"},
        {0x2A33A5, 0x63D870, 0x0D, "原始颜色"},
        {0x2A4803, 0x63DA08, 0x0D, "透明度条###Alpha Bar"},
        {0x3EAAAC, 0x64CC08, 0x0D, "找不到纹理 %s"},
        {0x3EAAC0, 0x64CC08, 0x0D, "找不到纹理 %s"},
        {0x3EAC8C, 0x64CC08, 0x0D, "找不到纹理 %s"},
        {0x3EACA0, 0x64CC08, 0x0D, "找不到纹理 %s"},
        {0x3EAEF7, 0x64CC30, 0x0D, "点击重新加载纹理。也可以在设置中启用自动加载图像，该选项可能导致游戏崩溃。"},
        {0x404919, 0x639710, 0x0D, "保存以下项目的更改？"},
        {0x41D0B4, 0x64E378, 0x0D, "操作"},
        {0x41D1EA, 0x64E460, 0x0D, "暂不支持文本输入"},
        {0x41D31D, 0x64E438, 0x0D, "暂不支持布尔值输入"},
    };
    constexpr Label hotkeys[] = {
        {0x425852, 0x64ED90, 0x0D, "快捷键 %s"},
        {0x425948, 0x64EDA0, 0x0D, "当前组合键 %s"},
        {0x428F25, 0x64EE80, 0x0D, "创建快捷键###Create Hotkey"},
        {0x428F61, 0x64ED90, 0x0D, "快捷键 %s"},
        {0x42905A, 0x64EDA0, 0x0D, "当前组合键 %s"},
    };
    constexpr Label filePreview[] = {
        {0x42CEBA, 0x64F450, 0x0D, "无法修改名称未知的文件。请计算哈希值以确认正确的文件名。"},
        {0x42CFDB, 0x64F3E8, 0x0D, "%6d 行  | %s"},
        {0x42DF88, 0x639648, 0x0D, "选项###Options"},
        {0x42E24A, 0x64F0B0, 0x0D, "显示数据预览###Show Data Preview"},
        {0x42E25A, 0x64F0C8, 0x0D, "显示 HexII###Show HexII"},
        {0x42E26A, 0x64F0D8, 0x0D, "显示 ASCII###Show Ascii"},
        {0x42E282, 0x64F0E8, 0x0D, "将零值显示为灰色###Grey out zeroes"},
        {0x42E292, 0x64F0F8, 0x0D, "十六进制字母大写###Uppercase Hex"},
        {0x42E390, 0x64F418, 0x0D, "该文件不属于图像格式"},
        {0x42E6F3, 0x64F2E0, 0x0D, "计算出的名称哈希 %llu"},
        {0x42EAAD, 0x64F360, 0x0D, "文件 %s"},
        {0x42EC4D, 0x64F380, 0x0D, "大小 %d"},
        {0x42EDDE, 0x64F3A0, 0x0D, "名称哈希 %llu"},
        {0x42EF41, 0x64F3C0, 0x0D, "数据块 %s"},
        {0x42F695, 0x64F168, 0x0D, "预览格式"},
        {0x42F916, 0x64F1A4, 0x0D, "十进制"},
        {0x42FA98, 0x63D984, 0x0D, "十六进制"},
        {0x42FC29, 0x63D984, 0x0D, "十六进制"},
        {0x42FC89, 0x64F1A8, 0x0D, "二进制"},
    };
    constexpr Label menus[] = {
        {0x45CEA8, 0x651490, 0x0D, "球员 %d"},
        {0x4765C0, 0x91B4E0, 0x0D, "不再显示此消息框###Don't show this message box anymore"},
        {0x48D7E4, 0x91BD40, 0x0D, "部分玩法功能会在比赛开始后显示。"},
        {0x48E8F4, 0x91C7F0, 0x0D, "正在加载###LOADING"},
        {0x48E923, 0x91C7B8, 0x0D, "旧版文件###LEGACY"},
        {0x48E9F2, 0x91C7D0, 0x0D, "%d 个文件位于 %s"},
    };
    PatchLabels(le, "general UI", general);
    PatchLabels(le, "hotkey headings", hotkeys);
    PatchLabels(le, "file preview", filePreview);
    PatchLabels(le, "menus", menus);
}

// Audited UI-only references. Preserve widget IDs and explicit UTF-8 byte lengths.
static void PatchDeepUiLabels(HMODULE le)
{
    constexpr Label labels[] = {
        {0x4117D1, 0x64DD40, 0x05, "当前值：{}", 0x4117E0, 18, false},
        {0x41CBA6, 0x64E350, 0x05, "批量编辑 {} 名球员", 0x41CBB5, 20, false},
        {0x4218DE, 0x64E6F0, 0x0D, "提高次要能力值。具体项目因球员位置而异，通常不提高总评，但可改善镇定等次要属性。"},
        {0x425BAC, 0x64ED6C, 0x15, "未设置按键", 0x425BA8, 6, false},
        {0x426CDB, 0x64ED6C, 0x15, "未设置按键"},
        {0x42744F, 0x64ED78, 0x15, "〈尚未分配按键〉", 0x42744B, 17, false},
        {0x42979A, 0x64ED6C, 0x15, "未设置按键", 0x429796, 6, false},
        {0x42996B, 0x64EF40, 0x15, "快捷键名称不能重复", 0x429967, 26, false},
        {0x42AABC, 0x64ED6C, 0x15, "未设置按键"},
        {0x42B1D1, 0x64EF60, 0x15, "快捷键名称不能为空", 0x42B1CD, 26, false},
        {0x42B5C9, 0x64EF80, 0x15, "暂无说明", 0x42B5C5, 14, false},
        {0x42CCD2, 0x64F250, 0x15, "输入完整文件路径以计算哈希值", 0x42CCCE, 38, false},
        {0x42CFFD, 0x64F3F8, 0x15, "文件编辑器###File Editor"},
        {0x451D10, 0x650DE0, 0x05, "{} 岁", 0x451D21, 6, false},
        {0x46EDB5, 0x91AD38, 0x0D, "隐藏实况编辑器###Hide Live Editor"},
        {0x46EE66, 0x91AD94, 0x0D, "使用文档###Wiki"},
        {0x46EE8C, 0x91ADE0, 0x0D, "在线数据库###Online Database"},
        {0x46F763, 0x91ACF0, 0x1D, "提示"},
        {0x46F76C, 0x91ACE8, 0x1D, "错误"},
        {0x46F775, 0x91ACE0, 0x1D, "警告"},
        {0x46F77E, 0x91ACD8, 0x1D, "成功"},
        {0x4765DF, 0x636854, 0x0D, "确定###OK"},
        {0x47661C, 0x636858, 0x0D, "取消###Cancel"},
        {0x48B724, 0x91C3D0, 0x05, "编辑球员档次 {0}###Edit Player Tier Config {0}##1", 0x48B735, 29, false},
        {0x48C73E, 0x91C648, 0x05, "当前选择：{}", 0x48C74F, 19, false},
        {0x48EC8E, 0x91C7F0, 0x0D, "正在加载###LOADING"},
        {0x48ECBA, 0x91C7B8, 0x0D, "旧版文件###LEGACY"},
        {0x48ECEF, 0x6396A8, 0x0D, "文件###File"},
        {0x48ED07, 0x91C7B0, 0x0D, "加载###Load"},
        {0x48F045, 0x91C854, 0x0D, "名称###NAME"},
        {0x48F05C, 0x91C85C, 0x0D, "类型###TYPE"},
        {0x48F073, 0x91C864, 0x0D, "大小###SIZE"},
        {0x48F46F, 0x91C870, 0x05, "文件大小：{} 字节", 0x48F47E, 19, false},
        {0x491E32, 0x91CA60, 0x15, "代码编辑器###Code Editor"},
        {0x468165, 0x651E60, 0x0D, "左键单击编辑\n右键单击复制数值。", 0x0, 0, false},
        {0x43DF09, 0x64F928, 0x05, "点击在浏览器中打开球员页面\n{}", 0x43DF1A, 39, false},
        {0x3EEADD, 0x64CDB0, 0x1D, "{}的进球数", 0x3EEAEE, 18, false},
        {0x4458F8, 0x650250, 0x05, "点击在浏览器中打开经理页面\n{}", 0x445907, 38, false},
        {0x452E9A, 0x650B58, 0x05, "点击在浏览器中打开球队页面\n{}", 0x452EAB, 37, false},
    };
    PatchLabels(le, "deep UI", labels);
}

// These display strings are copied with scalar/SIMD loads, not LEA pointers.
// Keep their byte lengths unchanged because the compiler inlined the copies.
static void PatchInlineUiLabels(HMODULE le)
{
    struct InlineLabel { uintptr_t rva; const char* original; const char* replacement; };
    constexpr InlineLabel labels[] = {
        {0x64E2D0, "SET TO", "设为"},
        {0x64E2D8, "INCREASE BY", "增加值  "},
        {0x64E2E8, "DECREASE BY", "减少值  "},
        {0x64E2F4, "RANDOM", "随机"},
        {0x64E570, "Force Player Tier Config", "球员档次参数配置"},
        {0x64EC44, "New...", "新建"},
        {0x64EE38, "Choose a LUA File", "选择 LUA 文件"},
        {0x64EE70, "Execute LUA", "运行 LUA "},
    };
    auto* module = reinterpret_cast<uint8_t*>(le);
    auto* first = module + labels[0].rva;
    const auto& last = labels[_countof(labels) - 1];
    const size_t span = last.rva + strlen(last.original) + 1 - labels[0].rva;
    MEMORY_BASIC_INFORMATION region{};
    if (!VirtualQuery(first, &region, sizeof(region)) || region.State != MEM_COMMIT ||
        region.AllocationBase != le || reinterpret_cast<uintptr_t>(first) + span >
        reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize) return;
    for (const auto& label : labels)
    {
        const size_t length = strlen(label.original);
        if (strlen(label.replacement) != length ||
            memcmp(module + label.rva, label.original, length + 1))
        {
            Log("inline UI signature/length mismatch at RVA 0x%llX", (unsigned long long)label.rva);
            return;
        }
    }
    DWORD oldProtect, ignored;
    if (!VirtualProtect(first, span, PAGE_READWRITE, &oldProtect)) return;
    for (const auto& label : labels)
        memcpy(module + label.rva, label.replacement, strlen(label.replacement) + 1);
    VirtualProtect(first, span, oldProtect, &ignored);
    Log("inline UI labels PATCHED: %zu", _countof(labels));
}

#include "StartupNotice.h"

static DWORD WINAPI Worker(LPVOID)
{
    wchar_t path[MAX_PATH];
    _snwprintf_s(path, _countof(path), _TRUNCATE, L"%s\\FC27LEzh_test.log", g_dir);
    g_log = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    Log("========== FC27LEzh phase1 (font) %s %s ==========", __DATE__, __TIME__);
    wchar_t exe[MAX_PATH] = {}; GetModuleFileNameW(nullptr, exe, _countof(exe));
    Log("host: %ls", exe);

    HMODULE le = nullptr;
    for (int i = 0; i < 6000; ++i)
    {
        le = GetModuleHandleW(L"FCLiveEditor.DLL");
        if (le) break;
        Sleep(50);
    }
    if (!le) { Log("!! FCLiveEditor.DLL not found"); return 0; }
    Log("FCLiveEditor.DLL @ %p (base 0x%llX)", le, (unsigned long long)le);

    const bool localized = PatchFont(le);
    if (localized)
    {
        PatchSettingsLabels(le);
        PatchPlayerEditorLabels(le);
        PatchDialogLabels(le);
        PatchRemainingUiLabels(le);
        PatchDeepUiLabels(le);
        PatchInlineUiLabels(le);
        PatchStartupNotice(le);
    }
    Log("worker done");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(inst);
        GetModuleFileNameW(inst, g_dir, _countof(g_dir));
        if (wchar_t* s = wcsrchr(g_dir, L'\\')) *s = 0;
        HANDLE worker = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr);
        if (worker) CloseHandle(worker);
    }
    return TRUE;
}
