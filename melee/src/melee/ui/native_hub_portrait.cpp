#include "native_hub_portrait.h"
#include <melee/lb/native_math_functions.h>
#include "native_hub_startup.h"
#include "native_hub_startup_roster.h"
#include "native_shell_workshop.h"
#include <melee/ft/native_fighter_family_project.h>
#include <melee/gm/native_home.h>
#include <melee/mn/native_menu_fighter.h>
#include <melee/mn/native_menu_scenery.h>
#include <melee/mn/native_menu_skin.h>
#include <melee/mn/native_menu_scene.h>
#include <melee/mn/native_menu_tile.h>
#include <melee/mn/native_css_original.h>
#include <sysdolphin/baselib/controller.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
    constexpr float x = 71, y = 133, size = 264;
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    NativePortraitSubject subjects[2];
    unsigned count;
    NativePortraitCamera camera;
    char destination[560];
    bool dragging, waiting;
    float small_size=64;
    bool editing,pending,return_editor,card_mode;
    unsigned edit_member;
    char draft[512],edit_name[64],edit_title[64];
    void apply_camera();
    char edit_error[192];
    NativePortraitCamera saved_camera[2];
    bool has_saved_camera[2];
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
        editing=pending=waiting=false;
        nativeMenuFighter_UnloadAll(); nativeMenuScenery_Unload();
        if (return_editor) {
            hub.visible=false; hub.dismissed=true;
            nativeShellWorkshop_Open(draft,NativeWorkshopOrigin::Hub);
        } else nativeHub_ChangePage(NativeHubPage_Workshop,true);
    }
    bool load_project()
    {
        NativeFighterFamilyProject family{};
        if (!nativeFighterFamilyProject_Open(draft,&family)) return false;
        edit_member=std::min(edit_member,family.member_count-1);
        auto& primary=family.members[edit_member];
        NativeSourceResources resources;
        std::memset(has_saved_camera,0,sizeof(has_saved_camera));
        if (nativeSourceResources_Read(family.members[0].resources,family.members[0].resource_size,&resources,nullptr,0))
            for (unsigned mode=0;mode<2;++mode) {
                const auto* record=nativeSourceResources_Find(&resources,mode?"ui/portrait-card.camera":"ui/portrait-tile.camera",NativeSourceResource_Blob);
                if (!record || record->size>=192) continue;
                char text[192]{}; std::memcpy(text,record->bytes,record->size);
                auto& c=saved_camera[mode];
                has_saved_camera[mode]=std::sscanf(text,"%f %f %f %f %f %f",&c.yaw,&c.pitch,&c.zoom,&c.pan_x,&c.pan_y,&c.frame)==6 &&
                    std::isfinite(c.yaw) && std::isfinite(c.pitch) && std::isfinite(c.zoom) && std::isfinite(c.pan_x) && std::isfinite(c.pan_y) &&
                    std::isfinite(c.frame) && c.zoom>=.15f && c.zoom<=32 && c.frame>=0 && c.frame<=1024;
            }
        std::snprintf(edit_name,sizeof(edit_name),"%s",primary.tile_name[0]?primary.tile_name:primary.title);
        std::snprintf(edit_title,sizeof(edit_title),"%s",primary.title);
        nativeMenuFighter_UnloadAll();
        count=family.mode==NativeFighterFamily_SimultaneousCompanion?2:1;
        bool ok=nativeMenuFighter_LoadProject(0,&primary);
        subjects[0]={0};
        if (ok && count==2) { ok=nativeMenuFighter_LoadProject(3,&family.members[1-edit_member]); subjects[1]={3}; }
        nativeFighterFamilyProject_Close(&family);
        if (ok) {
            nativeHubPortrait_Reset();
            if (has_saved_camera[0]) { camera=saved_camera[0]; apply_camera(); }
        }
        return ok;
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
}

void nativeHubPortrait_Reset()
{
    if (waiting) return;
    camera = NativePortraitCamera{};
    dragging = false;
    apply_camera();
}

void nativeHubPortrait_Begin()
{
    editing=pending=card_mode=false;
    waiting = dragging = false;
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
    if (!nativeHome_IsSelection()) nativeHome_ReturnToSelection();
}
bool nativeHubPortrait_Editing() { return editing; }
char* nativeHubPortrait_TileName() { return editing?edit_name:nativeHubStartup.tile_name; }
bool nativeHubPortrait_Activate(HubAction action,int value)
{
    if (!active()) return false;
    if (waiting) return true;
    if (action==HubAction::PortraitReset) { nativeHubPortrait_Reset(); return true; }
    if (action==HubAction::PortraitSave) { nativeHubPortrait_Save(); return true; }
    if (editing && action==HubAction::PortraitMode) {
        saved_camera[card_mode]=camera; has_saved_camera[card_mode]=true;
        card_mode=value!=0; camera=has_saved_camera[card_mode]?saved_camera[card_mode]:NativePortraitCamera{};
        dragging=false; apply_camera(); nativeHub_ChangePage(hub.page); return true;
    }
    if (editing && (action==HubAction::Back || action==HubAction::Close)) { close_editor(); return true; }
    return false;
}

void nativeHubPortrait_Save()
{
    const char* label=nativeHubPortrait_TileName();
    if (!active() || waiting || !count || !*label || std::strspn(label," \t\r\n")==std::strlen(label)) return;
    char root[512]; const char* folder = nativeHubCatalog_Root(root);
    if (!folder) {
        fail("Could not open your user-data folder.");
        return;
    }
    if (editing) std::snprintf(destination,sizeof(destination),"%s/%s.png",draft,card_mode?"player-card":"portrait");
    else std::snprintf(destination, sizeof(destination), "%s/startup-portrait.png", folder);
    apply_camera();
    nativeHub_ChangePage(hub.page);
    edit_error[0]=0;
    const auto picture = photo();
    waiting = nativeMenuPortraitCapture_Request(subjects, count, destination, card_mode?136:464,
        card_mode?188:unsigned(std::lround(464*picture.height/picture.width)));
    if (!waiting) fail("Could not capture this portrait. Try again.");
}

void nativeHubPortrait_Tick()
{
    if (pending && nativeHome_IsSelection() && nativeCssOriginal_CanEdit()) {
        pending=false; waiting=false;
        if (!load_project()) {
            nativeMenuFighter_UnloadAll(); nativeHub_Open(NativeHubPage_Workshop);
            nativeHub_Notice("Could not load this fighter's portrait preview."); return;
        }
        editing=true; nativeHub_Open(NativeHubPage_Portrait); nativeMenuScenery_Load("tournament-background");
    }
    if (!active() || !waiting) return;
    const auto state = nativeMenuPortraitCapture_State();
    if (state == NativePortraitCaptureState::Saved) {
        waiting = false;
        if (editing) {
            bool ok;
            if (card_mode) {
                size_t size=0; void* png=SDL_LoadFile(destination,&size);
                ok=png && nativeFighterProject_SetResource(draft,"ui/player-card.png",NativeSourceResource_PNG,png,size);
                SDL_free(png);
            } else ok=nativeFighterProject_SetTileName(draft,edit_name);
            if (!ok || !nativeHubPortrait_StoreCamera(draft) || !nativeHubCatalog_PublishDraft(draft,NativeHubPublish::Install))
                fail("Could not install the portrait. Your image is saved in the draft.");
            else close_editor();
        } else nativeHubStartup_PortraitSaved(destination);
    } else if (state == NativePortraitCaptureState::Failed) {
        waiting = false;
        fail("Could not save this portrait. Check free space and try again.");
    }
}

bool nativeHubPortrait_Controller(const HSD_PadStatus& pad)
{
    if (!active()) return false;
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
    if (!active()) { dragging = false; return false; }
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
    nativeMenuPortraitCapture_Preview(subjects, count, picture.x, picture.y, picture.width, picture.height);
    Mtx canvas; Mtx44 projection; nativeMenuScene_View(false, false, canvas, projection);
    nativeMenuPaint_Begin(canvas);
    NativeMenuTilePortrait small;
    if (card_mode) small={420,286,43.4f,60};
    else {
        nativeMenuTile_Draw(nullptr,0,0,title(),411,291,small_size,small_size*56/64);
        nativeMenuTile_PortraitRect(title(),411,291,small_size,small_size*56/64,&small);
    }
    nativeMenuPortraitCapture_Preview(subjects,count,small.x,small.y,small.width,small.height);
    nativeMenuPaint_Begin(canvas);
    if (editing) {
        nativeHub_Button("portrait-mode-tile","CSS tile",365,141,108,25,HubAction::PortraitMode,0,!card_mode);
        nativeHub_Button("portrait-mode-card","Player card",481,141,114,25,HubAction::PortraitMode,1,card_mode);
    } else nativeMenuPaint_Text(hub.title,362,141,238,22,gold,NativeMenuFont_MenuCaption);
    if (!card_mode) {
        nativeMenuPaint_Text("Short tile name",365,172,230,11,muted);
        nativeHub_Field("setup-tile-name",nativeHubPortrait_TileName(),365,190,230,30,HubAction::PortraitTileName);
    } else nativeMenuPaint_Text("Shown on the player's panel.",365,183,230,12,muted);
    nativeMenuPaint_Text("C-stick: orbit    X / Y: zoom",365,234,230,10,muted);
    nativeMenuPaint_Text("A + stick on portrait / D-pad: pan",365,253,230,10,muted);
    nativeMenuPaint_Text(card_mode?"Card size":"CSS size",486,307,105,10,muted);
    nativeMenuPaint_Text("Middle-drag: orbit    Wheel: zoom",72,383,264,10,muted);
    nativeMenuPaint_Text("Shift + middle-drag: pan",72,402,264,10,muted);
    nativeHub_Button("setup-portrait-reset", "Reset view", 363, 368, 150, 26,
        HubAction::PortraitReset, 0, false, !waiting);
    nativeHub_Button("setup-portrait-save", waiting ? "Saving..." : "Use portrait", 386, 418, 216, 37,
        HubAction::PortraitSave, 0, true, count && !waiting);
    if (editing) nativeHub_Button("portrait-back","< Back",40,427,110,28,HubAction::Back);
    if (editing && edit_error[0]) nativeMenuPaint_Text(edit_error,40,466,555,9,0xFFAE92FF);
}

void nativeHubPortrait_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] startup-camera active=%d subjects=%u waiting=%d yaw=%.6f pitch=%.6f zoom=%.6f pan=%.6f,%.6f\n",
        active(), count, waiting, camera.yaw, camera.pitch, camera.zoom, camera.pan_x, camera.pan_y);
}
bool nativeHubPortrait_StoreCamera(const char* path)
{
    char text[192]; int size=std::snprintf(text,sizeof(text),"%.9g %.9g %.9g %.9g %.9g %.9g\n",
        camera.yaw,camera.pitch,camera.zoom,camera.pan_x,camera.pan_y,camera.frame);
    return size>0 && size<sizeof(text) && nativeFighterProject_SetResource(path,
        card_mode?"ui/portrait-card.camera":"ui/portrait-tile.camera",NativeSourceResource_Blob,text,size);
}
