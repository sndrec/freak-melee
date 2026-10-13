#include "gmresult_assets.h"

#include <sysdolphin/baselib/dat_import_internal.h>

#include <string.h>

static HSD_DatReferenceKind reference(HSD_DatImport* import, u32 slot,
                                      u32* target)
{
    const char* external;
    HSD_DatReferenceKind kind =
        HSD_DatResolve(import->dat, slot, target, &external);
    if (kind == HSD_DAT_REF_INVALID || kind == HSD_DAT_REF_EXTERNAL) {
        import->status = HSD_DAT_RELOCATION;
        import->error_offset = slot;
    }
    return kind;
}

static bool count_list(HSD_DatImport* import, u32 offset, u32 limit,
                       u32* count)
{
    for (u32 i = 0; i <= limit; ++i) {
        u32 target;
        HSD_DatReferenceKind kind =
            reference(import, offset + i * 4, &target);
        if (kind == HSD_DAT_REF_NULL) {
            *count = i;
            return true;
        }
        if (kind != HSD_DAT_REF_LOCAL) {
            break;
        }
    }
    import->status = HSD_DAT_DESCRIPTOR;
    import->error_offset = offset;
    return false;
}

static void** import_graph_list(HSD_DatImport* import, u32 slot,
                                HSD_DatImportType type)
{
    u32 offset, count;
    HSD_DatReferenceKind kind = reference(import, slot, &offset);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL || !count_list(import, offset, 128, &count)) {
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
    u32 joint;
    DynamicModelDesc* model = HSD_DatImportAlloc(import, sizeof(*model));
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

static DynamicModelDesc** import_models(HSD_DatImport* import, u32 slot)
{
    u32 offset, count;
    if (reference(import, slot, &offset) != HSD_DAT_REF_LOCAL ||
        !count_list(import, offset, 16, &count))
    {
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

static bool import_scene(HSD_DatImport* import, const char* root_name,
                         SceneDesc* scene)
{
    u32 root, camera, light_list, target, count;
    if (!HSD_DatFindRoot(import->dat, root_name, &root)) {
        import->status = HSD_DAT_SYMBOL;
        return false;
    }
    scene->models = import_models(import, root);
    if (scene->models == NULL ||
        reference(import, root + 4, &camera) != HSD_DAT_REF_LOCAL)
    {
        return false;
    }
    scene->cameras = HSD_DatImportAlloc(import, sizeof(*scene->cameras));
    if (scene->cameras == NULL ||
        reference(import, camera, &target) != HSD_DAT_REF_LOCAL)
    {
        return false;
    }
    scene->cameras->desc = HSD_DatImportGraph(import, target, DAT_CAMERA);
    if (scene->cameras->desc == NULL || import->status != HSD_DAT_OK ||
        reference(import, camera + 4, &target) != HSD_DAT_REF_NULL ||
        reference(import, root + 8, &light_list) != HSD_DAT_REF_LOCAL ||
        !count_list(import, light_list, 16, &count))
    {
        return false;
    }
    scene->lights = HSD_DatImportAlloc(import,
                                       (count + 1) * sizeof(*scene->lights));
    if (scene->lights == NULL) {
        return false;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 record;
        if (reference(import, light_list + i * 4, &record) !=
                HSD_DAT_REF_LOCAL ||
            reference(import, record, &target) != HSD_DAT_REF_LOCAL)
        {
            return false;
        }
        struct LightList* light = HSD_DatImportAlloc(import, sizeof(*light));
        if (light == NULL) {
            return false;
        }
        light->desc = HSD_DatImportGraph(import, target, DAT_LIGHT);
        if (light->desc == NULL ||
            reference(import, record + 4, &target) != HSD_DAT_REF_NULL)
        {
            return false;
        }
        scene->lights[i] = light;
    }
    return true;
}

bool gmResultAssets_Import(const void* bytes, size_t size, void* arena,
                           size_t arena_size, u32 record_capacity,
                           GmResultAssets* output)
{
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
    return import_scene(&output->import, "pnlsce", &output->panel) &&
           import_scene(&output->import, "flmsce", &output->film);
}
