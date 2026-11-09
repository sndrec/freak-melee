#include "native_profile.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <string.h>
#include <windows.h>

enum {
    PROFILE_BODY_SIZE = 187,
    PROFILE_FILE_SIZE = PROFILE_BODY_SIZE + 4,
};

static const uint8_t profile_magic[4] = { 'F', 'M', 'P', 'F' };

static void write_u16(uint8_t* output, uint16_t value)
{
    output[0] = (uint8_t) value;
    output[1] = (uint8_t) (value >> 8);
}

static void write_u32(uint8_t* output, uint32_t value)
{
    for (uint32_t byte = 0; byte < 4; ++byte) {
        output[byte] = (uint8_t) (value >> (byte * 8));
    }
}

static uint16_t read_u16(const uint8_t* input)
{
    return (uint16_t) input[0] | (uint16_t) input[1] << 8;
}

static uint32_t read_u32(const uint8_t* input)
{
    uint32_t result = 0;
    for (uint32_t byte = 0; byte < 4; ++byte) {
        result |= (uint32_t) input[byte] << (byte * 8);
    }
    return result;
}

static uint32_t profile_hash(const uint8_t* bytes, size_t size)
{
    uint32_t hash = UINT32_C(2166136261);
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= UINT32_C(16777619);
    }
    return hash;
}

void nativeProfile_Default(NativeProfile* profile, const char* name)
{
    if (profile == NULL) {
        return;
    }
    *profile = (NativeProfile){
        .calibration = {
            .main_stick_dead_zone = 8000,
            .c_stick_dead_zone = 8000,
            .left_trigger_activation = 31150,
            .right_trigger_activation = 31150,
            .use_dead_zones = true,
            .emulate_digital_triggers = true,
        },
        .c_stick = NativeProfileCStick_Smash,
        .tap_jump = true,
    };
    if (name != NULL) {
        size_t length = strnlen(name, NATIVE_PROFILE_NAME_BYTES);
        memcpy(profile->name, name, length);
        profile->name[length] = '\0';
    }
    for (uint32_t action = 0; action < NativeProfileAction_Count; ++action) {
        profile->actions[action].bindings[0] = (NativeProfileBinding){
            .native_input = -1,
            .type = NativeProfileBinding_Default,
        };
        profile->actions[action].count =
            action == NativeProfileAction_ShortHop ? 0 : 1;
    }
    for (unsigned axis = 0; axis < NativeProfileAxis_Count; ++axis) {
        profile->calibration.minimum[axis] = axis < 4 ? -32767 : 0;
        profile->calibration.maximum[axis] = 32767;
    }
}

bool nativeProfile_Validate(const NativeProfile* profile)
{
    if (profile == NULL || profile->name[0] == '\0' ||
        strnlen(profile->name, sizeof(profile->name)) >
            NATIVE_PROFILE_NAME_BYTES ||
        profile->c_stick > NativeProfileCStick_Tilt ||
        profile->action_buffer_frames > 7 ||
        profile->calibration.main_stick_dead_zone > 32767 ||
        profile->calibration.c_stick_dead_zone > 32767 ||
        profile->calibration.left_trigger_activation > 32767 ||
        profile->calibration.right_trigger_activation > 32767)
    {
        return false;
    }
    if (profile->calibration.measured) {
        for (unsigned axis = 0; axis < NativeProfileAxis_Count; ++axis) {
            const NativeProfileCalibration* c = &profile->calibration;
            if ((int)c->maximum[axis] - c->center[axis] < 4096 ||
                (axis < 4 && (int)c->center[axis] - c->minimum[axis] < 4096))
                return false;
        }
    }
    for (uint32_t action = 0; action < NativeProfileAction_Count; ++action) {
        const NativeProfileActionBindings* bindings =
            &profile->actions[action];
        if (bindings->count > NATIVE_PROFILE_MAX_BINDINGS) {
            return false;
        }
        for (uint32_t i = 0; i < bindings->count; ++i) {
            const NativeProfileBinding* binding = &bindings->bindings[i];
            if (binding->type > NativeProfileBinding_AxisNegative ||
                (binding->type == NativeProfileBinding_Default
                     ? binding->native_input != -1
                     : binding->native_input < 0))
            {
                return false;
            }
        }
    }
    for (uint32_t axis = 0; axis < NativeProfileAxis_Count; ++axis) {
        const NativeProfileAxisBinding* binding = &profile->axes[axis];
        if (binding->assigned &&
            (binding->native_axis < 0 ||
             (binding->direction != -1 && binding->direction != 1)))
        {
            return false;
        }
    }
    return true;
}

static bool encode_profile(const NativeProfile* profile,
                           uint8_t output[PROFILE_FILE_SIZE])
{
    if (!nativeProfile_Validate(profile)) {
        return false;
    }
    memset(output, 0, PROFILE_FILE_SIZE);
    memcpy(output, profile_magic, sizeof(profile_magic));
    memcpy(output + 4, profile->name, strlen(profile->name));
    // Bit 0 is tap jump. Old profiles have zero in the three buffer bits.
    output[36] = (profile->action_buffer_frames << 1) | profile->tap_jump;
    output[37] = profile->c_stick;
    output[38] = profile->calibration.use_dead_zones ? 1 : 0;
    output[39] = profile->calibration.emulate_digital_triggers ? 1 : 0;
    write_u16(output + 40, profile->calibration.main_stick_dead_zone);
    write_u16(output + 42, profile->calibration.c_stick_dead_zone);
    write_u16(output + 44, profile->calibration.left_trigger_activation);
    write_u16(output + 46, profile->calibration.right_trigger_activation);
    size_t at = 48;
    for (uint32_t action = 0; action < NativeProfileAction_Count; ++action) {
        const NativeProfileActionBindings* bindings =
            &profile->actions[action];
        output[at++] = bindings->count;
        for (uint32_t i = 0; i < NATIVE_PROFILE_MAX_BINDINGS; ++i) {
            output[at++] = bindings->bindings[i].type;
            write_u16(output + at,
                      (uint16_t) bindings->bindings[i].native_input);
            at += 2;
        }
    }
    for (uint32_t axis = 0; axis < NativeProfileAxis_Count; ++axis) {
        const NativeProfileAxisBinding* binding = &profile->axes[axis];
        write_u16(output + at, (uint16_t) binding->native_axis);
        at += 2;
        output[at++] = (uint8_t) binding->direction;
        output[at++] = binding->assigned ? 1 : 0;
    }
    output[at++] = profile->calibration.measured;
    for (unsigned axis = 0; axis < NativeProfileAxis_Count; ++axis) {
        write_u16(output + at, (uint16_t)profile->calibration.center[axis]); at += 2;
        write_u16(output + at, (uint16_t)profile->calibration.minimum[axis]); at += 2;
        write_u16(output + at, (uint16_t)profile->calibration.maximum[axis]); at += 2;
    }
    if (at != PROFILE_BODY_SIZE) {
        return false;
    }
    write_u32(output + PROFILE_BODY_SIZE,
              profile_hash(output, PROFILE_BODY_SIZE));
    return true;
}

static bool decode_profile(NativeProfile* profile,
                           const uint8_t input[PROFILE_FILE_SIZE])
{
    if (memcmp(input, profile_magic, sizeof(profile_magic)) != 0 ||
        read_u32(input + PROFILE_BODY_SIZE) !=
            profile_hash(input, PROFILE_BODY_SIZE) ||
        input[36] > 15 || input[37] > NativeProfileCStick_Tilt ||
        input[38] > 1 || input[39] > 1)
    {
        return false;
    }
    NativeProfile decoded = {
        .calibration = {
            .main_stick_dead_zone = read_u16(input + 40),
            .c_stick_dead_zone = read_u16(input + 42),
            .left_trigger_activation = read_u16(input + 44),
            .right_trigger_activation = read_u16(input + 46),
            .use_dead_zones = input[38] != 0,
            .emulate_digital_triggers = input[39] != 0,
        },
        .c_stick = input[37],
        .tap_jump = (input[36] & 1) != 0,
        .action_buffer_frames = input[36] >> 1,
    };
    memcpy(decoded.name, input + 4, NATIVE_PROFILE_NAME_BYTES);
    decoded.name[NATIVE_PROFILE_NAME_BYTES] = '\0';
    size_t at = 48;
    for (uint32_t action = 0; action < NativeProfileAction_Count; ++action) {
        NativeProfileActionBindings* bindings = &decoded.actions[action];
        bindings->count = input[at++];
        for (uint32_t i = 0; i < NATIVE_PROFILE_MAX_BINDINGS; ++i) {
            bindings->bindings[i].type = input[at++];
            bindings->bindings[i].native_input =
                (int16_t) read_u16(input + at);
            at += 2;
        }
    }
    for (uint32_t axis = 0; axis < NativeProfileAxis_Count; ++axis) {
        NativeProfileAxisBinding* binding = &decoded.axes[axis];
        binding->native_axis = (int16_t) read_u16(input + at);
        at += 2;
        binding->direction = (int8_t) input[at++];
        binding->assigned = input[at++] != 0;
    }
    if (input[at] > 1) return false;
    decoded.calibration.measured = input[at++] != 0;
    for (unsigned axis = 0; axis < NativeProfileAxis_Count; ++axis) {
        decoded.calibration.center[axis] = (int16_t)read_u16(input + at); at += 2;
        decoded.calibration.minimum[axis] = (int16_t)read_u16(input + at); at += 2;
        decoded.calibration.maximum[axis] = (int16_t)read_u16(input + at); at += 2;
    }
    if (at != PROFILE_BODY_SIZE || !nativeProfile_Validate(&decoded)) {
        return false;
    }
    *profile = decoded;
    return true;
}

bool nativeProfile_SaveFile(const NativeProfile* profile, const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return false;
    }
    uint8_t bytes[PROFILE_FILE_SIZE];
    if (!encode_profile(profile, bytes)) {
        return false;
    }
    size_t path_length = strlen(path);
    if (path_length > MAX_PATH - 5) {
        return false;
    }
    char temporary[MAX_PATH];
    memcpy(temporary, path, path_length);
    memcpy(temporary + path_length, ".tmp", 5);
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

bool nativeProfile_LoadFile(NativeProfile* profile, const char* path)
{
    if (profile == NULL || path == NULL) {
        return false;
    }
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    uint8_t bytes[PROFILE_FILE_SIZE];
    DWORD read = 0;
    bool ok = ReadFile(file, bytes, sizeof(bytes), &read, NULL) &&
              read == sizeof(bytes);
    uint8_t extra;
    DWORD extra_read = 0;
    ok = ok && ReadFile(file, &extra, 1, &extra_read, NULL) && extra_read == 0;
    CloseHandle(file);
    return ok && decode_profile(profile, bytes);
}

bool nativeProfileStore_IsValidId(const char* profile_id)
{
    if (profile_id == NULL || profile_id[0] == '\0') {
        return false;
    }
    size_t length = 0;
    for (const char* at = profile_id; *at != '\0'; ++at) {
        bool valid = (*at >= 'a' && *at <= 'z') ||
                     (*at >= 'A' && *at <= 'Z') ||
                     (*at >= '0' && *at <= '9') || *at == '-' || *at == '_';
        if (!valid || ++length > NATIVE_PROFILE_ID_BYTES) {
            return false;
        }
    }
    return true;
}

bool nativeProfileStore_BuildPath(char* path, size_t capacity,
                                  const char* profile_id)
{
    if (path == NULL || capacity == 0 ||
        !nativeProfileStore_IsValidId(profile_id))
    {
        return false;
    }
    char app_data[MAX_PATH];
    DWORD length =
        GetEnvironmentVariableA("LOCALAPPDATA", app_data, sizeof(app_data));
    if (length == 0 || length >= sizeof(app_data)) {
        return false;
    }
    int written = snprintf(path, capacity, "%s\\Freak Melee\\profiles\\%s.fmp",
                           app_data, profile_id);
    return written > 0 && (size_t) written < capacity;
}

static bool ensure_store_directories(void)
{
    char app_data[MAX_PATH];
    DWORD length =
        GetEnvironmentVariableA("LOCALAPPDATA", app_data, sizeof(app_data));
    if (length == 0 || length >= sizeof(app_data)) {
        return false;
    }
    char product[MAX_PATH];
    char profiles[MAX_PATH];
    int product_length =
        snprintf(product, sizeof(product), "%s\\Freak Melee", app_data);
    int profiles_length =
        snprintf(profiles, sizeof(profiles), "%s\\profiles", product);
    if (product_length <= 0 || product_length >= sizeof(product) ||
        profiles_length <= 0 || profiles_length >= sizeof(profiles))
    {
        return false;
    }
    return (CreateDirectoryA(product, NULL) ||
            GetLastError() == ERROR_ALREADY_EXISTS) &&
           (CreateDirectoryA(profiles, NULL) ||
            GetLastError() == ERROR_ALREADY_EXISTS);
}

bool nativeProfileStore_Save(const NativeProfile* profile,
                             const char* profile_id)
{
    char path[MAX_PATH];
    return ensure_store_directories() &&
           nativeProfileStore_BuildPath(path, sizeof(path), profile_id) &&
           nativeProfile_SaveFile(profile, path);
}

bool nativeProfileStore_Load(NativeProfile* profile, const char* profile_id)
{
    char path[MAX_PATH];
    return nativeProfileStore_BuildPath(path, sizeof(path), profile_id) &&
           nativeProfile_LoadFile(profile, path);
}

bool nativeProfileStore_Delete(const char* profile_id)
{
    char path[MAX_PATH];
    return nativeProfileStore_BuildPath(path, sizeof(path), profile_id) &&
           DeleteFileA(path);
}

bool nativeProfileStore_List(NativeProfileEntry* entries, size_t capacity,
                             size_t* count)
{
    if (!entries || !count) {
        return false;
    }
    *count = 0;
    char pattern[MAX_PATH];
    if (!nativeProfileStore_BuildPath(pattern, sizeof(pattern), "list")) {
        return false;
    }
    strcpy(strrchr(pattern, '\\') + 1, "*.fmp");
    WIN32_FIND_DATAA data;
    HANDLE search = FindFirstFileA(pattern, &data);
    if (search == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    bool ok = true;
    do {
        size_t length = strlen(data.cFileName);
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            length <= 4 || length - 4 > NATIVE_PROFILE_ID_BYTES)
        {
            continue;
        }
        NativeProfileEntry entry = { 0 };
        memcpy(entry.id, data.cFileName, length - 4);
        NativeProfile profile;
        if (!nativeProfileStore_Load(&profile, entry.id)) {
            continue;
        }
        if (*count == capacity) {
            ok = false;
            break;
        }
        memcpy(entry.name, profile.name, sizeof(entry.name));
        entries[(*count)++] = entry;
    } while (FindNextFileA(search, &data));
    if (ok && GetLastError() != ERROR_NO_MORE_FILES) {
        ok = false;
    }
    FindClose(search);
    return ok;
}

#endif
