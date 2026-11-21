#pragma once
#include <cstdint>
#include <memory>
#include <string>

inline constexpr uint32_t NativeFeedbackNotesLimit = 512 * 1024;
inline constexpr uint32_t NativeFeedbackImageLimit = 16 * 1024 * 1024;
inline constexpr uint32_t NativeFeedbackContextLimit = 8 * 1024 * 1024;

struct NativeCommunityUpload {
    char id[37], metadata[1024], image[1024], context[1024];
    std::shared_ptr<const std::string> notes_bytes, image_bytes, context_bytes;
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
