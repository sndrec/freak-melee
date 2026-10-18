#ifndef MELEE_MN_ASSETS_H
#define MELEE_MN_ASSETS_H
#include <sysdolphin/baselib/dat_import.h>

// One archive per menu scene. Runtime objects must be released before Unload;
// the scene manager can instead reclaim the whole scene heap on exit.
typedef struct MnMenuAssets {
    HSD_Dat dat;
    HSD_DatImport import;
    void** roots;
    void* bytes;
    void* arena;
} MnMenuAssets;
extern MnMenuAssets main_menu_assets;
void mnAssets_Load(MnMenuAssets* assets);
void mnAssets_Unload(MnMenuAssets* assets);
void* mnAssets_Get(MnMenuAssets* assets, const char* name);
#endif
