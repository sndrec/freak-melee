#include "dat_import_internal.h"
#include "dat_texture.h"

#include <string.h>

void HSD_DatDecodeMaterialRecord(HSD_DatImport* import,
                                const HSD_DatImportRecord* record)
{
    const HSD_Dat* dat = import->dat;
    u32 offset = record->offset;
    u32 value;
    HSD_DatResult status = HSD_DAT_OK;
    switch (record->type) {
    case DAT_MOBJ: {
        HSD_MObjDesc* obj = record->object;
        u32 ignored;
        const char* external;
        obj->class_name = HSD_DatImportString(import, offset);
        HSD_DatReadU32(dat, offset + 4, &obj->rendermode);
        obj->texdesc = HSD_DatImportReference(import, offset + 8, DAT_TOBJ, 1);
        obj->mat = HSD_DatImportReference(import, offset + 12, DAT_MATERIAL, 1);
        // Sysdolphin's MObj loader never reads this legacy render descriptor.
        // Validate its relocation, but leave it out of the native descriptor.
        if (HSD_DatResolve(dat, offset + 16, &ignored, &external) ==
            HSD_DAT_REF_INVALID)
        {
            import->status = HSD_DAT_RELOCATION;
            import->error_offset = offset + 16;
        }
        obj->pedesc = HSD_DatImportReference(import, offset + 20, DAT_PE, 1);
        break;
    }
    case DAT_TOBJ: {
        HSD_TObjDesc* obj = record->object;
        const u8* bytes = HSD_DatBytes(dat, offset, 92);
        obj->class_name = HSD_DatImportString(import, offset);
        obj->next = HSD_DatImportReference(import, offset + 4, DAT_TOBJ, 1);
        HSD_DatReadU32(dat, offset + 8, &value); obj->id = (GXTexMapID) value;
        HSD_DatReadU32(dat, offset + 12, &value); obj->src = (GXTexGenSrc) value;
        HSD_DatReadF32(dat, offset + 16, &obj->rotate.x);
        HSD_DatReadF32(dat, offset + 20, &obj->rotate.y);
        HSD_DatReadF32(dat, offset + 24, &obj->rotate.z);
        HSD_DatReadF32(dat, offset + 28, &obj->scale.x);
        HSD_DatReadF32(dat, offset + 32, &obj->scale.y);
        HSD_DatReadF32(dat, offset + 36, &obj->scale.z);
        HSD_DatReadF32(dat, offset + 40, &obj->translate.x);
        HSD_DatReadF32(dat, offset + 44, &obj->translate.y);
        HSD_DatReadF32(dat, offset + 48, &obj->translate.z);
        HSD_DatReadU32(dat, offset + 52, &value); obj->wrap_s = (GXTexWrapMode) value;
        HSD_DatReadU32(dat, offset + 56, &value); obj->wrap_t = (GXTexWrapMode) value;
        obj->repeat_s = bytes[60]; obj->repeat_t = bytes[61];
        HSD_DatReadU32(dat, offset + 64, &obj->blend_flags);
        HSD_DatReadF32(dat, offset + 68, &obj->blending);
        HSD_DatReadU32(dat, offset + 72, &value); obj->magFilt = (GXTexFilter) value;
        obj->imagedesc = HSD_DatImportReference(import, offset + 76, DAT_IMAGE, 1);
        obj->tlutdesc = HSD_DatImportReference(import, offset + 80, DAT_PALETTE, 1);
        obj->lod = HSD_DatImportReference(import, offset + 84, DAT_LOD, 1);
        obj->tev = HSD_DatImportReference(import, offset + 88, DAT_TEV, 1);
        break;
    }
    case DAT_MATERIAL: {
        HSD_Material* obj = record->object;
        memcpy(&obj->ambient, HSD_DatBytes(dat, offset, 12), 12);
        HSD_DatReadF32(dat, offset + 12, &obj->alpha);
        HSD_DatReadF32(dat, offset + 16, &obj->shininess);
        break;
    }
    case DAT_PE:
        memcpy(record->object, HSD_DatBytes(dat, offset, 12), 12);
        break;
    case DAT_TEV: {
        HSD_TObjTevDesc* obj = record->object;
        memcpy(obj, HSD_DatBytes(dat, offset, 28), 28);
        HSD_DatReadU32(dat, offset + 28, &obj->active);
        break;
    }
    case DAT_IMAGE: {
        size_t bytes;
        HSD_ImageDesc* obj = record->object;
        status = HSD_DatDecodeImage(dat, offset, obj, &bytes);
        if (status == HSD_DAT_OK)
            HSD_DatImportSource(import, (u32)((const u8*)obj->image_ptr - dat->data), bytes, 0);
        break;
    }
    case DAT_PALETTE: {
        HSD_TlutDesc* obj = record->object;
        status = HSD_DatDecodePalette(dat, offset, obj);
        if (status == HSD_DAT_OK)
            HSD_DatImportSource(import, (u32)((const u8*)obj->lut - dat->data), (size_t)obj->n_entries * 2, 0);
        break;
    }
    case DAT_LOD:
        status = HSD_DatDecodeLOD(dat, offset, record->object);
        break;
    default:
        status = HSD_DAT_DESCRIPTOR;
        break;
    }
    if (import->status == HSD_DAT_OK && status != HSD_DAT_OK) {
        import->status = status;
        import->error_offset = offset;
    }
}
