#include "dat_import_internal.h"
#include <math.h>

// GX fetches attributes in hardware order, independent of descriptor order.
// This runs once during asset import, before any renderer can consume the bytes.
u32 HSD_DatVertexElementBytes(const HSD_VtxDescList* v)
{
    u32 count, scalar;
    if (v->attr <= GX_VA_TEX7MTXIDX) return 1;
    if (v->attr == GX_VA_CLR0 || v->attr == GX_VA_CLR1) {
        static const u8 sizes[] = { 2, 3, 4, 2, 3, 4 };
        return (u32) v->comp_cnt <= GX_CLR_RGBA && (u32) v->comp_type <= GX_RGBA8 ? sizes[v->comp_type] : 0;
    }
    switch (v->comp_type) {
    case GX_U8: case GX_S8: scalar = 1; break;
    case GX_U16: case GX_S16: scalar = 2; break;
    case GX_F32: scalar = 4; break;
    default: return 0;
    }
    if (v->attr == GX_VA_POS) {
        if ((u32) v->comp_cnt > GX_POS_XYZ) return 0;
        count = v->comp_cnt == GX_POS_XY ? 2 : 3;
    } else if (v->attr == GX_VA_NRM || v->attr == GX_VA_NBT) {
        if ((u32) v->comp_cnt > GX_NRM_NBT3) return 0;
        count = v->comp_cnt == GX_NRM_NBT ? 9 : 3;
    } else if (v->attr >= GX_VA_TEX0 && v->attr <= GX_VA_TEX7) {
        if ((u32) v->comp_cnt > GX_TEX_ST) return 0;
        count = v->comp_cnt == GX_TEX_S ? 1 : 2;
    } else return 0;
    return scalar * count;
}

bool HSD_DatValidateGeometry(HSD_DatImport* import, HSD_PObjDesc* polygon)
{
    if (pobj_type(polygon) == POBJ_SEPARATE_ENVELOPES &&
        !HSD_DatValidateEnvelopeMatrices(polygon)) return false;
    HSD_VtxDescList* ordered[GX_VA_TEX7 + 1] = { 0 };
    u32 sizes[GX_VA_TEX7 + 1] = { 0 };
    const u8* display = polygon->display;
    size_t length = (size_t) polygon->n_display * 32;
    size_t cursor = 0;
    const HSD_ShapeSetDesc* shape = (polygon->flags & 0x3000) == POBJ_SHAPEANIM ? polygon->u.shape_set : NULL;
    if(pobj_type(polygon)==POBJ_SHAPEANIM && !shape)return false;
    if (length && (!display || !polygon->verts)) return false;
    if (!polygon->verts) return true;
    for (HSD_VtxDescList* v = polygon->verts; v->attr != GX_VA_NULL; ++v) {
        u32 attr = v->attr == GX_VA_NBT ? GX_VA_NRM : v->attr;
        if (attr > GX_VA_TEX7 || ordered[attr] || (u32) v->attr_type > GX_INDEX16) return false;
        ordered[attr] = v;
        if (v->attr_type == GX_NONE) continue;
        sizes[attr] = HSD_DatVertexElementBytes(v);
        if (!sizes[attr] || (attr < GX_VA_POS && v->attr_type != GX_DIRECT)) return false;
        const bool generated = shape && (attr == GX_VA_POS || attr == GX_VA_NRM);
        if (generated) {
            // setupShapeAnimArrayDesc skips these arrays. Shape drawing reads
            // indices into the regenerated vertex/normal scratch prefixes.
            if (v->attr_type != GX_INDEX8 && v->attr_type != GX_INDEX16) return false;
        } else if (v->attr_type != GX_DIRECT && (!v->vertex || !v->stride || v->stride > 255)) return false;
    }
    while (cursor < length) {
        u8 command = display[cursor++];
        if (!command) continue; // GX NOP padding.
        // HSD configures VAT 0. Register writes and nested display lists are
        // not geometry payloads and must not bypass this decoder.
        if ((command & 7) || command < 0x80 || command > 0xB8 || command == 0x88 || length - cursor < 2) return false;
        u32 count = (u32) display[cursor] << 8 | display[cursor + 1];
        cursor += 2;
        for (u32 vertex = 0; vertex < count; ++vertex) {
            for (u32 attr = 0; attr <= GX_VA_TEX7; ++attr) {
                HSD_VtxDescList* v = ordered[attr];
                if (!v || v->attr_type == GX_NONE) continue;
                bool generated = shape && (attr == GX_VA_POS || attr == GX_VA_NRM);
                bool nbt3 = !generated && attr == GX_VA_NRM && v->comp_cnt == GX_NRM_NBT3;
                u32 repeats = nbt3 ? 3 : 1;
                u32 width = v->attr_type == GX_DIRECT ? sizes[attr] : v->attr_type == GX_INDEX8 ? 1 : 2;
                if (length - cursor < (size_t) width * repeats) return false;
                for (u32 part = 0; part < repeats; ++part) {
                    if (v->attr_type != GX_DIRECT) {
                        u32 index = display[cursor];
                        if (width == 2) index = index << 8 | display[cursor + 1];
                        if (generated) {
                            const s32 limit = attr == GX_VA_POS ? shape->nb_vertex_index : shape->nb_normal_index;
                            if (limit <= 0 || index >= (u32)limit) return false;
                        } else {
                            size_t extent = (size_t) index * v->stride + sizes[attr];
                            size_t base = (const u8*) v->vertex - import->dat->data;
                            if (base > import->dat->data_size || extent > import->dat->data_size - base) return false;
                            if (extent > v->vertex_size) v->vertex_size = (u32) extent;
                        }
                    }
                    cursor += width;
                }
            }
        }
    }
    for (u32 attr = 0; attr <= GX_VA_TEX7; ++attr) {
        const HSD_VtxDescList* v = ordered[attr];
        if (v && !(shape && (attr == GX_VA_POS || attr == GX_VA_NRM)) && v->attr_type > GX_DIRECT && v->vertex_size)
            HSD_DatImportSource(import, (u32)((const u8*)v->vertex - import->dat->data), v->vertex_size, 0);
    }
    return !shape || !(shape->flags&SHAPESET_SKIN) || HSD_DatValidateShapeSkin(import,polygon);
}

static bool shape_attribute(HSD_DatImport* import, HSD_VtxDescList* desc,
                              u8** indices, s32 count, u32 shapes, bool normal,bool wide)
{
    if (count == 0) return true;
    if (count < 0 || !desc || !indices || !desc->vertex || !desc->stride ||
        (desc->attr_type != GX_INDEX8 && desc->attr_type != GX_INDEX16)) return false;
    if(wide && desc->attr_type!=GX_INDEX16)return false;
    if (normal ? (desc->attr != GX_VA_NRM && desc->attr != GX_VA_NBT) : desc->attr != GX_VA_POS) return false;
    if ((u32)desc->comp_type > GX_F32 || desc->comp_cnt != (normal ? GX_NRM_XYZ : GX_POS_XYZ)) return false;
    const u32 scalar = desc->comp_type <= GX_S8 ? 1 : desc->comp_type <= GX_S16 ? 2 :
                       desc->comp_type == GX_F32 ? 4 : 0;
    if (!scalar) return false;
    const size_t width = (desc->attr == GX_VA_NBT ? 9 : 3) * scalar;
    size_t extent = 0;
    const size_t base = (const u8*)desc->vertex - import->dat->data;
    if (base > import->dat->data_size) return false;
    const unsigned index_bytes=wide?4:desc->attr_type==GX_INDEX8?1:2;
    for (u32 shape = 0; shape < shapes; ++shape) {
        if (!indices[shape]) return false;
        for (s32 i = 0; i < count; ++i) {
            const u8* p = indices[shape] + (size_t)i * index_bytes;
            const u32 index=wide?(u32)p[0]<<24|(u32)p[1]<<16|(u32)p[2]<<8|p[3]:
                index_bytes==1?p[0]:(u32)p[0]<<8|p[1];
            const size_t end = (size_t)index * desc->stride + width;
            if (end > import->dat->data_size - base) return false;
            if (end > extent) extent = end;
        }
    }
    HSD_DatImportSource(import, (u32)base, extent, 0);
    return true;
}

bool HSD_DatValidateShape(HSD_DatImport* import, HSD_ShapeSetDesc* shape)
{
    const unsigned mode=shape->flags&3;
    if ((shape->flags&SHAPESET_SIGNED_WEIGHTS) && mode!=SHAPESET_ADDITIVE) return false;
    if (shape->default_blend) {
        const unsigned count=mode==SHAPESET_ADDITIVE?shape->nb_shape:1;
        for(unsigned i=0;i<count;++i)if(!isfinite(shape->default_blend[i]))return false;
    }
    if((mode!=SHAPESET_AVERAGE && mode!=SHAPESET_ADDITIVE) ||
       shape->nb_vertex_index<0 || shape->nb_vertex_index>HSD_MAX_SHAPE_VERTICES ||
       shape->nb_normal_index<0 || shape->nb_normal_index>HSD_MAX_SHAPE_NORMALS)return false;
    const u32 count = shape->nb_shape + ((shape->flags & SHAPESET_ADDITIVE) != 0);
    return shape_attribute(import, shape->vertex_desc, shape->vertex_idx_list,
                              shape->nb_vertex_index, count, false,(shape->flags&SHAPESET_INDEX32)!=0) &&
           shape_attribute(import, shape->normal_desc, shape->normal_idx_list,
                              shape->nb_normal_index, count, true,(shape->flags&SHAPESET_INDEX32)!=0);
}
