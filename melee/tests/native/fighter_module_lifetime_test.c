#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <melee/gm/native_rollback_pages.h>
#include <sysdolphin/baselib/gobj.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    NativeFighterModule invalid = { 0 };
    if (nativeFighterModule_Open(argv[2], NULL, &invalid) ||
        invalid.library != NULL || invalid.definition != NULL) {
        fputs("A fighter module with another layout was accepted.\n", stderr);
        nativeFighterModule_Close(&invalid);
        return 1;
    }
    for (unsigned pass = 0; pass < 2; ++pass) {
        NativeFighterModule module = { 0 };
        if (!nativeFighterModule_Open(argv[1], NULL, &module) ||
            module.definition->state_bytes != 2 ||
            module.definition->special_n == NULL ||
            module.definition->on_death == NULL) {
            fputs("Could not load the test fighter module.\n", stderr);
            return 1;
        }
        SYSTEM_INFO system_info;
        GetSystemInfo(&system_info);
        size_t page_size = system_info.dwPageSize;
        size_t region_size = (sizeof(Fighter) * 2 + page_size - 1) &
                             ~(page_size - 1);
        Fighter* fighters = VirtualAlloc(NULL, region_size,
                                         MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE);
        if (fighters == NULL) return 1;
        HSD_GObj objects[2] = { 0 };
        objects[0].user_data = &fighters[0];
        objects[1].user_data = &fighters[1];
        module.definition->special_n(&objects[0]);
        module.definition->special_n(&objects[0]);
        module.definition->special_n(&objects[1]);
        fighters[0].native_module_state[1] = 1;
        bool resource_live = fighters[0].native_module_state[1] == 1;
        bool isolated = fighters[0].native_module_state[0] == 2 &&
                        fighters[1].native_module_state[0] == 1 && resource_live;
        NativeRollbackPagedRegion region = { fighters, region_size };
        bool restored = nativeRollbackPages_Init(&region, 1, NULL, 0, 4, 8) &&
                        nativeRollbackPages_BeginFrame(10);
        if (restored) {
            module.definition->special_n(&objects[0]);
            restored = nativeRollbackPages_EndFrame() &&
                       nativeRollbackPages_BeginFrame(11);
        }
        if (restored) {
            module.definition->special_n(&objects[1]);
            module.definition->on_death(&objects[0]);
            restored = nativeRollbackPages_EndFrame() &&
                       nativeRollbackPages_ToFrame(10, 12) &&
                       fighters[0].native_module_state[0] == 2 &&
                       fighters[1].native_module_state[0] == 1 &&
                       fighters[0].native_module_state[1] == 1;
        }
        nativeRollbackPages_Shutdown();
        VirtualFree(fighters, 0, MEM_RELEASE);
        nativeFighterModule_Close(&module);
        if (!isolated || !restored || module.library != NULL ||
            module.definition != NULL) {
            fputs("Module state or lifetime check failed.\n", stderr);
            return 1;
        }
    }
    puts("Native fighter module loaded twice with isolated instance state.");
    return 0;
}
