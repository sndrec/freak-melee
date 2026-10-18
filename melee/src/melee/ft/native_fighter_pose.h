#ifndef MELEE_FT_NATIVE_FIGHTER_POSE_H
#define MELEE_FT_NATIVE_FIGHTER_POSE_H

#include <melee/ft/forward.h>
#include <melee/lb/forward.h>
#include <sysdolphin/baselib/forward.h>
#include <stdbool.h>
#include <stdint.h>
#include "ftparts.h"

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
struct NativeFighterRig;

typedef struct NativeFighterPoseBinding {
    uint16_t count;
    uint16_t parts[MAX_FT_PARTS];
    HSD_Joint* descriptors[MAX_FT_PARTS];
} NativeFighterPoseBinding;

// Scene-construction validation resolves owned stable IDs and descriptor nodes
// once. The resulting binding stays immutable for the match.
bool nativeFighterPose_BindGraph(const struct FtCharacterAssets*, unsigned slot,
    const char* path, const struct NativeFighterRig*, const int32_t* bone_ids,
    unsigned count, NativeFighterPoseBinding*);
// Copy/blend the explicitly paired bones using the game's joint-pose operations.
// Validate every needed destination before mutation. No traversal or allocation.
bool nativeFighterPose_ApplyMapped(Fighter*, int32_t destination,
    const NativeFighterPoseBinding*, bool blend, float descriptor_weight,
    float pose_weight);

// Pure validation precedes the actual shared ftAnim/HSD operation. No pose
// mutation, clip selection policy, animation bookkeeping or ticking allocation.
bool nativeFighterPose_Validate(Fighter_GObj*, NativeFighterPoseOperation,
                                int32_t destination, int32_t raw_start,
                                HSD_Joint* graph, FigaTree* clip);
bool nativeFighterPose_PathValid(const char* path);
HSD_Joint* nativeFighterPose_SelectGraph(const struct FtCharacterAssets*, unsigned slot,
                                        const char* path);

#endif
