#include "native_menu_portrait_capture.h"
#include <melee/mn/native_menu_fighter.h>
#include <melee/mn/native_menu_scene.h>
#include <dolphin/gx.h>
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
        unsigned generations[NATIVE_MENU_FIGHTERS], count, width, height;
        char path[1024];
        bool queued, started, failed;
    } capture;

    void render_portrait(void*)
    {
        HSD_StateSetAlphaWriteOverride(true);
        for (unsigned i = 0; i < capture.count; ++i) {
            const auto& subject = capture.subjects[i];
            const auto& camera = subject.camera;
            nativeMenuFighter_DrawCamera(subject.slot, camera.yaw, camera.pitch, camera.zoom, camera.pan_x, camera.pan_y, camera.frame,
                float(capture.width)/capture.height);
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

bool nativeMenuPortraitCapture_Request(const NativePortraitSubject* subjects, unsigned count, const char* path,
                                       unsigned width, unsigned height)
{
    auto state = nativeMenuPortraitCapture_State();
    if (state == NativePortraitCaptureState::Queued || state == NativePortraitCaptureState::Saving ||
        !subjects || !count || count > NATIVE_MENU_FIGHTERS || !path || !*path || std::strlen(path) >= sizeof(capture.path) ||
        width < 16 || height < 16 || width > 512 || height > 512) return false;
    for (unsigned i = 0; i < count; ++i) {
        const auto& camera = subjects[i].camera;
        unsigned generation = nativeMenuFighter_Generation(subjects[i].slot);
        if (!generation || !std::isfinite(camera.yaw) || !std::isfinite(camera.pitch) || !std::isfinite(camera.zoom) ||
            !std::isfinite(camera.pan_x) || !std::isfinite(camera.pan_y) || !std::isfinite(camera.frame) || camera.zoom <= 0) return false;
        capture.generations[i] = generation;
    }
    std::memcpy(capture.subjects, subjects, count * sizeof(*subjects)); capture.count = count;
    capture.width = width; capture.height = height;
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
    capture.started = aurora_export_render_png(capture.path, capture.width, capture.height, true, render_portrait, nullptr);
    capture.failed = !capture.started;
}

void nativeMenuPortraitCapture_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] portrait-capture state=%u subjects=%u file=%s\n",
        unsigned(nativeMenuPortraitCapture_State()), capture.count, capture.path);
}

void nativeMenuPortraitCapture_Preview(const NativePortraitSubject* subjects, unsigned count,
                                       float x, float y, float preview_width, float preview_height)
{
    float u0, v0, u1, v1;
    if (!subjects || !count || count > NATIVE_MENU_FIGHTERS || preview_width <= 0 || preview_height <= 0 ||
        !nativeMenuScene_Project(x, y, &u0, &v0) ||
        !nativeMenuScene_Project(x + preview_width, y + preview_height, &u1, &v1) ||
        u0 < 0 || v0 < 0 || u1 > 1 || v1 > 1 || u1 <= u0 || v1 <= v0) return;
    float viewport[6], projection[7]; u32 scissor[4];
    GXGetViewportv(viewport); GXGetProjectionv(projection);
    GXGetScissor(&scissor[0], &scissor[1], &scissor[2], &scissor[3]);
    float left = viewport[0] + u0 * viewport[2], top = viewport[1] + v0 * viewport[3];
    float width = (u1 - u0) * viewport[2], height = (v1 - v0) * viewport[3];
    GXSetViewport(left, top, width, height, viewport[4], viewport[5]);
    GXSetScissor(unsigned(left), unsigned(top), unsigned(width), unsigned(height));
    for (unsigned i = 0; i < count; ++i) {
        const auto& c = subjects[i].camera;
        nativeMenuFighter_DrawCamera(subjects[i].slot, c.yaw, c.pitch, c.zoom, c.pan_x, c.pan_y, c.frame, preview_width/preview_height);
    }
    GXSetViewport(viewport[0], viewport[1], viewport[2], viewport[3], viewport[4], viewport[5]);
    GXSetScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
    GXSetProjectionv(projection);
    HSD_StateInvalidate(-1);
}
