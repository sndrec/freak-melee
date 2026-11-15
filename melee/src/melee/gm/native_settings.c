#include "native_settings.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/gm/types.h>
#include <melee/mn/forward.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum {
    SETTINGS_BODY_BYTES = 28,
    SETTINGS_FILE_BYTES = SETTINGS_BODY_BYTES + 4,
};

static uint8_t local_settings_before_room[SETTINGS_FILE_BYTES];
static bool restore_local_settings_after_room;

static uint32_t read_u32(const uint8_t* bytes)
{
    return (uint32_t) bytes[0] | ((uint32_t) bytes[1] << 8) |
           ((uint32_t) bytes[2] << 16) | ((uint32_t) bytes[3] << 24);
}

static uint64_t read_u64(const uint8_t* bytes)
{
    return (uint64_t) read_u32(bytes) |
           ((uint64_t) read_u32(bytes + 4) << 32);
}

static void write_u32(uint8_t* bytes, uint32_t value)
{
    for (int i = 0; i < 4; ++i) {
        bytes[i] = (uint8_t) (value >> (i * 8));
    }
}

static void write_u64(uint8_t* bytes, uint64_t value)
{
    write_u32(bytes, (uint32_t) value);
    write_u32(bytes + 4, (uint32_t) (value >> 32));
}

static uint32_t checksum(const uint8_t* bytes)
{
    uint32_t value = UINT32_C(2166136261);
    for (int i = 0; i < SETTINGS_BODY_BYTES; ++i) {
        value = (value ^ bytes[i]) * UINT32_C(16777619);
    }
    return value;
}

static bool valid(const GameRules* rules, const struct GamePrefs* prefs)
{
    return rules != NULL && prefs != NULL && rules->mode <= 3 &&
           rules->time_limit <= 99 && rules->stock_count >= 1 &&
           rules->stock_count <= 99 && rules->handicap <= 2 &&
           rules->damage_ratio >= 1 && rules->damage_ratio <= 99 &&
           rules->stage_sel <= StageSelectMode_Loser &&
           rules->stock_time_limit <= 99 && rules->friendly_fire <= 1 &&
           rules->pause <= 1 && rules->score_display <= 1 &&
           rules->unk_xc <= 2 &&
           (prefs->item_freq == UINT8_MAX || prefs->item_freq <= 4);
}

static bool encode(const GameRules* rules, const struct GamePrefs* prefs,
                   uint8_t bytes[SETTINGS_FILE_BYTES])
{
    if (!valid(rules, prefs)) {
        return false;
    }
    memcpy(bytes, "FMVS", 4);
    bytes[4] = rules->mode;
    bytes[5] = rules->time_limit;
    bytes[6] = rules->stock_count;
    bytes[7] = rules->handicap;
    bytes[8] = rules->damage_ratio;
    bytes[9] = rules->stage_sel;
    bytes[10] = rules->stock_time_limit;
    bytes[11] = rules->friendly_fire;
    bytes[12] = rules->pause;
    bytes[13] = rules->score_display;
    bytes[14] = rules->unk_xc;
    bytes[15] = prefs->item_freq;
    write_u64(bytes + 16, prefs->item_mask);
    write_u32(bytes + 24, prefs->stage_mask);
    write_u32(bytes + SETTINGS_BODY_BYTES, checksum(bytes));
    return true;
}

static bool decode(GameRules* rules, struct GamePrefs* prefs,
                   const uint8_t bytes[SETTINGS_FILE_BYTES])
{
    if (memcmp(bytes, "FMVS", 4) != 0 ||
        read_u32(bytes + SETTINGS_BODY_BYTES) != checksum(bytes))
    {
        return false;
    }
    GameRules next_rules = *rules;
    struct GamePrefs next_prefs = *prefs;
    next_rules.mode = bytes[4];
    next_rules.time_limit = bytes[5];
    next_rules.stock_count = bytes[6];
    next_rules.handicap = bytes[7];
    next_rules.damage_ratio = bytes[8];
    next_rules.stage_sel = bytes[9];
    next_rules.stock_time_limit = bytes[10];
    next_rules.friendly_fire = bytes[11];
    next_rules.pause = bytes[12];
    next_rules.score_display = bytes[13];
    next_rules.unk_xc = bytes[14];
    next_prefs.item_freq = bytes[15];
    next_prefs.item_mask = read_u64(bytes + 16);
    next_prefs.stage_mask = read_u32(bytes + 24);
    if (!valid(&next_rules, &next_prefs)) {
        return false;
    }
    *rules = next_rules;
    *prefs = next_prefs;
    return true;
}

bool nativeSettings_PreserveForRoomMatch(const GameRules* rules,
                                         const struct GamePrefs* prefs)
{
    if (restore_local_settings_after_room) {
        return true;
    }
    restore_local_settings_after_room =
        encode(rules, prefs, local_settings_before_room);
    return restore_local_settings_after_room;
}

bool nativeSettings_RestoreAfterRoomMatch(GameRules* rules,
                                          struct GamePrefs* prefs)
{
    if (!restore_local_settings_after_room) {
        return true;
    }
    if (!decode(rules, prefs, local_settings_before_room)) {
        return false;
    }
    restore_local_settings_after_room = false;
    return true;
}

bool nativeSettings_SaveFile(const GameRules* rules, const struct GamePrefs* prefs,
                             const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return false;
    }
    uint8_t bytes[SETTINGS_FILE_BYTES];
    if (!encode(rules, prefs, bytes)) {
        return false;
    }
    char temporary[MAX_PATH];
    int length = snprintf(temporary, sizeof(temporary), "%s.%lu.tmp", path,
                          GetCurrentProcessId());
    if (length <= 0 || length >= sizeof(temporary)) {
        return false;
    }
    HANDLE file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written = 0;
    bool ok = WriteFile(file, bytes, sizeof(bytes), &written, NULL) &&
              written == sizeof(bytes) && FlushFileBuffers(file);
    ok = CloseHandle(file) && ok;
    if (ok) {
        ok = MoveFileExA(temporary, path,
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    if (!ok) {
        DeleteFileA(temporary);
    }
    return ok;
}

bool nativeSettings_LoadFile(GameRules* rules, struct GamePrefs* prefs,
                             const char* path)
{
    if (rules == NULL || prefs == NULL || path == NULL) {
        return false;
    }
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    uint8_t bytes[SETTINGS_FILE_BYTES];
    DWORD read = 0;
    bool ok = ReadFile(file, bytes, sizeof(bytes), &read, NULL) &&
              read == sizeof(bytes);
    uint8_t extra;
    DWORD extra_read = 0;
    ok = ok && ReadFile(file, &extra, 1, &extra_read, NULL) && extra_read == 0;
    CloseHandle(file);
    return ok && decode(rules, prefs, bytes);
}

static bool store_path(char path[MAX_PATH], bool create_directory)
{
    char app_data[MAX_PATH];
    DWORD length =
        GetEnvironmentVariableA("LOCALAPPDATA", app_data, sizeof(app_data));
    if (length == 0 || length >= sizeof(app_data)) {
        return false;
    }
    char directory[MAX_PATH];
    int written = snprintf(directory, sizeof(directory), "%s\\Freak Melee",
                           app_data);
    if (written <= 0 || written >= sizeof(directory)) {
        return false;
    }
    if (create_directory && !CreateDirectoryA(directory, NULL) &&
        GetLastError() != ERROR_ALREADY_EXISTS)
    {
        return false;
    }
    written = snprintf(path, MAX_PATH, "%s\\versus-settings.fms", directory);
    return written > 0 && written < MAX_PATH;
}

bool nativeSettings_Load(GameRules* rules, struct GamePrefs* prefs)
{
    char path[MAX_PATH];
    return store_path(path, false) && nativeSettings_LoadFile(rules, prefs, path);
}

bool nativeSettings_Save(const GameRules* rules, const struct GamePrefs* prefs)
{
    char path[MAX_PATH];
    return store_path(path, true) && nativeSettings_SaveFile(rules, prefs, path);
}

#endif
