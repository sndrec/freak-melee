#include "native_fighter_project.h"
#include "native_source_assets.h"
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { ASSET_COUNT = 4, PATH_BYTES = 600, REPLACE_COUNT = ASSET_COUNT * 2 };
static const char* const asset_names[ASSET_COUNT] = {
    "fighter.dat", "animation.dat", "costume.dat", "assets.fma"
};
typedef struct AssetBytes { const void* data; size_t size; } AssetBytes;
typedef struct AssetReplace {
    char path[PATH_BYTES], staged[PATH_BYTES], rollback[PATH_BYTES];
    AssetBytes bytes;
    bool staged_owned, old_moved, installed;
} AssetReplace;

static bool error(char* output, size_t capacity, const char* message)
{
    if (output && capacity) snprintf(output, capacity, "%s", message);
    return false;
}
static bool filename(char output[PATH_BYTES], const char* path,
                     const char* name, const char* suffix)
{
    int n = snprintf(output, PATH_BYTES, "%s/%s%s", path, name, suffix);
    return n > 0 && n < PATH_BYTES;
}
static bool vacant(const char* path)
{
    return GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES &&
           GetLastError() == ERROR_FILE_NOT_FOUND;
}
static bool stage(AssetReplace* file)
{
    HANDLE handle = CreateFileA(file->staged, GENERIC_WRITE, 0, NULL,
                                CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) return false;
    file->staged_owned = true;
    DWORD written = 0;
    bool ok = file->bytes.size <= MAXDWORD &&
        WriteFile(handle, file->bytes.data, (DWORD)file->bytes.size, &written, NULL) &&
        written == file->bytes.size && FlushFileBuffers(handle);
    if (!CloseHandle(handle)) ok = false;
    return ok;
}
static void assets(const NativeFighterProject* p, AssetBytes output[ASSET_COUNT])
{
    output[0] = (AssetBytes){p->fighter_dat, p->fighter_size};
    output[1] = (AssetBytes){p->animation_dat, p->animation_size};
    output[2] = (AssetBytes){p->costume_dat, p->costume_size};
    output[3] = (AssetBytes){p->asset_record, p->asset_record_size};
}
bool nativeFighterProject_ValidateAssets(const NativeFighterProject* p,
                                         char* message, size_t capacity)
{
    if (!p) return error(message, capacity, "Asset validation needs a project.");
    AssetBytes bytes[ASSET_COUNT]; assets(p, bytes);
    for (unsigned i = 0; i < ASSET_COUNT; ++i)
        if (!bytes[i].data || !bytes[i].size || bytes[i].size > NATIVE_CONTENT_MAX_PACKAGE_BYTES)
            return error(message, capacity, "The draft has a missing or oversized asset.");
    NativeSourceAssetFacts facts;
    NativeScriptDiagnostic diagnostic = {0};
    uint8_t* canonical = malloc(NATIVE_SOURCE_FACT_BYTES);
    size_t size = 0;
    bool ok = canonical &&
        nativeSourceAssets_Read(p->asset_record, p->asset_record_size, &facts, &diagnostic) &&
        nativeSourceAssets_Write(&p->facts, canonical, NATIVE_SOURCE_FACT_BYTES, &size, &diagnostic) &&
        size == p->asset_record_size && !memcmp(canonical, p->asset_record, size) &&
        !strcmp(p->fighter_root, facts.fighter_root) &&
        p->action_count == facts.actions && p->demo_count == facts.demo_count;
    free(canonical);
    if (!ok) return error(message, capacity, "The draft's rig facts disagree with its asset record or descriptor counts.");
    NativeSourceImportedAssets* candidate = calloc(1, sizeof(*candidate));
    if (!candidate) return error(message, capacity, "Could not allocate asset validation storage.");
    NativeSourceAssetInput input = {p->fighter_dat, p->fighter_size,
        p->animation_dat, p->animation_size, p->costume_dat, p->costume_size};
    ok = nativeSourceAssets_Import(&facts, &input, Ft_Kind_CustomFirst, candidate, &diagnostic);
    nativeSourceAssets_Release(candidate);
    free(candidate);
    return ok || error(message, capacity, diagnostic.message);
}
static bool publish(const char* path, const AssetBytes current[ASSET_COUNT],
                     const AssetBytes previous[ASSET_COUNT], char* message, size_t capacity)
{
    AssetReplace files[REPLACE_COUNT] = {0};
    // Check all names before touching any file. Files left by an interrupted
    // operation are recoverable evidence, not scratch we can silently delete.
    for (unsigned i = 0; i < REPLACE_COUNT; ++i) {
        const unsigned a = i % ASSET_COUNT;
        const char* suffix = i < ASSET_COUNT ? "" : ".rig.previous";
        char name[80];
        snprintf(name, sizeof(name), "%s%s", asset_names[a], suffix);
        AssetReplace* f = &files[i];
        f->bytes = i < ASSET_COUNT ? current[a] : previous[a];
        if (!filename(f->path, path, name, "") ||
            !filename(f->staged, path, name, ".rig.stage") ||
            !filename(f->rollback, path, name, ".rig.rollback") ||
            !vacant(f->staged) || !vacant(f->rollback))
            return error(message, capacity, "Asset staging paths are unavailable. Check for files from an interrupted save.");
        DWORD attributes = GetFileAttributesA(f->path);
        if ((attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) ||
            (attributes == INVALID_FILE_ATTRIBUTES && GetLastError() != ERROR_FILE_NOT_FOUND))
            return error(message, capacity, "An asset destination is unavailable or is a directory.");
    }
    bool ok = true;
    for (unsigned i = 0; ok && i < REPLACE_COUNT; ++i) ok = stage(&files[i]);
    const DWORD flags = MOVEFILE_WRITE_THROUGH;
    for (unsigned i = 0; ok && i < REPLACE_COUNT; ++i) {
        AssetReplace* f = &files[i];
        if (!vacant(f->path)) {
            f->old_moved = MoveFileExA(f->path, f->rollback, flags) != 0;
            if (!f->old_moved) { ok = false; break; }
        }
        f->installed = MoveFileExA(f->staged, f->path, flags) != 0;
        if (!f->installed) { ok = false; break; }
        f->staged_owned = false;
    }
    bool restored = true;
    if (!ok) for (unsigned i = REPLACE_COUNT; i; --i) {
        AssetReplace* f = &files[i - 1];
        if (f->old_moved) {
            if (!MoveFileExA(f->rollback, f->path, flags | MOVEFILE_REPLACE_EXISTING)) restored = false;
            else f->old_moved = false;
        } else if (f->installed && !DeleteFileA(f->path)) restored = false;
    }
    for (unsigned i = 0; i < REPLACE_COUNT; ++i) {
        if (files[i].staged_owned) DeleteFileA(files[i].staged);
        if (ok && files[i].old_moved && !DeleteFileA(files[i].rollback))
            fprintf(stderr, "[workshop-assets] Saved assets; retained old file at %s\n", files[i].rollback);
    }
    if (!restored) return error(message, capacity, "Asset save failed and a previous file could not be restored. Its .rig.rollback file was retained.");
    if (!ok) return error(message, capacity, "Could not replace the asset files. The previous assets were restored.");
    if (message && capacity) message[0] = 0;
    return true;
}
bool nativeFighterProject_SaveAssets(const char* path,
    const NativeFighterProject* p, char* message, size_t capacity)
{
    if (!path || !p) return error(message, capacity, "Asset save needs a project and directory.");
    NativeFighterProject disk;
    if (!nativeFighterProject_Open(path, &disk))
        return error(message, capacity, "Could not open the saved draft.");
    // Rig counts/indices may change, but this is still the same editable draft.
    bool same = disk.base_kind == p->base_kind && !strcmp(disk.title, p->title) &&
        !strcmp(disk.template_key, p->template_key) &&
        !strcmp(disk.fighter_root, p->fighter_root) &&
        !strcmp(disk.facts.costume_joint, p->facts.costume_joint);
    AssetBytes before[ASSET_COUNT], after[ASSET_COUNT];
    assets(&disk, before); assets(p, after);
    bool changed = false;
    for (unsigned i = 0; i < ASSET_COUNT; ++i)
        changed |= before[i].size != after[i].size || !after[i].data ||
            (before[i].size == after[i].size && memcmp(before[i].data, after[i].data, before[i].size));
    bool ok = same && nativeFighterProject_ValidateAssets(p, message, capacity) &&
        (!changed || publish(path, after, before, message, capacity));
    nativeFighterProject_Close(&disk);
    if (!same) return error(message, capacity, "The saved directory belongs to a different draft.");
    if (ok && message && capacity) message[0] = 0;
    return ok;
}
bool nativeFighterProject_UndoLastAssetSave(const char* path, char* message, size_t capacity)
{
    NativeFighterProject p;
    if (!path || !nativeFighterProject_Open(path, &p))
        return error(message, capacity, "Could not open the saved draft.");
    uint8_t* previous[ASSET_COUNT] = {0}; size_t sizes[ASSET_COUNT] = {0};
    bool ok = true;
    for (unsigned i = 0; ok && i < ASSET_COUNT; ++i) {
        char name[PATH_BYTES];
        ok = filename(name, path, asset_names[i], ".rig.previous");
        FILE* file = ok ? fopen(name, "rb") : NULL;
        if (!file) { ok = false; break; }
        long size;
        ok = !fseek(file, 0, SEEK_END) && (size = ftell(file)) > 0 &&
            (size_t)size <= NATIVE_CONTENT_MAX_PACKAGE_BYTES && !fseek(file, 0, SEEK_SET);
        if (ok) { sizes[i] = (size_t)size; previous[i] = malloc(sizes[i]); }
        ok = ok && previous[i] && fread(previous[i], 1, sizes[i], file) == sizes[i];
        if (fclose(file)) ok = false;
    }
    NativeSourceAssetFacts facts; NativeScriptDiagnostic diagnostic = {0};
    ok = ok && nativeSourceAssets_Read(previous[3], sizes[3], &facts, &diagnostic);
    if (ok) {
        free(p.fighter_dat); p.fighter_dat = previous[0]; p.fighter_size = sizes[0];
        free(p.animation_dat); p.animation_dat = previous[1]; p.animation_size = sizes[1];
        free(p.costume_dat); p.costume_dat = previous[2]; p.costume_size = sizes[2];
        free(p.asset_record); p.asset_record = previous[3]; p.asset_record_size = sizes[3];
        memset(previous, 0, sizeof(previous));
        p.facts = facts; p.action_count = facts.actions; p.demo_count = facts.demo_count;
        snprintf(p.fighter_root, sizeof(p.fighter_root), "%s", facts.fighter_root);
        ok = nativeFighterProject_SaveAssets(path, &p, message, capacity);
    } else error(message, capacity, "The complete previous asset set is missing or invalid.");
    for (unsigned i = 0; i < ASSET_COUNT; ++i) free(previous[i]);
    nativeFighterProject_Close(&p);
    return ok;
}
#endif
