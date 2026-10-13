#include "native_fighter_cpu_tool.h"
#include <melee/ft/native_fighter_project.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <string.h>

int nativeFighterCpuTool_Import(const char* path, const char* iso,
                               const char* stock_name)
{
    NativeFighterProjectInfo info;
    if (!nativeFighterProject_ReadInfo(path, &info)) return 1;
    FighterKind kind = Ft_Kind_None;
    for (FighterKind candidate = 0; candidate < Ft_Kind_StockCount; ++candidate) {
        const char* name = nativeFighterProject_StockBaseName(candidate);
        if (name != NULL && _stricmp(name, stock_name) == 0) { kind = candidate; break; }
    }
    if (kind == Ft_Kind_None || !aurora_dvd_open(iso)) return 1;
    bool ok = nativeFighterProject_ImportCpuFromDisc(path, kind);
    aurora_dvd_close();
    if (!ok) { fputs("Could not import CPU tuning.\n", stderr); return 1; }
    printf("Imported independent CPU tuning from %s into %s.\n", stock_name, path);
    return 0;
}
