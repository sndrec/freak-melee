#include "native_hub_notice.h"
#include "native_hub_internal.h"
#include <melee/mn/native_menu_skin.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    constexpr unsigned page_rows = 8;
    struct Notice {
        char title[481], message[1521];
        unsigned short starts[768], lengths[768];
        unsigned row_count, first_row;
        bool prepared;
    };
    Notice notices[4];
    unsigned first, count;
    Uint64 opened_at, dismissed_until;
    void prepare(Notice& notice)
    {
        if (notice.prepared) return;
        unsigned start = 0;
        while (notice.message[start] && notice.row_count < 768) {
            unsigned end = start, space = start;
            char sample[160];
            while (notice.message[end] && notice.message[end] != '\n') {
                unsigned next = end + 1;
                while ((static_cast<unsigned char>(notice.message[next]) & 0xC0) == 0x80) ++next;
                const unsigned length = next - start;
                if (length >= sizeof(sample)) break;
                std::memcpy(sample, notice.message + start, length); sample[length] = 0;
                if (end > start && nativeMenuFont_Width(NativeMenuFont_Notice, sample, 12) > 420) {
                    if (space > start) end = space;
                    break;
                }
                if (notice.message[end] == ' ') space = end;
                end = next;
            }
            if (end > start) {
                notice.starts[notice.row_count] = static_cast<unsigned short>(start);
                notice.lengths[notice.row_count++] = static_cast<unsigned short>(end - start);
            }
            start = end;
            while (notice.message[start] == ' ' || notice.message[start] == '\n') ++start;
        }
        notice.prepared = true;
    }
}

void nativeHubNotice_Show(const char* title, const char* message)
{
    if (!message || !*message || count == 4) return;
    auto& notice = notices[(first + count) % 4];
    notice = {};
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
    auto& notice = notices[first];
    if (notice.prepared && notice.first_row + page_rows < notice.row_count) {
        notice.first_row += page_rows;
        opened_at = now;
        return;
    }
    first = (first + 1) % 4;
    if (--count) opened_at = now;
    dismissed_until = now + 180; // Also swallow associated text/mouse events.
}

void nativeHubNotice_Draw()
{
    if (!count) return;
    float t = std::min(1.f, (SDL_GetTicks() - opened_at) / 260.f);
    float x = 88 + 570 * (1 - t) * (1 - t) * (1 - t);
    auto& notice = notices[first];
    prepare(notice);
    const unsigned rows = std::min(page_rows, notice.row_count - notice.first_row);
    const float height = std::max(163.f, 90.f + rows * 17);
    const float top = (480 - height) * .5f;
    nativeMenuPaint_Rect(0, 0, 640, 480, 0x00000090);
    // Preserve the original main-menu border and caption lettering.
    nativeMenuSkin_Panel(x, top, 464, height, 0xDFD8C5FF, 0x171318F9);
    nativeMenuPaint_Gradient(x + 7, top + 7, 450, 36, 0x6C6357FF, 0x2F2929FF);
    nativeMenuPaint_Text(notice.title, x + 20, top + 13, 424, 22,
        0xFFE8A0FF, NativeMenuFont_MenuCaption);
    for (unsigned row = 0; row < rows; ++row) {
        const unsigned index = notice.first_row + row, length = notice.lengths[index];
        char line[160]; std::memcpy(line, notice.message + notice.starts[index], length); line[length] = 0;
        nativeMenuPaint_Text(line, x + 22, top + 57 + row * 17, 420, 12, 0xF2ECEDFF);
    }
    char prompt[64];
    if (notice.row_count > page_rows) std::snprintf(prompt, sizeof(prompt), "Press any button  %u / %u",
        notice.first_row / page_rows + 1, (notice.row_count + page_rows - 1) / page_rows);
    else std::snprintf(prompt, sizeof(prompt), "Press any button");
    nativeMenuPaint_Text(prompt, x + 200, top + height - 23, 242, 10,
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
