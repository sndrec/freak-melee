#include "native_hub_settings.h"
#include "native_hub_controls.h"
#include <melee/gm/native_home.h>
#include <melee/gm/native_presentation.h>
#include <melee/mn/native_menu_scene.h>
#include <aurora/gfx.h>
#include <SDL3/SDL.h>
extern "C" {
#include <melee/gm/gmmain_lib.h>
#include <melee/gm/gm_1601.h>
#include <melee/lb/lbaudio_ax.h>
}
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstdio>

namespace {
    SDL_Window* window;
    char path[560];
    NativeHubPage return_page = NativeHubPage_Workshop;
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    bool full_screen() { return window && (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN); }
    void save()
    {
        char temporary[568]; std::snprintf(temporary,sizeof(temporary),"%s.new",path);
        FILE* file = *path ? std::fopen(temporary,"wb") : nullptr;
        bool ok = file && std::fprintf(file,"fullscreen %u\nvsync %u\nmono %u\nbalance %d\n",
            full_screen(),aurora_vsync_enabled(),lbAudioAx_80024BD0(),int(int8_t(gmMainLib_8015ED74()))) > 0;
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
    unsigned full,vsync,mono; int balance;
    bool ok = std::fscanf(file,"fullscreen %u\nvsync %u\nmono %u\nbalance %d",&full,&vsync,&mono,&balance) == 4 &&
        full <= 1 && vsync <= 1 && mono <= 1 && balance >= -100 && balance <= 100;
    std::fclose(file); if (!ok) return;
    if (window) SDL_SetWindowFullscreen(window,full != 0);
    aurora_enable_vsync(vsync != 0); lbAudioAx_80024C08(mono);
    gmMainLib_8015ED80(int8_t(balance)); gm_801602C0(int8_t(balance));
}
bool nativeHubSettings_Page(NativeHubPage page)
{
    return page == NativeHubPage_Settings || page == NativeHubPage_Controls ||
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
    case HubAction::AudioBalance: {
        int balance = std::clamp(int(int8_t(gmMainLib_8015ED74())) + value,-100,100);
        gmMainLib_8015ED80(int8_t(balance)); gm_801602C0(int8_t(balance)); save(); return true;
    }
    default: return false;
    }
}
void nativeHubSettings_Render(NativeHubPage page)
{
    if (page == NativeHubPage_Settings) {
        Mtx canvas; Mtx44 projection; nativeMenuScene_View(false,false,canvas,projection);
        const char* labels[] = {"Video","Audio","Game","Online","Controls"};
        const char* help[] = {"Display mode and VSync.","Sound channels and the balance of music and effects.","Match rules, fighter appearances and first startup.","Your name, profile and online account.","Local players, controller profiles, bindings and calibration."};
        const char* description = "";
        for (unsigned i = 0; i < 5; ++i) {
            float y = 115 + i * 51;
            bool hover = nativeHub_Hover(45,y,310,43);
            nativeMenuScene_DrawRibbon(i,canvas,45,y,310,43,hover,labels[i]);
            nativeMenuPaint_Begin(canvas);
            char id[32]; std::snprintf(id,sizeof(id),"settings-category-%u",i);
            nativeHub_Region(id,45,y,310,43,HubAction::SettingsCategory,int(i));
            if (hover) description = help[i];
        }
        nativeHub_TextBlock(description,390,184,195,150,14,muted);
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
        nativeMenuPaint_Text("Channels",65,150,290,18,gold,NativeMenuFont_MenuOption);
        nativeHub_Button("audio-channels",lbAudioAx_80024BD0()?"Mono":"Stereo",373,140,210,36,HubAction::AudioChannels);
        nativeMenuPaint_Text("Effects / music",65,223,290,18,gold,NativeMenuFont_MenuOption);
        int balance = int(int8_t(gmMainLib_8015ED74()));
        nativeHub_Button("audio-effects","<",373,213,47,36,HubAction::AudioBalance,-5,false,balance > -100);
        char label[24]; std::snprintf(label,sizeof(label),"%+d",balance);
        nativeMenuPaint_Text(label,432,225,86,16,gold,NativeMenuFont_Notice,true);
        nativeHub_Button("audio-music",">",536,213,47,36,HubAction::AudioBalance,5,false,balance < 100);
        nativeMenuPaint_Text("Effects",373,265,100,11,muted);
        nativeMenuPaint_Text("Music",503,265,80,11,muted,NativeMenuFont_Notice,true);
    } else if (page == NativeHubPage_Game) {
        nativeHub_Button("settings-rules","Match rules",65,128,518,36,HubAction::Rules,0,false,nativeHome_IsSelection());
        nativeHub_Button("settings-appearances",nativePresentation_CustomModels()?"Custom fighter models: Show":"Custom fighter models: Neutral",65,178,518,36,HubAction::ToggleCustomModels);
        nativeHub_Button("settings-costumes","Costumes",65,228,250,33,HubAction::Costumes,0,false,nativeHome_IsSelection());
        nativeHub_Button("settings-packages","Installed packages",333,228,250,33,HubAction::Packages,0,false,nativeHome_IsSelection());
        nativeHub_Button("settings-startup","First startup",65,286,518,36,HubAction::Startup,0,false,nativeHome_IsSelection());
    } else if (page == NativeHubPage_OnlineSettings) {
        nativeHub_Button("settings-account","Online account",65,128,518,36,HubAction::Account);
        nativeMenuPaint_Text("Display name",65,191,250,14,gold);
        nativeHub_Field("display-name",hub.name,65,214,518,32,HubAction::FieldName);
        nativeHub_Button("edit-profile","Your profile",65,278,250,36,HubAction::Profile);
        nativeHub_Button("save-profile","Save name",333,278,250,36,HubAction::SaveProfile,0,true,*hub.name);
    }
}
void nativeHubSettings_Inspect()
{
    std::fprintf(stderr,"[hub-inspect] client-options fullscreen=%d vsync=%d mono=%d balance=%d page=%u\n",
        full_screen(),aurora_vsync_enabled(),lbAudioAx_80024BD0(),int(int8_t(gmMainLib_8015ED74())),unsigned(hub.page));
}
