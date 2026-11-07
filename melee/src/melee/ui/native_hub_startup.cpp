#define _CRT_RAND_S
#include "native_hub_startup.h"
#include "native_hub_account.h"
#include <melee/gm/native_profile_input.h>
#include <melee/gm/native_online_session.h>
#include <melee/gm/native_home.h>
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
    void restore_controls()
    {
        auto& s = nativeHubStartup;
        if (!s.applied) return;
        if (s.previous_assigned) nativeProfileInput_Assign(s.port, &s.previous_profile);
        else nativeProfileInput_Clear(s.port);
        s.applied = false;
    }
    void enter(NativeStartupStep step, bool backwards = false)
    {
        auto& s = nativeHubStartup;
        s.step = step; s.started_at = SDL_GetTicks(); s.error[0] = 0;
        if (step == NativeStartup_Controller) s.controller_claimed = false;
        if (step == NativeStartup_Clone && !*hub.title) {
            hub.template_kind = Ft_Kind_Mario;
            std::snprintf(hub.title, sizeof(hub.title), "Mario");
        }
        nativeHub_ChangePage(step == NativeStartup_Account ? NativeHubPage_Account :
            step == NativeStartup_Clone ? NativeHubPage_Clone : NativeHubPage_Startup, backwards);
        if (!save_progress()) std::snprintf(s.error, sizeof(s.error), "Could not save setup progress. Check the user-data folder.");
    }
    bool connected()
    {
        return PADGetIndexForPort(nativeHubStartup.port) >= 0;
    }
    void start_rest()
    {
        auto& s = nativeHubStartup;
        restore_controls(); s.calibrated = false;
        nativeControllerCalibration_Reset(&s.capture);
        s.controller_index = PADGetIndexForPort(s.port);
        enter(NativeStartup_Rest);
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
            enter(step >= NativeStartup_Controller && step <= NativeStartup_Test ? NativeStartup_Controller : NativeStartupStep(step));
        }
        return;
    }
    if (!has_profile && !nativeHubFighterCount) nativeHubStartup_Begin();
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
    enter(NativeStartup_Welcome);
}
void nativeHubStartup_Tick()
{
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
    for (unsigned axis = 0; axis < 6; ++axis) s.raw[axis] = nativeProfileInput_RawAxis(s.port, &s.profile, axis);
    if (device < 0) return;
    if (s.step == NativeStartup_Rest) {
        // Restart the still interval if any axis moves: the last second must
        // describe a stable released controller, not an average of motion.
        bool moving = false;
        for (unsigned axis = 0; axis < 6 && s.capture.samples; ++axis)
            moving |= s.raw[axis] < int(s.capture.rest_low[axis]) - 1500 ||
                      s.raw[axis] > int(s.capture.rest_high[axis]) + 1500;
        if (moving) { nativeControllerCalibration_Reset(&s.capture); s.started_at = SDL_GetTicks(); }
        nativeControllerCalibration_Sample(&s.capture, s.raw, true);
    } else if (s.step == NativeStartup_Sticks || s.step == NativeStartup_Triggers)
        nativeControllerCalibration_Sample(&s.capture, s.raw, false);
}
bool nativeHubStartup_Activate(HubAction action, int value)
{
    auto& s = nativeHubStartup;
    if (action == HubAction::Startup) { nativeHubStartup_Begin(); return true; }
    if (!s.active) return false;
    if (action == HubAction::Back || action == HubAction::StartupBack) {
        if (s.step == NativeStartup_Test) restore_controls();
        if (s.step > NativeStartup_Welcome) enter(NativeStartupStep(unsigned(s.step) - 1), true);
        return true;
    }
    if (action == HubAction::StartupPort) {
        if (value < 0 || value > 3) return true;
        restore_controls(); s.port = unsigned(value); s.calibrated = false;
        s.controller_claimed = true;
        const NativeProfile* assigned = nativeProfileInput_Assigned(s.port);
        if (assigned) s.profile = *assigned;
        s.controller_index = PADGetIndexForPort(s.port);
        return true;
    }
    if (action == HubAction::StartupRetry) { start_rest(); return true; }
    if (action == HubAction::StartupSkipController) {
        restore_controls(); s.calibrated = false;
        enter(NativeStartup_Clone); return true;
    }
    if (action != HubAction::StartupNext) {
        // Reuse the real account and clone pages, including their text fields.
        if (s.step == NativeStartup_Account || s.step == NativeStartup_Clone ||
            action == HubAction::FieldName || action == HubAction::FieldBio) return false;
        return true;
    }
    switch (s.step) {
    case NativeStartup_Welcome: enter(NativeStartup_Profile); break;
    case NativeStartup_Profile:
        if (nativeHub_SaveProfile()) enter(NativeStartup_Account);
        else std::snprintf(s.error, sizeof(s.error), "Enter a display name and try saving again.");
        break;
    case NativeStartup_Account:
        if (!nativeHubAccount_View().busy && !nativeHubAccount_View().recovery_pending) {
            if (nativeHub_SaveProfile()) enter(NativeStartup_Controller);
            else std::snprintf(s.error, sizeof(s.error), "Could not save your profile. Try again.");
        }
        break;
    case NativeStartup_Controller:
        if (connected()) start_rest();
        break;
    case NativeStartup_Rest:
        if (connected() && s.capture.samples >= 60 && SDL_GetTicks() - s.started_at >= 1000) enter(NativeStartup_Sticks);
        break;
    case NativeStartup_Sticks:
        if (connected() && stick_complete()) enter(NativeStartup_Triggers);
        break;
    case NativeStartup_Triggers:
        if (!connected()) break;
        if (s.raw[4] > s.capture.rest_high[4] + 1000 || s.raw[5] > s.capture.rest_high[5] + 1000) {
            std::snprintf(s.error, sizeof(s.error), "Release both triggers before continuing."); break;
        }
        if (!nativeControllerCalibration_Finish(&s.capture, &s.profile.calibration)) {
            std::snprintf(s.error, sizeof(s.error), "Press both triggers fully. If a stick was moving at rest, start over."); break;
        }
        s.previous_assigned = nativeProfileInput_Assigned(s.port) != nullptr;
        if (s.previous_assigned) s.previous_profile = *nativeProfileInput_Assigned(s.port);
        if (nativeProfileInput_Assign(s.port, &s.profile)) { s.applied = true; enter(NativeStartup_Test); }
        break;
    case NativeStartup_Test:
        std::snprintf(s.profile.name, sizeof(s.profile.name), "%s", hub.name);
        if (!nativeProfileStore_Save(&s.profile, s.profile_id) || !nativeProfileInput_Assign(s.port, &s.profile)) {
            std::snprintf(s.error, sizeof(s.error), "Could not save this calibration. Your measurements are still here."); break;
        }
        s.applied = false; s.calibrated = true;
        hub.template_kind = Ft_Kind_Mario;
        std::snprintf(hub.title, sizeof(hub.title), "Mario");
        enter(NativeStartup_Clone);
        break;
    case NativeStartup_Clone:
        if (nativeHubStartup_HasFighter()) enter(NativeStartup_Done);
        break;
    case NativeStartup_Done:
        s.completed = true;
        if (!save_progress()) { s.completed = false; std::snprintf(s.error, sizeof(s.error), "Could not save setup completion. Try again."); break; }
        s.active = false; s.completed = true;
        hub.dismissed = true; hub.visible = false;
        break;
    }
    return true;
}
void nativeHubStartup_Cloned()
{
    if (nativeHubStartup.active && nativeHubStartup.step == NativeStartup_Clone) enter(NativeStartup_Done);
}
bool nativeHubStartup_HasFighter()
{
    for (unsigned i = 0; i < nativeHubFighterCount; ++i)
        if (nativeHubFighters[i].installed) return true;
    return false;
}
void nativeHubStartup_Inspect()
{
    const auto& s = nativeHubStartup;
    std::fprintf(stderr, "[hub-inspect] startup active=%d step=%u port=%u connected=%d samples=%u measured=%d completed=%d error=%s\n",
        s.active, unsigned(s.step), s.port, connected(), s.capture.samples, s.profile.calibration.measured, s.completed, s.error);
}
