#include "native_replay_resources.h"
#include "native_semantic_state.h"

#include <melee/it/types.h>
#include <melee/it/itCharItems.h>
#include <melee/it/native_article_resource_internal.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <string.h>

static uint32_t bits(float value)
{
    uint32_t result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static void trace_node(uint64_t sample, unsigned item_index, unsigned index,
                       const ItemLink* link, const HSD_JObj* model)
{
    const CollData* collision = &link->coll_data;
    uint64_t pose = nativeSemanticState_Begin();
    uint64_t visual = nativeSemanticState_Begin();
    if (model != NULL) {
        pose = nativeSemanticState_AddJointTree(pose, model);
        visual = nativeSemanticState_AddJointVisuals(visual, model);
    }
    fprintf(stderr,
        "[item-resource] sample %llu I%u node=%u "
        "pos=%08x,%08x,%08x vel=%08x,%08x,%08x "
        "collision=%08x,%08x,%08x last=%08x,%08x,%08x "
        "line=%d env=%08x pose=%016llx visual=%016llx\n",
        (unsigned long long)sample, item_index, index,
        bits(link->pos.x), bits(link->pos.y), bits(link->pos.z),
        bits(link->vel.x), bits(link->vel.y), bits(link->vel.z),
        bits(collision->cur_pos.x), bits(collision->cur_pos.y),
        bits(collision->cur_pos.z), bits(collision->last_pos.x),
        bits(collision->last_pos.y), bits(collision->last_pos.z),
        (int)link->x1CC, (unsigned)collision->env_flags,
        (unsigned long long)pose, (unsigned long long)visual);
}

void nativeReplayResources_Trace(uint64_t sample, unsigned item_index,
                                 Item_GObj* object)
{
    const Item* item = object->user_data;
    ResourceCollection* owned = nativeArticleResource_Collection(object);
    if (owned != NULL) {
        fprintf(stderr, "[item-resources] sample %llu I%u count=%u\n",
                (unsigned long long)sample, item_index, owned->created);
        for (unsigned index = 0; index < owned->created; ++index) {
            ResourceCapture node;
            if (!nativeArticleResource_Capture(object, index, &node)) {
                fprintf(stderr, "[item-resource] sample %llu I%u node=%u invalid\n",
                        (unsigned long long)sample, item_index, index);
                continue;
            }
            trace_node(sample, item_index, index, node.link, node.model);
        }
        return;
    }
    // Authored payloads do not contain the stock linked-list union. Only the
    // original stock implementation supplies these reference graphs.
    if (item->native_script_owner != 0) return;
    const ItemLink* first;
    switch (item->kind) {
    case It_Kind_Samus_GBeam: first = item->xDD4_itemVar.samusgrapple.x4; break;
    case It_Kind_Link_HShot:
    case It_Kind_CLink_HShot: first = item->xDD4_itemVar.linkhookshot.x4; break;
    case It_Kind_Ness_Yoyo: first = item->xDD4_itemVar.nessyoyo.xC; break;
    case It_Kind_Seak_Chain: first = item->xDD4_itemVar.seakchain.x4; break;
    default: return;
    }
    const ItemLink* nodes[NATIVE_ARTICLE_RESOURCE_NODES];
    unsigned count = 0;
    for (const ItemLink* link = first; link != NULL; link = link->prev) {
        if (count == NATIVE_ARTICLE_RESOURCE_NODES || link->gobj == NULL ||
            link->gobj->user_data != link) {
            fprintf(stderr, "[item-resource] sample %llu I%u invalid stock chain\n",
                    (unsigned long long)sample, item_index);
            return;
        }
        nodes[count++] = link;
    }
    fprintf(stderr, "[item-resources] sample %llu I%u count=%u\n",
            (unsigned long long)sample, item_index, count);
    for (unsigned index = 0; index < count; ++index) {
        const HSD_GObj* node = nodes[index]->gobj;
        trace_node(sample, item_index, index, nodes[index],
                   node->obj_kind == HSD_GObj_JObjKind ? node->hsd_obj : NULL);
    }
}
