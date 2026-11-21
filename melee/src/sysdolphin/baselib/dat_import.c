#include "dat_import_internal.h"

#include <string.h>

void HSD_DatImportSource(HSD_DatImport* import, u32 offset, size_t size, u32 type)
{
    if (import->source_range && size) import->source_range(import->source_context, offset, size, type);
}

void* HSD_DatImportAlloc(HSD_DatImport* import, size_t size)
{
    uintptr_t address = (uintptr_t) import->arena + import->arena_used;
    size_t padding = (-(uintptr_t) address) & (sizeof(void*) - 1);
    if (padding > import->arena_size - import->arena_used ||
        size > import->arena_size - import->arena_used - padding) {
        import->status = HSD_DAT_CAPACITY;
        return NULL;
    }
    void* result = import->arena + import->arena_used + padding;
    import->arena_used += padding + size;
    memset(result, 0, size);
    return result;
}

// Sort target addresses independently of the on-disc order of pointer slots.
// Sifting a heap bounds construction work regardless of the input order.
static void sift_target(u32* targets, u32 root, u32 count)
{
    u32 value = targets[root];
    while (root < count / 2) {
        u32 child = root * 2 + 1;
        if (child + 1 < count && targets[child] < targets[child + 1]) ++child;
        if (value >= targets[child]) break;
        targets[root] = targets[child];
        root = child;
    }
    targets[root] = value;
}

static void index_targets(const HSD_Dat* dat, u32* targets)
{
    u32 count = dat->relocation_count;
    for (u32 i = 0; i < count; ++i) {
        u32 slot;
        HSD_DatRelocation(dat, i, &slot);
        HSD_DatReadU32(dat, slot, &targets[i]);
    }
    for (u32 i = count / 2; i > 0; --i) sift_target(targets, i - 1, count);
    while (count > 1) {
        u32 largest = targets[0];
        targets[0] = targets[--count];
        targets[count] = largest;
        sift_target(targets, 0, count);
    }
}

u32 HSD_DatImportNextTarget(const HSD_DatImport* import, u32 offset)
{
    const HSD_Dat* dat = import->dat;
    if (!import->sorted_targets) return HSD_DatNextTarget(dat, offset);
    u32 lo = 0, hi = dat->relocation_count;
    while (lo < hi) {
        u32 mid = lo + (hi - lo) / 2;
        if (import->sorted_targets[mid] <= offset) lo = mid + 1;
        else hi = mid;
    }
    return lo < dat->relocation_count ? import->sorted_targets[lo] : dat->data_size;
}

HSD_DatResult HSD_DatImportInit(HSD_DatImport* import, const HSD_Dat* dat,
                               void* arena, size_t arena_size, u32 max_records)
{
    memset(import, 0, sizeof(*import));
    if (arena == NULL || max_records == 0 || max_records > 0x20000000) {
        return import->status = HSD_DAT_CAPACITY;
    }
    import->dat = dat;
    import->texture_byte_limit = (size_t)-1;
    import->display_list_limit = (size_t)-1;
    import->arena = arena;
    import->arena_size = arena_size;
    import->record_capacity = max_records;
    import->slot_count = 2;
    while (import->slot_count < max_records * 2) {
        import->slot_count *= 2;
    }
    import->records = HSD_DatImportAlloc(import, (size_t) max_records * sizeof(*import->records));
    if (import->records == NULL) {
        return import->status;
    }
    import->slots = HSD_DatImportAlloc(import, (size_t) import->slot_count * sizeof(*import->slots));
    if (import->slots && dat && dat->relocation_count > HSD_DAT_LINEAR_RELOCATIONS) {
        import->sorted_targets = HSD_DatImportAlloc(import, (size_t) dat->relocation_count * sizeof(u32));
        if (import->sorted_targets) index_targets(dat, import->sorted_targets);
    }
    return import->status;
}

void* HSD_DatImportObject(HSD_DatImport* import, u32 offset, u32 type, u32 count)
{
    static const size_t native_sizes[] = {
        0, sizeof(HSD_MObjDesc), sizeof(HSD_TObjDesc), sizeof(HSD_Material),
        sizeof(HSD_PEDesc), sizeof(HSD_ImageDesc), sizeof(HSD_TlutDesc),
        sizeof(HSD_TexLODDesc), sizeof(HSD_TObjTevDesc),
        sizeof(HSD_MatAnimJoint), sizeof(HSD_MatAnim), sizeof(HSD_TexAnim),
        sizeof(HSD_AObjDesc), sizeof(HSD_FObjDesc), sizeof(void*), sizeof(void*),
        sizeof(HSD_Joint), sizeof(HSD_DObjDesc), sizeof(HSD_PObjDesc), sizeof(Mtx),
        sizeof(HSD_VtxDescList), sizeof(void*), sizeof(HSD_EnvelopeDesc),
        sizeof(HSD_RObjDesc), sizeof(HSD_IKHintDesc), sizeof(HSD_AnimJoint),
        sizeof(HSD_RObjAnimJoint),
        sizeof(HSD_CObjDesc), sizeof(HSD_WObjDesc), sizeof(Vec3),
        sizeof(HSD_FogDesc), sizeof(HSD_FogAdjDesc),
        sizeof(HSD_LightDesc), sizeof(HSD_LightPointDesc),
        sizeof(HSD_LightSpotDesc), sizeof(HSD_LightAttn),
        sizeof(HSD_ShapeAnimJoint), sizeof(HSD_ShapeAnimDObj), sizeof(HSD_ShapeAnim),
        sizeof(HSD_LightAnim), sizeof(HSD_WObjAnim), sizeof(void*),
        sizeof(HSD_ShapeSetDesc), sizeof(HSD_SList), sizeof(HSD_Spline),
        sizeof(HSD_ExpDesc), sizeof(HSD_ByteCodeExpDesc),
        sizeof(HSD_EnvelopeMatricesDesc),
        sizeof(HSD_NormalBasis) * 10,
    };
    static const u32 source_sizes[] = { 0, 24, 92, 20, 12, 24, 16, 16, 32,
                                       12, 16, 24, 16, 20, 4, 4,
                                       64, 16, 24, 48, 24, 4, 8, 12, 8, 20, 8,
                                       56, 20, 12, 20, 68, 28, 12, 20, 24,
                                       12, 8, 8, 16, 8, 4, 28, 8, 24, 8, 8, 24, 360 };
    u32 slot;
    HSD_DatImportRecord* record;
    if (import->status != HSD_DAT_OK) {
        return NULL;
    }
    if (type == 0 || type >= sizeof(source_sizes) / sizeof(*source_sizes) ||
        count == 0 || (type != DAT_IMAGE_TABLE && type != DAT_PALETTE_TABLE &&
                       type != DAT_VTXDESC && type != DAT_ENVELOPE_TABLE &&
                       type != DAT_ENVELOPE && type != DAT_LIGHT_ANIM_TABLE && count != 1) ||
        count > import->dat->data_size / source_sizes[type] ||
        HSD_DatBytes(import->dat, offset, (size_t) source_sizes[type] * count) == NULL) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = offset;
        return NULL;
    }
    slot = (((offset >> 2) * 2654435761U) ^ (type * 2246822519U)) &
           (import->slot_count - 1);
    while (import->slots[slot]) {
        record = &import->records[import->slots[slot] - 1];
        if (record->offset == offset && record->type == type) {
            if (record->count != count) {
                import->status = HSD_DAT_DESCRIPTOR;
                import->error_offset = offset;
                return NULL;
            }
            return record->object;
        }
        slot = (slot + 1) & (import->slot_count - 1);
    }
    if (import->record_count == import->record_capacity) {
        import->status = HSD_DAT_CAPACITY;
        import->error_offset = offset;
        return NULL;
    }
    record = &import->records[import->record_count];
    record->object = HSD_DatImportAlloc(import, native_sizes[type] * count);
    if (record->object == NULL) {
        import->error_offset = offset;
        return NULL;
    }
    record->offset = offset;
    record->type = type;
    record->count = count;
    HSD_DatImportSource(import, offset, (size_t) source_sizes[type] * count, type);
    import->slots[slot] = ++import->record_count;
    return record->object;
}

static HSD_DatReferenceKind reference(HSD_DatImport* import, u32 slot, u32* offset)
{
    const char* external;
    HSD_DatReferenceKind kind;
    if (import->status != HSD_DAT_OK) {
        return HSD_DAT_REF_INVALID;
    }
    kind = HSD_DatResolve(import->dat, slot, offset, &external);
    if (kind == HSD_DAT_REF_EXTERNAL && import->allow_unresolved_externals) {
        import->unresolved_external_count++;
        import->external_symbol = external;
        return kind;
    }
    if (kind == HSD_DAT_REF_INVALID || kind == HSD_DAT_REF_EXTERNAL) {
        import->status = kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL : HSD_DAT_RELOCATION;
        import->error_offset = slot;
        import->external_symbol = external;
    }
    return kind;
}

void* HSD_DatImportReference(HSD_DatImport* import, u32 slot, u32 type, u32 count)
{
    u32 offset;
    if (reference(import, slot, &offset) != HSD_DAT_REF_LOCAL) {
        return NULL;
    }
    if (type == 0) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
        return NULL;
    }
    return HSD_DatImportObject(import, offset, type, count);
}

u8* HSD_DatImportBytes(HSD_DatImport* import, u32 slot, size_t size)
{
    u32 offset;
    HSD_DatReferenceKind kind = reference(import, slot, &offset);
    const u8* data;
    if (kind != HSD_DAT_REF_LOCAL) {
        if (kind == HSD_DAT_REF_NULL && size != 0 && import->status == HSD_DAT_OK) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = slot;
        }
        return NULL;
    }
    data = HSD_DatBytes(import->dat, offset, size);
    if (data == NULL) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
    }
    if (data) HSD_DatImportSource(import, offset, size, 0);
    return (u8*) data;
}

void* HSD_DatImportList(HSD_DatImport* import, u32 slot, u32 type)
{
    u32 offset, count = 0;
    u32 stride = type == DAT_VTXDESC ? 24 : type == DAT_ENVELOPE ? 8 : 4;
    if (reference(import, slot, &offset) != HSD_DAT_REF_LOCAL) {
        return NULL;
    }
    for (u32 current = offset; ; current += stride) {
        bool end;
        if (HSD_DatBytes(import->dat, current, stride) == NULL) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = current;
            return NULL;
        }
        if (type == DAT_VTXDESC) {
            u32 attr;
            HSD_DatReadU32(import->dat, current, &attr);
            end = attr == GX_VA_NULL;
        } else {
            u32 target;
            HSD_DatReferenceKind kind = reference(import, current, &target);
            if (import->status != HSD_DAT_OK) return NULL;
            end = kind == HSD_DAT_REF_NULL;
        }
        ++count;
        if (end) break;
    }
    return HSD_DatImportObject(import, offset, type, count);
}

char* HSD_DatImportString(HSD_DatImport* import, u32 slot)
{
    u32 offset;
    const u8* data;
    if (reference(import, slot, &offset) != HSD_DAT_REF_LOCAL) {
        return NULL;
    }
    data = HSD_DatBytes(import->dat, offset, 1);
    const u8* end = data ? memchr(data, 0, import->dat->data_size - offset) : NULL;
    if (end == NULL) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
        return NULL;
    }
    HSD_DatImportSource(import, offset, (size_t)(end - data) + 1, 0);
    return (char*) data;
}

static bool valid_joint_tracks(const HSD_AnimJoint* animation)
{
    if (!animation->aobjdesc) return true;
    u32 count = 0;
    for (const HSD_FObjDesc* track = animation->aobjdesc->fobjdesc; track; track = track->next) {
        // FObjUpdateAnim supplies one scalar. JObjUpdateFunc reads a Vec3
        // for the four matrix columns supplied by the constraint solver.
        if (++count > 256 || (track->type >= 0x32 && track->type <= 0x35)) {
            return false;
        }
    }
    return true;
}

static bool valid_particle_list(const HSD_Joint* joint)
{
    if (!union_type_ptcl(joint)) return true;
    unsigned count = 0;
    for (const HSD_SList* particle = joint->u.ptcl; particle; particle = particle->next) {
        if (++count > 256) return false;
    }
    return true;
}

void* HSD_DatImportGraph(HSD_DatImport* import, u32 offset, HSD_DatImportType type)
{
    const u32 first = import->decoded_count;
    void* result = HSD_DatImportObject(import, offset, type, 1);
    // The growing record array is also the work queue. Register objects before
    // decoding links so shared references and cycles need no recursive descent.
    while (import->status == HSD_DAT_OK && import->decoded_count < import->record_count) {
        const HSD_DatImportRecord* record = &import->records[import->decoded_count++];
        if (record->type < DAT_MATANIM_JOINT) {
            HSD_DatDecodeMaterialRecord(import, record);
        } else if (record->type < DAT_JOINT) {
            HSD_DatDecodeAnimationRecord(import, record);
        } else if (record->type == DAT_SHAPE_SET ||
                   record->type == DAT_SLIST ||
                   record->type == DAT_SPLINE ||
                   record->type == DAT_EXP ||
                   record->type == DAT_BYTECODE_EXP ||
                   record->type == DAT_ENVELOPE_MATRICES || record->type == DAT_NORMAL_BASES) {
            HSD_DatDecodeModelRecord(import, record);
        } else if (record->type >= DAT_SHAPE_JOINT) {
            HSD_DatDecodeAnimationRecord(import, record);
        } else if (record->type >= DAT_CAMERA) {
            HSD_DatDecodeSceneRecord(import, record);
        } else {
            HSD_DatDecodeModelRecord(import, record);
        }
    }
    bool has_new_geometry = false;
    for (u32 i = first; i < import->record_count && import->status == HSD_DAT_OK; ++i) {
        HSD_DatImportRecord* record = &import->records[i];
        if (record->type == DAT_JOINT && !valid_particle_list(record->object)) {
            import->status = HSD_DAT_RESOURCE_LIMIT;
            import->error_offset = record->offset;
        }
        if (record->type == DAT_ANIMJOINT && !valid_joint_tracks(record->object)) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = record->offset;
        }
        if (record->type == DAT_TEXANIM &&
            !HSD_DatBindTextureAnimation(import, record->object)) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = record->offset;
        }
        if (record->type == DAT_POBJ) {
            has_new_geometry = true;
            if (!HSD_DatValidateGeometry(import, record->object)) {
                import->status = HSD_DAT_DESCRIPTOR;
                import->error_offset = record->offset;
            }
        }
        if (record->type == DAT_SHAPE_SET && !HSD_DatValidateShape(import, record->object)) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = record->offset;
        }
    }
    if (has_new_geometry && import->status == HSD_DAT_OK &&
        !HSD_DatNormalizeVertexArrays(import)) return NULL;
    if (type == DAT_JOINT && import->status == HSD_DAT_OK &&
        import->has_joint_bindings && !HSD_DatValidateConstraintBindings(import, result)) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = offset;
    }
    return import->status == HSD_DAT_OK ? result : NULL;
}
