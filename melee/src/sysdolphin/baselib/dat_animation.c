#include "dat_import_internal.h"
#include "native_fobj_stream.h"

// Include repeated references in the work budget even when they share bytes.
static bool inspect_track(HSD_DatImport* import, const HSD_FObjDesc* track, HSD_TrackFacts* facts)
{
    if (track->length > 64u * 1024 * 1024 - import->animation_stream_bytes) return false;
    import->animation_stream_bytes += track->length;
    return HSD_TrackInspect(track->ad, track->length, track->frac_value,
                            track->frac_slope, 131072, facts) == HSD_TRACK_VALID;
}

bool HSD_DatBindTextureAnimation(HSD_DatImport* import, HSD_TexAnim* anim)
{
    if (!anim->aobjdesc) return true;
    unsigned count = 0;
    bool extend_images = false, extend_palettes = false, selects_images = false;
    for (const HSD_FObjDesc* track = anim->aobjdesc->fobjdesc; track; track = track->next) {
        if (++count > 256) return false;
        if (track->type >= HSD_A_T_KONST_R && track->type <= HSD_A_T_TEV1_A)
            anim->needs_tev = true;
        bool palette = track->type == HSD_A_T_TCLT;
        if (track->type != HSD_A_T_TIMG && !palette) continue;
        // A texture without animated palettes ignores the palette channel.
        if (palette && !anim->n_tluttbl) continue;
        if (!palette) selects_images = true;
        HSD_TrackFacts facts;
        unsigned entries = palette ? anim->n_tluttbl : anim->n_imagetbl;
        if (palette && entries > 255) entries = 255;
        if (!inspect_track(import, track, &facts)) return false;
        if (HSD_TrackFitsTable(&facts, entries, palette)) continue;
        // Stock stage select holds an index equal to its table length on the
        // final frame. Give that index a real entry holding the last image.
        if (!entries || entries == UINT16_MAX ||
            !HSD_TrackFitsTable(&facts, entries + 1, palette)) return false;
        if (palette) extend_palettes = true;
        else extend_images = true;
    }
    if (selects_images) {
        u32 bytes = anim->n_imagetbl * 4u;
        if (bytes > 64u * 1024 * 1024 - import->animation_stream_bytes) return false;
        import->animation_stream_bytes += bytes;
        for (unsigned i = 0; i < anim->n_imagetbl; ++i) {
            const HSD_ImageDesc* image = anim->imagetbl[i];
            if (image && (image->format == GX_TF_C4 || image->format == GX_TF_C8 ||
                          image->format == GX_TF_C14X2)) {
                anim->selects_indexed_images = true;
                break;
            }
        }
    }
    if (extend_images) {
        unsigned n = anim->n_imagetbl;
        HSD_ImageDesc** table = HSD_DatImportAlloc(import, (n + 1) * sizeof(*table));
        if (!table) return false;
        memcpy(table, anim->imagetbl, n * sizeof(*table));
        table[n] = table[n - 1];
        anim->imagetbl = table;
        ++anim->n_imagetbl;
    }
    if (extend_palettes) {
        unsigned n = anim->n_tluttbl;
        HSD_TlutDesc** table = HSD_DatImportAlloc(import, (n + 1) * sizeof(*table));
        if (!table || !anim->tluttbl[n - 1]) return false;
        memcpy(table, anim->tluttbl, n * sizeof(*table));
        table[n] = table[n - 1];
        anim->tluttbl = table;
        ++anim->n_tluttbl;
    }
    return true;
}

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
        if (!isfinite(obj->end_frame) || obj->end_frame < 0) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset + 4;
            break;
        }
        obj->fobjdesc = HSD_DatImportReference(import, offset + 8, DAT_FOBJ, 1);
        kind = HSD_DatResolve(dat, offset + 12, &obj_id, &external);
        obj->obj_id_is_joint = kind == HSD_DAT_REF_LOCAL;
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
        HSD_TrackFacts facts;
        if (import->status == HSD_DAT_OK &&
            (!isfinite(obj->startframe) || obj->startframe <= -32769.0f || obj->startframe >= 32768.0f ||
             !inspect_track(import, obj, &facts))) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
        }
        break;
    }
    case DAT_IMAGE_TABLE:
    case DAT_PALETTE_TABLE: {
        void** table = record->object;
        u32 type = record->type == DAT_IMAGE_TABLE ? DAT_IMAGE : DAT_PALETTE;
        for (u32 i = 0; i < record->count && import->status == HSD_DAT_OK; ++i) {
            table[i] = HSD_DatImportReference(import, offset + i * 4, type, 1);
            // The runtime uses the first null palette to end its owned list.
            if (type == DAT_PALETTE && !table[i] && import->status == HSD_DAT_OK) {
                import->status = HSD_DAT_DESCRIPTOR;
                import->error_offset = offset + i * 4;
            }
        }
        break;
    }
    default:
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = offset;
        break;
    }
}
