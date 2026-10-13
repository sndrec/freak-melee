#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_events.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/types.h>
#include <melee/lb/native_disc_archive.h>
#include <aurora/dvd.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t receive_hash(void)
{
    uint32_t hash = 2166136261u;
    for (const unsigned char* c = (const unsigned char*) "receive"; *c; ++c)
        hash = (hash ^ *c) * 16777619u;
    return hash;
}

static int32_t received(const NativeFighterDefinition* definition, Fighter* fighter)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i) {
        const NativeFighterStateField* field = &definition->state_fields[i];
        if (strcmp(field->name, "received") == 0) {
            assert(field->type == NativeFighterState_Int && field->count == 1);
            int32_t value;
            memcpy(&value, fighter->native_module_state + field->offset, sizeof(value));
            return value;
        }
    }
    abort();
}

int main(int argc, char** argv)
{
    if (argc != 5 || !aurora_dvd_open(argv[1])) return 2;
    enum { ARENA_SIZE = 16 * 1024 * 1024 };
    void* arena = _aligned_malloc(ARENA_SIZE, 32);
    void* common = NULL;
    size_t size = 0;
    static FtCommonAssets common_assets;
    assert(arena && nativeDiscArchive_Read("PlCo.dat", &common, &size));
    assert(ftCommonAssets_Import(common, size, arena, ARENA_SIZE, 32768, &common_assets));
    aurora_dvd_close();
    ftPartsTable = common_assets.parts;
    Fighter_804D6540 = common_assets.omissions;
    Fighter_804D64FC = &common_assets.cpu_data.descriptor;
    const NativeFighterDefinition* definitions[2];
    for (unsigned i = 0; i < 2; ++i) {
        NativeContentId id;
        assert(nativeContentStore_IdFromHex(argv[3 + i], &id));
        assert(nativeFighterRuntime_Load(&id, NULL, ChKind_CustomFirst + i,
            Ft_Kind_CustomFirst + i, argv[2], false, false));
        definitions[i] = nativeFighterRuntime_Definition(Ft_Kind_CustomFirst + i);
        assert(definitions[i] && definitions[i]->callback_export_count == 1);
    }
    Fighter fighters[2] = { { .kind = Ft_Kind_CustomFirst, .x8_spawnNum = (int32_t) 0xFEDCBA98u },
                            { .kind = Ft_Kind_CustomFirst + 1, .x8_spawnNum = 16777217 } };
    HSD_GObj objects[2] = { { .user_data = &fighters[0] }, { .user_data = &fighters[1] } };
    objects[0].next = &objects[1];
    HSD_GObj* heads[32] = { 0 };
    HSD_GObj** previous = HSD_GObjPLinkHead;
    heads[HSD_GOBJ_PLINK_FIGHTER] = &objects[0];
    HSD_GObjPLinkHead = heads;
    assert(nativeFighterEvent_Call(0xFEDCBA98u, receive_hash()));
    assert(nativeFighterEvent_Call(16777217u, receive_hash()));
    assert(received(definitions[0], &fighters[0]) == 17);
    assert(received(definitions[1], &fighters[1]) == 29);
    HMODULE library;
    assert(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR) definitions[0]->callback_exports[0].callback, &library));
    const NativeFighterModule* active = nativeFighterRuntime_ActiveModule(library);
    assert(active && active->parameters_bound && active->parameter_count == 1);
    char path[4096];
    assert(GetModuleFileNameA(library, path, sizeof(path)) != 0);
    const NativeFighterDefinition* base = active->base;
    NativeFighterDefinition other_base = *base;
    NativeFighterModule rejected = { 0 };
    assert(!nativeFighterModule_Open(path, &other_base, &rejected));
    assert(!rejected.library && !rejected.definition);
    NativeFighterModule same = { 0 };
    assert(nativeFighterModule_Open(path, base, &same));
    assert(same.definition == active->definition);
    NativeFighterParameters parameters;
    assert(nativeFighterParameters_Parse("int marker = 17\n", 16, &parameters, NULL));
    assert(nativeFighterModule_SetParameters(&same, &parameters));
    parameters.fields[0].bits = 99;
    assert(!nativeFighterModule_SetParameters(&same, &parameters));
    assert(nativeFighterEvent_Call(0xFEDCBA98u, receive_hash()));
    assert(received(definitions[0], &fighters[0]) == 17);
    assert(received(definitions[1], &fighters[1]) == 29);
    nativeFighterModule_Close(&same);
    HSD_GObjPLinkHead = previous;
    nativeFighterRuntime_Clear();
    assert(nativeFighterRuntime_ActiveModule(library) == NULL);
    // The same image may bind another base after the old scene releases it.
    assert(nativeFighterModule_Open(path, &other_base, &same));
    nativeFighterModule_Close(&same);
    _aligned_free(common);
    _aligned_free(arena);
    puts("Two authored recipient packages preserve offsets, values and active DLL binding");
    return 0;
}
