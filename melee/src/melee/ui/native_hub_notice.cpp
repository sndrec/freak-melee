#include "native_hub_notice.h"
#include "native_hub_internal.h"
#include <melee/mn/native_menu_prize.h>
#include <melee/mn/native_menu_scene.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
    constexpr unsigned page_rows = 4;
    constexpr Uint64 dismiss_delay_ms = 2000;
    struct Notice {
        char title[481], message[1521];
        char id[37], action[49];
        void (*dismissed)(void*, bool);
        void* context;
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
                if (end > start && nativeMenuFont_Width(NativeMenuFont_Notice, sample, 14) > 430) {
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

bool nativeHubNotice_Show(const char* title, const char* message, NativeHubNoticeOptions options)
{
    if (!message || !*message || count == 4 || (options.id &&
        (std::strlen(options.id) >= sizeof(Notice::id) || nativeHubNotice_Contains(options.id)))) return false;
    auto& notice = notices[(first + count) % 4];
    notice = {};
    std::snprintf(notice.title, sizeof(notice.title), "%s", title ? title : "Notice");
    std::snprintf(notice.message, sizeof(notice.message), "%s", message);
    std::snprintf(notice.id, sizeof(notice.id), "%s", options.id ? options.id : "");
    std::snprintf(notice.action, sizeof(notice.action), "%s", options.action ? options.action : "");
    notice.dismissed = options.dismissed; notice.context = options.context;
    if (!count++) opened_at = SDL_GetTicks();
    hub.pending_action = 0;
    return true;
}

bool nativeHubNotice_Contains(const char* id)
{
    if (!id || !*id) return false;
    for (unsigned i = 0; i < count; ++i) if (!std::strcmp(notices[(first+i)%4].id, id)) return true;
    return false;
}
bool nativeHubNotice_Update(const char* id, const char* title, const char* message, const char* action)
{
    if (!id || !*id || !title || !message || !*message) return false;
    for (unsigned i = 0; i < count; ++i) {
        auto& notice = notices[(first+i)%4];
        if (std::strcmp(notice.id, id)) continue;
        std::snprintf(notice.action, sizeof(notice.action), "%s", action ? action : "");
        if (std::strcmp(notice.title, title) || std::strcmp(notice.message, message)) {
            std::snprintf(notice.title, sizeof(notice.title), "%s", title);
            std::snprintf(notice.message, sizeof(notice.message), "%s", message);
            notice.row_count = notice.first_row = 0; notice.prepared = false;
            if (!i) opened_at = SDL_GetTicks();
        }
        return true;
    }
    return false;
}
bool nativeHubNotice_Cancel(const char* id)
{
    if (!id || !*id) return false;
    for (unsigned i = 0; i < count; ++i) {
        if (std::strcmp(notices[(first+i)%4].id, id)) continue;
        for (unsigned j = i; j + 1 < count; ++j) notices[(first+j)%4] = notices[(first+j+1)%4];
        notices[(first+--count)%4] = {};
        if (!i) { opened_at = SDL_GetTicks(); dismissed_until = opened_at + 180; }
        return true;
    }
    return false;
}

bool nativeHubNotice_BlocksInput()
{
    return count || SDL_GetTicks() < dismissed_until;
}

void nativeHubNotice_Dismiss(bool choose_action)
{
    const Uint64 now = SDL_GetTicks();
    // Give each displayed notice two seconds before accepting dismissal or
    // its linked action. The opening press is consumed during this interval.
    if (!count || now - opened_at < dismiss_delay_ms) return;
    auto& notice = notices[first];
    choose_action = choose_action && *notice.action && notice.dismissed;
    if (!choose_action && notice.prepared && notice.first_row + page_rows < notice.row_count) {
        notice.first_row += page_rows;
        opened_at = now;
        return;
    }
    const auto dismissed = notice.dismissed; void* context = notice.context;
    first = (first + 1) % 4;
    if (--count) opened_at = now;
    dismissed_until = now + 180; // Also swallow associated text/mouse events.
    if (dismissed) dismissed(context, choose_action);
}

void nativeHubNotice_Click(float x, float y)
{
    bool action = false;
    if (count) {
        float u, v;
        if (nativeMenuScene_Project(x, y, &u, &v)) { x = u * 640; y = v * 480; }
        action = *notices[first].action && x >= 135 && x <= 285 && y >= 310 && y <= 338;
    }
    nativeHubNotice_Dismiss(action);
}

void nativeHubNotice_Draw()
{
    if (!count) return;
    const float elapsed = (SDL_GetTicks() - opened_at) * .060f;
    auto& notice = notices[first];
    prepare(notice);
    const unsigned rows = std::min(page_rows, notice.row_count - notice.first_row);
    nativeMenuPaint_Begin(nullptr);
    nativeMenuPaint_Rect(0, 0, 640, 480, 0x00000080);
    nativeMenuPaint_End();
    // ifprize.c requests entrance frames 0..5, then holds its card at 10.
    // Use the actual retail mesh, texture animation and camera unchanged.
    nativeMenuPrize_Draw(elapsed < 6 ? elapsed : 10, 0);
    nativeMenuPaint_Begin(nullptr);
    if (elapsed < 6) return;
    nativeMenuPaint_Text(notice.title, 286, 133, 244, 20, 0x171008FF,
        NativeMenuFont_MenuOption, true, false);
    const float top = 202 + (4 - rows) * 9;
    for (unsigned row = 0; row < rows; ++row) {
        const unsigned index = notice.first_row + row, length = notice.lengths[index];
        char line[160]; std::memcpy(line, notice.message + notice.starts[index], length); line[length] = 0;
        nativeMenuPaint_Text(line, 105, top + row * 18, 430, 14, 0xFFF4E0FF,
            NativeMenuFont_Notice, true, false);
    }
    if (*notice.action) {
        char label[64]; std::snprintf(label, sizeof(label), "X %s", notice.action);
        nativeMenuPaint_Text(label, 140, 319, 145, 11, 0xFFEAB0FF,
            NativeMenuFont_Notice, true, false);
    }
    char prompt[64];
    if (notice.row_count > page_rows) std::snprintf(prompt, sizeof(prompt), "Press any button  %u / %u",
        notice.first_row / page_rows + 1, (notice.row_count + page_rows - 1) / page_rows);
    else std::snprintf(prompt, sizeof(prompt), "%s", *notice.action ? "A Dismiss" : "Press any button");
    nativeMenuPaint_Text(prompt, *notice.action ? 300 : 140, 319, *notice.action ? 200 : 360, 11,
        0xC5B99EFF, NativeMenuFont_Notice, true, false);
}

void nativeHubNotice_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] hub-notice count=%u blocked=%d title=%s message=%s ready=%d\n",
        count, nativeHubNotice_BlocksInput(), count ? notices[first].title : "",
        count ? notices[first].message : "", count && SDL_GetTicks() - opened_at >= dismiss_delay_ms);
}

void nativeHubNotice_Clear()
{
    first = count = 0; opened_at = dismissed_until = 0;
    std::memset(notices, 0, sizeof(notices));
}
