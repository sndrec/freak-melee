#pragma once
#include "native_hub_internal.h"
#include <melee/gm/native_profile_input.h>
#include <melee/gm/native_controller_calibration.h>
#include <SDL3/SDL_keycode.h>
extern "C" {
#include <sysdolphin/baselib/controller.h>
}

enum NativeProfileCalibrationStep { ProfileCalibration_Rest, ProfileCalibration_Sticks,
    ProfileCalibration_Triggers, ProfileCalibration_Review };
struct NativeProfileEditor {
    NativeProfileEntry catalog[64];
    size_t count;
    NativeProfile draft;
    char id[65], zones[4][8], status[192];
    uint16_t zone_values[4];
    unsigned port, first, tab;
    bool initialized, deleting;
    uint32_t controller;
    int capture_action = -1, capture_axis = -1;
    bool capture_add, capture_ready, capture_release;
    uint64_t capture_deadline, capture_cancel_at;
    NativeProfileBinding captured_binding;
    NativeProfileAxisBinding captured_axis;
    NativeControllerCalibration capture;
    NativeProfileCalibration original_calibration;
    NativeProfile previous_profile;
    NativeProfileCalibrationStep step;
    int16_t raw[6];
    int8_t clicks[2];
    int dragged = -1;
    uint64_t step_started;
    bool testing, applied, previous_assigned, guided;
};
extern NativeProfileEditor nativeProfileEditor;
bool nativeHubProfiles_Page(NativeHubPage page);
void nativeHubProfiles_Open();
void nativeHubProfiles_Tick();
void nativeHubProfiles_Render();
bool nativeHubProfiles_Activate(HubAction action, int value);
void nativeHubProfiles_CommitField();
bool nativeHubProfiles_OwnsField(const char* field);
bool nativeHubProfiles_Key(SDL_Keycode key);
bool nativeHubProfiles_Controller(unsigned port, const HSD_PadStatus& pad);
bool nativeHubProfiles_LockCamera();
void nativeHubProfiles_Leave(NativeHubPage destination);
void nativeHubProfiles_Inspect();
void nativeHubProfiles_FormatZones();
void nativeHubProfileDevices_Render();
bool nativeHubProfileDevices_Activate(HubAction action, int value);
void nativeHubProfileDevices_Inspect();
void nativeHubProfileCalibration_Begin(bool review);
void nativeHubProfileCalibration_Tick();
void nativeHubProfileCalibration_Render();
bool nativeHubProfileCalibration_Activate(HubAction action, int value);
void nativeHubProfileCalibration_Restore(bool discard);
