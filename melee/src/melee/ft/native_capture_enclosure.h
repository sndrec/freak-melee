#ifndef MELEE_NATIVE_CAPTURE_ENCLOSURE_H
#define MELEE_NATIVE_CAPTURE_ENCLOSURE_H

#include <Runtime/platform.h>
#include <dolphin/mtx.h>
#include <melee/ft/forward.h>

typedef struct NativeCaptureEnclosureSettings {
    Vec3 launch_velocity;
    Vec3 escape_velocity;
    float damage_behavior;
    float shrink_fraction;
    float shrink_frames;
    float lifetime;
    float damage_time_scale;
    float timer_decay;
    float mash_reduction;
    float mash_pause_frames;
    float mash_animation_rate;
    int model_slot;
    // The common hurtbox restore operation takes invincibility frames.
    // Stock Yoshi supplies its boolean getter; copied Kirby supplies an int.
    int escape_invincibility_frames;
} NativeCaptureEnclosureSettings;

// Fighter-owned storage participates in the ordinary heap rollback snapshot.
// No holder tuning is read after entry; the holder reference is attribution.
typedef struct NativeCaptureEnclosureState {
    NativeCaptureEnclosureSettings settings;
    Fighter_GObj* holder;
    Vec3 root_scale;
    Vec3 accessory_scale;
    float shrink_remaining;
    float mash_pause_remaining;
    bool mashed;
    bool restore_model_on_escape;
    bool subtract_damage_form;
} NativeCaptureEnclosureState;

#endif
