#pragma once
#include "native_hub_internal.h"
#include <melee/gm/native_controller_calibration.h>

enum NativeStartupStep {
    NativeStartup_Welcome, NativeStartup_Account, NativeStartup_Profile,
    NativeStartup_Controller, NativeStartup_Rest, NativeStartup_Sticks,
    NativeStartup_Triggers, NativeStartup_Test, NativeStartup_Clone,
    NativeStartup_ConfirmFighter, NativeStartup_Portrait,
    NativeStartup_Placement, NativeStartup_Done
};
struct NativeHubStartupState {
    NativeStartupStep step;
    NativeControllerCalibration capture;
    NativeProfile profile, previous_profile;
    char profile_id[65], error[192], draft[512], tile_name[64];
    NativeContentId fighter_id;
    int16_t raw[6];
    int8_t trigger_click[2]; // -1 means no independent digital button.
    int controller_index;
    unsigned port;
    unsigned stick_trace_count;
    int16_t stick_trace[2048][4];
    uint64_t started_at;
    bool active, previous_assigned, applied, calibrated, completed, controller_claimed, testing_input;
};
extern NativeHubStartupState nativeHubStartup;
void nativeHubStartup_Initialize(bool has_profile);
void nativeHubStartup_Begin();
void nativeHubStartup_Tick();
bool nativeHubStartup_Activate(HubAction action, int value);
namespace NativeFighterPresentation { struct Framing; }
void nativeHubStartup_PortraitSaved(const char* path, const NativeFighterPresentation::Framing&);
void nativeHubStartup_Finish();
bool nativeHubStartup_HasRoster();
void nativeHubStartup_Render();
void nativeHubStartup_RenderFrame();
void nativeHubStartup_Inspect();
