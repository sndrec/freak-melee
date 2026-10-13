#include "ftaccessory_assets.h"

#include <sysdolphin/baselib/dat_import_internal.h>
#include <stdint.h>
#include <string.h>

static bool fail(HSD_DatImport* import, HSD_DatResult status, u32 offset)
{
    import->status = status;
    import->error_offset = offset;
    return false;
}

static bool graph(HSD_DatImport* import, u32 slot, HSD_DatImportType type,
                  bool required, void** output)
{
    *output = NULL;
    if (slot == UINT32_MAX) {
        return !required || fail(import, HSD_DAT_DESCRIPTOR, slot);
    }
    if (slot & 3) return fail(import, HSD_DAT_DESCRIPTOR, slot);
    u32 offset;
    const char* external;
    HSD_DatReferenceKind reference =
        HSD_DatResolve(import->dat, slot, &offset, &external);
    if (reference == HSD_DAT_REF_NULL && !required) return true;
    if (reference != HSD_DAT_REF_LOCAL)
        return fail(import, HSD_DAT_RELOCATION, slot);
    *output = HSD_DatImportGraph(import, offset, type);
    if (*output != NULL) return true;
    if (import->status == HSD_DAT_OK)
        fail(import, HSD_DAT_DESCRIPTOR, offset);
    return false;
}

static bool animation(HSD_DatImport* import,
                      const FtAccessoryAnimationSource* source,
                      FtAccessoryAnimation* output)
{
    // Do not write through an incompatible typed pointer-to-pointer alias.
    void* joint;
    void* material;
    void* shape;
    if (!graph(import, source->joint, DAT_ANIMJOINT, false, &joint) ||
        !graph(import, source->material, DAT_MATANIM_JOINT, false, &material) ||
        !graph(import, source->shape, DAT_SHAPE_JOINT, false, &shape)) return false;
    *output = (FtAccessoryAnimation){ joint, material, shape };
    return true;
}

FtAccessoryAsset* ftAccessoryAssets_Import(HSD_DatImport* import,
                                          const FtAccessoryAssetSource* source)
{
    if (!import || import->status != HSD_DAT_OK) return NULL;
    if (!import->dat || !import->arena || import->arena_used > import->arena_size) {
        fail(import, HSD_DAT_DESCRIPTOR, UINT32_MAX);
        return NULL;
    }
    if (!source || source->variant_count > FT_ACCESSORY_ANIMATION_VARIANTS) {
        fail(import, HSD_DAT_DESCRIPTOR, source ? source->root : UINT32_MAX);
        return NULL;
    }
    FtAccessoryAsset* asset = HSD_DatImportAlloc(import, sizeof(*asset));
    if (!asset) return NULL;
    memset(asset, 0, sizeof(*asset));
    void* root;
    if (!graph(import, source->root, DAT_JOINT, true, &root) ||
        !animation(import, &source->base, &asset->base)) return NULL;
    asset->root = root;
    asset->variant_count = source->variant_count;
    for (u32 i = 0; i < source->variant_count; ++i)
        if (!animation(import, &source->variants[i], &asset->variants[i]))
            return NULL;
    return asset;
}

static bool span(const HSD_DatImport* import, const void* pointer, size_t bytes)
{
    uintptr_t start = (uintptr_t)import->arena, at = (uintptr_t)pointer;
    return pointer && import->arena && at >= start &&
        at - start <= import->arena_used && bytes <= import->arena_used - (at - start);
}

static bool typed_graph(const HSD_DatImport* import, const void* graph,
                        HSD_DatImportType type, size_t bytes)
{
    if (!graph) return true;
    if (!span(import, graph, bytes)) return false;
    for (u32 i = 0; i < import->record_count; ++i)
        if (import->records[i].object == graph && import->records[i].type == (u32)type)
            return true;
    return false;
}

static bool valid_animation(const HSD_DatImport* import,
                            const FtAccessoryAnimation* value)
{
    return typed_graph(import, value->joint, DAT_ANIMJOINT, sizeof(*value->joint)) &&
        typed_graph(import, value->material, DAT_MATANIM_JOINT, sizeof(*value->material)) &&
        typed_graph(import, value->shape, DAT_SHAPE_JOINT, sizeof(*value->shape));
}

bool ftAccessoryAssets_Validate(const HSD_DatImport* import,
                                const FtAccessoryAsset* asset)
{
    if (!import || !import->dat || import->status != HSD_DAT_OK ||
        import->arena_used > import->arena_size ||
        import->record_count > import->record_capacity ||
        !span(import, import->records, import->record_count * sizeof(*import->records)) ||
        !span(import, asset, sizeof(*asset)) ||
        (uintptr_t)asset % _Alignof(FtAccessoryAsset) || !asset->root ||
        asset->variant_count > FT_ACCESSORY_ANIMATION_VARIANTS ||
        !typed_graph(import, asset->root, DAT_JOINT, sizeof(*asset->root)) ||
        !valid_animation(import, &asset->base)) return false;
    for (u32 i = 0; i < asset->variant_count; ++i)
        if (!valid_animation(import, &asset->variants[i])) return false;
    for (u32 i = asset->variant_count; i < FT_ACCESSORY_ANIMATION_VARIANTS; ++i)
        if (asset->variants[i].joint || asset->variants[i].material ||
            asset->variants[i].shape) return false;
    return true;
}
