// FC27 LE27.2.0 DX12 render integration; no game data or ImGui context layout writes.
#pragma once
#include <shellapi.h>
#include "NoticeUi.h"
#pragma comment(lib, "shell32.lib")
static uint8_t* g_noticeModule = nullptr;
static NoticeApi g_noticeApi{};
static void (*g_originalRender)() = nullptr;
static void NoticeOpenForum()
{
    ShellExecuteW(nullptr, L"open", L"https://bbs.gbtgame.me/", nullptr, nullptr, SW_SHOWNORMAL);
}
static void NoticeCenterPopup()
{
    // The target Win32 backend writes IO.DisplaySize at offsets8/12.
    auto getIo = reinterpret_cast<const uint8_t*(*)()>(g_noticeModule + 0x26B180);
    NoticeVec2 display; memcpy(&display, getIo() + 8, sizeof(display));
    NoticeVec2 center{display.x * 0.5f, display.y * 0.5f}, pivot{0.5f, 0.5f};
    auto setPosition = reinterpret_cast<void(*)(const NoticeVec2*, int, const NoticeVec2*)>(g_noticeModule + 0x27BB60);
    setPosition(&center, 8, &pivot); // ImGuiCond_Appearing
}
static void NoticeRender()
{
    // This flag is set by Present only after UIInitDoneNotify returns.
    // The wrapper is called after LE's widgets and before ImGui::Render.
    const auto* context = *reinterpret_cast<uint8_t**>(g_noticeModule + 0xA50E20);
    if (context && context[0x40A8])
        DrawAuthorNotice(g_noticeApi, g_noticeModule[0xA51A5D] != 0);
    g_originalRender();
}
static bool PatchStartupNotice(HMODULE le)
{
    struct Signature { uintptr_t rva; const char* bytes; size_t size; };
    static const Signature signatures[] = {
        {0x34106D, "\xE8\x1E\xF2\x12\x00\xC6\x05\xE4\x09\x71\x00\x01", 12},
        {0x340E90, "\xE8\x4B\x36\xF3\xFF\x48\x8B\xCB\xE8\x43\xB3\x12\x00\xE8\xFE\x57\xF3\xFF", 18},
        {0x2766A0, "\x40\x57\x48\x83\xEC\x50\x48\x8B\x3D\x73\xA7\x7D\x00\x8B\x87\x98", 16},
        {0x275500, "\x40\x53\x55\x56\x57\x41\x56\x48\x83\xEC\x20\x48\x8B\x2D\x0E\xB9", 16},
        {0x25C9C0, "\x40\x53\x55\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x20\x48", 16},
        {0x269530, "\x40\x53\x48\x83\xEC\x20\x48\x8B\x1D\xE3\x78\x7E\x00\x48\x8B\x83", 16},
        {0x25DDE0, "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8B\x3D\x2F\x30\x7F", 16},
        {0x2B12B0, "\x41\xB8\x01\x00\x00\x00\xE9\x95\xFB\xFF\xFF\xCC\xCC\xCC\xCC\xCC", 16},
        {0x29FE00, "\x45\x33\xC0\xE9\x58\x06\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC", 16},
        {0x27A2B0, "\x48\x8B\x05\x69\x6B\x7D\x00\x0F\x28\xD8\x48\x8B\x88\x40\x41\x00", 16},
        {0x2A5F60, "\x48\x83\xEC\x38\x48\x8B\x15\xB5\xAE\x7A\x00\x48\x8B\x82\x40\x41", 16},
        {0x26BF60, "\x48\x8B\x0D\xB9\x4E\x7E\x00\x48\x8B\x81\x40\x41\x00\x00\xC6\x80", 16},
        {0x26AB30, "\x48\x8B\x05\xE9\x62\x7E\x00\x48\x8B\x90\x40\x41\x00\x00\x48\x8B", 16},
        {0x286660, "\x41\xF7\xC1\x00\x00\x00\xFF\x0F\x84\x8B\x01\x00\x00\x53\x55\x56", 16},
        {0x27BB60, "\x4C\x8B\x0D\xB9\x52\x7D\x00\xB8\x01\x00\x00\x00\x41\x83\x89\x40", 16},
        {0x26B180, "\x48\x8B\x05\x99\x5C\x7E\x00\x48\x83\xC0\x08\xC3\xCC\xCC\xCC\xCC", 16},
        {0x26ABB0, "\x48\x8B\x05\x69\x62\x7E\x00\xF3\x0F\x10\x80\x78\x3E\x00\x00\xC3", 16},
    };
    auto* module = reinterpret_cast<uint8_t*>(le);
    for (const auto& sig : signatures)
    {
        MEMORY_BASIC_INFORMATION region{};
        auto* address = module + sig.rva;
        if (!VirtualQuery(address, &region, sizeof(region)) || region.State != MEM_COMMIT ||
            region.AllocationBase != le || reinterpret_cast<uintptr_t>(address) + sig.size >
            reinterpret_cast<uintptr_t>(region.BaseAddress) + region.RegionSize ||
            memcmp(address, sig.bytes, sig.size))
        {
            Log("author popup signature mismatch at RVA 0x%llX", (unsigned long long)sig.rva);
            return false;
        }
    }
    auto* bridge = static_cast<uint8_t*>(AllocNear(le, 16));
    if (!bridge) return false;
    // Absolute indirect jump preserves the call ABI and caller return address.
    const uint8_t jump[] = {0xFF, 0x25, 0, 0, 0, 0};
    memcpy(bridge, jump, sizeof(jump));
    auto target = &NoticeRender; memcpy(bridge + 6, &target, sizeof(target));
    DWORD oldProtect = 0;
    if (!VirtualProtect(bridge, 16, PAGE_EXECUTE_READ, &oldProtect))
    { VirtualFree(bridge, 0, MEM_RELEASE); return false; }
    auto* call = module + 0x340E9D;
    const intptr_t delta = reinterpret_cast<intptr_t>(bridge) - reinterpret_cast<intptr_t>(call + 5);
    if (delta < INT32_MIN || delta > INT32_MAX ||
        !VirtualProtect(call, 5, PAGE_EXECUTE_READWRITE, &oldProtect))
    { VirtualFree(bridge, 0, MEM_RELEASE); return false; }
    g_noticeModule = module;
    g_originalRender = reinterpret_cast<void(*)()>(module + 0x2766A0);
#define NOTICE_BIND(member, address) g_noticeApi.member = reinterpret_cast<decltype(g_noticeApi.member)>(module + address)
    NOTICE_BIND(openPopup, 0x275500); NOTICE_BIND(beginPopup, 0x25C9C0);
    NOTICE_BIND(endPopup, 0x269530); NOTICE_BIND(closePopup, 0x25DDE0);
    NOTICE_BIND(text, 0x2B12B0); NOTICE_BIND(button, 0x29FE00);
    NOTICE_BIND(sameLine, 0x27A2B0); NOTICE_BIND(dummy, 0x2A5F60);
    NOTICE_BIND(drawList, 0x26BF60); NOTICE_BIND(cursor, 0x26AB30);
    NOTICE_BIND(fillRect, 0x286660); NOTICE_BIND(fontSize, 0x26ABB0);
#undef NOTICE_BIND
    g_noticeApi.openForum = &NoticeOpenForum;
    g_noticeApi.centerPopup = &NoticeCenterPopup;
    FlushInstructionCache(GetCurrentProcess(), bridge, 16);
    MemoryBarrier();
    const int32_t displacement = static_cast<int32_t>(delta);
    memcpy(call + 1, &displacement, sizeof(displacement));
    DWORD ignored; VirtualProtect(call, 5, oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), bridge, 16);
    FlushInstructionCache(GetCurrentProcess(), call, 5);
    Log("LE-style author popup installed; waiting for UI initialization flag");
    return true;
}
