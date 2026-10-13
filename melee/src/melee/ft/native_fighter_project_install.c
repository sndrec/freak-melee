#include "native_fighter_project.h"
#include "native_fighter_family_project.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/mn/native_css_layout.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool draft_path(char result[MAX_PATH], const char* draft,
                       const char* name)
{
    int length = snprintf(result, MAX_PATH, "%s\\%s", draft, name);
    return length > 0 && length < MAX_PATH;
}

static bool prior_install(const char* path, NativeContentId* id, bool* found)
{
    *found = false;
    char file_path[MAX_PATH];
    if (!draft_path(file_path, path, "installed.id")) return false;
    FILE* file = fopen(file_path, "rb");
    if (file == NULL) return GetFileAttributesA(file_path) ==
                                   INVALID_FILE_ATTRIBUTES &&
                             GetLastError() == ERROR_FILE_NOT_FOUND;
    char hex[66];
    size_t count = fread(hex, 1, sizeof(hex), file);
    bool ok = fclose(file) == 0 && count == 65 && hex[64] == '\n';
    if (ok) {
        hex[64] = '\0';
        ok = nativeContentStore_IdFromHex(hex, id);
    }
    *found = ok;
    return ok;
}

static bool record_install(const char* path, const NativeContentId* id)
{
    char output[MAX_PATH], temporary[MAX_PATH], hex[65];
    if (!draft_path(output, path, "installed.id") ||
        !draft_path(temporary, path, "installed.id.tmp")) return false;
    nativeContentStore_IdHex(id, hex);
    char contents[66];
    memcpy(contents, hex, 64);
    contents[64] = '\n';
    contents[65] = '\0';
    HANDLE file = CreateFileA(temporary, GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    bool ok = WriteFile(file, contents, 65, &written, NULL) &&
              written == 65 && FlushFileBuffers(file);
    ok = CloseHandle(file) && ok;
    if (ok) ok = MoveFileExA(temporary, output,
                            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok) DeleteFileA(temporary);
    return ok;
}

static bool draft_matches_cache(const char* path, const char* content_root,
                                const NativeContentId* id)
{
    NativeFighterFamilyProject draft;
    if (!nativeFighterFamilyProject_Open(path, &draft)) return false;
    char portrait_path[MAX_PATH];
    bool ok = draft_path(portrait_path, path, "portrait.png");
    FILE* file = ok ? fopen(portrait_path, "rb") : NULL;
    long size = 0;
    ok = file != NULL && fseek(file, 0, SEEK_END) == 0 &&
         (size = ftell(file)) >= 32 && size <= 16 * 1024 * 1024 &&
         fseek(file, 0, SEEK_SET) == 0;
    uint8_t* portrait = ok ? malloc((size_t) size) : NULL;
    ok = portrait != NULL &&
         fread(portrait, 1, (size_t) size, file) == (size_t) size;
    if (file != NULL) ok = fclose(file) == 0 && ok;
    uint8_t* package = NULL;
    size_t package_size = 0;
    NativeContentId built;
    ok = ok && nativeFighterFamilyProject_Build(&draft, portrait, (size_t) size,
                                           &package, &package_size, &built) &&
         memcmp(&built, id, sizeof(built)) == 0 &&
         nativeContentStore_IsCached(content_root, id);
    free(package);
    free(portrait);
    nativeFighterFamilyProject_Close(&draft);
    return ok;
}

bool nativeFighterProject_Install(const char* path, const char* content_root,
                                  const NativeContentId* id,
                                  NativeContentId* replaced)
{
    if (replaced != NULL) *replaced = (NativeContentId){ 0 };
    if (path == NULL || content_root == NULL || id == NULL ||
        !draft_matches_cache(path, content_root, id)) return false;
    NativeContentId old_id;
    bool has_old;
    if (!prior_install(path, &old_id, &has_old)) return false;
    bool replacing = has_old && memcmp(&old_id, id, sizeof(*id)) != 0;
    bool new_was_installed = nativeContentStore_IsInstalled(content_root, id);
    if (!nativeContentStore_Install(content_root, id)) return false;

    char layout_path[MAX_PATH];
    NativeCssLayout layout, original;
    bool missing = false;
    bool changed_layout = false;
    bool ok = true;
    if (replacing) {
        ok = nativeCssLayout_DefaultPath(layout_path, sizeof(layout_path));
        if (ok) {
            ok = nativeCssLayout_LoadFile(&layout, layout_path, &missing);
            if (!ok && missing) ok = true;
        }
        if (ok && !missing) {
            original = layout;
            ok = nativeCssLayout_ReplaceFighter(&layout, &old_id, id);
            changed_layout = ok && memcmp(&layout, &original,
                                          sizeof(layout)) != 0;
            if (changed_layout) ok = nativeCssLayout_SaveFile(&layout, layout_path);
        }
    }
    if (ok) ok = record_install(path, id);
    if (ok && replacing && nativeContentStore_IsInstalled(content_root, &old_id))
        ok = nativeContentStore_Uninstall(content_root, &old_id);
    if (!ok) {
        if (has_old) record_install(path, &old_id);
        if (changed_layout) nativeCssLayout_SaveFile(&original, layout_path);
        if (!new_was_installed) nativeContentStore_Uninstall(content_root, id);
    }
    if (ok && replacing && replaced != NULL) *replaced = old_id;
    return ok;
}

#endif
