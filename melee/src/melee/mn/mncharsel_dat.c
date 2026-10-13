#include "mncharsel_assets.h"

#include <sysdolphin/baselib/dat_import_internal.h>

enum {
    CSS_SOURCE_MODEL_COUNT = 9,
    CSS_SOURCE_MODEL_SIZE = 16,
    CSS_SOURCE_TABLE_SIZE = 16 + CSS_SOURCE_MODEL_COUNT * CSS_SOURCE_MODEL_SIZE,
};

static void* import_graph_reference(HSD_DatImport* import, u32 slot,
                                    HSD_DatImportType type)
{
    u32 offset;
    const char* external;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, &offset, &external);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        import->status = kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                       : HSD_DAT_RELOCATION;
        import->error_offset = slot;
        import->external_symbol = external;
        return NULL;
    }
    return HSD_DatImportGraph(import, offset, type);
}

static void decode_model(HSD_DatImport* import, u32 offset,
                         StaticModelDesc* model)
{
    model->joint = import_graph_reference(import, offset, DAT_JOINT);
    model->animjoint =
        import_graph_reference(import, offset + 4, DAT_ANIMJOINT);
    model->matanim_joint =
        import_graph_reference(import, offset + 8, DAT_MATANIM_JOINT);
    model->shapeanim_joint =
        import_graph_reference(import, offset + 12, DAT_SHAPE_JOINT);
}

MnSelectChrDataTable* mnCharSelAssets_Decode(HSD_DatImport* import,
                                             u32 root_offset)
{
    MnSelectChrDataTable* table;
    StaticModelDesc* models;
    u32 i;

    if (import == NULL || import->status != HSD_DAT_OK ||
        HSD_DatBytes(import->dat, root_offset, CSS_SOURCE_TABLE_SIZE) == NULL)
    {
        if (import != NULL) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = root_offset;
        }
        return NULL;
    }

    table = HSD_DatImportAlloc(import, sizeof(*table));
    if (table == NULL) {
        import->error_offset = root_offset;
        return NULL;
    }
    table->cam = import_graph_reference(import, root_offset, DAT_CAMERA);
    table->light0 = import_graph_reference(import, root_offset + 4, DAT_LIGHT);
    table->light1 = import_graph_reference(import, root_offset + 8, DAT_LIGHT);
    table->fog = import_graph_reference(import, root_offset + 12, DAT_FOG);

    models = &table->models.background;
    for (i = 0; i < CSS_SOURCE_MODEL_COUNT; ++i) {
        decode_model(import, root_offset + 16 + i * CSS_SOURCE_MODEL_SIZE,
                     &models[i]);
    }
    return import->status == HSD_DAT_OK ? table : NULL;
}
