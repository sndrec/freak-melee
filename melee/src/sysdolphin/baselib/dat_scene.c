#include "dat_import_internal.h"
#include <string.h>

void HSD_DatDecodeSceneRecord(HSD_DatImport* import,
                             const HSD_DatImportRecord* record)
{
    const HSD_Dat* dat = import->dat;
    u32 offset = record->offset;
    switch (record->type) {
    case DAT_LIGHT: {
        HSD_LightDesc* obj = record->object;
        obj->class_name = HSD_DatImportString(import, offset);
        obj->next = HSD_DatImportReference(import, offset + 4, DAT_LIGHT, 1);
        HSD_DatReadU16(dat, offset + 8, &obj->flags);
        HSD_DatReadU16(dat, offset + 10, &obj->attnflags);
        memcpy(&obj->color, dat->data + offset + 12, 4);
        obj->position = HSD_DatImportReference(import, offset + 16, DAT_WOBJ, 1);
        obj->interest = HSD_DatImportReference(import, offset + 20, DAT_WOBJ, 1);
        switch (obj->flags & LOBJ_TYPE_MASK) {
        case LOBJ_POINT:
            obj->u.p = HSD_DatImportReference(import, offset + 24,
                obj->attnflags & LOBJ_LIGHT_ATTN ? DAT_LIGHT_ATTN : DAT_LIGHT_POINT, 1);
            if (!obj->u.p && import->status == HSD_DAT_OK)
                import->status = HSD_DAT_DESCRIPTOR;
            break;
        case LOBJ_SPOT:
            obj->u.p = HSD_DatImportReference(import, offset + 24,
                obj->attnflags ? DAT_LIGHT_ATTN : DAT_LIGHT_SPOT, 1);
            if (!obj->u.p && import->status == HSD_DAT_OK)
                import->status = HSD_DAT_DESCRIPTOR;
            break;
        }
        break;
    }
    case DAT_LIGHT_POINT: {
        HSD_LightPointDesc* obj = record->object;
        HSD_DatReadF32(dat, offset, &obj->ref_br);
        HSD_DatReadF32(dat, offset + 4, &obj->ref_dist);
        HSD_DatReadU32(dat, offset + 8, &obj->dist_func);
        break;
    }
    case DAT_LIGHT_SPOT: {
        HSD_LightSpotDesc* obj = record->object;
        HSD_DatReadF32(dat, offset, &obj->cutoff);
        HSD_DatReadU32(dat, offset + 4, &obj->spot_func);
        HSD_DatReadF32(dat, offset + 8, &obj->ref_br);
        HSD_DatReadF32(dat, offset + 12, &obj->ref_dist);
        HSD_DatReadU32(dat, offset + 16, &obj->dist_func);
        break;
    }
    case DAT_LIGHT_ATTN: {
        HSD_LightAttn* obj = record->object;
        HSD_DatReadF32(dat, offset, &obj->a0);
        HSD_DatReadF32(dat, offset + 4, &obj->a1);
        HSD_DatReadF32(dat, offset + 8, &obj->a2);
        HSD_DatReadF32(dat, offset + 12, &obj->k0);
        HSD_DatReadF32(dat, offset + 16, &obj->k1);
        HSD_DatReadF32(dat, offset + 20, &obj->k2);
        break;
    }
    case DAT_CAMERA: {
        HSD_CObjDesc* obj = record->object;
        HSD_CameraDescCommon* c = &obj->common;
        c->class_name = HSD_DatImportString(import, offset);
        HSD_DatReadU16(dat, offset + 4, &c->flags);
        HSD_DatReadU16(dat, offset + 6, &c->projection_type);
        HSD_DatReadU16(dat, offset + 8, (u16*) &c->viewport.xmin);
        HSD_DatReadU16(dat, offset + 10, (u16*) &c->viewport.xmax);
        HSD_DatReadU16(dat, offset + 12, (u16*) &c->viewport.ymin);
        HSD_DatReadU16(dat, offset + 14, (u16*) &c->viewport.ymax);
        HSD_DatReadU16(dat, offset + 16, &c->scissor.left);
        HSD_DatReadU16(dat, offset + 18, &c->scissor.right);
        HSD_DatReadU16(dat, offset + 20, &c->scissor.top);
        HSD_DatReadU16(dat, offset + 22, &c->scissor.bottom);
        c->eyepos = HSD_DatImportReference(import, offset + 24, DAT_WOBJ, 1);
        c->interest = HSD_DatImportReference(import, offset + 28, DAT_WOBJ, 1);
        HSD_DatReadF32(dat, offset + 32, &c->roll);
        c->up_vector = HSD_DatImportReference(import, offset + 36, DAT_VECTOR, 1);
        HSD_DatReadF32(dat, offset + 40, &c->nnear);
        HSD_DatReadF32(dat, offset + 44, &c->ffar);
        if (c->projection_type == PROJ_PERSPECTIVE) {
            HSD_DatReadF32(dat, offset + 48, &obj->perspective.fov);
            HSD_DatReadF32(dat, offset + 52, &obj->perspective.aspect);
        } else if ((c->projection_type == PROJ_FRUSTUM ||
                    c->projection_type == PROJ_ORTHO) &&
                   HSD_DatBytes(dat, offset, 64)) {
            HSD_DatReadF32(dat, offset + 48, &obj->frustum.top);
            HSD_DatReadF32(dat, offset + 52, &obj->frustum.bottom);
            HSD_DatReadF32(dat, offset + 56, &obj->frustum.left);
            HSD_DatReadF32(dat, offset + 60, &obj->frustum.right);
        } else {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
        }
        break;
    }
    case DAT_WOBJ: {
        HSD_WObjDesc* obj = record->object;
        obj->class_name = HSD_DatImportString(import, offset);
        HSD_DatReadF32(dat, offset + 4, &obj->pos.x);
        HSD_DatReadF32(dat, offset + 8, &obj->pos.y);
        HSD_DatReadF32(dat, offset + 12, &obj->pos.z);
        obj->robjdesc = HSD_DatImportReference(import, offset + 16, DAT_ROBJ, 1);
        break;
    }
    case DAT_VECTOR: {
        Vec3* obj = record->object;
        HSD_DatReadF32(dat, offset, &obj->x);
        HSD_DatReadF32(dat, offset + 4, &obj->y);
        HSD_DatReadF32(dat, offset + 8, &obj->z);
        break;
    }
    case DAT_FOG: {
        HSD_FogDesc* obj = record->object;
        HSD_DatReadU32(dat, offset, &obj->type);
        obj->fogadjdesc = HSD_DatImportReference(import, offset + 4, DAT_FOG_ADJ, 1);
        HSD_DatReadF32(dat, offset + 8, &obj->start);
        HSD_DatReadF32(dat, offset + 12, &obj->end);
        memcpy(&obj->color, dat->data + offset + 16, 4);
        break;
    }
    case DAT_FOG_ADJ: {
        HSD_FogAdjDesc* obj = record->object;
        HSD_DatReadU16(dat, offset, &obj->center);
        HSD_DatReadU16(dat, offset + 2, &obj->width);
        for (u32 row = 0; row < 4; ++row)
            for (u32 column = 0; column < 4; ++column)
                HSD_DatReadF32(dat, offset + 4 + (row * 4 + column) * 4,
                               &obj->mtx[row][column]);
        break;
    }
    }
}
