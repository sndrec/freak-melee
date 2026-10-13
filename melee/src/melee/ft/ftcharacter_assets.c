#include "ftcharacter_assets.h"

#include "dobjlist.h"
#include "ftdat.h"
#include "ftdata.h"

#include <melee/it/itCharItems.h>
#include <melee/it/article_model_layers.h>
#include <melee/it/itYoyo.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>

#include <limits.h>
#include <stddef.h>
#include <string.h>

static bool fail(HSD_DatImport* import, HSD_DatResult status, u32 offset)
{
    import->status = status;
    import->error_offset = offset;
    return false;
}

static HSD_DatReferenceKind resolve(const HSD_Dat* dat, u32 slot,
                                    u32* target)
{
    const char* external;
    return HSD_DatResolve(dat, slot, target, &external);
}

static bool local(const HSD_Dat* dat, u32 slot, u32* target)
{
    return resolve(dat, slot, target) == HSD_DAT_REF_LOCAL;
}

static bool read_s32(const HSD_Dat* dat, u32 offset, s32* output)
{
    u32 value;
    if (!HSD_DatReadU32(dat, offset, &value)) {
        return false;
    }
    *output = (s32) value;
    return true;
}

static bool read_int(const HSD_Dat* dat, u32 offset, int* output)
{
    s32 value;
    if (!read_s32(dat, offset, &value)) {
        return false;
    }
    *output = (int) value;
    return true;
}

static bool read_f32_words(const HSD_Dat* dat, u32 offset, void* output,
                           size_t size)
{
    u8* dst = output;
    if ((size & 3) != 0 || HSD_DatBytes(dat, offset, size) == NULL) {
        return false;
    }
    for (size_t i = 0; i < size; i += 4) {
        if (!HSD_DatReadF32(dat, offset + (u32) i, (f32*) (dst + i))) {
            return false;
        }
    }
    return true;
}

static bool read_u32_words(const HSD_Dat* dat, u32 offset, void* output,
                           size_t size)
{
    u8* dst = output;
    if ((size & 3) != 0 || HSD_DatBytes(dat, offset, size) == NULL) {
        return false;
    }
    for (size_t i = 0; i < size; i += 4) {
        u32 value;
        if (!HSD_DatReadU32(dat, offset + (u32) i, &value)) {
            return false;
        }
        memcpy(dst + i, &value, sizeof(value));
    }
    return true;
}

static void* import_words(HSD_DatImport* import, u32 offset, size_t size)
{
    u8* output;
    u32 value;
    if ((size & 3) != 0 || HSD_DatBytes(import->dat, offset, size) == NULL ||
        (output = HSD_DatImportAlloc(import, size)) == NULL)
    {
        fail(import, import->status == HSD_DAT_OK ? HSD_DAT_DESCRIPTOR
                                                  : import->status,
             offset);
        return NULL;
    }
    for (size_t i = 0; i < size; i += 4) {
        if (!HSD_DatReadU32(import->dat, offset + (u32) i, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, offset + (u32) i);
            return NULL;
        }
        memcpy(output + i, &value, 4);
    }
    return output;
}

static FtPartsVisLookup* import_visibility_lookups(HSD_DatImport* import,
                                                   u32 source, u32 count)
{
    if (HSD_DatBytes(import->dat, source, (size_t) count * 8) == NULL) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    FtPartsVisLookup* lookups = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*lookups));
    if (lookups == NULL) {
        import->error_offset = source;
        return NULL;
    }
    for (u32 model = 0; model < count; ++model) {
        s32 variant_count;
        u32 variants_source;
        u32 at = source + model * 8;
        if (!read_s32(import->dat, at, &variant_count) ||
            variant_count < 0 || variant_count > 256)
        {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
        lookups[model].x0 = variant_count;
        if (variant_count == 0) {
            continue;
        }
        if (!local(import->dat, at + 4, &variants_source) ||
            HSD_DatBytes(import->dat, variants_source,
                         (size_t) variant_count * 8) == NULL)
        {
            fail(import, HSD_DAT_RELOCATION, at + 4);
            return NULL;
        }
        TempS* variants = HSD_DatImportAlloc(
            import, (size_t) variant_count * sizeof(*variants));
        if (variants == NULL) {
            import->error_offset = variants_source;
            return NULL;
        }
        lookups[model].x4 = variants;
        for (s32 i = 0; i < variant_count; ++i) {
            s32 draw_count;
            u32 draws_source;
            u32 variant = variants_source + (u32) i * 8;
            if (!read_s32(import->dat, variant, &draw_count) ||
                draw_count < 0 || draw_count > 256)
            {
                fail(import, HSD_DAT_DESCRIPTOR, variant);
                return NULL;
            }
            variants[i].x0 = draw_count;
            if (draw_count != 0) {
                if (!local(import->dat, variant + 4, &draws_source)) {
                    fail(import, HSD_DAT_RELOCATION, variant + 4);
                    return NULL;
                }
                const u8* draws = HSD_DatBytes(
                    import->dat, draws_source, (size_t) draw_count);
                if (draws == NULL ||
                    (variants[i].x4 = HSD_DatImportAlloc(
                         import, (size_t) draw_count)) == NULL)
                {
                    import->error_offset = draws_source;
                    return NULL;
                }
                memcpy(variants[i].x4, draws, (size_t) draw_count);
            }
        }
    }
    return lookups;
}

static unsigned count_model_draws(const HSD_Joint* joint)
{
    unsigned count = 0;
    for (; joint != NULL; joint = joint->next) {
        for (const HSD_DObjDesc* draw = union_type_dobj(joint)
                                          ? joint->u.dobjdesc : NULL;
             draw != NULL;
             draw = draw->next)
            ++count;
        if (!(joint->flags & JOBJ_INSTANCE))
            count += count_model_draws(joint->child);
    }
    return count;
}

bool ftCharacterAssets_ValidateVisibility(const FtCharacterAssets* assets,
                                         uint16_t visibility_mask,
                                         const HSD_Joint* costume)
{
    if (assets == NULL || costume == NULL) return false;
    const unsigned groups = assets->model_data.x0.model_num;
    if (groups > 11 || (visibility_mask != 0 && groups == 0)) return false;
    const unsigned draws = count_model_draws(costume);
    for (unsigned slot = 0; slot < FT_CHARACTER_ITEM_CAPACITY; ++slot) {
        if ((visibility_mask & (1u << slot)) == 0) continue;
        const FtPartsVisLookup* lookup = assets->visibility_assets[slot];
        if (lookup == NULL) return false;
        for (unsigned group = 0; group < groups; ++group) {
            if (lookup[group].x0 < 0 || lookup[group].x0 > 256 ||
                (lookup[group].x0 != 0 && lookup[group].x4 == NULL)) return false;
            for (int variant = 0; variant < lookup[group].x0; ++variant) {
                const TempS* selection = &lookup[group].x4[variant];
                if (selection->x0 < 0 || selection->x0 > 256 ||
                    (selection->x0 != 0 && selection->x4 == NULL)) return false;
                for (int i = 0; i < selection->x0; ++i)
                    if (selection->x4[i] >= draws) return false;
            }
        }
    }
    return true;
}

static bool validate_metal_visibility(FtCharacterAssets* assets, u32 costumes,
                                      u32 source)
{
    unsigned draws = count_model_draws(assets->metal_model);
    FtPartsDesc* model = &assets->model_data.x0;
    for (u32 costume = 0; costume < costumes; ++costume) {
        // Group 2 addresses the separate metal DObj list, not the costume.
        FtPartsVisLookup* groups = model->vis_table[costume][2];
        if (groups == NULL) continue;
        for (u32 group = 0; group < model->model_num; ++group) {
            for (int variant = 0; variant < groups[group].x0; ++variant) {
                const TempS* selection = &groups[group].x4[variant];
                for (int i = 0; i < selection->x0; ++i) {
                    if (selection->x4[i] >= draws)
                        return fail(&assets->import, HSD_DAT_DESCRIPTOR,
                                    source);
                }
            }
        }
    }
    return true;
}

static bool import_visibility(HSD_DatImport* import, u32 source,
                              u32 model_count, u32 costume_count,
                              void* (**result)[4])
{
    void*(*table)[4] = HSD_DatImportAlloc(
        import, (size_t) costume_count * sizeof(*table));
    if (table == NULL) {
        import->error_offset = source;
        return false;
    }
    for (u32 costume = 0; costume < costume_count; ++costume) {
        for (u32 group = 0; group < 4; ++group) {
            u32 lookup_source;
            u32 slot = source + (costume * 4 + group) * 4;
            HSD_DatReferenceKind kind = resolve(import->dat, slot,
                                                &lookup_source);
            if (kind == HSD_DAT_REF_NULL) {
                continue;
            }
            if (kind != HSD_DAT_REF_LOCAL) {
                return fail(import, HSD_DAT_RELOCATION, slot);
            }
            FtPartsVisLookup* lookups = import_visibility_lookups(
                import, lookup_source, model_count);
            if (lookups == NULL) {
                return false;
            }
            table[costume][group] = lookups;
        }
    }
    *result = table;
    return true;
}

static bool import_texture_indices(HSD_DatImport* import, u32 source,
                                   u32 texture_count, u32 costume_count,
                                   u16*** result)
{
    u16** table = HSD_DatImportAlloc(
        import, (size_t) costume_count * sizeof(*table));
    if (table == NULL) {
        import->error_offset = source;
        return false;
    }
    for (u32 costume = 0; costume < costume_count; ++costume) {
        u32 indices_source;
        HSD_DatReferenceKind kind = resolve(
            import->dat, source + costume * 4, &indices_source);
        if (kind == HSD_DAT_REF_NULL) {
            continue;
        }
        if (kind != HSD_DAT_REF_LOCAL ||
            HSD_DatBytes(import->dat, indices_source,
                         (size_t) texture_count * 2) == NULL ||
            (table[costume] = HSD_DatImportAlloc(
                 import, (size_t) texture_count * sizeof(*table[costume]))) ==
                NULL)
        {
            return fail(import, HSD_DAT_RELOCATION,
                        source + costume * 4);
        }
        for (u32 i = 0; i < texture_count; ++i) {
            HSD_DatReadU16(import->dat, indices_source + i * 2,
                           &table[costume][i]);
        }
    }
    *result = table;
    return true;
}

static bool import_model_data(HSD_DatImport* import, u32 source,
                              u32 costume_count, struct ftData_x8* output)
{
    u32 visibility_source;
    u32 texture_source;
    if (!HSD_DatReadU32(import->dat, source, &output->x0.model_num) ||
        output->x0.model_num > 11 ||
        !local(import->dat, source + 4, &visibility_source) ||
        !HSD_DatReadU32(import->dat, source + 8, &output->x8.x8) ||
        output->x8.x8 > 5 ||
        !local(import->dat, source + 12, &texture_source))
    {
        return false;
    }
    // Model variants belong to the DAT, independently of how many costume
    // assets the package installs. Keep every row for later imported skins.
    u32 visibility_bytes = HSD_DatNextTarget(import->dat, visibility_source) - visibility_source;
    u32 texture_bytes = HSD_DatNextTarget(import->dat, texture_source) - texture_source;
    u32 visibility_count = visibility_bytes / 16;
    // Both tables index the same variants. Edited scripts may leave an
    // unreferenced gap after the texture table, so its next target is only
    // an upper bound, not a row count.
    u32 texture_count = visibility_count;
    if (visibility_bytes % 16 != 0 || texture_bytes < texture_count * 4 ||
        visibility_count < costume_count || visibility_count > 8 ||
        texture_count < costume_count)
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    output->x0.costume_count = visibility_count;
    output->x8.costume_count = texture_count;
    if (!import_visibility(import, visibility_source, output->x0.model_num,
                           visibility_count, &output->x0.vis_table) ||
        !import_texture_indices(import, texture_source, output->x8.x8,
                                texture_count, &output->x8.xC))
        return false;
    const u8* parts = HSD_DatBytes(import->dat, source + 16, 5);
    if (parts == NULL) {
        return fail(import, HSD_DAT_DESCRIPTOR, source + 16);
    }
    output->x10 = parts[0];
    output->x11 = parts[1];
    output->x12 = parts[2];
    output->x13 = parts[3];
    output->x14 = parts[4];
    return true;
}

static bool import_wait_table(HSD_DatImport* import, u32 source, u32 count,
                              struct Fighter_WaitAnimData** result,
                              FtActionDataStats* stats, bool action_scripts)
{
    struct Fighter_WaitAnimData* table = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*table));
    if (table == NULL ||
        HSD_DatBytes(import->dat, source, (size_t) count * 24) == NULL)
    {
        import->error_offset = source;
        return false;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 at = source + i * 24;
        u32 target;
        HSD_DatReferenceKind kind = resolve(import->dat, at, &target);
        if (kind == HSD_DAT_REF_LOCAL) {
            table[i].x0 = (char*) HSD_DatBytes(import->dat, target, 1);
        } else if (kind != HSD_DAT_REF_NULL) {
            return fail(import, HSD_DAT_RELOCATION, at);
        }
        if (!read_s32(import->dat, at + 4, &table[i].x4) ||
            !read_s32(import->dat, at + 8, &table[i].x8) ||
            !read_s32(import->dat, at + 16,
                      &table[i].x10_animCurrFlags))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, at);
        }
#ifdef MELEE_NATIVE
        // The source word uses PowerPC bitfield order. Store each field in
        // Fighter's native layout before action-state code reads the union.
        u32 source_flags = (u32) table[i].x10_animCurrFlags;
        u32 native_flags = ((source_flags >> 6) & 7) << 8;
        native_flags |= ((source_flags >> 9) & 0x1FFF) << 11;
        native_flags |= (source_flags & 0x3F) << 26;
        for (u32 bit = 0; bit < 8; ++bit) {
            native_flags |= ((source_flags >> (31 - bit)) & 1) << bit;
        }
        table[i].x10_animCurrFlags = (s32) native_flags;
#endif
        kind = resolve(import->dat, at + 12, &target);
        if (kind == HSD_DAT_REF_LOCAL) {
            table[i].xC = NULL;
        } else if (kind != HSD_DAT_REF_NULL) {
            return fail(import, HSD_DAT_RELOCATION, at + 12);
        }
        table[i].x14 = 0;
    }
    *result = table;
    return !action_scripts ||
           ftActionData_Import(import, source, count, table, stats);
}

static bool import_wait_choices(HSD_DatImport* import, u32 source,
                                WaitStruct** result)
{
    u32 count = 0;
    s32 animation;
    do {
        if (count == 256 ||
            !read_s32(import->dat, source + count * 8, &animation))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, source + count * 8);
        }
        ++count;
    } while (animation != -1);
    WaitStruct* choices = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*choices));
    if (choices == NULL) {
        import->error_offset = source;
        return false;
    }
    for (u32 i = 0; i < count; ++i) {
        if (!read_int(import->dat, source + i * 8,
                      &choices[i].animation_id) ||
            !read_int(import->dat, source + i * 8 + 4,
                      &choices[i].weight))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, source + i * 8);
        }
    }
    *result = choices;
    return true;
}

static bool import_part_animations(HSD_DatImport* import, u32 source,
                                   u32 end, FtCharacterAssets* output)
{
    if (end < source || ((end - source) & 3) != 0) {
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    }
    u32 group_count = (end - source) / 4;
    if (group_count > ARRAY_SIZE(output->part_animation_groups)) {
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    }
    for (u32 group = 0; group < group_count; ++group) {
        u32 group_source;
        HSD_DatReferenceKind group_kind =
            resolve(import->dat, source + group * 4, &group_source);
        if (group_kind == HSD_DAT_REF_NULL) {
            continue;
        }
        if (group_kind != HSD_DAT_REF_LOCAL) {
            return fail(import, HSD_DAT_RELOCATION, source + group * 4);
        }
        struct ftData_x1C* data = &output->part_animation_group_data[group];
        u16 start_part;
        if (!HSD_DatReadU16(import->dat, group_source, &start_part) ||
            !HSD_DatReadU16(import->dat, group_source + 2, &data->x2))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, group_source);
        }
        data->x0 = start_part;
        u32 parts_source;
        if (data->x2 != 0) {
            if (!local(import->dat, group_source + 4, &parts_source)) {
                return fail(import, HSD_DAT_RELOCATION, group_source + 4);
            }
            const u8* parts = HSD_DatBytes(import->dat, parts_source,
                                           data->x2);
            if (parts == NULL ||
                (data->x4 = HSD_DatImportAlloc(import, data->x2)) == NULL)
            {
                import->error_offset = parts_source;
                return false;
            }
            memcpy(data->x4, parts, data->x2);
        }
        u32 animations_source;
        if (!local(import->dat, group_source + 8, &animations_source)) {
            return fail(import, HSD_DAT_RELOCATION, group_source + 8);
        }
        data->x8 = HSD_DatImportAlloc(
            import, FT_CHARACTER_PART_ANIMATION_VARIANTS * sizeof(*data->x8));
        if (data->x8 == NULL) {
            import->error_offset = animations_source;
            return false;
        }
        for (u32 i = 0; i < FT_CHARACTER_PART_ANIMATION_VARIANTS; ++i) {
            u32 animation_source;
            HSD_DatReferenceKind kind = resolve(
                import->dat, animations_source + i * 4, &animation_source);
            if (kind == HSD_DAT_REF_INVALID) {
                break;
            }
            if (kind == HSD_DAT_REF_NULL) {
                break;
            }
            if (kind == HSD_DAT_REF_EXTERNAL) {
                import->unresolved_external_count++;
                continue;
            }
            if (kind == HSD_DAT_REF_LOCAL) {
                bool descriptor = true;
                for (u32 link = 0; link < 4; ++link) {
                    u32 ignored;
                    HSD_DatReferenceKind link_kind = resolve(
                        import->dat, animation_source + link * 4, &ignored);
                    if (link_kind == HSD_DAT_REF_INVALID) {
                        descriptor = false;
                        break;
                    }
                }
                if (!descriptor) {
                    break;
                }
                bool allow_unresolved_externals =
                    import->allow_unresolved_externals;
                import->allow_unresolved_externals = true;
                data->x8[i] = HSD_DatImportGraph(import, animation_source,
                                                  DAT_ANIMJOINT);
                import->allow_unresolved_externals =
                    allow_unresolved_externals;
                if (data->x8[i] == NULL) {
                    return false;
                }
            }
        }
        output->part_animation_groups[group] = data;
    }
    output->data.x1C = output->part_animation_groups;
    return true;
}

static bool import_shield_model(HSD_DatImport* import, u32 source,
                                FtCharacterAssets* output)
{
    u32 joints_source;
    HSD_DatReferenceKind kind = resolve(import->dat, source, &joints_source);
    if (kind == HSD_DAT_REF_NULL) {
        return true;
    }
    if (kind != HSD_DAT_REF_LOCAL ||
        !HSD_DatReadF32(import->dat, source + 4,
                        &output->shield_model.x8))
    {
        return fail(import, HSD_DAT_RELOCATION, source);
    }
    for (u32 i = 0; i < ARRAY_SIZE(output->shield_joints); ++i) {
        u32 joint_source;
        kind = resolve(import->dat, joints_source + i * 4, &joint_source);
        if (kind == HSD_DAT_REF_LOCAL) {
            output->shield_joints[i] = HSD_DatImportGraph(
                import, joint_source, DAT_JOINT);
            if (output->shield_joints[i] == NULL) {
                return false;
            }
        } else if (kind == HSD_DAT_REF_EXTERNAL) {
            return fail(import, HSD_DAT_EXTERNAL, joints_source + i * 4);
        }
    }
    output->shield_model.x0 = output->shield_joints;
    output->data.x20 = &output->shield_model;
    return true;
}

static bool import_collision_bubbles(HSD_DatImport* import, u32 source,
                                     FtCharacterAssets* output)
{
    for (u32 i = 0; i < ARRAY_SIZE(output->collision_bubbles); ++i) {
        struct ftData_x38* bubble = &output->collision_bubbles[i];
        u32 at = source + i * 20;
        if (!read_int(import->dat, at, &bubble->x0) ||
            !read_f32_words(import->dat, at + 4, &bubble->x4, 16))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, at);
        }
    }
    output->data.x38 = output->collision_bubbles;
    return true;
}

static bool import_dynamic_nodes(HSD_DatImport* import, u32 source,
                                 u32 count, DynamicsDesc* output)
{
    struct lb_00F9_UnkDesc1Inner* nodes = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*nodes));
    if (nodes == NULL ||
        HSD_DatBytes(import->dat, source,
                     (size_t) count * sizeof(*nodes)) == NULL)
    {
        import->error_offset = source;
        return false;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 current = source + i * (u32) sizeof(*nodes);
        if (!read_f32_words(import->dat, current, &nodes[i], sizeof(nodes[i])))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, current);
        }
    }
    output->data = (struct DynamicsData*) nodes;
    return true;
}

static bool import_dynamics(HSD_DatImport* import, u32 source,
                            FtCharacterAssets* output)
{
    u32 bones_source;
    u32 collision_source;
    s32 count;
    if (!read_s32(import->dat, source, &count) || count < 0 ||
        count > Ft_Dynamics_NumMax ||
        !read_int(import->dat, source + 8, &output->dynamics.x4) ||
        output->dynamics.x4 < 0 || output->dynamics.x4 > 0xB)
    {
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    }
    output->dynamics.dynamicsNum = count;
    output->dynamics.ftDynamicBones = &output->dynamic_bones;
    output->dynamics.descriptor_count = count;
    if (output->dynamics.x4 != 0) {
        if (!local(import->dat, source + 12, &collision_source) ||
            HSD_DatBytes(import->dat, collision_source,
                         (size_t) output->dynamics.x4 * 20) == NULL)
        {
            return fail(import, HSD_DAT_RELOCATION, source + 12);
        }
        output->dynamics.x8 = HSD_DatImportAlloc(
            import, (size_t) output->dynamics.x4 * sizeof(*output->dynamics.x8));
        if (output->dynamics.x8 == NULL) {
            return false;
        }
        for (s32 i = 0; i < output->dynamics.x4; ++i) {
            struct ftData_x38* bubble = &output->dynamics.x8[i];
            u32 at = collision_source + (u32) i * 20;
            if (!read_int(import->dat, at, &bubble->x0) ||
                !read_f32_words(import->dat, at + 4, &bubble->x4, 16))
            {
                return fail(import, HSD_DAT_DESCRIPTOR, at);
            }
        }
    }
    if (count == 0) {
        return true;
    }
    if (!local(import->dat, source + 4, &bones_source) ||
        HSD_DatBytes(import->dat, bones_source, (size_t) count * 24) == NULL)
    {
        return fail(import, HSD_DAT_RELOCATION, source + 4);
    }
    u32 descriptor_bytes = HSD_DatNextTarget(import->dat, bones_source) - bones_source;
    u32 descriptor_count = descriptor_bytes / 24;
    if (descriptor_count < (u32) count || descriptor_count > Ft_Dynamics_NumMax)
        return fail(import, HSD_DAT_DESCRIPTOR, bones_source);
    output->dynamics.descriptor_count = descriptor_count;
    for (u32 i = 0; i < descriptor_count; ++i) {
        u32 at = bones_source + i * 24;
        BoneDynamicsDesc* bone = &output->dynamic_bones.array[i];
        u32 nodes_source;
        s32 bone_id;
        u32 node_count;
        if (!read_s32(import->dat, at, &bone_id) ||
            !HSD_DatReadU32(import->dat, at + 8, &node_count) ||
            node_count > 256 ||
            !read_f32_words(import->dat, at + 12, &bone->dyn_desc.pos,
                            sizeof(bone->dyn_desc.pos)))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, at);
        }
        bone->bone_id = (Fighter_Part) bone_id;
        bone->dyn_desc.count = node_count;
        HSD_DatReferenceKind kind = resolve(import->dat, at + 4,
                                            &nodes_source);
        if (bone->dyn_desc.count == 0) {
            if (kind != HSD_DAT_REF_NULL) {
                return fail(import, HSD_DAT_DESCRIPTOR, at + 4);
            }
        } else if (kind != HSD_DAT_REF_LOCAL ||
                   !import_dynamic_nodes(import, nodes_source,
                                         bone->dyn_desc.count,
                                         &bone->dyn_desc))
        {
            return false;
        }
    }
    return true;
}

static bool import_hurtboxes(HSD_DatImport* import, u32 source,
                             struct ftData_x30* output)
{
    u32 inits_source;
    s32 count;
    if (!read_s32(import->dat, source, &count) || count < 0 || count > 256 ||
        !local(import->dat, source + 4, &inits_source) ||
        HSD_DatBytes(import->dat, inits_source, (size_t) count * 40) == NULL)
    {
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    }
    output->count = count;
    output->inits = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*output->inits));
    if (output->inits == NULL) {
        import->error_offset = inits_source;
        return false;
    }
    for (s32 i = 0; i < count; ++i) {
        ftHurtboxInit* init = &output->inits[i];
        u32 at = inits_source + (u32) i * 40;
        u32 value;
        if (!HSD_DatReadU32(import->dat, at, &value)) return false;
        init->bone_idx = (Fighter_Part) value;
        if (!HSD_DatReadU32(import->dat, at + 4, &value)) return false;
        init->height = (HurtHeight) value;
        if (!HSD_DatReadU32(import->dat, at + 8, &init->is_grabbable) ||
            !read_f32_words(import->dat, at + 12, &init->a_offset, 28))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, at);
        }
    }
    return true;
}

static bool import_sfx_array(HSD_DatImport* import, u32 source,
                             FtSFXArr* output)
{
    u32 ids_source;
    if (!read_int(import->dat, source, &output->num) || output->num < 0 ||
        output->num > 256)
    {
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    }
    if (output->num == 0) {
        return true;
    }
    if (!local(import->dat, source + 4, &ids_source)) {
        return fail(import, HSD_DAT_RELOCATION, source + 4);
    }
    output->sfx_ids = import_words(import, ids_source,
                                   (size_t) output->num * 4);
    return output->sfx_ids != NULL;
}

static bool import_sfx(HSD_DatImport* import, u32 source,
                       FtCharacterAssets* output)
{
    u32 smash_source;
    u32 light_damage_source;
    u32 damage_source;
    HSD_DatReferenceKind smash_kind =
        resolve(import->dat, source, &smash_source);
    HSD_DatReferenceKind light_damage_kind =
        resolve(import->dat, source + 28, &light_damage_source);
    HSD_DatReferenceKind damage_kind =
        resolve(import->dat, source + 32, &damage_source);
    if (smash_kind != HSD_DAT_REF_LOCAL && smash_kind != HSD_DAT_REF_NULL) {
        return fail(import, HSD_DAT_RELOCATION, source);
    }
    if (light_damage_kind != HSD_DAT_REF_LOCAL &&
        light_damage_kind != HSD_DAT_REF_NULL)
    {
        return fail(import, HSD_DAT_RELOCATION, source + 28);
    }
    if (damage_kind != HSD_DAT_REF_LOCAL &&
        damage_kind != HSD_DAT_REF_NULL)
    {
        return fail(import, HSD_DAT_RELOCATION, source + 32);
    }
    if (smash_kind == HSD_DAT_REF_LOCAL &&
        !import_sfx_array(import, smash_source, &output->smash_sfx))
    {
        return false;
    }
    if (light_damage_kind == HSD_DAT_REF_LOCAL &&
        !import_sfx_array(import, light_damage_source,
                          &output->light_damage_sfx))
    {
        return false;
    }
    if (damage_kind == HSD_DAT_REF_LOCAL &&
        !import_sfx_array(import, damage_source, &output->damage_sfx))
    {
        return false;
    }
    output->sfx.smash = smash_kind == HSD_DAT_REF_LOCAL
                            ? &output->smash_sfx
                            : NULL;
    output->sfx.x1C = light_damage_kind == HSD_DAT_REF_LOCAL
                          ? &output->light_damage_sfx
                          : NULL;
    output->sfx.x20 = damage_kind == HSD_DAT_REF_LOCAL
                          ? &output->damage_sfx
                          : NULL;
    int* fields[] = {
        &output->sfx.x4,  &output->sfx.x8,  &output->sfx.xC,
        &output->sfx.x10, &output->sfx.x14, &output->sfx.x18,
        &output->sfx.x24, &output->sfx.x28,
        &output->sfx.x2C, &output->sfx.x30, &output->sfx.x34,
    };
    for (u32 i = 0; i < ARRAY_SIZE(fields); ++i) {
        u32 at = source + 4 + i * 4;
        if (i >= 6) {
            at += 8;
        }
        if (!read_int(import->dat, at, fields[i])) {
            return fail(import, HSD_DAT_DESCRIPTOR, at);
        }
    }
    return true;
}

static bool import_ik(const HSD_Dat* dat, u32 source,
                      struct ftData_x58_t* output)
{
    const u8* bytes = HSD_DatBytes(dat, source, 28);
    if (bytes == NULL) {
        return false;
    }
    output->x0 = bytes[0];
    output->x1 = bytes[1];
    output->x8 = bytes[8];
    output->x9 = bytes[9];
    output->x10 = bytes[16];
    output->x11 = bytes[17];
    return HSD_DatReadF32(dat, source + 4, &output->x4) &&
           HSD_DatReadF32(dat, source + 12, &output->xC) &&
           HSD_DatReadF32(dat, source + 24, &output->x18);
}

static void* import_graph_slot(HSD_DatImport* import, u32 slot,
                               HSD_DatImportType type)
{
    u32 source;
    if (!local(import->dat, slot, &source)) {
        fail(import, HSD_DAT_RELOCATION, slot);
        return NULL;
    }
    void* output = HSD_DatImportGraph(import, source, type);
    if (output == NULL && import->status == HSD_DAT_OK) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
    }
    return output;
}

static bool import_optional_graph_slot(HSD_DatImport* import, u32 slot,
                                       HSD_DatImportType type, void** output)
{
    u32 source;
    const char* external;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &source, &external);
    *output = NULL;
    if (kind == HSD_DAT_REF_NULL) {
        return true;
    }
    if (kind == HSD_DAT_REF_EXTERNAL) {
        import->unresolved_external_count++;
        import->external_symbol = external;
        return true;
    }
    if (kind != HSD_DAT_REF_LOCAL ||
        (*output = HSD_DatImportGraph(import, source, type)) == NULL)
    {
        fail(import, kind == HSD_DAT_REF_LOCAL ? HSD_DAT_DESCRIPTOR
                                               : HSD_DAT_RELOCATION,
             slot);
        return false;
    }
    return true;
}

static bool import_animation_bundle(HSD_DatImport* import, u32 source,
                                    AnimBundle* output)
{
    return import_optional_graph_slot(import, source, DAT_ANIMJOINT,
                                      (void**) &output->anim) &&
           import_optional_graph_slot(import, source + 4, DAT_MATANIM_JOINT,
                                      (void**) &output->matanim) &&
           import_optional_graph_slot(import, source + 8, DAT_SHAPE_JOINT,
                                      (void**) &output->shapeanim);
}

static bool import_graph_pointer_slot(HSD_DatImport* import, u32 slot,
                                      HSD_DatImportType type, void*** output)
{
    u32 source;
    const char* external;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &source, &external);
    *output = NULL;
    if (kind == HSD_DAT_REF_NULL) {
        return true;
    }
    if (kind == HSD_DAT_REF_EXTERNAL) {
        import->unresolved_external_count++;
        import->external_symbol = external;
        return true;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        return fail(import, HSD_DAT_RELOCATION, slot);
    }
    void** pointer = HSD_DatImportAlloc(import, sizeof(*pointer));
    if (pointer == NULL ||
        !import_optional_graph_slot(import, source, type, pointer))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return false;
    }
    *output = pointer;
    return true;
}

static itSamusGrappleAttributes* import_samus_grapple_attributes(
    HSD_DatImport* import, u32 article_source)
{
    enum { SOURCE_SIZE = 0xB0, SCALAR_SIZE = 0x64 };
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatNextTarget(import->dat, source) - source != SOURCE_SIZE)
    {
        fail(import, HSD_DAT_DESCRIPTOR, article_source + 4);
        return NULL;
    }
    itSamusGrappleAttributes* attrs =
        HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!read_u32_words(import->dat, source, attrs, SCALAR_SIZE) ||
        !import_optional_graph_slot(import, source + 0x64, DAT_JOINT,
                                    (void**) &attrs->x64) ||
        !import_optional_graph_slot(import, source + 0x68, DAT_JOINT,
                                    (void**) &attrs->x68) ||
        !import_optional_graph_slot(import, source + 0x6C, DAT_JOINT,
                                    (void**) &attrs->x6C) ||
        !import_optional_graph_slot(import, source + 0x70, DAT_JOINT,
                                    (void**) &attrs->x70) ||
        !import_graph_pointer_slot(import, source + 0x74, DAT_ANIMJOINT,
                                   (void***) &attrs->x74) ||
        !import_graph_pointer_slot(import, source + 0x78, DAT_MATANIM_JOINT,
                                   (void***) &attrs->x78) ||
        !import_graph_pointer_slot(import, source + 0x7C, DAT_SHAPE_JOINT,
                                   (void***) &attrs->x7C) ||
        !import_graph_pointer_slot(import, source + 0x80, DAT_ANIMJOINT,
                                   (void***) &attrs->x80) ||
        !import_graph_pointer_slot(import, source + 0x84, DAT_MATANIM_JOINT,
                                   (void***) &attrs->x84) ||
        !import_graph_pointer_slot(import, source + 0x88, DAT_SHAPE_JOINT,
                                   (void***) &attrs->x88) ||
        !import_graph_pointer_slot(import, source + 0x8C, DAT_ANIMJOINT,
                                   (void***) &attrs->x8C) ||
        !import_graph_pointer_slot(import, source + 0x90, DAT_MATANIM_JOINT,
                                   (void***) &attrs->x90) ||
        !import_graph_pointer_slot(import, source + 0x94, DAT_SHAPE_JOINT,
                                   (void***) &attrs->x94) ||
        !import_graph_pointer_slot(import, source + 0x98, DAT_ANIMJOINT,
                                   (void***) &attrs->x98) ||
        !import_graph_pointer_slot(import, source + 0x9C, DAT_MATANIM_JOINT,
                                   (void***) &attrs->x9C) ||
        !import_graph_pointer_slot(import, source + 0xA0, DAT_SHAPE_JOINT,
                                   (void***) &attrs->xA0) ||
        !import_graph_pointer_slot(import, source + 0xA4, DAT_ANIMJOINT,
                                   (void***) &attrs->xA4) ||
        !import_graph_pointer_slot(import, source + 0xA8, DAT_MATANIM_JOINT,
                                   (void***) &attrs->xA8) ||
        !import_graph_pointer_slot(import, source + 0xAC, DAT_SHAPE_JOINT,
                                   (void***) &attrs->xAC))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return NULL;
    }
    return attrs;
}

static itYoyoAttributes* import_ness_yoyo_attributes(
    HSD_DatImport* import, u32 article_source)
{
    enum { SOURCE_SIZE = 0x5C, SCALAR_SIZE = 0x50 };
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatNextTarget(import->dat, source) - source != SOURCE_SIZE)
    {
        fail(import, HSD_DAT_DESCRIPTOR, article_source + 4);
        return NULL;
    }
    itYoyoAttributes* attrs = HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!read_u32_words(import->dat, source, attrs, SCALAR_SIZE) ||
        attrs->link_count < 2 || attrs->link_count > 64 ||
        attrs->up_smash_charge_link_count < 1 ||
        attrs->up_smash_charge_link_count > attrs->link_count ||
        attrs->down_smash_charge_link_count < 1 ||
        attrs->down_smash_charge_link_count > attrs->link_count)
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    attrs->string_joint =
        import_graph_slot(import, source + 0x50, DAT_JOINT);
    attrs->yoyo_joint = import_graph_slot(import, source + 0x54, DAT_JOINT);
    attrs->yoyo_material_animation =
        import_graph_slot(import, source + 0x58, DAT_MATANIM_JOINT);
    return import->status == HSD_DAT_OK ? attrs : NULL;
}


static itGamewatchArticleAttributes* import_gamewatch_article_attributes(
    HSD_DatImport* import, u32 article_source, Article* article)
{
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatNextTarget(import->dat, source) - source != 4)
    {
        fail(import, HSD_DAT_DESCRIPTOR, article_source + 4);
        return NULL;
    }
    itGamewatchArticleAttributes* attrs =
        HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    attrs->model_layers = articleModelLayers_Import(import, source,
        article->x10_modelDesc != NULL ? article->x10_modelDesc->x0_joint : NULL);
    article->model_layers = attrs->model_layers;
    return import->status == HSD_DAT_OK ? attrs : NULL;
}

static itGamewatchchefAttributes* import_gamewatch_chef_attributes(
    HSD_DatImport* import, u32 article_source, Article* article)
{
    enum { SOURCE_SIZE = 0x74, SOURCE_SCALAR_SIZE = SOURCE_SIZE - 4 };
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatNextTarget(import->dat, source) - source != SOURCE_SIZE)
    {
        fail(import, HSD_DAT_DESCRIPTOR, article_source + 4);
        return NULL;
    }
    itGamewatchchefAttributes* attrs =
        HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    attrs->model_layers = articleModelLayers_Import(import, source,
        article->x10_modelDesc != NULL ? article->x10_modelDesc->x0_joint : NULL);
    if (attrs->model_layers == NULL ||
        !read_f32_words(import->dat, source + 4,
                        &attrs->wall_bounce_velocity_multiplier,
                        SOURCE_SCALAR_SIZE))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return NULL;
    }
    article->model_layers = attrs->model_layers;
    return attrs;
}

static itClimbersStringAttributes* import_climbers_string_attributes(
    HSD_DatImport* import, u32 article_source)
{
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatBytes(import->dat, source, 0x2C) == NULL) {
        fail(import, HSD_DAT_RELOCATION, article_source + 4);
        return NULL;
    }
    itClimbersStringAttributes* attrs =
        HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    for (u32 i = 0; i < 0x24; i += 4) {
        u32 value;
        if (!HSD_DatReadU32(import->dat, source + i, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, source + i);
            return NULL;
        }
        memcpy((u8*) attrs + i, &value, sizeof(value));
    }
    if (attrs->x0_count < 2 || attrs->x0_count > 64) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    for (u32 i = 0; i < 2; ++i) {
        u32 slot = source + 0x24 + i * 4;
        u32 target;
        const char* external;
        HSD_Joint** joint = i == 0 ? &attrs->x24_joint : &attrs->x28_joint;
        HSD_DatReferenceKind kind =
            HSD_DatResolve(import->dat, slot, &target, &external);
        if (kind == HSD_DAT_REF_EXTERNAL) {
            import->unresolved_external_count++;
            import->external_symbol = external;
        } else if (kind == HSD_DAT_REF_LOCAL) {
            *joint = HSD_DatImportGraph(import, target, DAT_JOINT);
        } else if (kind != HSD_DAT_REF_NULL) {
            fail(import, HSD_DAT_RELOCATION, slot);
            return NULL;
        }
    }
    return import->status == HSD_DAT_OK ? attrs : NULL;
}

static itLinkBoomerangAttributes* import_link_boomerang_attributes(
    HSD_DatImport* import, u32 article_source)
{
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatBytes(import->dat, source, 0x64) == NULL)
    {
        fail(import, HSD_DAT_RELOCATION, article_source + 4);
        return NULL;
    }
    itLinkBoomerangAttributes* attrs =
        HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!read_u32_words(import->dat, source, attrs, 0x44) ||
        !import_optional_graph_slot(import, source + 0x44, DAT_JOINT,
                                    (void**) &attrs->x44) ||
        !import_optional_graph_slot(import, source + 0x48, DAT_JOINT,
                                    (void**) &attrs->x48) ||
        !import_animation_bundle(import, source + 0x4C, &attrs->x4C_anim) ||
        !import_animation_bundle(import, source + 0x58, &attrs->x58_anim))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return NULL;
    }
    return attrs;
}

static itLinkHookshotAttributes* import_link_hookshot_attributes(
    HSD_DatImport* import, u32 article_source)
{
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatBytes(import->dat, source, 0x60) == NULL)
    {
        fail(import, HSD_DAT_RELOCATION, article_source + 4);
        return NULL;
    }
    itLinkHookshotAttributes* attrs =
        HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!read_u32_words(import->dat, source, attrs, 0x54) ||
        !import_optional_graph_slot(import, source + 0x54, DAT_JOINT,
                                    (void**) &attrs->x54) ||
        !import_optional_graph_slot(import, source + 0x58, DAT_JOINT,
                                    (void**) &attrs->x58) ||
        !import_optional_graph_slot(import, source + 0x5C, DAT_JOINT,
                                    (void**) &attrs->x5C))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return NULL;
    }
    return attrs;
}

static itLinkArrowAttributes* import_link_arrow_attributes(
    HSD_DatImport* import, u32 article_source)
{
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatBytes(import->dat, source, 0x30) == NULL)
    {
        fail(import, HSD_DAT_RELOCATION, article_source + 4);
        return NULL;
    }
    itLinkArrowAttributes* attrs = HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!read_u32_words(import->dat, source, attrs, 0x24) ||
        !import_optional_graph_slot(import, source + 0x24, DAT_JOINT,
                                    (void**) &attrs->x24) ||
        !import_optional_graph_slot(import, source + 0x28, DAT_JOINT,
                                    (void**) &attrs->x28) ||
        !HSD_DatReadF32(import->dat, source + 0x2C, &attrs->x2C))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return NULL;
    }
    return attrs;
}

static itSeakChain_Attrs* import_seak_chain_attributes(
    HSD_DatImport* import, u32 article_source)
{
    u32 source;
    if (!local(import->dat, article_source + 4, &source) ||
        HSD_DatBytes(import->dat, source, 0x6C) == NULL)
    {
        fail(import, HSD_DAT_RELOCATION, article_source + 4);
        return NULL;
    }
    itSeakChain_Attrs* attrs = HSD_DatImportAlloc(import, sizeof(*attrs));
    if (attrs == NULL) {
        import->error_offset = source;
        return NULL;
    }
    for (u32 i = 0; i < 0x64; i += 4) {
        u32 value;
        if (!HSD_DatReadU32(import->dat, source + i, &value)) {
            fail(import, HSD_DAT_DESCRIPTOR, source + i);
            return NULL;
        }
        memcpy((u8*) attrs + i, &value, sizeof(value));
    }
    if (attrs->x0 < 2 || attrs->x0 > 30) {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    attrs->x64_joint = import_graph_slot(import, source + 0x64, DAT_JOINT);
    attrs->x68_joint = import_graph_slot(import, source + 0x68, DAT_JOINT);
    return import->status == HSD_DAT_OK ? attrs : NULL;
}

static FtPartsDesc* import_parts_desc(HSD_DatImport* import, u32 source,
                                      u32 costume_count)
{
    FtPartsDesc* output = HSD_DatImportAlloc(import, sizeof(*output));
    u32 visibility_source;
    if (output == NULL) {
        import->error_offset = source;
        return NULL;
    }
    if (!local(import->dat, source + 4, &visibility_source))
        return NULL;
    u32 visibility_count =
        (HSD_DatNextTarget(import->dat, visibility_source) - visibility_source) / 16;
    if (visibility_count < costume_count || visibility_count > 8) {
        fail(import, HSD_DAT_DESCRIPTOR, visibility_source);
        return NULL;
    }
    output->costume_count = visibility_count;
    if (!HSD_DatReadU32(import->dat, source, &output->model_num) ||
        output->model_num > 11 ||
        !import_visibility(import, visibility_source, output->model_num,
                           visibility_count, &output->vis_table))
    {
        if (import->status == HSD_DAT_OK) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
        }
        return NULL;
    }
    return output;
}

const FtCharacterAssetLayout ftCharacterStockLayouts[Ft_Kind_StockCount] = {
    [Ft_Kind_Mario] = { .article_mask = 0x005 },
    [Ft_Kind_Fox] = { .article_mask = 0x007 },
    [Ft_Kind_Kirby] = { .article_mask = 0x00F, .joint_graph_mask = 0x010 },
    [Ft_Kind_Koopa] = { .article_mask = 0x001 },
    [Ft_Kind_Link] = { .article_mask = 0x01F, .joint_graph_mask = 0x040,
                      .item_format = FtCharacterItems_LinkWeapons },
    [Ft_Kind_Seak] = { .article_mask = 0x00F, .joint_graph_mask = 0x030,
                      .item_format = FtCharacterItems_Chain },
    [Ft_Kind_Ness] = { .article_mask = 0x7FF,
                      .item_format = FtCharacterItems_Yoyo },
    [Ft_Kind_Peach] = { .article_mask = 0x01F },
    [Ft_Kind_Popo] = { .article_mask = 0x007,
                      .item_format = FtCharacterItems_ClimberString },
    [Ft_Kind_Nana] = { .article_mask = 0x007,
                      .item_format = FtCharacterItems_ClimberString },
    [Ft_Kind_Pikachu] = { .article_mask = 0x007 },
    [Ft_Kind_Samus] = { .article_mask = 0x00F,
                       .item_format = FtCharacterItems_Grapple },
    [Ft_Kind_Yoshi] = { .article_mask = 0x007, .joint_graph_mask = 0x008,
                      .byte_range_count = 1, .byte_ranges = {{ 0x12C, 12 }} },
    [Ft_Kind_Mewtwo] = { .article_mask = 0x003 },
    [Ft_Kind_Luigi] = { .article_mask = 0x001 },
    [Ft_Kind_Zelda] = { .article_mask = 0x003 },
    [Ft_Kind_CLink] = { .article_mask = 0x03F, .joint_graph_mask = 0x040,
                       .item_format = FtCharacterItems_LinkWeapons },
    [Ft_Kind_DrMario] = { .article_mask = 0x00A },
    [Ft_Kind_Falco] = { .article_mask = 0x00B },
    [Ft_Kind_Pichu] = { .article_mask = 0x007 },
    [Ft_Kind_Purin] = { .item_format = FtCharacterItems_ModelParts },
    [Ft_Kind_GameWatch] = { .article_mask = 0x3FF, .visibility_mask = 0x400,
                          .item_format = FtCharacterItems_PaperArticles,
                          // Four costume colors and one outline are byte tuples.
                          .byte_range_count = 1, .byte_ranges = {{ 4, 20 }} },
    [Ft_Kind_MasterH] = { .article_mask = 0x007 },
    [Ft_Kind_CrezyH] = { .article_mask = 0x007 },
    [Ft_Kind_GKoops] = { .article_mask = 0x001 },
};


static bool import_fighter_visibility(HSD_DatImport* import,
                                      const FtCharacterAssetLayout* layout,
                                      u32 source, FtCharacterAssets* output)
{
    for (unsigned i = 0; i < FT_CHARACTER_ITEM_CAPACITY; ++i) {
        if ((layout->visibility_mask & (1u << i)) == 0) continue;
        u32 lookup_source;
        if (output->model_data.x0.model_num == 0 ||
            !local(import->dat, source + i * 4, &lookup_source))
            return fail(import, HSD_DAT_DESCRIPTOR, source + i * 4);
        FtPartsVisLookup* lookup = import_visibility_lookups(
            import, lookup_source, output->model_data.x0.model_num);
        if (lookup == NULL) return false;
        output->visibility_assets[i] = lookup;
        output->fighter_items[i] = lookup;
    }
    return import->status == HSD_DAT_OK;
}

static bool import_fighter_items(HSD_DatImport* import,
                                 const FtCharacterAssetLayout* layout,
                                 u32 source, u32 costume_count,
                                 FtCharacterAssets* output,
                                 ItScriptCollect* scripts)
{
    memset(output->fighter_items, 0, sizeof(output->fighter_items));
    u32 article_mask = layout->article_mask;
    u32 article_sources[FT_CHARACTER_ITEM_CAPACITY];
    u32 article_source_count = 0;
    for (u32 i = 0; i < ARRAY_SIZE(output->fighter_items); ++i) {
        if ((article_mask & (1U << i)) == 0) {
            continue;
        }
        u32 article_source;
        if (!local(import->dat, source + i * 4, &article_source) ||
            !itArticle_IsDescriptor(import->dat, article_source, false))
        {
            return fail(import, HSD_DAT_DESCRIPTOR, source + i * 4);
        }
        article_sources[article_source_count++] = article_source;
    }
    for (u32 i = 0; i < ARRAY_SIZE(output->fighter_items); ++i) {
        if ((article_mask & (1U << i)) == 0) {
            continue;
        }
        u32 article_source;
        if (!local(import->dat, source + i * 4, &article_source) ||
            (output->fighter_items[i] = itArticle_Import(
                 import, article_source, &output->article_stats, scripts,
                 false, -1, article_sources, article_source_count)) == NULL)
        {
            if (import->status == HSD_DAT_OK) {
                fail(import, HSD_DAT_DESCRIPTOR, source + i * 4);
            }
            return false;
        }
    }

    switch (layout->item_format) {
    case FtCharacterItems_ClimberString: {
        u32 article_source;
        if (!local(import->dat, source + 2 * 4, &article_source)) {
            return fail(import, HSD_DAT_RELOCATION, source + 2 * 4);
        }
        ((Article*) output->fighter_items[2])->x4_specialAttributes =
            import_climbers_string_attributes(import, article_source);
        break;
    }
    case FtCharacterItems_LinkWeapons: {
        u32 boomerang_source;
        u32 hookshot_source;
        u32 arrow_source;
        if (!local(import->dat, source + 1 * 4, &boomerang_source) ||
            !local(import->dat, source + 2 * 4, &hookshot_source) ||
            !local(import->dat, source + 3 * 4, &arrow_source))
        {
            return fail(import, HSD_DAT_RELOCATION, source);
        }
        ((Article*) output->fighter_items[1])->x4_specialAttributes =
            import_link_boomerang_attributes(import, boomerang_source);
        ((Article*) output->fighter_items[2])->x4_specialAttributes =
            import_link_hookshot_attributes(import, hookshot_source);
        ((Article*) output->fighter_items[3])->x4_specialAttributes =
            import_link_arrow_attributes(import, arrow_source);
        break;
    }
    case FtCharacterItems_Chain:
        {
            u32 article_source;
            if (!local(import->dat, source + 3 * 4, &article_source)) {
                return fail(import, HSD_DAT_RELOCATION, source + 3 * 4);
            }
            ((Article*) output->fighter_items[3])->x4_specialAttributes =
                import_seak_chain_attributes(import, article_source);
        }
        break;
    case FtCharacterItems_Yoyo: {
        u32 article_source;
        if (!local(import->dat, source + 10 * 4, &article_source)) {
            return fail(import, HSD_DAT_RELOCATION, source + 10 * 4);
        }
        ((Article*) output->fighter_items[10])->x4_specialAttributes =
            import_ness_yoyo_attributes(import, article_source);
        break;
    }
    case FtCharacterItems_Grapple: {
        u32 article_source;
        u32 desc_source;
        u32 animations_source;
        if (!local(import->dat, source + 3 * 4, &article_source)) {
            return fail(import, HSD_DAT_RELOCATION, source + 3 * 4);
        }
        ((Article*) output->fighter_items[3])->x4_specialAttributes =
            import_samus_grapple_attributes(import, article_source);
        if (((Article*) output->fighter_items[3])->x4_specialAttributes ==
            NULL)
        {
            return false;
        }
        if (!local(import->dat, source + 4 * 4, &desc_source) ||
            !local(import->dat, desc_source + 4, &animations_source))
        {
            return fail(import, import->status == HSD_DAT_OK
                                    ? HSD_DAT_DESCRIPTOR
                                    : import->status,
                        source + 4 * 4);
        }
        FtAccessoryAssetSource recipe = {
            .root = desc_source,
            .base = { desc_source + 8, desc_source + 12, UINT32_MAX },
            .variant_count = 4,
        };
        for (u32 i = 0; i < recipe.variant_count; ++i)
            recipe.variants[i] = (FtAccessoryAnimationSource){
                animations_source + i * 4, UINT32_MAX, UINT32_MAX };
        FtAccessoryAsset* accessory = ftAccessoryAssets_Import(import, &recipe);
        if (!accessory || !ftAccessoryAssets_Validate(import, accessory))
            return fail(import, import->status == HSD_DAT_OK
                                    ? HSD_DAT_DESCRIPTOR : import->status,
                        desc_source);
        output->accessory_assets[4] = accessory;
        output->fighter_items[4] = accessory;
        break;
    }
    case FtCharacterItems_ModelParts: {
        u32 desc_source;
        if (!local(import->dat, source + 4, &desc_source)) {
            return fail(import, HSD_DAT_RELOCATION, source + 4);
        }
        output->fighter_items[1] = import_parts_desc(
            import, desc_source + 4, costume_count);
        break;
    }
    case FtCharacterItems_PaperArticles: {
        for (u32 i = 0; i < 10; ++i) {
            u32 article_source;
            if (!local(import->dat, source + i * 4, &article_source)) {
                return fail(import, HSD_DAT_RELOCATION, source + i * 4);
            }
            Article* article = output->fighter_items[i];
            article->x4_specialAttributes =
                i == 8 ? (void*) import_gamewatch_chef_attributes(
                             import, article_source, article)
                       : (void*) import_gamewatch_article_attributes(
                             import, article_source, article);
            if (article->x4_specialAttributes == NULL) {
                return false;
            }
        }
        break;
    }
    default:
        break;
    }
    for (unsigned i = 0; i < FT_CHARACTER_ITEM_CAPACITY; ++i)
        if ((layout->joint_graph_mask & (1u << i)) != 0) {
            HSD_Joint* graph = import_graph_slot(import, source + i * 4, DAT_JOINT);
            output->joint_graphs[i] = graph;
            output->fighter_items[i] = graph;
        }
    if (!import_fighter_visibility(import, layout, source, output)) return false;
    return import->status == HSD_DAT_OK;
}

bool ftCharacterAssets_Import(const void* bytes, size_t size,
                              const FtCharacterAssetLayout* layout,
                              const char* root_name,
                              u32 wait_count, u32 demo_count,
                              u32 costume_count, void* arena,
                              size_t arena_size, u32 record_capacity,
                              FtCharacterAssets* output)
{
    u32 root;
    u32 fields[24];
    ItScriptEntry item_script_entries[256];
    ItScriptCollect item_scripts = {
        item_script_entries, 0, ARRAY_SIZE(item_script_entries)
    };
    if (output == NULL || !ftCharacterAssetLayout_IsValid(layout)) {
        return false;
    }
    memset(output, 0, sizeof(*output));
    if (HSD_DatOpen(&output->dat, bytes, size) != HSD_DAT_OK ||
        HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&output->dat, root_name, &root) ||
        wait_count == 0 || demo_count == 0 || costume_count == 0 ||
        costume_count > 8)
    {
        return false;
    }
    for (u32 i = 0; i < ARRAY_SIZE(fields); ++i) {
        HSD_DatReferenceKind kind = resolve(&output->dat, root + i * 4,
                                            &fields[i]);
        if (kind != HSD_DAT_REF_LOCAL && kind != HSD_DAT_REF_NULL) {
            return fail(&output->import, HSD_DAT_RELOCATION, root + i * 4);
        }
    }
    if (resolve(&output->dat, root, &fields[0]) != HSD_DAT_REF_LOCAL ||
        ftData_DecodeCommonAttrs(&output->dat, fields[0],
                                 &output->common_attributes) != HSD_DAT_OK ||
        resolve(&output->dat, root + 4, &fields[1]) != HSD_DAT_REF_LOCAL)
    {
        return fail(&output->import, HSD_DAT_DESCRIPTOR, root);
    }
    u32 ext_end = HSD_DatNextTarget(&output->dat, fields[1]);
    size_t ext_size = ext_end - fields[1];
    if (ext_size > sizeof(output->extended_attributes)) {
        ext_size = sizeof(output->extended_attributes);
    }
    ext_size &= ~(size_t) 3;
    for (size_t i = 0; i < ext_size; i += 4) {
        u32 value;
        if (!HSD_DatReadU32(&output->dat, fields[1] + (u32) i, &value)) {
            return fail(&output->import, HSD_DAT_DESCRIPTOR,
                        fields[1] + (u32) i);
        }
        memcpy(output->extended_attributes + i, &value, 4);
    }

    for (unsigned i = 0; i < layout->byte_range_count; ++i) {
        // Preserve explicitly declared byte arrays among decoded scalar words.
        const size_t offset = layout->byte_ranges[i].offset;
        const size_t count = layout->byte_ranges[i].size;
        const void* frames = HSD_DatBytes(&output->dat,
                                        fields[1] + (u32) offset, count);
        if (ext_size < offset + count || frames == NULL) {
            return fail(&output->import, HSD_DAT_DESCRIPTOR,
                        fields[1] + (u32) offset);
        }
        memcpy(output->extended_attributes + offset, frames, count);
    }

    output->data.x0 = &output->common_attributes;
    output->data.ext_attr = output->extended_attributes;
    output->data.x8 = &output->model_data;
    output->data.x2C = &output->dynamics;
    output->data.x30 = &output->hurtboxes;
    output->data.x34 = &output->scale;
    output->data.x3C = &output->camera;
    output->data.x40 = &output->pickup;
    output->data.x44 = &output->collision;
    output->data.x4C_sfx = &output->sfx;
    output->data.x50 = &output->x50;
    output->data.x54 = output->x54;

    if (!import_model_data(&output->import, fields[2], costume_count,
                           &output->model_data) ||
        !import_wait_table(&output->import, fields[3], wait_count,
                           &output->data.xC, &output->action_stats, true) ||
        !(output->data.x10 = HSD_DatImportAlloc(
              &output->import, (size_t) wait_count * 2)) ||
        !HSD_DatBytes(&output->dat, fields[4], (size_t) wait_count * 2))
    {
        return false;
    }
    memcpy(output->data.x10,
           HSD_DatBytes(&output->dat, fields[4], (size_t) wait_count * 2),
           (size_t) wait_count * 2);

#ifdef MELEE_NATIVE
    if (!import_wait_table(&output->import, fields[5], demo_count,
                           &output->data.x14, NULL, false) ||
        !(output->data.x18 = HSD_DatImportAlloc(
              &output->import, (size_t) demo_count * 2)) ||
        !HSD_DatBytes(&output->dat, fields[6], (size_t) demo_count * 2))
    {
        return false;
    }
    memcpy(output->data.x18,
           HSD_DatBytes(&output->dat, fields[6], (size_t) demo_count * 2),
           (size_t) demo_count * 2);
#endif

    if (!import_part_animations(&output->import, fields[7], fields[8],
                                output) ||
        !import_shield_model(&output->import, fields[8], output))
    {
        return false;
    }

    if (resolve(&output->dat, root + 36, &fields[9]) == HSD_DAT_REF_LOCAL &&
        !import_wait_choices(&output->import, fields[9], &output->data.x24))
    {
        return false;
    }
    if (resolve(&output->dat, root + 40, &fields[10]) == HSD_DAT_REF_LOCAL &&
        !import_wait_choices(&output->import, fields[10], &output->data.x28))
    {
        return false;
    }
    if (!import_dynamics(&output->import, fields[11], output) ||
        !import_hurtboxes(&output->import, fields[12], &output->hurtboxes) ||
        !read_s32(&output->dat, fields[13], (s32*) &output->scale.x0) ||
        !HSD_DatReadF32(&output->dat, fields[13] + 4,
                        &output->scale.scale) ||
        !import_collision_bubbles(&output->import, fields[14], output) ||
        !read_f32_words(&output->dat, fields[15], &output->camera,
                        sizeof(output->camera)) ||
        !read_f32_words(&output->dat, fields[16], &output->pickup,
                        sizeof(output->pickup)) ||
        ftData_DecodeCollision(&output->dat, fields[17],
                               &output->collision) != HSD_DAT_OK ||
        !import_sfx(&output->import, fields[19], output) ||
        !read_f32_words(&output->dat, fields[20], &output->x50,
                        sizeof(output->x50)) ||
        HSD_DatBytes(&output->dat, fields[21], sizeof(output->x54)) == NULL)
    {
        return false;
    }
    for (u32 i = 0; i < ARRAY_SIZE(output->x54); ++i) {
        if (!read_int(&output->dat, fields[21] + i * 4,
                      &output->x54[i]))
        {
            return fail(&output->import, HSD_DAT_DESCRIPTOR,
                        fields[21] + i * 4);
        }
    }
    if (resolve(&output->dat, root + 72, &fields[18]) ==
        HSD_DAT_REF_LOCAL)
    {
        if (!import_fighter_items(&output->import, layout, fields[18],
                                  costume_count, output, &item_scripts))
        {
            return false;
        }
        output->data.x48_items = output->fighter_items;
    } else if (layout->article_mask != 0 || layout->joint_graph_mask != 0 ||
               layout->item_format != FtCharacterItems_Plain) {
        return fail(&output->import, HSD_DAT_DESCRIPTOR, root + 72);
    }
    output->data.x54 = output->x54;
    if (resolve(&output->dat, root + 88, &fields[22]) == HSD_DAT_REF_LOCAL) {
        if (!import_ik(&output->dat, fields[22],
                       &output->inverse_kinematics))
        {
            return fail(&output->import, HSD_DAT_DESCRIPTOR, fields[22]);
        }
        output->data.x58 = &output->inverse_kinematics;
    }
    if (resolve(&output->dat, root + 92, &fields[23]) == HSD_DAT_REF_LOCAL) {
        output->metal_model = HSD_DatImportGraph(&output->import, fields[23],
                                                  DAT_JOINT);
        output->data.x5C = output->metal_model;
        if (output->metal_model == NULL) {
            return false;
        }
        if (!validate_metal_visibility(output, output->model_data.x0.costume_count, root + 92))
            return false;
    }
    return output->import.status == HSD_DAT_OK &&
           itCommandData_Import(&output->import, &item_scripts,
                                &output->item_command_data);
}
