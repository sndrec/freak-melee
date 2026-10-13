#ifndef MELEE_NATIVE_ARTICLE_H
#define MELEE_NATIVE_ARTICLE_H

#include <melee/ft/forward.h>
#include <melee/it/forward.h>
#include <dolphin/mtx.h>
#include <stdbool.h>
#include <stdint.h>

enum NativeArticleLinkProperty {
    NativeArticleLink_Exists,
    NativeArticleLink_PositionX,
    NativeArticleLink_PositionY,
    NativeArticleLink_PositionZ,
    NativeArticleLink_NormalX,
    NativeArticleLink_NormalY,
    NativeArticleLink_NormalZ,
    NativeArticleLink_Facing,
};

Fighter_GObj* nativeArticle_Creator(Item_GObj*);

int32_t nativeArticle_Create(Fighter_GObj*, ItemKind, Vec3*, Vec3*, float, bool, uint32_t, float, bool);
int32_t nativeArticle_CreateAtPosition(Item_GObj*, ItemKind, Vec3*, float);
int32_t nativeArticle_CreateChild(Item_GObj*, ItemKind, Vec3*, float,
                                  uint32_t, uint32_t, float, unsigned);
int32_t nativeArticle_CreateHeldAtCenter(Fighter_GObj*, ItemKind, Vec3*, unsigned, float);
int32_t nativeArticle_CreateUnowned(Fighter_GObj*, ItemKind, Vec3*, Vec3*, float);
void nativeArticle_SetAnimationDescriptor(Item_GObj*, unsigned);
void nativeArticle_AddAnimationDescriptor(Item_GObj*, unsigned);
bool nativeArticle_ReadAttachmentMatrixRotation(Item_GObj*, Vec3*);
bool nativeArticle_Link(Item_GObj*, unsigned, Item_GObj*);
bool nativeArticle_SpawnLink(Item_GObj*, unsigned, unsigned, ItemKind, Vec3*, float);
void nativeArticle_Unlink(Item_GObj*, unsigned);
bool nativeArticle_LinkReciprocal(Item_GObj*, unsigned, unsigned);
float nativeArticle_LinkValue(Item_GObj*, unsigned, unsigned);
int32_t nativeArticle_LinkId(Item_GObj*, unsigned);
float nativeArticle_BoneValue(Item_GObj*, unsigned, unsigned);
float nativeArticle_LinkBoneValue(Item_GObj*, unsigned, unsigned, unsigned);
void nativeArticle_CopyLinkCollision(Item_GObj*, unsigned);
bool nativeArticle_SignalLink(Item_GObj*, unsigned, uint32_t, float);
bool nativeArticle_OverlapsOwner(Item_GObj*, unsigned);

#endif
