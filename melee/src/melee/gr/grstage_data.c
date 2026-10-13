#include "grstage_data.h"
#include "gricemt_data.h"

#include <melee/lb/lbcolanim_data.h>
#include <melee/lb/types.h>
#include <melee/mp/mpdat.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/dat_sis.h>

#include <limits.h>
#include <string.h>

typedef struct LightOverrideEntry {
    HSD_LightDesc* desc;
    u8 flags;
} LightOverrideEntry;

static DynamicsDesc* import_dynamics(HSD_DatImport* import, u32 root);

static bool fail(HSD_DatImport* import, HSD_DatResult status, u32 offset)
{
    import->status = status;
    import->error_offset = offset;
    return false;
}

static bool read_u32(const HSD_Dat* dat, u32 offset, u32* value)
{
    return HSD_DatReadU32(dat, offset, value);
}

static bool read_s16(const HSD_Dat* dat, u32 offset, s16* value)
{
    u16 bits;
    if (!HSD_DatReadU16(dat, offset, &bits)) {
        return false;
    }
    *value = (s16) bits;
    return true;
}

static void* import_reference(HSD_DatImport* import, u32 slot,
                              HSD_DatImportType type)
{
    const char* external;
    u32 target;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &target, &external);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind == HSD_DAT_REF_EXTERNAL && import->allow_unresolved_externals) {
        ++import->unresolved_external_count;
        import->external_symbol = external;
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        fail(import, kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                   : HSD_DAT_RELOCATION,
             slot);
        import->external_symbol = external;
        return NULL;
    }
    return HSD_DatImportGraph(import, target, type);
}

static bool pointer_list_count(const HSD_Dat* dat, u32 offset, u32* count)
{
    const char* external;
    u32 i;
    u32 target;
    if ((offset & 3) != 0 || offset > dat->data_size) {
        return false;
    }
    u32 available = (dat->data_size - offset) / 4;
    for (i = 0; i < available; ++i) {
        HSD_DatReferenceKind kind =
            HSD_DatResolve(dat, offset + i * 4, &target, &external);
        if (kind == HSD_DAT_REF_NULL) {
            *count = i;
            return true;
        }
        if (kind != HSD_DAT_REF_LOCAL) {
            return false;
        }
    }
    return false;
}

static void** import_graph_list(HSD_DatImport* import, u32 slot,
                                HSD_DatImportType type, u32* result_count)
{
    const char* external;
    u32 count;
    u32 list_offset;
    u32 target;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &list_offset, &external);
    if (kind == HSD_DAT_REF_NULL) {
        if (result_count != NULL) {
            *result_count = 0;
        }
        return NULL;
    }
    if (kind == HSD_DAT_REF_EXTERNAL && import->allow_unresolved_externals) {
        ++import->unresolved_external_count;
        import->external_symbol = external;
        if (result_count != NULL) {
            *result_count = 0;
        }
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL ||
        !pointer_list_count(import->dat, list_offset, &count))
    {
        fail(import, HSD_DAT_RELOCATION, slot);
        return NULL;
    }
    void** list = HSD_DatImportAlloc(import, (size_t) (count + 1) * sizeof(*list));
    if (list == NULL) {
        import->error_offset = slot;
        return NULL;
    }
    for (u32 i = 0; i < count; ++i) {
        if (HSD_DatResolve(import->dat, list_offset + i * 4, &target,
                           &external) != HSD_DAT_REF_LOCAL ||
            (list[i] = HSD_DatImportGraph(import, target, type)) == NULL)
        {
            if (import->status == HSD_DAT_OK) {
                fail(import, HSD_DAT_RELOCATION, list_offset + i * 4);
            }
            return NULL;
        }
    }
    if (result_count != NULL) {
        *result_count = count;
    }
    return list;
}

static void** import_animation_arrays(HSD_DatImport* import, u32 slot,
                                      HSD_DatImportType type,
                                      u32 first_group_joint_count,
                                      u32 other_group_joint_count,
                                      u32* result_count)
{
    const char* external;
    u32 list_offset, group_offset, count;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &list_offset, &external);
    if (kind == HSD_DAT_REF_NULL) {
        if (result_count != NULL) {
            *result_count = 0;
        }
        return NULL;
    }
    if (kind == HSD_DAT_REF_EXTERNAL && import->allow_unresolved_externals) {
        ++import->unresolved_external_count;
        import->external_symbol = external;
        if (result_count != NULL) {
            *result_count = 0;
        }
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL ||
        !pointer_list_count(import->dat, list_offset, &count))
    {
        fail(import, HSD_DAT_RELOCATION, slot);
        return NULL;
    }
    void** groups = HSD_DatImportAlloc(import,
                                       (size_t) (count + 1) * sizeof(*groups));
    if (groups == NULL) {
        import->error_offset = slot;
        return NULL;
    }
    u32 stride = type == DAT_ANIMJOINT ? 20 : 12;
    u32 pointer_count = type == DAT_ANIMJOINT ? 4 : 3;
    size_t native_size = type == DAT_ANIMJOINT ? sizeof(HSD_AnimJoint)
        : type == DAT_MATANIM_JOINT ? sizeof(HSD_MatAnimJoint)
                                    : sizeof(HSD_ShapeAnimJoint);
    for (u32 i = 0; i < count; ++i) {
        // Each group is a stage animation state. Its contiguous descriptors
        // address joints by index; the archive does not store their count.
        u32 entries = i == 0 ? first_group_joint_count :
                               other_group_joint_count;
        if (HSD_DatResolve(import->dat, list_offset + i * 4,
                           &group_offset, &external) != HSD_DAT_REF_LOCAL)
        {
            fail(import, HSD_DAT_RELOCATION, list_offset + i * 4);
            return NULL;
        }
        for (u32 entry = 0; entry < entries; ++entry) {
            u32 at = group_offset + entry * stride;
            if (HSD_DatBytes(import->dat, at, stride) == NULL) {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                return NULL;
            }
            for (u32 j = 0; j < pointer_count; ++j) {
                u32 target;
                HSD_DatReferenceKind ref = HSD_DatResolve(
                    import->dat, at + j * 4, &target, &external);
                if (ref != HSD_DAT_REF_NULL && ref != HSD_DAT_REF_LOCAL) {
                    fail(import, HSD_DAT_RELOCATION, at + j * 4);
                    return NULL;
                }
            }
        }
        groups[i] = HSD_DatImportAlloc(import,
                                       (size_t) entries * native_size);
        if (groups[i] == NULL) {
            import->error_offset = group_offset;
            return NULL;
        }
        for (u32 j = 0; j < entries; ++j) {
            void* source = HSD_DatImportGraph(import,
                            group_offset + j * stride, type);
            if (source == NULL) {
                return NULL;
            }
            memcpy((u8*) groups[i] + (size_t) j * native_size,
                   source, native_size);
        }
    }
    if (result_count != NULL) {
        *result_count = count;
    }
    return groups;
}

static bool import_dynamic_model(HSD_DatImport* import, u32 offset,
                                 DynamicModelDesc* output, u32* anim_count,
                                 const GrStageAnimationLayout* layout)
{
    u32 counts[3] = { 0 };
    u32 joint_offset;
    const char* joint_external;
    HSD_DatReferenceKind joint_kind = HSD_DatResolve(
        import->dat, offset, &joint_offset, &joint_external);
    output->joint = import_reference(import, offset, DAT_JOINT);
    if (layout != NULL) {
        output->anims = (HSD_AnimJoint**) import_animation_arrays(
            import, offset + 4, DAT_ANIMJOINT,
            layout->first_group_joint_count, layout->other_group_joint_count,
            &counts[0]);
        output->matanims = (HSD_MatAnimJoint**) import_animation_arrays(
            import, offset + 8, DAT_MATANIM_JOINT,
            layout->first_group_joint_count, layout->other_group_joint_count,
            &counts[1]);
        output->shapeanims = (HSD_ShapeAnimJoint**) import_animation_arrays(
            import, offset + 12, DAT_SHAPE_JOINT,
            layout->first_group_joint_count, layout->other_group_joint_count,
            &counts[2]);
    } else {
        output->anims = (HSD_AnimJoint**) import_graph_list(
            import, offset + 4, DAT_ANIMJOINT, &counts[0]);
        output->matanims = (HSD_MatAnimJoint**) import_graph_list(
            import, offset + 8, DAT_MATANIM_JOINT, &counts[1]);
        output->shapeanims = (HSD_ShapeAnimJoint**) import_graph_list(
            import, offset + 12, DAT_SHAPE_JOINT, &counts[2]);
    }
    if (anim_count != NULL) {
        *anim_count = counts[0];
        if (counts[1] > *anim_count) {
            *anim_count = counts[1];
        }
        if (counts[2] > *anim_count) {
            *anim_count = counts[2];
        }
    }
    return import->status == HSD_DAT_OK &&
           (output->joint != NULL ||
            (joint_kind == HSD_DAT_REF_EXTERNAL &&
             import->allow_unresolved_externals));
}

static LightList** import_light_list(HSD_DatImport* import, u32 list_offset)
{
    const char* external;
    u32 count;
    u32 record_offset;
    if (!pointer_list_count(import->dat, list_offset, &count)) {
        fail(import, HSD_DAT_RELOCATION, list_offset);
        return NULL;
    }
    LightList** list = HSD_DatImportAlloc(
        import, (size_t) (count + 1) * sizeof(*list));
    if (list == NULL) {
        import->error_offset = list_offset;
        return NULL;
    }
    for (u32 i = 0; i < count; ++i) {
        if (HSD_DatResolve(import->dat, list_offset + i * 4, &record_offset,
                           &external) != HSD_DAT_REF_LOCAL ||
            HSD_DatBytes(import->dat, record_offset, 8) == NULL)
        {
            fail(import, HSD_DAT_RELOCATION, list_offset + i * 4);
            return NULL;
        }
        list[i] = HSD_DatImportAlloc(import, sizeof(*list[i]));
        if (list[i] == NULL) {
            import->error_offset = record_offset;
            return NULL;
        }
        list[i]->desc = import_reference(import, record_offset, DAT_LIGHT);
        list[i]->anims = (HSD_LightAnim**) import_graph_list(
            import, record_offset + 4, DAT_LIGHT_ANIM, NULL);
        if (import->status != HSD_DAT_OK || list[i]->desc == NULL) {
            return NULL;
        }
    }
    return list;
}

static bool import_scene_table(HSD_DatImport* import, u32 offset, u32 count,
                               struct UnkStageDat_x8_t** result,
                               const GrStageImportOptions* options)
{
    if (count == 0 || HSD_DatBytes(import->dat, offset, (size_t) count * 0x34) == NULL) {
        return fail(import, HSD_DAT_DESCRIPTOR, offset);
    }
    struct UnkStageDat_x8_t* scenes = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*scenes));
    if (scenes == NULL) {
        import->error_offset = offset;
        return false;
    }
    for (u32 i = 0; i < count; ++i) {
        struct UnkStageDat_x8_t* scene = &scenes[i];
        u32 at = offset + i * 0x34;
        u32 anim_count = 0;
        u32 target;
        const char* external;
        const GrStageAnimationLayout* layout = NULL;
        for (u32 j = 0; j < options->animation_layout_count; ++j) {
            if (options->animation_layouts[j].scene_index == i) {
                layout = &options->animation_layouts[j];
                break;
            }
        }
        if (!import_dynamic_model(import, at, (DynamicModelDesc*) scene,
                                  &anim_count, layout))
        {
            return false;
        }
        scene->x10 = import_reference(import, at + 0x10, DAT_CAMERA);
        HSD_DatReferenceKind kind =
            HSD_DatResolve(import->dat, at + 0x14, &target, &external);
        if (kind == HSD_DAT_REF_LOCAL) {
            scene->x14 = (void*) HSD_DatBytes(import->dat, target, 1);
        } else if (kind == HSD_DAT_REF_EXTERNAL &&
                   import->allow_unresolved_externals)
        {
            ++import->unresolved_external_count;
            import->external_symbol = external;
        } else if (kind != HSD_DAT_REF_NULL) {
            return fail(import, HSD_DAT_RELOCATION, at + 0x14);
        }
        kind = HSD_DatResolve(import->dat, at + 0x18, &target, &external);
        if (kind == HSD_DAT_REF_LOCAL) {
            scene->x18 = import_light_list(import, target);
        } else if (kind == HSD_DAT_REF_EXTERNAL &&
                   import->allow_unresolved_externals)
        {
            ++import->unresolved_external_count;
            import->external_symbol = external;
        } else if (kind != HSD_DAT_REF_NULL) {
            return fail(import, HSD_DAT_RELOCATION, at + 0x18);
        }
        scene->x1C = import_reference(import, at + 0x1C, DAT_FOG);

        u32 joint_count;
        if (!read_u32(import->dat, at + 0x24, &joint_count) ||
            joint_count > INT_MAX)
        {
            return fail(import, HSD_DAT_DESCRIPTOR, at + 0x24);
        }
        scene->unk24 = (s32) joint_count;
        kind = HSD_DatResolve(import->dat, at + 0x20, &target, &external);
        if (joint_count != 0) {
            if (kind != HSD_DAT_REF_LOCAL ||
                HSD_DatBytes(import->dat, target, (size_t) joint_count * 6) == NULL)
            {
                return fail(import, HSD_DAT_RELOCATION, at + 0x20);
            }
            scene->unk20 = HSD_DatImportAlloc(
                import, (size_t) joint_count * sizeof(*scene->unk20));
            if (scene->unk20 == NULL) {
                import->error_offset = target;
                return false;
            }
            for (u32 j = 0; j < joint_count; ++j) {
                read_s16(import->dat, target + j * 6, &scene->unk20[j].x);
                read_s16(import->dat, target + j * 6 + 2, &scene->unk20[j].y);
                read_s16(import->dat, target + j * 6 + 4, &scene->unk20[j].z);
            }
        } else if (kind != HSD_DAT_REF_NULL) {
            return fail(import, HSD_DAT_DESCRIPTOR, at + 0x20);
        }

        kind = HSD_DatResolve(import->dat, at + 0x28, &target, &external);
        if (kind == HSD_DAT_REF_LOCAL) {
            size_t flag_count = anim_count == 0 ? 1 : anim_count;
            const u8* flags = HSD_DatBytes(import->dat, target, flag_count);
            if (flags == NULL ||
                (scene->x28 = HSD_DatImportAlloc(import, flag_count)) == NULL)
            {
                import->error_offset = target;
                return false;
            }
            memcpy(scene->x28, flags, flag_count);
        } else if (kind == HSD_DAT_REF_EXTERNAL &&
                   import->allow_unresolved_externals)
        {
            ++import->unresolved_external_count;
            import->external_symbol = external;
        } else if (kind != HSD_DAT_REF_NULL) {
            return fail(import, HSD_DAT_RELOCATION, at + 0x28);
        }

        u32 hidden_count;
        if (!read_u32(import->dat, at + 0x30, &hidden_count) ||
            hidden_count > INT_MAX)
        {
            return fail(import, HSD_DAT_DESCRIPTOR, at + 0x30);
        }
        scene->x30 = (s32) hidden_count;
        kind = HSD_DatResolve(import->dat, at + 0x2C, &target, &external);
        if (hidden_count != 0) {
            if (kind != HSD_DAT_REF_LOCAL ||
                HSD_DatBytes(import->dat, target,
                             (size_t) hidden_count * sizeof(s16)) == NULL)
            {
                return fail(import, HSD_DAT_RELOCATION, at + 0x2C);
            }
            scene->x2C = HSD_DatImportAlloc(
                import, (size_t) hidden_count * sizeof(*scene->x2C));
            if (scene->x2C == NULL) {
                import->error_offset = target;
                return false;
            }
            for (u32 j = 0; j < hidden_count; ++j) {
                read_s16(import->dat, target + j * 2, &scene->x2C[j]);
            }
        } else if (kind != HSD_DAT_REF_NULL) {
            return fail(import, HSD_DAT_DESCRIPTOR, at + 0x2C);
        }
        if (import->status != HSD_DAT_OK) {
            return false;
        }
    }
    *result = scenes;
    return true;
}

static bool import_joint_maps(HSD_DatImport* import, u32 offset, u32 count,
                              void** result)
{
    if (count == 0) {
        *result = NULL;
        return true;
    }
    if (HSD_DatBytes(import->dat, offset, (size_t) count * 12) == NULL) {
        return fail(import, HSD_DAT_DESCRIPTOR, offset);
    }
    GroundJointMapSet* sets = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*sets));
    if (sets == NULL) {
        import->error_offset = offset;
        return false;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 at = offset + i * 12;
        u32 pair_count;
        u32 pair_offset;
        const char* external;
        sets[i].joint = import_reference(import, at, DAT_JOINT);
        if (sets[i].joint == NULL ||
            !read_u32(import->dat, at + 8, &pair_count) || pair_count > INT_MAX)
        {
            return fail(import, HSD_DAT_DESCRIPTOR, at);
        }
        sets[i].count = (s32) pair_count;
        if (pair_count == 0) {
            continue;
        }
        if (HSD_DatResolve(import->dat, at + 4, &pair_offset, &external) !=
                HSD_DAT_REF_LOCAL ||
            HSD_DatBytes(import->dat, pair_offset,
                         (size_t) pair_count * 4) == NULL)
        {
            return fail(import, HSD_DAT_RELOCATION, at + 4);
        }
        sets[i].entries = HSD_DatImportAlloc(
            import, (size_t) pair_count * sizeof(*sets[i].entries));
        if (sets[i].entries == NULL) {
            import->error_offset = pair_offset;
            return false;
        }
        for (u32 j = 0; j < pair_count; ++j) {
            read_s16(import->dat, pair_offset + j * 4, &sets[i].entries[j].a);
            read_s16(import->dat, pair_offset + j * 4 + 2,
                     &sets[i].entries[j].b);
            if (sets[i].entries[j].a < 0 || sets[i].entries[j].b < 0 ||
                sets[i].entries[j].b >= GROUND_JOINT_POINT_COUNT)
            {
                return fail(import, HSD_DAT_DESCRIPTOR,
                            pair_offset + j * 4);
            }
        }
    }
    *result = sets;
    return true;
}

static void* find_imported_object(const HSD_DatImport* import, u32 offset,
                                  HSD_DatImportType type)
{
    for (u32 i = 0; i < import->record_count; ++i) {
        const HSD_DatImportRecord* record = &import->records[i];
        if (record->offset == offset && record->type == type) {
            return record->object;
        }
    }
    return NULL;
}

static bool import_map_head(HSD_DatImport* import, u32 offset,
                            UnkStageDat* output,
                            const GrStageImportOptions* options)
{
    const char* external;
    u32 counts[6];
    u32 targets[6];
    static const u32 pointer_fields[6] = { 0, 8, 0x10, 0x18, 0x20, 0x28 };
    static const u32 count_fields[6] = { 4, 0xC, 0x14, 0x1C, 0x24, 0x2C };
    if (HSD_DatBytes(import->dat, offset, 0x30) == NULL) {
        return fail(import, HSD_DAT_DESCRIPTOR, offset);
    }
    for (u32 i = 0; i < 6; ++i) {
        HSD_DatReferenceKind kind = HSD_DatResolve(
            import->dat, offset + pointer_fields[i], &targets[i], &external);
        if (!read_u32(import->dat, offset + count_fields[i], &counts[i]) ||
            counts[i] > INT_MAX ||
            (counts[i] != 0 && kind != HSD_DAT_REF_LOCAL) ||
            (counts[i] == 0 && kind != HSD_DAT_REF_NULL))
        {
            return fail(import, HSD_DAT_DESCRIPTOR,
                        offset + pointer_fields[i]);
        }
    }
    output->unk4 = (s32) counts[0];
    if (!import_joint_maps(import, targets[0], counts[0], &output->unk0)) {
        return false;
    }
    output->unkC = (s32) counts[1];
    if (!import_scene_table(import, targets[1], counts[1], &output->unk8,
                            options)) {
        return false;
    }

    output->unk14 = (s32) counts[2];
    if (counts[2] != 0) {
        output->unk10 = (HSD_Spline**) HSD_DatImportAlloc(
            import, (size_t) counts[2] * sizeof(*output->unk10));
        if (output->unk10 == NULL) {
            import->error_offset = targets[2];
            return false;
        }
        for (u32 i = 0; i < counts[2]; ++i) {
            output->unk10[i] = import_reference(
                import, targets[2] + i * 4, DAT_SPLINE);
            if (output->unk10[i] == NULL) {
                return false;
            }
        }
    }

    output->unk1C = (s32) counts[3];
    if (counts[3] != 0) {
        LightOverrideEntry* overrides = HSD_DatImportAlloc(
            import, (size_t) counts[3] * sizeof(*overrides));
        if (overrides == NULL ||
            HSD_DatBytes(import->dat, targets[3], (size_t) counts[3] * 8) == NULL)
        {
            import->error_offset = targets[3];
            return false;
        }
        output->unk18 = overrides;
        for (u32 i = 0; i < counts[3]; ++i) {
            u32 desc_offset;
            const u8* flags = HSD_DatBytes(import->dat, targets[3] + i * 8 + 4, 1);
            HSD_DatReferenceKind kind = HSD_DatResolve(
                import->dat, targets[3] + i * 8, &desc_offset, &external);
            if (flags == NULL) {
                return fail(import, HSD_DAT_DESCRIPTOR,
                            targets[3] + i * 8 + 4);
            }
            if (kind == HSD_DAT_REF_LOCAL) {
                overrides[i].desc = find_imported_object(
                    import, desc_offset, DAT_LIGHT);
            }
            overrides[i].flags = *flags;
        }
    }

    output->unk24 = (s32) counts[4];
    if (counts[4] != 0) {
        output->unk20 = HSD_DatImportAlloc(
            import, (size_t) counts[4] * sizeof(*output->unk20));
        if (output->unk20 == NULL ||
            HSD_DatBytes(import->dat, targets[4], (size_t) counts[4] * 8) == NULL)
        {
            import->error_offset = targets[4];
            return false;
        }
        for (u32 i = 0; i < counts[4]; ++i) {
            const u8* flag = HSD_DatBytes(import->dat, targets[4] + i * 8 + 4, 1);
            output->unk20[i].unk0 = import_reference(
                import, targets[4] + i * 8, DAT_LIGHT_ANIM);
            output->unk20[i].flag = flag != NULL && (*flag & 0x80) != 0;
            if (output->unk20[i].unk0 == NULL) {
                return false;
            }
        }
    }

    output->unk2C = (s32) counts[5];
    if (counts[5] != 0) {
        output->unk28 = HSD_DatImportAlloc(
            import, (size_t) counts[5] * sizeof(*output->unk28));
        if (output->unk28 == NULL) {
            import->error_offset = targets[5];
            return false;
        }
        for (u32 i = 0; i < counts[5]; ++i) {
            u32 entry_offset;
            HSD_DatReferenceKind kind = HSD_DatResolve(
                import->dat, targets[5] + i * 4, &entry_offset, &external);
            if (kind == HSD_DAT_REF_NULL) {
                continue;
            }
            if (kind == HSD_DAT_REF_EXTERNAL &&
                import->allow_unresolved_externals)
            {
                ++import->unresolved_external_count;
                import->external_symbol = external;
                continue;
            }
            if (kind != HSD_DAT_REF_LOCAL ||
                HSD_DatBytes(import->dat, entry_offset, 8) == NULL)
            {
                return fail(import, HSD_DAT_RELOCATION, targets[5] + i * 4);
            }
            output->unk28[i] = HSD_DatImportAlloc(
                import, sizeof(*output->unk28[i]));
            if (output->unk28[i] == NULL ||
                !read_u32(import->dat, entry_offset + 4,
                          &output->unk28[i]->unk4))
            {
                import->error_offset = entry_offset;
                return false;
            }
        }
    }
    return import->status == HSD_DAT_OK;
}

static bool import_stage_param(const HSD_Dat* dat, u32 offset,
                               StageParam* output)
{
    u32 value;
    if (!read_u32(dat, offset, &value)) return false;
    output->stkind = (StKind) value;
    if (!read_u32(dat, offset + 4, &value)) return false;
    output->x4 = (s32) value;
    if (!read_u32(dat, offset + 8, &value)) return false;
    output->x8 = (s32) value;
    if (!read_u32(dat, offset + 0xC, &output->xC) ||
        !read_u32(dat, offset + 0x10, &output->x10) ||
        !read_s16(dat, offset + 0x14, &output->x14) ||
        !read_s16(dat, offset + 0x16, &output->x16) ||
        !read_s16(dat, offset + 0x18, &output->x18))
        return false;
    for (u32 i = 0; i < ARRAY_SIZE(output->x1A); ++i) {
        if (!read_s16(dat, offset + 0x1A + i * 2, &output->x1A[i])) return false;
    }
    return true;
}

static bool import_ground_param(HSD_DatImport* import, u32 offset,
                                GroundParam* output)
{
    const HSD_Dat* dat = import->dat;
    const char* external;
    u32 params_offset;
    u32 value;
    if (HSD_DatBytes(dat, offset, 0xDC) == NULL ||
        !HSD_DatReadF32(dat, offset, &output->y) ||
        !read_s16(dat, offset + 4, &output->x4) ||
        !read_s16(dat, offset + 8, &output->x8) ||
        !read_s16(dat, offset + 0xA, &output->xA))
        return fail(import, HSD_DAT_DESCRIPTOR, offset);
    memcpy(output->x6_pad, HSD_DatBytes(dat, offset + 6, 2), 2);
    if (!read_u32(dat, offset + 0xC, &value)) return false;
    output->xC = (s32) value;
    if (!read_u32(dat, offset + 0x10, &value)) return false;
    output->x10 = (s32) value;
    if (!read_u32(dat, offset + 0x14, &value)) return false;
    output->x14 = (s32) value;
    f32* floats0[] = { &output->x18, &output->x1C, &output->x20,
                       &output->x24, &output->x28 };
    for (u32 i = 0; i < ARRAY_SIZE(floats0); ++i)
        if (!HSD_DatReadF32(dat, offset + 0x18 + i * 4, floats0[i])) return false;
    memcpy(output->x2C_pad, HSD_DatBytes(dat, offset + 0x2C, 2), 2);
    if (!read_s16(dat, offset + 0x2E, &output->x2E)) return false;
    s32* ints[] = { &output->x30, &output->x34, &output->x38 };
    for (u32 i = 0; i < ARRAY_SIZE(ints); ++i) {
        if (!read_u32(dat, offset + 0x30 + i * 4, &value)) return false;
        *ints[i] = (s32) value;
    }
    f32* floats1[] = { &output->x3C, &output->x40, &output->x44, &output->x48 };
    for (u32 i = 0; i < ARRAY_SIZE(floats1); ++i)
        if (!HSD_DatReadF32(dat, offset + 0x3C + i * 4, floats1[i])) return false;
    const u8* fixed = HSD_DatBytes(dat, offset + 0x4C, 1);
    if (fixed == NULL) return false;
    output->x4C_fixed_cam = *fixed != 0;
    f32* floats2[] = { &output->x50, &output->x54, &output->x58,
                       &output->x5C, &output->x60, &output->x64 };
    for (u32 i = 0; i < ARRAY_SIZE(floats2); ++i)
        if (!HSD_DatReadF32(dat, offset + 0x50 + i * 4, floats2[i])) return false;
    if (!read_s16(dat, offset + 0x68, &output->x68)) return false;
    for (u32 i = 0; i < ARRAY_SIZE(output->x6A); ++i)
        if (!read_s16(dat, offset + 0x6A + i * 2, &output->x6A[i])) return false;
    u32 param_count;
    if (!read_u32(dat, offset + 0xB4, &param_count) || param_count > INT_MAX ||
        HSD_DatResolve(dat, offset + 0xB0, &params_offset, &external) !=
            (param_count == 0 ? HSD_DAT_REF_NULL : HSD_DAT_REF_LOCAL))
        return fail(import, HSD_DAT_DESCRIPTOR, offset + 0xB0);
    output->stage_param_count = (s32) param_count;
    if (param_count != 0) {
        output->stage_params = HSD_DatImportAlloc(
            import, (size_t) param_count * sizeof(*output->stage_params));
        if (output->stage_params == NULL ||
            HSD_DatBytes(dat, params_offset, (size_t) param_count * 0x64) == NULL)
        {
            import->error_offset = params_offset;
            return false;
        }
        for (u32 i = 0; i < param_count; ++i)
            if (!import_stage_param(dat, params_offset + i * 0x64,
                                    &output->stage_params[i])) return false;
    }
    GXColor* colors[] = { &output->xB8, &output->xBC, &output->xC0,
                          &output->xC4, &output->xC8, &output->xCC,
                          &output->xD0, &output->xD4, &output->xD8 };
    for (u32 i = 0; i < ARRAY_SIZE(colors); ++i)
        memcpy(colors[i], HSD_DatBytes(dat, offset + 0xB8 + i * 4, 4), 4);
    return true;
}

static struct GroundItemData** import_stage_items(
    HSD_DatImport* import, u32 offset, ItArticleImportStats* stats,
    ItScriptCollect* scripts)
{
    const char* external;
    u32 count;
    u32 target;
    if (!pointer_list_count(import->dat, offset, &count))
        return fail(import, HSD_DAT_DESCRIPTOR, offset), NULL;
    struct GroundItemData** result = HSD_DatImportAlloc(
        import, (size_t) (count + 1) * sizeof(*result));
    if (result == NULL) {
        import->error_offset = offset;
        return NULL;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 article_source;
        u32 kind;
        if (HSD_DatResolve(import->dat, offset + i * 4, &target,
                           &external) != HSD_DAT_REF_LOCAL ||
            !read_u32(import->dat, target, &kind) ||
            HSD_DatResolve(import->dat, target + 4, &article_source,
                           &external) != HSD_DAT_REF_LOCAL)
        {
            return fail(import, HSD_DAT_DESCRIPTOR, offset + i * 4), NULL;
        }
        struct GroundItemData* record = HSD_DatImportAlloc(
            import, sizeof(*record));
        if (record == NULL) {
            import->error_offset = target;
            return NULL;
        }
        record->unk0 = (s32) kind;
        record->unk4 = itArticle_Import(import, article_source, stats, scripts,
                                        false, (s32) kind, NULL, 0);
        if (record->unk4 == NULL) return NULL;
        result[i] = record;
    }
    return result;
}

static union CmdUnion** import_stage_item_scripts(HSD_DatImport* import,
                                                   u32 offset,
                                                   ItScriptCollect* scripts,
                                                   const Article* article)
{
    const char* external;
    u32 target;
    if (HSD_DatResolve(import->dat, offset, &target, &external) !=
        HSD_DAT_REF_NULL)
        return fail(import, HSD_DAT_DESCRIPTOR, offset), NULL;
    u32 count;
    for (count = 1; count < 64; ++count) {
        HSD_DatReferenceKind kind = HSD_DatResolve(
            import->dat, offset + count * 4, &target, &external);
        if (kind == HSD_DAT_REF_NULL) break;
        if (kind != HSD_DAT_REF_LOCAL)
            return fail(import, HSD_DAT_RELOCATION, offset + count * 4), NULL;
    }
    if (count == 64 || scripts->capacity - scripts->count < count - 1)
        return fail(import, HSD_DAT_CAPACITY, offset), NULL;
    union CmdUnion** result = HSD_DatImportAlloc(
        import, (size_t) (count + 1) * sizeof(*result));
    if (result == NULL) {
        import->error_offset = offset;
        return NULL;
    }
    for (u32 i = 1; i < count; ++i) {
        if (HSD_DatResolve(import->dat, offset + i * 4, &target,
                           &external) != HSD_DAT_REF_LOCAL)
            return fail(import, HSD_DAT_RELOCATION, offset + i * 4), NULL;
        scripts->entries[scripts->count++] = (ItScriptEntry) {
            .destination=&result[i], .source=target, .article=article
        };
    }
    return result;
}

static void* import_words(HSD_DatImport* import, u32 offset,
                          const GrStageImportOptions* options)
{
    const size_t source_size = options->yakumono_size;
    const size_t pointer_align = sizeof(void*);
    size_t native_size = source_size;
    size_t source_cursor = 0;
    size_t native_cursor = 0;
    if ((source_size & 3) != 0 ||
        HSD_DatBytes(import->dat, offset, source_size) == NULL)
        return fail(import, HSD_DAT_DESCRIPTOR, offset), NULL;
    for (u32 i = 0; i < import->dat->relocation_count; ++i) {
        u32 slot;
        if (!HSD_DatRelocation(import->dat, i, &slot)) {
            return fail(import, HSD_DAT_RELOCATION, offset), NULL;
        }
        if (slot < offset || (size_t) (slot - offset) >= source_size) {
            continue;
        }
        bool declared = false;
        for (u32 field = 0; field < options->yakumono_pointer_count; ++field) {
            if (slot - offset == options->yakumono_pointers[field].offset) {
                declared = true;
                break;
            }
        }
        if (!declared) {
            return fail(import, HSD_DAT_RELOCATION, slot), NULL;
        }
    }
    for (u32 i = 0; i < options->yakumono_pointer_count; ++i) {
        size_t at = options->yakumono_pointers[i].offset;
        if ((at & 3) != 0 || at < source_cursor || at + 4 > source_size) {
            return fail(import, HSD_DAT_DESCRIPTOR, offset + (u32) at), NULL;
        }
        native_cursor += at - source_cursor;
        native_cursor = (native_cursor + pointer_align - 1) &
                        ~(pointer_align - 1);
        native_cursor += sizeof(void*);
        source_cursor = at + 4;
    }
    native_cursor += source_size - source_cursor;
    if (options->yakumono_pointer_count != 0) {
        native_cursor = (native_cursor + pointer_align - 1) &
                        ~(pointer_align - 1);
        native_size = native_cursor;
    }
    u8* output = HSD_DatImportAlloc(import, native_size);
    if (output == NULL) {
        import->error_offset = offset;
        return NULL;
    }
    memset(output, 0, native_size);
    source_cursor = 0;
    native_cursor = 0;
    for (u32 field = 0; field <= options->yakumono_pointer_count;
         ++field)
    {
        size_t field_offset = field < options->yakumono_pointer_count
            ? options->yakumono_pointers[field].offset : source_size;
        size_t scalar_size = field_offset - source_cursor;
        for (size_t i = 0; i < scalar_size; i += 4) {
            u32 value;
            if (!read_u32(import->dat,
                          offset + (u32) source_cursor + (u32) i, &value))
                return NULL;
            memcpy(output + native_cursor + i, &value, 4);
        }
        native_cursor += scalar_size;
        source_cursor = field_offset;
        if (field == options->yakumono_pointer_count) break;
        native_cursor = (native_cursor + pointer_align - 1) &
                        ~(pointer_align - 1);
        u32 target;
        const char* external;
        u32 slot = offset + (u32) source_cursor;
        if (HSD_DatResolve(import->dat, slot, &target, &external) !=
            HSD_DAT_REF_LOCAL)
        {
            return fail(import, HSD_DAT_RELOCATION, slot), NULL;
        }
        void* pointer = NULL;
        switch (options->yakumono_pointers[field].kind) {
        case GrStageParamPointer_ColorScript: {
            u32 end = HSD_DatNextTarget(import->dat, target);
            if (end <= target || ((end - target) & 3) != 0) {
                return fail(import, HSD_DAT_DESCRIPTOR, target), NULL;
            }
            u32 capacity = (end - target) / 4;
            union ColorOverlay_x8_t* commands = HSD_DatImportAlloc(
                import, (size_t) capacity * sizeof(*commands));
            union ColorOverlay_x8_t* script;
            u32 command_count;
            if (commands == NULL ||
                !lbColorScript_ImportDirect(
                    import->dat, slot, end, commands, capacity,
                    &command_count, &script, &import->status,
                    &import->error_offset))
            {
                if (commands == NULL) import->error_offset = target;
                return NULL;
            }
            pointer = script;
            break;
        }
        case GrStageParamPointer_EnvironmentHit: {
            lbColl_80008D30_arg1* hit = HSD_DatImportAlloc(
                import, sizeof(*hit));
            if (hit == NULL) {
                import->error_offset = target;
                return NULL;
            }
            u32* words = (u32*) hit;
            for (size_t i = 0; i < sizeof(*hit) / sizeof(*words); ++i) {
                if (!read_u32(import->dat, target + (u32) i * 4,
                              &words[i]))
                {
                    return NULL;
                }
            }
            pointer = hit;
            break;
        }
        default:
            return fail(import, HSD_DAT_DESCRIPTOR, slot), NULL;
        }
        memcpy(output + native_cursor, &pointer, sizeof(pointer));
        native_cursor += sizeof(pointer);
        source_cursor += 4;
    }
    if (options->yakumono_halfword_range_offset > source_size ||
        options->yakumono_halfword_range_count >
            (source_size - options->yakumono_halfword_range_offset) / 2 ||
        options->yakumono_halfword_count >
            ~(u32) 0 - options->yakumono_halfword_range_count)
    {
        return fail(import, HSD_DAT_DESCRIPTOR, offset), NULL;
    }
    u32 halfword_count = options->yakumono_halfword_count +
                         options->yakumono_halfword_range_count;
    for (u32 i = 0; i < halfword_count; ++i) {
        size_t at = i < options->yakumono_halfword_count
            ? options->yakumono_halfword_offsets[i]
            : options->yakumono_halfword_range_offset +
                  (size_t) (i - options->yakumono_halfword_count) * 2;
        u16 value;
        if ((at & 1) != 0 || (size_t) at + 2 > source_size ||
            !HSD_DatReadU16(import->dat, offset + at, &value))
            return fail(import, HSD_DAT_DESCRIPTOR, offset + at), NULL;
        size_t source_at = 0;
        size_t native_at = 0;
        for (u32 field = 0; field < options->yakumono_pointer_count;
             ++field)
        {
            size_t pointer_at = options->yakumono_pointers[field].offset;
            if (at < pointer_at) break;
            if (at < pointer_at + 4)
                return fail(import, HSD_DAT_DESCRIPTOR, offset + at), NULL;
            native_at += pointer_at - source_at;
            native_at = (native_at + pointer_align - 1) &
                        ~(pointer_align - 1);
            native_at += sizeof(void*);
            source_at = pointer_at + 4;
        }
        native_at += at - source_at;
        memcpy(output + native_at, &value, sizeof(value));
    }
    return output;
}

static GrIceMtYakumonoParam* import_icicle_mountain_param(
    HSD_DatImport* import, u32 offset)
{
    const HSD_Dat* dat = import->dat;
    if (HSD_DatBytes(dat, offset, 0x13C) == NULL) {
        fail(import, HSD_DAT_DESCRIPTOR, offset);
        return NULL;
    }
    GrIceMtYakumonoParam* result = HSD_DatImportAlloc(import, sizeof(*result));
    if (result == NULL) {
        import->error_offset = offset;
        return NULL;
    }
    // The scalar prefix keeps its original offsets. Only the three pointer
    // fields and the records after them need a different native layout.
    for (u32 at = 0; at < 0xAC; at += 4) {
        u32 value;
        if (!read_u32(dat, offset + at, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, offset + at);
            return NULL;
        }
        memcpy((u8*) result + at, &value, sizeof(value));
    }
    static const u8 halfwords[] = {
        0, 2, 0x34, 0x36, 0x38, 0x3A, 0x98, 0x9A,
        0xA4, 0xA6, 0xA8, 0xAA,
    };
    for (u32 i = 0; i < ARRAY_SIZE(halfwords); ++i) {
        u16 value;
        u32 at = halfwords[i];
        if (!HSD_DatReadU16(dat, offset + at, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, offset + at);
            return NULL;
        }
        memcpy((u8*) result + at, &value, sizeof(value));
    }
    s16** arrays[] = { &result->field_ixs, &result->xB0, &result->xB4 };
    static const u8 lengths[] = { 16, 12, 12 };
    const char* external;
    for (u32 i = 0; i < ARRAY_SIZE(arrays); ++i) {
        u32 source;
        u32 slot = offset + 0xAC + i * 4;
        if (HSD_DatResolve(dat, slot, &source, &external) !=
                HSD_DAT_REF_LOCAL ||
            HSD_DatBytes(dat, source, lengths[i] * sizeof(s16)) == NULL ||
            (i < 2 && HSD_DatNextTarget(dat, source) !=
                           source + lengths[i] * sizeof(s16)) ||
            (i == 2 && source + lengths[i] * sizeof(s16) != offset))
        {
            fail(import, HSD_DAT_RELOCATION, slot);
            return NULL;
        }
        *arrays[i] = HSD_DatImportAlloc(import,
                                        lengths[i] * sizeof(s16));
        if (*arrays[i] == NULL) {
            import->error_offset = source;
            return NULL;
        }
        for (u32 j = 0; j < lengths[i]; ++j) {
            if (!read_s16(dat, source + j * 2, &(*arrays[i])[j])) {
                fail(import, HSD_DAT_DESCRIPTOR, source + j * 2);
                return NULL;
            }
        }
    }
    if (!read_s16(dat, offset + 0xB8, &result->xB8) ||
        !read_s16(dat, offset + 0xBA, &result->pad))
    {
        fail(import, HSD_DAT_DESCRIPTOR, offset + 0xB8);
        return NULL;
    }
    for (u32 i = 0; i < ARRAY_SIZE(result->xBC); ++i) {
        u32 at = offset + 0xBC + i * 4;
        const u8* bytes = HSD_DatBytes(dat, at, 4);
        if (bytes == NULL ||
            !HSD_DatReadU16(dat, at, &result->xBC[i].kind))
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
        result->xBC[i].x2 = bytes[2];
        result->xBC[i].respawn = bytes[3];
    }
    return result;
}

static DynamicsDesc* import_dynamics(HSD_DatImport* import, u32 root)
{
    const HSD_Dat* dat = import->dat;
    const char* external;
    u32 source, count;
    if (HSD_DatResolve(dat, root, &source, &external) != HSD_DAT_REF_LOCAL ||
        !HSD_DatReadU32(dat, root + 4, &count) || count == 0 ||
        count > dat->data_size / sizeof(struct lb_00F9_UnkDesc1Inner) ||
        HSD_DatBytes(dat, source,
                     (size_t) count * sizeof(struct lb_00F9_UnkDesc1Inner)) ==
            NULL)
    {
        fail(import, HSD_DAT_DESCRIPTOR, root);
        return NULL;
    }
    DynamicsDesc* result = HSD_DatImportAlloc(import, sizeof(*result));
    size_t source_size = (size_t) count * sizeof(struct lb_00F9_UnkDesc1Inner);
    size_t native_size = source_size > sizeof(struct DynamicsData)
        ? source_size : sizeof(struct DynamicsData);
    u8* data = HSD_DatImportAlloc(import, native_size);
    if (result == NULL || data == NULL) {
        import->error_offset = root;
        return NULL;
    }
    for (size_t i = 0; i < source_size; i += 4) {
        u32 value;
        if (!HSD_DatReadU32(dat, source + (u32) i, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, source + (u32) i);
            return NULL;
        }
        memcpy(data + i, &value, sizeof(value));
    }
    result->data = (struct DynamicsData*) data;
    result->count = count;
    if (!HSD_DatReadF32(dat, root + 8, &result->pos.x) ||
        !HSD_DatReadF32(dat, root + 12, &result->pos.y) ||
        !HSD_DatReadF32(dat, root + 16, &result->pos.z))
    {
        fail(import, HSD_DAT_DESCRIPTOR, root + 8);
        return NULL;
    }
    return result;
}

DynamicsDesc* grStageAssets_ImportDynamics(GrStageAssets* assets,
                                          const char* symbol)
{
    u32 root;
    if (!HSD_DatFindRoot(&assets->dat, symbol, &root)) {
        fail(&assets->import, HSD_DAT_SYMBOL, 0);
        return NULL;
    }
    return import_dynamics(&assets->import, root);
}

void** grStageAssets_ImportSis(GrStageAssets* assets, const char* symbol,
                               u32* count)
{
    u32 root;
    u32 entry_count;
    if (assets == NULL || symbol == NULL ||
        !HSD_DatFindRoot(&assets->dat, symbol, &root) ||
        !HSD_DatSisLayout(&assets->dat, root, &entry_count))
    {
        return NULL;
    }
    for (u32 i = 0; i < assets->sis_root_count; ++i) {
        if (assets->sis_roots[i].offset != root) continue;
        if (count != NULL) *count = assets->sis_roots[i].count;
        return assets->sis_roots[i].table;
    }
    if (assets->sis_root_count == ARRAY_SIZE(assets->sis_roots)) return NULL;
    void** table = HSD_DatImportAlloc(
        &assets->import, (size_t) entry_count * sizeof(*table));
    if (table == NULL ||
        !HSD_DatImportSis(&assets->dat, root, table, entry_count))
    {
        return NULL;
    }
    u32 index = assets->sis_root_count++;
    assets->sis_roots[index].offset = root;
    assets->sis_roots[index].count = entry_count;
    assets->sis_roots[index].table = table;
    if (count != NULL) {
        *count = entry_count;
    }
    return table;
}

bool grStageAssets_Import(const void* bytes, size_t size, void* arena,
                          size_t arena_size, u32 record_capacity,
                          const GrStageImportOptions* options,
                          GrStageAssets* output)
{
    u32 ald_root, coll_root, ground_root, item_root, map_root;
    u32 particle_root, texture_root, yakumono_root, light_root, quake_root;
    u32 vertex_count, line_count, joint_count;
    ItScriptEntry script_entries[128];
    ItScriptCollect scripts = { script_entries, 0, ARRAY_SIZE(script_entries) };
    memset(output, 0, sizeof(*output));
    if (options == NULL ||
        (options->animation_layout_count != 0 &&
         options->animation_layouts == NULL) ||
        (options->yakumono_halfword_count != 0 &&
         options->yakumono_halfword_offsets == NULL) ||
        (options->yakumono_pointer_count != 0 &&
         options->yakumono_pointers == NULL) ||
        HSD_DatOpen(&output->dat, bytes, size) != HSD_DAT_OK ||
        HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&output->dat, "map_head", &map_root) ||
        !HSD_DatFindRoot(&output->dat, "coll_data", &coll_root) ||
        !HSD_DatFindRoot(&output->dat, "grGroundParam", &ground_root) ||
        !HSD_DatFindRoot(&output->dat, "itemdata", &item_root) ||
        !HSD_DatFindRoot(&output->dat, "ALDYakuAll", &ald_root) ||
        !HSD_DatFindRoot(&output->dat, "yakumono_param", &yakumono_root))
        return false;
    output->import.allow_unresolved_externals = true;
    bool has_light = HSD_DatFindRoot(&output->dat, "map_plit", &light_root);
    bool has_quake =
        HSD_DatFindRoot(&output->dat, "quake_model_set", &quake_root);
    if ((!options->stage_fragment && (!has_light || !has_quake)) ||
        (has_light &&
         (output->map_plit =
              import_light_list(&output->import, light_root)) == NULL) ||
        !import_map_head(&output->import, map_root, &output->map_head,
                         options) ||
        !read_u32(&output->dat, coll_root + 4, &vertex_count) ||
        !read_u32(&output->dat, coll_root + 12, &line_count) ||
        !read_u32(&output->dat, coll_root + 40, &joint_count))
        return false;
    Vec2* vertices = HSD_DatImportAlloc(
        &output->import, (size_t) vertex_count * sizeof(*vertices));
    MapLine* lines = HSD_DatImportAlloc(
        &output->import, (size_t) line_count * sizeof(*lines));
    MapJoint* joints = HSD_DatImportAlloc(
        &output->import, (size_t) joint_count * sizeof(*joints));
    if (vertices == NULL || lines == NULL || joints == NULL ||
        mpDecodeCollision(&output->dat, coll_root, &output->collision,
                          vertices, vertex_count, lines, line_count, joints,
                          joint_count) != HSD_DAT_OK ||
        !import_ground_param(&output->import, ground_root,
                             &output->ground_param))
        return false;
    output->itemdata = import_stage_items(&output->import, item_root,
                                          &output->item_article_stats,
                                          &scripts);
    output->ald_yaku_all = import_stage_item_scripts(&output->import, ald_root,
                                                      &scripts,options->stage_item_script_article);
    output->yakumono_param = options->icicle_mountain_param
        ? import_icicle_mountain_param(&output->import, yakumono_root)
        : import_words(&output->import, yakumono_root, options);
    if (output->itemdata == NULL || output->ald_yaku_all == NULL ||
        output->yakumono_param == NULL)
        return false;
    if (has_quake) {
        output->quake_model_set = HSD_DatImportAlloc(
            &output->import, sizeof(*output->quake_model_set));
        if (output->quake_model_set == NULL ||
            !import_dynamic_model(&output->import, quake_root,
                                  output->quake_model_set, NULL, NULL))
            return false;
    }
    bool has_particle = HSD_DatFindRoot(&output->dat, "map_ptcl", &particle_root);
    bool has_texture = HSD_DatFindRoot(&output->dat, "map_texg", &texture_root);
    if (has_particle != has_texture ||
        (has_particle && !HSD_DatImportParticleBank(
            &output->import, particle_root, texture_root,
            &output->particle_bank)))
        return false;
    // Text tables are stage assets too. Import all public SIS roots during
    // loading; conversation-time lookups reuse the fixed cached entries.
    for (u32 i = 0; i < output->dat.root_count; ++i) {
        u32 offset;
        const char* name;
        if (!HSD_DatSymbol(&output->dat, false, i, &offset, &name)) return false;
        if (strncmp(name, "SIS_", 4) == 0 &&
            grStageAssets_ImportSis(output, name, NULL) == NULL)
            return false;
    }
    return output->import.status == HSD_DAT_OK &&
           itCommandData_Import(&output->import, &scripts,
                                &output->item_command_data);
}
