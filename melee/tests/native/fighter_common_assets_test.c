#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/native_fighter_cpu.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/lb/lb_013B.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

static void unused_color_callback(Fighter_GObj* gobj, CommandInfo* command,
                                  int opcode)
{
    require(false, "unexpected fighter color callback\n");
}

static u32 native_word(const void* data, u32 index)
{
    u32 value;
    memcpy(&value, (const u8*) data + index * sizeof(value), sizeof(value));
    return value;
}

static void audit_cpu_data(const HSD_Dat* dat, u32 descriptor_source,
                           const FtCpuData* cpu)
{
    const char* external;
    u32 tables[10];
    for (u32 table = 0; table < 10; ++table) {
        require(HSD_DatResolve(dat, descriptor_source + table * 4,
                               &tables[table], &external) ==
                    HSD_DAT_REF_LOCAL,
                "resolve retail CPU table\n");
    }

    u32 script_bytes = 0;
    u32 script_commands = 0;
    for (u32 script = 1; script < FT_CPU_SCRIPT_COUNT; ++script) {
        u32 source;
        require(HSD_DatResolve(dat, tables[0] + script * 4, &source,
                               &external) == HSD_DAT_REF_LOCAL,
                "resolve retail CPU command script\n");
        u32 offset = 0;
        for (;;) {
            const u8* command = HSD_DatBytes(dat, source + offset, 1);
            require(command != NULL, "read retail CPU command\n");
            u32 width = *command <= 0x7F ? 1 : *command <= 0xBF ? 2 : 3;
            const u8* bytes = HSD_DatBytes(dat, source + offset, width);
            require(bytes != NULL &&
                        memcmp(cpu->scripts[script] + offset, bytes, width) == 0,
                    "native CPU command differs from retail source\n");
            offset += width;
            ++script_commands;
            if (*command == 0x7F) break;
        }
        script_bytes += offset;
    }
    require(script_bytes == cpu->script_byte_count &&
                script_commands == cpu->command_count,
            "CPU command audit coverage\n");

    u32 attack_entries = 0;
    for (u32 family = 0; family < FT_CPU_ATTACK_FAMILIES; ++family) {
        u32 table = tables[family + 1];
        for (u32 kind = 0; kind < FT_CPU_FIGHTER_COUNT; ++kind) {
            u32 source;
            require(HSD_DatResolve(dat, table + kind * 4, &source,
                                   &external) == HSD_DAT_REF_LOCAL,
                    "resolve retail CPU attack array\n");
            const ftCo_AttackEntry* entries = cpu->attacks[family][kind];
            for (u32 entry = 0; entry <= 32; ++entry) {
                for (u32 word = 0; word < 9; ++word) {
                    u32 retail;
                    require(HSD_DatReadU32(
                                dat, source + entry * sizeof(*entries) +
                                         word * sizeof(retail),
                                &retail) &&
                                native_word(&entries[entry], word) == retail,
                            "native CPU attack field differs from retail source\n");
                }
                if (entries[entry].cmd == 0) break;
                ++attack_entries;
                require(entry < 32, "CPU attack array terminator\n");
            }
        }
    }
    require(attack_entries == cpu->attack_entry_count,
            "CPU attack audit coverage\n");

    for (u32 kind = 0; kind < FT_CPU_FIGHTER_COUNT; ++kind) {
        u32 retail;
        require(HSD_DatReadU32(dat, tables[8] + kind * 4, &retail) &&
                    native_word(&cpu->distance_thresholds[kind], 0) == retail,
                "native CPU distance differs from retail source\n");
    }
    for (u32 weapon = 0; weapon < 6; ++weapon) {
        u32 retail;
        require(HSD_DatReadU32(dat, tables[9] + weapon * 4, &retail) &&
                    native_word(&cpu->weapon_reach[weapon], 0) == retail,
                "native CPU weapon reach differs from retail source\n");
    }
}

int main(int argc, char** argv)
{
    DVDFileInfo file;
    require(argc == 2, "expected ISO path\n");
    require(aurora_dvd_open(argv[1]) && DVDOpen("/PlCo.dat", &file),
            "open PlCo.dat\n");
    size_t size = file.length;
    void* bytes = _aligned_malloc((size + 31) & ~(size_t) 31, 32);
    enum { ARENA_SIZE = 16 * 1024 * 1024 };
    void* arena = _aligned_malloc(ARENA_SIZE, sizeof(void*));
    require(bytes != NULL && arena != NULL &&
                DVDReadPrio(&file, bytes, (size + 31) & ~(size_t) 31, 0, 2) >=
                    (s32) size && DVDClose(&file),
            "read PlCo.dat\n");

    FtCommonAssets assets;
    if (!ftCommonAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                               &assets)) {
        fprintf(stderr, "fighter common status=%d offset=%lx external=%s\n",
                assets.import.status, assets.import.error_offset,
                assets.import.external_symbol != NULL
                    ? assets.import.external_symbol : "-");
        require(false, "import fighter common assets\n");
    }
    require(assets.parts[Ft_Kind_None] != NULL &&
                assets.parts[Ft_Kind_None]->parts_num == 53 &&
                assets.omissions[Ft_Kind_None] != NULL,
            "import the source's generic thrown-animation part table\n");
    require(assets.common.x194_unkHitLagFrames == 20.0F &&
                assets.common.x198 > 0.333F &&
                assets.common.x198 < 0.334F &&
                assets.common.x19C == 3.0F &&
                assets.common.x1A0 > 0.666F &&
                assets.common.x1A0 < 0.667F &&
                assets.common.horizontal_stick_deadzone == 0.28F &&
                assets.common.vertical_stick_deadzone == 0.28F &&
                assets.item_throw_attributes[0] > 2.59F &&
                assets.item_throw_attributes[0] < 2.61F &&
                assets.swing_multipliers[0][0] == 1.0F &&
                assets.stale_move_penalties[0] > 0.08F &&
                assets.stale_move_penalties[0] < 0.10F &&
                assets.parts[Ft_Kind_Mario]->parts_num > 40 &&
                assets.parts[Ft_Kind_Mars]->parts_num > 40 &&
                sizeof(assets.crowd_config) == 0x44 &&
                assets.crowd_config.cheer_limit > 0 &&
                assets.crowd_config.max_gasp_count > 0 &&
                assets.entry_platform != NULL &&
                assets.respawn_platform.joint != NULL &&
                assets.respawn_platform.animation != NULL &&
                assets.color_entries[0].unk == NULL &&
                assets.color_entries[117].unk != NULL &&
                ((union ColorOverlay_x8_t*)
                     assets.color_entries[117].unk)->unk.unk == 18 &&
                assets.special_color_entries[5].unk != NULL &&
                assets.color_command_count == 1972 &&
                assets.damage_shift_tables[0].count == 4 &&
                assets.damage_shift_tables[1].count == 4 &&
                assets.damage_shift_tables[2].count == 5 &&
                assets.damage_shift_tables[0].shifts == assets.damage_shifts &&
                assets.damage_shift_tables[2].shifts ==
                    assets.damage_shifts + 8 &&
                assets.grab_mash_shake.x0 == assets.grab_mash_offsets &&
                assets.grab_mash_shake.x4 == 5 &&
                assets.smash_charge_shake.x0 ==
                    assets.smash_charge_offsets &&
                assets.smash_charge_shake.x4 == 5 &&
                assets.x6510_colors != NULL &&
                assets.x650C_colors != NULL &&
                assets.x6508_colors != NULL &&
                assets.cpu_data.script_count == 61 &&
                assets.cpu_data.script_byte_count == 1158 &&
                assets.cpu_data.command_count == 652 &&
                assets.cpu_data.attack_array_count == 224 &&
                assets.cpu_data.attack_entry_count == 1136 &&
                assets.cpu_data.descriptor.cmdscripts ==
                    assets.cpu_data.scripts &&
                assets.cpu_data.descriptor.x4 ==
                    assets.cpu_data.attacks[0] &&
                assets.cpu_data.descriptor.x1C ==
                    assets.cpu_data.attacks[6] &&
                assets.cpu_data.descriptor.x20 ==
                    assets.cpu_data.distance_thresholds &&
                assets.cpu_data.descriptor.x24 ==
                    assets.cpu_data.weapon_reach &&
                assets.metal_model != NULL,
            "fighter common dimensions\n");

    ftCommonData* previous_common = p_ftCommonData;
    p_ftCommonData = &assets.common;
    require(ftCommon_CalcHitlag(1, ftCo_MS_Wait, 1.0F) == 3.0F &&
                ftCommon_CalcHitlag(22, ftCo_MS_Wait, 1.0F) == 10.0F,
            "hitlag must scale with the full damage value\n");
    p_ftCommonData = previous_common;

    require(assets.cpu_data.scripts[0] == NULL &&
                assets.cpu_data.scripts[1] != NULL &&
                assets.cpu_data.scripts[1][0] == 0x92 &&
                assets.cpu_data.scripts[1][8] == 0x7F &&
                assets.cpu_data.distance_thresholds[0] == 15.5F &&
                assets.cpu_data.weapon_reach[0] < -1.59F &&
                assets.cpu_data.weapon_reach[0] > -1.61F,
            "CPU scripts and scalar tables\n");
    u32 attack_entries = 0;
    for (u32 family = 0; family < FT_CPU_ATTACK_FAMILIES; ++family) {
        for (u32 kind = 0; kind < FT_CPU_FIGHTER_COUNT; ++kind) {
            ftCo_AttackEntry* entries = assets.cpu_data.attacks[family][kind];
            require(entries != NULL, "CPU attack array pointer\n");
            u32 count = 0;
            while (count <= FT_CPU_ATTACK_MAX_ENTRIES &&
                   entries[count].cmd != 0) ++count;
            require(count <= FT_CPU_ATTACK_MAX_ENTRIES,
                    "CPU attack array terminator\n");
            attack_entries += count;
        }
    }
    require(attack_entries == 1136, "CPU attack record coverage\n");
    for (u32 family = 0; family < FT_CPU_ATTACK_FAMILIES; ++family) {
        for (u32 kind = FT_CPU_FIGHTER_COUNT;
             kind < FT_CPU_RUNTIME_FIGHTER_COUNT; ++kind)
        {
            require(assets.cpu_data.attacks[family][kind] ==
                        &assets.cpu_data.empty_attack &&
                        assets.cpu_data.attacks[family][kind]->cmd == 0 &&
                        assets.cpu_data.distance_thresholds[kind] == 0,
                    "custom CPU kinds use a safe empty attack list\n");
        }
    }
    // Every imported stock profile must agree with the existing typed importer,
    // and survive exact text export/import without retaining donor pointers.
    for (FighterKind kind = 0; kind < FT_CPU_FIGHTER_COUNT; ++kind) {
        NativeFighterCpuProfile profile, parsed;
        require(nativeFighterCpu_ImportStock(bytes, size, kind, &profile),
                "import independent CPU profile\n");
        require(profile.distance_threshold == assets.cpu_data.distance_thresholds[kind],
                "CPU distance threshold matches stock\n");
        for (unsigned family = 0; family < FT_CPU_ATTACK_FAMILIES; ++family) {
            const ftCo_AttackEntry* source = assets.cpu_data.attacks[family][kind];
            unsigned count = 0;
            while (source[count].cmd != 0) ++count;
            require(memcmp(profile.attacks[family], source, count * sizeof(*source)) == 0 &&
                    profile.attacks[family][count].cmd == 0, "CPU rows match stock\n");
        }
        FILE* file = tmpfile();
        require(file && nativeFighterCpu_Write(file, &profile), "write CPU tuning\n");
        long length = ftell(file);
        char text[65536];
        require(length > 0 && length < sizeof(text) && fseek(file, 0, SEEK_SET) == 0 &&
                fread(text, 1, length, file) == length &&
                nativeFighterCpu_Parse(text, length, &parsed) &&
                memcmp(&profile, &parsed, sizeof(profile)) == 0,
                "CPU text preserves all binary32 values exactly\n");
        fclose(file);
    }
    NativeFighterCpuProfile invalid_cpu;
    require(nativeFighterCpu_Parse(NULL, 0, &invalid_cpu) &&
            invalid_cpu.attacks[0][0].cmd == 0,
            "an unauthored CPU profile has no donor fallback\n");
    const char* invalid_profiles[] = {
        "distance nan\n", "distance 1\ndistance 2\n", "distance -1\n",
        "distance 1\nunknown 1\n", "distance 1 trailing\n",
        "distance 1\nground 62 5 -2 15 -5 7 1 1 0\n",
        "distance 1\nground 1 5 -2 15 -5 7 1 0 0\n",
        "distance 1\nground 1 5 -2 15 -5 7 inf 1 0\n",
        "distance 1\nground 1 5 -2 15 -5 7 1 1 10\n",
    };
    for (unsigned i = 0; i < ARRAY_SIZE(invalid_profiles); ++i)
        require(!nativeFighterCpu_Parse(invalid_profiles[i], strlen(invalid_profiles[i]), &invalid_cpu),
                "reject invalid authored CPU tuning\n");
    Fighter cpu_fighter = { 0 };
    struct Fighter_804D64FC_t* previous_cpu_data = Fighter_804D64FC;
    Fighter_804D64FC = &assets.cpu_data.descriptor;
    ftCo_800B462C(&cpu_fighter);
    ftCo_800B4880(&cpu_fighter, 1);
    require(cpu_fighter.cpu.write_pos - cpu_fighter.cpu.buffer == 9 &&
                memcmp(cpu_fighter.cpu.buffer,
                       assets.cpu_data.scripts[1], 9) == 0,
            "CPU command runner copies imported script\n");
    Fighter_804D64FC = previous_cpu_data;

    ColorOverlay overlay = { 0 };
    lb_80014498(&overlay);
    require(lb_800144C8(&overlay, assets.color_entries, 117, 0),
            "start entry-animation color script\n");
    bool color_complete = false;
    for (u32 frame = 0; frame < 1000; ++frame) {
        if (lb_80014258(NULL, &overlay, unused_color_callback)) {
            color_complete = true;
            break;
        }
    }
    require(color_complete,
            "entry-animation color script reaches its terminator\n");

    u32 root;
    u32 color_table;
    u32 callback_source;
    u32 callback_word;
    const char* external;
    require(HSD_DatFindRoot(&assets.dat, "ftLoadCommonData", &root) &&
                HSD_DatResolve(&assets.dat, root + 24, &color_table,
                               &external) == HSD_DAT_REF_LOCAL &&
                HSD_DatResolve(&assets.dat, color_table + 118 * 8,
                               &callback_source, &external) ==
                    HSD_DAT_REF_LOCAL &&
                HSD_DatReadU32(&assets.dat, callback_source,
                               &callback_word),
            "find retail fighter color callback\n");
    u32 cpu_source;
    require(HSD_DatResolve(&assets.dat, root + 88, &cpu_source,
                           &external) == HSD_DAT_REF_LOCAL,
            "find retail CPU descriptor\n");
    audit_cpu_data(&assets.dat, cpu_source, &assets.cpu_data);
    union CmdUnion* callback = assets.color_entries[118].unk;
    require(callback_word >> 26 == 21 &&
                callback->spawn_gfx_0.opcode == 21 &&
                callback->spawn_gfx_0.boneId ==
                    ((callback_word >> 18) & 255) &&
                callback->spawn_gfx_0.useCommonBoneIDs ==
                    ((callback_word >> 17) & 1),
            "native color callback fields match retail word\n");
    union ColorOverlay_x8_t* rotation = assets.color_entries[43].unk;
    union ColorOverlay_x8_t* position = assets.color_entries[10].unk;
    require(rotation->unk.unk == 13 &&
                rotation->light_rot2.x == 180 &&
                rotation->light_rot2.yz == -10 &&
                position->unk.unk == 16 &&
                position->light_rot1.x == 0 &&
                position->light_rot1.yz == -70,
            "native color rotation fields match retail words\n");
    FtCommonAssets truncated;
    require(!ftCommonAssets_Import(bytes, size - 1, arena, ARENA_SIZE,
                                   32768, &truncated),
            "reject truncated PlCo.dat\n");
    _aligned_free(arena);
    _aligned_free(bytes);
    puts("fighter common native assets imported");
    return 0;
}
