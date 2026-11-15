#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/gm/native_settings.h>
#include <melee/gm/types.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static void overwrite_byte(const char* path, DWORD offset, uint8_t value)
{
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    require(file != INVALID_HANDLE_VALUE, "open saved rules for damage test");
    require(SetFilePointer(file, offset, NULL, FILE_BEGIN) == offset,
            "seek within saved rules");
    DWORD written = 0;
    require(WriteFile(file, &value, 1, &written, NULL) && written == 1,
            "damage saved rules");
    require(CloseHandle(file), "close damaged rules");
}

int main(void)
{
    char temp_root[MAX_PATH];
    DWORD root_length = GetTempPathA(sizeof(temp_root), temp_root);
    require(root_length != 0 && root_length < sizeof(temp_root),
            "locate temporary directory");
    char directory[MAX_PATH];
    int length = snprintf(directory, sizeof(directory),
                          "%sFreakMeleeSettingsTest_%lu", temp_root,
                          (unsigned long) GetCurrentProcessId());
    require(length > 0 && length < sizeof(directory) &&
                CreateDirectoryA(directory, NULL),
            "create isolated settings directory");
    char path[MAX_PATH];
    length = snprintf(path, sizeof(path), "%s\\versus.fms", directory);
    require(length > 0 && length < sizeof(path), "build settings path");

    GameRules saved_rules = { 0 };
    saved_rules.mode = 1;
    saved_rules.time_limit = 8;
    saved_rules.stock_count = 5;
    saved_rules.handicap = 2;
    saved_rules.damage_ratio = 12;
    saved_rules.stage_sel = 2;
    saved_rules.stock_time_limit = 30;
    saved_rules.friendly_fire = 1;
    saved_rules.pause = 1;
    saved_rules.score_display = 1;
    saved_rules.unk_xc = 2;
    struct GamePrefs saved_prefs = { 0 };
    saved_prefs.item_freq = UINT8_MAX;
    saved_prefs.item_mask = UINT64_C(0x123456789ABCDEF0);
    saved_prefs.stage_mask = UINT32_C(0xA55A27C3);
    require(nativeSettings_SaveFile(&saved_rules, &saved_prefs, path),
            "save versus rules with items off");

    GameRules loaded_rules = { 0 };
    loaded_rules.bgm = 0x34;
    loaded_rules.xD = 7;
    struct GamePrefs loaded_prefs = { 0 };
    loaded_prefs.rumble_enabled[0] = 1;
    loaded_prefs.sound_balance = -3;
    require(nativeSettings_LoadFile(&loaded_rules, &loaded_prefs, path),
            "load saved versus rules");
    require(loaded_rules.mode == 1 && loaded_rules.time_limit == 8 &&
                loaded_rules.stock_count == 5 && loaded_rules.handicap == 2 &&
                loaded_rules.damage_ratio == 12 && loaded_rules.stage_sel == 2 &&
                loaded_rules.stock_time_limit == 30 &&
                loaded_rules.friendly_fire == 1 && loaded_rules.pause == 1 &&
                loaded_rules.score_display == 1 && loaded_rules.unk_xc == 2 &&
                loaded_prefs.item_freq == UINT8_MAX &&
                loaded_prefs.item_mask == UINT64_C(0x123456789ABCDEF0) &&
                loaded_prefs.stage_mask == UINT32_C(0xA55A27C3),
            "restore every persisted versus setting");
    require(loaded_rules.bgm == 0x34 && loaded_rules.xD == 7 &&
                loaded_prefs.rumble_enabled[0] == 1 &&
                loaded_prefs.sound_balance == -3,
            "preserve runtime fields outside versus settings");

    require(nativeSettings_PreserveForRoomMatch(&loaded_rules, &loaded_prefs),
            "capture local rules before room ownership takes over");
    loaded_rules.mode = 3;
    loaded_rules.time_limit = 1;
    loaded_rules.bgm = 0x55;
    loaded_prefs.item_freq = 4;
    loaded_prefs.rumble_enabled[0] = 0;
    require(nativeSettings_PreserveForRoomMatch(&loaded_rules, &loaded_prefs),
            "keep the first local snapshot across another room setup");
    require(nativeSettings_RestoreAfterRoomMatch(&loaded_rules, &loaded_prefs) &&
                loaded_rules.mode == 1 && loaded_rules.time_limit == 8 &&
                loaded_prefs.item_freq == UINT8_MAX &&
                loaded_rules.bgm == 0x55 &&
                loaded_prefs.rumble_enabled[0] == 0,
            "restore only local versus choices after a guest match");
    loaded_rules.time_limit = 9;
    require(nativeSettings_RestoreAfterRoomMatch(&loaded_rules, &loaded_prefs) &&
                loaded_rules.time_limit == 9,
            "restore a room snapshot only once");

    saved_rules.stock_count = 2;
    saved_prefs.item_freq = 4;
    require(nativeSettings_SaveFile(&saved_rules, &saved_prefs, path) &&
                nativeSettings_LoadFile(&loaded_rules, &loaded_prefs, path) &&
                loaded_rules.stock_count == 2 && loaded_prefs.item_freq == 4,
            "atomically replace an existing settings file");
    GameRules before_rules = loaded_rules;
    struct GamePrefs before_prefs = loaded_prefs;
    overwrite_byte(path, 9, 0xFF);
    require(!nativeSettings_LoadFile(&loaded_rules, &loaded_prefs, path) &&
                memcmp(&before_rules, &loaded_rules, sizeof(before_rules)) == 0 &&
                memcmp(&before_prefs, &loaded_prefs, sizeof(before_prefs)) == 0,
            "reject corruption without changing live rules");

    require(nativeSettings_SaveFile(&saved_rules, &saved_prefs, path),
            "restore complete settings file");
    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    require(file != INVALID_HANDLE_VALUE &&
                SetFilePointer(file, 4, NULL, FILE_BEGIN) == 4 &&
                SetEndOfFile(file) && CloseHandle(file),
            "truncate saved settings");
    require(!nativeSettings_LoadFile(&loaded_rules, &loaded_prefs, path),
            "reject truncated settings");

    require(nativeSettings_SaveFile(&saved_rules, &saved_prefs, path),
            "restore complete settings again");
    file = CreateFileA(path, FILE_APPEND_DATA, 0, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL);
    uint8_t extra = 0;
    DWORD written = 0;
    require(file != INVALID_HANDLE_VALUE &&
                WriteFile(file, &extra, 1, &written, NULL) && written == 1 &&
                CloseHandle(file),
            "append trailing data");
    require(!nativeSettings_LoadFile(&loaded_rules, &loaded_prefs, path),
            "reject trailing data");

    char original_app_data[MAX_PATH];
    DWORD original_length = GetEnvironmentVariableA(
        "LOCALAPPDATA", original_app_data, sizeof(original_app_data));
    require(original_length != 0 && original_length < sizeof(original_app_data),
            "preserve the normal settings root");
    require(SetEnvironmentVariableA("LOCALAPPDATA", directory),
            "isolate the installation settings root");
    require(nativeSettings_Save(&saved_rules, &saved_prefs),
            "save through the installation path");
    loaded_rules.stock_count = 1;
    loaded_prefs.item_freq = UINT8_MAX;
    require(nativeSettings_Load(&loaded_rules, &loaded_prefs) &&
                loaded_rules.stock_count == 2 && loaded_prefs.item_freq == 4,
            "load from the installation path");
    require(SetEnvironmentVariableA("LOCALAPPDATA", original_app_data),
            "restore the normal settings root");
    char installation_path[MAX_PATH];
    length = snprintf(installation_path, sizeof(installation_path),
                      "%s\\Freak Melee\\versus-settings.fms", directory);
    require(length > 0 && length < sizeof(installation_path) &&
                DeleteFileA(installation_path),
            "remove isolated installation settings");
    char installation_directory[MAX_PATH];
    length = snprintf(installation_directory, sizeof(installation_directory),
                      "%s\\Freak Melee", directory);
    require(length > 0 && length < sizeof(installation_directory) &&
                RemoveDirectoryA(installation_directory),
            "remove isolated installation directory");

    require(DeleteFileA(path) && RemoveDirectoryA(directory),
            "clean isolated settings files");
    puts("native settings passed");
    return 0;
}
