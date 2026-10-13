#ifndef MELEE_MN_MNCHARSEL_ASSETS_H
#define MELEE_MN_MNCHARSEL_ASSETS_H

#include <melee/sc/types.h>
#include <sysdolphin/baselib/dat_import.h>

typedef struct MnSelectChrModels {
    StaticModelDesc background;
    StaticModelDesc hand;
    StaticModelDesc token;
    StaticModelDesc menu;
    StaticModelDesc press_start;
    StaticModelDesc debug_camera;
    StaticModelDesc regend_menu;
    StaticModelDesc regend_options;
    StaticModelDesc door;
} MnSelectChrModels;

typedef struct MnSelectChrDataTable {
    HSD_CObjDesc* cam;
    HSD_LightDesc* light0;
    HSD_LightDesc* light1;
    HSD_FogDesc* fog;
    MnSelectChrModels models;
} MnSelectChrDataTable;

MnSelectChrDataTable* mnCharSelAssets_Decode(HSD_DatImport* import,
                                             u32 root_offset);
MnSelectChrDataTable* mnCharSelAssets_Load(const char* filename);

#endif
