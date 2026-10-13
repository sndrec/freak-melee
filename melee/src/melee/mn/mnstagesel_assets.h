#ifndef MELEE_MN_MNSTAGESEL_ASSETS_H
#define MELEE_MN_MNSTAGESEL_ASSETS_H

#include <melee/sc/types.h>
#include <sysdolphin/baselib/dat_import.h>

typedef struct MnSelectStageModels {
    StaticModelDesc x0;
    StaticModelDesc x10;
    StaticModelDesc x20;
    StaticModelDesc x30;
    StaticModelDesc x40;
    StaticModelDesc x50;
    StaticModelDesc x60;
    StaticModelDesc x70;
    StaticModelDesc x80;
    StaticModelDesc x90;
    StaticModelDesc xA0;
    StaticModelDesc xB0;
} MnSelectStageModels;

typedef struct MnSelectStageDataTable {
    HSD_CObjDesc* cam;
    HSD_LightDesc* light0;
    HSD_LightDesc* light1;
    HSD_FogDesc* fog;
    MnSelectStageModels models;
} MnSelectStageDataTable;

MnSelectStageDataTable* mnStageSelAssets_Decode(HSD_DatImport* import,
                                                u32 root_offset);
MnSelectStageDataTable* mnStageSelAssets_Load(const char* filename);

#endif
