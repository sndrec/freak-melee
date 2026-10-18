#include "dat_import_internal.h"
#include <string.h>

static HSD_RvalueList* import_rvalue_list(HSD_DatImport* import, u32 slot)
{
    const char* external;
    u32 source;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &source, &external);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        import->status = kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                       : HSD_DAT_RELOCATION;
        import->error_offset = slot;
        import->external_symbol = external;
        return NULL;
    }
    u32 count = 0;
    for (; count < 256; ++count) {
        u32 ignored;
        kind = HSD_DatResolve(import->dat, source + count * 8 + 4,
                              &ignored, &external);
        if (kind == HSD_DAT_REF_NULL) {
            break;
        }
        if (kind != HSD_DAT_REF_LOCAL) {
            import->status = kind == HSD_DAT_REF_EXTERNAL
                                 ? HSD_DAT_EXTERNAL
                                 : HSD_DAT_RELOCATION;
            import->error_offset = source + count * 8 + 4;
            import->external_symbol = external;
            return NULL;
        }
    }
    if (count == 256) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = source;
        return NULL;
    }
    HSD_RvalueList* output = HSD_DatImportAlloc(
        import, (size_t) (count + 1) * sizeof(*output));
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    HSD_DatImportSource(import, source, (size_t)(count + 1) * 8, 0);
    for (u32 i = 0; i < count; ++i) {
        u32 joint;
        HSD_DatReadU32(import->dat, source + i * 8, &output[i].flags);
        HSD_DatResolve(import->dat, source + i * 8 + 4, &joint, &external);
        output[i].joint = HSD_DatImportObject(import, joint, DAT_JOINT, 1);
        if (output[i].joint == NULL) {
            return NULL;
        }
    }
    return output;
}

static u8* import_bytecode(HSD_DatImport* import, u32 slot)
{
    const char* external;
    u32 source;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &source, &external);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        import->status = kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                       : HSD_DAT_RELOCATION;
        import->error_offset = slot;
        import->external_symbol = external;
        return NULL;
    }
    u32 end = HSD_DatNextTarget(import->dat, source);
    size_t size = end - source;
    if (size == 0 || size > 64 * 1024 ||
        HSD_DatBytes(import->dat, source, size) == NULL)
    {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = source;
        return NULL;
    }
    const u8* input = HSD_DatBytes(import->dat, source, size);
    size_t used = 0;
    while (used < size) {
        u8 command = input[used++];
        size_t operand_size = 0;
        if (command == 1) {
            break;
        }
        if (command == 5 || command == 0x3C || command == 0xFF) {
            operand_size = 1;
        } else if (command >= 2 && command <= 4) {
            operand_size = 2;
        } else if (command == 6) {
            operand_size = 4;
        } else if (command > 0x3C) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = source + (u32) used - 1;
            return NULL;
        }
        if (operand_size > size - used) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = source + (u32) used - 1;
            return NULL;
        }
        used += operand_size;
    }
    if (used == 0 || input[used - 1] != 1) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = source;
        return NULL;
    }
    u8* output = HSD_DatImportAlloc(import, size);
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    memcpy(output, input, size);
    HSD_DatImportSource(import, source, size, 0);
    return output;
}

static f32* import_float_array(HSD_DatImport* import, u32 slot, u32 count,
                               bool required)
{
    const char* external;
    u32 offset;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &offset, &external);
    if (kind == HSD_DAT_REF_NULL && !required) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL || count == 0 ||
        count > import->dat->data_size / sizeof(f32))
    {
        import->status = kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                       : HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
        import->external_symbol = external;
        return NULL;
    }
    f32* output = HSD_DatImportAlloc(import, (size_t) count * sizeof(*output));
    if (output == NULL) {
        import->error_offset = slot;
        return NULL;
    }
    for (u32 i = 0; i < count; ++i) {
        if (!HSD_DatReadF32(import->dat, offset + i * 4, &output[i])) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = slot;
            return NULL;
        }
    }
    HSD_DatImportSource(import, offset, (size_t)count * sizeof(f32), 0);
    return output;
}

static u8** import_shape_index_table(HSD_DatImport* import, u32 slot,
                                     u32 shape_count, u32 index_count,
                                     GXAttrType index_type)
{
    u32 table_offset;
    const char* external;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &table_offset, &external);
    if (kind == HSD_DAT_REF_NULL) {
        if (index_count == 0) {
            return NULL;
        }
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL ||
        HSD_DatBytes(import->dat, table_offset, (size_t) shape_count * 4) == NULL)
    {
        import->status = kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                       : HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
        import->external_symbol = external;
        return NULL;
    }
    if (index_type != GX_INDEX8 && index_type != GX_INDEX16) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
        return NULL;
    }
    HSD_DatImportSource(import, table_offset, (size_t)shape_count * 4, 0);
    u8** table = HSD_DatImportAlloc(import, (size_t) shape_count * sizeof(*table));
    if (table == NULL) {
        import->error_offset = slot;
        return NULL;
    }
    size_t byte_count = (size_t) index_count *
                        (index_type == GX_INDEX16 ? 2 : 1);
    for (u32 i = 0; i < shape_count; ++i) {
        u32 data_offset;
        kind = HSD_DatResolve(import->dat, table_offset + i * 4, &data_offset,
                              &external);
        if (kind != HSD_DAT_REF_LOCAL ||
            (table[i] = (u8*) HSD_DatBytes(import->dat, data_offset,
                                           byte_count)) == NULL)
        {
            import->status = kind == HSD_DAT_REF_EXTERNAL
                                 ? HSD_DAT_EXTERNAL
                                 : HSD_DAT_DESCRIPTOR;
            import->error_offset = table_offset + i * 4;
            import->external_symbol = external;
            return NULL;
        }
        HSD_DatImportSource(import, data_offset, byte_count, 0);
    }
    return table;
}

void HSD_DatDecodeModelRecord(HSD_DatImport* import,
                             const HSD_DatImportRecord* record)
{
    const HSD_Dat* dat = import->dat;
    u32 offset = record->offset;
    switch (record->type) {
    case DAT_JOINT: {
        HSD_Joint* obj = record->object;
        obj->class_name = HSD_DatImportString(import, offset);
        HSD_DatReadU32(dat, offset + 4, &obj->flags);
        obj->child = HSD_DatImportReference(import, offset + 8, DAT_JOINT, 1);
        obj->next = HSD_DatImportReference(import, offset + 12, DAT_JOINT, 1);
        if (obj->flags & JOBJ_PTCL) {
            obj->u.ptcl =
                HSD_DatImportReference(import, offset + 16, DAT_SLIST, 1);
        } else if (obj->flags & JOBJ_SPLINE) {
            obj->u.spline =
                HSD_DatImportReference(import, offset + 16, DAT_SPLINE, 1);
        } else {
            obj->u.dobjdesc =
                HSD_DatImportReference(import, offset + 16, DAT_DOBJ, 1);
        }
        HSD_DatReadF32(dat, offset + 20, &obj->rotation.x);
        HSD_DatReadF32(dat, offset + 24, &obj->rotation.y);
        HSD_DatReadF32(dat, offset + 28, &obj->rotation.z);
        HSD_DatReadF32(dat, offset + 32, &obj->scale.x);
        HSD_DatReadF32(dat, offset + 36, &obj->scale.y);
        HSD_DatReadF32(dat, offset + 40, &obj->scale.z);
        HSD_DatReadF32(dat, offset + 44, &obj->position.x);
        HSD_DatReadF32(dat, offset + 48, &obj->position.y);
        HSD_DatReadF32(dat, offset + 52, &obj->position.z);
        obj->mtx = HSD_DatImportReference(import, offset + 56, DAT_MATRIX, 1);
        obj->robjdesc = HSD_DatImportReference(import, offset + 60, DAT_ROBJ, 1);
        break;
    }
    case DAT_DOBJ: {
        HSD_DObjDesc* obj = record->object;
        obj->class_name = HSD_DatImportString(import, offset);
        obj->next = HSD_DatImportReference(import, offset + 4, DAT_DOBJ, 1);
        obj->mobjdesc = HSD_DatImportReference(import, offset + 8, DAT_MOBJ, 1);
        obj->pobjdesc = HSD_DatImportReference(import, offset + 12, DAT_POBJ, 1);
        break;
    }
    case DAT_POBJ: {
        HSD_PObjDesc* obj = record->object;
        obj->class_name = HSD_DatImportString(import, offset);
        obj->next = HSD_DatImportReference(import, offset + 4, DAT_POBJ, 1);
        obj->verts = HSD_DatImportList(import, offset + 8, DAT_VTXDESC);
        HSD_DatReadU16(dat, offset + 12, &obj->flags);
        HSD_DatReadU16(dat, offset + 14, &obj->n_display);
        obj->display = HSD_DatImportBytes(import, offset + 16, (size_t) obj->n_display * 32);
        // The fighter/refraction loader rewrites skinned texture matrix indices.
        // Keep those writes in the model arena, never in the immutable DAT.
        if ((pobj_type(obj) == POBJ_ENVELOPE) && obj->display && obj->n_display) {
            size_t size = (size_t) obj->n_display * 32;
            void* writable = HSD_DatImportAlloc(import, size);
            if (!writable) break;
            memcpy(writable, obj->display, size);
            obj->display = writable;
        }
        switch (pobj_type(obj)) {
        case POBJ_SKIN:
            obj->u.joint = HSD_DatImportReference(import, offset + 20, DAT_JOINT, 1);
            break;
        case POBJ_SHAPEANIM:
            obj->u.shape_set =
                HSD_DatImportReference(import, offset + 20, DAT_SHAPE_SET, 1);
            break;
        case POBJ_ENVELOPE:
            obj->u.envelope_p = HSD_DatImportList(import, offset + 20, DAT_ENVELOPE_TABLE);
            break;
        default:
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
            break;
        }
        break;
    }
    case DAT_MATRIX: {
        f32* matrix = record->object;
        for (u32 i = 0; i < 12; ++i) HSD_DatReadF32(dat, offset + i * 4, &matrix[i]);
        break;
    }
    case DAT_VTXDESC: {
        HSD_VtxDescList* entries = record->object;
        for (u32 i = 0; i < record->count && import->status == HSD_DAT_OK; ++i) {
            u32 source = offset + i * 24;
            u32 value;
            HSD_VtxDescList* obj = &entries[i];
            HSD_DatReadU32(dat, source, &value); obj->attr = (GXAttr) value;
            if (obj->attr == GX_VA_NULL) break;
            HSD_DatReadU32(dat, source + 4, &value); obj->attr_type = (GXAttrType) value;
            HSD_DatReadU32(dat, source + 8, &value); obj->comp_cnt = (GXCompCnt) value;
            HSD_DatReadU32(dat, source + 12, &value); obj->comp_type = (GXCompType) value;
            obj->frac = HSD_DatBytes(dat, source + 16, 1)[0];
            HSD_DatReadU16(dat, source + 18, &obj->stride);
            // Original vertex payload remains big-endian for GX. The graph
            // finalization pass validates indices and records the byte extent.
            obj->vertex = HSD_DatImportBytes(import, source + 20,
                obj->attr_type == GX_INDEX8 || obj->attr_type == GX_INDEX16 ? obj->stride : 0);
        }
        break;
    }
    case DAT_ENVELOPE_TABLE: {
        HSD_EnvelopeDesc** table = record->object;
        for (u32 i = 0; i < record->count && import->status == HSD_DAT_OK; ++i)
            table[i] = HSD_DatImportList(import, offset + i * 4, DAT_ENVELOPE);
        break;
    }
    case DAT_ENVELOPE: {
        HSD_EnvelopeDesc* entries = record->object;
        for (u32 i = 0; i < record->count && import->status == HSD_DAT_OK; ++i) {
            entries[i].joint = HSD_DatImportReference(import, offset + i * 8, DAT_JOINT, 1);
            HSD_DatReadF32(dat, offset + i * 8 + 4, &entries[i].weight);
        }
        break;
    }
    case DAT_ROBJ: {
        HSD_RObjDesc* obj = record->object;
        obj->next = HSD_DatImportReference(import, offset, DAT_ROBJ, 1);
        HSD_DatReadU32(dat, offset + 4, &obj->flags);
        switch (obj->flags & ROBJ_TYPE_MASK) {
        case REFTYPE_JOBJ:
            obj->u.joint = HSD_DatImportReference(import, offset + 8, DAT_JOINT, 1); break;
        case REFTYPE_LIMIT:
            HSD_DatReadF32(dat, offset + 8, &obj->u.limit); break;
        case REFTYPE_IKHINT:
            obj->u.ik_hint = HSD_DatImportReference(import, offset + 8, DAT_IKHINT, 1); break;
        case REFTYPE_EXP:
            obj->u.exp = HSD_DatImportReference(import, offset + 8,
                                                DAT_EXP, 1); break;
        case REFTYPE_BYTECODE:
            obj->u.bcexp = HSD_DatImportReference(
                import, offset + 8, DAT_BYTECODE_EXP, 1); break;
        default:
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset + 4;
            break;
        }
        break;
    }
    case DAT_EXP: {
        HSD_ExpDesc* obj = record->object;
        u32 ignored;
        const char* external;
        HSD_DatReferenceKind kind =
            HSD_DatResolve(dat, offset, &ignored, &external);
        if (kind == HSD_DAT_REF_EXTERNAL &&
            import->allow_unresolved_externals)
        {
            import->unresolved_external_count++;
            import->external_symbol = external;
            obj->func = NULL;
        } else if (kind == HSD_DAT_REF_NULL) {
            obj->func = NULL;
        } else {
            import->status = kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                           : HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
            import->external_symbol = external;
            break;
        }
        obj->rvalue = import_rvalue_list(import, offset + 4);
        break;
    }
    case DAT_BYTECODE_EXP: {
        HSD_ByteCodeExpDesc* obj = record->object;
        obj->bytecode = import_bytecode(import, offset);
        if (import->status == HSD_DAT_OK) {
            obj->rvalue = import_rvalue_list(import, offset + 4);
        }
        break;
    }
    case DAT_IKHINT: {
        HSD_IKHintDesc* obj = record->object;
        HSD_DatReadF32(dat, offset, &obj->bone_length);
        HSD_DatReadF32(dat, offset + 4, &obj->rotate_x);
        break;
    }
    case DAT_SLIST: {
        HSD_SList* obj = record->object;
        u32 data;
        obj->next = HSD_DatImportReference(import, offset, DAT_SLIST, 1);
        HSD_DatReadU32(dat, offset + 4, &data);
        obj->data = (void*) (uintptr_t) data;
        break;
    }
    case DAT_SPLINE: {
        HSD_Spline* obj = record->object;
        const u8* source = HSD_DatBytes(dat, offset, 24);
        u16 numcv;
        u32 cv_count;
        if (source == NULL || !HSD_DatReadU16(dat, offset + 2, &numcv) ||
            source[0] > 3 || numcv < 2)
        {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
            break;
        }
        obj->type = source[0];
        obj->numcv = (s16) numcv;
        HSD_DatReadF32(dat, offset + 4, &obj->tension);
        HSD_DatReadF32(dat, offset + 12, &obj->totalLength);
        if (obj->type == 0) {
            cv_count = numcv;
        } else if (obj->type == 1) {
            cv_count = numcv * 3 - 2;
        } else {
            cv_count = numcv + 2;
        }
        obj->cv = (Vec3*) import_float_array(import, offset + 8,
                                             cv_count * 3, true);
        obj->segLength =
            import_float_array(import, offset + 16, numcv, true);
        obj->segPoly = (f32(*)[5]) import_float_array(
            import, offset + 20, (numcv - 1) * 5, obj->type != 0);
        break;
    }
    case DAT_SHAPE_SET: {
        HSD_ShapeSetDesc* obj = record->object;
        u32 vertex_count;
        u32 normal_count;
        HSD_DatReadU16(dat, offset, &obj->flags);
        HSD_DatReadU16(dat, offset + 2, &obj->nb_shape);
        HSD_DatReadU32(dat, offset + 4, &vertex_count);
        HSD_DatReadU32(dat, offset + 16, &normal_count);
        if (vertex_count > S32_MAX || normal_count > S32_MAX ||
            obj->nb_shape == 0 ||
            !(obj->flags & (SHAPESET_AVERAGE | SHAPESET_ADDITIVE)))
        {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
            break;
        }
        obj->nb_vertex_index = (s32) vertex_count;
        obj->nb_normal_index = (s32) normal_count;
        obj->vertex_desc =
            HSD_DatImportList(import, offset + 8, DAT_VTXDESC);
        obj->normal_desc =
            HSD_DatImportList(import, offset + 20, DAT_VTXDESC);
        if (import->status != HSD_DAT_OK || obj->vertex_desc == NULL) {
            break;
        }
        u32 shape_count = obj->nb_shape +
                          ((obj->flags & SHAPESET_ADDITIVE) != 0);
        u32 vertex_desc_offset;
        u32 vertex_type;
        const char* external;
        if (HSD_DatResolve(dat, offset + 8, &vertex_desc_offset, &external) !=
                HSD_DAT_REF_LOCAL ||
            !HSD_DatReadU32(dat, vertex_desc_offset + 4, &vertex_type))
        {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset + 8;
            break;
        }
        obj->vertex_idx_list = import_shape_index_table(
            import, offset + 12, shape_count, vertex_count,
            (GXAttrType) vertex_type);
        if (normal_count != 0) {
            u32 normal_desc_offset;
            u32 normal_type;
            if (obj->normal_desc == NULL ||
                HSD_DatResolve(dat, offset + 20, &normal_desc_offset,
                               &external) != HSD_DAT_REF_LOCAL ||
                !HSD_DatReadU32(dat, normal_desc_offset + 4, &normal_type))
            {
                import->status = HSD_DAT_DESCRIPTOR;
                import->error_offset = offset + 20;
                break;
            }
            obj->normal_idx_list = import_shape_index_table(
                import, offset + 24, shape_count, normal_count,
                (GXAttrType) normal_type);
        }
        break;
    }
    case DAT_ANIMJOINT: {
        HSD_AnimJoint* obj = record->object;
        obj->child = HSD_DatImportReference(import, offset, DAT_ANIMJOINT, 1);
        obj->next = HSD_DatImportReference(import, offset + 4, DAT_ANIMJOINT, 1);
        obj->aobjdesc = HSD_DatImportReference(import, offset + 8, DAT_AOBJ, 1);
        obj->robj_anim = HSD_DatImportReference(import, offset + 12, DAT_ROBJ_ANIM, 1);
        HSD_DatReadU32(dat, offset + 16, &obj->flags);
        break;
    }
    case DAT_ROBJ_ANIM: {
        HSD_RObjAnimJoint* obj = record->object;
        obj->next = HSD_DatImportReference(import, offset, DAT_ROBJ_ANIM, 1);
        obj->aobjdesc = HSD_DatImportReference(import, offset + 4, DAT_AOBJ, 1);
        break;
    }
    default:
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = offset;
        break;
    }
}
