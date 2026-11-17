#include "native_fighter_project.h"
#include "native_fighter_draft_access.h"
#include <cstdio>
#include <cstring>

extern "C" NativeFighterRenameResult nativeFighterProject_Rename(const char* path, const char* title,
    const char* expected_revision, NativeFighterRenameDetails* details)
{
    if (details) *details = {};
    std::string message, before, after;
    NativeDraftAccess::Lease lease;
    NativeFighterRenameResult result = NativeFighterRename_Invalid;
    bool ok = path && title;
    if (ok) { result = NativeFighterRename_Busy; ok = lease.acquire(path, message); }
    if (ok) result = NativeFighterRename_IoError;
    if (ok) ok = NativeDraftAccess::revision(path, before, message);
    if (ok && expected_revision && before != expected_revision) {
        message = "Draft changed. Inspect its current revision before renaming."; ok = false;
        result = NativeFighterRename_Conflict;
    }
    NativeFighterProjectInfo info{};
    if (ok && !nativeFighterProject_ReadInfo(path, &info)) {
        message = "Could not read this draft's manifest."; ok = false;
    }
    char manifest[256];
    size_t size = ok ? nativeFighterProject_EncodeManifest(title, info.tile_name, info.template_key, manifest, sizeof(manifest)) : 0;
    if (ok && !size) { message = "Choose a nonempty fighter title of at most 63 bytes, without control characters."; ok = false; result = NativeFighterRename_Invalid; }
    if (ok) {
        ok = NativeDraftAccess::apply(path, before, {{"project.txt", std::string(manifest, size)}}, after, message);
        if (!ok) {
            std::string current, diagnostic;
            if (NativeDraftAccess::revision(path, current, diagnostic) && current != before)
                result = NativeFighterRename_Conflict;
        }
    }
    if (details) {
        if (ok) std::snprintf(details->revision, sizeof(details->revision), "%s", after.c_str());
        else std::snprintf(details->error, sizeof(details->error), "%s", message.empty() ? "Could not rename this draft." : message.c_str());
    }
    return ok ? NativeFighterRename_Success : result;
}
