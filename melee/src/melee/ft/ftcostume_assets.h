#ifndef MELEE_FT_FTCOSTUME_ASSETS_H
#define MELEE_FT_FTCOSTUME_ASSETS_H

#include <sysdolphin/baselib/dat_import.h>

enum {
    FT_COSTUME_IMPORT_ARENA_SIZE = 512 * 1024,
    FT_COSTUME_IMPORT_RECORD_CAPACITY = 2048,
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

bool ftCostumeAssets_Import(const void* bytes, size_t size,
                            const char* joint_name,
                            const char* matanim_joint_name, void* arena,
                            size_t arena_size, u32 record_capacity,
                            FtCostumeAssets* output);

#endif
