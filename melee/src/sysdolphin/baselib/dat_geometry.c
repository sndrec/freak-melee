#include "dat_import_internal.h"

// GX fetches attributes in hardware order, independent of descriptor order.
// This runs once during asset import, before any renderer can consume the bytes.
static u32 element_size(const HSD_VtxDescList* v)
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
    HSD_VtxDescList* ordered[GX_VA_TEX7 + 1] = { 0 };
    u32 sizes[GX_VA_TEX7 + 1] = { 0 };
    const u8* display = polygon->display;
    size_t length = (size_t) polygon->n_display * 32;
    size_t cursor = 0;
    if (length && (!display || !polygon->verts)) return false;
    if (!polygon->verts) return true;
    for (HSD_VtxDescList* v = polygon->verts; v->attr != GX_VA_NULL; ++v) {
        u32 attr = v->attr == GX_VA_NBT ? GX_VA_NRM : v->attr;
        if (attr > GX_VA_TEX7 || ordered[attr] || (u32) v->attr_type > GX_INDEX16) return false;
        ordered[attr] = v;
        if (v->attr_type == GX_NONE) continue;
        sizes[attr] = element_size(v);
        if (!sizes[attr] || (attr < GX_VA_POS && v->attr_type != GX_DIRECT)) return false;
        if (v->attr_type != GX_DIRECT && (!v->vertex || !v->stride || v->stride > 255)) return false;
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
                bool nbt3 = attr == GX_VA_NRM && v->comp_cnt == GX_NRM_NBT3;
                u32 repeats = nbt3 ? 3 : 1;
                u32 width = v->attr_type == GX_DIRECT ? sizes[attr] : v->attr_type == GX_INDEX8 ? 1 : 2;
                if (length - cursor < (size_t) width * repeats) return false;
                for (u32 part = 0; part < repeats; ++part) {
                    if (v->attr_type != GX_DIRECT) {
                        u32 index = display[cursor];
                        if (width == 2) index = index << 8 | display[cursor + 1];
                        size_t extent = (size_t) index * v->stride + sizes[attr];
                        size_t base = (const u8*) v->vertex - import->dat->data;
                        if (base > import->dat->data_size || extent > import->dat->data_size - base) return false;
                        if (extent > v->vertex_size) v->vertex_size = (u32) extent;
                    }
                    cursor += width;
                }
            }
        }
    }
    return true;
}
