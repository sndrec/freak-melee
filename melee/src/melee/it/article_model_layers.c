#include "article_model_layers.h"
#include "types.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/jobj.h>
#include <string.h>

static bool fail(HSD_DatImport* import, HSD_DatResult status, u32 offset)
{
    import->status = status;
    import->error_offset = offset;
    return false;
}

ArticleModelLayers* articleModelLayers_Import(HSD_DatImport* import,
    u32 reference_slot, const HSD_Joint* model_root)
{
    u32 source;
    const char* external;
    if (import == NULL || import->status != HSD_DAT_OK) return NULL;
    if (HSD_DatResolve(import->dat, reference_slot, &source, &external) != HSD_DAT_REF_LOCAL)
    {
        fail(import, HSD_DAT_RELOCATION, reference_slot);
        return NULL;
    }
    if (HSD_DatImportNextTarget(import, source) - source != 16 || model_root == NULL)
    {
        fail(import, HSD_DAT_DESCRIPTOR, source);
        return NULL;
    }
    // Match the actual model's depth-first dynamic-bone ordinals. Bounded work
    // also rejects cyclic or oversized graphs; descriptors shared by separate
    // branches are visited at each actual model occurrence, not deduplicated.
    struct { const HSD_Joint* joint; bool parent; } pending[ARTICLE_MODEL_LAYER_JOINT_LIMIT];
    bool parents[ARTICLE_MODEL_LAYER_JOINT_LIMIT];
    unsigned pending_count = 1, joint_count = 0;
    pending[0].joint = model_root;
    pending[0].parent = false;
    while (pending_count != 0) {
        --pending_count;
        const HSD_Joint* joint = pending[pending_count].joint;
        bool parent = pending[pending_count].parent;
        if (joint_count == ARTICLE_MODEL_LAYER_JOINT_LIMIT) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
            return NULL;
        }
        parents[joint_count++] = parent;
        unsigned added = (joint->next != NULL) + (joint->child != NULL);
        if (added > ARTICLE_MODEL_LAYER_JOINT_LIMIT - pending_count) {
            fail(import, HSD_DAT_DESCRIPTOR, source);
            return NULL;
        }
        if (joint->next != NULL) {
            pending[pending_count].joint = joint->next;
            pending[pending_count++].parent = parent;
        }
        if (joint->child != NULL) {
            pending[pending_count].joint = joint->child;
            pending[pending_count++].parent = true;
        }
    }
    ArticleModelLayers* layers = HSD_DatImportAlloc(import, sizeof(*layers));
    if (layers == NULL) return NULL;
    u16* counts[2] = { &layers->normal_count, &layers->outline_count };
    const u8** indices[2] = { &layers->normal_indices, &layers->outline_indices };
    for (unsigned list = 0; list < 2; ++list) {
        u32 at = source + list * 8, index_source;
        if (!HSD_DatReadU16(import->dat, at, counts[list]) ||
            *counts[list] > ARTICLE_MODEL_LAYER_INDEX_LIMIT) {
            fail(import, HSD_DAT_DESCRIPTOR, at);
            return NULL;
        }
        HSD_DatReferenceKind kind = HSD_DatResolve(import->dat, at + 4, &index_source, &external);
        if (*counts[list] == 0) {
            if (kind != HSD_DAT_REF_NULL) {
                fail(import, HSD_DAT_RELOCATION, at + 4);
                return NULL;
            }
            continue;
        }
        const u8* bytes = kind == HSD_DAT_REF_LOCAL
            ? HSD_DatBytes(import->dat, index_source, *counts[list]) : NULL;
        if (bytes == NULL) {
            fail(import, HSD_DAT_RELOCATION, at + 4);
            return NULL;
        }
        for (unsigned i = 0; i < *counts[list]; ++i) {
            if (bytes[i] >= joint_count || !parents[bytes[i]]) {
                fail(import, HSD_DAT_DESCRIPTOR, index_source + i);
                return NULL;
            }
        }
        u8* owned = HSD_DatImportAlloc(import, *counts[list]);
        if (owned == NULL) return NULL;
        memcpy(owned, bytes, *counts[list]);
        *indices[list] = owned;
    }
    return layers;
}

bool articleModelLayers_ValidateItem(const Item* item)
{
    if (item == NULL || item->entity == NULL || item->entity->hsd_obj == NULL ||
        item->xC4_article_data == NULL || item->xC4_article_data->model_layers == NULL)
        return false;
    const ArticleModelLayers* layers = item->xC4_article_data->model_layers;
    if (layers->normal_count > ARTICLE_MODEL_LAYER_INDEX_LIMIT ||
        layers->outline_count > ARTICLE_MODEL_LAYER_INDEX_LIMIT ||
        (layers->normal_count == 0) != (layers->normal_indices == NULL) ||
        (layers->outline_count == 0) != (layers->outline_indices == NULL))
        return false;
    HSD_JObj* nodes[ARTICLE_MODEL_LAYER_JOINT_LIMIT];
    HSD_JObj* pending[ARTICLE_MODEL_LAYER_JOINT_LIMIT];
    unsigned pending_count = 1, joint_count = 0;
    pending[0] = item->entity->hsd_obj;
    while (pending_count != 0) {
        HSD_JObj* joint = pending[--pending_count];
        if (joint_count == ARTICLE_MODEL_LAYER_JOINT_LIMIT) return false;
        nodes[joint_count++] = joint;
        unsigned added = (joint->next != NULL) + (joint->child != NULL);
        if (added > ARTICLE_MODEL_LAYER_JOINT_LIMIT - pending_count) return false;
        if (joint->next != NULL) pending[pending_count++] = joint->next;
        if (joint->child != NULL) pending[pending_count++] = joint->child;
    }
    const u16 counts[2] = { layers->normal_count, layers->outline_count };
    const u8* indices[2] = { layers->normal_indices, layers->outline_indices };
    for (unsigned list = 0; list < 2; ++list)
        for (unsigned i = 0; i < counts[list]; ++i) {
            unsigned index = indices[list][i];
            if (index >= joint_count || item->xBBC_dynamicBoneTable == NULL ||
                item->xBBC_dynamicBoneTable->bones[index] != nodes[index] ||
                nodes[index]->parent == NULL)
                return false;
        }
    return true;
}

bool articleModelLayers_Bind(Item_GObj* item_gobj, bool enabled)
{
    Item* item = item_gobj != NULL ? item_gobj->user_data : NULL;
    if (item == NULL) return false;
    item->model_layers = NULL;
    item->xDCF_flag.b3 = false;
    if (!enabled) return true;
    if (item->entity != item_gobj || !articleModelLayers_ValidateItem(item))
        return false;
    item->model_layers = item->xC4_article_data->model_layers;
    item->xDCF_flag.b3 = true;
    return true;
}
