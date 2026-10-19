#include "ftcostume_assets.h"

#include <sysdolphin/baselib/jobj.h>

#include <string.h>
#include <stdlib.h>

static u32 count_joints(const HSD_Joint* joint)
{
    u32 count = 0;
    for (; joint != NULL; joint = joint->next) {
        ++count;
        if (!(joint->flags & JOBJ_INSTANCE)) {
            count += count_joints(joint->child);
        }
    }
    return count;
}

bool ftCostumeAssets_Import(const void* bytes, size_t size,
                            const char* joint_name,
                            const char* matanim_joint_name, void* arena,
                            size_t arena_size, u32 record_capacity,
                            FtCostumeAssets* output)
{
    u32 joint_root;
    u32 matanim_root;
    if (output == NULL) return false;
    memset(output, 0, sizeof(*output));
    if (joint_name == NULL) {
        output->import.status = HSD_DAT_SYMBOL;
        return false;
    }
    output->import.status = HSD_DatOpen(&output->dat, bytes, size);
    if (output->import.status != HSD_DAT_OK) return false;
    if (HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK) return false;
    if (!HSD_DatFindRoot(&output->dat, joint_name, &joint_root)) {
        output->import.status = HSD_DAT_SYMBOL;
        return false;
    }
    output->joint = HSD_DatImportGraph(&output->import, joint_root,
                                       DAT_JOINT);
    if (output->joint == NULL) {
        return false;
    }
    output->joint_count = count_joints(output->joint);
    if (matanim_joint_name != NULL && matanim_joint_name[0] != '\0') {
        if (!HSD_DatFindRoot(&output->dat, matanim_joint_name,
                             &matanim_root))
        {
            output->import.status = HSD_DAT_SYMBOL;
            return false;
        }
        output->matanim_joint = HSD_DatImportGraph(
            &output->import, matanim_root, DAT_MATANIM_JOINT);
        if (output->matanim_joint == NULL) {
            return false;
        }
    }
    return true;
}

bool ftCostumeAssets_Load(const void* bytes, size_t size,
                          const char* joint_name,
                          const char* matanim_joint_name,
                          FtCostumeAssets* output)
{
    if (!output) return false;
    memset(output, 0, sizeof(*output));
    size_t arena_size = FT_COSTUME_IMPORT_ARENA_SIZE;
    u32 records = FT_COSTUME_IMPORT_RECORD_CAPACITY;
    for (;;) {
        void* arena = malloc(arena_size);
        if (!arena) {
            memset(output, 0, sizeof(*output));
            output->import.status = HSD_DAT_CAPACITY;
            return false;
        }
        if (ftCostumeAssets_Import(bytes, size, joint_name, matanim_joint_name,
                                   arena, arena_size, records, output)) {
            output->arena = arena;
            return true;
        }
        const bool capacity = output->import.status == HSD_DAT_CAPACITY;
        const bool record_limit = output->import.record_count == records;
        free(arena);
        // Preserve diagnostics, but never expose pointers into a failed arena.
        output->joint = NULL;
        output->matanim_joint = NULL;
        output->joint_count = 0;
        output->import.arena = NULL;
        output->import.records = NULL;
        output->import.slots = NULL;
        if (!capacity) return false;
        if (record_limit) {
            records *= 2;
        } else {
            if (arena_size == FT_COSTUME_IMPORT_MAX_ARENA_SIZE) return false;
            arena_size *= 2;
        }
    }
}
