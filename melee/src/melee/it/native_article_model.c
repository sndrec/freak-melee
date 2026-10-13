#include "native_article_model.h"

#ifdef MELEE_NATIVE
#include "native_article_model_scene.h"
#include <melee/it/types.h>
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>

static HSD_JObj* model_root(Item_GObj* gobj, unsigned model)
{
    if (gobj == NULL || model > 3 || (model && gobj->user_data == NULL))
        return NULL;
    Item* item = gobj->user_data;
    return model ? item->native_models[model - 1] : gobj->hsd_obj;
}

bool nativeArticleModel_ReadWorldPosition(Item_GObj* gobj, unsigned model,
                                          const char* path, Vec3* position)
{
    if (position == NULL) return false;
    *position = (Vec3) { 0 };
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    if (joint == NULL) return false;
    lb_8000B1CC(joint, NULL, position);
    return true;
}

void nativeArticleModel_SpawnEffect(Item_GObj* gobj, int effect, const char* path)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, 0), path);
    if (joint != NULL && effect >= 0) efSync_Spawn(effect, gobj, joint);
}

void nativeArticleModel_SpawnAsyncEffect(Item_GObj* gobj, int effect,
                                         const char* path, unsigned mode)
{
    Item* item = gobj != NULL ? gobj->user_data : NULL;
    if (item == NULL || effect < 0 || (mode != 0 && mode != 3)) return;
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, 0), path);
    if (joint == NULL) return;
    if (mode == 3)
        efAsync_Spawn(gobj, &item->xBC0, 3, effect, joint, &item->facing_dir);
    else
        efAsync_Spawn(gobj, &item->xBC0, 0, effect, joint);
}

void nativeArticleModel_SpawnEffectAt(Item_GObj* gobj, int effect,
                                      const char* path, const Vec3* offset)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, 0), path);
    if (joint != NULL && effect >= 0 && offset != NULL &&
        isfinite(offset->x) && isfinite(offset->y) && isfinite(offset->z)) {
        Vec3 position = *offset;
        efSync_Spawn(effect, gobj, joint, &position);
    }
}

float nativeArticleModel_Read(Item_GObj* gobj, unsigned model, const char* path,
                               unsigned channel)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    return nativeModelJoint_Read(joint, channel);
}

void nativeArticleModel_Write(Item_GObj* gobj, unsigned model, const char* path,
                               unsigned channel, float value)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    nativeModelJoint_Write(joint, channel, value);
}

float nativeArticleModel_ReadDynamic(Item_GObj* gobj, unsigned bone, unsigned channel)
{
    Item* item = gobj != NULL ? gobj->user_data : NULL;
    HSD_JObj* joint = item != NULL && item->xBBC_dynamicBoneTable != NULL && bone < 100
        ? item->xBBC_dynamicBoneTable->bones[bone] : NULL;
    return nativeModelJoint_Read(joint, channel);
}

void nativeArticleModel_WriteDynamic(Item_GObj* gobj, unsigned bone,
                                     unsigned channel, float value)
{
    Item* item = gobj != NULL ? gobj->user_data : NULL;
    HSD_JObj* joint = item != NULL && item->xBBC_dynamicBoneTable != NULL && bone < 100
        ? item->xBBC_dynamicBoneTable->bones[bone] : NULL;
    nativeModelJoint_Write(joint, channel, value);
}

void nativeArticleModel_SetTransform(Item_GObj* gobj, unsigned model,
                                     const char* path, const Vec3* translation,
                                     const Quaternion* rotation, const Vec3* scale,
                                     bool force_dirty)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    if (joint == NULL || translation == NULL || rotation == NULL || scale == NULL ||
        !isfinite(translation->x) || !isfinite(translation->y) ||
        !isfinite(translation->z) || !isfinite(rotation->x) ||
        !isfinite(rotation->y) || !isfinite(rotation->z) ||
        !isfinite(rotation->w) || !isfinite(scale->x) ||
        !isfinite(scale->y) || !isfinite(scale->z)) return;
    Vec3 position = *translation, scaling = *scale;
    Quaternion orientation = *rotation;
    if (force_dirty) {
        HSD_JObjSetTranslateWithMtxDirty(joint, &position);
        HSD_JObjSetRotationWithMtxDirty(joint, &orientation);
        HSD_JObjSetScaleWithMtxDirty(joint, &scaling);
    } else {
        HSD_JObjSetTranslate(joint, &position);
        HSD_JObjSetRotation(joint, &orientation);
        HSD_JObjSetScale(joint, &scaling);
    }
}

bool nativeArticleModel_Load(Item_GObj* gobj, unsigned model, unsigned name_hash)
{
    Item* item = gobj != NULL ? gobj->user_data : NULL;
    if (item == NULL || model < 1 || model > 3 || name_hash == 0 ||
        item->xC4_article_data == NULL) return false;
    Article* article = item->xC4_article_data;
    for (unsigned i = 0; i < article->native_model_root_count; ++i) {
        if (article->native_model_roots[i].name_hash != name_hash) continue;
        HSD_Joint* descriptor = article->native_model_roots[i].root;
        if (descriptor == NULL) return false;
        HSD_JObj* joint = HSD_JObjLoadJoint(descriptor);
        if (joint == NULL) return false;
        nativeArticleModel_Destroy(gobj, model);
        item->native_models[model - 1] = joint;
        return true;
    }
    return false;
}

bool nativeArticleModel_AddAnimation(Item_GObj* gobj, unsigned model,
                                     const char* path, unsigned name_hash)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    Item* item = gobj != NULL ? gobj->user_data : NULL;
    if (joint == NULL || item == NULL || item->xC4_article_data == NULL || name_hash == 0)
        return false;
    Article* article = item->xC4_article_data;
    for (unsigned i = 0; i < article->native_model_animation_count; ++i) {
        if (article->native_model_animations[i].name_hash != name_hash) continue;
        HSD_JObjAddAnimAll(joint, article->native_model_animations[i].anim,
            article->native_model_animations[i].matanim,
            article->native_model_animations[i].shapeanim);
        return true;
    }
    return false;
}

void nativeArticleModel_RequestAnimation(Item_GObj* gobj, unsigned model,
                                          const char* path, float frame)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    if (joint != NULL && isfinite(frame)) HSD_JObjReqAnimAll(joint, frame);
}

void nativeArticleModel_AdvanceAnimation(Item_GObj* gobj, unsigned model,
                                          const char* path)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    if (joint != NULL) HSD_JObjAnimAll(joint);
}

void nativeArticleModel_RemoveAnimation(Item_GObj* gobj, unsigned model,
                                         const char* path)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    if (joint != NULL) HSD_JObjRemoveAnimAll(joint);
}

void nativeArticleModel_SetAnimationRate(Item_GObj* gobj, unsigned model,
                                        const char* path, float rate)
{
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    if (joint != NULL && isfinite(rate)) lb_8000BA0C(joint, rate);
}

bool nativeArticleModel_ReadMatrixRotation(Item_GObj* gobj, unsigned model,
                                           const char* path, Vec3* rotation)
{
    if (rotation == NULL) return false;
    *rotation = (Vec3){ 0 };
    HSD_JObj* joint = nativeModelJoint_Resolve(model_root(gobj, model), path);
    if (joint == NULL) return false;
    // Read the existing world matrix; setup would change source call order.
    HSD_MtxGetRotation(joint->mtx, rotation);
    return true;
}

bool nativeArticleModel_Clone(Item_GObj* gobj, unsigned model)
{
    if (gobj == NULL || gobj->user_data == NULL || model < 1 || model > 3)
        return false;
    Item* item = gobj->user_data;
    if (item->native_models[model - 1] != NULL || gobj->hsd_obj == NULL ||
        item->xC4_article_data == NULL || item->xC4_article_data->x10_modelDesc == NULL)
        return false;
    HSD_JObj* joint = HSD_JObjLoadJoint(item->xC4_article_data->x10_modelDesc->x0_joint);
    if (joint == NULL) return false;
    Quaternion rotation;
    HSD_JObjGetRotation(gobj->hsd_obj, &rotation);
    HSD_JObjSetRotation(joint, &rotation);
    Vec3 scale = { item->scl, item->scl, item->scl };
    HSD_JObjSetScale(joint, &scale);
    item->native_models[model - 1] = joint;
    return true;
}

void nativeArticleModel_Destroy(Item_GObj* gobj, unsigned model)
{
    if (gobj == NULL || gobj->user_data == NULL || model < 1 || model > 3)
        return;
    Item* item = gobj->user_data;
    HSD_JObj* joint = item->native_models[model - 1];
    if (joint != NULL && !nativeArticleModelScene_Revoke(gobj, model, joint)) {
        HSD_ASSERTREPORT(0, false, "Cannot destroy a borrowed or active secondary model");
        return;
    }
    item->native_models[model - 1] = NULL;
    if (joint != NULL) HSD_JObjRemoveAll(joint);
}
#endif
