#ifndef MELEE_IT_NATIVE_ARTICLE_RESOURCE_H
#define MELEE_IT_NATIVE_ARTICLE_RESOURCE_H

#include <melee/it/forward.h>
#include <dolphin/mtx.h>
#include <stdbool.h>
#include <stdint.h>
#include <melee/ft/forward.h>
#ifdef MELEE_NATIVE
#include <melee/gm/native_match_pool.h>
#endif

// Engine resource limits, not fighter selectors. Retail census maximum is the
// doubled aerial resource count 90; excess/custom requests fail without clamping.
enum { NATIVE_ARTICLE_RESOURCE_COLLECTIONS = 16,
       NATIVE_ARTICLE_RESOURCE_NODES = 90 };

// Pure recipe validation against the final owned rig. Zero capacity means absent.
bool nativeArticleResource_ValidateRecipe(unsigned capacity, int reserved_part,
                                        unsigned actual_part_count);
// Pure lookup of an exact live node owning this Fighter's reserved slot.
// False preserves output; teardown revokes its matching hitbox borrowers.
bool nativeArticleResource_ResolveBoundPart(Fighter_GObj*,unsigned part,HSD_JObj**);
// Capacity/reservation come only from the exact immutable ItemProgram.
bool nativeArticleResource_Begin(Item_GObj*, unsigned count);
// Constructor data is supplied before the engine initializes collision history.
// Later position writes move an existing node; they do not reinitialize its ECB.
typedef struct NativeArticleResourceInit {
    unsigned attachment_part;
    float radius;
    unsigned collision_category;
    Vec3 position;
} NativeArticleResourceInit;
bool nativeArticleResource_CreateNode(Item_GObj*, unsigned index,
                                      const NativeArticleResourceInit*);
bool nativeArticleResource_LoadModel(Item_GObj*, unsigned index, uint32_t root_ordinal);
bool nativeArticleResource_SetDraw(Item_GObj*, unsigned index, bool enabled);
unsigned nativeArticleResource_Count(Item_GObj*);
bool nativeArticleResource_Exists(Item_GObj*, unsigned index);
bool nativeArticleResource_BindExternalPart(Item_GObj*, unsigned index,
                                           unsigned declared_external_part);
bool nativeArticleResource_SetCaptureAnchor(Item_GObj*, unsigned index);
void nativeArticleResource_Release(Item_GObj*);
// Teardown keys are retained identities, not live-object lookups. Call creator
// cleanup before their rig/metadata is retired.
void nativeArticleResource_CancelGeneration(Item_GObj*, uint32_t creation_id);
void nativeArticleResource_CancelCreator(Fighter_GObj*, uint32_t creation_id);
#ifdef MELEE_NATIVE
void nativeArticleResource_CancelConstructingAttachment(Fighter_GObj*,
    NativeMatchPoolReference object_reference,
    NativeMatchPoolReference fighter_reference);
#endif
void nativeArticleResource_Clear(void);

// The generic HSD component operates on actual resource models, never on fake
// Item contexts. Joint paths/channels retain nativeModelJoint semantics.
float nativeArticleResource_ReadJoint(Item_GObj*, unsigned index,
                                      const uint8_t* path, unsigned depth, unsigned channel);
bool nativeArticleResource_WriteJoint(Item_GObj*, unsigned index,
                                      const uint8_t* path, unsigned depth, unsigned channel,
                                      float value);
bool nativeArticleResource_SetWorldMatrix(Item_GObj*, unsigned index,
                                         const Mtx matrix);

// Named typed channels. Positions refer to actual node/collision storage;
// CollisionPrevious is CollData.last_pos, not CollData.prev_pos.
typedef enum NativeArticleResourceFloat {
    NativeArticleResource_PositionX, NativeArticleResource_PositionY,
    NativeArticleResource_PositionZ,
    NativeArticleResource_VelocityX, NativeArticleResource_VelocityY,
    NativeArticleResource_VelocityZ,
    NativeArticleResource_CollisionPositionX, NativeArticleResource_CollisionPositionY,
    NativeArticleResource_CollisionPositionZ,
    NativeArticleResource_CollisionPreviousX, NativeArticleResource_CollisionPreviousY,
    NativeArticleResource_CollisionPreviousZ,
    NativeArticleResource_FloorNormalX, NativeArticleResource_FloorNormalY,
    NativeArticleResource_FloorNormalZ,
    NativeArticleResource_LeftWallNormalX, NativeArticleResource_LeftWallNormalY,
    NativeArticleResource_LeftWallNormalZ,
    NativeArticleResource_RightWallNormalX, NativeArticleResource_RightWallNormalY,
    NativeArticleResource_RightWallNormalZ,
    NativeArticleResource_CeilingNormalX, NativeArticleResource_CeilingNormalY,
    NativeArticleResource_CeilingNormalZ,
    NativeArticleResource_FloatCount,
} NativeArticleResourceFloat;

typedef enum NativeArticleResourceInt {
    NativeArticleResource_SurfaceLine,
    NativeArticleResource_EnvironmentFlags,
    NativeArticleResource_FloorLine,
    NativeArticleResource_LeftWallLine,
    NativeArticleResource_RightWallLine,
    NativeArticleResource_CeilingLine,
    NativeArticleResource_IntCount,
} NativeArticleResourceInt;

float nativeArticleResource_ReadFloat(Item_GObj*, unsigned index,
                                      NativeArticleResourceFloat);
bool nativeArticleResource_WriteFloat(Item_GObj*, unsigned index,
                                      NativeArticleResourceFloat, float);
int32_t nativeArticleResource_ReadInt(Item_GObj*, unsigned index,
                                      NativeArticleResourceInt);
bool nativeArticleResource_WriteInt(Item_GObj*, unsigned index,
                                    NativeArticleResourceInt, int32_t);
// Contact callbacks may retire the collection. Sweeps keep its collision
// storage until the outermost call returns; retiring sets are unavailable.
bool nativeArticleResource_SweepStage(Item_GObj*, unsigned index);
// Swept current/last point against any stage surface. This does not load the
// node's ECB source; authored code supplies collision history and response.
bool nativeArticleResource_SweepPoint(Item_GObj*, unsigned index);
// Dispatch with the actual resource GObj. The selected effect must consume
// position or position+facing respectively; only audited independent-generator
// payloads are supported (see article_resources.md). No spawn-result pointer
// is exposed; mismatched/unknown IDs are rejected before varargs dispatch.
// False rejects invalid ownership/index/nonfinite input; true means dispatched,
// not necessarily that effect allocation succeeded. Cleanup is deferred during
// synchronous dispatch, just as during stage sweeps.
bool nativeArticleResource_SpawnEffectAtPosition(Item_GObj*, unsigned index,
                                                int32_t effect, const Vec3* position);
bool nativeArticleResource_SpawnEffectFacingAtPosition(Item_GObj*, unsigned index,
                                                      int32_t effect, const Vec3* position,
                                                      float facing);
bool nativeArticleResource_FollowFloor(Item_GObj*, unsigned index);
bool nativeArticleResource_ReadLineSpeed(Item_GObj*, unsigned index, int line,
                                         Vec3* output);
bool nativeArticleResource_ReadAttachmentPoint(Item_GObj*, unsigned index,
                                               const Vec3* local, Vec3* world);
bool nativeArticleResource_ReadAttachmentMatrix(Item_GObj*, unsigned index, Mtx);
// Read the model's existing cached world matrix; do not rebuild it from SRT.
bool nativeArticleResource_ReadWorldMatrix(Item_GObj*, unsigned index, Mtx);
bool nativeArticleResource_AddAnimation(Item_GObj*, unsigned index,
                                        const uint8_t* path, unsigned depth, uint32_t bundle_ordinal);
bool nativeArticleResource_RequestAnimation(Item_GObj*, unsigned index,
                                            const uint8_t* path, unsigned depth, float frame);
bool nativeArticleResource_AdvanceAnimation(Item_GObj*, unsigned index,
                                            const uint8_t* path, unsigned depth);
bool nativeArticleResource_RemoveAnimation(Item_GObj*, unsigned index,
                                           const uint8_t* path, unsigned depth);
bool nativeArticleResource_SetAnimationRate(Item_GObj*, unsigned index,
                                            const uint8_t* path, unsigned depth, float rate);

#endif
