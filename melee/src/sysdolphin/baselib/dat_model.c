#include "dat_import_internal.h"
#include "native_constraint.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

static HSD_RvalueList* import_rvalue_list(HSD_DatImport* import, u32 slot, u32* arguments)
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
        // The evaluator emits one scalar for each selected axis of local or
        // world rotation, translation and scale.
        u32 axes = output[i].flags;
        if (axes & ~0x03F703F7u) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = source + i * 8;
            return NULL;
        }
        for (; axes; axes &= axes - 1) ++*arguments;
        if (*arguments > HSD_CONSTRAINT_ARGUMENTS) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = source + i * 8;
            return NULL;
        }
        HSD_DatResolve(import->dat, source + i * 8 + 4, &joint, &external);
        output[i].joint = HSD_DatImportObject(import, joint, DAT_JOINT, 1);
        if (output[i].joint == NULL) {
            return NULL;
        }
    }
    return output;
}

typedef struct ConstraintStackRange {
    u8 min, max; // Depth plus one; zero marks unreachable bytes.
} ConstraintStackRange;

static void join_depth(ConstraintStackRange* range, unsigned min, unsigned max)
{
    if (!range->min || range->min > min) range->min = (u8)min;
    if (range->max < max) range->max = (u8)max;
}

static size_t inspect_constraint(const u8* bytes, size_t size, u32 arguments,
                                 ConstraintStackRange* path)
{
    path[0] = (ConstraintStackRange){1, 1};
    size_t last = 0, used = 0;
    unsigned instructions = 0;
    // Branches only move forward. Every predecessor has contributed its
    // possible stack depths by the time this walk reaches an instruction.
    for (size_t pc = 0; pc <= last; ++pc) {
        if (!path[pc].min) continue;
        if (++instructions > HSD_CONSTRAINT_INSTRUCTIONS) return 0;
        unsigned min = path[pc].min, max = path[pc].max;
        unsigned op = bytes[pc], width = 0, needed = 0, pop = 0, push = 0;
        if (op > Cns_Copy) return 0;
        if (op == Cns_Argument || op == Cns_JumpIf || op == Cns_Jump) width = 2;
        if (op == Cns_Drop || op == Cns_Copy) width = 1;
        if (op == Cns_Literal) width = 4;
        if (width > size - pc - 1) return 0;
        unsigned operand = 0;
        for (unsigned n = 1; n <= width; ++n) {
            if (path[pc + n].min) return 0;
            operand = (operand << 8) | bytes[pc + n];
        }
        size_t next = pc + 1 + width;
        used = next;
        switch (op) {
        case Cns_Return:
            if (min <= 1) return 0;
            continue;
        case Cns_Argument:
            if (operand >= arguments) return 0;
            push = 1;
            break;
        case Cns_Literal: push = 1; break;
        case Cns_Copy: needed = operand + 1; push = 1; break;
        case Cns_Drop: needed = pop = operand; break;
        case Cns_JumpIf: needed = pop = 1; break;
        case Cns_Nop: case Cns_Jump: break;
        default:
            needed = (op <= Cns_Sqrt || op == Cns_AbsInt || op == Cns_Not) ? 1 : 2;
            pop = needed - 1;
            break;
        }
        if (min <= needed || max - pop + push > HSD_CONSTRAINT_STACK + 1) return 0;
        min = min - pop + push;
        max = max - pop + push;
        if (op == Cns_Jump || op == Cns_JumpIf) {
            size_t target = next + operand;
            if (target >= size) return 0;
            join_depth(&path[target], min, max);
            if (target > last) last = target;
            if (op == Cns_Jump) continue;
        }
        if (next == size) return 0;
        join_depth(&path[next], min, max);
        if (next > last) last = next;
    }
    return used;
}

static u8* import_bytecode(HSD_DatImport* import, u32 slot, u32 arguments)
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
    u32 end = HSD_DatImportNextTarget(import, source);
    size_t size = end - source;
    if (size == 0 || size > 64 * 1024 ||
        HSD_DatBytes(import->dat, source, size) == NULL)
    {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = source;
        return NULL;
    }
    const u8* input = HSD_DatBytes(import->dat, source, size);
    if (size > 64u * 1024 * 1024 - import->animation_stream_bytes) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = source;
        return NULL;
    }
    import->animation_stream_bytes += (u32)size;
    ConstraintStackRange* path = calloc(size, sizeof(*path));
    if (!path) {
        import->status = HSD_DAT_CAPACITY;
        import->error_offset = source;
        return NULL;
    }
    size_t used = inspect_constraint(input, size, arguments, path);
    free(path);
    if (!used) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = source;
        return NULL;
    }
    u8* output = HSD_DatImportAlloc(import, used);
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    memcpy(output, input, used);
    HSD_DatImportSource(import, source, used, 0);
    return output;
}

// Construction registers ordinary children and siblings in HSD's joint ID
// table. A joint reached only through a constraint has no runtime ID.
typedef struct ConstraintJoints {
    const HSD_Joint** slots;
    u32 mask;
    u32 count;
} ConstraintJoints;

static const HSD_Joint** constraint_joint_slot(const ConstraintJoints* joints,
                                              const HSD_Joint* joint)
{
    u32 index = (u32)(((uintptr_t)joint >> 3) * 2654435761u) & joints->mask;
    while (joints->slots[index] && joints->slots[index] != joint)
        index = (index + 1) & joints->mask;
    return &joints->slots[index];
}

static bool collect_constraint_joints(ConstraintJoints* joints, const HSD_Joint* joint,
                                      unsigned depth)
{
    // The runtime constructor recurses into both children and siblings.
    for (; joint; joint = joint->next, ++depth) {
        if (depth > 256 || joints->count >= (joints->mask + 1) / 2) return false;
        const HSD_Joint** slot = constraint_joint_slot(joints, joint);
        if (*slot) return false;
        *slot = joint;
        ++joints->count;
        if (!(joint->flags & JOBJ_INSTANCE) &&
            !collect_constraint_joints(joints, joint->child, depth + 1)) return false;
    }
    return true;
}

bool HSD_DatValidateConstraintBindings(const HSD_DatImport* import, const HSD_Joint* root)
{
    u32 capacity = 2;
    while (capacity < 8192 && capacity / 2 < import->record_count) capacity *= 2;
    ConstraintJoints joints = {calloc(capacity, sizeof(*joints.slots)), capacity - 1, 0};
    if (!joints.slots) return false;
    bool valid = collect_constraint_joints(&joints, root, 0);
    unsigned budget = 4096, expressions = 0;
    for (u32 i = 0; valid && i < capacity; ++i) {
        const HSD_Joint* joint = joints.slots[i];
        if (!joint) continue;
        unsigned constraints = 0;
        for (const HSD_RObjDesc* r = joint->robjdesc; r; r = r->next) {
            if (!budget || ++constraints > 256) { valid = false; break; }
            --budget;
            u32 kind = r->flags & ROBJ_TYPE_MASK;
            if (kind == REFTYPE_JOBJ && (!r->u.joint || !*constraint_joint_slot(&joints, r->u.joint))) {
                valid = false;
                break;
            }
            if (kind != REFTYPE_EXP && kind != REFTYPE_BYTECODE) continue;
            if (++expressions > HSD_CONSTRAINT_EXPRESSIONS_PER_MODEL) { valid = false; break; }
            u32 channel = r->flags & 0x0fffffff;
            if (channel >= HSD_A_J_ROTX && channel <= HSD_A_J_ROTZ) {
                valid = !(joint->flags & JOBJ_USE_QUATERNION);
            } else {
                valid = channel >= HSD_A_J_TRAX && channel <= HSD_A_J_BRANCH;
            }
            const HSD_RvalueList* input =
                kind == REFTYPE_EXP ? r->u.exp->rvalue : r->u.bcexp->rvalue;
            for (; valid && input && input->joint; ++input) {
                valid = budget && *constraint_joint_slot(&joints, input->joint);
                if (valid) --budget;
            }
            if (!valid) break;
        }
    }
    free(joints.slots);
    return valid;
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
    if (kind != HSD_DAT_REF_LOCAL || count == 0 || offset > import->dat->data_size ||
        count > (import->dat->data_size - offset) / sizeof(f32))
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
        if (!HSD_DatReadF32(import->dat, offset + i * 4, &output[i]) || !isfinite(output[i])) {
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
                                     GXAttrType index_type,bool wide)
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
    if ((index_type != GX_INDEX8 && index_type != GX_INDEX16) || (wide && index_type!=GX_INDEX16)) {
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
                        (wide?4:index_type == GX_INDEX16 ? 2 : 1);
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
        // The billboard matrix builder supports modes through RBILLBOARD.
        if ((obj->flags & JOBJ_BILLBOARD_FIELD) > JOBJ_RBILLBOARD ||
            (union_type_spline(obj) && union_type_ptcl(obj))) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset + 4;
            break;
        }
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
        if (!isfinite(obj->rotation.x) || !isfinite(obj->rotation.y) || !isfinite(obj->rotation.z) ||
            !isfinite(obj->scale.x) || !isfinite(obj->scale.y) || !isfinite(obj->scale.z) ||
            !isfinite(obj->position.x) || !isfinite(obj->position.y) || !isfinite(obj->position.z)) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
            break;
        }
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
        const size_t size = (size_t) obj->n_display * 32;
        if (size > import->display_list_limit - import->display_list_bytes) {
            import->status = HSD_DAT_RESOURCE_LIMIT;
            import->error_offset = offset;
            break;
        }
        import->display_list_bytes += size;
        obj->display = HSD_DatImportBytes(import, offset + 16, size);
        // The fighter/refraction loader rewrites skinned texture matrix indices.
        // Keep those writes in the model arena, never in the immutable DAT.
        if ((pobj_type(obj) == POBJ_ENVELOPE) && obj->display && obj->n_display) {
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
        case POBJ_SEPARATE_ENVELOPES:
            obj->u.matrices = HSD_DatImportReference(import, offset + 20, DAT_ENVELOPE_MATRICES, 1);
            break;
        default:
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
            break;
        }
        break;
    }
    case DAT_ENVELOPE_MATRICES: {
        HSD_EnvelopeMatricesDesc* obj = record->object;
        obj->positions = HSD_DatImportList(import, offset, DAT_ENVELOPE_TABLE);
        obj->textures = HSD_DatImportList(import, offset + 4, DAT_ENVELOPE_TABLE);
        memcpy(obj->followers, HSD_DatBytes(dat, offset + 8, 10), 10);
        HSD_DatReadU16(dat, offset + 18, &obj->constant_mask);
        obj->normal_bases = HSD_DatImportReference(import, offset + 20, DAT_NORMAL_BASES, 1);
        break;
    }
    case DAT_NORMAL_BASES: {
        f32* values = record->object;
        for (u32 i = 0; i < 90; ++i) HSD_DatReadF32(dat, offset + i*4, &values[i]);
        break;
    }
    case DAT_MATRIX: {
        f32* matrix = record->object;
        for (u32 i = 0; i < 12; ++i) {
            if (!HSD_DatReadF32(dat, offset + i * 4, &matrix[i]) || !isfinite(matrix[i])) {
                import->status = HSD_DAT_DESCRIPTOR;
                import->error_offset = offset + i * 4;
                break;
            }
        }
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
        u32 kind = obj->flags & ROBJ_TYPE_MASK;
        switch (kind) {
        case REFTYPE_JOBJ:
            import->has_joint_bindings = true;
            obj->u.joint = HSD_DatImportReference(import, offset + 8, DAT_JOINT, 1); break;
        case REFTYPE_LIMIT:
            HSD_DatReadF32(dat, offset + 8, &obj->u.limit);
            if (!isfinite(obj->u.limit)) {
                import->status = HSD_DAT_DESCRIPTOR;
                import->error_offset = offset + 8;
            }
            break;
        case REFTYPE_IKHINT:
            obj->u.ik_hint = HSD_DatImportReference(import, offset + 8, DAT_IKHINT, 1); break;
        case REFTYPE_EXP:
            import->has_joint_bindings = true;
            obj->u.exp = HSD_DatImportReference(import, offset + 8,
                                                DAT_EXP, 1); break;
        case REFTYPE_BYTECODE:
            import->has_joint_bindings = true;
            obj->u.bcexp = HSD_DatImportReference(
                import, offset + 8, DAT_BYTECODE_EXP, 1); break;
        default:
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset + 4;
            break;
        }
        if (import->status == HSD_DAT_OK &&
            ((kind == REFTYPE_EXP && !obj->u.exp) ||
             (kind == REFTYPE_BYTECODE && !obj->u.bcexp) ||
             (kind == REFTYPE_IKHINT && !obj->u.ik_hint))) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset + 8;
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
        u32 arguments = 0;
        obj->rvalue = import_rvalue_list(import, offset + 4, &arguments);
        break;
    }
    case DAT_BYTECODE_EXP: {
        HSD_ByteCodeExpDesc* obj = record->object;
        u32 arguments = 0;
        obj->rvalue = import_rvalue_list(import, offset + 4, &arguments);
        if (import->status == HSD_DAT_OK)
            obj->bytecode = import_bytecode(import, offset, arguments);
        break;
    }
    case DAT_IKHINT: {
        HSD_IKHintDesc* obj = record->object;
        HSD_DatReadF32(dat, offset, &obj->bone_length);
        HSD_DatReadF32(dat, offset + 4, &obj->rotate_x);
        if (!isfinite(obj->bone_length) || !isfinite(obj->rotate_x)) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
        }
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
            source[0] > 3 || numcv < 2 || numcv > 32767)
        {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
            break;
        }
        obj->type = source[0];
        obj->numcv = (s16) numcv;
        HSD_DatReadF32(dat, offset + 4, &obj->tension);
        HSD_DatReadF32(dat, offset + 12, &obj->totalLength);
        if (!isfinite(obj->tension) || !isfinite(obj->totalLength) || obj->totalLength < 0) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset;
            break;
        }
        if (obj->type == 0) {
            cv_count = numcv;
        } else if (obj->type == 1) {
            cv_count = numcv * 3 - 2;
        } else {
            cv_count = numcv + 2;
        }
        obj->cv = (Vec3*) import_float_array(import, offset + 8,
                                             cv_count * 3, true);
        if (!obj->cv) break;
        obj->segLength =
            import_float_array(import, offset + 16, numcv, true);
        if (!obj->segLength) break;
        // The runtime searches cumulative distances for any interior float in (0, 1).
        bool covers_path = obj->segLength[0] == 0 &&
                           obj->segLength[numcv - 1] >= 0x1.fffffep-1f;
        for (u32 i = 1; covers_path && i < numcv; ++i)
            covers_path = obj->segLength[i] >= obj->segLength[i - 1];
        if (!covers_path) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = offset + 16;
            break;
        }
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
        if(obj->flags & SHAPESET_PROPERTIES) {
            if(!HSD_DatBytes(dat,offset,40)) {
                import->status=HSD_DAT_DESCRIPTOR;import->error_offset=offset;break;
            }
            HSD_DatImportSource(import,offset,40,DAT_SHAPE_SET);
            const unsigned values=obj->flags&SHAPESET_ADDITIVE?obj->nb_shape:1;
            obj->default_blend=import_float_array(import,offset+32,values,false);
            u32 names;const char* external;
            const HSD_DatReferenceKind kind=HSD_DatResolve(dat,offset+36,&names,&external);
            if(kind==HSD_DAT_REF_LOCAL) {
                if(!HSD_DatBytes(dat,names,(size_t)obj->nb_shape*4)) {
                    import->status=HSD_DAT_DESCRIPTOR;import->error_offset=names;break;
                }
                HSD_DatImportSource(import,names,(size_t)obj->nb_shape*4,0);
                obj->target_names=HSD_DatImportAlloc(import,(size_t)obj->nb_shape*sizeof(*obj->target_names));
                if(!obj->target_names)break;
                for(unsigned i=0;i<obj->nb_shape && import->status==HSD_DAT_OK;++i) {
                    obj->target_names[i]=HSD_DatImportString(import,names+i*4);
                    if(!obj->target_names[i] || !*obj->target_names[i] || strlen(obj->target_names[i])>127) {
                        import->status=HSD_DAT_DESCRIPTOR;import->error_offset=names+i*4;
                    }
                }
            } else if(kind!=HSD_DAT_REF_NULL) {
                import->status=HSD_DAT_DESCRIPTOR;import->error_offset=offset+36;
            }
            if(import->status!=HSD_DAT_OK)break;
        }
        if(obj->flags & SHAPESET_SKIN) {
            if(!HSD_DatBytes(dat,offset,32)) {
                import->status=HSD_DAT_DESCRIPTOR;import->error_offset=offset;break;
            }
            HSD_DatImportSource(import,offset,32,DAT_SHAPE_SET);
            obj->skin=HSD_DatImportReference(import,offset+28,DAT_POBJ,1);
            if(!obj->skin) {
                import->status=HSD_DAT_DESCRIPTOR;import->error_offset=offset+28;break;
            }
        }
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
            (GXAttrType) vertex_type,(obj->flags&SHAPESET_INDEX32)!=0);
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
                (GXAttrType) normal_type,(obj->flags&SHAPESET_INDEX32)!=0);
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
