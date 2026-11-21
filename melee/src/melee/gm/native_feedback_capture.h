#pragma once
#include "native_community_upload.h"
#include <melee/ft/native_file_publish.h>
#include <aurora/aurora.h>
#include <yyjson.h>

struct NativeFeedbackPin { unsigned id; float x, y; char text[8193]; };
struct NativeFeedbackDraft {
    char id[37], captured_at[40], image_path[1024], context_path[1024];
    AuroraPresentedCapture image;
    NativeFileIdentity screenshot_file, context_file, notes_file;
    NativeFeedbackPin pins[64];
    unsigned count, next_pin;
};

namespace NativeFeedbackCapture {
std::shared_ptr<const std::string> read_attachment(const char* path, uint32_t limit);
// Captures displayed pixels and diagnostics before any feedback UI/input change.
bool create(NativeFeedbackDraft&, NativeCommunityUpload&, bool retain_image,
            bool& context_complete, yyjson_val* agent_report = nullptr);
bool fallback(NativeFeedbackDraft&, NativeCommunityUpload&, yyjson_val* agent_report = nullptr);
bool notes(NativeFeedbackDraft&, NativeCommunityUpload&, bool context_complete);
void discard(const NativeFeedbackDraft&, const NativeCommunityUpload&);
bool submit(const NativeCommunityUpload&);
}
