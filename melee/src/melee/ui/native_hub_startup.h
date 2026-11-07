#pragma once
#include "native_hub_internal.h"
#include <melee/gm/native_controller_calibration.h>

enum NativeStartupStep {
    NativeStartup_Welcome, NativeStartup_Profile, NativeStartup_Account,
    NativeStartup_Controller, NativeStartup_Rest, NativeStartup_Sticks,
    NativeStartup_Triggers, NativeStartup_Test, NativeStartup_Clone, NativeStartup_Done
};
struct NativeHubStartupState {
    NativeStartupStep step;
    NativeControllerCalibration capture;
    NativeProfile profile, previous_profile;
    char profile_id[65], error[192];
    int16_t raw[6];
    int controller_index;
    unsigned port;
    uint64_t started_at;
    bool active, previous_assigned, applied, calibrated, completed, controller_claimed;
};
extern NativeHubStartupState nativeHubStartup;
void nativeHubStartup_Initialize(bool has_profile);
void nativeHubStartup_Begin();
void nativeHubStartup_Tick();
bool nativeHubStartup_Activate(HubAction action, int value);
void nativeHubStartup_Cloned();
bool nativeHubStartup_HasFighter();
void nativeHubStartup_Render();
void nativeHubStartup_RenderFrame();
void nativeHubStartup_Inspect();
