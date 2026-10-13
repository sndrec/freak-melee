#include "dat_import_internal.h"

void HSD_DatDecodeAnimationRecord(HSD_DatImport* import,
                                 const HSD_DatImportRecord* record)
{
    const HSD_Dat* dat = import->dat;
    u32 offset = record->offset;
    switch (record->type) {
    case DAT_LIGHT_ANIM: {
        HSD_LightAnim* obj = record->object;
        obj->next = HSD_DatImportReference(import, offset, DAT_LIGHT_ANIM, 1);
        obj->aobjdesc = HSD_DatImportReference(import, offset + 4, DAT_AOBJ, 1);
        obj->position_anim = HSD_DatImportReference(import, offset + 8, DAT_WOBJ_ANIM, 1);
        obj->interest_anim = HSD_DatImportReference(import, offset + 12, DAT_WOBJ_ANIM, 1);
        break;
    }
    case DAT_WOBJ_ANIM: {
        HSD_WObjAnim* obj = record->object;
        obj->aobjdesc = HSD_DatImportReference(import, offset, DAT_AOBJ, 1);
        obj->robjanim = HSD_DatImportReference(import, offset + 4, DAT_ROBJ_ANIM, 1);
        break;
    }
    case DAT_LIGHT_ANIM_TABLE: {
        HSD_LightAnim** table = record->object;
        for (u32 i = 0; i < record->count; ++i)
            table[i] = HSD_DatImportReference(import, offset + i * 4, DAT_LIGHT_ANIM, 1);
        break;
    }
    case DAT_SHAPE_JOINT: {
        HSD_ShapeAnimJoint* obj = record->object;
        obj->child = HSD_DatImportReference(import, offset, DAT_SHAPE_JOINT, 1);
        obj->next = HSD_DatImportReference(import, offset + 4, DAT_SHAPE_JOINT, 1);
        obj->shapeanimdobj = HSD_DatImportReference(import, offset + 8, DAT_SHAPE_DOBJ, 1);
        break;
    }
    case DAT_SHAPE_DOBJ: {
        HSD_ShapeAnimDObj* obj = record->object;
        obj->next = HSD_DatImportReference(import, offset, DAT_SHAPE_DOBJ, 1);
        obj->shapeanim = HSD_DatImportReference(import, offset + 4, DAT_SHAPE_ANIM, 1);
        break;
    }
    case DAT_SHAPE_ANIM: {
        HSD_ShapeAnim* obj = record->object;
        obj->next = HSD_DatImportReference(import, offset, DAT_SHAPE_ANIM, 1);
        obj->aobjdesc = HSD_DatImportReference(import, offset + 4, DAT_AOBJ, 1);
        break;
    }
    case DAT_MATANIM_JOINT: {
        HSD_MatAnimJoint* obj = record->object;
        obj->child = HSD_DatImportReference(import, offset, DAT_MATANIM_JOINT, 1);
        obj->next = HSD_DatImportReference(import, offset + 4, DAT_MATANIM_JOINT, 1);
        obj->matanim = HSD_DatImportReference(import, offset + 8, DAT_MATANIM, 1);
        break;
    }
    case DAT_MATANIM: {
        HSD_MatAnim* obj = record->object;
        u32 ignored;
        const char* external;
        obj->next = HSD_DatImportReference(import, offset, DAT_MATANIM, 1);
        obj->aobjdesc = HSD_DatImportReference(import, offset + 4, DAT_AOBJ, 1);
        obj->texanim = HSD_DatImportReference(import, offset + 8, DAT_TEXANIM, 1);
        // HSD_MObjAddAnim never reads the legacy render-animation field.
        if (HSD_DatResolve(dat, offset + 12, &ignored, &external) ==
            HSD_DAT_REF_INVALID)
        {
            import->status = HSD_DAT_RELOCATION;
            import->error_offset = offset + 12;
        }
        break;
    }
    case DAT_TEXANIM: {
        HSD_TexAnim* obj = record->object;
        u32 value;
        obj->next = HSD_DatImportReference(import, offset, DAT_TEXANIM, 1);
        HSD_DatReadU32(dat, offset + 4, &value); obj->id = (GXTexMapID) value;
        obj->aobjdesc = HSD_DatImportReference(import, offset + 8, DAT_AOBJ, 1);
        HSD_DatReadU16(dat, offset + 20, &obj->n_imagetbl);
        HSD_DatReadU16(dat, offset + 22, &obj->n_tluttbl);
        obj->imagetbl = HSD_DatImportReference(import, offset + 12, DAT_IMAGE_TABLE, obj->n_imagetbl);
        obj->tluttbl = HSD_DatImportReference(import, offset + 16, DAT_PALETTE_TABLE, obj->n_tluttbl);
        if (import->status == HSD_DAT_OK &&
            ((obj->n_imagetbl && obj->imagetbl == NULL) ||
             (obj->n_tluttbl && obj->tluttbl == NULL))) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
        }
        break;
    }
    case DAT_AOBJ: {
        HSD_AObjDesc* obj = record->object;
        const char* external;
        u32 obj_id;
        HSD_DatReferenceKind kind;
        HSD_DatReadU32(dat, offset, &obj->flags);
        HSD_DatReadF32(dat, offset + 4, &obj->end_frame);
        obj->fobjdesc = HSD_DatImportReference(import, offset + 8, DAT_FOBJ, 1);
        kind = HSD_DatResolve(dat, offset + 12, &obj_id, &external);
        if (kind == HSD_DAT_REF_LOCAL) {
            obj->obj_id = (uintptr_t) HSD_DatImportObject(
                import, obj_id, DAT_JOINT, 1);
        } else if (kind == HSD_DAT_REF_NULL) {
            obj->obj_id = 0;
        } else if (kind == HSD_DAT_REF_INVALID) {
            // AObj IDs are packed integers unless the DAT relocation table
            // explicitly marks the field as an embedded Joint descriptor.
            obj->obj_id = obj_id;
        } else {
            import->status = HSD_DAT_EXTERNAL;
            import->error_offset = offset + 12;
            import->external_symbol = external;
        }
        break;
    }
    case DAT_FOBJ: {
        HSD_FObjDesc* obj = record->object;
        const u8* bytes = HSD_DatBytes(dat, offset, 20);
        obj->next = HSD_DatImportReference(import, offset, DAT_FOBJ, 1);
        HSD_DatReadU32(dat, offset + 4, &obj->length);
        HSD_DatReadF32(dat, offset + 8, &obj->startframe);
        obj->type = bytes[12]; obj->frac_value = bytes[13];
        obj->frac_slope = bytes[14]; obj->dummy0 = bytes[15];
        // FObj's compressed stream has its own encoding (including little-endian
        // scalar values). Preserve it for the original interpreter, not DAT swap.
        obj->ad = HSD_DatImportBytes(import, offset + 16, obj->length);
        break;
    }
    case DAT_IMAGE_TABLE:
    case DAT_PALETTE_TABLE: {
        void** table = record->object;
        u32 type = record->type == DAT_IMAGE_TABLE ? DAT_IMAGE : DAT_PALETTE;
        for (u32 i = 0; i < record->count && import->status == HSD_DAT_OK; ++i) {
            table[i] = HSD_DatImportReference(import, offset + i * 4, type, 1);
        }
        break;
    }
    default:
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = offset;
        break;
    }
}
