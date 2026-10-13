#include "native_fighter_move_copy.h"

#ifdef MELEE_NATIVE

#include "native_fighter_action_editor.h"
#include "native_fighter_animation_editor.h"
#include <sysdolphin/baselib/dat.h>

#include <stdlib.h>
#include <string.h>

static bool overlap(unsigned left, unsigned left_size,
                    unsigned right, unsigned right_size)
{
    return left < right + right_size && right < left + left_size;
}

static bool command_overlap(const NativeFighterActionCommand* selected,
                            size_t selected_count,
                            const NativeFighterActionCommand* other,
                            size_t other_count)
{
    size_t i = 0, j = 0;
    while (i < selected_count && j < other_count) {
        unsigned start = selected[i].source_offset;
        unsigned end = start + selected[i].words * 4;
        unsigned other_start = other[j].source_offset;
        unsigned other_end = other_start + other[j].words * 4;
        if (start < other_end && other_start < end) return true;
        if (end <= other_start) ++i;
        else ++j;
    }
    return false;
}

static bool commands_exclusive(const NativeFighterProject* project,
                               unsigned action,
                               const NativeFighterActionCommand* selected,
                               size_t selected_count)
{
    NativeFighterActionCommand other[1024];
    for (unsigned index = 0; index < NATIVE_FIGHTER_MARIO_ACTION_COUNT;
         ++index) {
        if (index == action) continue;
        size_t count;
        if (!nativeFighterAction_Commands(project, index, other,
                                          1024, &count) ||
            command_overlap(selected, selected_count, other, count))
            return false;
    }
    return true;
}

static bool clip_exclusive(const NativeFighterProject* project,
                           unsigned action,
                           const NativeFighterAnimationInfo* selected)
{
    for (unsigned index = 0; index < NATIVE_FIGHTER_MARIO_ACTION_COUNT;
         ++index) {
        if (index == action) continue;
        NativeFighterAnimationInfo other;
        if (nativeFighterAnimation_Info(project, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, index }, &other) &&
            overlap(selected->clip_offset, selected->clip_size,
                    other.clip_offset, other.clip_size)) return false;
    }
    return true;
}

static bool pointer_fields_match(const HSD_Dat* source, const HSD_Dat* destination,
                                 const NativeFighterActionCommand* commands,
                                 size_t count)
{
    for (u32 index = 0; index < source->relocation_count; ++index) {
        u32 slot, left, right;
        if (!HSD_DatRelocation(source, index, &slot)) return false;
        for (size_t command = 0; command < count; ++command) {
            unsigned start = commands[command].source_offset;
            if (slot < start) break;
            if (slot >= start + commands[command].words * 4) continue;
            if (!HSD_DatReadU32(source, slot, &left) ||
                !HSD_DatReadU32(destination, slot, &right) ||
                left != right) return false;
            break;
        }
    }
    return true;
}

bool nativeFighterMove_CopyNormalAction(const NativeFighterProject* source,
                                        NativeFighterProject* destination,
                                        unsigned action)
{
    if (source == NULL || destination == NULL || source == destination ||
        source->fighter_dat == destination->fighter_dat ||
        source->animation_dat == destination->animation_dat ||
        action < 46 || action > 77 ||
        source->fighter_size != destination->fighter_size ||
        source->animation_size != destination->animation_size)
        return false;
    // Copy assets, never source behavior/state. Validate actual owned profiles
    // instead of deciding archive compatibility from a donor/code identity.
    NativeSourceAssetFacts from,to;NativeScriptDiagnostic diagnostic;
    if(!nativeSourceAssets_Read(source->asset_record,source->asset_record_size,&from,&diagnostic)||
       !nativeSourceAssets_Read(destination->asset_record,destination->asset_record_size,&to,&diagnostic)||
       from.actions!=NATIVE_FIGHTER_MARIO_ACTION_COUNT||to.actions!=from.actions||
       strcmp(source->fighter_root,from.fighter_root)||strcmp(destination->fighter_root,to.fighter_root)||
       from.joints!=to.joints||from.bone_count!=to.bone_count||from.omission_count!=to.omission_count||
       memcmp(&from.layout,&to.layout,sizeof(from.layout))||
       memcmp(from.bones,to.bones,from.bone_count*sizeof(*from.bones))||
       memcmp(from.omissions,to.omissions,from.omission_count*sizeof(*from.omissions)))return false;

    NativeFighterActionInfo source_actions[NATIVE_FIGHTER_MARIO_ACTION_COUNT];
    NativeFighterActionInfo destination_actions[NATIVE_FIGHTER_MARIO_ACTION_COUNT];
    size_t source_count, destination_count;
    if (!nativeFighterAction_List(source, source_actions,
                                  NATIVE_FIGHTER_MARIO_ACTION_COUNT,
                                  &source_count) ||
        !nativeFighterAction_List(destination, destination_actions,
                                  NATIVE_FIGHTER_MARIO_ACTION_COUNT,
                                  &destination_count) ||
        source_count != destination_count ||
        (action <= 72
             ? strncmp(source_actions[action].name, "Attack", 6) != 0
             : strncmp(source_actions[action].name, "LandingAir", 10) != 0) ||
        strcmp(source_actions[action].name,
               destination_actions[action].name) != 0)
        return false;

    HSD_Dat source_dat, destination_dat;
    if (HSD_DatOpen(&source_dat, source->fighter_dat,
                    source->fighter_size) != HSD_DAT_OK ||
        HSD_DatOpen(&destination_dat, destination->fighter_dat,
                    destination->fighter_size) != HSD_DAT_OK ||
        source_dat.data_size != destination_dat.data_size ||
        memcmp(source->fighter_dat + 32 + source_dat.data_size,
               destination->fighter_dat + 32 + destination_dat.data_size,
               source->fighter_size - 32 - source_dat.data_size) != 0)
        return false;

    NativeFighterActionMetadata source_meta, destination_meta;
    NativeFighterAnimationInfo source_clip, destination_clip;
    if (!nativeFighterAction_GetMetadata(source, action, &source_meta) ||
        !nativeFighterAction_GetMetadata(destination, action,
                                         &destination_meta) ||
        source_meta.animation_offset != destination_meta.animation_offset ||
        source_meta.animation_size != destination_meta.animation_size ||
        ((source_meta.animation_flags ^ destination_meta.animation_flags) &
         0x00C00000U) != 0 ||
        !nativeFighterAnimation_Info(source, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, &source_clip) ||
        !nativeFighterAnimation_Info(destination, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, &destination_clip) ||
        source_clip.clip_offset != destination_clip.clip_offset ||
        source_clip.clip_size != destination_clip.clip_size ||
        source_clip.joints != destination_clip.joints ||
        source_clip.tracks != destination_clip.tracks ||
        !clip_exclusive(source, action, &source_clip) ||
        !clip_exclusive(destination, action, &destination_clip))
        return false;

    size_t source_commands, destination_commands;
    if (!nativeFighterAction_Commands(source, action, NULL, 0,
                                      &source_commands) ||
        !nativeFighterAction_Commands(destination, action, NULL, 0,
                                      &destination_commands) ||
        source_commands == 0 || source_commands != destination_commands ||
        source_commands > 1024) return false;
    NativeFighterActionCommand* left = malloc(source_commands * sizeof(*left));
    NativeFighterActionCommand* right = malloc(source_commands * sizeof(*right));
    bool ok = left != NULL && right != NULL &&
        nativeFighterAction_Commands(source, action, left, source_commands,
                                     &source_commands) &&
        nativeFighterAction_Commands(destination, action, right,
                                     destination_commands,
                                     &destination_commands);
    for (size_t i = 0; ok && i < source_commands; ++i) {
        ok = left[i].source_offset == right[i].source_offset &&
             left[i].opcode == right[i].opcode &&
             left[i].words == right[i].words;
        if (ok && (left[i].opcode == 5 || left[i].opcode == 7)) {
            u32 src, dst;
            ok = HSD_DatReadU32(&source_dat, left[i].source_offset + 4,
                                &src) &&
                 HSD_DatReadU32(&destination_dat, right[i].source_offset + 4,
                                &dst) && src == dst;
        }
    }
    if (ok)
        ok = commands_exclusive(source, action, left, source_commands) &&
             commands_exclusive(destination, action, right,
                                destination_commands) &&
             pointer_fields_match(&source_dat, &destination_dat, left,
                                  source_commands);
    if (!ok) {
        free(right);
        free(left);
        return false;
    }

    uint8_t* fighter = malloc(destination->fighter_size);
    uint8_t* animation = malloc(destination->animation_size);
    if (fighter == NULL || animation == NULL) {
        free(animation);
        free(fighter);
        free(right);
        free(left);
        return false;
    }
    memcpy(fighter, destination->fighter_dat, destination->fighter_size);
    memcpy(animation, destination->animation_dat, destination->animation_size);
    for (size_t i = 0; i < source_commands; ++i) {
        unsigned offset = left[i].source_offset;
        memcpy(fighter + 32 + offset, source->fighter_dat + 32 + offset,
               left[i].words * 4);
    }
    memcpy(animation + source_clip.clip_offset,
           source->animation_dat + source_clip.clip_offset,
           source_clip.clip_size);
    NativeFighterProject candidate = *destination;
    candidate.fighter_dat = fighter;
    candidate.animation_dat = animation;
    NativeFighterAnimationInfo copied_clip;
    size_t copied_commands;
    ok = nativeFighterAction_SetAnimationFlags(
             &candidate, action, source_meta.animation_flags) &&
         nativeFighterAnimation_Info(&candidate, (NativeFighterAnimationClip){ NativeFighterAnimation_Action, action }, &copied_clip) &&
         nativeFighterAction_Commands(&candidate, action, NULL, 0,
                                      &copied_commands) &&
         copied_clip.clip_offset == source_clip.clip_offset &&
         copied_clip.clip_size == source_clip.clip_size &&
         copied_commands == source_commands;
    if (ok) {
        free(destination->fighter_dat);
        free(destination->animation_dat);
        destination->fighter_dat = fighter;
        destination->animation_dat = animation;
    } else {
        free(animation);
        free(fighter);
    }
    free(right);
    free(left);
    return ok;
}

bool nativeFighterMove_CopyNormalGroup(const NativeFighterProject* source,
                                       NativeFighterProject* destination,
                                       NativeFighterMoveGroup group)
{
    static const struct {
        unsigned count;
        unsigned actions[3];
    } groups[] = {
        { 3, { 46, 47, 48 } }, // Three jab steps.
        { 3, { 53, 55, 57 } }, // Forward tilt high, straight and low.
        { 3, { 60, 62, 64 } }, // Forward smash high, straight and low.
        { 2, { 68, 73 } }, // Aerial and its landing action.
        { 2, { 69, 74 } },
        { 2, { 70, 75 } },
        { 2, { 71, 76 } },
        { 2, { 72, 77 } },
    };
    if (source == NULL || destination == NULL || source == destination ||
        (unsigned) group >= sizeof(groups) / sizeof(groups[0]) ||
        destination->fighter_dat == NULL ||
        destination->animation_dat == NULL) return false;

    NativeFighterProject working = *destination;
    working.fighter_dat = malloc(destination->fighter_size);
    working.animation_dat = malloc(destination->animation_size);
    if (working.fighter_dat == NULL || working.animation_dat == NULL) {
        free(working.animation_dat);
        free(working.fighter_dat);
        return false;
    }
    memcpy(working.fighter_dat, destination->fighter_dat,
           destination->fighter_size);
    memcpy(working.animation_dat, destination->animation_dat,
           destination->animation_size);
    bool copied = true;
    for (unsigned i = 0; i < groups[group].count && copied; ++i)
        copied = nativeFighterMove_CopyNormalAction(source, &working,
                                                    groups[group].actions[i]);
    if (copied) {
        free(destination->fighter_dat);
        free(destination->animation_dat);
        destination->fighter_dat = working.fighter_dat;
        destination->animation_dat = working.animation_dat;
    } else {
        free(working.fighter_dat);
        free(working.animation_dat);
    }
    return copied;
}

#endif
