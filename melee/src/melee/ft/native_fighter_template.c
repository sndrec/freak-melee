#include "native_fighter_template.h"
#include "native_fighter_project.h"
#include "native_fighter_parameters.h"
#include "native_fighter_action_editor.h"
#include <melee/ft/native_source_bank.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool nativeFighterTemplate_Read(FighterKind kind, char** source, size_t* size)
{
    if (!source || !size) {
        return false;
    }
    const char* name = NULL;
    switch (kind) {
    case Ft_Kind_Mario:
        name = "mario_full";
        break;
    case Ft_Kind_DrMario:
        name = "drmario_full";
        break;
    case Ft_Kind_Captain:
        name = "captain_wip";
        break;
    case Ft_Kind_Ganon:
        name = "ganon_wip";
        break;
    case Ft_Kind_Luigi:
        name = "luigi_wip";
        break;
    case Ft_Kind_Purin:
        name = "purin_wip";
        break;
    case Ft_Kind_Fox:
        name = "fox_wip";
        break;
    case Ft_Kind_Falco:
        name = "falco_wip";
        break;
    case Ft_Kind_Mars:
        name = "marth_wip";
        break;
    case Ft_Kind_Emblem:
        name = "roy_wip";
        break;
    case Ft_Kind_Donkey:
        name = "donkey_wip";
        break;
    case Ft_Kind_Koopa:
        name = "koopa_wip";
        break;
    case Ft_Kind_Pikachu:
        name = "pikachu_wip";
        break;
    case Ft_Kind_Pichu:
        name = "pichu_wip";
        break;
    case Ft_Kind_Samus:
        name = "samus_full";
        break;
    case Ft_Kind_Link:
        name = "link_full";
        break;
    case Ft_Kind_CLink:
        name = "young_link_full";
        break;
    case Ft_Kind_Mewtwo:
        name = "mewtwo_wip";
        break;
    case Ft_Kind_Yoshi:
        name = "yoshi_wip";
        break;
    case Ft_Kind_Ness:
        name = "ness_wip";
        break;
    case Ft_Kind_Peach:
        name = "peach_full";
        break;
    case Ft_Kind_GameWatch:
        name = "gamewatch_full";
        break;
    case Ft_Kind_Zelda:
        name = "zelda_full";
        break;
    case Ft_Kind_Seak:
        name = "sheik_full";
        break;
    case Ft_Kind_Popo:
        name = "popo_full";
        break;
    case Ft_Kind_Nana:
        name = "nana_full";
        break;
    default:
        return false;
    }
    char path[512];
    DWORD length = GetModuleFileNameA(NULL, path, sizeof(path));
    char* basename =
        length > 0 && length < sizeof(path) ? strrchr(path, '\\') : NULL;
    if (!basename) {
        return false;
    }
    int n = snprintf(basename, sizeof(path) - (basename - path),
                     "\\ui\\%s.fms", name);
    if (n <= 0 || n >= sizeof(path) - (basename - path)) {
        return false;
    }
    FILE* file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    long bytes = 0;
    bool ok = fseek(file, 0, SEEK_END) == 0 && (bytes = ftell(file)) > 0 &&
              bytes < NATIVE_SOURCE_SOURCE_BYTES &&
              fseek(file, 0, SEEK_SET) == 0;
    char* result = ok ? malloc((size_t) bytes + 1) : NULL;
    ok = result && fread(result, 1, (size_t) bytes, file) == (size_t) bytes;
    ok = fclose(file) == 0 && ok;
    if (!ok) {
        free(result);
        return false;
    }
    size_t used = 0;
    for (long i = 0; i < bytes; ++i)
        if (result[i] != '\r') result[used++] = result[i];
    result[used] = 0;
    *source = result;
    *size = used;
    return true;
}
static bool write_new(const char* path, const char* name, const void* bytes,
                      size_t size)
{
    char target[560];
    int length = snprintf(target, sizeof(target), "%s/%s", path, name);
    if (length <= 0 || length >= sizeof(target)) {
        return false;
    }
    HANDLE file = CreateFileA(target, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    DWORD written = 0;
    bool ok = WriteFile(file, bytes, (DWORD) size, &written, NULL) &&
              written == size && FlushFileBuffers(file);
    return CloseHandle(file) && ok;
}
bool nativeFighterTemplate_Apply(const char* path)
{
    NativeFighterProject project = { 0 };
    if (!path || !nativeFighterProject_Open(path, &project)) {
        return false;
    }
    if (project.script_size || project.parameter_size) {
        nativeFighterProject_Close(&project);
        return false;
    }
    // The editable hitbox durations belong to the new fighter. Prepare and
    // save them during cloning, so merely opening the editor makes no edits.
    unsigned converted_hitboxes = 0;
    nativeFighterAction_ConvertAllHitboxIntervals(&project,
                                                  &converted_hitboxes);
    if (converted_hitboxes && !nativeFighterProject_Save(path, &project)) {
        nativeFighterProject_Close(&project);
        return false;
    }
    char *source = NULL, *converted = NULL, *parameters = NULL;
    size_t size = 0, converted_size = 0, parameter_size = 0;
    unsigned line = 0;
    bool ok =
        project.script_size == 0 && project.parameter_size == 0 &&
        nativeFighterTemplate_Read(project.base_kind, &source, &size) &&
        nativeFighterParameters_Import(
            project.fighter_dat, project.fighter_size, project.fighter_root,
            project.base_kind, source, size, &converted, &converted_size,
            &parameters, &parameter_size, &line) &&
        write_new(path, "fighter_parameters.fmp", parameters,
                  parameter_size) &&
        write_new(path, "behavior.fms", converted, converted_size);
    free(source);
    free(converted);
    free(parameters);
    nativeFighterProject_Close(&project);
    return ok;
}
