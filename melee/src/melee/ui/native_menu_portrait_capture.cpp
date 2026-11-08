#include "native_menu_portrait_capture.h"
#include <melee/mn/native_menu_fighter.h>
extern "C" {
#include <sysdolphin/baselib/state.h>
}
#include <aurora/gfx.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
    struct Capture {
        NativePortraitSubject subjects[NATIVE_MENU_FIGHTERS];
        unsigned generations[NATIVE_MENU_FIGHTERS], count;
        char path[1024];
        bool queued, started, failed;
    } capture;

    void render_portrait(void*)
    {
        HSD_StateSetAlphaWriteOverride(true);
        for (unsigned i = 0; i < capture.count; ++i) {
            const auto& subject = capture.subjects[i];
            const auto& camera = subject.camera;
            nativeMenuFighter_DrawCamera(subject.slot, camera.yaw, camera.pitch, camera.zoom, camera.pan_x, camera.pan_y, camera.frame);
        }
        HSD_StateSetAlphaWriteOverride(false);
        HSD_StateInvalidate(-1);
    }
}

NativePortraitCaptureState nativeMenuPortraitCapture_State()
{
    if (capture.queued) return NativePortraitCaptureState::Queued;
    if (capture.failed) return NativePortraitCaptureState::Failed;
    if (!capture.started) return NativePortraitCaptureState::Idle;
    switch (aurora_image_export_state()) {
    case AuroraImageExport_Pending: return NativePortraitCaptureState::Saving;
    case AuroraImageExport_Complete: return NativePortraitCaptureState::Saved;
    default: return NativePortraitCaptureState::Failed;
    }
}

bool nativeMenuPortraitCapture_Request(const NativePortraitSubject* subjects, unsigned count, const char* path)
{
    auto state = nativeMenuPortraitCapture_State();
    if (state == NativePortraitCaptureState::Queued || state == NativePortraitCaptureState::Saving ||
        !subjects || !count || count > NATIVE_MENU_FIGHTERS || !path || !*path || std::strlen(path) >= sizeof(capture.path)) return false;
    for (unsigned i = 0; i < count; ++i) {
        const auto& camera = subjects[i].camera;
        unsigned generation = nativeMenuFighter_Generation(subjects[i].slot);
        if (!generation || !std::isfinite(camera.yaw) || !std::isfinite(camera.pitch) || !std::isfinite(camera.zoom) ||
            !std::isfinite(camera.pan_x) || !std::isfinite(camera.pan_y) || !std::isfinite(camera.frame) || camera.zoom <= 0) return false;
        capture.generations[i] = generation;
    }
    std::memcpy(capture.subjects, subjects, count * sizeof(*subjects)); capture.count = count;
    std::snprintf(capture.path, sizeof(capture.path), "%s", path);
    capture.queued = true; capture.started = capture.failed = false;
    return true;
}

void nativeMenuPortraitCapture_Draw()
{
    if (!capture.queued) return;
    capture.queued = false;
    for (unsigned i = 0; i < capture.count; ++i)
        if (nativeMenuFighter_Generation(capture.subjects[i].slot) != capture.generations[i]) { capture.failed = true; return; }
    capture.started = aurora_export_render_png(capture.path, 256, 256, true, render_portrait, nullptr);
    capture.failed = !capture.started;
}

void nativeMenuPortraitCapture_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] portrait-capture state=%u subjects=%u file=%s\n",
        unsigned(nativeMenuPortraitCapture_State()), capture.count, capture.path);
}
