// Uses the host LE ImGui API and current style/font on its own render thread.
#pragma once
#include "ForumQr.h"
struct NoticeVec2 { float x, y; };
struct NoticeApi
{
    void (*openPopup)(const char*, int);
    bool (*beginPopup)(const char*, bool*, int);
    void (*endPopup)();
    void (*closePopup)();
    void (*text)(const char*, const char*);
    bool (*button)(const char*, const NoticeVec2*);
    void (*sameLine)(float, float);
    void (*dummy)(const NoticeVec2*);
    void* (*drawList)();
    NoticeVec2* (*cursor)(NoticeVec2*);
    void (*fillRect)(void*, const NoticeVec2*, const NoticeVec2*, unsigned, float, int);
    void (*openForum)();
    void (*centerPopup)();
    float (*fontSize)();
    bool (*escape)();
};
static bool g_noticeOpened = false;
static bool g_noticeDismissed = false;

static void DrawAuthorNotice(const NoticeApi& api, bool ready)
{
    if (!ready || g_noticeDismissed) return;
    constexpr const char* title = "汉化加载完成###FC27LEzhCredits";
    if (!g_noticeOpened)
    {
        api.openPopup(title, 0);
        g_noticeOpened = true;
    }
    bool open = true;
    api.centerPopup();
    // AlwaysAutoResize | NoSavedSettings. No style/font overrides.
    if (api.beginPopup(title, &open, (1 << 6) | (1 << 8)))
    {
        api.text("汉化作者：A-FEI-", nullptr);
        api.text("BBS.GBTGAME.ME", nullptr);
        api.text("免费在站点获取", nullptr);
        NoticeVec2 gap{0, 8}; api.dummy(&gap);
        NoticeVec2 origin{}; api.cursor(&origin);
        // QR modules have integer edges and an embedded four-module quiet zone.
        int pixels = static_cast<int>(api.fontSize() * 8.0f / 15.0f + 0.5f);
        if (pixels < 4) pixels = 4;
        if (pixels > 16) pixels = 16;
        const float module = static_cast<float>(pixels);
        NoticeVec2 end{origin.x + module * kForumQrSize, origin.y + module * kForumQrSize};
        void* list = api.drawList();
        api.fillRect(list, &origin, &end, 0xFFFFFFFF, 0, 0);
        for (int y = 0; y < kForumQrSize; ++y)
            for (int x = 0; x < kForumQrSize; ++x)
                if (kForumQr[y][x] == '1')
                {
                    NoticeVec2 first{origin.x + x * module, origin.y + y * module};
                    NoticeVec2 last{first.x + module, first.y + module};
                    api.fillRect(list, &first, &last, 0xFF000000, 0, 0);
                }
        NoticeVec2 size{module * kForumQrSize, module * kForumQrSize}; api.dummy(&size);
        api.text("扫码访问论坛", nullptr);
        api.dummy(&gap);
        NoticeVec2 button{0, 0};
        if (api.button("打开论坛###FC27LEzhForum", &button)) api.openForum();
        api.sameLine(0, -1);
        if (api.button("关闭 (Esc)###FC27LEzhClose", &button) || api.escape())
        {
            g_noticeDismissed = true;
            api.closePopup();
        }
        api.endPopup();
    }
    if (!open) g_noticeDismissed = true;
}
