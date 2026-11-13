#include "native_hub_news.h"
#include "native_hub_notice.h"
#include "native_hub_startup.h"
#include "native_hub_portrait.h"
#include "native_hub_defaults.h"
#include <melee/gm/native_community.h>
#include <melee/gm/native_feedback.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_services.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/native_menu_scene.h>
extern "C" {
#include <sysdolphin/baselib/controller.h>
}
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace {
    NativeCommunityView community;
    bool attempted, started, article_open, loading_page;
    NativeHubPage return_page = NativeHubPage_Workshop;
    char selected_article[37], seen_path[1024], awaiting_notice[37];
    char recent_seen[64][37]; unsigned recent_count, recent_next;
    unsigned scanned_feed;
    bool scan_needed = true;
    char ticker_text[961]; float ticker_offset;
    uint64_t ticker_time;
    unsigned history_offset, article_offset, line_count;
    unsigned scroll_total, scroll_shown;
    bool dragging_scroll;
    uint64_t next_scroll;
    struct Line { unsigned start, length; unsigned char size, indent; };
    Line lines[32001];
    char layout_body[128001];
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;

    void date(uint64_t stamp, char (&output)[32])
    {
        const time_t seconds = time_t(stamp / 1000);
        tm local{};
        if (!localtime_s(&local, &seconds)) std::strftime(output, sizeof(output), "%b %d, %Y", &local);
        else *output = 0;
    }
    bool seen(const char* delivery)
    {
        for (unsigned i = 0; i < recent_count; ++i)
            if (!std::strcmp(delivery, recent_seen[i])) return true;
        FILE* file = std::fopen(seen_path, "rb");
        if (!file) return false;
        char line[64]; bool found = false;
        while (std::fgets(line, sizeof(line), file)) {
            line[std::strcspn(line, "\r\n")] = 0;
            if (!std::strcmp(line, delivery)) { found = true; break; }
        }
        std::fclose(file);
        return found;
    }
    void remember_notice()
    {
        // Keep a small immediate cache too, so a disk failure cannot repeatedly
        // reopen a notice during this session. The append-only file is tiny.
        std::snprintf(recent_seen[recent_next], sizeof(recent_seen[0]), "%s", awaiting_notice);
        recent_next = (recent_next + 1) % 64;
        recent_count = std::min(64U, recent_count + 1);
        FILE* file = std::fopen(seen_path, "ab");
        bool ok = file && std::fprintf(file, "%s\n", awaiting_notice) > 0;
        if (file) ok = std::fclose(file) == 0 && ok;
        if (!ok) nativeHub_Notice("Could not save the dismissed notice.");
        *awaiting_notice = 0;
        scan_needed = true;
    }
    void open_article(const char* key)
    {
        if (!nativeCommunity_RequestArticle(key)) return;
        if (key != selected_article) std::snprintf(selected_article, sizeof(selected_article), "%s", key);
        article_open = true; article_offset = 0;
    }
    bool request_history(const char* before)
    {
        if (!nativeCommunity_RequestPage(before)) return false;
        history_offset = 0; loading_page = true; article_open = false;
        return true;
    }
    unsigned character(const char* text, unsigned i)
    {
        ++i;
        while (text[i] && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) ++i;
        return i;
    }
    void layout()
    {
        const char* body = community.article.body;
        if (!std::strcmp(body, layout_body)) return;
        std::snprintf(layout_body, sizeof(layout_body), "%s", body);
        line_count = 0;
        unsigned start = 0;
        while (body[start] && line_count < 32001) {
            unsigned size = 10, indent = 0, prefix = 0;
            while (prefix < 3 && body[start + prefix] == '#') ++prefix;
            if (prefix && body[start + prefix] == ' ') { size = 17 - prefix; start += prefix + 1; }
            else if (body[start] == '-' && body[start + 1] == ' ') indent = 9;
            if (body[start] == '\n') {
                lines[line_count++] = { start, 0, 6, 0 };
                while (body[start] == '\n') ++start;
                continue;
            }
            do {
                unsigned end = start, space = start;
                char sample[512];
                while (body[end] && body[end] != '\n') {
                    const unsigned next = character(body, end), length = next - start;
                    if (length >= sizeof(sample)) break;
                    std::memcpy(sample, body + start, length); sample[length] = 0;
                    if (end > start && nativeMenuFont_Width(NativeMenuFont_Notice, sample, float(size)) > 386 - indent) {
                        if (space > start) end = space;
                        break;
                    }
                    if (body[end] == ' ') space = end;
                    end = next;
                }
                if (end == start) end = character(body, start);
                lines[line_count++] = { start, end - start, static_cast<unsigned char>(size), static_cast<unsigned char>(indent) };
                start = end;
                while (body[start] == ' ') ++start;
            } while (body[start] && body[start] != '\n' && line_count < 32001);
            if (body[start] == '\n') ++start;
        }
    }
    void scrollbar(unsigned count, unsigned shown, unsigned offset)
    {
        scroll_total = count; scroll_shown = shown;
        if (count <= shown) return;
        const float top = 174, height = 178;
        nativeMenuPaint_Round(590, top, 5, height, 2, 0x70646D88);
        const float thumb = std::max(10.f, height * shown / count);
        nativeMenuPaint_Round(590, top + (height - thumb) * offset / (count - shown), 5, thumb, 2, gold);
        nativeHub_Region("news-scrollbar", 585, top, 16, height, HubAction::NewsScrollTrack);
    }
    void drag_scroll()
    {
        const float amount = std::clamp((hub.pointer_y - 174) / 178, 0.f, 1.f);
        const unsigned offset = unsigned(std::round(amount * (scroll_total > scroll_shown ? scroll_total - scroll_shown : 0)));
        if (article_open) article_offset = offset;
        else history_offset = offset;
    }
}

void nativeHubNews_Tick()
{
    if (nativeFeedback_IsOpen()) return;
    if (!attempted && hub.initialized) {
        attempted = true;
        char buffer[512]; const char* root = nativeHubCatalog_Root(buffer);
        const char* endpoint = std::getenv("FREAK_MELEE_COMMUNITY_ENDPOINT");
        started = root && nativeCommunity_Start(root, endpoint && *endpoint ? endpoint : NATIVE_COMMUNITY_ENDPOINT);
        if (root) std::snprintf(seen_path, sizeof(seen_path), "%s/seen-announcements.txt", root);
    }
    if (!started) return;
    nativeCommunity_Poll(community);
    if (loading_page && !community.busy) loading_page = false;
    if (*awaiting_notice) {
        if (!nativeHubNotice_BlocksInput()) remember_notice();
        return;
    }
    const auto* quick = nativeOnlineSession_Quick();
    if (!hub.visible || !nativeHome_IsSelection() || nativeHubStartup.active ||
        nativeHubPortrait_Editing() || hub.field || hub.rename || hub.delete_confirmation ||
        nativeHubDefaults_IsOpen() || nativeMenuScene_Active() || nativeHubNotice_BlocksInput() ||
        hub.page == NativeHubPage_QuickStage || nativeOnlineSession_State() == NativeLobbySessionState_Starting ||
        nativeOnlineSession_State() == NativeLobbySessionState_Ready ||
        (quick && quick->state == NativeQuickState_Assigned)) return;
    if (!scan_needed && scanned_feed == community.feed_revision) return;
    scan_needed = false; scanned_feed = community.feed_revision;
    const uint64_t now = uint64_t(std::time(nullptr)) * 1000;
    for (unsigned i = 0; i < community.feed.count; ++i) {
        const auto& notice = community.feed.announcements[i];
        if ((notice.expires_at && notice.expires_at <= now) || seen(notice.delivery_id)) continue;
        nativeHubNotice_Show(notice.title, notice.body);
        std::snprintf(awaiting_notice, sizeof(awaiting_notice), "%s", notice.delivery_id);
        break;
    }
}
void nativeHubNews_Input()
{
    if (!hub.visible || hub.page != NativeHubPage_News || nativeMenuScene_Active() || nativeHubNotice_BlocksInput()) {
        dragging_scroll = false; return;
    }
    if (dragging_scroll) {
        if (hub.pressed) drag_scroll();
        else dragging_scroll = false;
    }
    const auto now = SDL_GetTicks();
    for (unsigned port = 0; port < 4; ++port) {
        const auto& pad = HSD_PadCopyStatus[port];
        if (pad.err) continue;
        int direction = (pad.trigger & PAD_BUTTON_DOWN) ? 1 : (pad.trigger & PAD_BUTTON_UP) ? -1 : 0;
        if (!direction && now >= next_scroll)
            direction = pad.subStickY > 32 ? -1 : pad.subStickY < -32 ? 1 : 0;
        if (direction) { nativeHubNews_Scroll(direction * 3); next_scroll = now + 160; }
        break;
    }
}
void nativeHubNews_Ticker()
{
    constexpr float x = 337, y = 57, width = 258, height = 21;
    const auto& headline = community.feed.headline;
    const char* text = *headline.ticker ? headline.ticker : *headline.title ? headline.title : "News";
    if (std::strcmp(ticker_text, text)) {
        std::snprintf(ticker_text, sizeof(ticker_text), "%s", text); ticker_offset = 0;
    }
    const uint64_t now = SDL_GetTicks();
    const bool hover = nativeHub_Hover(x, y, width, height);
    const float text_width = nativeMenuFont_Width(NativeMenuFont_Notice, text, 10);
    if (!hover && text_width > width - 46 && ticker_time)
        ticker_offset = std::fmod(ticker_offset + std::min(100ULL, now - ticker_time) * .020f, text_width + 40);
    ticker_time = now;
    if (hover) nativeMenuPaint_Round(x, y, width, height, 4, 0xA8943D30);
    nativeMenuPaint_Text("NEWS", x + 1, y + 7, 30, 8, gold);
    nativeMenuFont_Clip(x + 37, x + width - 4);
    nativeMenuPaint_Text(text, x + 37 - ticker_offset, y + 6, text_width + 1, 10, hover ? gold : 0xEEE9EDFF);
    if (text_width > width - 46)
        nativeMenuPaint_Text(text, x + 77 + text_width - ticker_offset, y + 6, text_width + 1, 10, hover ? gold : 0xEEE9EDFF);
    nativeMenuFont_Clip(0, 0);
    nativeHub_Region("news-ticker", x, y, width, height, HubAction::News);
}
void nativeHubNews_Render()
{
    nativeHub_Panel(175, 97, 430, 302);
    if (article_open) {
        const bool valid = community.article.available && !std::strcmp(selected_article, community.article.headline.id);
        nativeHub_Button("news-history", "News history", 188, 108, 104, 22, HubAction::NewsHistory);
        nativeHub_Button("news-refresh", "Refresh", 511, 108, 79, 22, HubAction::NewsRefresh, 0, false, !community.busy);
        if (!valid) {
            nativeMenuPaint_Text(community.busy ? "Loading article..." : *community.article.error ? community.article.error : "Choose an article.",
                193, 168, 390, 12, muted);
            return;
        }
        nativeMenuPaint_Text(community.article.headline.title, 191, 139, 395, 17, gold, NativeMenuFont_MenuOption);
        char published[32]{}; date(community.article.headline.published_at, published);
        nativeMenuPaint_Text(published, 193, 163, 385, 8, muted);
        layout();
        float y = 184; unsigned shown = 0;
        for (unsigned i = article_offset; i < line_count; ++i) {
            const auto& row = lines[i];
            if (y + row.size > 355) break;
            char text[512]; std::memcpy(text, layout_body + row.start, row.length); text[row.length] = 0;
            nativeMenuPaint_Text(text, 193 + row.indent, y, 386 - row.indent, row.size,
                row.size > 10 ? gold : 0xF2ECEDFF);
            y += row.size + 5; ++shown;
        }
        scrollbar(line_count, shown, article_offset);
        nativeHub_Button("news-up", "Up", 475, 367, 53, 21, HubAction::NewsScroll, -5, false, article_offset != 0);
        nativeHub_Button("news-down", "Down", 535, 367, 53, 21, HubAction::NewsScroll, 5, false, article_offset + shown < line_count);
        if (*community.article.error) nativeMenuPaint_Text("Offline copy", 193, 375, 270, 8, muted);
        return;
    }
    nativeMenuPaint_Text("News & updates", 191, 110, 275, 20, gold, NativeMenuFont_MenuOption);
    nativeHub_Button("news-latest", "Latest", 512, 108, 78, 23, HubAction::NewsLatest, 0, false, !community.busy);
    if (loading_page || !community.page.available) {
        nativeMenuPaint_Text(community.busy ? "Loading news..." : *community.page.error ? community.page.error : "News is unavailable.", 193, 161, 390, 12, muted);
        return;
    }
    const unsigned end = std::min(community.page.count, history_offset + 8);
    for (unsigned i = history_offset; i < end; ++i) {
        const auto& row = community.page.entries[i];
        const float y = 146 + (i - history_offset) * 25;
        char published[32]{}; date(row.published_at, published);
        nativeHub_Button("news-article", row.title, 190, y, 315, 23, HubAction::NewsArticle, int(i));
        nativeMenuPaint_Text(published, 514, y + 9, 73, 7, muted);
    }
    if (!community.page.count) nativeMenuPaint_Text("No news yet.", 193, 162, 390, 13, muted);
    scrollbar(community.page.count, 8, history_offset);
    nativeHub_Button("news-up", "Up", 190, 366, 54, 22, HubAction::NewsScroll, -5, false, history_offset != 0);
    nativeHub_Button("news-down", "Down", 252, 366, 54, 22, HubAction::NewsScroll, 5, false, end < community.page.count);
    nativeHub_Button("news-older", "Older news", 476, 366, 112, 22, HubAction::NewsOlder, 0, false, !community.busy && *community.page.next);
}
bool nativeHubNews_Activate(HubAction action, int value)
{
    if (action == HubAction::Back && hub.page == NativeHubPage_News) {
        if (article_open) {
            article_open = false;
            if (!community.page.available) request_history(nullptr);
        } else nativeHub_ChangePage(return_page, true);
        return true;
    }
    switch (action) {
    case HubAction::News:
        if (hub.page != NativeHubPage_News) return_page = hub.page;
        nativeHub_ChangePage(NativeHubPage_News);
        if (*community.feed.headline.id) open_article(community.feed.headline.id);
        else request_history(nullptr);
        return true;
    case HubAction::NewsHistory:
        article_open = false;
        if (!community.page.available) request_history(nullptr);
        return true;
    case HubAction::NewsLatest: request_history(nullptr); return true;
    case HubAction::NewsOlder: request_history(community.page.next); return true;
    case HubAction::NewsArticle:
        if (unsigned(value) < community.page.count) open_article(community.page.entries[value].id);
        return true;
    case HubAction::NewsScroll: nativeHubNews_Scroll(value); return true;
    case HubAction::NewsScrollTrack: drag_scroll(); dragging_scroll = true; return true;
    case HubAction::NewsRefresh:
        if (article_open) open_article(selected_article);
        else request_history(nullptr);
        return true;
    default: return false;
    }
}
void nativeHubNews_Scroll(int rows)
{
    if (article_open) article_offset = unsigned(std::clamp(int(article_offset) + rows, 0, std::max(0, int(line_count) - 1)));
    else history_offset = unsigned(std::clamp(int(history_offset) + rows, 0, std::max(0, int(community.page.count) - 8)));
}
void nativeHubNews_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] hub-news started=%d busy=%d feed=%u available=%d offline=%d headline=%s notices=%u page-count=%u article=%s lines=%u scroll=%u error=%s\n",
        started, community.busy, community.feed_revision, community.feed.available, community.feed.offline,
        community.feed.headline.title, community.feed.count, community.page.count,
        article_open ? selected_article : "", line_count, article_open ? article_offset : history_offset,
        article_open ? community.article.error : community.page.error);
}
void nativeHubNews_Shutdown()
{
    nativeCommunity_Shutdown();
    community = {}; attempted = started = article_open = loading_page = false;
    *selected_article = *seen_path = *awaiting_notice = *ticker_text = *layout_body = 0;
    ticker_offset = 0; ticker_time = 0;
    recent_count = recent_next = scanned_feed = history_offset = article_offset = line_count = 0;
    scan_needed = true;
    dragging_scroll = false; next_scroll = 0;
}
