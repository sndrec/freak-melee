#include "ftcostume_assets.h"

#include <sysdolphin/baselib/jobj.h>

#include <string.h>

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
    if (output == NULL || joint_name == NULL) {
        return false;
    }
    memset(output, 0, sizeof(*output));
    if (HSD_DatOpen(&output->dat, bytes, size) != HSD_DAT_OK ||
        HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK)
    {
        return false;
    }
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
