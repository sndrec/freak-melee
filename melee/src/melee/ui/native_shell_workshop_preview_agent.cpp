#include "native_shell_workshop_preview_agent.h"
#include "native_shell_workshop_editor_internal.h"
#include "native_shell_workshop_model_internal.h"

#include <aurora/aurora.h>
#include <bcrypt.h>
#include <melee/gm/native_feedback.h>
#include <melee/gm/native_workshop_practice.h>

using NativeAuthoring::Reply;
using NativeAuthoring::add_string;
using namespace WorkshopEditor;

namespace {
struct Capture {
    std::string token, temporary, output, scope, hash, error;
    AuroraPresentedCapture frame{};
    yyjson_mut_doc* metadata = nullptr;
    size_t bytes = 0;
    unsigned practice_id = 0, practice_frame = 0;
} capture;

bool playing()
{
    return session->mode == WorkshopMode::Model
        ? WorkshopModelInternal::playing : session->action_preview_playing;
}

void report_capture(Reply& reply)
{
    add_string(reply, reply.result, "capture_id", capture.token);
    add_string(reply, reply.result, "scope", capture.scope);
    yyjson_mut_obj_add_val(reply.document, reply.result, "view",
        yyjson_mut_val_mut_copy(reply.document, yyjson_mut_doc_get_root(capture.metadata)));
    const char* status = "pending";
    if (!capture.hash.empty()) {
        status = "ready";
        add_string(reply, reply.result, "output", capture.output);
        add_string(reply, reply.result, "png_sha256", capture.hash);
        yyjson_mut_obj_add_uint(reply.document, reply.result, "bytes", capture.bytes);
        yyjson_mut_obj_add_uint(reply.document, reply.result, "width", capture.frame.width);
        yyjson_mut_obj_add_uint(reply.document, reply.result, "height", capture.frame.height);
        yyjson_mut_obj_add_uint(reply.document, reply.result, "displayed_frame", capture.frame.displayed_frame);
        yyjson_mut_obj_add_uint(reply.document, reply.result, "presented_at_ns", capture.frame.presented_at_ns);
    } else if (!capture.error.empty()) {
        status = "failed";
        add_string(reply, reply.result, "message", capture.error);
    }
    add_string(reply, reply.result, "status", status);
}

bool reserve_capture(yyjson_val* args, yyjson_mut_val* view, Reply& reply,
    unsigned practice_id = 0, unsigned practice_frame = 0)
{
    if (!capture.temporary.empty()) {
        reply.fail("capture_busy", "Wait for the outstanding Workshop capture.");
        return false;
    }
    const char* scope = yyjson_get_str(yyjson_obj_get(args, "scope"));
    const bool viewport = std::strcmp(scope, "viewport") == 0;
    const std::string requested = yyjson_get_str(yyjson_obj_get(args, "output"));
    char full[768];
    const bool drive_path = requested.size() >= 7 && requested[1] == ':' &&
        (requested[2] == '/' || requested[2] == '\\') && requested.find(':', 2) == std::string::npos;
    DWORD length = drive_path ? GetFullPathNameA(requested.c_str(), sizeof(full), full, nullptr) : 0;
    if (!length || length >= sizeof(full) || !requested.ends_with(".png")) {
        reply.fail("invalid_output", "Choose an absolute .png file path on a local drive.");
        return false;
    }
    if (GetFileAttributesA(full) != INVALID_FILE_ATTRIBUTES) {
        reply.fail("output_exists", "Choose a new PNG path.");
        return false;
    }
    unsigned char random[16];
    if (BCryptGenRandom(nullptr, random, sizeof(random), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        reply.fail("capture_failed", "Could not allocate a capture identity.");
        return false;
    }
    char token[33]{};
    for (unsigned i = 0; i < sizeof(random); ++i) std::snprintf(token + i * 2, 3, "%02x", random[i]);
    const std::string temporary = std::string(full) + "." + token + ".png";
    auto* metadata = yyjson_mut_doc_new(nullptr);
    if (!metadata) {
        reply.fail("capture_failed", "Could not allocate frame metadata.");
        return false;
    }
    yyjson_mut_doc_set_root(metadata, yyjson_mut_val_mut_copy(metadata, view));
    HANDLE file = CreateFileA(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        yyjson_mut_doc_free(metadata);
        reply.fail("capture_failed", "Could not reserve a file in the requested output directory.");
        return false;
    }
    CloseHandle(file);
    AuroraPresentedCapture reserved{};
    if (!practice_id && !aurora_capture_next_frame(temporary.c_str(), viewport, &reserved)) {
        DeleteFileA(temporary.c_str());
        yyjson_mut_doc_free(metadata);
        reply.fail("capture_busy", "Finish the current screenshot or feedback capture first.");
        return false;
    }
    yyjson_mut_doc_free(capture.metadata);
    capture = {};
    capture.token = token;
    capture.temporary = temporary;
    capture.output = full;
    capture.scope = scope;
    capture.frame = reserved;
    capture.metadata = metadata;
    capture.practice_id = practice_id;
    capture.practice_frame = practice_frame;
    return true;
}
}

void nativeWorkshopPreview_Describe(Reply& reply, yyjson_mut_val* object)
{
    NativeHomeWorkshopCamera camera;
    auto* view = yyjson_mut_obj(reply.document);
    yyjson_mut_obj_add_val(reply.document, object, "preview_view", view);
    yyjson_mut_obj_add_bool(reply.document, view, "playing", playing());
    const bool action = session->mode == WorkshopMode::Action;
    const bool animation = session->mode == WorkshopMode::Animation;
    const bool model = session->mode == WorkshopMode::Model;
    yyjson_mut_obj_add_bool(reply.document, view, "hitboxes", action && session->show_hitboxes);
    yyjson_mut_obj_add_bool(reply.document, view, "hurtboxes", action && session->show_hurtboxes);
    const auto& model_view = *WorkshopModelInternal::active;
    yyjson_mut_obj_add_bool(reply.document, view, "bones",
        action ? session->show_action_bones :
        animation ? session->show_animation_bones && !session->graph_focused :
        model && (model_view.bones_visible || model_view.context == WorkshopModelInternal::Context::Skeleton));
    yyjson_mut_obj_add_bool(reply.document, view, "viewport_visible", !animation || !session->graph_focused);
    NativeFighterAnimationClip clip;
    float frame;
    if (nativeHome_WorkshopPoseStatus(session->project, &clip, &frame)) {
        auto* pose = yyjson_mut_obj(reply.document);
        yyjson_mut_obj_add_val(reply.document, view, "pose", pose);
        add_string(reply, pose, "bank", clip.bank == NativeFighterAnimation_Demo ? "demo" : "action");
        yyjson_mut_obj_add_uint(reply.document, pose, "index", clip.index);
        yyjson_mut_obj_add_real(reply.document, pose, "frame", frame);
    } else {
        yyjson_mut_obj_add_null(reply.document, view, "pose");
    }
    if (nativeHome_WorkshopCameraRead(&camera)) {
        auto* setup = yyjson_mut_obj(reply.document);
        auto* target = yyjson_mut_arr(reply.document);
        yyjson_mut_obj_add_val(reply.document, view, "camera", setup);
        yyjson_mut_obj_add_val(reply.document, setup, "target", target);
        for (float component : camera.target) yyjson_mut_arr_add_real(reply.document, target, component);
        yyjson_mut_obj_add_real(reply.document, setup, "yaw", camera.yaw);
        yyjson_mut_obj_add_real(reply.document, setup, "pitch", camera.pitch);
        yyjson_mut_obj_add_real(reply.document, setup, "distance", camera.distance);
    } else {
        yyjson_mut_obj_add_null(reply.document, view, "camera");
    }
}

void nativeWorkshopPreview_Configure(yyjson_val* args, Reply& reply)
{
    const bool action = session->mode == WorkshopMode::Action;
    const bool animation = session->mode == WorkshopMode::Animation;
    auto* camera_input = yyjson_obj_get(args, "camera");
    auto* reset = yyjson_obj_get(args, "reset_camera");
    auto* hitboxes = yyjson_obj_get(args, "hitboxes");
    auto* hurtboxes = yyjson_obj_get(args, "hurtboxes");
    auto* bones = yyjson_obj_get(args, "bones");
    auto* playback = yyjson_obj_get(args, "playing");
    if ((camera_input && reset) || (!action && (hitboxes || hurtboxes)) ||
        (!action && !animation && (bones || playback))) {
        reply.fail("preview_setting_unavailable",
            "Choose a camera or reset. Action supplies hitboxes and hurtboxes. Use Model selection for its bones and playback.");
        return;
    }
    NativeHomeWorkshopCamera camera;
    if (!nativeHome_WorkshopCameraRead(&camera)) {
        reply.fail("preview_pending", "Wait for the frozen fighter before configuring its view.");
        return;
    }
    const bool change_playback = playback && yyjson_get_bool(playback) != playing();
    if (change_playback && yyjson_get_bool(playback)) {
        NativeFighterAnimationClip clip;
        float frame;
        if (!nativeHome_WorkshopPoseStatus(session->project, &clip, &frame)) {
            reply.fail("preview_pending", "Select an available clip before starting playback.");
            return;
        }
    }
    if (camera_input) {
        if (auto* target = yyjson_obj_get(camera_input, "target"))
            for (unsigned axis = 0; axis < 3; ++axis)
                camera.target[axis] = (float) yyjson_get_num(yyjson_arr_get(target, axis));
        if (auto* yaw = yyjson_obj_get(camera_input, "yaw")) camera.yaw = (float) yyjson_get_num(yaw);
        if (auto* pitch = yyjson_obj_get(camera_input, "pitch")) camera.pitch = (float) yyjson_get_num(pitch);
        if (auto* distance = yyjson_obj_get(camera_input, "distance")) camera.distance = (float) yyjson_get_num(distance);
        if (!nativeHome_WorkshopCameraSet(&camera)) {
            reply.fail("preview_setting_unavailable", "The camera could not apply that framing.");
            return;
        }
    }
    if (reset) nativeHome_WorkshopCameraReset(std::strcmp(yyjson_get_str(reset), "front") == 0);
    if (hitboxes || hurtboxes || bones)
        set_preview_overlays(hitboxes ? yyjson_get_bool(hitboxes) : session->show_hitboxes,
            hurtboxes ? yyjson_get_bool(hurtboxes) : session->show_hurtboxes,
            bones ? yyjson_get_bool(bones) : animation ? session->show_animation_bones : session->show_action_bones);
    if (change_playback) toggle_action_preview();
}

void nativeWorkshopPreview_Capture(yyjson_val* args, yyjson_mut_val* state, Reply& reply)
{
    auto* view = yyjson_mut_obj_get(state, "preview_view");
    if (nativeFeedback_IsOpen() || !yyjson_mut_get_bool(yyjson_mut_obj_get(state, "preview_ready")) ||
        !yyjson_mut_is_obj(yyjson_mut_obj_get(view, "pose"))) {
        reply.fail("preview_pending", "A frozen Workshop pose must be visible before capturing it.");
        return;
    }
    const char* scope = yyjson_get_str(yyjson_obj_get(args, "scope"));
    const bool viewport = std::strcmp(scope, "viewport") == 0;
    if (viewport && !yyjson_mut_get_bool(yyjson_mut_obj_get(view, "viewport_visible"))) {
        reply.fail("viewport_hidden", "Leave graph focus to capture the game viewport.");
        return;
    }
    auto* captured = yyjson_mut_obj(reply.document);
    for (const char* key : {"instance", "build_identity", "member", "workspace", "revision",
                            "fighter_revision", "preview_fighter_revision", "loaded_preview_package_id",
                            "preview_view", "model_view"}) {
        auto* value = yyjson_mut_obj_get(state, key);
        if (value) yyjson_mut_obj_add_val(reply.document, captured, key, yyjson_mut_val_mut_copy(reply.document, value));
    }
    if (reserve_capture(args, captured, reply)) report_capture(reply);
}

bool nativeWorkshopPreview_ReservePracticeCapture(yyjson_val* args, const char* instance,
                                                unsigned end_frame, Reply& reply)
{
    const auto& state = *nativeWorkshopPractice_State();
    auto* view = yyjson_mut_obj(reply.document);
    char package[65];
    nativeContentStore_IdHex(&state.package, package);
    add_string(reply, view, "instance", instance);
    add_string(reply, view, "build_identity", NativeAuthoring::build_identity());
    add_string(reply, view, "fighter_revision", state.fighter_revision);
    add_string(reply, view, "package_id", package);
    yyjson_mut_obj_add_uint(reply.document, view, "test_id", state.id);
    yyjson_mut_obj_add_uint(reply.document, view, "practice_frame", end_frame);
    if (!reserve_capture(args, view, reply, state.id, end_frame)) return false;
    add_string(reply, reply.result, "capture_id", capture.token);
    return true;
}

extern "C" void nativeShell_CapturePracticeFrame(void)
{
    if (!capture.practice_id || capture.temporary.empty() || capture.frame.capture_id) return;
    const auto& state = *nativeWorkshopPractice_State();
    if (!nativeWorkshopPractice_Stepping() || state.id != capture.practice_id ||
        state.frame + 1 != capture.practice_frame) return;
    if (!aurora_capture_next_frame(capture.temporary.c_str(), capture.scope == "viewport", &capture.frame))
        capture.error = "The requested practice frame could not reserve the renderer.";
}

void nativeWorkshopPreview_UpdateCapture()
{
    if (capture.temporary.empty()) return;
    if (capture.practice_id && !capture.frame.capture_id) {
        const auto& state = *nativeWorkshopPractice_State();
        if (capture.error.empty() && state.id == capture.practice_id && nativeWorkshopPractice_Stepping() &&
            state.frame < capture.practice_frame) return;
        if (capture.error.empty()) capture.error = "The requested practice frame was not presented.";
        DeleteFileA(capture.temporary.c_str());
        capture.temporary.clear();
        return;
    }
    AuroraPresentedCapture completed{};
    aurora_presented_capture_status(&completed);
    if (completed.capture_id == capture.frame.capture_id && completed.status == AURORA_CAPTURE_PENDING) return;
    if (completed.capture_id == capture.frame.capture_id && completed.status == AURORA_CAPTURE_READY) {
        capture.frame = completed;
        FILE* file = std::fopen(capture.temporary.c_str(), "rb");
        long size = 0;
        std::vector<uint8_t> bytes;
        bool ok = file && !std::fseek(file, 0, SEEK_END) && (size = std::ftell(file)) > 0 &&
            size <= 64 * 1024 * 1024 && !std::fseek(file, 0, SEEK_SET);
        if (ok) {
            bytes.resize(size);
            ok = std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
        }
        if (file) std::fclose(file);
        NativeContentId digest;
        if (ok && nativeContent_Hash(bytes.data(), bytes.size(), &digest) &&
            MoveFileExA(capture.temporary.c_str(), capture.output.c_str(), MOVEFILE_WRITE_THROUGH)) {
            char hex[65];
            nativeContentStore_IdHex(&digest, hex);
            capture.hash = hex;
            capture.bytes = bytes.size();
        } else {
            capture.error = "Could not publish the captured PNG. Choose a new writable output path.";
        }
    } else {
        capture.error = *completed.error ? completed.error : "The frame capture was interrupted.";
    }
    if (completed.capture_id == capture.frame.capture_id) aurora_release_presented_capture();
    if (capture.hash.empty()) DeleteFileA(capture.temporary.c_str());
    capture.temporary.clear();
}

void nativeWorkshopPreview_CaptureStatus(yyjson_val* args, Reply& reply)
{
    if (capture.token.empty() || capture.token != yyjson_get_str(yyjson_obj_get(args, "capture_id"))) {
        reply.fail("unknown_capture", "Use the most recent capture ID from this Workshop instance.");
        return;
    }
    report_capture(reply);
}

void nativeWorkshopPreview_Shutdown()
{
    if (!capture.temporary.empty()) {
        if (capture.frame.capture_id) aurora_release_presented_capture();
        DeleteFileA(capture.temporary.c_str());
    }
    yyjson_mut_doc_free(capture.metadata);
    capture = {};
}
