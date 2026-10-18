#include "mnassets.h"
#include <melee/lb/lbfile.h>
#include <melee/sc/types.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/memory.h>
#include <string.h>

MnMenuAssets main_menu_assets;

void mnAssets_Load(MnMenuAssets* assets)
{
    memset(assets, 0, sizeof(*assets));
    size_t size = lbFileGetSize("MnMaAll");
    void* bytes = HSD_MemAlloc((size + 31) & ~(size_t) 31);
    assets->bytes = bytes;
    lbFile_8001668C("MnMaAll", bytes, &size);
    HSD_ASSERT(0, HSD_DatOpen(&assets->dat, bytes, size) == HSD_DAT_OK);
    // Original regional archives require fewer than 8500 graph records.
    // Allocate once at the scene boundary, including room for native pointers.
    void* arena = HSD_MemAlloc(4 * 1024 * 1024);
    assets->arena = arena;
    HSD_ASSERT(0, HSD_DatImportInit(&assets->import, &assets->dat, arena,
                                   4 * 1024 * 1024, 16384) == HSD_DAT_OK);
    assets->roots = HSD_DatImportAlloc(&assets->import, assets->dat.root_count * sizeof(void*));
    HSD_ASSERT(0, assets->roots);
}

void mnAssets_Unload(MnMenuAssets* assets)
{
    if (assets->arena) HSD_Free(assets->arena);
    if (assets->bytes) HSD_Free(assets->bytes);
    memset(assets, 0, sizeof(*assets));
}

static bool ends_with(const char* name, const char* suffix)
{
    size_t length = strlen(name), tail = strlen(suffix);
    return length >= tail && strcmp(name + length - tail, suffix) == 0;
}

void* mnAssets_Get(MnMenuAssets* assets, const char* name)
{
    u32 offset = 0, index;
    for (index = 0; index < assets->dat.root_count; ++index) {
        const char* symbol;
        HSD_ASSERT(0, HSD_DatSymbol(&assets->dat, false, index, &offset, &symbol));
        if (!strcmp(symbol, name)) break;
    }
    HSD_ASSERTREPORT(0, index < assets->dat.root_count, "Missing menu root %s", name);
    if (assets->roots[index]) return assets->roots[index];
    void* result = NULL;
    if (!strcmp(name, "ScMenMain_scene_lights")) {
        u32 count = 0, target;
        const char* external;
        HSD_DatReferenceKind kind;
        while ((kind = HSD_DatResolve(&assets->dat, offset + count * 4,
                                       &target, &external)) == HSD_DAT_REF_LOCAL) ++count;
        HSD_ASSERT(0, kind == HSD_DAT_REF_NULL);
        LightList** lights = HSD_DatImportAlloc(&assets->import, (count + 1) * sizeof(*lights));
        HSD_ASSERT(0, lights);
        for (u32 i = 0; i < count; ++i) {
            HSD_DatResolve(&assets->dat, offset + i * 4, &target, &external);
            LightList* light = HSD_DatImportAlloc(&assets->import, sizeof(*light));
            HSD_ASSERT(0, light);
            u32 desc;
            HSD_ASSERT(0, HSD_DatResolve(&assets->dat, target, &desc, &external) == HSD_DAT_REF_LOCAL);
            // Queue animations first; graph import drains all queued records.
            light->anims = HSD_DatImportList(&assets->import, target + 4, DAT_LIGHT_ANIM_TABLE);
            light->desc = HSD_DatImportGraph(&assets->import, desc, DAT_LIGHT);
            HSD_ASSERT(0, light->desc);
            lights[i] = light;
        }
        result = lights;
    } else if (!strncmp(name, "mnName", 6)) {
        u32 end = assets->dat.data_size;
        for (u32 i = 0; i < assets->dat.root_count; ++i) {
            u32 next;
            const char* symbol;
            HSD_DatSymbol(&assets->dat, false, i, &next, &symbol);
            if (next > offset && next < end) end = next;
        }
        HSD_ASSERT(0, (end - offset) % 4 == 0);
        u32 count = (end - offset) / 4;
        u8** names = HSD_DatImportAlloc(&assets->import, count * sizeof(*names));
        HSD_ASSERT(0, names);
        for (u32 i = 0; i < count; ++i) {
            u32 target;
            const char* external;
            HSD_ASSERT(0, HSD_DatResolve(&assets->dat, offset + i * 4, &target,
                &external) == HSD_DAT_REF_LOCAL);
            names[i] = (u8*) HSD_DatBytes(&assets->dat, target, 1);
            HSD_ASSERT(0, names[i]);
        }
        result = names;
    } else {
        HSD_DatImportType type = 0;
        if (ends_with(name, "_shapeanim_joint")) type = DAT_SHAPE_JOINT;
        else if (ends_with(name, "_matanim_joint")) type = DAT_MATANIM_JOINT;
        else if (ends_with(name, "_animjoint")) type = DAT_ANIMJOINT;
        else if (ends_with(name, "_joint")) type = DAT_JOINT;
        else if (ends_with(name, "_camera")) type = DAT_CAMERA;
        else if (ends_with(name, "_fog")) type = DAT_FOG;
        HSD_ASSERTREPORT(0, type, "Unknown menu root schema %s", name);
        result = HSD_DatImportGraph(&assets->import, offset, type);
    }
    HSD_ASSERTREPORT(0, result && assets->import.status == HSD_DAT_OK,
        "Menu root %s conversion failed: %d at %lx", name,
        assets->import.status, assets->import.error_offset);
    assets->roots[index] = result;
    return result;
}
