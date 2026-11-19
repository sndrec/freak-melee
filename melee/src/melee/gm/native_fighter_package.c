#include "native_content_store.h"
#include <melee/ft/native_source_package.h>

#ifdef MELEE_NATIVE

#include <stdlib.h>
#include <string.h>

enum { FAMILY_HEADER = 80, MEMBER_DIRECTORY = 64, MEMBER_CHUNKS = 8,
       FAMILY_PAYLOAD = FAMILY_HEADER + NATIVE_FIGHTER_FAMILY_MEMBERS * MEMBER_DIRECTORY };

static uint32_t read_u32(const uint8_t* p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void write_u32(uint8_t* p, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(value >> (i * 8));
}

static bool zeros(const uint8_t* p, size_t size)
{
    for (size_t i = 0; i < size; ++i) if (p[i] != 0) return false;
    return true;
}

static bool valid_key(const char* key, size_t* length)
{
    size_t count = strnlen(key, NATIVE_FIGHTER_CODE_KEY_BYTES);
    if (count == 0 || count == NATIVE_FIGHTER_CODE_KEY_BYTES) return false;
    for (size_t i = 0; i < count; ++i) {
        char c = key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '-' || c == '_')) return false;
    }
    *length = count;
    return true;
}

static bool valid_mode(NativeFighterFamilyMode mode, unsigned count)
{
    return (mode == NativeFighterFamily_Single && count == 1) ||
           ((mode == NativeFighterFamily_AlternateForm ||
             mode == NativeFighterFamily_SimultaneousCompanion) && count == 2);
}

static uint32_t read_be_u32(const uint8_t* p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8 | p[3];
}

bool nativeContent_ValidFighterPortrait(const uint8_t* p, size_t size)
{
    static const uint8_t png_magic[8] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
    if (p == NULL || size < 45 || size > 1024 * 1024 ||
        memcmp(p, png_magic, 8) != 0 || read_be_u32(p + 8) != 13 ||
        memcmp(p + 12, "IHDR", 4) != 0) return false;
    uint32_t width = read_be_u32(p + 16), height = read_be_u32(p + 20);
    return read_be_u32(p + size - 12) == 0 &&
        memcmp(p + size - 8, "IEND", 4) == 0 &&
        width >= 16 && width <= 512 && height >= 16 && height <= 512 &&
        p[24] == 8 && (p[25] == 2 || p[25] == 6) &&
        p[26] == 0 && p[27] == 0 && p[28] == 0;
}

static void chunks(const NativeFighterMemberData* member,
                   const uint8_t* pointers[MEMBER_CHUNKS],
                   size_t sizes[MEMBER_CHUNKS])
{
    pointers[0] = member->fighter_dat; sizes[0] = member->fighter_size;
    pointers[1] = member->animation_dat; sizes[1] = member->animation_size;
    pointers[2] = member->costume_dat; sizes[2] = member->costume_size;
    pointers[3] = member->asset_record; sizes[3] = member->asset_record_size;
    pointers[4] = member->script_text; sizes[4] = member->script_size;
    pointers[5] = member->parameter_text; sizes[5] = member->parameter_size;
    pointers[6] = member->cpu_text; sizes[6] = member->cpu_size;
    pointers[7] = member->resources; sizes[7] = member->resource_size;
}

static bool valid_sizes(const size_t sizes[MEMBER_CHUNKS])
{
    if (sizes[0] < 32 || sizes[1] < 32 || sizes[2] < 32 ||
        sizes[0]>NATIVE_SOURCE_DAT_BYTES||sizes[1]>NATIVE_SOURCE_DAT_BYTES||sizes[2]>NATIVE_SOURCE_DAT_BYTES||
        sizes[3]<4||sizes[3]>NATIVE_SOURCE_FACT_BYTES||!sizes[4]||sizes[4]>=NATIVE_SOURCE_SOURCE_BYTES||
        sizes[5]>NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY||
        sizes[7]>NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES) return false;
    for (unsigned i = 3; i < MEMBER_CHUNKS; ++i)
        if (sizes[i] > 0xFFFFFF) return false;
    return true;
}

static bool read_family_title(const uint8_t* bytes, size_t size, char* title)
{
    if (!bytes || size < FAMILY_HEADER || memcmp(bytes, "FMSC", 4) != 0 ||
        bytes[4] != 0 || bytes[7] != 0 || !valid_mode(bytes[5], bytes[6])) return false;
    size_t length = strnlen((const char*)bytes + 16, NATIVE_CONTENT_TITLE_BYTES);
    if (!length || length == NATIVE_CONTENT_TITLE_BYTES ||
        !zeros(bytes + 16 + length, NATIVE_CONTENT_TITLE_BYTES - length)) return false;
    memcpy(title, bytes + 16, NATIVE_CONTENT_TITLE_BYTES);
    return true;
}

bool nativeContentStore_ReadPreview(const void* prefix, size_t received,
                                    NativeContentPreview* preview)
{
    if (!preview) return false;
    *preview = (NativeContentPreview){0};
    const uint8_t* bytes = prefix;
    if (read_family_title(bytes, received, preview->title)) {
        const size_t portrait_size = read_u32(bytes + 8);
        if (received >= FAMILY_PAYLOAD && portrait_size <= received - FAMILY_PAYLOAD &&
            nativeContent_ValidFighterPortrait(bytes + FAMILY_PAYLOAD, portrait_size)) {
            preview->portrait_png = bytes + FAMILY_PAYLOAD;
            preview->portrait_size = portrait_size;
        }
        return true;
    }
    return false;
}

bool nativeContentStore_BuildFighter(const NativeFighterPackage* fighter,
                                    const NativeFighterPackageData* data,
                                    uint8_t** package, size_t* package_size,
                                    NativeContentId* id)
{
    if (fighter == NULL || data == NULL || package == NULL ||
        package_size == NULL || id == NULL ||
        !valid_mode(fighter->mode, fighter->member_count) ||
        !nativeContent_ValidFighterPortrait(data->portrait_png, data->portrait_size)) return false;
    size_t title_size = strnlen(fighter->title, NATIVE_CONTENT_TITLE_BYTES);
    if (title_size == 0 || title_size == NATIVE_CONTENT_TITLE_BYTES) return false;
    size_t tile_size = strnlen(fighter->tile_name, NATIVE_CONTENT_TITLE_BYTES);
    if (tile_size == NATIVE_CONTENT_TITLE_BYTES) return false;
    for (size_t i = 0; i < tile_size; ++i)
        if ((unsigned char)fighter->tile_name[i] < 32 || fighter->tile_name[i] == 127) return false;
    const uint8_t* pointers[NATIVE_FIGHTER_FAMILY_MEMBERS][MEMBER_CHUNKS];
    size_t sizes[NATIVE_FIGHTER_FAMILY_MEMBERS][MEMBER_CHUNKS];
    size_t key_sizes[NATIVE_FIGHTER_FAMILY_MEMBERS] = {0};
    size_t remaining = NATIVE_CONTENT_MAX_PACKAGE_BYTES - FAMILY_PAYLOAD;
    if (data->portrait_size > remaining) return false;
    remaining -= data->portrait_size;
    if (tile_size > remaining) return false;
    remaining -= tile_size;
    for (unsigned m = 0; m < NATIVE_FIGHTER_FAMILY_MEMBERS; ++m) {
        chunks(&data->members[m], pointers[m], sizes[m]);
        const NativeFighterPackageMember* member = &fighter->members[m];
        if (m >= fighter->member_count) {
            if (!zeros((const uint8_t*)member->key, sizeof(member->key)))
                return false;
            for (unsigned c = 0; c < MEMBER_CHUNKS; ++c)
                if (sizes[m][c] != 0 || pointers[m][c] != NULL) return false;
            continue;
        }
        if (!valid_key(member->key, &key_sizes[m]) ||
            !valid_sizes(sizes[m]) ||
            (m == 1 && strcmp(member->key, fighter->members[0].key) == 0))
            return false;
        for (unsigned c = 0; c < MEMBER_CHUNKS; ++c) {
            if (sizes[m][c] > remaining ||
                (sizes[m][c] != 0 && pointers[m][c] == NULL)) return false;
            remaining -= sizes[m][c];
        }
        NativeSourceAssetFacts facts;NativeScriptDiagnostic diagnostic;
        NativeSourceResources resources;
        if(!nativeSourceResources_Read(pointers[m][7],sizes[m][7],&resources,diagnostic.message,sizeof(diagnostic.message))||
           !nativeSourceAssets_Read(pointers[m][3],sizes[m][3],&facts,&diagnostic)||
           memchr(pointers[m][4],0,sizes[m][4])||(sizes[m][5]&&memchr(pointers[m][5],0,sizes[m][5])))return false;
    }
    size_t total = NATIVE_CONTENT_MAX_PACKAGE_BYTES - remaining;
    uint8_t* built = malloc(total);
    if (built == NULL) return false;
    memset(built, 0, FAMILY_PAYLOAD);
    memcpy(built, "FMSC", 4);
    built[5] = (uint8_t)fighter->mode;
    built[6] = fighter->member_count;
    write_u32(built + 8, (uint32_t)data->portrait_size);
    write_u32(built + 12, (uint32_t)tile_size);
    memcpy(built + 16, fighter->title, title_size);
    uint8_t* output = built + FAMILY_PAYLOAD;
    memcpy(output, data->portrait_png, data->portrait_size);
    output += data->portrait_size;
    memcpy(output, fighter->tile_name, tile_size);
    output += tile_size;
    for (unsigned m = 0; m < fighter->member_count; ++m) {
        uint8_t* directory = built + FAMILY_HEADER + m * MEMBER_DIRECTORY;
        memcpy(directory, fighter->members[m].key, key_sizes[m]);
        for (unsigned c = 0; c < MEMBER_CHUNKS; ++c) {
            write_u32(directory + 32 + c * 4, (uint32_t)sizes[m][c]);
            if (sizes[m][c] != 0) memcpy(output, pointers[m][c], sizes[m][c]);
            output += sizes[m][c];
        }
    }
    if (!nativeContent_Hash(built, total, id)) { free(built); return false; }
    *package = built;
    *package_size = total;
    return true;
}

bool nativeContentStore_ParseFighter(const void* package, size_t package_size,
                                    NativeFighterPackage* fighter,
                                    NativeFighterPackageData* data)
{
    if (fighter == NULL || data == NULL) return false;
    if (package == NULL || package_size < FAMILY_PAYLOAD ||
        package_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES) return false;
    const uint8_t* bytes = package;
    NativeFighterPackage parsed = {0};
    NativeFighterPackageData spans = {0};
    parsed.mode = (NativeFighterFamilyMode)bytes[5];
    parsed.member_count = bytes[6];
    if (!read_family_title(bytes, package_size, parsed.title)) return false;
    size_t remaining = package_size - FAMILY_PAYLOAD;
    spans.portrait_size = read_u32(bytes + 8);
    if (spans.portrait_size > remaining) return false;
    spans.portrait_png = bytes + FAMILY_PAYLOAD;
    if (!nativeContent_ValidFighterPortrait(spans.portrait_png, spans.portrait_size)) return false;
    const uint8_t* cursor = spans.portrait_png + spans.portrait_size;
    remaining -= spans.portrait_size;
    size_t tile_size = read_u32(bytes + 12);
    if (tile_size >= NATIVE_CONTENT_TITLE_BYTES || tile_size > remaining) return false;
    for (size_t i = 0; i < tile_size; ++i)
        if (cursor[i] < 32 || cursor[i] == 127) return false;
    memcpy(parsed.tile_name, cursor, tile_size);
    cursor += tile_size; remaining -= tile_size;
    for (unsigned m = 0; m < NATIVE_FIGHTER_FAMILY_MEMBERS; ++m) {
        const uint8_t* directory = bytes + FAMILY_HEADER + m * MEMBER_DIRECTORY;
        if (m >= parsed.member_count) {
            if (!zeros(directory, MEMBER_DIRECTORY)) return false;
            continue;
        }
        size_t key_size;
        if (!valid_key((const char*)directory, &key_size) ||
            !zeros(directory + key_size, 32 - key_size)) return false;
        memcpy(parsed.members[m].key, directory, key_size);
        if (m == 1 && strcmp(parsed.members[0].key, parsed.members[1].key) == 0)
            return false;
        size_t sizes[MEMBER_CHUNKS];
        const uint8_t* pointers[MEMBER_CHUNKS];
        for (unsigned c = 0; c < MEMBER_CHUNKS; ++c)
            sizes[c] = read_u32(directory + 32 + c * 4);
        if (!valid_sizes(sizes)) return false;
        for (unsigned c = 0; c < MEMBER_CHUNKS; ++c) {
            if (sizes[c] > remaining) return false;
            pointers[c] = sizes[c] != 0 ? cursor : NULL;
            cursor += sizes[c];
            remaining -= sizes[c];
        }
        spans.members[m] = (NativeFighterMemberData){
            .fighter_dat = pointers[0], .fighter_size = sizes[0],
            .animation_dat = pointers[1], .animation_size = sizes[1],
            .costume_dat = pointers[2], .costume_size = sizes[2],
            .asset_record = pointers[3], .asset_record_size = sizes[3],
            .script_text = pointers[4], .script_size = sizes[4],
            .parameter_text = pointers[5], .parameter_size = sizes[5],
            .cpu_text = pointers[6], .cpu_size = sizes[6],
            .resources = pointers[7], .resource_size = sizes[7],
        };
        NativeSourceAssetFacts facts;NativeScriptDiagnostic diagnostic;
        NativeSourceResources resources;
        if(!nativeSourceResources_Read(pointers[7],sizes[7],&resources,diagnostic.message,sizeof(diagnostic.message))||
           !nativeSourceAssets_Read(pointers[3],sizes[3],&facts,&diagnostic)||
           memchr(pointers[4],0,sizes[4])||(sizes[5]&&memchr(pointers[5],0,sizes[5])))return false;
    }
    if (remaining != 0) return false;
    *fighter = parsed;
    *data = spans;
    return true;
}

#endif
