#include "native_hub_settings.h"
#include "native_hub_controls.h"
#include "native_hub_profiles.h"
#include "native_hub_rules.h"
#include "native_hub_packages.h"
#include "native_hub_account.h"
#include <melee/gm/native_home.h>
#include <melee/gm/native_presentation.h>
#include <melee/mn/native_menu_scene.h>
#include <aurora/gfx.h>
#include <SDL3/SDL.h>
extern "C" {
#include <melee/gm/gmmain_lib.h>
#include <melee/gm/gm_1601.h>
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/audio_output.h>
#include <sysdolphin/baselib/audio_mix.h>
}
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstdio>

namespace {
    SDL_Window* window;
    char path[560];
    unsigned volumes[3] = {100,100,100};
    int dragged_volume = -1;
    bool volume_dirty;
    NativeHubPage return_page = NativeHubPage_Workshop;
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    bool full_screen() { return window && (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN); }
    void save()
    {
        char temporary[568]; std::snprintf(temporary,sizeof(temporary),"%s.new",path);
        FILE* file = *path ? std::fopen(temporary,"wb") : nullptr;
        bool ok = file && std::fprintf(file,"fullscreen %u\nvsync %u\nmono %u\nmaster %u\neffects %u\nmusic %u\n",
            full_screen(),aurora_vsync_enabled(),lbAudioAx_80024BD0(),volumes[0],volumes[1],volumes[2]) > 0;
        if (file) ok = std::fclose(file) == 0 && ok;
        if (!ok || !MoveFileExA(temporary,path,MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DeleteFileA(temporary); nativeHub_Notice("Could not save these settings.");
        }
    }
    void back()
    {
        nativeHub_Button("settings-back","< Back",40,402,110,28,HubAction::Back);
    }
}
void nativeHubSettings_Load()
{
    int count = 0; SDL_Window** windows = SDL_GetWindows(&count);
    window = windows && count ? windows[0] : nullptr; SDL_free(windows);
    char root[512]; const char* directory = nativeHubCatalog_Root(root);
    if (!directory) return;
    std::snprintf(path,sizeof(path),"%s/client-options.txt",directory);
    FILE* file = std::fopen(path,"rb"); if (!file) return;
    unsigned full,vsync,mono,master,effects,music;
    bool ok = std::fscanf(file,"fullscreen %u\nvsync %u\nmono %u\nmaster %u\neffects %u\nmusic %u",&full,&vsync,&mono,&master,&effects,&music) == 6 &&
        full <= 1 && vsync <= 1 && mono <= 1 && master <= 100 && effects <= 100 && music <= 100;
    std::fclose(file); if (!ok) return;
    if (window) SDL_SetWindowFullscreen(window,full != 0);
    aurora_enable_vsync(vsync != 0); lbAudioAx_80024C08(mono);
    volumes[0]=master; volumes[1]=effects; volumes[2]=music;
    HSD_AudioOutputSetMasterVolume(master); HSD_AudioMixSetVolumes(effects,music);
}
bool nativeHubSettings_Page(NativeHubPage page)
{
    return nativeHubRules_Page(page) || nativeHubPackages_Page(page) || page == NativeHubPage_Costumes || nativeHubProfiles_Page(page) || page == NativeHubPage_Settings || page == NativeHubPage_Controls ||
        page == NativeHubPage_Video || page == NativeHubPage_Audio ||
        page == NativeHubPage_Game || page == NativeHubPage_OnlineSettings;
}
bool nativeHubSettings_Activate(HubAction action, int value)
{
    if (action == HubAction::Settings) {
        if (!nativeHubSettings_Page(hub.page)) return_page = hub.page;
        nativeHub_ChangePage(NativeHubPage_Settings); return true;
    }
    if (action == HubAction::SettingsCategory) {
        const NativeHubPage pages[] = {NativeHubPage_Video,NativeHubPage_Audio,NativeHubPage_Game,NativeHubPage_OnlineSettings,NativeHubPage_Controls};
        if (unsigned(value) < 5) nativeHub_ChangePage(pages[value]);
        return true;
    }
    if (action == HubAction::Back && nativeHubSettings_Page(hub.page)) {
        nativeHub_ChangePage(hub.page == NativeHubPage_Settings ? return_page : NativeHubPage_Settings,true); return true;
    }
    if (action == HubAction::Back && (hub.page == NativeHubPage_Account || hub.page == NativeHubPage_Profile) && hub.previous == NativeHubPage_OnlineSettings) {
        nativeHub_ChangePage(NativeHubPage_OnlineSettings,true); return true;
    }
    switch (action) {
    case HubAction::VideoFullscreen:
        if (!window || !SDL_SetWindowFullscreen(window,!full_screen())) nativeHub_Notice("Could not change the display mode.");
        else save();
        return true;
    case HubAction::VideoVsync: aurora_enable_vsync(!aurora_vsync_enabled()); save(); return true;
    case HubAction::AudioChannels: lbAudioAx_80024C08(!lbAudioAx_80024BD0()); save(); return true;
    case HubAction::AudioVolume:
        if (unsigned(value)<3) dragged_volume=value;
        return true;
    default: return false;
    }
}
void nativeHubSettings_Render(NativeHubPage page)
{
    if (page == NativeHubPage_Settings) {
        Mtx canvas; Mtx44 projection; nativeMenuScene_View(false,false,canvas,projection);
        const char* labels[] = {"Video","Audio","Game","Online","Controls"};
        const char* help[] = {"Display mode and VSync.","Master, effects and music volume.","Match rules, fighter appearances and first startup.","Your name, profile and online account.","Local players, controller profiles, bindings and calibration."};
        const char* description = "";
        const float xs[] = {50,78,58,102,75}, widths[] = {301,334,291,321,277}, heights[] = {45,42,47,43,46};
        for (unsigned i = 0; i < 5; ++i) {
            float y = 109 + i * 55;
            bool hover = nativeHub_Hover(xs[i],y,widths[i],heights[i]);
            nativeMenuScene_DrawRibbon(i,canvas,xs[i],y,widths[i],heights[i],hover,labels[i]);
            nativeMenuPaint_Begin(canvas);
            char id[32]; std::snprintf(id,sizeof(id),"settings-category-%u",i);
            nativeHub_Region(id,xs[i],y,widths[i],heights[i],HubAction::SettingsCategory,int(i));
            if (hover) description = help[i];
        }
        nativeHub_TextBlock(description,455,184,135,170,13,muted);
        back(); return;
    }
    if (page == NativeHubPage_Controls) { nativeHubControls_Render(); return; }
    back();
    if (page == NativeHubPage_Video) {
        nativeMenuPaint_Text("Display mode",65,152,290,18,gold,NativeMenuFont_MenuOption);
        nativeHub_Button("video-fullscreen",full_screen()?"Fullscreen":"Windowed",373,142,210,36,HubAction::VideoFullscreen);
        nativeMenuPaint_Text("VSync",65,221,290,18,gold,NativeMenuFont_MenuOption);
        nativeHub_Button("video-vsync",aurora_vsync_enabled()?"On":"Off",373,211,210,36,HubAction::VideoVsync);
        nativeHub_TextBlock("The game keeps its original proportions when you resize the window.",65,288,515,60,12,muted);
    } else if (page == NativeHubPage_Audio) {
        nativeMenuPaint_Text("Channels",65,132,290,18,gold,NativeMenuFont_MenuOption);
        nativeHub_Button("audio-channels",lbAudioAx_80024BD0()?"Mono":"Stereo",373,122,210,31,HubAction::AudioChannels);
        const char* names[] = {"Master","Effects","Music"};
        const char* ids[] = {"audio-master","audio-effects","audio-music"};
        for (unsigned i=0;i<3;++i) {
            float y=193+i*64;
            nativeMenuPaint_Text(names[i],65,y-4,160,17,gold,NativeMenuFont_MenuOption);
            nativeMenuPaint_Round(239,y-4,309,25,4,0x86795EFF);
            nativeMenuPaint_Round(241,y-2,305,21,3,0x09080EFF);
            nativeMenuPaint_Gradient(245,y+3,297*volumes[i]/100.f,11,0xFFE977FF,0xA88019FF);
            float point=245+297*volumes[i]/100.f;
            nativeMenuPaint_Round(point-4,y-6,8,29,2,0xF4E8C4FF);
            char value[12]; std::snprintf(value,sizeof(value),"%u%%",volumes[i]);
            nativeMenuPaint_Text(value,553,y+1,39,13,gold,NativeMenuFont_Notice,true);
            nativeHub_Region(ids[i],235,y-12,315,45,HubAction::AudioVolume,int(i));
        }
    } else if (page == NativeHubPage_Game) {
        nativeHub_Button("settings-rules","Match rules",65,128,518,36,HubAction::Rules,0,false,nativeHome_IsSelection());
        nativeHub_Button("settings-appearances",nativePresentation_CustomModels()?"Custom fighter models: Show":"Custom fighter models: Neutral",65,178,518,36,HubAction::ToggleCustomModels);
        nativeHub_Button("settings-costumes","Costumes",65,228,250,33,HubAction::Costumes,0,false,nativeHome_IsSelection());
        nativeHub_Button("settings-packages","Installed packages",333,228,250,33,HubAction::Packages,0,false,nativeHome_IsSelection());
        nativeHub_Button("settings-startup","First startup",65,286,518,36,HubAction::Startup,0,false,nativeHome_IsSelection());
    } else if (page == NativeHubPage_OnlineSettings) {
        nativeHubAccount_Render();

    }
}
void nativeHubSettings_Input()
{
    if (dragged_volume<0) return;
    if (!hub.visible || hub.page!=NativeHubPage_Audio) {
        dragged_volume=-1; if (volume_dirty) { save(); volume_dirty=false; } return;
    }
    unsigned value=unsigned(std::clamp(int((hub.pointer_x-245)*100/297+.5f),0,100));
    if (volumes[dragged_volume]!=value) {
        volumes[dragged_volume]=value; volume_dirty=true;
        HSD_AudioOutputSetMasterVolume(volumes[0]); HSD_AudioMixSetVolumes(volumes[1],volumes[2]);
    }
    if (!hub.pressed) { dragged_volume=-1; if (volume_dirty) { save(); volume_dirty=false; } }
}
void nativeHubSettings_Inspect()
{
    std::fprintf(stderr,"[hub-inspect] client-options fullscreen=%d vsync=%d mono=%d master=%u effects=%u music=%u page=%u\n",
        full_screen(),aurora_vsync_enabled(),lbAudioAx_80024BD0(),volumes[0],volumes[1],volumes[2],unsigned(hub.page));
}
