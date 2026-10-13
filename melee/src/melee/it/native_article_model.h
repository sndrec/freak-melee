#ifndef MELEE_IT_NATIVE_ARTICLE_MODEL_H
#define MELEE_IT_NATIVE_ARTICLE_MODEL_H

#include <melee/it/forward.h>

#ifdef MELEE_NATIVE
// Model zero is the primary root; slots 1..3 are additional instances.
// Root-relative child ordinals, separated by '/'. An empty path is the root.
// Channels: translation XYZ, rotation XYZ (radians), scale XYZ, existence,
// subtree visibility (10), raw quaternion XYZW (11..14), self visibility (15).
// Subtree visibility uses Set/ClearFlagsAll (stops at instance joints); the
// selected joint's own siblings are unaffected. Self visibility uses the
// single-joint Set/ClearFlags helpers. Both reads inspect the local hidden flag;
// neither reports effective visibility through ancestors. Existence is read-only.
float nativeArticleModel_Read(Item_GObj*, unsigned model, const char* path,
                              unsigned channel);
void nativeArticleModel_Write(Item_GObj*, unsigned model, const char* path,
                               unsigned channel, float value);
bool nativeArticleModel_Clone(Item_GObj*, unsigned model);
bool nativeArticleModel_Load(Item_GObj*, unsigned model, unsigned name_hash);
bool nativeArticleModel_AddAnimation(Item_GObj*, unsigned model, const char* path,
                                     unsigned name_hash);
void nativeArticleModel_RequestAnimation(Item_GObj*, unsigned model,
                                          const char* path, float frame);
void nativeArticleModel_AdvanceAnimation(Item_GObj*, unsigned model, const char* path);
void nativeArticleModel_RemoveAnimation(Item_GObj*, unsigned model, const char* path);
void nativeArticleModel_SetAnimationRate(Item_GObj*, unsigned model, const char*, float);
bool nativeArticleModel_ReadMatrixRotation(Item_GObj*, unsigned model, const char*, Vec3*);
bool nativeArticleModel_ReadWorldPosition(Item_GObj*, unsigned model, const char*, Vec3*);
void nativeArticleModel_SetTransform(Item_GObj*, unsigned model, const char* path,
                                     const Vec3*, const Quaternion*, const Vec3*,
                                     bool force_dirty);
void nativeArticleModel_Destroy(Item_GObj*, unsigned model);
void nativeArticleModel_SpawnEffect(Item_GObj*, int effect, const char* path);
void nativeArticleModel_SpawnAsyncEffect(Item_GObj*, int effect, const char* path,
                                         unsigned mode);
void nativeArticleModel_SpawnEffectAt(Item_GObj*, int effect, const char* path,
                                     const Vec3* offset);
float nativeArticleModel_ReadDynamic(Item_GObj*, unsigned bone, unsigned channel);
void nativeArticleModel_WriteDynamic(Item_GObj*, unsigned bone, unsigned channel, float);
#endif

#endif
