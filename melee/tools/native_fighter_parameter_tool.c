#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "native_fighter_parameter_tool.h"
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_behavior_source.h>

#include <stdio.h>
#include <stdlib.h>

int nativeFighterParameterTool_Import(const char* project_path,
                                    const char* source_path,
                                    const char* output_path)
{
    NativeFighterProject project;
    if (!nativeFighterProject_Open(project_path, &project)) return 1;
    FILE* source_file = fopen(source_path, "rb");
    long source_size = -1;
    bool ok = source_file != NULL && fseek(source_file, 0, SEEK_END) == 0 &&
        (source_size = ftell(source_file)) > 0 &&
        source_size <= NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY &&
        fseek(source_file, 0, SEEK_SET) == 0;
    char* source = ok ? malloc((size_t) source_size) : NULL;
    ok = source != NULL && fread(source, 1, (size_t) source_size, source_file)
        == (size_t) source_size;
    if (source_file != NULL) ok = fclose(source_file) == 0 && ok;
    char *script = NULL, *parameters = NULL;
    size_t script_size = 0, parameter_size = 0;
    unsigned error_line = 0;
    char parameter_path[512];
    int length = snprintf(parameter_path, sizeof(parameter_path),
        "%s\\fighter_parameters.fmp", project_path);
    ok = ok && length > 0 && length < sizeof(parameter_path) &&
        nativeFighterParameters_Import(project.fighter_dat,
            project.fighter_size, project.fighter_root, project.base_kind,
            source, (size_t) source_size, &script, &script_size,
            &parameters, &parameter_size, &error_line);
    HANDLE values = ok ? CreateFileA(parameter_path, GENERIC_WRITE, 0, NULL,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL) : INVALID_HANDLE_VALUE;
    bool values_created = values != INVALID_HANDLE_VALUE;
    DWORD written;
    ok = values_created && WriteFile(values, parameters,
        (DWORD) parameter_size, &written, NULL) && written == parameter_size &&
        FlushFileBuffers(values);
    if (values_created) ok = CloseHandle(values) && ok;
    HANDLE converted = ok ? CreateFileA(output_path, GENERIC_WRITE, 0, NULL,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL) : INVALID_HANDLE_VALUE;
    bool converted_created = converted != INVALID_HANDLE_VALUE;
    ok = converted_created && WriteFile(converted, script,
        (DWORD) script_size, &written, NULL) && written == script_size &&
        FlushFileBuffers(converted);
    if (converted_created) ok = CloseHandle(converted) && ok;
    if (!ok) {
        if (values_created) DeleteFileA(parameter_path);
        if (converted_created) DeleteFileA(output_path);
        fprintf(stderr, "Could not import parameters at source line %u. "
                "Parameter and output files must not already exist.\n",
                error_line);
    } else printf("Copied stock values to %s; converted source: %s\n",
                   parameter_path, output_path);
    free(source);
    free(script);
    free(parameters);
    nativeFighterProject_Close(&project);
    return ok ? 0 : 1;
}
