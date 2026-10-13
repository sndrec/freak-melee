#include "ftparts.h"
#include "fighter.h"
#include "types.h"

struct Fighter_804D6540_t** Fighter_804D6540 = NULL;
FighterPartsTable** ftPartsTable = NULL;

Fighter_Part ftParts_GetBoneIndex(Fighter* fp, Fighter_Part part)
{
    return ftPartsTable[fp->kind]->part_to_joint[part];
}

int ftPartsRemap(size_t to_table_idx, size_t from_table_idx, size_t joint_idx)
{
    FighterPartsTable* from_table = ftPartsTable[from_table_idx];
    if (joint_idx < from_table->parts_num) {
        size_t part_idx = from_table->joint_to_part[joint_idx];
        if (part_idx != FTPART_INVALID) {
            return ftPartsTable[to_table_idx]->part_to_joint[part_idx];
        }
    }
    return FTPART_INVALID;
}

u32 ftParts_8007506C(enum FighterKind ftkind, int part)
{
    struct Fighter_804D6540_x0_t* var_r3;
    int i;
    struct Fighter_804D6540_t* temp_r3;

    temp_r3 = Fighter_804D6540[ftkind];
    if (temp_r3 != NULL && temp_r3->x4 != 0) {
        var_r3 = temp_r3->x0;
        for (i = 0; i < temp_r3->x4; i++, var_r3++) {
            if (var_r3->x0 == part) {
                return 1U << i;
            }
        }
    }
    return 0;
}

