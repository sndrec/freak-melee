#ifndef MELEE_FT_NATIVE_FIGHTER_ACTION_EDITOR_H
#define MELEE_FT_NATIVE_FIGHTER_ACTION_EDITOR_H

#ifdef MELEE_NATIVE

#include "native_fighter_project.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NATIVE_FIGHTER_ACTION_CAPACITY = 1024,
    NATIVE_FIGHTER_MARIO_ACTION_COUNT = 303,
    NATIVE_FIGHTER_MARIO_JOINT_COUNT = 61,
};

typedef struct NativeFighterActionInfo {
    uint16_t index;
    bool has_script;
    char name[64];
} NativeFighterActionInfo;

typedef struct NativeFighterActionMetadata {
    int32_t animation_offset;
    int32_t animation_size;
    uint32_t animation_flags;
} NativeFighterActionMetadata;

typedef enum NativeFighterActionEventKind {
    NativeFighterActionEvent_SyncTimer,
    NativeFighterActionEvent_AsyncTimer,
    NativeFighterActionEvent_CreateHitbox,
    NativeFighterActionEvent_SetHitboxDamage,
    NativeFighterActionEvent_SetHitboxScale,
    NativeFighterActionEvent_SetHitboxFlag,
    NativeFighterActionEvent_SetCommandVar,
    NativeFighterActionEvent_SetBodyCollisionState,
    NativeFighterActionEvent_SetGlobalHurtState,
    NativeFighterActionEvent_SetBoneHurtState,
    NativeFighterActionEvent_SoundEffect,
    NativeFighterActionEvent_SpawnEffect,
    NativeFighterActionEvent_ThrowHitbox,
    NativeFighterActionEvent_AllowInterrupt,
    NativeFighterActionEvent_NeutralCue,
    NativeFighterActionEvent_ClearHitbox,
    NativeFighterActionEvent_ClearAllHitboxes,
    NativeFighterActionEvent_ReleaseThrow,
    NativeFighterActionEvent_ReverseFacing,
    NativeFighterActionEvent_SetAirborneState,
} NativeFighterActionEventKind;

typedef struct NativeFighterActionEvent {
    uint32_t source_offset;
    uint32_t value;
    uint32_t target_id;
    uint8_t kind;
} NativeFighterActionEvent;

typedef struct NativeFighterActionCommand {
    uint32_t source_offset;
    uint8_t opcode;
    uint8_t words;
} NativeFighterActionCommand;

typedef struct NativeFighterActionOccurrence {
    uint32_t source_offset;
    uint16_t frame;
    uint8_t opcode;
} NativeFighterActionOccurrence;

// Original five-word CreateHitbox command. Radius and offsets use the game's
// 1/256-unit encoding; no floating-point round trip is involved. Offset names
// follow the source command fields, whose evaluator maps z/x into b_offset.x/z.
typedef struct NativeFighterHitbox {
    uint8_t id;
    uint8_t hit_group;
    uint8_t only_hit_grabbed;
    uint8_t bone;
    uint8_t use_common_bone_ids;
    uint16_t damage;
    uint16_t radius;
    int16_t offset_x;
    int16_t offset_y;
    int16_t offset_z;
    uint16_t angle;
    uint16_t knockback_growth;
    uint16_t weight_set_knockback;
    uint8_t item_hit_interaction;
    uint8_t ignore_thrown_fighters;
    uint8_t ignore_fighter_scale;
    uint8_t clank;
    uint8_t rebound;
    uint16_t base_knockback;
    uint8_t element;
    int8_t shield_damage;
    uint8_t hit_sfx_severity;
    uint8_t hit_sfx_kind;
    uint8_t hit_grounded;
    uint8_t hit_aerial;
} NativeFighterHitbox;

typedef struct NativeFighterSoundEffect {
    uint8_t behavior;
    uint8_t volume;
    uint8_t panning;
    uint32_t sfx_id;
} NativeFighterSoundEffect;

// Original five-word GFX command. Offsets and ranges are unscaled source
// units (the evaluator multiplies them by 1/256). Unknown fields are retained.
typedef struct NativeFighterSpawnEffect {
    uint8_t bone;
    uint8_t use_common_bone_ids;
    uint8_t destroy_on_state_change;
    uint8_t use_unknown_bone;
    uint16_t gfx_id;
    int16_t offset_z;
    int16_t offset_y;
    int16_t offset_x;
    uint16_t range_z;
    uint16_t range_y;
    uint16_t range_x;
} NativeFighterSpawnEffect;

// List original archive actions and reachable editable commands. Source
// offsets identify actual commands in this draft, including subroutines.
// No control flow is flattened into a guessed timeline.
bool nativeFighterAction_List(const NativeFighterProject* project,
                              NativeFighterActionInfo* actions,
                              size_t capacity, size_t* count);
// Flags use the original DAT word's bit positions. The native importer maps
// those bits into Fighter_WaitAnimData; bits 22-23 are not imported and cannot
// be edited through this API.
bool nativeFighterAction_GetMetadata(const NativeFighterProject* project,
                                     unsigned action,
                                     NativeFighterActionMetadata* metadata);
bool nativeFighterAction_SetAnimationFlags(NativeFighterProject* project,
                                           unsigned action, uint32_t flags);
bool nativeFighterAction_Events(const NativeFighterProject* project,
                                unsigned action, NativeFighterActionEvent* events,
                                size_t capacity, size_t* count);
// Reachable original source commands, including commands not yet editable by
// the event inspector. A NULL array with capacity zero queries the count.
bool nativeFighterAction_Commands(const NativeFighterProject* project,
                                  unsigned action,
                                  NativeFighterActionCommand* commands,
                                  size_t capacity, size_t* count);
// Unroll deterministic timers, loops, subroutines and gotos in execution
// order. Repeated source commands produce repeated occurrences. A command
// that waits for animation completion, a malformed control stack, or the
// requested bounds ends the trace with complete=false; callers must not
// assign guessed frames to commands outside the trace.
bool nativeFighterAction_Timeline(const NativeFighterProject* project,
                                  unsigned action,
                                  NativeFighterActionOccurrence* occurrences,
                                  size_t capacity, size_t* count,
                                  bool* complete);
// Insert a copy of an existing non-control command after one reachable
// command. The destination action becomes local; reachable branches and
// subroutines are copied with remapped targets.
bool nativeFighterAction_InsertCommand(NativeFighterProject* project,
                                       unsigned action, uint32_t after_offset,
                                       unsigned template_action,
                                       uint32_t template_offset,
                                       uint32_t* inserted_offset);
// Delete one reachable non-control command from an action-local copy of its
// source graph. References into that command reject the edit.
bool nativeFighterAction_DeleteCommand(NativeFighterProject* project,
                                       unsigned action, uint32_t source_offset);
// Copy the action's reachable source into private archive space, preserving
// behavior and mapping a selected command to its new source offset.
bool nativeFighterAction_LocalizeCommand(NativeFighterProject* project,
                                         unsigned action,
                                         uint32_t source_offset,
                                         uint32_t* local_offset);
bool nativeFighterAction_Users(const NativeFighterProject* project,
                               uint32_t source_offset, unsigned* count);
bool nativeFighterAction_SetValue(NativeFighterProject* project,
                                  unsigned action, uint32_t source_offset,
                                  uint32_t value);
bool nativeFighterAction_GetHitbox(const NativeFighterProject* project,
                                   unsigned action, uint32_t source_offset,
                                   NativeFighterHitbox* hitbox);
bool nativeFighterAction_SetHitbox(NativeFighterProject* project,
                                   unsigned action, uint32_t source_offset,
                                   const NativeFighterHitbox* hitbox);
// Opcode 17 carries an ID and volume/panning in the following two words.
// Canned behavior variants (10-15) have no editable SFX ID.
bool nativeFighterAction_GetSoundEffect(const NativeFighterProject* project,
                                        unsigned action, uint32_t source_offset,
                                        NativeFighterSoundEffect* sound);
bool nativeFighterAction_SetSoundEffect(NativeFighterProject* project,
                                        unsigned action, uint32_t source_offset,
                                        const NativeFighterSoundEffect* sound);
bool nativeFighterAction_GetSpawnEffect(const NativeFighterProject* project,
                                        unsigned action, uint32_t source_offset,
                                        NativeFighterSpawnEffect* effect);
bool nativeFighterAction_SetSpawnEffect(NativeFighterProject* project,
                                        unsigned action, uint32_t source_offset,
                                        const NativeFighterSpawnEffect* effect);

#ifdef __cplusplus
}
#endif

#endif
#endif
