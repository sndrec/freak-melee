#ifndef MELEE_IT_NATIVE_ARTICLE_RESOURCE_INTERNAL_H
#define MELEE_IT_NATIVE_ARTICLE_RESOURCE_INTERNAL_H

#include "native_article_resource.h"
#include "native_item_lifetime.h"
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/it/itCharItems.h>

typedef struct ResourceNode {
    ItemLink* link;
    NativeMatchPoolReference payload, object;
    NativeMatchPoolReference graph;
    const HSD_Joint* descriptor;
    HSD_JObj* model;
    HSD_JObj* attachment_joint;
    uint32_t generation, root_index, attachment_part;
    bool bound_part, bound_capture, object_retired;
} ResourceNode;

typedef struct ResourceCollection {
    NativeItemLifetime owner;
    NativeFighterLifetime attachment;
    FighterBone* parts;
    const FighterPartsTable* parts_table;
    HSD_JObj* rig;
    uint32_t generation, part_count;
    uint32_t created;
    uint16_t count, active_operations;
    int16_t external_part;
    uint8_t rig_kind;
    bool releasing, destroying;
    ResourceNode nodes[NATIVE_ARTICLE_RESOURCE_NODES];
} ResourceCollection;

typedef struct ResourceCapture {
    ResourceCollection* collection;
    ResourceNode* node;
    NativeItemLifetime owner;
    NativeMatchPoolReference payload, object, graph;
    ItemLink* link;
    HSD_GObj* gobj;
    HSD_JObj* model;
    const HSD_Joint* descriptor;
    uint32_t generation, index, root_index;
    uint8_t object_kind;
    bool active;
} ResourceCapture;

// Actual .mstate registry shared by node and graph kernels, not a second mirror.
extern ResourceCollection nativeArticleResourceCollections[NATIVE_ARTICLE_RESOURCE_COLLECTIONS];
extern uint32_t nativeArticleResourceGeneration;
ResourceCollection* nativeArticleResource_Collection(Item_GObj*);
bool nativeArticleResource_Capture(Item_GObj*, unsigned, ResourceCapture*);
// Teardown admission does not require live owned script storage or a live Item.
// It still proves node payload/GObj/graph identities before native dereferences.
bool nativeArticleResource_CapturePhysical(ResourceCollection*, ResourceNode*, ResourceCapture*);
bool nativeArticleResource_Current(const ResourceCapture*);
bool nativeArticleResource_BeginOperation(Item_GObj*, unsigned, ResourceCapture*);
// Always balances a successful Begin; deferred release follows validation.
bool nativeArticleResource_EndOperation(ResourceCapture*);
// Logical attachment admission for new borrowing/reads, independent physical
// identity admission for teardown after actor Detach or attachment revocation.
bool nativeArticleResource_AttachmentCurrent(const ResourceCollection*);
bool nativeArticleResource_AttachmentPhysical(const ResourceCollection*);
void nativeArticleResource_DetachBorrowers(ResourceCollection*, ResourceNode*);
void nativeArticleResource_ReleaseCollection(ResourceCollection*);

#endif
