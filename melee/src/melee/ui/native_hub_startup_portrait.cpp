#include "native_hub_startup_portrait.h"
#include "native_hub_startup.h"
#include "native_hub_startup_roster.h"
#include <melee/mn/native_menu_scene.h>
#include <sysdolphin/baselib/controller.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
    constexpr float x = 71, y = 133, size = 264;
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    NativePortraitSubject subjects[2];
    unsigned count;
    NativePortraitCamera camera;
    char destination[560];
    bool dragging, waiting;

    bool active() { return nativeHubStartup.active && nativeHubStartup.step == NativeStartup_Portrait; }
    float axis(float value)
    {
        // _Toy_80309404 uses this remapping after the menu's axis normalization.
        return std::fabs(value) < .2f ? 0 : (value - std::copysign(.2f, value)) / .8f;
    }
    void constrain()
    {
        camera.yaw = std::remainder(camera.yaw, 6.2831853f);
        camera.pitch = std::clamp(camera.pitch, -1.553343f, 1.553343f);
        camera.zoom = std::clamp(camera.zoom, .15f, 8.f);
        camera.pan_x = std::clamp(camera.pan_x, -4.f, 4.f);
        camera.pan_y = std::clamp(camera.pan_y, -4.f, 4.f);
    }
    void apply_camera()
    {
        for (unsigned i = 0; i < count; ++i) {
            subjects[i].camera = camera;
            if (count == 2) {
                subjects[i].camera.zoom *= .65f;
                subjects[i].camera.pan_x += (i ? .37f : -.37f) * camera.zoom;
            }
        }
    }
}

void nativeHubStartupPortrait_Reset()
{
    if (waiting) return;
    camera = NativePortraitCamera{};
    dragging = false;
    apply_camera();
}

void nativeHubStartupPortrait_Begin()
{
    waiting = dragging = false;
    count = nativeHubStartupRoster_Subjects(subjects);
    nativeHubStartupPortrait_Reset();
}

bool nativeHubStartupPortrait_Busy() { return waiting; }

void nativeHubStartupPortrait_Save()
{
    if (!active() || waiting || !count) return;
    char root[512]; const char* folder = nativeHubCatalog_Root(root);
    if (!folder) {
        std::snprintf(nativeHubStartup.error, sizeof(nativeHubStartup.error), "Could not open your user-data folder.");
        return;
    }
    std::snprintf(destination, sizeof(destination), "%s/startup-portrait.png", folder);
    apply_camera();
    waiting = nativeMenuPortraitCapture_Request(subjects, count, destination);
    if (!waiting) std::snprintf(nativeHubStartup.error, sizeof(nativeHubStartup.error), "Could not capture this portrait. Try again.");
}

void nativeHubStartupPortrait_Tick()
{
    if (!active() || !waiting) return;
    const auto state = nativeMenuPortraitCapture_State();
    if (state == NativePortraitCaptureState::Saved) {
        waiting = false;
        nativeHubStartup_PortraitSaved(destination);
    } else if (state == NativePortraitCaptureState::Failed) {
        waiting = false;
        std::snprintf(nativeHubStartup.error, sizeof(nativeHubStartup.error), "Could not save this portrait. Check free space and try again.");
    }
}

bool nativeHubStartupPortrait_Controller(const HSD_PadStatus& pad)
{
    if (!active()) return false;
    if (waiting) return true;
    const bool pan = (pad.button & PAD_BUTTON_A) && nativeHub_Hover(x, y, size, size);
    float dx = 0, dy = 0;
    if (pad.button & PAD_BUTTON_LEFT) dx = -1;
    if (pad.button & PAD_BUTTON_RIGHT) dx = 1;
    if (pad.button & PAD_BUTTON_UP) dy = 1;
    if (pad.button & PAD_BUTTON_DOWN) dy = -1;
    if (pan) {
        if (float a = axis(pad.nml_stickX)) dx = a;
        if (float a = axis(pad.nml_stickY)) dy = a;
    }
    // The gallery turns the camera; this renderer turns the subject instead.
    camera.yaw -= .052359878f * axis(pad.nml_subStickX);
    camera.pitch += .052359878f * axis(pad.nml_subStickY);
    camera.pan_x += .015789474f * dx;
    camera.pan_y += .015789474f * dy;
    if (!dx && !dy) {
        if (pad.button & PAD_BUTTON_Y) camera.zoom /= .975f;
        else if (pad.button & PAD_BUTTON_X) camera.zoom /= 1.025f;
    }
    constrain(); apply_camera();
    // Holding A over the image belongs to panning, not the hand or buttons.
    return pan;
}

bool nativeHubStartupPortrait_Event(const SDL_Event* event)
{
    if (!active()) { dragging = false; return false; }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_MIDDLE) {
        dragging = false; return true;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) dragging = false;
    if (waiting) return event->type == SDL_EVENT_MOUSE_WHEEL ||
        (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_MIDDLE);
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_MIDDLE) {
        dragging = nativeHub_Hover(x, y, size, size);
        return dragging;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION && dragging) {
        SDL_Window* window = SDL_GetWindowFromID(event->motion.windowID);
        int width = 640, height = 480;
        if (window) SDL_GetWindowSize(window, &width, &height);
        float pixels = std::max(.1f, std::min(width / 640.f, height / 480.f));
        float dx = event->motion.xrel / pixels, dy = event->motion.yrel / pixels;
        if (SDL_GetModState() & SDL_KMOD_SHIFT) {
            camera.pan_x += 2 * dx / size; camera.pan_y -= 2 * dy / size;
        } else {
            camera.yaw -= dx * .012f; camera.pitch += dy * .012f;
        }
        constrain(); apply_camera(); return true;
    }
    if (event->type == SDL_EVENT_MOUSE_WHEEL && nativeHub_Hover(x, y, size, size)) {
        camera.zoom *= std::pow(1.12f, event->wheel.y);
        constrain(); apply_camera(); return true;
    }
    return false;
}

void nativeHubStartupPortrait_Render()
{
    nativeMenuPaint_Text("Your fighter's portrait", 266, 96, 338, 26, gold, NativeMenuFont_MenuCaption);
    nativeMenuPaint_Round(x - 2, y - 2, size + 4, size + 4, 4, 0xB9ADBD99);
    nativeMenuPaint_Rect(x, y, size, size, 0x101018FF);
    nativeMenuPortraitCapture_Preview(subjects, count, x, y, size);
    Mtx canvas; Mtx44 projection; nativeMenuScene_View(false, false, canvas, projection);
    nativeMenuPaint_Begin(canvas);
    nativeMenuPaint_Text(hub.title, 362, 154, 238, 25, gold, NativeMenuFont_MenuCaption);
    nativeMenuPaint_Text("C-stick  Orbit", 365, 211, 220, 13, muted);
    nativeMenuPaint_Text("A + stick on portrait / D-pad  Pan", 365, 236, 230, 11, muted);
    nativeMenuPaint_Text("X / Y  Zoom", 365, 261, 220, 13, muted);
    nativeMenuPaint_Text("Middle-drag  Orbit", 365, 307, 230, 10, muted);
    nativeMenuPaint_Text("Shift + middle-drag  Pan", 365, 325, 230, 10, muted);
    nativeMenuPaint_Text("Wheel  Zoom", 365, 343, 230, 10, muted);
    nativeHub_Button("setup-portrait-reset", "Reset view", 363, 368, 150, 26,
        HubAction::StartupPortraitReset, 0, false, !waiting);
    nativeHub_Button("setup-portrait-save", waiting ? "Saving..." : "Use portrait", 386, 418, 216, 37,
        HubAction::StartupPortraitSave, 0, true, count && !waiting);
}

void nativeHubStartupPortrait_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] startup-camera active=%d subjects=%u waiting=%d yaw=%.6f pitch=%.6f zoom=%.6f pan=%.6f,%.6f\n",
        active(), count, waiting, camera.yaw, camera.pitch, camera.zoom, camera.pan_x, camera.pan_y);
}
