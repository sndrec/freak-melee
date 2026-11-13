#pragma once
#include <cstdint>
#include "native_community_upload.h"

// Public community data is owned by the menu, outside deterministic game state.
struct NativeNewsHeadline {
    char id[37], title[481], ticker[961];
    uint64_t published_at;
};
struct NativeAnnouncement {
    char id[37], delivery_id[37], title[481], body[1521], news_id[37];
    uint64_t expires_at;
};
struct NativeAnnouncementPage {
    NativeAnnouncement entries[64];
    char before[96], next[96], error[192];
    unsigned count, revision;
    bool available;
};
struct NativeAnnouncementCheck {
    NativeAnnouncement entry;
    char delivery_id[37], error[192];
    unsigned revision;
    bool found, available;
};
struct NativeCommunityFeed {
    NativeNewsHeadline headline;
    NativeAnnouncement announcements[64];
    unsigned count;
    bool available, offline;
    char error[192];
    char announcements_next[96];
};
struct NativeNewsPage {
    NativeNewsHeadline entries[25];
    char next[96], error[192];
    unsigned count;
    bool available;
};
struct NativeNewsArticle {
    NativeNewsHeadline headline;
    char body[128001], error[192];
    bool available;
};
struct NativeCommunityView {
    NativeCommunityFeed feed;
    NativeNewsPage page;
    NativeNewsArticle article;
    NativeAnnouncementPage notices;
    NativeAnnouncementCheck notice;
    unsigned revision, feed_revision;
    bool busy;
    bool announcement_busy;
};

bool nativeCommunity_Start(const char* root, const char* endpoint);
bool nativeCommunity_Poll(NativeCommunityView& view);
bool nativeCommunity_RequestPage(const char* before = nullptr);
bool nativeCommunity_RequestArticle(const char* id);
bool nativeCommunity_Refresh();
bool nativeCommunity_RequestAnnouncements(const char* before);
bool nativeCommunity_CheckAnnouncement(const char* delivery_id);
bool nativeCommunity_RequestUpload(const NativeCommunityUpload& upload);
bool nativeCommunity_PollReceipt(NativeCommunityReceipt& receipt);
void nativeCommunity_Shutdown();
