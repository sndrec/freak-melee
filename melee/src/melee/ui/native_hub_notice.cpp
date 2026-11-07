#include "native_hub_notice.h"
#include "native_hub_internal.h"
#include <melee/mn/native_menu_skin.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    struct Notice { char title[64], message[384]; };
    Notice notices[4];
    unsigned first, count;
    Uint64 opened_at, dismissed_until;
}

void nativeHubNotice_Show(const char* title, const char* message)
{
    if (!message || !*message || count == 4) return;
    auto& notice = notices[(first + count) % 4];
    std::snprintf(notice.title, sizeof(notice.title), "%s", title ? title : "Notice");
    std::snprintf(notice.message, sizeof(notice.message), "%s", message);
    if (!count++) opened_at = SDL_GetTicks();
    hub.pending_action = 0;
}

bool nativeHubNotice_BlocksInput()
{
    return count || SDL_GetTicks() < dismissed_until;
}

void nativeHubNotice_Dismiss()
{
    const Uint64 now = SDL_GetTicks();
    // The press that submitted the request must not dismiss its response.
    if (!count || now - opened_at < 350) return;
    first = (first + 1) % 4;
    if (--count) opened_at = now;
    dismissed_until = now + 180; // Also swallow associated text/mouse events.
}

void nativeHubNotice_Draw()
{
    if (!count) return;
    float t = std::min(1.f, (SDL_GetTicks() - opened_at) / 260.f);
    float x = 88 + 570 * (1 - t) * (1 - t) * (1 - t);
    const auto& notice = notices[first];
    nativeMenuPaint_Rect(0, 0, 640, 480, 0x00000090);
    // Preserve the original main-menu border and caption lettering.
    nativeMenuSkin_Panel(x, 163, 464, 163, 0xDFD8C5FF, 0x171318F9);
    nativeMenuPaint_Gradient(x + 7, 170, 450, 36, 0x6C6357FF, 0x2F2929FF);
    nativeMenuPaint_Text(notice.title, x + 20, 176, 424, 22,
        0xFFE8A0FF, NativeMenuFont_MenuCaption);
    char line[96];
    const char* cursor = notice.message;
    float y = 220;
    for (unsigned row = 0; *cursor && row < 4; ++row, y += 17) {
        size_t length = std::min(size_t(62), std::strlen(cursor));
        const char* newline = static_cast<const char*>(std::memchr(cursor, '\n', length));
        if (newline) length = size_t(newline - cursor);
        else if (cursor[length] && cursor[length] != ' ') {
            size_t word = length;
            while (word && cursor[word] != ' ') --word;
            if (word) length = word;
        }
        while (length && (static_cast<unsigned char>(cursor[length]) & 0xC0) == 0x80) --length;
        std::memcpy(line, cursor, length); line[length] = 0;
        nativeMenuPaint_Text(line, x + 22, y, 420, 12, 0xF2ECEDFF);
        cursor += length;
        while (*cursor == ' ' || *cursor == '\n') ++cursor;
    }
    nativeMenuPaint_Text("Press any button", x + 256, 303, 186, 10,
        0xD9B754FF, NativeMenuFont_Notice, true);
}

void nativeHubNotice_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] hub-notice count=%u blocked=%d title=%s message=%s\n",
        count, nativeHubNotice_BlocksInput(), count ? notices[first].title : "",
        count ? notices[first].message : "");
}

void nativeHubNotice_Clear()
{
    first = count = 0; opened_at = dismissed_until = 0;
    std::memset(notices, 0, sizeof(notices));
}
