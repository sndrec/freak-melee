#ifndef MELEE_GM_NATIVE_SEMANTIC_STATE_H
#define MELEE_GM_NATIVE_SEMANTIC_STATE_H

#ifdef MELEE_NATIVE

#include <stdint.h>

#include <melee/cm/forward.h>
#include <melee/ft/forward.h>
#include <melee/gm/forward.h>
#include <melee/gr/forward.h>
#include <melee/it/forward.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/forward.h>

enum {
    NATIVE_SEMANTIC_FIGHTER_CAPACITY = 16,
    NATIVE_SEMANTIC_ITEM_CAPACITY = 512,
    NATIVE_SEMANTIC_GROUND_CAPACITY = 256,
    NATIVE_SEMANTIC_ITEM_SCALAR_COUNT = 17,
};

typedef struct NativeSemanticObjectMap {
    const HSD_GObj* fighters[NATIVE_SEMANTIC_FIGHTER_CAPACITY];
    const HSD_GObj* items[NATIVE_SEMANTIC_ITEM_CAPACITY];
    const HSD_GObj* grounds[NATIVE_SEMANTIC_GROUND_CAPACITY];
    uint32_t fighter_count;
    uint32_t item_count;
    uint32_t ground_count;
    bool overflowed;
} NativeSemanticObjectMap;

typedef struct NativeSemanticItemTrace {
    uint64_t identity;
    uint64_t motion;
    uint64_t animation;
    uint64_t hitbox[4];
    uint32_t hitbox_state[4];
    uint32_t hurtbox_count;
    uint32_t hurtbox_state[2];
    uint64_t hurtbox_shape[2];
    uint32_t hurtbox_skip[2];
    uint64_t hurtbox_position[2];
    uint64_t hurtbox[2];
    uint64_t hurtboxes;
    uint64_t hitboxes;
    uint32_t scalar[NATIVE_SEMANTIC_ITEM_SCALAR_COUNT];
    uint64_t damage;
    uint64_t commands;
} NativeSemanticItemTrace;

uint64_t nativeSemanticState_Begin(void);
uint64_t nativeSemanticState_AddU32(uint64_t hash, uint32_t value);
uint64_t nativeSemanticState_AddController(uint64_t hash,
                                           const VsSceneController* controller);
uint64_t nativeSemanticState_AddPlayer(uint64_t hash,
                                       const StaticPlayer* player);
uint64_t nativeSemanticState_AddCamera(uint64_t hash, const Camera* camera,
                                       const CmSubject* subject_tail);
uint64_t nativeSemanticState_AddFighter(uint64_t hash, const Fighter* fighter,
                                        uint32_t part_count);
uint64_t nativeSemanticState_FighterHitboxDigest(const Fighter* fighter);
uint64_t nativeSemanticState_FighterHurtboxDigest(const Fighter* fighter);
uint64_t nativeSemanticState_AddItem(uint64_t hash, const Item* item,
                                     NativeSemanticItemTrace* trace);
uint64_t nativeSemanticState_AddGround(uint64_t hash, const Ground* ground);
uint64_t nativeSemanticState_AddJointTree(uint64_t hash,
                                          const HSD_JObj* root);
uint64_t nativeSemanticState_AddJointVisuals(uint64_t hash,
                                             const HSD_JObj* root);
void nativeSemanticState_BuildObjectMap(NativeSemanticObjectMap* map,
                                        const HSD_GObj* fighter_head,
                                        const HSD_GObj* item_head,
                                        const HSD_GObj* ground_head);
uint32_t nativeSemanticState_ObjectId(const NativeSemanticObjectMap* map,
                                      const HSD_GObj* object);
uint64_t nativeSemanticState_AddObjectMap(
    uint64_t hash, const NativeSemanticObjectMap* map);
uint64_t nativeSemanticState_AddFighterRelations(
    uint64_t hash, const Fighter* fighter,
    const NativeSemanticObjectMap* map);
uint64_t nativeSemanticState_AddItemRelations(
    uint64_t hash, const Item* item, const NativeSemanticObjectMap* map);

#endif
#endif
