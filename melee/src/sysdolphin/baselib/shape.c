#include "shape.h"
#include "debug.h"
#include <math.h>
#include <string.h>

static inline u16 shape_read_u16(const HSD_VtxDescList* desc, const u8* src)
{
#if defined(MELEE_NATIVE)
    if (!desc->vertex_little_endian) {
        return (u16) src[0] << 8 | src[1];
    }
#endif
    u16 value;
    memcpy(&value, src, sizeof(value));
    return value;
}

static inline f32 shape_read_f32(const HSD_VtxDescList* desc, const u8* src)
{
#if defined(MELEE_NATIVE)
    if (!desc->vertex_little_endian) {
        u32 bits = (u32) src[0] << 24 | (u32) src[1] << 16 |
                   (u32) src[2] << 8 | src[3];
        f32 value;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }
#endif
    f32 value;
    memcpy(&value, src, sizeof(value));
    return value;
}

static void decode_shape_components(const HSD_VtxDescList* desc,
                                    const void* source, f32* destination,
                                    u32 count)
{
    const u8* src = source;
    const f32 scale = ldexpf(1.0f, desc->frac);
    for (u32 i = 0; i < count; ++i) {
        switch (desc->comp_type) {
        case GX_U8:
            destination[i] = (f32) src[i] / scale;
            break;
        case GX_S8:
            destination[i] = (f32) ((const s8*) src)[i] / scale;
            break;
        case GX_U16:
            destination[i] =
                (f32) shape_read_u16(desc, src + i * 2) / scale;
            break;
        case GX_S16:
            destination[i] =
                (f32) (s16) shape_read_u16(desc, src + i * 2) / scale;
            break;
        case GX_F32:
            destination[i] = shape_read_f32(desc, src + i * 4);
            break;
        default:
            HSD_Panic(__FILE__, 0, "unexpected shape component type.\n");
        }
    }
}

void HSD_ShapeSetReadVector(const HSD_VtxDescList* desc, const u8* indices,
                            unsigned vertex, unsigned components, f32 output[9])
{
    const unsigned index = desc->attr_type==GX_INDEX16
        ? (unsigned)indices[vertex*2] << 8 | indices[vertex*2+1] : indices[vertex];
    decode_shape_components(desc, (const u8*)desc->vertex + index*desc->stride,
                            output, components);
}

static void evaluate_attribute(const HSD_ShapeSet* shape, bool normal,
                                f32 (*output)[3])
{
    const unsigned count = normal ? shape->nb_normal_index : shape->nb_vertex_index;
    if (!count || !output) return;
    const HSD_VtxDescList* desc = normal ? shape->normal_desc : shape->vertex_desc;
    u8* const* indices = normal ? shape->normal_idx_list : shape->vertex_idx_list;
    const unsigned vectors = normal && desc->attr == GX_VA_NBT ? 3 : 1;
    if (shape->flags & SHAPESET_AVERAGE) {
        const float value = shape->blend.bl;
        const unsigned first = value<=0 ? 0 : value>=shape->nb_shape-1 ? shape->nb_shape-1 : (unsigned)value;
        const unsigned next = MIN(first+1, shape->nb_shape-1);
        const float fraction = MIN(MAX(0.0f, value-(f32)first), 1.0f);
        for (unsigned i=0;i<count;++i) {
            float a[9],b[9];
            HSD_ShapeSetReadVector(desc,indices[first],i,vectors*3,a);
            HSD_ShapeSetReadVector(desc,indices[next],i,vectors*3,b);
            for(unsigned c=0;c<vectors*3;++c)
                output[i*vectors+c/3][c%3]=(b[c]-a[c])*fraction+a[c];
        }
    } else {
        for(unsigned i=0;i<count;++i) {
            float base[9];
            HSD_ShapeSetReadVector(desc,indices[0],i,vectors*3,base);
            for(unsigned c=0;c<vectors*3;++c)output[i*vectors+c/3][c%3]=base[c];
            for(unsigned target=0;target<shape->nb_shape;++target) {
                const float weight=shape->flags&SHAPESET_SIGNED_WEIGHTS ? shape->blend.bp[target] : MAX(0.0f,shape->blend.bp[target]);
                float offset[9];HSD_ShapeSetReadVector(desc,indices[target+1],i,vectors*3,offset);
                for(unsigned c=0;c<vectors*3;++c)
                    output[i*vectors+c/3][c%3]+=offset[c]*weight;
            }
        }
    }
}

bool HSD_ShapeSetEvaluate(const HSD_ShapeSet* shape, f32 (*positions)[3], f32 (*normals)[3])
{
    if(!shape || !positions || !shape->nb_shape ||
       ((shape->flags&3)!=SHAPESET_AVERAGE && (shape->flags&3)!=SHAPESET_ADDITIVE) ||
       (shape->nb_vertex_index && (!shape->vertex_desc || !shape->vertex_idx_list)) ||
       (normals && shape->nb_normal_index && (!shape->normal_desc || !shape->normal_idx_list)) ||
       shape->nb_vertex_index<0 || shape->nb_vertex_index>HSD_MAX_SHAPE_VERTICES ||
       shape->nb_normal_index<0 || shape->nb_normal_index>HSD_MAX_SHAPE_NORMALS ||
       (!(shape->flags&SHAPESET_AVERAGE) && !shape->blend.bp))return false;
    evaluate_attribute(shape,false,positions);
    evaluate_attribute(shape,true,normals);
    return true;
}

