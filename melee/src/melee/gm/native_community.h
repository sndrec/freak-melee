#pragma once
#include <cstdint>

// Public community data is owned by the menu, outside deterministic game state.
struct NativeNewsHeadline {
    char id[37], title[481], ticker[961];
    uint64_t published_at;
};
struct NativeAnnouncement {
    char delivery_id[37], title[481], body[1521], news_id[37];
    uint64_t expires_at;
};
struct NativeCommunityFeed {
    NativeNewsHeadline headline;
    NativeAnnouncement announcements[64];
    unsigned count;
    bool available, offline;
    char error[192];
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
    unsigned revision, feed_revision;
    bool busy;
};

bool nativeCommunity_Start(const char* root, const char* endpoint);
bool nativeCommunity_Poll(NativeCommunityView& view);
bool nativeCommunity_RequestPage(const char* before = nullptr);
bool nativeCommunity_RequestArticle(const char* id);
bool nativeCommunity_Refresh();
void nativeCommunity_Shutdown();
