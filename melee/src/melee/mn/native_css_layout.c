#include "native_css_layout.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/forward.h>
#include <melee/gm/native_stock_content.h>

#include <stdio.h>
#include <string.h>

enum {
    LAYOUT_HEADER_BYTES = 6,
    LAYOUT_TILE_BYTES = 48,
    LAYOUT_HASH_BYTES = 4,
    LAYOUT_MAX_BYTES = LAYOUT_HEADER_BYTES +
                       NATIVE_CSS_LAYOUT_MAX_TILES * LAYOUT_TILE_BYTES +
                       LAYOUT_HASH_BYTES,
};

static const uint8_t layout_magic[4] = { 'F', 'M', 'C', 'L' };
static LONG temporary_number;

static uint16_t read_u16(const uint8_t* data)
{
    return (uint16_t) data[0] | ((uint16_t) data[1] << 8);
}

static uint32_t read_u32(const uint8_t* data)
{
    return (uint32_t) data[0] | ((uint32_t) data[1] << 8) |
           ((uint32_t) data[2] << 16) | ((uint32_t) data[3] << 24);
}

static uint64_t read_u64(const uint8_t* data)
{
    uint64_t value = 0;
    for (unsigned i = 0; i < 8; ++i) {
        value |= (uint64_t) data[i] << (i * 8);
    }
    return value;
}

static void write_u16(uint8_t* data, uint16_t value)
{
    data[0] = (uint8_t) value;
    data[1] = (uint8_t) (value >> 8);
}

static void write_u32(uint8_t* data, uint32_t value)
{
    for (unsigned i = 0; i < 4; ++i) {
        data[i] = (uint8_t) (value >> (i * 8));
    }
}

static void write_u64(uint8_t* data, uint64_t value)
{
    for (unsigned i = 0; i < 8; ++i) {
        data[i] = (uint8_t) (value >> (i * 8));
    }
}

static uint32_t layout_hash(const uint8_t* data, size_t size)
{
    uint32_t hash = UINT32_C(2166136261);
    for (size_t i = 0; i < size; ++i) {
        hash = (hash ^ data[i]) * UINT32_C(16777619);
    }
    return hash;
}

static bool empty_package_id(const NativeContentId* id)
{
    static const NativeContentId empty = { 0 };
    return memcmp(id, &empty, sizeof(empty)) == 0;
}

static bool valid_tile(const NativeCssTile* tile)
{
    if (tile->width < NATIVE_CSS_LAYOUT_MIN_TILE_SIZE ||
        tile->height < NATIVE_CSS_LAYOUT_MIN_TILE_SIZE ||
        tile->x + tile->width > NATIVE_CSS_LAYOUT_EXTENT ||
        tile->y + tile->height > NATIVE_CSS_LAYOUT_EXTENT) {
        return false;
    }
    if (empty_package_id(&tile->fighter_package_id)) {
        uint8_t character;
        return nativeStockContent_CharacterFromId(tile->stock_character_id,
                                                   &character);
    }
    return tile->stock_character_id == 0;
}

static bool same_fighter(const NativeCssTile* left,
                         const NativeCssTile* right)
{
    return left->stock_character_id == right->stock_character_id &&
           memcmp(&left->fighter_package_id, &right->fighter_package_id,
                  sizeof(left->fighter_package_id)) == 0;
}

void nativeCssLayout_Default(NativeCssLayout* layout)
{
    static const uint8_t characters[] = {
        CKind_DrMario, CKind_Mario, CKind_Luigi, CKind_Koopa,
        CKind_Peach, CKind_Yoshi, CKind_Donkey, CKind_Captain,
        CKind_Ganon, CKind_Falco, CKind_Fox, CKind_Ness,
        CKind_PopoNana, CKind_Kirby, CKind_Samus, CKind_Zelda,
        CKind_Link, CKind_CLink, CKind_Pichu, CKind_Pikachu,
        CKind_Purin, CKind_Mewtwo, CKind_GameWatch, CKind_Mars,
        CKind_Emblem,
    };
    if (layout == NULL) {
        return;
    }
    memset(layout, 0, sizeof(*layout));
    layout->count = sizeof(characters);
    for (unsigned i = 0; i < layout->count; ++i) {
        const unsigned row = i < 9 ? 0 : i < 18 ? 1 : 2;
        const unsigned column = i < 9 ? i : i < 18 ? i - 9 : i - 18;
        NativeCssTile* tile = &layout->tiles[i];
        tile->stock_character_id =
            nativeStockContent_CharacterId(characters[i]);
        tile->x = (uint16_t) ((row == 2 ? 146 : 45) + column * 101);
        tile->y = (uint16_t) (205 + row * 145);
        tile->width = 92;
        tile->height = 120;
    }
}

bool nativeCssLayout_Validate(const NativeCssLayout* layout)
{
    if (layout == NULL || layout->count > NATIVE_CSS_LAYOUT_MAX_TILES) {
        return false;
    }
    for (unsigned i = 0; i < layout->count; ++i) {
        if (!valid_tile(&layout->tiles[i])) {
            return false;
        }
        for (unsigned previous = 0; previous < i; ++previous) {
            if (same_fighter(&layout->tiles[i],
                             &layout->tiles[previous])) {
                return false;
            }
        }
    }
    return true;
}

bool nativeCssLayout_Add(NativeCssLayout* layout, const NativeCssTile* tile)
{
    if (layout == NULL || tile == NULL ||
        layout->count >= NATIVE_CSS_LAYOUT_MAX_TILES ||
        !nativeCssLayout_Validate(layout) || !valid_tile(tile)) {
        return false;
    }
    for (unsigned i = 0; i < layout->count; ++i) {
        if (same_fighter(&layout->tiles[i], tile)) {
            return false;
        }
    }
    layout->tiles[layout->count++] = *tile;
    return true;
}

bool nativeCssLayout_Update(NativeCssLayout* layout, unsigned index,
                            const NativeCssTile* tile)
{
    if (layout == NULL || tile == NULL || index >= layout->count ||
        !nativeCssLayout_Validate(layout) || !valid_tile(tile) ||
        !same_fighter(&layout->tiles[index], tile)) {
        return false;
    }
    layout->tiles[index] = *tile;
    return true;
}

bool nativeCssLayout_Remove(NativeCssLayout* layout, unsigned index)
{
    if (layout == NULL || index >= layout->count ||
        !nativeCssLayout_Validate(layout)) {
        return false;
    }
    memmove(&layout->tiles[index], &layout->tiles[index + 1],
            (layout->count - index - 1) * sizeof(layout->tiles[0]));
    --layout->count;
    return true;
}

bool nativeCssLayout_ReplaceFighter(NativeCssLayout* layout,
                                    const NativeContentId* previous,
                                    const NativeContentId* next)
{
    if (layout == NULL || previous == NULL || next == NULL ||
        empty_package_id(previous) || empty_package_id(next) ||
        !nativeCssLayout_Validate(layout)) return false;
    if (memcmp(previous, next, sizeof(*next)) == 0) return true;
    int prior = -1;
    for (unsigned i = 0; i < layout->count; ++i) {
        if (layout->tiles[i].stock_character_id != 0) continue;
        if (memcmp(&layout->tiles[i].fighter_package_id, next,
                   sizeof(*next)) == 0) return false;
        if (memcmp(&layout->tiles[i].fighter_package_id, previous,
                   sizeof(*previous)) == 0) prior = (int) i;
    }
    if (prior >= 0) layout->tiles[prior].fighter_package_id = *next;
    return true;
}

static size_t encode_layout(const NativeCssLayout* layout, uint8_t* bytes)
{
    if (!nativeCssLayout_Validate(layout)) {
        return 0;
    }
    memcpy(bytes, layout_magic, sizeof(layout_magic));
    write_u16(bytes + 4, layout->count);
    for (unsigned i = 0; i < layout->count; ++i) {
        const NativeCssTile* tile = &layout->tiles[i];
        uint8_t* entry = bytes + LAYOUT_HEADER_BYTES + i * LAYOUT_TILE_BYTES;
        write_u64(entry, tile->stock_character_id);
        memcpy(entry + 8, tile->fighter_package_id.bytes,
               NATIVE_CONTENT_ID_BYTES);
        write_u16(entry + 40, tile->x);
        write_u16(entry + 42, tile->y);
        write_u16(entry + 44, tile->width);
        write_u16(entry + 46, tile->height);
    }
    size_t body = LAYOUT_HEADER_BYTES + layout->count * LAYOUT_TILE_BYTES;
    write_u32(bytes + body, layout_hash(bytes, body));
    return body + LAYOUT_HASH_BYTES;
}

static bool decode_layout(NativeCssLayout* layout, const uint8_t* bytes,
                          size_t size)
{
    if (size < LAYOUT_HEADER_BYTES + LAYOUT_HASH_BYTES ||
        memcmp(bytes, layout_magic, sizeof(layout_magic)) != 0) {
        return false;
    }
    unsigned count = read_u16(bytes + 4);
    size_t body = LAYOUT_HEADER_BYTES + count * LAYOUT_TILE_BYTES;
    if (count > NATIVE_CSS_LAYOUT_MAX_TILES ||
        size != body + LAYOUT_HASH_BYTES ||
        read_u32(bytes + body) != layout_hash(bytes, body)) {
        return false;
    }
    NativeCssLayout decoded = { 0 };
    decoded.count = (uint16_t) count;
    for (unsigned i = 0; i < count; ++i) {
        NativeCssTile* tile = &decoded.tiles[i];
        const uint8_t* entry =
            bytes + LAYOUT_HEADER_BYTES + i * LAYOUT_TILE_BYTES;
        tile->stock_character_id = read_u64(entry);
        memcpy(tile->fighter_package_id.bytes, entry + 8,
               NATIVE_CONTENT_ID_BYTES);
        tile->x = read_u16(entry + 40);
        tile->y = read_u16(entry + 42);
        tile->width = read_u16(entry + 44);
        tile->height = read_u16(entry + 46);
    }
    if (!nativeCssLayout_Validate(&decoded)) {
        return false;
    }
    *layout = decoded;
    return true;
}

bool nativeCssLayout_LoadFile(NativeCssLayout* layout, const char* path,
                              bool* missing)
{
    if (layout == NULL || path == NULL || missing == NULL) {
        return false;
    }
    *missing = false;
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        *missing = GetLastError() == ERROR_FILE_NOT_FOUND;
        return false;
    }
    LARGE_INTEGER length;
    uint8_t bytes[LAYOUT_MAX_BYTES];
    DWORD received = 0;
    bool ok = GetFileSizeEx(file, &length) && length.QuadPart > 0 &&
              length.QuadPart <= sizeof(bytes) &&
              ReadFile(file, bytes, (DWORD) length.QuadPart, &received, NULL) &&
              received == length.QuadPart;
    ok = CloseHandle(file) && ok;
    return ok && decode_layout(layout, bytes, received);
}

bool nativeCssLayout_SaveFile(const NativeCssLayout* layout, const char* path)
{
    if (path == NULL) {
        return false;
    }
    uint8_t bytes[LAYOUT_MAX_BYTES];
    size_t size = encode_layout(layout, bytes);
    if (size == 0) {
        return false;
    }
    char temporary[MAX_PATH];
    LONG number = InterlockedIncrement(&temporary_number);
    int length = snprintf(temporary, sizeof(temporary), "%s.%lu.%ld.tmp",
                          path, GetCurrentProcessId(), number);
    if (length <= 0 || length >= sizeof(temporary)) {
        return false;
    }
    HANDLE file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written = 0;
    bool ok = WriteFile(file, bytes, (DWORD) size, &written, NULL) &&
              written == size && FlushFileBuffers(file);
    ok = CloseHandle(file) && ok;
    if (ok) {
        ok = MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING |
                                            MOVEFILE_WRITE_THROUGH);
    }
    if (!ok) {
        DeleteFileA(temporary);
    }
    return ok;
}

bool nativeCssLayout_DefaultPath(char* path, unsigned capacity)
{
    if (path == NULL || capacity == 0) {
        return false;
    }
    char app_data[MAX_PATH];
    DWORD length = GetEnvironmentVariableA("LOCALAPPDATA", app_data,
                                           sizeof(app_data));
    if (length == 0 || length >= sizeof(app_data)) {
        return false;
    }
    char directory[MAX_PATH];
    int written = snprintf(directory, sizeof(directory),
                           "%s\\Freak Melee", app_data);
    if (written <= 0 || written >= sizeof(directory)) {
        return false;
    }
    if (!CreateDirectoryA(directory, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }
    written = snprintf(path, capacity, "%s\\css-layout.bin", directory);
    return written > 0 && (unsigned) written < capacity;
}

#endif
