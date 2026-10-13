/* Test-only C oracle exporter. Ordinary source loading uses the typed frontend. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "native_fighter_script.h"
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

static bool read_file(const char* path, char** bytes, size_t* size)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) return false;
    bool ok = fseek(file, 0, SEEK_END) == 0;
    long length = ok ? ftell(file) : -1;
    ok = ok && length >= 0 && length <= NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY &&
         fseek(file, 0, SEEK_SET) == 0;
    char* data = ok ? malloc((size_t)length + 1) : NULL;
    ok = ok && data != NULL && fread(data, 1, (size_t)length, file) == (size_t)length;
    ok = fclose(file) == 0 && ok;
    if (!ok) { free(data);return false; }
    data[length] = 0;*bytes = data;*size = (size_t)length;return true;
}

int main(int argc, char** argv)
{
    if (argc < 4) {
        fputs("Test reference exporter: SOURCE KEY OUTPUT_C [ASSET_DIRECTORY [PARAMETERS_FMP]] [--construction ROOT FIRST COUNT]\n", stderr);
        return 1;
    }
    NativeFighterScriptConstruction construction={0};bool has_construction=false;
    const char* asset_argument=NULL;const char* parameter_argument=NULL;
    for(int i=4;i<argc;++i) {
        if(!strcmp(argv[i],"--construction")) {
            if(has_construction || i+3>=argc)return 1;
            construction.fighter_root=argv[++i];uint32_t* fields[]={&construction.motion_first,&construction.motion_count};
            for(unsigned field=0;field<2;++field) {
                const char* text=argv[++i];char* end;errno=0;unsigned long value=strtoul(text,&end,10);
                if(errno || !*text || *text=='-' || *end || value>UINT32_MAX)return 1;
                *fields[field]=(uint32_t)value;
            }
            has_construction=true;
        } else if(!asset_argument)asset_argument=argv[i];
        else if(!parameter_argument)parameter_argument=argv[i];
        else return 1;
    }
    char directory[512], parameter_path[512], temporary[512];
    if (strlen(argv[1]) >= sizeof(directory)) return 1;
    strcpy(directory, argv[1]);
    char* slash = strrchr(directory, '/');char* backslash = strrchr(directory, '\\');
    if (backslash != NULL && (slash == NULL || backslash > slash)) slash = backslash;
    if (slash != NULL) *slash = 0;else strcpy(directory, ".");
    const char* assets = asset_argument ? asset_argument : directory;
    int count = snprintf(parameter_path, sizeof(parameter_path), "%s/fighter_parameters.fmp", assets);
    if (count <= 0 || count >= sizeof(parameter_path)) return 1;
    const char* parameter_file = parameter_argument ? parameter_argument : parameter_path;
    char *source = NULL, *parameter_source = NULL;size_t source_size = 0, parameter_size = 0;
    if (!read_file(argv[1], &source, &source_size)) {
        fprintf(stderr, "Could not read reference source %s.\n", argv[1]);return 1;
    }
    DWORD attributes = GetFileAttributesA(parameter_file);
    bool ok = attributes == INVALID_FILE_ATTRIBUTES
        ? parameter_argument==NULL && GetLastError() == ERROR_FILE_NOT_FOUND
        : read_file(parameter_file, &parameter_source, &parameter_size);
    NativeFighterParameters parameters;unsigned line = 0;
    ok = ok && nativeFighterParameters_Parse(parameter_source != NULL ? parameter_source : "",
                                            parameter_size, &parameters, &line);
    if (!ok) {
        fprintf(stderr, "Reference parameters %s:%u rejected.\n", parameter_file, line);
        free(source);free(parameter_source);return 1;
    }
    count = snprintf(temporary, sizeof(temporary), "%s.reference-%lu.tmp", argv[3], (unsigned long)GetCurrentProcessId());
    FILE* output = count > 0 && count < sizeof(temporary) ? fopen(temporary, "wb") : NULL;
    NativeFighterScriptError error = {0};
    bool generated = output != NULL && nativeFighterScript_GenerateC(source, source_size,
        argv[2], assets, &parameters, has_construction?&construction:NULL, output, &error);
    bool closed = output != NULL && fclose(output) == 0;
    free(source);free(parameter_source);
    bool saved = generated && closed && MoveFileExA(temporary, argv[3],
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!saved) {
        if (output != NULL) DeleteFileA(temporary);
        fprintf(stderr, "%s:%u: %s\n", argv[1], error.line,
                error.message[0] ? error.message : "Reference output could not be saved");
        return 1;
    }
    return 0;
}
