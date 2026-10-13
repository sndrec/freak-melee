#ifndef MELEE_FT_FTDAT_H
#define MELEE_FT_FTDAT_H
#include "types.h"
#include "ft_part_layout.h"
#include <sysdolphin/baselib/dat.h>

HSD_DatResult ftData_DecodeCommonAttrs(const HSD_Dat*, u32, ftCo_DatAttrs*);

// Decode the x44 collision record into the original native type. Part indices
// refer to the fighter part table, including any omitted model slots.
HSD_DatResult ftData_DecodeCollision(const HSD_Dat* dat, u32 offset,
                                    ftData_x44_t* output);

// The common semantic mapping in PlCo.dat contains 54 entries, including the
// unnamed entry after TransN2. The other mapping has parts_num entries.
HSD_DatResult ftData_DecodeParts(const HSD_Dat* dat, u32 offset,
    FighterPartsTable* output, u8* joint_to_part, size_t joint_capacity,
    u8 part_to_joint[FT_COMMON_PART_COUNT]);
struct Fighter_804D6540_t;
struct Fighter_804D6540_x0_t;
HSD_DatResult ftData_DecodePartOmissions(const HSD_Dat* dat, u32 offset,
    struct Fighter_804D6540_t* output,
    struct Fighter_804D6540_x0_t* records, size_t capacity, u32 part_count);
#endif
