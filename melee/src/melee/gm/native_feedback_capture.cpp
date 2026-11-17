#include "native_feedback_capture.h"
#include "native_feedback_context.h"
#include "native_services.h"
#include "native_community.h"
#include <melee/ui/native_hub.h>
#include <melee/ui/native_hub_catalog.h>
#include <melee/ui/native_shell_workshop.h>
#include <melee/ui/native_shell_workshop_editor.h>
#include <melee/mn/native_menu_scene.h>
#include <aurora/rmlui.hpp>
#include <RmlUi/Core/Element.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

namespace NativeFeedbackCapture {
namespace {
#ifdef NATIVE_NETPLAY_DIAGNOSTICS_BUILD
constexpr const char* build = NATIVE_NETPLAY_DIAGNOSTICS_BUILD;
#else
constexpr const char* build = "development";
#endif
bool new_id(char (&id)[37])
{
    unsigned char b[16];
    if (BCryptGenRandom(nullptr, b, sizeof(b), BCRYPT_USE_SYSTEM_PREFERRED_RNG)) return false;
    b[6] = (b[6] & 15) | 0x40; b[8] = (b[8] & 63) | 0x80;
    std::snprintf(id, sizeof(id), "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],b[10],b[11],b[12],b[13],b[14],b[15]);
    return true;
}
bool write(const char* path, yyjson_mut_doc* document)
{
    size_t length = 0; char* bytes = yyjson_mut_write(document, 0, &length);
    FILE* file = bytes && length <= 512 * 1024 ? std::fopen(path, "wb") : nullptr;
    bool ok = file && std::fwrite(bytes, 1, length, file) == length;
    if (file) ok = std::fclose(file) == 0 && ok;
    std::free(bytes); return ok;
}
}

bool fallback(const NativeFeedbackDraft& draft, yyjson_val* agent_report)
{
    auto* doc = yyjson_mut_doc_new(nullptr); if (!doc) return false;
    auto* root = yyjson_mut_obj(doc); yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "id", draft.id);
    yyjson_mut_obj_add_strcpy(doc, root, "captured_at", draft.captured_at);
    yyjson_mut_obj_add_str(doc, root, "build", build);
    yyjson_mut_obj_add_bool(doc, root, "context_complete", false);
    yyjson_mut_obj_add_str(doc, root, "error", "Gameplay diagnostics could not be collected.");
    yyjson_mut_obj_add_uint(doc, root, "displayed_frame", draft.image.displayed_frame);
    yyjson_mut_obj_add_uint(doc, root, "presented_at_ns", draft.image.presented_at_ns);
    yyjson_mut_obj_add_uint(doc, root, "image_width", draft.image.width);
    yyjson_mut_obj_add_uint(doc, root, "image_height", draft.image.height);
    if (agent_report) yyjson_mut_obj_add_val(doc, root, "agent_report", yyjson_val_mut_copy(doc, agent_report));
    bool ok = write(draft.context_path, doc); yyjson_mut_doc_free(doc); return ok;
}

bool notes(const NativeFeedbackDraft& draft, const NativeCommunityUpload& upload, bool complete)
{
    auto* doc = yyjson_mut_doc_new(nullptr); if (!doc) return false;
    auto* root = yyjson_mut_obj(doc); yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_strcpy(doc, root, "id", draft.id);
    yyjson_mut_obj_add_strcpy(doc, root, "capture_at", draft.captured_at);
    yyjson_mut_obj_add_str(doc, root, "build", build);
    yyjson_mut_obj_add_bool(doc, root, "context_complete", complete);
    auto* pins = yyjson_mut_arr(doc); yyjson_mut_obj_add_val(doc, root, "pins", pins);
    for (unsigned i = 0; i < draft.count; ++i) {
        const auto& pin = draft.pins[i]; auto* row = yyjson_mut_obj(doc); yyjson_mut_arr_append(pins, row);
        yyjson_mut_obj_add_uint(doc, row, "id", pin.id);
        yyjson_mut_obj_add_real(doc, row, "x", pin.x); yyjson_mut_obj_add_real(doc, row, "y", pin.y);
        yyjson_mut_obj_add_strcpy(doc, row, "text", pin.text);
    }
    bool ok = write(upload.metadata, doc); yyjson_mut_doc_free(doc); return ok;
}

bool create(NativeFeedbackDraft& draft, NativeCommunityUpload& upload, bool retain_image,
            bool& complete, yyjson_val* agent_report)
{
    draft = {}; upload = {}; complete = false;
    char root[512], folder[1024];
    const char* content = nativeHubCatalog_Root(root);
    if (!new_id(draft.id)) return false;
    if (!content || std::snprintf(folder, sizeof(folder), "%s/feedback", content) >= sizeof(folder)) return false;
    if (!CreateDirectoryA(folder, nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    const DWORD attributes = GetFileAttributesA(folder);
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY) || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    std::snprintf(draft.image_path, sizeof(draft.image_path), "%s/%s.png", folder, draft.id);
    std::snprintf(draft.context_path, sizeof(draft.context_path), "%s/%s.context.json", folder, draft.id);
    std::snprintf(upload.metadata, sizeof(upload.metadata), "%s/%s.notes.json", folder, draft.id);
    std::snprintf(upload.id, sizeof(upload.id), "%s", draft.id);
    std::snprintf(upload.image, sizeof(upload.image), "%s", draft.image_path);
    std::snprintf(upload.context, sizeof(upload.context), "%s", draft.context_path);
    // Generated IDs name new captures. Never replace a previous capture.
    for (const char* path : {upload.metadata, upload.image, upload.context})
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return false;
    SYSTEMTIME time; GetSystemTime(&time);
    std::snprintf(draft.captured_at, sizeof(draft.captured_at), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
        time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,time.wMilliseconds);
    if (!aurora_capture_presented(draft.image_path, retain_image, &draft.image)) return false;
    auto* context = aurora::rmlui::get_context();
    const auto* focused = context ? context->GetFocusElement() : nullptr;
    const bool workshop = nativeShellWorkshop_IsOpen();
    NativeFeedbackInterface state{};
    state.workspace = workshop ? "Workshop" : nativeHub_IsVisible() ? "Hub" : "Game";
    state.mode = !workshop ? "" : nativeShellWorkshop_IsTesting() ? "Test Character" :
        nativeShellWorkshopEditor_IsModelMode() ? "Model" : nativeShellWorkshopEditor_IsAnimationMode() ? "Animation" :
        nativeShellWorkshopEditor_IsScriptMode() ? "Scripting" : "Action Editor";
    state.hub_page = nativeHub_IsVisible() ? int(nativeHub_CurrentPage()) : -1;
    state.menu_transition = nativeMenuScene_Active();
    state.focused_control = focused ? focused->GetId().c_str() : "";
    state.agent_report = agent_report;
    complete = nativeFeedbackContext_Write(draft.context_path, draft.id, draft.captured_at, &draft.image, &state);
    return true;
}

bool submit(const NativeCommunityUpload& upload)
{
    char root[512]; const char* content = nativeHubCatalog_Root(root);
    const char* endpoint = std::getenv("FREAK_MELEE_COMMUNITY_ENDPOINT");
    return content && nativeCommunity_Start(content, endpoint && *endpoint ? endpoint : NATIVE_COMMUNITY_ENDPOINT) &&
        nativeCommunity_RequestUpload(upload);
}
}
