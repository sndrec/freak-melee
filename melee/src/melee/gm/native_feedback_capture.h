#pragma once
#include "native_community_upload.h"
#include <aurora/aurora.h>
#include <yyjson.h>

struct NativeFeedbackPin { unsigned id; float x, y; char text[8193]; };
struct NativeFeedbackDraft {
    char id[37], captured_at[40], image_path[1024], context_path[1024];
    AuroraPresentedCapture image;
    NativeFeedbackPin pins[64];
    unsigned count, next_pin;
};

namespace NativeFeedbackCapture {
// Captures displayed pixels and diagnostics before any feedback UI/input change.
bool create(NativeFeedbackDraft&, NativeCommunityUpload&, bool retain_image,
            bool& context_complete, yyjson_val* agent_report = nullptr);
bool fallback(const NativeFeedbackDraft&, yyjson_val* agent_report = nullptr);
bool notes(const NativeFeedbackDraft&, const NativeCommunityUpload&, bool context_complete);
bool submit(const NativeCommunityUpload&);
}
