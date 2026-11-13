#include "native_hub_delete.h"
#include "native_hub_catalog.h"
#include "native_shell_css_original.h"
#include <melee/gm/native_home.h>
#include <melee/mn/native_css_original.h>
#include <yyjson.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstring>

namespace {
    struct Deletion {
        char project[256];
        NativeContentId package;
        bool installed;
    };

    bool paths(char (&content)[512], char (&projects)[512],
               char (&journal)[560], char (&staged)[560])
    {
        const char* root = nativeHubCatalog_Root(content);
        if (!root || !nativeFighterFamilyProject_LocalRoot(projects, sizeof(projects)))
            return false;
        if (root != content) std::snprintf(content, sizeof(content), "%s", root);
        return std::snprintf(journal, sizeof(journal), "%s/delete-fighter.json", content) < sizeof(journal) &&
            std::snprintf(staged, sizeof(staged), "%s/.delete-pending", projects) < sizeof(staged);
    }

    bool project_name(const char* name)
    {
        const size_t size = strnlen(name, 256);
        if (!size || size == 256 || name[0] == '.' || name[size - 1] == '.') return false;
        for (size_t i = 0; i < size; ++i) {
            const unsigned char c = name[i];
            if (c < 32 || std::strchr("/\\:*?\"<>|", c)) return false;
        }
        return true;
    }

    bool exists(const char* path)
    {
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) return true;
        const DWORD error = GetLastError();
        // An unreadable journal must block another deletion, just as an
        // existing journal does. Only a missing path is safely absent.
        return error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND;
    }

    bool remove_tree(const char* path, unsigned depth = 0)
    {
        if (depth > 64) return false;
        const DWORD attributes = GetFileAttributesA(path);
        if (attributes == INVALID_FILE_ATTRIBUTES)
            return GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND;
        if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) return DeleteFileA(path) != 0;
        // Delete junctions as directory entries, without visiting their targets.
        if (attributes & FILE_ATTRIBUTE_REPARSE_POINT) return RemoveDirectoryA(path) != 0;
        char pattern[1024];
        if (std::snprintf(pattern, sizeof(pattern), "%s/*", path) >= sizeof(pattern)) return false;
        WIN32_FIND_DATAA entry;
        HANDLE search = FindFirstFileA(pattern, &entry);
        if (search == INVALID_HANDLE_VALUE) return false;
        bool ok = true;
        do {
            if (!std::strcmp(entry.cFileName, ".") || !std::strcmp(entry.cFileName, "..")) continue;
            char child[1024];
            if (std::snprintf(child, sizeof(child), "%s/%s", path, entry.cFileName) >= sizeof(child) ||
                !remove_tree(child, depth + 1)) {
                ok = false;
                break;
            }
        } while (FindNextFileA(search, &entry));
        if (ok && GetLastError() != ERROR_NO_MORE_FILES) ok = false;
        FindClose(search);
        return ok && RemoveDirectoryA(path);
    }

    bool read_journal(const char* path, Deletion& deletion)
    {
        FILE* file = std::fopen(path, "rb");
        if (!file) return false;
        char bytes[1025];
        const size_t size = std::fread(bytes, 1, sizeof(bytes), file);
        const bool read = !std::ferror(file) && size < sizeof(bytes);
        std::fclose(file);
        if (!read) return false;
        yyjson_doc* doc = yyjson_read(bytes, size, 0);
        if (!doc) return false;
        yyjson_val* root = yyjson_doc_get_root(doc);
        const char* project = yyjson_get_str(yyjson_obj_get(root, "project"));
        const char* id = yyjson_get_str(yyjson_obj_get(root, "package"));
        yyjson_val* installed = yyjson_obj_get(root, "installed");
        bool ok = project && std::strlen(project) < sizeof(deletion.project) &&
            (!*project || project_name(project)) && yyjson_is_bool(installed) && id &&
            nativeContentStore_IdFromHex(id, &deletion.package);
        if (ok) {
            std::snprintf(deletion.project, sizeof(deletion.project), "%s", project);
            deletion.installed = yyjson_get_bool(installed);
            ok = *project || deletion.installed;
        }
        yyjson_doc_free(doc);
        return ok;
    }

    bool write_journal(const char* path, const Deletion& deletion)
    {
        char hex[65], temporary[568];
        nativeContentStore_IdHex(&deletion.package, hex);
        if (std::snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= sizeof(temporary)) return false;
        yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
        if (!doc) return false;
        yyjson_mut_val* root = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, root);
        const bool values = yyjson_mut_obj_add_str(doc, root, "project", deletion.project) &&
            yyjson_mut_obj_add_str(doc, root, "package", hex) &&
            yyjson_mut_obj_add_bool(doc, root, "installed", deletion.installed);
        bool ok = values && yyjson_mut_write_file(temporary, doc, 0, nullptr, nullptr);
        yyjson_mut_doc_free(doc);
        if (ok) ok = MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        if (!ok) DeleteFileA(temporary);
        return ok;
    }

    bool finish(const char* content, const char* projects, const char* journal,
                const char* staged, const Deletion& deletion)
    {
        if (!nativeHome_IsSelection() || !nativeCssOriginal_CanEdit()) return false;
        if (*deletion.project) {
            char original[800];
            if (std::snprintf(original, sizeof(original), "%s/%s", projects, deletion.project) >= sizeof(original))
                return false;
            if (exists(original)) {
                const DWORD attributes = GetFileAttributesA(original);
                if (!(attributes & FILE_ATTRIBUTE_DIRECTORY) ||
                    (attributes & FILE_ATTRIBUTE_REPARSE_POINT) || exists(staged) ||
                    !MoveFileExA(original, staged, MOVEFILE_WRITE_THROUGH)) return false;
            }
        }
        if (deletion.installed) {
            NativeCssLayout layout = *nativeCssOriginal_Layout();
            bool changed = false;
            for (unsigned i = layout.count; i > 0; --i) {
                if (!std::memcmp(&layout.tiles[i - 1].fighter_package_id, &deletion.package, sizeof(deletion.package))) {
                    if (!nativeCssLayout_Remove(&layout, i - 1)) return false;
                    changed = true;
                }
            }
            nativeHome_RemoveInstalledFighter(&deletion.package);
            if (changed && !nativeCssOriginal_ReplaceLayout(&layout)) return false;
            if (nativeContentStore_IsInstalled(content, &deletion.package) &&
                !nativeContentStore_Uninstall(content, &deletion.package)) return false;
        }
        if (*deletion.project && !remove_tree(staged)) return false;
        if (!DeleteFileA(journal)) return false;
        nativeShellCssOriginal_RefreshInstalledFighters();
        nativeCssOriginal_ReloadInstalledFighters();
        return true;
    }

    struct DeletionLock {
        HANDLE mutex = CreateMutexA(nullptr, FALSE, "Local\\FreakMeleeFighterDeletion");
        bool acquired = false;
        DeletionLock() {
            if (mutex) {
                const DWORD result = WaitForSingleObject(mutex, 0);
                acquired = result == WAIT_OBJECT_0 || result == WAIT_ABANDONED;
            }
        }
        ~DeletionLock() {
            if (acquired) ReleaseMutex(mutex);
            if (mutex) CloseHandle(mutex);
        }
    };
}

bool nativeHubDelete_Recover()
{
    char content[512], projects[512], journal[560], staged[560];
    if (!paths(content, projects, journal, staged)) return false;
    if (!exists(journal)) return true;
    DeletionLock lock;
    Deletion deletion{};
    return lock.acquired && read_journal(journal, deletion) &&
        finish(content, projects, journal, staged, deletion);
}

bool nativeHubDelete_Fighter(unsigned index)
{
    if (index >= nativeHubFighterCount || !nativeHome_IsSelection() ||
        !nativeCssOriginal_CanEdit()) return false;
    DeletionLock lock;
    if (!lock.acquired) return false;
    char content[512], projects[512], journal[560], staged[560];
    if (!paths(content, projects, journal, staged) || exists(journal) || exists(staged)) return false;
    const NativeHubFighter& fighter = nativeHubFighters[index];
    Deletion deletion{};
    deletion.installed = fighter.installed;
    deletion.package = fighter.id;
    if (*fighter.draft) {
        char absolute[512], root[512];
        const DWORD n = GetFullPathNameA(fighter.draft, sizeof(absolute), absolute, nullptr);
        const DWORD r = GetFullPathNameA(projects, sizeof(root), root, nullptr);
        if (!n || n >= sizeof(absolute) || !r || r >= sizeof(root) ||
            _strnicmp(root, absolute, r) || absolute[r] != '\\') return false;
        const char* leaf = absolute + r + 1;
        if (!project_name(leaf)) return false;
        std::snprintf(deletion.project, sizeof(deletion.project), "%s", leaf);
    }
    return write_journal(journal, deletion) &&
        finish(content, projects, journal, staged, deletion);
}
