#include "ifcoget_assets.h"

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

bool ifCoGetAssets_Import(const void* bytes, size_t size, void* arena,
                          size_t arena_size, u32 record_capacity,
                          IfCoGetAssets* output)
{
    u32 root, model_list, model, target, anim_list, camera, light_list;
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
    if (!HSD_DatFindRoot(&output->dat, "ScInfCgt_scene_data", &root)) {
        output->import.status = HSD_DAT_SYMBOL;
        return false;
    }
    if (!resolve_local(&output->import, root, &model_list) ||
        !resolve_local(&output->import, model_list, &model) ||
        !resolve_local(&output->import, model, &target))
    {
        return false;
    }
    output->model.joint = HSD_DatImportGraph(&output->import, target,
                                             DAT_JOINT);
    if (output->model.joint == NULL ||
        !resolve_local(&output->import, model + 4, &anim_list) ||
        !resolve_local(&output->import, anim_list, &target))
    {
        return false;
    }
    output->anims[0] = HSD_DatImportGraph(&output->import, target,
                                          DAT_ANIMJOINT);
    if (output->anims[0] == NULL) {
        return false;
    }
    output->model.anims = output->anims;
    output->models[0] = &output->model;
    output->scene.models = output->models;
    if (!resolve_local(&output->import, root + 4, &camera) ||
        !resolve_local(&output->import, camera, &target))
    {
        return false;
    }
    output->cameras[0].desc = HSD_DatImportGraph(&output->import, target,
                                                 DAT_CAMERA);
    if (output->cameras[0].desc == NULL ||
        !resolve_local(&output->import, root + 8, &light_list))
    {
        return false;
    }
    output->scene.cameras = output->cameras;
    for (u32 i = 0; i < 2; ++i) {
        u32 light;
        if (!resolve_local(&output->import, light_list + i * 4, &light) ||
            !resolve_local(&output->import, light, &target))
        {
            return false;
        }
        output->light_nodes[i].desc = HSD_DatImportGraph(&output->import,
                                                         target, DAT_LIGHT);
        if (output->light_nodes[i].desc == NULL) {
            return false;
        }
        output->lights[i] = &output->light_nodes[i];
    }
    output->scene.lights = output->lights;
    return true;
}
