#ifndef MELEE_FT_FTCOSTUME_ASSETS_H
#define MELEE_FT_FTCOSTUME_ASSETS_H

#include <sysdolphin/baselib/dat_import.h>

enum {
    FT_COSTUME_IMPORT_ARENA_SIZE = 512 * 1024,
    FT_COSTUME_IMPORT_RECORD_CAPACITY = 2048,
    FT_COSTUME_IMPORT_MAX_ARENA_SIZE = 64 * 1024 * 1024,
};

typedef struct FtCostumeAssets {
    void* owned_bytes;
    void* arena;
    HSD_Dat dat;
    HSD_DatImport import;
    HSD_Joint* joint;
    HSD_MatAnimJoint* matanim_joint;
    u32 joint_count;
} FtCostumeAssets;

typedef enum FtModelValidity {
    FT_MODEL_VALID,
    FT_MODEL_INVALID_JOINT,
    FT_MODEL_INVALID_JOINT_ANIMATION,
    FT_MODEL_INVALID_MATERIAL_ANIMATION,
} FtModelValidity;

typedef struct FtModelCounts {
    u32 joints;
    u32 construction_units;
} FtModelCounts;

FtModelValidity ftCostumeAssets_ValidateModel(const HSD_Joint* joint,
    const HSD_AnimJoint* animation, const HSD_MatAnimJoint* materials,
    u32 mesh_limit, FtModelCounts* counts);

HSD_Joint* ftCostumeAssets_LoadAttachment(FtCostumeAssets* assets, const char* symbol);

bool ftCostumeAssets_Import(const void* bytes, size_t size,
                            const char* joint_name,
                            const char* matanim_joint_name, void* arena,
                            size_t arena_size, u32 record_capacity,
                            FtCostumeAssets* output);

// Scene/authoring construction only. Owns output->arena; source bytes remain
// caller-owned. Capacity retries discard the incomplete graph before rebuilding.
// Supply a vacant output at its final address; free its arena when retiring it.
bool ftCostumeAssets_Load(const void* bytes, size_t size,
                          const char* joint_name,
                          const char* matanim_joint_name,
                          FtCostumeAssets* output);

#endif
