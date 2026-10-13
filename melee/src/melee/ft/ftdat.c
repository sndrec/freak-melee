#include "ftdat.h"
#include "fighter.h"
#include <math.h>
#include <string.h>

HSD_DatResult ftData_DecodeCollision(const HSD_Dat* dat, u32 offset,
                                    ftData_x44_t* output)
{
    ftData_x44_t result;
    u16 indices[6];
    if (!output || !HSD_DatBytes(dat, offset, 28)) return HSD_DAT_DESCRIPTOR;
    for (unsigned i = 0; i < 6; ++i) {
        HSD_DatReadU16(dat, offset + i * 2, &indices[i]);
        if (indices[i] > 32767) return HSD_DAT_DESCRIPTOR;
    }
    result.unk0 = indices[0]; result.unk2 = indices[1];
    result.unk4 = indices[2]; result.unk6 = indices[3];
    result.unk8 = indices[4]; result.unkA = indices[5];
    HSD_DatReadF32(dat, offset + 12, &result.unkC);
    HSD_DatReadF32(dat, offset + 16, &result.ledge_snap_x);
    HSD_DatReadF32(dat, offset + 20, &result.ledge_snap_y);
    HSD_DatReadF32(dat, offset + 24, &result.ledge_snap_height);
    if (!isfinite(result.unkC) || !isfinite(result.ledge_snap_x) ||
        !isfinite(result.ledge_snap_y) || !isfinite(result.ledge_snap_height))
        return HSD_DAT_DESCRIPTOR;
    *output = result;
    return HSD_DAT_OK;
}

HSD_DatResult ftData_DecodeParts(const HSD_Dat* dat, u32 offset,
    FighterPartsTable* output, u8* joint_to_part, size_t joint_capacity,
    u8 part_to_joint[FT_COMMON_PART_COUNT])
{
    u32 count, joints_offset, parts_offset;
    const char* external;
    const u8* joints;
    const u8* parts;
    if (!output || !HSD_DatBytes(dat, offset, 12)) return HSD_DAT_DESCRIPTOR;
    HSD_DatReadU32(dat, offset + 8, &count);
    if (!count || count > 256 ||
        HSD_DatResolve(dat, offset, &joints_offset, &external) != HSD_DAT_REF_LOCAL ||
        HSD_DatResolve(dat, offset + 4, &parts_offset, &external) != HSD_DAT_REF_LOCAL ||
        !(joints = HSD_DatBytes(dat, joints_offset, count)) ||
        !(parts = HSD_DatBytes(dat, parts_offset, FT_COMMON_PART_COUNT)))
        return HSD_DAT_DESCRIPTOR;
    if (count > joint_capacity) return HSD_DAT_CAPACITY;
    if (!joint_to_part || !part_to_joint) return HSD_DAT_DESCRIPTOR;
    for (u32 i = 0; i < count; ++i) {
        if (joints[i] != FTPART_INVALID && joints[i] >= FT_COMMON_PART_COUNT)
            return HSD_DAT_DESCRIPTOR;
    }
    for (unsigned i = 0; i < FT_COMMON_PART_COUNT; ++i) {
        if (parts[i] != FTPART_INVALID && parts[i] >= count)
            return HSD_DAT_DESCRIPTOR;
    }
    memcpy(joint_to_part, joints, count);
    memcpy(part_to_joint, parts, FT_COMMON_PART_COUNT);
    *output = (FighterPartsTable) { joint_to_part, part_to_joint, count };
    return HSD_DAT_OK;
}

HSD_DatResult ftData_DecodePartOmissions(const HSD_Dat* dat, u32 offset,
    struct Fighter_804D6540_t* output,
    struct Fighter_804D6540_x0_t* records, size_t capacity, u32 part_count)
{
    u32 count, records_offset;
    const char* external;
    if (!output || !HSD_DatBytes(dat, offset, 8)) return HSD_DAT_DESCRIPTOR;
    HSD_DatReadU32(dat, offset + 4, &count);
    // Each omitted slot corresponds to a bit in the original selection mask.
    if (count > 32) return HSD_DAT_DESCRIPTOR;
    if (count > capacity) return HSD_DAT_CAPACITY;
    if (count) {
        if (!records || HSD_DatResolve(dat, offset, &records_offset, &external) != HSD_DAT_REF_LOCAL)
            return HSD_DAT_DESCRIPTOR;
        const u8* source = HSD_DatBytes(dat, records_offset, (size_t) count * 4);
        if (!source) return HSD_DAT_DESCRIPTOR;
        for (u32 i = 0; i < count; ++i) {
            if (source[i * 4] >= part_count) return HSD_DAT_DESCRIPTOR;
            records[i] = (struct Fighter_804D6540_x0_t) {
                source[i * 4], source[i * 4 + 1], source[i * 4 + 2], source[i * 4 + 3] };
        }
    }
    *output = (struct Fighter_804D6540_t) { count ? records : NULL, (int) count };
    return HSD_DAT_OK;
}

HSD_DatResult ftData_DecodeCommonAttrs(const HSD_Dat* dat, u32 offset,
                                      ftCo_DatAttrs* output)
{
    // This record has 96 four-byte scalar words (including Vec3 fields),
    // followed by one byte. Its native type contains no pointers.
    _Static_assert(sizeof(float) == 4 && sizeof(int) == 4 && sizeof(Vec3) == 12,
                   "Common fighter attributes require four-byte scalars");
    _Static_assert(offsetof(ftCo_DatAttrs, weight_independent_throws_mask) == 0x180,
                   "Common fighter scalar layout changed");
    ftCo_DatAttrs result = { 0 };
    const u8* source = HSD_DatBytes(dat, offset, 0x181);
    if (!output || !source) return HSD_DAT_DESCRIPTOR;
    for (u32 position = 0; position < 0x180; position += 4) {
        u32 word;
        HSD_DatReadU32(dat, offset + position, &word);
        // Integer fields retain their bit patterns. All other words are f32.
        if (position != 0x58 && position != 0x98 && position != 0xA0 &&
            position != 0xA4 && position != 0x16C) {
            float value;
            memcpy(&value, &word, sizeof(value));
            if (!isfinite(value)) return HSD_DAT_DESCRIPTOR;
        }
        memcpy((u8*) &result + position, &word, sizeof(word));
    }
    result.weight_independent_throws_mask = source[0x180];
    *output = result;
    return HSD_DAT_OK;
}
