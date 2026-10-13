#include <melee/ft/native_fighter_module.h>

#include <stdlib.h>

__declspec(dllexport) uint64_t __cdecl
FreakMelee_GetFighterModuleLayout(void)
{
    return nativeFighterModule_LayoutFingerprint() ^ UINT64_C(1);
}

__declspec(dllexport) const NativeFighterDefinition* __cdecl
FreakMelee_GetFighterModule(const NativeFighterDefinition* base,
                           const NativeFighterHost* host)
{
    (void) base;
    (void) host;
    abort();
}
