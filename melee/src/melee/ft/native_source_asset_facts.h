#ifndef MELEE_FT_NATIVE_SOURCE_ASSET_FACTS_H
#define MELEE_FT_NATIVE_SOURCE_ASSET_FACTS_H
#include "ft_part_layout.h"
#include "native_fighter_runtime.h"
#include "native_script_moves.h"

enum { NATIVE_SOURCE_NAME_BYTES=128, NATIVE_SOURCE_MOTIONS=1024,
    NATIVE_SOURCE_OMISSIONS=32, NATIVE_SOURCE_POSE_GRAPHS=16,NATIVE_SOURCE_ARTICLES=64 };
typedef struct NativeSourceMotionData {
    int32_t animation;
    uint32_t flags, attack;
    uint8_t camera; // 0 none, 1 shared standard camera; never an address.
} NativeSourceMotionData;
typedef struct NativeSourcePoseGraph {
    uint32_t name;
    uint16_t slot;
    char path[NATIVE_SOURCE_NAME_BYTES];
} NativeSourcePoseGraph;
typedef struct NativeSourceArticleAsset {
    char declaration[NATIVE_SOURCE_NAME_BYTES];
    int32_t kind;
    uint16_t slot;
    int16_t kind_attribute_offset;
} NativeSourceArticleAsset;

// Owned data only. There are no donor behavior IDs, callback addresses or
// external string/array pointers to retain in a source package.
typedef struct NativeSourceAssetFacts {
    char fighter_root[NATIVE_SOURCE_NAME_BYTES];
    char costume_joint[NATIVE_SOURCE_NAME_BYTES];
    char costume_matanim[NATIVE_SOURCE_NAME_BYTES];
    char demos[4][NATIVE_SOURCE_NAME_BYTES];
    FtCharacterAssetLayout layout;
    NativeScriptMoveLayout moves;
    NativeSourceMotionData motions[NATIVE_SOURCE_MOTIONS];
    NativeFighterBone bones[FT_COMMON_PART_COUNT];
    NativeFighterPartOmission omissions[NATIVE_SOURCE_OMISSIONS];
    NativeSourceArticleAsset articles[NATIVE_SOURCE_ARTICLES];
    NativeSourcePoseGraph poses[NATIVE_SOURCE_POSE_GRAPHS];
    NativeFighterSoundIdRule sound_rules[NATIVE_FIGHTER_SOUND_RULE_CAPACITY];
    NativeFighterKoCue ko_cues[NATIVE_FIGHTER_KO_CUE_CAPACITY];
    uint64_t sound_banks;
    uint16_t actions, demo_count, joints, bone_count, omission_count;
    uint16_t article_count, pose_count, sound_rule_count, ko_cue_count;
    uint8_t animation_tag, effect_bank, sound_mute_channels;
    bool ignore_surface_friction;
} NativeSourceAssetFacts;

#ifdef __cplusplus
extern "C" {
#endif

// Authoring-only copy when starting a draft. Runtime preparation consumes the
// copied facts and DATs directly, never a Definition or its behavior handlers.
bool nativeSourceAssets_Copy(const NativeFighterDefinition*,NativeSourceAssetFacts*,NativeScriptDiagnostic*);
bool nativeSourceAssets_Validate(const NativeSourceAssetFacts*,NativeScriptDiagnostic*);
// Canonical data record, no version bridge and no native structure/pointer dump.
bool nativeSourceAssets_Write(const NativeSourceAssetFacts*,void*,size_t,size_t*,NativeScriptDiagnostic*);
bool nativeSourceAssets_Read(const void*,size_t,NativeSourceAssetFacts*,NativeScriptDiagnostic*);

#ifdef __cplusplus
}
#endif

#endif
