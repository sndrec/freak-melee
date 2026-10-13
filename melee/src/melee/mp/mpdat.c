#include "mpdat.h"
#include <math.h>
#include <limits.h>

static s16 read_s16(const HSD_Dat* dat, u32 offset)
{
    u16 value;
    HSD_DatReadU16(dat, offset, &value);
    return (s16) value;
}

static bool valid_range(s16 start, s16 count, u32 total)
{
    return count >= 0 && (count == 0 ||
        (start >= 0 && (u32) start <= total && (u32) count <= total - start));
}

static bool valid_link(s16 link, u32 count)
{
    return link == -1 || (link >= 0 && (u32) link < count);
}

HSD_DatResult mpDecodeCollision(const HSD_Dat* dat, u32 offset,
    MapCollData* output, Vec2* vertices, size_t vertex_capacity,
    MapLine* lines, size_t line_capacity, MapJoint* joints, size_t joint_capacity)
{
    MapCollData result = { 0 };
    u32 nv, nl, nj, vo, lo, jo, unused;
    const char* external;
    if (!output || !HSD_DatBytes(dat, offset, 48))
        return HSD_DAT_DESCRIPTOR;
    HSD_DatReadU32(dat, offset + 4, &nv);
    HSD_DatReadU32(dat, offset + 12, &nl);
    HSD_DatReadU32(dat, offset + 40, &nj);
    if (!nv || !nl || !nj || nv > INT_MAX || nl > INT_MAX || nj > INT_MAX)
        return HSD_DAT_DESCRIPTOR;
    if (HSD_DatResolve(dat, offset, &vo, &external) != HSD_DAT_REF_LOCAL ||
        HSD_DatResolve(dat, offset + 8, &lo, &external) != HSD_DAT_REF_LOCAL ||
        HSD_DatResolve(dat, offset + 36, &jo, &external) != HSD_DAT_REF_LOCAL ||
        !HSD_DatBytes(dat, vo, (size_t) nv * 8) ||
        !HSD_DatBytes(dat, lo, (size_t) nl * 16) ||
        !HSD_DatBytes(dat, jo, (size_t) nj * 40))
        return HSD_DAT_DESCRIPTOR;
    if (nv > vertex_capacity || nl > line_capacity || nj > joint_capacity)
        return HSD_DAT_CAPACITY;
    if (!vertices || !lines || !joints) return HSD_DAT_DESCRIPTOR;
    result.verts = vertices; result.vert_count = nv;
    result.lines = lines; result.line_count = nl;
    result.joints = joints; result.joint_count = nj;
    HSD_DatReadU32(dat, offset + 44, &unused);
    result.x2C = (int) unused;
    result.floor_start = read_s16(dat, offset + 16);
    result.floor_count = read_s16(dat, offset + 18);
    if (!valid_range(result.floor_start, result.floor_count, nl))
        return HSD_DAT_DESCRIPTOR;
    result.ceiling_start = read_s16(dat, offset + 20);
    result.ceiling_count = read_s16(dat, offset + 22);
    if (!valid_range(result.ceiling_start, result.ceiling_count, nl))
        return HSD_DAT_DESCRIPTOR;
    result.right_wall_start = read_s16(dat, offset + 24);
    result.right_wall_count = read_s16(dat, offset + 26);
    if (!valid_range(result.right_wall_start, result.right_wall_count, nl))
        return HSD_DAT_DESCRIPTOR;
    result.left_wall_start = read_s16(dat, offset + 28);
    result.left_wall_count = read_s16(dat, offset + 30);
    if (!valid_range(result.left_wall_start, result.left_wall_count, nl))
        return HSD_DAT_DESCRIPTOR;
    result.dynamic_start = read_s16(dat, offset + 32);
    result.dynamic_count = read_s16(dat, offset + 34);
    if (!valid_range(result.dynamic_start, result.dynamic_count, nl))
        return HSD_DAT_DESCRIPTOR;
    for (u32 i = 0; i < nv; ++i) {
        HSD_DatReadF32(dat, vo + i * 8, &vertices[i].x);
        HSD_DatReadF32(dat, vo + i * 8 + 4, &vertices[i].y);
        if (!isfinite(vertices[i].x) || !isfinite(vertices[i].y))
            return HSD_DAT_DESCRIPTOR;
    }
    for (u32 i = 0; i < nl; ++i) {
        MapLine* line = &lines[i];
        u32 at = lo + i * 16;
        HSD_DatReadU16(dat, at, &line->v0_idx);
        HSD_DatReadU16(dat, at + 2, &line->v1_idx);
        if (line->v0_idx >= nv || line->v1_idx >= nv)
            return HSD_DAT_DESCRIPTOR;
        line->prev_id0 = read_s16(dat, at + 4);
        if (!valid_link(line->prev_id0, nl)) return HSD_DAT_DESCRIPTOR;
        line->next_id0 = read_s16(dat, at + 6);
        if (!valid_link(line->next_id0, nl)) return HSD_DAT_DESCRIPTOR;
        line->prev_id1 = read_s16(dat, at + 8);
        if (!valid_link(line->prev_id1, nl)) return HSD_DAT_DESCRIPTOR;
        line->next_id1 = read_s16(dat, at + 10);
        if (!valid_link(line->next_id1, nl)) return HSD_DAT_DESCRIPTOR;
        HSD_DatReadU16(dat, at + 12, &line->hi_flags);
        HSD_DatReadU16(dat, at + 14, &line->lo_flags);
    }
    for (u32 i = 0; i < nj; ++i) {
        MapJoint* joint = &joints[i];
        u32 at = jo + i * 40;
        joint->floor_start = read_s16(dat, at + 0);
        joint->floor_count = read_s16(dat, at + 2);
        if (!valid_range(joint->floor_start, joint->floor_count, nl))
            return HSD_DAT_DESCRIPTOR;
        joint->ceiling_start = read_s16(dat, at + 4);
        joint->ceiling_count = read_s16(dat, at + 6);
        if (!valid_range(joint->ceiling_start, joint->ceiling_count, nl))
            return HSD_DAT_DESCRIPTOR;
        joint->right_wall_start = read_s16(dat, at + 8);
        joint->right_wall_count = read_s16(dat, at + 10);
        if (!valid_range(joint->right_wall_start, joint->right_wall_count, nl))
            return HSD_DAT_DESCRIPTOR;
        joint->left_wall_start = read_s16(dat, at + 12);
        joint->left_wall_count = read_s16(dat, at + 14);
        if (!valid_range(joint->left_wall_start, joint->left_wall_count, nl))
            return HSD_DAT_DESCRIPTOR;
        joint->dynamic_start = read_s16(dat, at + 16);
        joint->dynamic_count = read_s16(dat, at + 18);
        if (!valid_range(joint->dynamic_start, joint->dynamic_count, nl))
            return HSD_DAT_DESCRIPTOR;
        HSD_DatReadF32(dat, at + 20, &joint->left_bound);
        if (!isfinite(joint->left_bound)) return HSD_DAT_DESCRIPTOR;
        HSD_DatReadF32(dat, at + 24, &joint->bottom_bound);
        if (!isfinite(joint->bottom_bound)) return HSD_DAT_DESCRIPTOR;
        HSD_DatReadF32(dat, at + 28, &joint->right_bound);
        if (!isfinite(joint->right_bound)) return HSD_DAT_DESCRIPTOR;
        HSD_DatReadF32(dat, at + 32, &joint->top_bound);
        if (!isfinite(joint->top_bound)) return HSD_DAT_DESCRIPTOR;
        joint->vtx_start = read_s16(dat, at + 36);
        joint->vtx_count = read_s16(dat, at + 38);
        if (!valid_range(joint->vtx_start, joint->vtx_count, nv))
            return HSD_DAT_DESCRIPTOR;
    }
    *output = result;
    return HSD_DAT_OK;
}

