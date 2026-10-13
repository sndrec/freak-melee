#ifndef MELEE_FT_NATIVE_FIGHTER_POSE_H
#define MELEE_FT_NATIVE_FIGHTER_POSE_H

#include <melee/ft/forward.h>
#include <melee/lb/forward.h>
#include <sysdolphin/baselib/forward.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum NativeFighterPoseDestination {
    NativeFighterPose_Live,
    NativeFighterPose_Blend,
} NativeFighterPoseDestination;

typedef enum NativeFighterPoseOperation {
    NativeFighterPose_LoadAnimation,
    NativeFighterPose_RequestFrame,
    NativeFighterPose_Evaluate,
    NativeFighterPose_CopyCostume,
    NativeFighterPose_CopyGraph,
    NativeFighterPose_BlendGraph,
    NativeFighterPose_BlendToLive,
    NativeFighterPose_CopyToLive,
    NativeFighterPose_RestorePartAnimation,
} NativeFighterPoseOperation;

struct FtCharacterAssets;

// Pure validation precedes the actual shared ftAnim/HSD operation. No pose
// mutation, clip selection policy, animation bookkeeping or ticking allocation.
bool nativeFighterPose_Validate(Fighter_GObj*, NativeFighterPoseOperation,
                                int32_t destination, int32_t raw_start,
                                HSD_Joint* graph, FigaTree* clip);
bool nativeFighterPose_PathValid(const char* path);
HSD_Joint* nativeFighterPose_SelectGraph(const struct FtCharacterAssets*, unsigned slot,
                                        const char* path);

#endif
