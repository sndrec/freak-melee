#include "ftcpu_data.h"

#include <sysdolphin/baselib/dat_import_internal.h>

#include <string.h>

STATIC_ASSERT(sizeof(ftCo_AttackEntry) == 36);

static bool fail(HSD_DatImport* import, HSD_DatResult result, u32 offset)
{
    import->status = result;
    import->error_offset = offset;
    return false;
}

static bool local(HSD_DatImport* import, u32 slot, u32* target)
{
    const char* external;
    HSD_DatReferenceKind kind = HSD_DatResolve(import->dat, slot, target,
                                                &external);
    if (kind == HSD_DAT_REF_LOCAL) return true;
    import->external_symbol = external;
    return fail(import, kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                   : HSD_DAT_RELOCATION,
                slot);
}

static bool import_scripts(HSD_DatImport* import, u32 table,
                           FtCpuData* output)
{
    u32 ignored;
    const char* external;
    if (HSD_DatResolve(import->dat, table, &ignored, &external) !=
        HSD_DAT_REF_NULL)
        return fail(import, HSD_DAT_DESCRIPTOR, table);
    for (u32 index = 1; index < FT_CPU_SCRIPT_COUNT; ++index) {
        u32 source;
        u32 at;
        u32 end;
        if (!local(import, table + index * 4, &source)) return false;
        at = source;
        end = HSD_DatNextTarget(import->dat, source);
        bool terminated = false;
        while (at < end) {
            const u8* command = HSD_DatBytes(import->dat, at, 1);
            if (command == NULL)
                return fail(import, HSD_DAT_DESCRIPTOR, at);
            u32 width = *command <= 0x7F ? 1 : *command <= 0xBF ? 2 : 3;
            if (width > end - at ||
                HSD_DatBytes(import->dat, at, width) == NULL)
                return fail(import, HSD_DAT_DESCRIPTOR, at);
            ++output->command_count;
            at += width;
            if (*command == 0x7F) {
                terminated = true;
                break;
            }
        }
        if (!terminated)
            return fail(import, HSD_DAT_DESCRIPTOR, source);
        u32 length = at - source;
        output->scripts[index] = HSD_DatImportAlloc(import, length);
        if (output->scripts[index] == NULL) {
            import->error_offset = source;
            return false;
        }
        memcpy(output->scripts[index], HSD_DatBytes(import->dat, source,
                                                     length), length);
        ++output->script_count;
        output->script_byte_count += length;
    }
    output->descriptor.cmdscripts = output->scripts;
    return true;
}

static bool import_attacks(HSD_DatImport* import, u32 table,
                           ftCo_AttackEntry** output, FtCpuData* stats)
{
    for (u32 fighter = 0; fighter < FT_CPU_FIGHTER_COUNT; ++fighter) {
        u32 source;
        if (!local(import, table + fighter * 4, &source)) return false;
        u32 end = HSD_DatNextTarget(import->dat, source);
        u32 count;
        for (count = 0; count <= FT_CPU_ATTACK_MAX_ENTRIES; ++count) {
            if (source > end ||
                count + 1 > (end - source) / sizeof(ftCo_AttackEntry))
                return fail(import, HSD_DAT_DESCRIPTOR, source);
            u32 at = source + count * sizeof(ftCo_AttackEntry);
            u32 command;
            if (!HSD_DatReadU32(import->dat, at, &command))
                return fail(import, HSD_DAT_DESCRIPTOR, at);
            if (command == 0) break;
        }
        if (count > FT_CPU_ATTACK_MAX_ENTRIES)
            return fail(import, HSD_DAT_DESCRIPTOR, source);
        ftCo_AttackEntry* entries = HSD_DatImportAlloc(
            import, (size_t) (count + 1) * sizeof(*entries));
        if (entries == NULL) {
            import->error_offset = source;
            return false;
        }
        for (u32 entry = 0; entry <= count; ++entry) {
            for (u32 word = 0; word < 9; ++word) {
                u32 value;
                u32 at = source + entry * sizeof(*entries) + word * 4;
                if (!HSD_DatReadU32(import->dat, at, &value))
                    return fail(import, HSD_DAT_DESCRIPTOR, at);
                memcpy((u8*) &entries[entry] + word * 4, &value, 4);
            }
        }
        output[fighter] = entries;
        ++stats->attack_array_count;
        stats->attack_entry_count += count;
    }
    return true;
}

bool ftCpuData_Import(HSD_DatImport* import, u32 source, FtCpuData* output)
{
    if (import == NULL) return false;
    if (output == NULL || (source & 3) != 0 ||
        source > import->dat->data_size ||
        import->dat->data_size - source < 48 ||
        HSD_DatNextTarget(import->dat, source) - source != 48)
        return fail(import, HSD_DAT_DESCRIPTOR, source);
    memset(output, 0, sizeof(*output));
    u32 tail0, tail1;
    if (!HSD_DatReadU32(import->dat, source + 40, &tail0) ||
        !HSD_DatReadU32(import->dat, source + 44, &tail1) ||
        tail0 != 0 || tail1 != 0)
        return fail(import, HSD_DAT_DESCRIPTOR, source + 40);
    u32 tables[10];
    for (u32 i = 0; i < ARRAY_SIZE(tables); ++i) {
        u32 expected_size = i == 0 ? FT_CPU_SCRIPT_COUNT * 4
                            : i == 9 ? 6 * 4
                                     : FT_CPU_FIGHTER_COUNT * 4;
        if (!local(import, source + i * 4, &tables[i]) ||
            HSD_DatNextTarget(import->dat, tables[i]) - tables[i] !=
                expected_size)
            return fail(import, HSD_DAT_DESCRIPTOR, source + i * 4);
    }
    if (!import_scripts(import, tables[0], output)) return false;
    for (u32 family = 0; family < FT_CPU_ATTACK_FAMILIES; ++family)
        if (!import_attacks(import, tables[family + 1],
                            output->attacks[family], output)) return false;
    for (u32 i = 0; i < FT_CPU_FIGHTER_COUNT; ++i)
        if (!HSD_DatReadF32(import->dat, tables[8] + i * 4,
                            &output->distance_thresholds[i]))
            return fail(import, HSD_DAT_DESCRIPTOR, tables[8] + i * 4);
    // PlCo.dat only has stock entries. Native fighter definitions may replace
    // these safe empty lists with authored CPU attacks later.
    for (u32 family = 0; family < FT_CPU_ATTACK_FAMILIES; ++family) {
        for (u32 kind = FT_CPU_FIGHTER_COUNT;
             kind < FT_CPU_RUNTIME_FIGHTER_COUNT; ++kind)
        {
            output->attacks[family][kind] = &output->empty_attack;
        }
    }
    for (u32 i = 0; i < ARRAY_SIZE(output->weapon_reach); ++i)
        if (!HSD_DatReadF32(import->dat, tables[9] + i * 4,
                            &output->weapon_reach[i]))
            return fail(import, HSD_DAT_DESCRIPTOR, tables[9] + i * 4);
    struct Fighter_804D64FC_t* desc = &output->descriptor;
    desc->x4 = output->attacks[0];
    desc->x8 = output->attacks[1];
    desc->xC = output->attacks[2];
    desc->x10 = output->attacks[3];
    desc->x14 = output->attacks[4];
    desc->x18 = output->attacks[5];
    desc->x1C = output->attacks[6];
    desc->x20 = output->distance_thresholds;
    desc->x24 = output->weapon_reach;
    return true;
}
