#ifndef MELEE_GM_NATIVE_PROFILE_H
#define MELEE_GM_NATIVE_PROFILE_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_PROFILE_NAME_BYTES = 32,
    NATIVE_PROFILE_ID_BYTES = 64,
    NATIVE_PROFILE_MAX_BINDINGS = 4,
};

typedef enum NativeProfileAction {
    NativeProfileAction_Attack,
    NativeProfileAction_Special,
    NativeProfileAction_Jump,
    NativeProfileAction_Shield,
    NativeProfileAction_Grab,
    NativeProfileAction_ShortHop,
    NativeProfileAction_Count,
} NativeProfileAction;

typedef enum NativeProfileBindingType {
    NativeProfileBinding_Default,
    NativeProfileBinding_Button,
    NativeProfileBinding_AxisPositive,
    NativeProfileBinding_AxisNegative,
} NativeProfileBindingType;

typedef enum NativeProfileAxis {
    NativeProfileAxis_MoveX,
    NativeProfileAxis_MoveY,
    NativeProfileAxis_CStickX,
    NativeProfileAxis_CStickY,
    NativeProfileAxis_LeftTrigger,
    NativeProfileAxis_RightTrigger,
    NativeProfileAxis_Count,
} NativeProfileAxis;

typedef enum NativeProfileCStick {
    NativeProfileCStick_Smash,
    NativeProfileCStick_Tilt,
} NativeProfileCStick;

typedef struct NativeProfileBinding {
    int16_t native_input;
    uint8_t type;
} NativeProfileBinding;

typedef struct NativeProfileActionBindings {
    NativeProfileBinding bindings[NATIVE_PROFILE_MAX_BINDINGS];
    uint8_t count;
} NativeProfileActionBindings;

typedef struct NativeProfileAxisBinding {
    int16_t native_axis;
    int8_t direction;
    bool assigned;
} NativeProfileAxisBinding;

typedef struct NativeProfileCalibration {
    uint16_t main_stick_dead_zone;
    uint16_t c_stick_dead_zone;
    uint16_t left_trigger_activation;
    uint16_t right_trigger_activation;
    bool use_dead_zones;
    bool emulate_digital_triggers;
    bool measured;
    // Logical axes: main X/Y, C X/Y (positive Y is up), then L/R.
    int16_t center[NativeProfileAxis_Count];
    int16_t minimum[NativeProfileAxis_Count];
    int16_t maximum[NativeProfileAxis_Count];
} NativeProfileCalibration;

typedef struct NativeProfile {
    NativeProfileActionBindings actions[NativeProfileAction_Count];
    NativeProfileAxisBinding axes[NativeProfileAxis_Count];
    NativeProfileCalibration calibration;
    char name[NATIVE_PROFILE_NAME_BYTES + 1];
    uint8_t c_stick;
    bool tap_jump;
} NativeProfile;

typedef struct NativeProfileEntry {
    char id[NATIVE_PROFILE_ID_BYTES + 1];
    char name[NATIVE_PROFILE_NAME_BYTES + 1];
} NativeProfileEntry;

#ifdef __cplusplus
extern "C" {
#endif

void nativeProfile_Default(NativeProfile* profile, const char* name);
bool nativeProfile_Validate(const NativeProfile* profile);
bool nativeProfile_SaveFile(const NativeProfile* profile, const char* path);
bool nativeProfile_LoadFile(NativeProfile* profile, const char* path);
bool nativeProfileStore_IsValidId(const char* profile_id);
bool nativeProfileStore_BuildPath(char* path, size_t capacity,
                                  const char* profile_id);
bool nativeProfileStore_Save(const NativeProfile* profile,
                             const char* profile_id);
bool nativeProfileStore_Load(NativeProfile* profile, const char* profile_id);
bool nativeProfileStore_Delete(const char* profile_id);
// Enumerate valid saved profiles. More than capacity entries is an explicit
// failure, rather than a silently incomplete picker. Missing storage is empty.
bool nativeProfileStore_List(NativeProfileEntry* entries, size_t capacity,
                             size_t* count);

#ifdef __cplusplus
}
#endif

#endif
#endif
