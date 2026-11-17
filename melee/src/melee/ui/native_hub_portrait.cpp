#include "native_hub_portrait.h"
#include <melee/lb/native_math_functions.h>
#include "native_hub_startup.h"
#include "native_hub_startup_roster.h"
#include "native_shell_workshop.h"
#include <melee/ft/native_fighter_draft_access.h>
#include <melee/ft/native_authoring_reply.h>
#include <melee/gm/native_feedback.h>
#include <melee/ft/native_fighter_family_project.h>
#include <melee/ft/native_gx_image.h>
#include <melee/gm/native_home.h>
#include <melee/mn/native_menu_fighter.h>
#include <melee/mn/native_menu_portrait.h>
#include <melee/mn/native_menu_scenery.h>
#include <melee/mn/native_menu_skin.h>
#include <melee/mn/native_menu_scene.h>
#include <melee/mn/native_menu_tile.h>
#include <melee/mn/native_css_original.h>
#include <melee/mn/native_css_cards.h>
#include <sysdolphin/baselib/controller.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace Presentation = NativeFighterPresentation;

namespace {
    constexpr float x = 71, y = 133, size = 264;
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    NativePortraitSubject subjects[2];
    NativeMenuPortrait saved_portraits[2];
    unsigned count;
    NativePortraitCamera camera;
    char destination[560];
    bool dragging, waiting;
    float small_size=64;
    bool editing,pending,return_editor,card_mode;
    bool install_capture;
    struct {
        Presentation::Framing framing;
        char label[64];
        bool card;
        unsigned id;
        const char* status = "idle";
    } capture;
    unsigned generation;
    unsigned edit_member;
    char draft[512],edit_name[64],edit_title[64],edit_template[NATIVE_FIGHTER_CODE_KEY_BYTES];
    void apply_camera();
    void fail(const char* message);
    char edit_error[192];
    std::string disk_revision;
    NativePortraitCamera saved_camera[2];
    bool has_saved_camera[2];
    char saved_animation[2][64], frame_text[32];
    bool animation_open;
    unsigned animation_offset;
    float last_frame() { return count ? std::min(1024.f, nativeMenuFighter_AnimationFrames(subjects[0].slot)) : 0; }
    void set_frame(float value)
    {
        camera.frame = std::clamp(value, 0.f, last_frame());
        std::snprintf(frame_text, sizeof(frame_text), "%.6g", camera.frame);
        apply_camera();
    }
    void commit_frame()
    {
        char* end; const float value = std::strtof(frame_text, &end);
        if (end == frame_text || *end || !std::isfinite(value)) { fail("Enter a frame number."); set_frame(camera.frame); }
        else if (value != camera.frame) set_frame(value);
    }
    bool select_animation(const char* name)
    {
        char previous[2][64];
        for (unsigned i = 0; i < count; ++i)
            std::snprintf(previous[i], sizeof(previous[i]), "%s", nativeMenuFighter_CurrentAnimation(subjects[i].slot));
        for (unsigned i = 0; i < count; ++i)
            if (!nativeMenuFighter_SelectAnimation(subjects[i].slot, name)) {
                for (unsigned n = 0; n < i; ++n) nativeMenuFighter_SelectAnimation(subjects[n].slot, previous[n]);
                return false;
            }
        animation_open = false;
        set_frame(0);
        return true;
    }
    void restore_camera()
    {
        camera = has_saved_camera[card_mode] ? saved_camera[card_mode] : NativePortraitCamera{};
        const float frame = camera.frame;
        if (saved_animation[card_mode][0] && !select_animation(saved_animation[card_mode]))
            has_saved_camera[card_mode] = false;
        set_frame(frame);
    }
    void fail(const char* message)
    {
        std::snprintf(editing?edit_error:nativeHubStartup.error,192,"%s",message);
    }

    const char* title() { return nativeHubPortrait_TileName()[0] ? nativeHubPortrait_TileName() : editing?edit_title:hub.title; }
    NativeMenuTilePortrait photo()
    {
        if (card_mode) {
            float height=size*56/64,width=height*136/188;
            return {x+(size-width)*.5f,y,width,height};
        }
        NativeMenuTilePortrait result;
        nativeMenuTile_PortraitRect(title(), x, y, size, size*56/64, &result);
        return result;
    }

    bool active() { return editing || (nativeHubStartup.active && nativeHubStartup.step == NativeStartup_Portrait); }
    void close_editor()
    {
        editing=pending=waiting=animation_open=false;
        for (auto& portrait : saved_portraits) nativeMenuPortrait_Free(&portrait);
        nativeMenuFighter_UnloadAll(); nativeMenuScenery_Unload();
        if (return_editor) {
            hub.visible=false; hub.dismissed=true;
            nativeShellWorkshop_Open(draft,NativeWorkshopOrigin::Hub);
        } else nativeHub_ChangePage(NativeHubPage_Workshop,true);
    }
    bool load_project()
    {
        NativeDraftAccess::Lease lease;
        std::string error;
        if (!lease.acquire(draft, error) || !NativeDraftAccess::revision(draft, disk_revision, error)) return false;
        NativeFighterProjectInfo info{};
        if (!nativeFighterProject_ReadInfo(draft, &info)) return false;
        std::snprintf(edit_name, sizeof(edit_name), "%s",
                      info.tile_name[0] ? info.tile_name : info.title);
        std::snprintf(edit_title, sizeof(edit_title), "%s", info.title);
        std::strcpy(edit_template, info.template_key);
        nativeMenuFighter_UnloadAll();
        count = 0;
        for (auto& portrait : saved_portraits) nativeMenuPortrait_Free(&portrait);
        std::memset(has_saved_camera, 0, sizeof(has_saved_camera));
        std::memset(saved_animation, 0, sizeof(saved_animation));

        NativeFighterFamilyProject family{};
        bool opened = nativeFighterFamilyProject_Open(draft, &family);
        NativeSourceResources resources{};
        bool has_resources = opened &&
            nativeSourceResources_Read(family.members[0].resources, family.members[0].resource_size,
                                       &resources, nullptr, 0);
        if (has_resources)
            for (unsigned mode=0;mode<2;++mode) {
                Presentation::Framing framing;
                has_saved_camera[mode] = Presentation::read_framing(resources,
                    mode ? Presentation::Target::Card : Presentation::Target::Tile, framing);
                if (has_saved_camera[mode]) {
                    saved_camera[mode] = framing.camera;
                    std::strcpy(saved_animation[mode], framing.animation);
                }
            }
        if (opened) {
            edit_member = std::min(edit_member, family.member_count - 1);
            if (nativeMenuFighter_LoadProject(0, &family.members[edit_member])) {
                subjects[0] = {0};
                count = 1;
                if (family.mode == NativeFighterFamily_SimultaneousCompanion) {
                    if (nativeMenuFighter_LoadProject(3, &family.members[1 - edit_member])) {
                        subjects[1] = {3};
                        count = 2;
                    } else {
                        nativeMenuFighter_UnloadAll();
                        count = 0;
                    }
                }
            }
        }
        if (!count) {
            // Keep the saved image and tile name independent of the 3D preview.
            char path[560];
            std::snprintf(path, sizeof(path), "%s/portrait.png", draft);
            size_t size = 0;
            auto* png = static_cast<unsigned char*>(SDL_LoadFile(path, &size));
            auto& tile = saved_portraits[0];
            if (png && size && size <= 1024 * 1024)
                tile.pixels = nativeGxImage_Decode(png, unsigned(size), &tile.width, &tile.height);
            SDL_free(png);
            if (has_resources) {
                const auto* card = nativeSourceResources_Find(&resources, "ui/player-card.png",
                                                               NativeSourceResource_PNG);
                auto& portrait = saved_portraits[1];
                if (card && card->size <= 1024 * 1024)
                    portrait.pixels = nativeGxImage_Decode(card->bytes, unsigned(card->size),
                                                           &portrait.width, &portrait.height);
            }
        }
        nativeFighterFamilyProject_Close(&family);
        if (count) for (unsigned mode = 0; mode < 2; ++mode)
            if (!saved_animation[mode][0]) std::snprintf(saved_animation[mode], sizeof(saved_animation[mode]), "%s", nativeMenuFighter_CurrentAnimation(subjects[0].slot));
        camera = {};
        nativeHubPortrait_Reset();
        if (count) restore_camera();
        return true;
    }
    float axis(float value)
    {
        // _Toy_80309404 uses this remapping after the menu's axis normalization.
        return std::fabs(value) < .2f ? 0 : (value - std::copysign(.2f, value)) / .8f;
    }
    void constrain()
    {
        camera.yaw = std::remainder(camera.yaw, 6.2831853f);
        camera.pitch = std::clamp(camera.pitch, -1.553343f, 1.553343f);
        camera.zoom = std::clamp(camera.zoom, .15f, 32.f);
    }
    void apply_camera()
    {
        for (unsigned i = 0; i < count; ++i) {
            subjects[i].camera = camera;
            if (count == 2) {
                subjects[i].camera.zoom *= .65f;
                subjects[i].camera.pan_x += i ? .37f : -.37f;
            }
        }
    }

    Presentation::Framing framing()
    {
        Presentation::Framing value{camera};
        std::snprintf(value.animation, sizeof(value.animation), "%s",
            count ? nativeMenuFighter_CurrentAnimation(subjects[0].slot) : "Wait1");
        return value;
    }

    void select_target(bool card)
    {
        if (card == card_mode) return;
        if (count) commit_frame();
        const auto previous = framing();
        saved_camera[card_mode] = previous.camera;
        has_saved_camera[card_mode] = true;
        std::strcpy(saved_animation[card_mode], previous.animation);
        card_mode = card;
        dragging = animation_open = false;
        restore_camera();
    }

    bool request_capture(bool install)
    {
        char root[512];
        const char* folder = nativeHubCatalog_Root(root);
        char temporary[MAX_PATH];
        if (!folder || !GetTempFileNameA(folder, "fmp", 0, temporary)) {
            fail("Could not create a temporary portrait in your user-data folder.");
            return false;
        }
        std::snprintf(destination, sizeof(destination), "%s", temporary);
        commit_frame();
        apply_camera();
        nativeHub_ChangePage(hub.page);
        edit_error[0] = 0;
        capture.framing = framing();
        std::strcpy(capture.label, nativeHubPortrait_TileName());
        capture.card = card_mode;
        const auto picture = photo();
        waiting = nativeMenuPortraitCapture_Request(subjects, count, destination, card_mode ? 136 : 464,
            card_mode ? 188 : unsigned(std::lround(464 * picture.height / picture.width)), card_mode);
        install_capture = install;
        if (++capture.id == 0) ++capture.id;
        capture.status = waiting ? "pending" : "failed";
        if (!waiting) {
            DeleteFileA(destination);
            fail("Could not capture this portrait. Try again.");
        }
        return waiting;
    }

    void draw_saved_portrait(const NativeMenuTilePortrait& rect)
    {
        const auto& portrait = saved_portraits[card_mode && saved_portraits[1].pixels ? 1 : 0];
        if (!portrait.pixels) return;
        if (card_mode) {
            nativeMenuPaint_Image(portrait.pixels, portrait.width, portrait.height, rect.x, rect.y, rect.width, rect.height);
            return;
        }
        float scale = std::min(rect.width / portrait.width, rect.height / portrait.height);
        float width = portrait.width * scale, height = portrait.height * scale;
        nativeMenuPaint_Image(portrait.pixels, portrait.width, portrait.height,
                             rect.x + (rect.width - width) * .5f,
                             rect.y + (rect.height - height) * .5f, width, height);
    }
}

void nativeHubPortrait_Reset()
{
    if (waiting) return;
    const float frame = camera.frame;
    camera = NativePortraitCamera{};
    dragging = animation_open = false;
    set_frame(frame);
}

void nativeHubPortrait_Begin()
{
    editing=pending=card_mode=false;
    waiting = dragging = animation_open = false;
    camera = {};
    count = nativeHubStartupRoster_Subjects(subjects);
    NativeCssLayout stock; FighterKind kinds[25];
    if (nativeCssOriginal_StockRoster(&stock,kinds) && stock.count) small_size=stock.tiles[0].width*.64f;
    nativeHubPortrait_Reset();
}

bool nativeHubPortrait_Busy() { return waiting; }
void nativeHubPortrait_Open(const char* path,unsigned member,bool return_to_editor)
{
    if (!path || !*path || nativeHubStartup.active) return;
    std::snprintf(draft,sizeof(draft),"%s",path); edit_member=member; return_editor=return_to_editor;
    pending=true; editing=card_mode=false; edit_error[0]=0;
    ++generation;
    disk_revision.clear();
    capture.status = "idle";
    if (!nativeHome_IsSelection()) nativeHome_ReturnToSelection();
}
bool nativeHubPortrait_Editing() { return editing; }
bool nativeHubPortrait_Requested() { return editing || pending; }
char* nativeHubPortrait_FrameText() { return frame_text; }
char* nativeHubPortrait_TileName() { return editing?edit_name:nativeHubStartup.tile_name; }
bool nativeHubPortrait_Activate(HubAction action,int value)
{
    if (!active()) return false;
    if (waiting) return true;
    if (animation_open && (action == HubAction::Back || action == HubAction::Close)) {
        animation_open = false; nativeHub_ChangePage(hub.page); return true;
    }
    if (action == HubAction::PortraitAnimation && count) {
        if (value < 0) { animation_open = !animation_open; animation_offset = 0; }
        else {
            char name[64]; const char* selected = nativeMenuFighter_AnimationName(subjects[0].slot, unsigned(value));
            if (selected) { std::snprintf(name, sizeof(name), "%s", selected);
                if (!select_animation(name)) fail("Could not open this animation."); }
        }
        nativeHub_ChangePage(hub.page); return true;
    }
    if (action == HubAction::PortraitAnimationPage) {
        const unsigned total = count ? nativeMenuFighter_AnimationCount(subjects[0].slot) : 0;
        animation_offset = unsigned(std::clamp(int(animation_offset) + value, 0, total ? int((total - 1) / 8 * 8) : 0));
        nativeHub_ChangePage(hub.page); return true;
    }
    if (action == HubAction::PortraitFrameStep) { set_frame(camera.frame + value); nativeHub_ChangePage(hub.page); return true; }
    if (action==HubAction::PortraitReset) { nativeHubPortrait_Reset(); return true; }
    if (action==HubAction::PortraitSave) { nativeHubPortrait_Save(); return true; }
    if (editing && action==HubAction::PortraitMode) {
        select_target(value != 0);
        nativeHub_ChangePage(hub.page);
        return true;
    }
    if (editing && (action==HubAction::Back || action==HubAction::Close)) { close_editor(); return true; }
    return false;
}

void nativeHubPortrait_Save()
{
    const char* label=nativeHubPortrait_TileName();
    if (!active() || waiting || !*label || std::strspn(label," \t\r\n")==std::strlen(label)) return;
    if (!count) {
        if (!editing || card_mode) return;
        std::string error;
        if (Presentation::save(draft, disk_revision.c_str(), Presentation::Target::Tile, edit_name,
                               nullptr, nullptr, disk_revision, error) != Presentation::Result::Saved)
            fail(error.c_str());
        else if (!nativeHubCatalog_PublishDraft(draft, NativeHubPublish::Install))
            fail("Tile name saved to the draft. Could not update the installed fighter.");
        else
            close_editor();
        return;
    }
    request_capture(editing);
}

void nativeHubPortrait_Tick()
{
    if (pending && nativeHome_IsSelection() && nativeCssOriginal_CanEdit()) {
        pending=false; waiting=false;
        if (!load_project()) {
            nativeMenuFighter_UnloadAll(); nativeHub_Open(NativeHubPage_Workshop);
            nativeHub_Notice("Could not read this fighter's draft."); return;
        }
        editing=true; nativeHub_Open(NativeHubPage_Portrait); nativeMenuScenery_Load("tournament-background");
    }
    if (active() && count && !waiting && hub.field != frame_text) commit_frame();
    if (!active() || !waiting) return;
    const auto state = nativeMenuPortraitCapture_State();
    if (state == NativePortraitCaptureState::Saved) {
        waiting = false;
        capture.status = "saved";
        if (editing) {
            std::string error;
            const auto target = capture.card ? Presentation::Target::Card : Presentation::Target::Tile;
            if (Presentation::save(draft, disk_revision.c_str(), target, capture.card ? nullptr : capture.label,
                                   destination, &capture.framing, disk_revision, error) != Presentation::Result::Saved) {
                capture.status = "failed";
                fail(error.c_str());
            } else if (install_capture) {
                if (nativeHubCatalog_PublishDraft(draft, NativeHubPublish::Install)) close_editor();
                else fail("Portrait saved to the draft. Could not update the installed fighter.");
            }
        } else nativeHubStartup_PortraitSaved(destination, capture.framing);
        DeleteFileA(destination);
    } else if (state == NativePortraitCaptureState::Failed) {
        waiting = false;
        capture.status = "failed";
        DeleteFileA(destination);
        fail("Could not save this portrait. Check free space and try again.");
    }
}

bool nativeHubPortrait_Controller(const HSD_PadStatus& pad)
{
    if (!active() || !count) return false;
    if (waiting) return true;
    const auto picture = photo();
    const bool pan = (pad.button & PAD_BUTTON_A) && nativeHub_Hover(picture.x, picture.y, picture.width, picture.height);
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
    camera.pan_x += .015789474f * dx / camera.zoom;
    camera.pan_y += .015789474f * dy / camera.zoom;
    if (!dx && !dy) {
        if (pad.button & PAD_BUTTON_Y) camera.zoom /= .975f;
        else if (pad.button & PAD_BUTTON_X) camera.zoom /= 1.025f;
    }
    constrain(); apply_camera();
    // Holding A over the image belongs to panning, not the hand or buttons.
    return pan;
}

bool nativeHubPortrait_Event(const SDL_Event* event)
{
    if (!active() || !count) { dragging = false; return false; }
    if (!waiting && animation_open && event->type == SDL_EVENT_MOUSE_WHEEL && nativeHub_Hover(357,183,246,230))
        return nativeHubPortrait_Activate(HubAction::PortraitAnimationPage,event->wheel.y > 0 ? -8 : 8);
    const auto picture = photo();
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_MIDDLE) {
        dragging = false; return true;
    }
    if (event->type == SDL_EVENT_WINDOW_FOCUS_LOST) dragging = false;
    if (waiting) return event->type == SDL_EVENT_MOUSE_WHEEL ||
        (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_MIDDLE);
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_MIDDLE) {
        dragging = nativeHub_Hover(picture.x, picture.y, picture.width, picture.height);
        return dragging;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION && dragging) {
        SDL_Window* window = SDL_GetWindowFromID(event->motion.windowID);
        int width = 640, height = 480;
        if (window) SDL_GetWindowSize(window, &width, &height);
        float pixels = std::max(.1f, std::min(width / 640.f, height / 480.f));
        float dx = event->motion.xrel / pixels, dy = event->motion.yrel / pixels;
        if (SDL_GetModState() & SDL_KMOD_SHIFT) {
            camera.pan_x += 2 * dx / (picture.height*camera.zoom); camera.pan_y -= 2 * dy / (picture.height*camera.zoom);
        } else {
            camera.yaw -= dx * .012f; camera.pitch += dy * .012f;
        }
        constrain(); apply_camera(); return true;
    }
    if (event->type == SDL_EVENT_MOUSE_WHEEL && nativeHub_Hover(picture.x, picture.y, picture.width, picture.height)) {
        camera.zoom *= cr_powf(1.12f, event->wheel.y);
        constrain(); apply_camera(); return true;
    }
    return false;
}

void nativeHubPortrait_Render()
{
    nativeMenuPaint_Text(editing?edit_title:"Your fighter's portrait", 266, 96, 338, 26, gold, NativeMenuFont_MenuCaption);
    const auto picture = photo();
    if (card_mode) nativeMenuSkin_Panel(picture.x-3,picture.y-3,picture.width+6,picture.height+6,0xC2BEC7FF,0x10101880);
    else nativeMenuTile_Draw(nullptr, 0, 0, title(), x, y, size, size*56/64);
    if (count) nativeMenuPortraitCapture_Preview(subjects, count, picture.x, picture.y, picture.width, picture.height, card_mode);
    else if (editing) draw_saved_portrait(picture);
    Mtx canvas; Mtx44 projection; nativeMenuScene_View(false, false, canvas, projection);
    nativeMenuPaint_Begin(canvas);
    NativeMenuTilePortrait small;
    if (card_mode) {
        NativeCssCardPreview card;
        if (nativeCssCards_DrawPreview(canvas, 365, 260, 90, &card))
            small = {card.x, card.y, card.width, card.height};
        else small = {365, 260, 90 * 13.6f / 14.8f, 90 * 18.8f / 14.8f};
    } else {
        nativeMenuTile_Draw(nullptr,0,0,title(),395,327,small_size,small_size*56/64);
        nativeMenuTile_PortraitRect(title(),395,327,small_size,small_size*56/64,&small);
    }
    if (count) nativeMenuPortraitCapture_Preview(subjects,count,small.x,small.y,small.width,small.height,card_mode,card_mode ? 136.f/188 : 0);
    else if (editing) draw_saved_portrait(small);
    nativeMenuPaint_Begin(canvas);
    if (card_mode) nativeMenuFont_Draw(NativeMenuFont_ResultName, editing ? edit_title : hub.title, 365 + .4f * 90 / 14.8f,
        -260 - 18 * 90 / 14.8f, .8f, 13.2f * 90 / 14.8f, 1.15f * 90 / 14.8f, 0xF5EFEAFF, true, true);
    if (editing) {
        nativeHub_Button("portrait-mode-tile","CSS tile",365,141,108,25,HubAction::PortraitMode,0,!card_mode);
        nativeHub_Button("portrait-mode-card","Player card",481,141,114,25,HubAction::PortraitMode,1,card_mode);
    } else nativeMenuPaint_Text(hub.title,362,141,238,22,gold,NativeMenuFont_MenuCaption);
    if (!card_mode) {
        nativeMenuPaint_Text("Short tile name",365,263,230,11,muted);
        nativeHub_Field("setup-tile-name",nativeHubPortrait_TileName(),365,280,230,30,HubAction::PortraitTileName);
    }
    if (count) {
        nativeMenuPaint_Text("Animation",365,176,230,10,muted);
        nativeHub_Button("portrait-animation", nativeMenuFighter_CurrentAnimation(subjects[0].slot),365,190,230,25,HubAction::PortraitAnimation,-1);
        nativeMenuPaint_Text("Frame",365,231,44,10,muted);
        nativeHub_Field("portrait-frame",frame_text,410,223,76,28,HubAction::PortraitFrame);
        nativeHub_Button("portrait-frame-prev","<",490,223,28,28,HubAction::PortraitFrameStep,-1,false,camera.frame>0);
        nativeHub_Button("portrait-frame-next",">",522,223,28,28,HubAction::PortraitFrameStep,1,false,camera.frame<last_frame());
        char frames[24]; std::snprintf(frames,sizeof(frames),"/ %.0f",last_frame());
        nativeMenuPaint_Text(frames,555,231,43,10,muted);
        nativeMenuPaint_Text("C-stick: orbit    X/Y: zoom    D-pad: pan",72,378,264,8,muted);
        nativeMenuPaint_Text("Middle-drag: orbit    Wheel: zoom",72,391,264,8,muted);
        nativeMenuPaint_Text("Shift + middle-drag / A + stick: pan",72,404,264,8,muted);
        nativeHub_Button("setup-portrait-reset", "Reset view", 480, 375, 115, 26,
                         HubAction::PortraitReset, 0, false, !waiting);
    } else if (editing) nativeMenuPaint_Text("3D preview unavailable",365,234,230,11,muted);
    nativeMenuPaint_Text(card_mode?"P1 card":"CSS size",486,338,105,10,muted);
    if (count || (editing && !card_mode))
        nativeHub_Button("setup-portrait-save", waiting ? "Saving..." : count ? "Use portrait" : "Save tile name",
                         386, 418, 216, 37, HubAction::PortraitSave, 0, true, !waiting);
    if (editing) nativeHub_Button("portrait-back","< Back",40,427,110,28,HubAction::Back);
    if (editing && edit_error[0]) nativeMenuPaint_Text(edit_error,40,466,555,9,0xFFAE92FF);
    if (animation_open && count) {
        const unsigned total = nativeMenuFighter_AnimationCount(subjects[0].slot);
        nativeMenuSkin_Panel(357,183,246,230,0xC2BEC7FF,0x09080EFF);
        for (unsigned i = animation_offset; i < total && i < animation_offset + 8; ++i) {
            char id[40]; std::snprintf(id,sizeof(id),"portrait-animation-%u",i);
            const char* name = nativeMenuFighter_AnimationName(subjects[0].slot,i);
            nativeHub_Button(id,name,365,190+(i-animation_offset)*23,230,22,HubAction::PortraitAnimation,int(i),
                !std::strcmp(name,nativeMenuFighter_CurrentAnimation(subjects[0].slot)));
        }
        nativeHub_Button("portrait-animation-prev","<",365,382,45,23,HubAction::PortraitAnimationPage,-8,false,animation_offset>0);
        nativeHub_Button("portrait-animation-next",">",550,382,45,23,HubAction::PortraitAnimationPage,8,false,animation_offset+8<total);
        char page[40]; std::snprintf(page,sizeof(page),"%u-%u / %u",animation_offset+1,std::min(animation_offset+8,total),total);
        nativeMenuPaint_Text(page,420,389,120,9,muted);
    }
}

void nativeHubPortrait_Inspect(const char* element)
{
    if (!std::strcmp(element, "portrait-animations")) {
        for (unsigned i = 0; count && i < nativeMenuFighter_AnimationCount(subjects[0].slot); ++i)
            std::fprintf(stderr, "[hub-inspect] portrait-animation index=%u name=%s\n", i, nativeMenuFighter_AnimationName(subjects[0].slot, i));
        return;
    }
    std::fprintf(stderr, "[hub-inspect] portrait-draft path=%s\n", editing ? draft : "startup");
    std::fprintf(stderr, "[hub-inspect] startup-camera active=%d subjects=%u waiting=%d yaw=%.6f pitch=%.6f zoom=%.6f pan=%.6f,%.6f card=%d animation=%s frame=%.6f last=%.6f clips=%u\n",
        active(), count, waiting, camera.yaw, camera.pitch, camera.zoom, camera.pan_x, camera.pan_y, card_mode,
        count ? nativeMenuFighter_CurrentAnimation(subjects[0].slot) : "none", camera.frame, last_frame(),
        count ? nativeMenuFighter_AnimationCount(subjects[0].slot) : 0);
}

namespace {
using NativeAuthoring::Reply;
using NativeAuthoring::add_string;

bool portrait_revision(char (&hex)[65])
{
    const auto pose = framing();
    char identity[2048];
    const int size = std::snprintf(identity, sizeof(identity), "%u/%u/%u/%d/%d/%d/%u/%s\n%s\n%s\n%s\n%.9g %.9g %.9g %.9g %.9g %.9g\n%s",
        generation, edit_member, count, pending, waiting, card_mode, capture.id, capture.status,
        draft, disk_revision.c_str(), edit_name, camera.yaw, camera.pitch, camera.zoom,
        camera.pan_x, camera.pan_y, camera.frame, pose.animation);
    NativeContentId id{};
    if (size < 0 || size >= sizeof(identity) || !nativeContent_Hash(identity, size, &id)) return false;
    nativeContentStore_IdHex(&id, hex);
    return true;
}

bool animation_available(const char* animation)
{
    for (unsigned i = 0; i < count; ++i) {
        const unsigned slot = subjects[i].slot;
        unsigned j = 0, total = nativeMenuFighter_AnimationCount(slot);
        while (j < total && std::strcmp(animation, nativeMenuFighter_AnimationName(slot, j))) ++j;
        if (j == total) return false;
    }
    return count != 0;
}

void describe_portrait(Reply& reply, unsigned offset)
{
    char hash[65];
    if (!portrait_revision(hash)) { reply.fail("revision_unavailable", "Could not fingerprint this portrait view."); return; }
    add_string(reply, reply.result, "revision", hash);
    add_string(reply, reply.result, "project", draft);
    add_string(reply, reply.result, "status", pending ? "opening" : waiting ? "saving" : "ready");
    if (pending) return;
    add_string(reply, reply.result, "saved_disk_revision", disk_revision);
    add_string(reply, reply.result, "target", card_mode ? "card" : "tile");
    add_string(reply, reply.result, "tile_name", edit_name);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "subjects", count);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "member_index", edit_member);
    if (capture.id && std::strcmp(capture.status, "idle")) {
        auto* result = yyjson_mut_obj(reply.document);
        yyjson_mut_obj_add_uint(reply.document, result, "id", capture.id);
        add_string(reply, result, "status", capture.status);
        add_string(reply, result, "target", capture.card ? "card" : "tile");
        yyjson_mut_obj_add_val(reply.document, reply.result, "capture", result);
    }
    if (edit_error[0]) add_string(reply, reply.result, "message", edit_error);
    if (!count) return;
    const auto pose = framing();
    add_string(reply, reply.result, "animation", pose.animation);
    yyjson_mut_obj_add_real(reply.document, reply.result, "frame", camera.frame);
    yyjson_mut_obj_add_real(reply.document, reply.result, "last_frame", last_frame());
    auto* view = yyjson_mut_obj(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "camera", view);
    yyjson_mut_obj_add_real(reply.document, view, "yaw", camera.yaw);
    yyjson_mut_obj_add_real(reply.document, view, "pitch", camera.pitch);
    yyjson_mut_obj_add_real(reply.document, view, "zoom", camera.zoom);
    yyjson_mut_obj_add_real(reply.document, view, "pan_x", camera.pan_x);
    yyjson_mut_obj_add_real(reply.document, view, "pan_y", camera.pan_y);
    const unsigned total = nativeMenuFighter_AnimationCount(subjects[0].slot);
    auto* clips = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "animations", clips);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "total_animations", total);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "animation_offset", offset);
    for (unsigned i = offset; i < total && i - offset < 100; ++i) {
        const char* name = nativeMenuFighter_AnimationName(subjects[0].slot, i);
        if (name && animation_available(name)) yyjson_mut_arr_append(clips, yyjson_mut_strcpy(reply.document, name));
    }
}
}

void nativeHubPortrait_Agent(const char* method, yyjson_val* args, Reply& reply)
{
    if (!nativeHubPortrait_Requested()) {
        reply.fail("portrait_closed", "Open Portrait & tile from the Workshop first.");
        return;
    }
    if (!std::strcmp(method, "inspect")) {
        describe_portrait(reply, unsigned(yyjson_get_uint(yyjson_obj_get(args, "offset"))));
        return;
    }
    if (pending || waiting || nativeFeedback_IsOpen() || hub.field ||
        !nativeHome_IsSelection() || !nativeCssOriginal_CanEdit() || !hub.visible || hub.page != NativeHubPage_Portrait) {
        reply.fail("portrait_busy", "Finish the portrait transition, text field edit or capture first.");
        return;
    }
    const char* expected = yyjson_get_str(yyjson_obj_get(args, "revision"));
    char hash[65];
    if (!expected || !portrait_revision(hash) || std::strcmp(hash, expected)) {
        reply.fail("revision_conflict", "Inspect the current portrait before changing it.");
        return;
    }
    if (!std::strcmp(method, "close")) {
        close_editor();
        add_string(reply, reply.result, "status", "closed");
        return;
    }
    if (!std::strcmp(method, "capture")) {
        if (!count || !request_capture(false)) {
            reply.fail("portrait_capture_failed", count ? edit_error : "This fighter has no loaded 3D portrait preview.");
            return;
        }
        describe_portrait(reply, 0);
        return;
    }
    const char* name = yyjson_get_str(yyjson_obj_get(args, "tile_name"));
    const char* clip = yyjson_get_str(yyjson_obj_get(args, "animation"));
    const char* target = yyjson_get_str(yyjson_obj_get(args, "target"));
    auto* camera_args = yyjson_obj_get(args, "camera");
    auto* frame = yyjson_obj_get(args, "frame");
    if (name) {
        char manifest[256];
        if (!nativeFighterProject_EncodeManifest(edit_title, name, edit_template, manifest, sizeof(manifest))) {
            reply.fail("invalid_tile_name", "Use at most 63 bytes without control characters.");
            return;
        }
    }
    if ((clip && !animation_available(clip)) || (!count && (camera_args || frame))) {
        reply.fail("animation_unavailable", "Choose a clip available for every portrait subject.");
        return;
    }
    if (target) select_target(std::strcmp(target, "card") == 0);
    if (clip && !select_animation(clip)) {
        reply.fail("animation_unavailable", "Could not open that portrait animation.");
        return;
    }
    if (name) std::strcpy(edit_name, name);
    if (camera_args) {
        const auto read = [camera_args](const char* key, float& component) {
            if (auto* value = yyjson_obj_get(camera_args, key)) component = float(yyjson_get_num(value));
        };
        read("yaw", camera.yaw);
        read("pitch", camera.pitch);
        read("zoom", camera.zoom);
        read("pan_x", camera.pan_x);
        read("pan_y", camera.pan_y);
        constrain();
    }
    set_frame(frame ? float(yyjson_get_num(frame)) : camera.frame);
    nativeHub_ChangePage(hub.page);
    describe_portrait(reply, 0);
}
