#include "native_fighter_rig.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static bool error(char* text, size_t capacity, const char* message)
{
    if (text && capacity) snprintf(text, capacity, "%s", message);
    return false;
}
static unsigned bucket(uint32_t id)
{ return (id * UINT32_C(2654435761)) & (NATIVE_RIG_HASH_SLOTS - 1); }
bool nativeFighterRig_Index(NativeFighterRig* rig, char* text, size_t capacity)
{
    if (!rig || !rig->count || rig->count > MAX_FT_PARTS ||
        rig->next_id < MAX_FT_PARTS || rig->next_id > INT32_MAX)
        return error(text, capacity, "Bone IDs need a bounded rig and next ID.");
    uint32_t keys[NATIVE_RIG_HASH_SLOTS];
    int16_t parts[NATIVE_RIG_HASH_SLOTS];
    memset(keys, 255, sizeof(keys));
    memset(parts, 255, sizeof(parts));
    unsigned mapped = 0;
    for (unsigned i = 0; i < MAX_FT_PARTS; ++i) {
        if (rig->ids[i] == UINT32_MAX) {
            if (i < rig->count || rig->flags[i] || *rig->names[i])
                return error(text, capacity, "A missing bone ID still has a live part, name or editing flags.");
            continue;
        }
        ++mapped;
        if (rig->ids[i] >= rig->next_id || rig->flags[i] & ~NATIVE_RIG_EXTRA)
            return error(text, capacity, "Bone ID or editing flags are invalid.");
        size_t n = strnlen(rig->names[i], 64);
        if (n == 64) return error(text, capacity, "A bone name exceeds 63 bytes.");
        for (size_t k = 0; k < n; ++k)
            if ((unsigned char)rig->names[i][k] < 32 || rig->names[i][k] == 127)
                return error(text, capacity, "Bone names must contain printable text.");
        for (unsigned j = 0; n && j < i; ++j)
            if (!strcmp(rig->names[i], rig->names[j]))
                return error(text, capacity, "Bone names must be unique.");
        unsigned at = bucket(rig->ids[i]);
        while (keys[at] != UINT32_MAX) {
            if (keys[at] == rig->ids[i]) return error(text, capacity, "Bone IDs must be unique.");
            at = (at + 1) & (NATIVE_RIG_HASH_SLOTS - 1);
        }
        keys[at] = rig->ids[i]; parts[at] = (int16_t)i;
    }
    memcpy(rig->keys, keys, sizeof(keys));
    memcpy(rig->parts, parts, sizeof(parts));
    rig->mapped = (uint16_t)mapped;
    rig->authored = true;
    if (text && capacity) text[0] = 0;
    return true;
}
int32_t nativeFighterRig_Find(const NativeFighterRig* rig, const char* name)
{
    if (rig && name && *name) for (unsigned i = 0; i < MAX_FT_PARTS; ++i)
        if (!strcmp(name, rig->names[i])) return (int32_t)rig->ids[i];
    return -1;
}
bool nativeFighterRig_Read(const HSD_Dat* dat, unsigned parts, NativeFighterRig* output,
    char* text, size_t capacity)
{
    if (!dat || !output || !parts || parts > MAX_FT_PARTS)
        return error(text, capacity, "Bone IDs require a valid fighter rig.");
    NativeFighterRig rig = {0}; rig.count = (uint16_t)parts;
    rig.next_id = MAX_FT_PARTS; rig.mapped = MAX_FT_PARTS;
    u32 root;
    if (!HSD_DatFindRoot(dat, NATIVE_RIG_ROOT, &root)) {
        for (unsigned i = 0; i < MAX_FT_PARTS; ++i) rig.ids[i] = i;
        *output = rig;
        if (text && capacity) text[0] = 0;
        return true;
    } else {
        unsigned matches = 0;
        for (u32 i = 0; i < dat->root_count; ++i) {
            u32 at; const char* name;
            if (!HSD_DatSymbol(dat, false, i, &at, &name))
                return error(text, capacity, "A fighter root is invalid.");
            matches += !strcmp(name, NATIVE_RIG_ROOT);
        }
        if (matches != 1) return error(text, capacity, "The fighter has duplicate bone-ID roots.");
        u32 magic, next, count, mapped, table; const char* external;
        if (!HSD_DatReadU32(dat, root, &magic) || magic != UINT32_C(0x52494742) ||
            !HSD_DatReadU32(dat, root + 4, &next) ||
            !HSD_DatReadU32(dat, root + 8, &count) || count != parts ||
            !HSD_DatReadU32(dat, root + 12, &mapped) || mapped > MAX_FT_PARTS ||
            HSD_DatResolve(dat, root + 16, &table, &external) != HSD_DAT_REF_LOCAL ||
            !HSD_DatBytes(dat, table, MAX_FT_PARTS * (12 + 64)))
            return error(text, capacity, "The fighter's bone-ID table is malformed or has a different part count.");
        rig.next_id = (uint32_t)next;
        bool seen[MAX_FT_PARTS] = {0};
        for (unsigned i = 0; i < MAX_FT_PARTS; ++i) {
            u32 at = table + i * 12, id, name; u16 part, flags;
            if (!HSD_DatReadU32(dat, at, &id) || !HSD_DatReadU16(dat, at + 4, &part) ||
                !HSD_DatReadU16(dat, at + 6, &flags) || part != i || seen[part] ||
                (flags & ~NATIVE_RIG_EXTRA)) return error(text, capacity, "A bone-ID entry has an invalid or repeated part.");
            seen[part] = true; rig.ids[part] = (uint32_t)id; rig.flags[part] = (uint8_t)flags;
            HSD_DatReferenceKind kind = HSD_DatResolve(dat, at + 8, &name, &external);
            if (kind == HSD_DAT_REF_LOCAL) {
                if (name != table + MAX_FT_PARTS * 12 + i * 64)
                    return error(text, capacity, "A bone name is outside its owned name slot.");
                const char* value = (const char*)HSD_DatBytes(dat, name, 1);
                size_t available = dat->data_size - name;
                const char* end = value ? memchr(value, 0, available < 64 ? available : 64) : NULL;
                if (!end) return error(text, capacity, "A bone name is missing its terminator.");
                memcpy(rig.names[part], value, (size_t)(end - value));
            } else if (kind != HSD_DAT_REF_NULL)
                return error(text, capacity, "A bone name has an invalid or external reference.");
        }
        rig.mapped = (uint16_t)mapped;
    }
    unsigned mapped = rig.mapped;
    if (!nativeFighterRig_Index(&rig, text, capacity)) return false;
    if (mapped != rig.mapped) return error(text, capacity, "The bone-ID mapping count is invalid.");
    *output = rig;
    return true;
}
