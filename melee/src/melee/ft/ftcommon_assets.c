#include "ftcommon_assets.h"

#include <melee/lb/lbcolanim_data.h>

#include <stddef.h>
#include <string.h>

static bool fail(HSD_DatImport* import, HSD_DatResult status, u32 offset)
{
    import->status = status;
    import->error_offset = offset;
    return false;
}

static bool resolve_local(const HSD_Dat* dat, u32 slot, u32* target)
{
    const char* external;
    return HSD_DatResolve(dat, slot, target, &external) == HSD_DAT_REF_LOCAL;
}

static bool import_words(const HSD_Dat* dat, u32 source, void* destination,
                         size_t size)
{
    u8* output = destination;
    u32 value;
    if ((size & 3) != 0 || HSD_DatBytes(dat, source, size) == NULL) {
        return false;
    }
    for (size_t offset = 0; offset < size; offset += sizeof(value)) {
        if (!HSD_DatReadU32(dat, source + (u32) offset, &value)) {
            return false;
        }
        memcpy(output + offset, &value, sizeof(value));
    }
    return true;
}

bool ftCommonAssets_Import(const void* bytes, size_t size, void* arena,
                           size_t arena_size, u32 record_capacity,
                           FtCommonAssets* output)
{
    const char* external;
    u32 common;
    u32 model;
    u32 omission_table;
    u32 parts_table;
    u32 root;
    u32 source;

    if (output == NULL) {
        return false;
    }
    memset(output, 0, sizeof(*output));
    if (sizeof(output->common) != 0x818 ||
        HSD_DatOpen(&output->dat, bytes, size) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&output->dat, "ftLoadCommonData", &root) ||
        HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK ||
        !resolve_local(&output->dat, root, &common) ||
        !import_words(&output->dat, common, &output->common,
                      sizeof(output->common)) ||
        !resolve_local(&output->dat, root + 16, &parts_table) ||
        !resolve_local(&output->dat, root + 20, &omission_table) ||
        !resolve_local(&output->dat, root + 48, &source) ||
        !import_words(&output->dat, source, &output->scale_modifiers,
                      sizeof(output->scale_modifiers)) ||
        !resolve_local(&output->dat, root + 52, &source) ||
        !import_words(&output->dat, source, &output->bunny_modifiers,
                      sizeof(output->bunny_modifiers)) ||
        !resolve_local(&output->dat, root + 56, &source) ||
        !import_words(&output->dat, source, &output->metal_modifiers,
                      sizeof(output->metal_modifiers)) ||
        !resolve_local(&output->dat, root + 60, &source) ||
        !import_words(&output->dat, source, &output->zako_modifiers,
                      sizeof(output->zako_modifiers)) ||
        !resolve_local(&output->dat, root + 84, &source) ||
        !import_words(&output->dat, source, &output->crowd_config,
                      sizeof(output->crowd_config)) ||
        !resolve_local(&output->dat, root + 80, &model))
    {
        return false;
    }

    // GXColor is four ordered bytes rather than a scalar word.
    memcpy(output->common.x6DC_colorsByPlayer,
           HSD_DatBytes(&output->dat,
                        common + offsetof(ftCommonData, x6DC_colorsByPlayer),
                        sizeof(output->common.x6DC_colorsByPlayer)),
           sizeof(output->common.x6DC_colorsByPlayer));
    memcpy(&output->common.x6EC_specialMaterialColor,
           HSD_DatBytes(&output->dat,
                        common + offsetof(ftCommonData, x6EC_specialMaterialColor),
                        sizeof(GXColor)),
           sizeof(GXColor));
    memcpy(&output->common.x7D8,
           HSD_DatBytes(&output->dat, common + offsetof(ftCommonData, x7D8),
                        4),
           4);

    void* numeric_tables[3] = {
        output->item_throw_attributes,
        output->swing_multipliers,
        output->stale_move_penalties,
    };
    const size_t numeric_sizes[3] = {
        sizeof(output->item_throw_attributes),
        sizeof(output->swing_multipliers),
        sizeof(output->stale_move_penalties),
    };
    for (u32 i = 0; i < 3; ++i) {
        u32 slot = root + 4 + i * 4;
        if (!resolve_local(&output->dat, slot, &source) ||
            HSD_DatNextTarget(&output->dat, source) - source !=
                numeric_sizes[i] ||
            !import_words(&output->dat, source, numeric_tables[i],
                          numeric_sizes[i]))
        {
            return fail(&output->import, HSD_DAT_DESCRIPTOR, slot);
        }
    }

    for (u32 kind = 0; kind < FT_COMMON_PART_TABLE_COUNT; ++kind) {
        u32 descriptor;
        HSD_DatReferenceKind reference = HSD_DatResolve(
            &output->dat, parts_table + kind * 4, &descriptor, &external);
        if (reference != HSD_DAT_REF_LOCAL ||
            ftData_DecodeParts(&output->dat, descriptor,
                               &output->part_records[kind],
                               output->joint_to_part[kind], 256,
                               output->part_to_joint[kind]) != HSD_DAT_OK)
        {
            output->import.status = HSD_DAT_DESCRIPTOR;
            output->import.error_offset = parts_table + kind * 4;
            return false;
        }
        output->parts[kind] = &output->part_records[kind];

        reference = HSD_DatResolve(&output->dat,
                                   omission_table + kind * 4,
                                   &descriptor, &external);
        if (reference == HSD_DAT_REF_NULL) {
            output->omissions[kind] = &output->omission_records[kind];
        } else if (reference == HSD_DAT_REF_LOCAL &&
                   ftData_DecodePartOmissions(
                       &output->dat, descriptor,
                       &output->omission_records[kind],
                       output->omission_parts[kind], 32,
                       output->part_records[kind].parts_num) == HSD_DAT_OK)
        {
            output->omissions[kind] = &output->omission_records[kind];
        } else {
            output->import.status = HSD_DAT_DESCRIPTOR;
            output->import.error_offset = omission_table + kind * 4;
            return false;
        }
    }

    u32 respawn_table;
    if (!resolve_local(&output->dat, root + 32, &respawn_table) ||
        HSD_DatNextTarget(&output->dat, respawn_table) - respawn_table != 8 ||
        !resolve_local(&output->dat, respawn_table, &source))
    {
        return fail(&output->import, HSD_DAT_DESCRIPTOR, root + 32);
    }
    output->respawn_platform.joint =
        HSD_DatImportGraph(&output->import, source, DAT_JOINT);
    if (output->respawn_platform.joint == NULL ||
        !resolve_local(&output->dat, respawn_table + 4, &source))
    {
        return fail(&output->import, HSD_DAT_DESCRIPTOR, respawn_table + 4);
    }
    output->respawn_platform.animation =
        HSD_DatImportGraph(&output->import, source, DAT_ANIMJOINT);
    if (output->respawn_platform.animation == NULL) {
        return false;
    }

    u32 damage_table;
    if (!resolve_local(&output->dat, root + 36, &damage_table) ||
        HSD_DatNextTarget(&output->dat, damage_table) - damage_table != 24)
    {
        return fail(&output->import, HSD_DAT_DESCRIPTOR, root + 36);
    }
    u32 damage_count = 0;
    for (u32 i = 0; i < 3; ++i) {
        u32 slot = damage_table + i * 8;
        u32 count;
        if (!resolve_local(&output->dat, slot, &source) ||
            !HSD_DatReadU32(&output->dat, slot + 4, &count) || count == 0 ||
            count > 13 - damage_count ||
            HSD_DatNextTarget(&output->dat, source) - source <
                count * sizeof(Vec2) ||
            !import_words(&output->dat, source,
                          &output->damage_shifts[damage_count],
                          count * sizeof(Vec2)))
        {
            return fail(&output->import, HSD_DAT_DESCRIPTOR, slot);
        }
        output->damage_shift_tables[i].shifts =
            &output->damage_shifts[damage_count];
        output->damage_shift_tables[i].count = count;
        damage_count += count;
    }
    if (damage_count != 13) {
        return fail(&output->import, HSD_DAT_DESCRIPTOR, damage_table);
    }

    struct Fighter_ShakeTable_t* shakes[2] = {
        &output->grab_mash_shake, &output->smash_charge_shake,
    };
    Vec2* shake_offsets[2] = {
        output->grab_mash_offsets, output->smash_charge_offsets,
    };
    for (u32 i = 0; i < 2; ++i) {
        u32 table;
        u32 count;
        u32 slot = root + 40 + i * 4;
        if (!resolve_local(&output->dat, slot, &table) ||
            (i == 1 ? root : HSD_DatNextTarget(&output->dat, table)) -
                    table != 8 ||
            !resolve_local(&output->dat, table, &source) ||
            !HSD_DatReadU32(&output->dat, table + 4, &count) || count != 5 ||
            HSD_DatNextTarget(&output->dat, source) - source <
                count * sizeof(Vec2) ||
            !import_words(&output->dat, source, shake_offsets[i],
                          count * sizeof(Vec2)))
        {
            return fail(&output->import, HSD_DAT_DESCRIPTOR, slot);
        }
        shakes[i]->x0 = shake_offsets[i];
        shakes[i]->x4 = count;
    }

    u32 color_table;
    u32 special_color_table;
    if (!resolve_local(&output->dat, root + 24, &color_table) ||
        !resolve_local(&output->dat, root + 28, &special_color_table) ||
        HSD_DatNextTarget(&output->dat, color_table) - color_table !=
            FT_COMMON_COLOR_ENTRY_COUNT * 8 ||
        HSD_DatNextTarget(&output->dat, special_color_table) -
                special_color_table !=
            FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT * 8)
    {
        return fail(&output->import, HSD_DAT_DESCRIPTOR, root + 24);
    }
    LbColorScriptTable color_tables[2] = {
        { color_table, FT_COMMON_COLOR_ENTRY_COUNT,
          output->color_entries },
        { special_color_table, FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT,
          output->special_color_entries },
    };
    if (!lbColorScript_Import(
            &output->dat, color_tables, 2, color_table,
            output->color_commands, FT_COMMON_COLOR_COMMAND_CAPACITY,
            &output->color_command_count, &output->import.status,
            &output->import.error_offset))
    {
        return false;
    }

    if (!resolve_local(&output->dat, root + 64, &source)) {
        return fail(&output->import, HSD_DAT_RELOCATION, root + 64);
    }
    output->entry_platform = HSD_DatImportGraph(&output->import, source,
                                                 DAT_JOINT);
    u8** colors[3] = { &output->x6510_colors, &output->x650C_colors,
                       &output->x6508_colors };
    for (u32 i = 0; i < 3; ++i) {
        if (!resolve_local(&output->dat, root + 68 + i * 4, &source) ||
            (*colors[i] = (u8*) HSD_DatBytes(&output->dat, source, 20)) == NULL)
        {
            return fail(&output->import, HSD_DAT_DESCRIPTOR,
                        root + 68 + i * 4);
        }
    }
    output->metal_model = HSD_DatImportGraph(&output->import, model, DAT_JOINT);
    u32 cpu_source;
    if (!resolve_local(&output->dat, root + 88, &cpu_source) ||
        !ftCpuData_Import(&output->import, cpu_source, &output->cpu_data))
        return false;
    return output->entry_platform != NULL && output->metal_model != NULL &&
           output->import.status == HSD_DAT_OK;
}
