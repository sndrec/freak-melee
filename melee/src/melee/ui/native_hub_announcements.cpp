#include "native_hub_announcements.h"
#include "native_hub_internal.h"
#include "native_hub_notice.h"
#include "native_hub_news.h"
#include "native_hub_startup.h"
#include "native_hub_portrait.h"
#include "native_hub_defaults.h"
#include <melee/gm/native_community.h>
#include <melee/gm/native_feedback.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_online_session.h>
#include <melee/mn/native_menu_scene.h>
#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>

namespace {
char seen_path[1024], recent_seen[64][37], invalid[128][37];
unsigned recent_count, recent_next, invalid_count, feed_revision, page_revision, check_revision;
char next_page[96], requested_page[96];
bool page_ready, page_pending, dismissed, read_news, scan_needed = true;
uint64_t retry_at, check_after;
NativeAnnouncement active;

bool seen(const char* id)
{
    for (unsigned i = 0; i < recent_count; ++i) if (!std::strcmp(id, recent_seen[i])) return true;
    FILE* file = std::fopen(seen_path, "rb");
    if (!file) return false;
    char line[64]; bool found = false;
    while (std::fgets(line, sizeof(line), file)) {
        line[std::strcspn(line, "\r\n")] = 0;
        if (!std::strcmp(line, id)) { found = true; break; }
    }
    std::fclose(file); return found;
}
bool rejected(const char* id)
{
    for (unsigned i = 0; i < invalid_count; ++i) if (!std::strcmp(id, invalid[i])) return true;
    return false;
}
void remember()
{
    std::snprintf(recent_seen[recent_next], sizeof(recent_seen[0]), "%s", active.delivery_id);
    recent_next = (recent_next + 1) % 64; recent_count = std::min(64U, recent_count + 1);
    FILE* file = std::fopen(seen_path, "ab");
    bool ok = file && std::fprintf(file, "%s\n", active.delivery_id) > 0;
    if (file) ok = std::fclose(file) == 0 && ok;
    if (!ok) nativeHub_Notice("Could not save the dismissed notice.");
}
bool convenient()
{
    const auto* quick = nativeOnlineSession_Quick();
    return hub.visible && nativeHome_IsSelection() && !nativeHubStartup.active &&
        !nativeHubPortrait_Editing() && !hub.field && !hub.rename && !hub.delete_confirmation &&
        !nativeHubDefaults_IsOpen() && !nativeMenuScene_Active() && !nativeHubNotice_BlocksInput() &&
        !nativeFeedback_IsOpen() && hub.page != NativeHubPage_QuickStage &&
        nativeOnlineSession_State() != NativeLobbySessionState_Starting &&
        nativeOnlineSession_State() != NativeLobbySessionState_Ready &&
        (!quick || quick->state != NativeQuickState_Assigned);
}
void cancel()
{
    nativeHubNotice_Cancel(active.delivery_id);
    if (invalid_count < 128) std::snprintf(invalid[invalid_count++], sizeof(invalid[0]), "%s", active.delivery_id);
    active = {}; dismissed = read_news = false; scan_needed = true;
}
bool show(const NativeAnnouncement* entries, unsigned count, uint64_t now, uint64_t started_at)
{
    for (unsigned i = 0; i < count; ++i) {
        const auto& entry = entries[i];
        if (entry.published_at < started_at) break; // The feed and its pages are ordered newest first.
        if ((entry.expires_at && entry.expires_at <= now) || rejected(entry.delivery_id) || seen(entry.delivery_id)) continue;
        NativeHubNoticeOptions options;
        options.id = entry.delivery_id; options.action = *entry.news_id ? "Read news" : nullptr;
        options.dismissed = [](void*, bool action) { dismissed = true; read_news = action; };
        if (!nativeHubNotice_Show(entry.title, entry.body, options)) return false;
        active = entry; check_after = SDL_GetTicks() + 30000;
        return true;
    }
    return false;
}
}

void nativeHubAnnouncements_Init(const char* root)
{ if (root) std::snprintf(seen_path, sizeof(seen_path), "%s/seen-announcements.txt", root); }

void nativeHubAnnouncements_Tick(const NativeCommunityView& community)
{
    if (!*seen_path || !community.session_started_at || nativeFeedback_IsOpen()) return;
    const uint64_t now = uint64_t(std::time(nullptr)) * 1000, clock = SDL_GetTicks();
    if (feed_revision != community.feed_revision) {
        feed_revision = community.feed_revision;
        // Refresh the older page from its stable cursor before displaying it
        // again. Newer notices keep priority without expanding the queue.
        page_ready = false; page_pending = false; *requested_page = 0;
        std::snprintf(next_page, sizeof(next_page), "%s", community.feed.announcements_next);
        if (!community.feed.offline) invalid_count = 0;
        check_after = 0; retry_at = 0; scan_needed = true;
    }
    if (page_pending && !community.announcement_busy && community.notices.revision != page_revision &&
        !std::strcmp(requested_page, community.notices.before)) {
        page_revision = community.notices.revision; page_pending = false;
        page_ready = community.notices.available;
        retry_at = page_ready ? 0 : clock + 5000; scan_needed = true;
    }
    if (*active.delivery_id) {
        if (dismissed) {
            char article[37]; std::snprintf(article, sizeof(article), "%s", read_news ? active.news_id : "");
            remember(); active = {}; dismissed = read_news = false; scan_needed = true;
            if (*article) nativeHubNews_OpenArticle(article);
        } else if (active.expires_at && active.expires_at <= now) {
            cancel();
        } else if (!nativeHubNotice_Contains(active.delivery_id)) {
            active = {}; scan_needed = true;
        } else {
            if (community.notice.revision != check_revision) {
                check_revision = community.notice.revision;
                if (!std::strcmp(community.notice.delivery_id, active.delivery_id)) {
                    if (community.notice.available && !community.notice.found) cancel();
                    else if (community.notice.available) {
                        active = community.notice.entry;
                        nativeHubNotice_Update(active.delivery_id, active.title, active.body, *active.news_id ? "Read news" : nullptr);
                    } else check_after = clock + 5000;
                }
            }
            if (*active.delivery_id && clock >= check_after && !community.announcement_busy &&
                nativeCommunity_CheckAnnouncement(active.delivery_id)) check_after = clock + 30000;
            return;
        }
    }
    if (!scan_needed || clock < retry_at || community.announcement_busy || !convenient() || !community.feed.available) return;
    const uint64_t started_at = community.session_started_at;
    if (show(community.feed.announcements, community.feed.count, now, started_at)) return;
    if (community.feed.count && community.feed.announcements[community.feed.count - 1].published_at < started_at) {
        scan_needed = false;
        return;
    }
    if (page_ready) {
        if (show(community.notices.entries, community.notices.count, now, started_at)) return;
        std::snprintf(next_page, sizeof(next_page), "%s", community.notices.next);
        if (community.notices.count && community.notices.entries[community.notices.count - 1].published_at < started_at)
            *next_page = 0;
        page_ready = false;
    }
    if (*next_page && !page_pending && clock >= retry_at && !community.announcement_busy &&
        nativeCommunity_RequestAnnouncements(next_page)) {
        std::snprintf(requested_page, sizeof(requested_page), "%s", next_page);
        page_revision = community.notices.revision; page_pending = true;
    }
    scan_needed = false;
}
void nativeHubAnnouncements_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] announcements active=%s page-ready=%d pending=%d cursor=%s invalid=%u\n",
        active.delivery_id, page_ready, page_pending, requested_page, invalid_count);
}
void nativeHubAnnouncements_Shutdown()
{
    if (*active.delivery_id) nativeHubNotice_Cancel(active.delivery_id);
    active = {}; *seen_path = *next_page = *requested_page = 0;
    recent_count = recent_next = invalid_count = feed_revision = page_revision = check_revision = 0;
    page_ready = page_pending = dismissed = read_news = false; retry_at = check_after = 0;
    scan_needed = true;
}
