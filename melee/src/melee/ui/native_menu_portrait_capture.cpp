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
        bool queued, started, failed, shadow;
    } capture;

    void render_portrait(void*)
    {
        HSD_StateSetAlphaWriteOverride(true);
        for (unsigned pass = capture.shadow ? 0 : 1; pass < 2; ++pass)
        for (unsigned i = 0; i < capture.count; ++i) {
            const auto& subject = capture.subjects[i];
            const auto& camera = subject.camera;
            nativeMenuFighter_DrawCamera(subject.slot, camera.yaw, camera.pitch, camera.zoom, camera.pan_x, camera.pan_y, camera.frame,
                float(capture.width)/capture.height, pass ? NativeMenuFighter_Normal : NativeMenuFighter_Shadow);
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
                                       unsigned width, unsigned height, bool shadow)
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
    capture.width = width; capture.height = height; capture.shadow = shadow;
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

void nativeMenuPortraitCapture_Stage(const char* specification)
{
    unsigned id,kind,costume; int end=0; char animation[64];
    NativePortraitSubject subject{NATIVE_MENU_FIGHTERS-1};
    auto& c=subject.camera;
    bool ok=std::sscanf(specification,"%u,%u,%u,%63[A-Za-z0-9_],%f,%f,%f,%f,%f,%f%n",
        &id,&kind,&costume,animation,&c.frame,&c.yaw,&c.pitch,&c.zoom,&c.pan_x,&c.pan_y,&end)==10 &&
        specification[end]==0 && id<=99999 && std::isfinite(c.frame) && c.frame>=0 && c.frame<=1024 &&
        std::isfinite(c.yaw) && std::isfinite(c.pitch) && std::isfinite(c.zoom) && c.zoom>=.15f && c.zoom<=32 &&
        std::isfinite(c.pan_x) && std::isfinite(c.pan_y);
    auto state=nativeMenuPortraitCapture_State();
    ok=ok && state!=NativePortraitCaptureState::Queued && state!=NativePortraitCaptureState::Saving;
    // The shot tool uses stock costumes and the requested named animation.
    // Move-specific visibility is evaluated by the same preview importer.
    short variants[11]; for (unsigned i=0;i<11;++i) variants[i]=i==0?0:-1;
    if (ok) ok=nativeMenuFighter_Load(subject.slot,FighterKind(kind),costume,animation,variants);
    char path[48];
    if (ok) {
        std::snprintf(path,sizeof(path),"menu-shot-%05u.png",id);
        ok=nativeMenuPortraitCapture_Request(&subject,1,path,512,512);
    }
    std::fprintf(stderr,"[hub-inspect] menu-shot accepted=%d\n",ok);
}

void nativeMenuPortraitCapture_Preview(const NativePortraitSubject* subjects, unsigned count,
                                       float x, float y, float preview_width, float preview_height, bool shadow, float image_aspect)
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
    for (unsigned pass = shadow ? 0 : 1; pass < 2; ++pass)
    for (unsigned i = 0; i < count; ++i) {
        const auto& c = subjects[i].camera;
        nativeMenuFighter_DrawCamera(subjects[i].slot, c.yaw, c.pitch, c.zoom, c.pan_x, c.pan_y, c.frame, image_aspect > 0 ? image_aspect : preview_width/preview_height, pass ? NativeMenuFighter_Normal : NativeMenuFighter_Shadow);
    }
    GXSetViewport(viewport[0], viewport[1], viewport[2], viewport[3], viewport[4], viewport[5]);
    GXSetScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
    GXSetProjectionv(projection);
    HSD_StateInvalidate(-1);
}
