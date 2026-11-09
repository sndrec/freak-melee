#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <melee/gm/native_profile.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

int main(void)
{
    char original_app_data[MAX_PATH];
    DWORD original_app_data_length = GetEnvironmentVariableA(
        "LOCALAPPDATA", original_app_data, sizeof(original_app_data));
    require(original_app_data_length != 0 &&
                original_app_data_length < sizeof(original_app_data),
            "preserve the native profile-store root");
    char temporary_root[MAX_PATH];
    DWORD root_length = GetTempPathA(sizeof(temporary_root), temporary_root);
    require(root_length != 0 && root_length < sizeof(temporary_root),
            "locate the Windows temporary directory");
    char directory[MAX_PATH];
    int directory_length =
        snprintf(directory, sizeof(directory), "%sFreakMeleeProfileTest_%lu",
                 temporary_root, (unsigned long) GetCurrentProcessId());
    require(directory_length > 0 &&
                (size_t) directory_length < sizeof(directory) &&
                (CreateDirectoryA(directory, NULL) ||
                 GetLastError() == ERROR_ALREADY_EXISTS),
            "create an isolated profile test directory");
    char path[MAX_PATH];
    int path_length = snprintf(path, sizeof(path), "%s\\alice.fmp", directory);
    require(path_length > 0 && (size_t) path_length < sizeof(path),
            "build the test profile path");

    NativeProfile saved;
    nativeProfile_Default(&saved, "Alice");
    require(saved.action_buffer_frames == 0, "action buffering defaults to off");
    saved.tap_jump = false;
    saved.action_buffer_frames = 7;
    saved.c_stick = NativeProfileCStick_Tilt;
    saved.calibration.main_stick_dead_zone = 4321;
    saved.calibration.measured = true;
    saved.calibration.center[0] = 123;
    saved.calibration.minimum[0] = -25000;
    saved.calibration.maximum[0] = 26000;
    saved.actions[NativeProfileAction_Jump].count = 2;
    saved.actions[NativeProfileAction_Jump].bindings[0] =
        (NativeProfileBinding){
            .native_input = 2,
            .type = NativeProfileBinding_Button,
        };
    saved.actions[NativeProfileAction_Jump].bindings[1] =
        (NativeProfileBinding){
            .native_input = 4,
            .type = NativeProfileBinding_AxisPositive,
        };
    saved.axes[NativeProfileAxis_MoveX] = (NativeProfileAxisBinding){
        .native_axis = 1,
        .direction = -1,
        .assigned = true,
    };
    require(nativeProfile_Validate(&saved),
            "accept multiple action bindings and profile calibration");
    require(nativeProfile_SaveFile(&saved, path),
            "atomically save the profile");
    char temporary[MAX_PATH];
    snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    require(GetFileAttributesA(temporary) == INVALID_FILE_ATTRIBUTES,
            "remove the temporary file after replacement");

    NativeProfile loaded;
    require(nativeProfile_LoadFile(&loaded, path),
            "load the complete profile");
    require(strcmp(loaded.name, "Alice") == 0 && !loaded.tap_jump &&
                loaded.c_stick == NativeProfileCStick_Tilt &&
                loaded.action_buffer_frames == 7 &&
                loaded.calibration.main_stick_dead_zone == 4321 &&
                loaded.calibration.measured && loaded.calibration.center[0] == 123 &&
                loaded.calibration.minimum[0] == -25000 && loaded.calibration.maximum[0] == 26000 &&
                loaded.actions[NativeProfileAction_Jump].count == 2 &&
                loaded.actions[NativeProfileAction_Jump]
                        .bindings[1]
                        .native_input == 4 &&
                loaded.axes[NativeProfileAxis_MoveX].assigned &&
                loaded.axes[NativeProfileAxis_MoveX].direction == -1,
            "round-trip identity, behavior, bindings, axes, and calibration");

    memcpy(saved.name, "Alice Updated", sizeof("Alice Updated"));
    require(nativeProfile_SaveFile(&saved, path) &&
                nativeProfile_LoadFile(&loaded, path) &&
                strcmp(loaded.name, "Alice Updated") == 0,
            "replace an existing profile atomically");

    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    require(file != INVALID_HANDLE_VALUE &&
                SetFilePointer(file, 36, NULL, FILE_BEGIN) !=
                    INVALID_SET_FILE_POINTER,
            "open the profile for a corruption check");
    uint8_t corrupt = 0xFF;
    DWORD written = 0;
    require(WriteFile(file, &corrupt, 1, &written, NULL) && written == 1 &&
                CloseHandle(file),
            "corrupt one stored binding byte");
    nativeProfile_Default(&loaded, "Unchanged");
    require(!nativeProfile_LoadFile(&loaded, path) &&
                strcmp(loaded.name, "Unchanged") == 0,
            "reject corrupt data without mutating the destination");

    NativeProfile invalid = saved;
    invalid.action_buffer_frames = 8;
    require(!nativeProfile_Validate(&invalid), "limit action buffering to seven frames");
    invalid = saved;
    invalid.calibration.left_trigger_activation = 40000;
    require(!nativeProfile_Validate(&invalid),
            "reject trigger thresholds beyond SDL gamepad travel");
    invalid = saved;
    invalid.actions[NativeProfileAction_Attack].count =
        NATIVE_PROFILE_MAX_BINDINGS + 1;
    require(!nativeProfile_SaveFile(&invalid, path),
            "reject an invalid in-memory profile");
    char store_path[MAX_PATH];
    char maximum_id[NATIVE_PROFILE_ID_BYTES + 1];
    memset(maximum_id, 'p', NATIVE_PROFILE_ID_BYTES);
    maximum_id[NATIVE_PROFILE_ID_BYTES] = '\0';
    char oversized_id[NATIVE_PROFILE_ID_BYTES + 2];
    memset(oversized_id, 'p', NATIVE_PROFILE_ID_BYTES + 1);
    oversized_id[NATIVE_PROFILE_ID_BYTES + 1] = '\0';
    require(SetEnvironmentVariableA("LOCALAPPDATA", directory),
            "redirect the profile store for the test");
    require(nativeProfileStore_IsValidId("alice_local") &&
                nativeProfileStore_IsValidId(maximum_id) &&
                !nativeProfileStore_IsValidId(oversized_id) &&
                !nativeProfileStore_IsValidId("../alice") &&
                nativeProfileStore_BuildPath(store_path, sizeof(store_path),
                                             "alice_local") &&
                strstr(store_path,
                       "\\Freak Melee\\profiles\\alice_local.fmp") != NULL &&
                !nativeProfileStore_BuildPath(store_path, sizeof(store_path),
                                              "../alice"),
            "keep profile IDs inside the LocalAppData profile directory");
    NativeProfileEntry entries[2];
    size_t count;
    require(nativeProfileStore_List(entries, 2, &count) && count == 0,
            "a fresh installation has an empty profile catalog");
    require(nativeProfileStore_Save(&saved, "alice_local") &&
                nativeProfileStore_Load(&loaded, "alice_local") &&
                strcmp(loaded.name, "Alice Updated") == 0,
            "save and load through the installation profile store");
    char broken_path[MAX_PATH];
    require(nativeProfileStore_BuildPath(broken_path, sizeof(broken_path),
                                         "broken") &&
                CopyFileA(path, broken_path, TRUE),
            "place a corrupt profile alongside the valid catalog entry");
    require(
        nativeProfileStore_List(entries, 2, &count) && count == 1 &&
            strcmp(entries[0].id, "alice_local") == 0 &&
            strcmp(entries[0].name, "Alice Updated") == 0,
        "enumerate validated profiles by stable identity and display name");
    require(!nativeProfileStore_List(entries, 0, &count) && count == 0,
            "report catalog capacity exhaustion instead of hiding profiles");
    char other_path[MAX_PATH];
    require(nativeProfileStore_BuildPath(other_path, sizeof(other_path),
                                         "other_local") &&
                nativeProfileStore_Save(&saved, "other_local") &&
                !nativeProfileStore_Delete("../alice") &&
                nativeProfileStore_Delete("alice_local") &&
                !nativeProfileStore_Delete("alice_local") &&
                !nativeProfileStore_Load(&loaded, "alice_local") &&
                nativeProfileStore_Load(&loaded, "other_local") &&
                nativeProfileStore_List(entries, 2, &count) && count == 1 &&
                strcmp(entries[0].id, "other_local") == 0,
            "delete only the selected saved profile and refresh the catalog");
    require(SetEnvironmentVariableA("LOCALAPPDATA", original_app_data),
            "restore the native profile-store root");

    DeleteFileA(path);
    DeleteFileA(store_path);
    DeleteFileA(other_path);
    DeleteFileA(broken_path);
    char store_directory[MAX_PATH];
    char product_directory[MAX_PATH];
    snprintf(store_directory, sizeof(store_directory),
             "%s\\Freak Melee\\profiles", directory);
    snprintf(product_directory, sizeof(product_directory), "%s\\Freak Melee",
             directory);
    RemoveDirectoryA(store_directory);
    RemoveDirectoryA(product_directory);
    RemoveDirectoryA(directory);
    puts("Native profiles persist action controls and calibration atomically");
    return 0;
}
