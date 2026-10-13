#include "native_fighter_cpu.h"
#include <sysdolphin/baselib/dat.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

static const char* const family_names[FT_CPU_ATTACK_FAMILIES] = {
    "ground", "air", "ranged", "smash", "special", "weapon", "edge"
};

static bool valid_entry(const ftCo_AttackEntry* entry)
{
    return entry->cmd > 0 && entry->cmd < FT_CPU_SCRIPT_COUNT &&
           entry->x04 >= 0 && entry->x1C > 0 && entry->x20 >= 0 &&
           entry->x20 <= 9 && isfinite(entry->x08) && isfinite(entry->x0C) &&
           isfinite(entry->x10) && isfinite(entry->x14) &&
           isfinite(entry->weight);
}

bool nativeFighterCpu_Parse(const char* text, size_t size,
                            NativeFighterCpuProfile* profile)
{
    if (profile == NULL || (size != 0 && text == NULL)) return false;
    memset(profile, 0, sizeof(*profile));
    if (size == 0) return true; // No authored CPU attacks: all lists are empty.
    unsigned counts[FT_CPU_ATTACK_FAMILIES] = { 0 };
    bool distance_seen = false;
    for (size_t at = 0; at < size;) {
        size_t end = at;
        while (end < size && text[end] != '\n') ++end;
        if (end - at >= 512 || memchr(text + at, '\0', end - at)) return false;
        char line[512];
        memcpy(line, text + at, end - at);
        line[end - at] = 0;
        at = end < size ? end + 1 : end;
        char* comment = strchr(line, '#');
        if (comment) *comment = 0;
        char name[32], extra;
        if (sscanf(line, " %31s", name) != 1) continue;
        if (strcmp(name, "distance") == 0) {
            if (distance_seen || sscanf(line, " %31s %f %c", name,
                                       &profile->distance_threshold, &extra) != 2 ||
                !isfinite(profile->distance_threshold) ||
                profile->distance_threshold < 0) return false;
            distance_seen = true;
            continue;
        }
        unsigned family = 0;
        while (family < FT_CPU_ATTACK_FAMILIES &&
               strcmp(name, family_names[family]) != 0) ++family;
        if (family == FT_CPU_ATTACK_FAMILIES ||
            counts[family] == FT_CPU_ATTACK_MAX_ENTRIES) return false;
        ftCo_AttackEntry entry = { 0 };
        // s32 is long on the native Windows build, not int.
        if (sscanf(line, " %31s %ld %ld %f %f %f %f %f %ld %ld %c", name,
                   &entry.cmd, &entry.x04, &entry.x08, &entry.x0C,
                   &entry.x10, &entry.x14, &entry.weight, &entry.x1C,
                   &entry.x20, &extra) != 10 || !valid_entry(&entry)) return false;
        profile->attacks[family][counts[family]++] = entry;
    }
    return distance_seen;
}

bool nativeFighterCpu_Write(FILE* output, const NativeFighterCpuProfile* profile)
{
    if (output == NULL || profile == NULL ||
        !isfinite(profile->distance_threshold) || profile->distance_threshold < 0)
        return false;
    if (fprintf(output, "# family command prediction_frames x_min x_max y_min y_max weight period min_level\n"
                        "distance %a\n", (double) profile->distance_threshold) < 0) return false;
    for (unsigned family = 0; family < FT_CPU_ATTACK_FAMILIES; ++family) {
        unsigned count;
        for (count = 0; count <= FT_CPU_ATTACK_MAX_ENTRIES; ++count) {
            const ftCo_AttackEntry* entry = &profile->attacks[family][count];
            if (entry->cmd == 0) break;
            if (count == FT_CPU_ATTACK_MAX_ENTRIES || !valid_entry(entry) ||
                fprintf(output, "%s %ld %ld %a %a %a %a %a %ld %ld\n",
                        family_names[family], entry->cmd, entry->x04,
                        (double) entry->x08, (double) entry->x0C,
                        (double) entry->x10, (double) entry->x14,
                        (double) entry->weight, entry->x1C, entry->x20) < 0) return false;
        }
        if (count > FT_CPU_ATTACK_MAX_ENTRIES) return false;
    }
    return !ferror(output);
}

static bool local_target(const HSD_Dat* dat, u32 at, u32* target)
{
    const char* external;
    return HSD_DatResolve(dat, at, target, &external) == HSD_DAT_REF_LOCAL;
}

bool nativeFighterCpu_ImportStock(const void* bytes, size_t size, FighterKind kind,
                                  NativeFighterCpuProfile* profile)
{
    HSD_Dat dat;
    u32 root, descriptor, table;
    if (profile == NULL || kind < 0 || kind >= FT_CPU_FIGHTER_COUNT ||
        HSD_DatOpen(&dat, bytes, size) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&dat, "ftLoadCommonData", &root) ||
        !local_target(&dat, root + 88, &descriptor)) return false;
    memset(profile, 0, sizeof(*profile));
    if (!local_target(&dat, descriptor + 32, &table) ||
        !HSD_DatReadF32(&dat, table + kind * 4, &profile->distance_threshold) ||
        !isfinite(profile->distance_threshold) || profile->distance_threshold < 0)
        return false;
    for (u32 family = 0; family < FT_CPU_ATTACK_FAMILIES; ++family) {
        u32 source;
        if (!local_target(&dat, descriptor + 4 + family * 4, &table) ||
            !local_target(&dat, table + kind * 4, &source)) return false;
        u32 end = HSD_DatNextTarget(&dat, source);
        bool terminated = false;
        for (u32 entry = 0; entry <= FT_CPU_ATTACK_MAX_ENTRIES; ++entry) {
            u32 at = source + entry * 36, command;
            if (at > end || end - at < 36 || !HSD_DatReadU32(&dat, at, &command))
                return false;
            if (command == 0) { terminated = true; break; }
            if (entry == FT_CPU_ATTACK_MAX_ENTRIES) return false;
            ftCo_AttackEntry* output = &profile->attacks[family][entry];
            for (u32 word = 0; word < 9; ++word) {
                u32 value;
                if (!HSD_DatReadU32(&dat, at + word * 4, &value)) return false;
                memcpy((u8*) output + word * 4, &value, 4);
            }
            if (!valid_entry(output)) return false;
        }
        if (!terminated) return false;
    }
    return true;
}
