#include "ifall_assets.h"

#include <sysdolphin/baselib/dat_import_internal.h>

#include <string.h>

static const char* const model_roots[IfAllModel_RootCount] = {
    "DmgMrk_scene_models", "DmgNum_scene_models",
    "ScInfCnt_scene_models", "ScInfPnm_scene_models",
    "ScInfStc_scene_models", "ScInfTim_scene_models",
    "Stc_rarwmdls", "Stc_scemdls", "lupe", "tdsce",
};

static bool fail(HSD_DatImport* import, HSD_DatResult status, u32 slot)
{
    import->status = status;
    import->error_offset = slot;
    return false;
}

static HSD_DatReferenceKind reference(HSD_DatImport* import, u32 slot,
                                      u32* target)
{
    const char* external;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, target, &external);
    if (kind == HSD_DAT_REF_INVALID || kind == HSD_DAT_REF_EXTERNAL) {
        fail(import, HSD_DAT_RELOCATION, slot);
    }
    return kind;
}

static bool count_list(HSD_DatImport* import, u32 offset, u32 limit,
                       u32* count)
{
    for (u32 i = 0; i <= limit; ++i) {
        u32 target;
        HSD_DatReferenceKind kind = reference(import, offset + i * 4,
                                              &target);
        if (kind == HSD_DAT_REF_NULL) {
            *count = i;
            return true;
        }
        if (kind != HSD_DAT_REF_LOCAL) {
            return fail(import, HSD_DAT_RELOCATION, offset + i * 4);
        }
    }
    return fail(import, HSD_DAT_DESCRIPTOR, offset);
}

static void** import_graph_list(HSD_DatImport* import, u32 slot,
                                HSD_DatImportType type)
{
    u32 offset, count;
    HSD_DatReferenceKind kind = reference(import, slot, &offset);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL ||
        !count_list(import, offset, 256, &count))
    {
        return NULL;
    }
    void** list = HSD_DatImportAlloc(import, (count + 1) * sizeof(*list));
    if (list == NULL) {
        return NULL;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 target;
        if (reference(import, offset + i * 4, &target) !=
                HSD_DAT_REF_LOCAL ||
            (list[i] = HSD_DatImportGraph(import, target, type)) == NULL)
        {
            return NULL;
        }
    }
    return list;
}

static DynamicModelDesc* import_model(HSD_DatImport* import, u32 offset)
{
    DynamicModelDesc* model = HSD_DatImportAlloc(import, sizeof(*model));
    u32 joint;
    if (model == NULL ||
        reference(import, offset, &joint) != HSD_DAT_REF_LOCAL)
    {
        return NULL;
    }
    model->anims = (HSD_AnimJoint**) import_graph_list(
        import, offset + 4, DAT_ANIMJOINT);
    model->matanims = (HSD_MatAnimJoint**) import_graph_list(
        import, offset + 8, DAT_MATANIM_JOINT);
    model->shapeanims = (HSD_ShapeAnimJoint**) import_graph_list(
        import, offset + 12, DAT_SHAPE_JOINT);
    if (import->status != HSD_DAT_OK) {
        return NULL;
    }
    model->joint = HSD_DatImportGraph(import, joint, DAT_JOINT);
    return model->joint != NULL ? model : NULL;
}

static DynamicModelDesc** import_models(HSD_DatImport* import, u32 offset)
{
    u32 count;
    if (!count_list(import, offset, 32, &count)) {
        return NULL;
    }
    DynamicModelDesc** models = HSD_DatImportAlloc(
        import, (count + 1) * sizeof(*models));
    if (models == NULL) {
        return NULL;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 target;
        if (reference(import, offset + i * 4, &target) !=
                HSD_DAT_REF_LOCAL ||
            (models[i] = import_model(import, target)) == NULL)
        {
            return NULL;
        }
    }
    return models;
}

bool ifAllAssets_Import(const void* bytes, size_t size, void* arena,
                        size_t arena_size, u32 record_capacity,
                        IfAllAssets* output)
{
    u32 root, target, camera, light_list, light;
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
    for (u32 i = 0; i < IfAllModel_RootCount; ++i) {
        if (!HSD_DatFindRoot(&output->dat, model_roots[i], &root) ||
            (output->models[i] = import_models(&output->import, root)) == NULL)
        {
            if (output->import.status == HSD_DAT_OK) {
                fail(&output->import, HSD_DAT_SYMBOL, root);
            }
            return false;
        }
    }
    if (!HSD_DatFindRoot(&output->dat, "ScInfDmg_scene_data", &root)) {
        return fail(&output->import, HSD_DAT_SYMBOL, 0);
    }
    if (reference(&output->import, root, &target) != HSD_DAT_REF_LOCAL ||
        (output->scene.models = import_models(&output->import, target)) ==
            NULL ||
        reference(&output->import, root + 4, &camera) != HSD_DAT_REF_LOCAL ||
        reference(&output->import, camera, &target) != HSD_DAT_REF_LOCAL)
    {
        return false;
    }
    output->cameras[0].desc = HSD_DatImportGraph(&output->import, target,
                                                 DAT_CAMERA);
    if (output->cameras[0].desc == NULL ||
        reference(&output->import, root + 8, &light_list) !=
            HSD_DAT_REF_LOCAL ||
        reference(&output->import, light_list, &light) !=
            HSD_DAT_REF_LOCAL ||
        reference(&output->import, light, &target) !=
            HSD_DAT_REF_LOCAL)
    {
        return false;
    }
    output->light.desc = HSD_DatImportGraph(&output->import, target,
                                            DAT_LIGHT);
    if (output->light.desc == NULL) {
        return false;
    }
    output->scene.cameras = output->cameras;
    output->lights[0] = &output->light;
    output->scene.lights = output->lights;
    return true;
}
