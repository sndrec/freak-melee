#include "gmpause_assets.h"

#include <string.h>

static bool resolve_local(HSD_DatImport* import, u32 slot, u32* target)
{
    const char* external;
    if (HSD_DatResolve(import->dat, slot, target, &external) ==
        HSD_DAT_REF_LOCAL)
    {
        return true;
    }
    import->status = HSD_DAT_RELOCATION;
    import->error_offset = slot;
    return false;
}

bool gmPauseAssets_Import(const void* bytes, size_t size, void* arena,
                          size_t arena_size, u32 record_capacity,
                          GmPauseAssets* output)
{
    u32 root, model_list, model, joint, matanim_list, matanim;
    if (output == NULL) {
        return false;
    }
    memset(output, 0, sizeof(*output));
    if (HSD_DatOpen(&output->dat, bytes, size) != HSD_DAT_OK ||
        HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK)
    {
        return false;
    }
    if (!HSD_DatFindRoot(&output->dat, "ScGamPause_scene_data", &root)) {
        output->import.status = HSD_DAT_SYMBOL;
        return false;
    }
    if (!resolve_local(&output->import, root, &model_list) ||
        !resolve_local(&output->import, model_list, &model) ||
        !resolve_local(&output->import, model, &joint) ||
        !resolve_local(&output->import, model + 8, &matanim_list) ||
        !resolve_local(&output->import, matanim_list, &matanim))
    {
        return false;
    }
    output->model.joint = HSD_DatImportGraph(&output->import, joint,
                                             DAT_JOINT);
    if (output->model.joint == NULL) {
        return false;
    }
    output->matanims[0] = HSD_DatImportGraph(&output->import, matanim,
                                               DAT_MATANIM_JOINT);
    if (output->matanims[0] == NULL) {
        return false;
    }
    output->model.matanims = output->matanims;
    output->models[0] = &output->model;
    output->scene.models = output->models;
    return true;
}
