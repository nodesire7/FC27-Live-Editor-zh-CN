#include <cstdio>
#include <cstring>
#include "../src/NoticeUi.h"
static int opened = 0, ended = 0, rectangles = 0;
static bool closeButton = false, titleClose = false;
static void Open(const char*, int) { ++opened; }
static bool Begin(const char*, bool* open, int) { *open = !titleClose; return !titleClose; }
static void End() { ++ended; }
static void Nothing() {}
static void Text(const char*, const char*) {}
static bool Button(const char* title, const NoticeVec2*) { return closeButton && strstr(title, "FC27LEzhClose"); }
static void SameLine(float, float) {}
static void Dummy(const NoticeVec2*) {}
static void* List() { return nullptr; }
static NoticeVec2* Cursor(NoticeVec2* out) { *out = {100, 100}; return out; }
static void Rect(void*, const NoticeVec2*, const NoticeVec2*, unsigned, float, int) { ++rectangles; }
static float Font() { return 15; }
int main()
{
    NoticeApi api{Open, Begin, End, Nothing, Text, Button, SameLine, Dummy, List, Cursor, Rect, Nothing, Nothing, Font};
    DrawAuthorNotice(api, false);
    if (opened || ended || rectangles) return 1;
    DrawAuthorNotice(api, true); DrawAuthorNotice(api, true);
    if (opened != 1 || ended != 2 || rectangles < 400) return 2;
    closeButton = true; DrawAuthorNotice(api, true);
    int last = ended; DrawAuthorNotice(api, true);
    if (!g_noticeDismissed || ended != last || opened != 1) return 3;
    g_noticeOpened = g_noticeDismissed = false; closeButton = false; titleClose = true;
    DrawAuthorNotice(api, true); last = opened; DrawAuthorNotice(api, true);
    if (!g_noticeDismissed || opened != last) return 4;
    puts("Notice lifecycle verified: gated by readiness, opens once, close button and title close stay dismissed.");
    return 0;
}
