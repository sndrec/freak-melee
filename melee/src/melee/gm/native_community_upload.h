#pragma once
#include <cstdint>

struct NativeCommunityUpload {
    char id[37], metadata[1024], image[1024], context[1024];
};
struct NativeCommunityReceipt {
    char id[37], error[256];
    unsigned revision;
    uint64_t sent, total;
    bool busy, stored;
};
// Runs on the community worker, with progress published by its owner.
using NativeCommunityProgress = bool (*)(void*, uint64_t sent, uint64_t total);
bool nativeCommunity_SendReport(void* connection, bool secure, const NativeCommunityUpload& upload,
    NativeCommunityReceipt& receipt, NativeCommunityProgress progress, void* owner);
