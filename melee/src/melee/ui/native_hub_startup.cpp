#define _CRT_RAND_S
#include "native_hub_startup.h"
#include "native_hub_account.h"
#include "native_hub_startup_roster.h"
#include "native_hub_portrait.h"
#include "native_hub_startup_stamp.h"
#include "native_hub_startup_triggers.h"
#include <melee/ft/native_fighter_presentation.h>
#include <melee/mn/native_css_original.h>
#include <melee/gm/native_profile_input.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_home.h>
#include <melee/mn/native_menu_scenery.h>
extern "C" {
#include <melee/lb/lbaudio_ax.h>
#include <melee/gm/gmmain_lib.h>
#include <dolphin/dvd.h>
}
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

NativeHubStartupState nativeHubStartup;
namespace {
    bool path(char result[560])
    {
        char root[512];
        const char* directory = nativeHubCatalog_Root(root);
        return directory && std::snprintf(result, 560, "%s/startup.txt", directory) < 560;
    }
    bool save_progress()
    {
        char file[560], temporary[568], root[512];
        const char* directory = nativeHubCatalog_Root(root);
        if (!directory || !SDL_CreateDirectory(directory)) return false;
        if (!path(file)) return false;
        std::snprintf(temporary, sizeof(temporary), "%s.tmp", file);
        FILE* output = std::fopen(temporary, "wb");
        if (!output) return false;
        bool ok = std::fprintf(output, "%u %u %u\n%s\n", unsigned(nativeHubStartup.step), nativeHubStartup.port,
            nativeHubStartup.completed,
            nativeHubStartup.profile_id) > 0;
        ok = std::fclose(output) == 0 && ok;
        return ok && MoveFileExA(temporary, file, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    void save_stick_trace()
    {
        auto& s=nativeHubStartup;
        char root[512],file[560]; const char* folder=nativeHubCatalog_Root(root);
        if (!folder) return;
        std::snprintf(file,sizeof(file),"%s/stick-capture.csv",folder);
        FILE* output=std::fopen(file,"wb"); if (!output) return;
        std::fprintf(output,"# rest_center,%d,%d,%d,%d\n",
            (int(s.capture.rest_low[0])+s.capture.rest_high[0])/2,(int(s.capture.rest_low[1])+s.capture.rest_high[1])/2,
            (int(s.capture.rest_low[2])+s.capture.rest_high[2])/2,(int(s.capture.rest_low[3])+s.capture.rest_high[3])/2);
        std::fprintf(output,"sample,main_x,main_y,c_x,c_y\n");
        unsigned first=s.stick_trace_count>2048?s.stick_trace_count-2048:0;
        for (unsigned i=first;i<s.stick_trace_count;++i) {
            const auto* v=s.stick_trace[i%2048];
            std::fprintf(output,"%u,%d,%d,%d,%d\n",i,v[0],v[1],v[2],v[3]);
        }
        std::fclose(output);
    }
    void restore_controls()
    {
        auto& s = nativeHubStartup;
        if (!s.applied) return;
        if (s.previous_assigned) nativeProfileInput_Assign(s.port, &s.previous_profile);
        else nativeProfileInput_Clear(s.port);
        s.applied = false;
    }
    bool apply_controls()
    {
        auto& s=nativeHubStartup;
        if (!s.applied) {
            const auto* previous=nativeProfileInput_Assigned(s.port);
            s.previous_assigned=previous!=nullptr;
            if (previous) s.previous_profile=*previous;
        }
        if (!nativeProfileInput_Assign(s.port,&s.profile)) return false;
        s.applied=true; return true;
    }
    void enter(NativeStartupStep step, bool backwards = false)
    {
        auto& s = nativeHubStartup;
        if (s.step==NativeStartup_Sticks && step!=NativeStartup_Sticks) save_stick_trace();
        if (s.step == NativeStartup_Placement && step != NativeStartup_Placement) nativeHubStartupStamp_CancelPlacement();
        if (step == NativeStartup_Clone || step == NativeStartup_Portrait || step == NativeStartup_Done)
            nativeHubStartupStamp_Unload();
        if (step == NativeStartup_Portrait && s.step > NativeStartup_Portrait) nativeHubStartupRoster_Begin();
        if (s.step >= NativeStartup_Clone && s.step <= NativeStartup_Portrait &&
            (step < NativeStartup_Clone || step > NativeStartup_Portrait)) nativeHubStartupRoster_Unload();
        s.step = step; s.started_at = SDL_GetTicks(); s.error[0] = 0; s.testing_input=false;
        if (step == NativeStartup_Account) nativeHubAccount_BeginStartup();
        if (step == NativeStartup_Controller) s.controller_claimed = false;
        if (step == NativeStartup_Sticks) s.stick_trace_count=0;
        if (step == NativeStartup_Clone && !*hub.title) {
            hub.template_kind = Ft_Kind_Mario;
            std::snprintf(hub.title, sizeof(hub.title), "Mario");
        }
        nativeHub_ChangePage(step == NativeStartup_Account ? NativeHubPage_Account :
            step == NativeStartup_Clone ? NativeHubPage_Clone : NativeHubPage_Startup, backwards);
        if (step == NativeStartup_Clone) nativeHubStartupRoster_Begin();
        if (step == NativeStartup_Portrait) nativeHubPortrait_Begin();
        if (!save_progress()) std::snprintf(s.error, sizeof(s.error), "Could not save setup progress. Check the user-data folder.");
    }
    bool connected()
    {
        return PADGetIndexForPort(nativeHubStartup.port) >= 0;
    }
    void trigger_clicks()
    {
        auto& s = nativeHubStartup;
        s.trigger_click[0] = s.trigger_click[1] = -1;
        for (unsigned trigger = 0; trigger < 2; ++trigger) {
            const auto& axis = s.profile.axes[4 + trigger];
            // A trigger remapped to a different analog source has no associated click.
            if (axis.assigned && (axis.native_axis != int(4 + trigger) || axis.direction != 1)) continue;
            s.trigger_click[trigger] = int8_t(PADGetMappedButtonState(s.port, trigger ? PAD_TRIGGER_R : PAD_TRIGGER_L));
        }
    }
    void start_rest(bool backwards = false)
    {
        auto& s = nativeHubStartup;
        restore_controls(); s.calibrated = false;
        // Continue can confirm the default port without clicking its row.
        std::snprintf(s.profile.name, sizeof(s.profile.name), "%s", hub.name);
        s.controller_index = PADGetIndexForPort(s.port);
        enter(NativeStartup_Rest, backwards);
        nativeControllerCalibration_Reset(&s.capture);
    }
    bool stick_complete()
    {
        const auto& c = nativeHubStartup.capture;
        for (unsigned axis = 0; axis < 4; ++axis) {
            int center = (int(c.rest_low[axis]) + c.rest_high[axis]) / 2;
            if (int(c.high[axis]) - center < 8192 || center - int(c.low[axis]) < 8192) return false;
        }
        return true;
    }
}
void nativeHubStartup_Initialize(bool has_profile)
{
    auto& s = nativeHubStartup;
    char file[560];
    if (!path(file)) return;
    FILE* input = std::fopen(file, "rb");
    if (input) {
        unsigned step = 0, port = 0, completed = 0; char id[66]{};
        bool valid = std::fscanf(input, "%u %u %u\n%65[^\n]", &step, &port, &completed, id) >= 3 &&
                     step <= NativeStartup_Done && port < 4 && completed <= 1;
        std::fclose(input);
        if (!valid) return; // Keep an unreadable record for manual recovery.
        s.completed = completed != 0;
        s.port = port;
        nativeProfile_Default(&s.profile, hub.name);
        if (*id && nativeProfileStore_IsValidId(id)) {
            std::snprintf(s.profile_id, sizeof(s.profile_id), "%s", id);
            if (nativeProfileStore_Load(&s.profile, id)) {
                nativeProfileInput_Assign(port, &s.profile);
                s.calibrated = s.profile.calibration.measured;
            }
        }
        if (!s.completed) {
            nativeHubStartup_Begin();
            // Measurements are intentionally repeated after an interrupted capture.
            enter(step >= NativeStartup_Controller && step <= NativeStartup_Test ? NativeStartup_Controller :
                  step > NativeStartup_Clone && step < NativeStartup_Done ? NativeStartup_Clone : NativeStartupStep(step));
        }
        return;
    }
    if (!has_profile && !nativeHubFighterCount) {
        hub.name[0] = hub.bio[0] = 0;
        nativeHubStartup_Begin();
    }
}
void nativeHubStartup_Begin()
{
    if (!nativeHome_IsSelection() || nativeOnlineSession_State() != NativeLobbySessionState_Disconnected) {
        nativeHub_Notice("Leave your lobby and return to character select to run setup."); return;
    }
    auto& s = nativeHubStartup;
    restore_controls();
    s.active = true; s.completed = false; s.controller_index = -1;
    if (!*s.profile_id) {
        unsigned random[2];
        if (rand_s(&random[0]) || rand_s(&random[1])) { s.active = false; nativeHub_Notice("Could not create a controller profile."); return; }
        std::snprintf(s.profile_id, sizeof(s.profile_id), "setup-%08x%08x", random[0], random[1]);
        nativeProfile_Default(&s.profile, hub.name);
    }
    hub.visible = true; hub.dismissed = false;
    nativeMenuScenery_Load("tournament-background");
    const char* music = DVDConvertPathToEntrynum("/audio/first_startup.hps") >= 0
        ? "/audio/first_startup.hps" : "/audio/menu01.hps";
    lbAudioAx_PlayMusicFile(music);
    enter(NativeStartup_Welcome);
}
void nativeHubStartup_Tick()
{
    nativeHubPortrait_Tick();
    auto& s = nativeHubStartup;
    if (!s.active || s.step < NativeStartup_Controller || s.step > NativeStartup_Test) return;
    int device = PADGetIndexForPort(s.port);
    if (device != s.controller_index) {
        if (s.step != NativeStartup_Controller) {
            restore_controls(); s.calibrated = false;
            enter(NativeStartup_Controller);
            std::snprintf(s.error, sizeof(s.error), "Controller changed. Choose it and calibrate again.");
        }
        s.controller_index = device;
    }
    nativeProfileInput_RawAxes(s.port,&s.profile,s.raw);
    if (device < 0) return;
    trigger_clicks();
    if (s.step == NativeStartup_Rest) {
        // Restart the still interval if any axis moves: the last second must
        // describe a stable released controller, not an average of motion.
        bool moving = false;
        for (unsigned axis = 0; axis < 6 && s.capture.samples; ++axis)
            moving |= s.raw[axis] < int(s.capture.rest_low[axis]) - 1500 ||
                      s.raw[axis] > int(s.capture.rest_high[axis]) + 1500;
        if (moving) { nativeControllerCalibration_Reset(&s.capture); s.started_at = SDL_GetTicks(); }
        nativeControllerCalibration_Sample(&s.capture, s.raw, true, nullptr);
    } else if (s.step == NativeStartup_Sticks || s.step == NativeStartup_Triggers) {
        nativeControllerCalibration_Sample(&s.capture, s.raw, false,
            s.step == NativeStartup_Triggers ? s.trigger_click : nullptr);
        if (s.step==NativeStartup_Sticks) {
            std::memcpy(s.stick_trace[s.stick_trace_count%2048],s.raw,4*sizeof(int16_t));
            ++s.stick_trace_count;
        }
    }
}
bool nativeHubStartup_Activate(HubAction action, int value)
{
    auto& s = nativeHubStartup;
    if (action == HubAction::Startup) { nativeHubStartup_Begin(); return true; }
    if (!s.active) return false;
    if (s.step == NativeStartup_Clone && action == HubAction::StartupConfirmFighter) {
        s.draft[0] = 0;
        std::snprintf(s.tile_name, sizeof(s.tile_name), "%s", hub.title);
        enter(NativeStartup_ConfirmFighter); return true;
    }
    if (s.step == NativeStartup_Portrait && action == HubAction::PortraitTileName) return false;
    if (s.step == NativeStartup_Placement && action == HubAction::StartupPlaceTile) {
        if (!nativeHubStartupStamp_Place()) std::snprintf(s.error, sizeof(s.error), "Could not save this position. Try again.");
        else nativeHubStartup_Finish();
        return true;
    }
    if (s.step == NativeStartup_Clone && action == HubAction::StartupFighterMove) {
        nativeHubStartupRoster_Move(value); return true;
    }
    if (s.step == NativeStartup_Clone && action == HubAction::StockTemplate) {
        nativeHubStartupRoster_Select(value); return true;
    }
    if (action == HubAction::Back || action == HubAction::StartupBack) {
        if (s.step == NativeStartup_Account && nativeHubAccount_BackStartup()) return true;
        if (s.step == NativeStartup_Profile) { enter(NativeStartup_Account,true); return true; }
        if (s.step == NativeStartup_Controller && nativeHubAccount_View().signed_in) { enter(NativeStartup_Account,true); return true; }
        if (s.step == NativeStartup_Sticks) { start_rest(true); return true; }
        if (s.step == NativeStartup_Clone) { enter(NativeStartup_Controller, true); return true; }
        if (s.step == NativeStartup_ConfirmFighter) { enter(NativeStartup_Clone, true); return true; }
        if (s.step == NativeStartup_Portrait) { enter(NativeStartup_ConfirmFighter, true); return true; }
        if (s.step == NativeStartup_Placement) { enter(NativeStartup_Portrait, true); return true; }
        if (s.step > NativeStartup_Welcome) enter(NativeStartupStep(unsigned(s.step) - 1), true);
        return true;
    }
    if (action == HubAction::StartupPort) {
        if (value < 0 || value > 3) return true;
        restore_controls(); s.port = unsigned(value); s.calibrated = false;
        s.controller_claimed = true;
        const NativeProfile* assigned = nativeProfileInput_Assigned(s.port);
        if (assigned) s.profile = *assigned;
        else nativeProfile_Default(&s.profile, hub.name);
        s.controller_index = PADGetIndexForPort(s.port);
        return true;
    }
    if (action == HubAction::StartupRetry) { start_rest(); return true; }
    if (action == HubAction::StartupTest) { if (s.step==NativeStartup_Test) s.testing_input=!s.testing_input; return true; }
    if (action == HubAction::StartupTriggerRange) {
        nativeHubStartupTriggers_Begin(value);
        return true;
    }
    if (action == HubAction::StartupSkipController) {
        restore_controls(); s.calibrated = false;
        enter(NativeStartup_Clone); return true;
    }
    if (action != HubAction::StartupNext) {
        // Account fields and the profile fields use the hub's text editor.
        if (s.step == NativeStartup_Account ||
            action == HubAction::FieldName || action == HubAction::FieldBio) return false;
        return true;
    }
    switch (s.step) {
    case NativeStartup_Welcome: enter(NativeStartup_Account); break;
    case NativeStartup_Profile:
        if (nativeHub_SaveProfile()) enter(NativeStartup_Controller);
        else std::snprintf(s.error, sizeof(s.error), "Enter a display name and try saving again.");
        break;
    case NativeStartup_Account:
        if (!nativeHubAccount_View().busy && !nativeHubAccount_View().recovery_pending) {
            if (!nativeHubAccount_View().signed_in) enter(NativeStartup_Profile);
            else {
                const auto& account=nativeHubAccount_View();
                std::snprintf(hub.name,sizeof(hub.name),"%s",account.name);
                std::snprintf(hub.bio,sizeof(hub.bio),"%s",account.bio);
                std::snprintf(hub.public_name,sizeof(hub.public_name),"%s",account.name);
                if (nativeHub_SaveProfile()) enter(NativeStartup_Controller);
                else std::snprintf(s.error, sizeof(s.error), "Could not save your profile. Try again.");
            }
        }
        break;
    case NativeStartup_Controller:
        if (connected()) start_rest();
        break;
    case NativeStartup_Rest:
        if (connected() && s.capture.samples >= 60 && SDL_GetTicks() - s.started_at >= 1000) enter(NativeStartup_Sticks);
        break;
    case NativeStartup_Sticks:
        if (connected() && stick_complete()) {
            NativeControllerCalibration sticks = s.capture;
            for (unsigned i = 0; i < 2; ++i)
                nativeControllerCalibration_SetTriggerRange(&sticks, i, 0, 32767);
            if (!nativeControllerCalibration_Finish(&sticks, &s.profile.calibration)) {
                std::snprintf(s.error, sizeof(s.error), "Go back and keep the controller still while capturing its resting position.");
            } else if (!apply_controls()) {
                std::snprintf(s.error, sizeof(s.error), "Could not apply this controller profile.");
            } else {
                enter(NativeStartup_Triggers);
            }
        }
        break;
    case NativeStartup_Triggers:
        if (!connected()) break;
        if (!nativeControllerCalibration_Finish(&s.capture, &s.profile.calibration)) {
            std::snprintf(s.error, sizeof(s.error), "Squeeze both triggers, or set their Start and End markers."); break;
        }
        if (nativeControllerCalibration_Normalize(&s.profile.calibration,4,s.raw[4]) > 0 ||
            nativeControllerCalibration_Normalize(&s.profile.calibration,5,s.raw[5]) > 0 ||
            s.trigger_click[0] > 0 || s.trigger_click[1] > 0) {
            std::snprintf(s.error, sizeof(s.error), "Release the triggers. Move Start past each resting reading to make it 0%%."); break;
        }
        if (apply_controls()) enter(NativeStartup_Test);
        break;
    case NativeStartup_Test:
        if (!nativeProfileStore_Save(&s.profile, s.profile_id) || !nativeProfileInput_Assign(s.port, &s.profile)) {
            std::snprintf(s.error, sizeof(s.error), "Could not save this calibration. Your measurements are still here."); break;
        }
        s.applied = false; s.calibrated = true;
        hub.template_kind = Ft_Kind_Mario;
        std::snprintf(hub.title, sizeof(hub.title), "Mario");
        enter(NativeStartup_Clone);
        break;
    case NativeStartup_Clone:
        if (nativeHubStartup_HasRoster()) enter(NativeStartup_Done);
        break;
    case NativeStartup_ConfirmFighter: enter(NativeStartup_Portrait); break;
    case NativeStartup_Portrait: break;
    case NativeStartup_Placement: break;
    case NativeStartup_Done:
        nativeHubStartup_Finish();
        break;
    }
    return true;
}
void nativeHubStartup_Finish()
{
    auto& s = nativeHubStartup;
    s.completed = true;
    if (!save_progress()) {
        s.completed = false;
        enter(NativeStartup_Done);
        std::snprintf(s.error, sizeof(s.error), "Could not save setup completion. Try again.");
        return;
    }
    nativeHubStartupStamp_Unload(); nativeHubStartupRoster_Unload();
    s.active = false;
    nativeMenuScenery_Unload();
    lbAudioAx_80023F28(gmMainLib_8015ECB0());
    hub.dismissed = true; hub.visible = false;
}
void nativeHubStartup_PortraitSaved(const char* portrait, const NativeFighterPresentation::Framing& framing)
{
    auto& s = nativeHubStartup;
    if (!s.active || s.step != NativeStartup_Portrait) return;
    if (!s.draft[0]) {
        char path[512];
        if (!nativeHubCatalog_CreateStockDraft(static_cast<FighterKind>(hub.template_kind), hub.title, path)) {
            std::snprintf(s.error, sizeof(s.error), "Could not create your fighter. Try again."); return;
        }
        std::snprintf(s.draft, sizeof(s.draft), "%s", path);
    }
    using namespace NativeFighterPresentation;
    std::string revision, error;
    if (save(s.draft, nullptr, Target::Tile, s.tile_name, portrait, &framing, revision, error) != Result::Saved ||
        !nativeHubStartupStamp_Load()) {
        std::snprintf(s.error, sizeof(s.error), "Could not save your portrait. Your draft is saved in the Workshop."); return;
    }
    if (!nativeHubCatalog_PublishDraft(s.draft,NativeHubPublish::Install) || !nativeHubCatalog_Load()) {
        std::snprintf(s.error,sizeof(s.error),"Could not install your fighter. Your draft is saved in the Workshop."); return;
    }
    bool found=false;
    for (unsigned i=0;i<nativeHubFighterCount;++i) if (nativeHubFighters[i].installed && !std::strcmp(s.draft,nativeHubFighters[i].draft)) {
        hub.selected=int(i); s.fighter_id=nativeHubFighters[i].id; found=true;
    }
    if (!found || !nativeHubStartupStamp_BeginPlacement()) {
        std::snprintf(s.error,sizeof(s.error),"Could not open character select for placement. Try again."); return;
    }
    enter(NativeStartup_Placement);
}
bool nativeHubStartup_HasRoster()
{
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    return layout && layout->count;
}
void nativeHubStartup_Inspect()
{
    const auto& s = nativeHubStartup;
    std::fprintf(stderr, "[hub-inspect] startup active=%d step=%u port=%u connected=%d samples=%u measured=%d completed=%d error=%s\n",
        s.active, unsigned(s.step), s.port, connected(), s.capture.samples, s.profile.calibration.measured, s.completed, s.error);
    for (unsigned trigger=0; trigger<2; ++trigger) {
        int16_t start,end; nativeControllerCalibration_TriggerRange(&s.capture,trigger,&start,&end);
        std::fprintf(stderr,"[hub-inspect] trigger=%u raw=%d rest=%d..%d start=%d end=%d adjusted=%d click=%d\n",
            trigger,s.raw[4+trigger],s.capture.rest_low[4+trigger],s.capture.rest_high[4+trigger],
            start,end,s.capture.trigger_adjusted[trigger],s.trigger_click[trigger]);
    }
}
