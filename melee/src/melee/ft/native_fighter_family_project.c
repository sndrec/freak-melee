#include "native_fighter_family_project.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_cpu.h>
#include <melee/ft/native_source_package.h>

bool nativeFighterFamilyProject_LocalRoot(char* path, size_t capacity)
{
    char app_data[512], parent[512];
    DWORD length = GetEnvironmentVariableA("LOCALAPPDATA", app_data, sizeof(app_data));
    if (path == NULL || capacity == 0 || length == 0 || length >= sizeof(app_data))
        return false;
    int n = snprintf(parent, sizeof(parent), "%s\\Freak Melee", app_data);
    if (n <= 0 || n >= sizeof(parent)) return false;
    n = snprintf(path, capacity, "%s\\projects", parent);
    if (n <= 0 || (size_t)n >= capacity) return false;
    if (!CreateDirectoryA(parent, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return false;
    return CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
}

static int compare_drafts(const void* a, const void* b)
{
    const NativeFighterDraft* left = a;
    const NativeFighterDraft* right = b;
    int order = _stricmp(left->title, right->title);
    return order != 0 ? order : strcmp(left->path, right->path);
}

bool nativeFighterFamilyProject_ListLocal(NativeFighterDraft* drafts,
                                         size_t capacity, size_t* count)
{
    char root[512], pattern[520];
    if (drafts == NULL || count == NULL || capacity == 0 ||
        !nativeFighterFamilyProject_LocalRoot(root, sizeof(root))) return false;
    *count = 0;
    int n = snprintf(pattern, sizeof(pattern), "%s\\*", root);
    if (n <= 0 || n >= sizeof(pattern)) return false;
    WIN32_FIND_DATAA found;
    HANDLE search = FindFirstFileA(pattern, &found);
    if (search == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;
    do {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (found.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
            found.cFileName[0] == '.' || *count == capacity) continue;
        NativeFighterDraft draft;
        n = snprintf(draft.path, sizeof(draft.path), "%s\\%s", root, found.cFileName);
        NativeFighterFamilyProjectInfo info;
        if (n <= 0 || n >= sizeof(draft.path) ||
            !nativeFighterFamilyProject_ReadInfo(draft.path, &info)) continue;
        memcpy(draft.title, info.members[0].title, sizeof(draft.title));
        drafts[(*count)++] = draft;
    } while (FindNextFileA(search, &found));
    DWORD error = GetLastError();
    FindClose(search);
    if (error != ERROR_NO_MORE_FILES) return false;
    qsort(drafts, *count, sizeof(*drafts), compare_drafts);
    return true;
}
enum { PATH_BYTES = NATIVE_FIGHTER_FAMILY_PROJECT_PATH_BYTES };

static bool join_path(char* output, const char* path, const char* name)
{
    int n = snprintf(output, PATH_BYTES, "%s\\%s", path, name);
    return n > 0 && n < PATH_BYTES;
}

static bool key_valid(const char* key)
{
    size_t n = strnlen(key, NATIVE_FIGHTER_CODE_KEY_BYTES);
    if (n == 0 || n == NATIVE_FIGHTER_CODE_KEY_BYTES) return false;
    for (size_t i = 0; i < n; ++i) {
        char c = key[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-')) return false;
    }
    return true;
}

static bool mode_valid(NativeFighterFamilyMode mode, unsigned count)
{
    return (mode == NativeFighterFamily_Single && count == 1) ||
           ((mode == NativeFighterFamily_AlternateForm ||
             mode == NativeFighterFamily_SimultaneousCompanion) && count == 2);
}

static bool absent(const char* path)
{
    if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return false;
    DWORD error = GetLastError();
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
}

static bool read_layout(const char* path, NativeFighterFamilyProjectInfo* info)
{
    if (path == NULL || info == NULL) return false;
    *info = (NativeFighterFamilyProjectInfo){0};
    char marker[PATH_BYTES], descriptor[PATH_BYTES];
    if (!join_path(marker, path, "creation.pending") || !absent(marker) ||
        !join_path(descriptor, path, "family.txt")) return false;
    DWORD n = GetFullPathNameA(path, PATH_BYTES, info->paths[0], NULL);
    if (n == 0 || n >= PATH_BYTES) return false;
    info->mode = NativeFighterFamily_Single;
    info->member_count = 1;
    strcpy(info->keys[0], "main");
    FILE* file = fopen(descriptor, "rb");
    if (file == NULL) return absent(descriptor);
    char text[256] = {0};
    size_t size = fread(text, 1, sizeof(text) - 1, file);
    bool ok = !ferror(file) && feof(file);
    ok = fclose(file) == 0 && ok;
    if (!ok || size == 0 || text[size - 1] != '\n' ||
        memchr(text, 0, size) != NULL) return false;
    unsigned seen = 0;
    for (char* line = text; *line;) {
        char* end = strchr(line, '\n');
        if (end == NULL) return false;
        *end = 0;
        if (end > line && end[-1] == '\r') end[-1] = 0;
        char* value = strchr(line, '=');
        if (value == NULL) return false;
        *value++ = 0;
        unsigned bit;
        if (!strcmp(line, "mode")) {
            bit = 1;
            if (!strcmp(value, "single")) info->mode = NativeFighterFamily_Single;
            else if (!strcmp(value, "alternate")) info->mode = NativeFighterFamily_AlternateForm;
            else if (!strcmp(value, "companion")) info->mode = NativeFighterFamily_SimultaneousCompanion;
            else return false;
        } else if (!strcmp(line, "primary_key")) {
            bit = 2;
            if (!key_valid(value)) return false;
            strcpy(info->keys[0], value);
        } else if (!strcmp(line, "partner_key")) {
            bit = 4;
            if (!key_valid(value)) return false;
            strcpy(info->keys[1], value);
        } else if (!strcmp(line, "partner")) {
            bit = 8;
            if (strcmp(value, "partner")) return false;
        } else return false;
        if (seen & bit) return false;
        seen |= bit;
        line = end + 1;
    }
    if (info->mode == NativeFighterFamily_Single) return seen == 3;
    if (seen != 15 || !strcmp(info->keys[0], info->keys[1]) ||
        !join_path(info->paths[1], info->paths[0], "partner")) return false;
    info->member_count = 2;
    return true;
}

bool nativeFighterFamilyProject_ReadInfo(const char* path,
                                        NativeFighterFamilyProjectInfo* info)
{
    if (info == NULL) return false;
    *info = (NativeFighterFamilyProjectInfo){0};
    NativeFighterFamilyProjectInfo candidate;
    if (!read_layout(path, &candidate)) return false;
    for (unsigned i = 0; i < candidate.member_count; ++i) {
        char marker[PATH_BYTES];
        if (!join_path(marker, candidate.paths[i], "creation.pending") || !absent(marker) ||
            !nativeFighterProject_ReadInfo(candidate.paths[i], &candidate.members[i]))
            return false;
    }
    *info = candidate;
    return true;
}

bool nativeFighterFamilyProject_Open(const char* path,
                                    NativeFighterFamilyProject* project)
{
    if (project == NULL) return false;
    *project = (NativeFighterFamilyProject){0};
    NativeFighterFamilyProjectInfo info;
    if (!nativeFighterFamilyProject_ReadInfo(path, &info)) return false;
    project->mode = info.mode;
    project->member_count = info.member_count;
    memcpy(project->keys, info.keys, sizeof(project->keys));
    memcpy(project->paths, info.paths, sizeof(project->paths));
    for (unsigned i = 0; i < info.member_count; ++i) {
        if (!nativeFighterProject_Open(info.paths[i], &project->members[i])) {
            nativeFighterFamilyProject_Close(project);
            return false;
        }
    }
    return true;
}

void nativeFighterFamilyProject_Close(NativeFighterFamilyProject* project)
{
    if (project == NULL) return;
    for (unsigned i = 0; i < NATIVE_FIGHTER_FAMILY_MEMBERS; ++i)
        nativeFighterProject_Close(&project->members[i]);
    *project = (NativeFighterFamilyProject){0};
}

static bool write_file(const char* path, const char* name,
                       const char* text, size_t size)
{
    char target[PATH_BYTES];
    if (!join_path(target, path, name)) return false;
    HANDLE file = CreateFileA(target, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(file, text, (DWORD)size, &written, NULL) &&
              written == size && FlushFileBuffers(file);
    return CloseHandle(file) && ok;
}

static bool begin_creation(const char* path, char destination[PATH_BYTES],
                            char staging[PATH_BYTES])
{
    if (path == NULL) return false;
    DWORD n = GetFullPathNameA(path, PATH_BYTES, destination, NULL);
    if (n == 0 || n >= PATH_BYTES || !absent(destination)) return false;
    for (unsigned i = 0; i < 16; ++i) {
        int length = snprintf(staging, PATH_BYTES, "%s.creating-%08lx-%u",
                               destination, GetCurrentProcessId(), i);
        if (length <= 0 || length >= PATH_BYTES) return false;
        if (CreateDirectoryA(staging, NULL))
            return write_file(staging, "creation.pending", "constructing\n", 13);
        if (GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    return false;
}

static bool publish(const char* destination, const char* staging,
                     NativeFighterFamilyMode mode, unsigned count,
                     const char* primary_key, const char* partner_key)
{
    if (!mode_valid(mode, count) || !key_valid(primary_key) ||
        (count == 2 && (!key_valid(partner_key) || !strcmp(primary_key, partner_key))))
        return false;
    if (count == 2 || strcmp(primary_key, "main")) {
        char descriptor[256];
        int size = count == 1
            ? snprintf(descriptor, sizeof(descriptor), "mode=single\nprimary_key=%s\n", primary_key)
            : snprintf(descriptor, sizeof(descriptor),
                "mode=%s\nprimary_key=%s\npartner_key=%s\npartner=partner\n",
                mode == NativeFighterFamily_AlternateForm ? "alternate" : "companion",
                primary_key, partner_key);
        if (size <= 0 || (size_t)size >= sizeof(descriptor) ||
            !write_file(staging, "family.txt", descriptor, (size_t)size)) return false;
    }
    if (!MoveFileExA(staging, destination, MOVEFILE_WRITE_THROUGH)) return false;
    char marker[PATH_BYTES];
    return join_path(marker, destination, "creation.pending") && DeleteFileA(marker);
}

// Transfer the complete primary payload, including split source directories,
// without interpreting filenames. Both absolute paths are fixed under staging.
static bool publish_primary_files(const char* staging)
{
    char primary[PATH_BYTES], pattern[PATH_BYTES];
    if (!join_path(primary, staging, "main") || !join_path(pattern, primary, "*"))
        return false;
    WIN32_FIND_DATAA entry;
    HANDLE search = FindFirstFileA(pattern, &entry);
    if (search == INVALID_HANDLE_VALUE) return false;
    bool ok = true;
    unsigned count = 0;
    do {
        if (!strcmp(entry.cFileName, ".") || !strcmp(entry.cFileName, "..")) continue;
        char from[PATH_BYTES], to[PATH_BYTES];
        if (++count > 1024 || (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
            !join_path(from, primary, entry.cFileName) ||
            !join_path(to, staging, entry.cFileName) ||
            !MoveFileExA(from, to, MOVEFILE_WRITE_THROUGH)) {
            ok = false;
            break;
        }
    } while (FindNextFileA(search, &entry));
    if (ok && GetLastError() != ERROR_NO_MORE_FILES) ok = false;
    ok = FindClose(search) && ok;
    return ok && RemoveDirectoryA(primary);
}

bool nativeFighterFamilyProject_CreateStockFromDisc(
    const char* path, const char* title, NativeFighterFamilyMode mode,
    FighterKind primary, FighterKind partner)
{
    const unsigned count = mode == NativeFighterFamily_Single ? 1 : 2;
    if (!mode_valid(mode, count) || primary == Ft_Kind_None ||
        (count == 1 ? partner != Ft_Kind_None : partner == Ft_Kind_None))
        return false;
    char destination[PATH_BYTES], staging[PATH_BYTES], primary_path[PATH_BYTES];
    if (!begin_creation(path, destination, staging) ||
        !join_path(primary_path, staging, "main")) return false;
    // The stock member writer requires a new directory. Construct primary in
    // its own temporary child, then move its concrete files into the staged root.
    if (!nativeFighterProject_CreateStockFromDisc(primary_path, title, primary))
        return false;
    if (count == 2) {
        char partner_path[PATH_BYTES];
        if (!join_path(partner_path, staging, "partner") ||
            !nativeFighterProject_CreateStockFromDisc(partner_path, title, partner))
            return false;
    }
    if (!publish_primary_files(staging)) return false;
    return publish(destination, staging, mode, count, "main", "partner");
}

bool nativeFighterFamilyProject_CreateFromPackage(
    const char* path, const char* title, const void* package, size_t package_size)
{
    NativeFighterPackage metadata;
    NativeFighterPackageData data;
    if (!nativeContentStore_ParseFighter(package, package_size, &metadata, &data))
        return false;
    char destination[PATH_BYTES], staging[PATH_BYTES], member_path[PATH_BYTES];
    if (!begin_creation(path, destination, staging)) return false;
    // Each direct writer creates its directory, so primary is constructed in
    // a temporary main child before publishing its actual payload at root.
    for (unsigned i = 0; i < metadata.member_count; ++i) {
        if (!join_path(member_path, staging, i == 0 ? "main" : "partner") ||
            !nativeFighterProject_CreateFromMember(member_path, title,
                &metadata.members[i], &data.members[i], data.portrait_png,
                data.portrait_size)) return false;
    }
    if (!publish_primary_files(staging)) return false;
    return publish(destination, staging, metadata.mode, metadata.member_count,
                    metadata.members[0].key, metadata.members[1].key);
}

bool nativeFighterFamilyProject_Build(const NativeFighterFamilyProject* project,
    const void* portrait_png, size_t portrait_size, uint8_t** package,
    size_t* package_size, NativeContentId* id)
{
    if (!project || !package || !package_size || !id ||
        !mode_valid(project->mode, project->member_count)) return false;
    for (unsigned m = 0; m < project->member_count; ++m)
        if (!key_valid(project->keys[m]) || !project->members[m].action_count ||
            (m && !strcmp(project->keys[0], project->keys[m]))) return false;

    NativeSourcePackage* candidates = calloc(project->member_count, sizeof(*candidates));
    if (!candidates) return false;
    NativeFighterPackage metadata = {.mode=project->mode, .member_count=(uint8_t)project->member_count};
    NativeFighterPackageData data = {.portrait_png=portrait_png, .portrait_size=portrait_size};
    memcpy(metadata.title, project->members[0].title, sizeof(metadata.title));
    bool valid = true;
    for (unsigned m = 0; valid && m < project->member_count; ++m) {
        const NativeFighterProject* member = &project->members[m];
        NativeFighterCpuProfile cpu;
        NativeSourcePackageInput input = {.family=NativeSourcePackage_Single,
            .facts=member->asset_record, .facts_bytes=member->asset_record_size,
            .assets={member->fighter_dat,member->fighter_size,member->animation_dat,
                member->animation_size,member->costume_dat,member->costume_size},
            .source=(const char*)member->script_text, .source_bytes=member->script_size,
            .parameters=(const char*)member->parameter_text, .parameter_bytes=member->parameter_size,
            .resources=member->resources, .resource_bytes=member->resource_size};
        NativeScriptDiagnostic diagnostic = {0};
        valid = nativeFighterCpu_Parse((const char*)member->cpu_text, member->cpu_size, &cpu) &&
            nativeSourcePackage_Prepare(&candidates[m], &input,
                (FighterKind)(Ft_Kind_CustomFirst+m), NULL, &diagnostic);
        if (!valid) {
            fprintf(stderr,"Source family member %s could not prepare: %s\n",
                project->keys[m],diagnostic.message[0]?diagnostic.message:"invalid CPU profile");
            break;
        }
        strcpy(metadata.members[m].key, project->keys[m]);
        data.members[m] = (NativeFighterMemberData){
            .fighter_dat=member->fighter_dat, .fighter_size=member->fighter_size,
            .animation_dat=member->animation_dat, .animation_size=member->animation_size,
            .costume_dat=member->costume_dat, .costume_size=member->costume_size,
            .asset_record=member->asset_record, .asset_record_size=member->asset_record_size,
            .script_text=member->script_text, .script_size=member->script_size,
            .parameter_text=member->parameter_text, .parameter_size=member->parameter_size,
            .cpu_text=member->cpu_text, .cpu_size=member->cpu_size,
            .resources=member->resources, .resource_size=member->resource_size};
    }
    bool released = true;
    for (unsigned m = 0; m < project->member_count; ++m)
        if (!nativeSourcePackage_Release(&candidates[m])) released = false;
    if (released) free(candidates);
    return valid && released && nativeContentStore_BuildFighter(&metadata,&data,package,package_size,id);
}

#endif
