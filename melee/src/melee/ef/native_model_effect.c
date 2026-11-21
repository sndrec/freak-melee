#include "native_model_effect.h"

#ifdef MELEE_NATIVE
#include "eflib.h"
#include "efsync.h"
#include "efasync.h"
#include "types.h"
#include "native_script_effect_actor.h"
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/objalloc.h>
#include <string.h>

static EF_Effect* find_effect(HSD_GObj* object, const EF_Effect* address)
{
    if (HSD_GObjPLinkHead == NULL || (object == NULL && address == NULL))
        return NULL;
    const unsigned links[] = { HSD_GOBJ_PLINK_EFFECT_UNK0,
                               HSD_GOBJ_PLINK_EFFECT_UNK1 };
    for (unsigned i = 0; i < 2; ++i) {
        for (HSD_GObj* live = HSD_GObjPLinkHead[links[i]]; live; live = live->next) {
            if (object != NULL && live != object) continue;
            if (live->classifier != HSD_GOBJ_CLASS_EFFECT ||
                live->user_data_kind != 8 || live->user_data == NULL ||
                live->user_data_remove_func != efLib_remove_user_data)
                continue;
            if (address != NULL && live->user_data != address) continue;
            EF_Effect* effect = live->user_data;
            if (effect->gobj == live) return effect;
        }
    }
    return NULL;
}

static Fighter_GObj* bound_creator(const EF_Effect* effect)
{
    if (effect->native_policy == NULL || effect->native_author == NULL ||
        effect->native_cancelled || !nativeCallbackSource_Alive(
            effect->parent_gobj, effect->native_creator_generation,
            NativeCallbackSource_Fighter)) return NULL;
    Fighter_GObj* object = effect->parent_gobj;
    Fighter* fighter = object->user_data;
    const NativeFighterDefinition* definition =
        nativeFighterRuntime_Definition(fighter->kind);
    // The current registration establishes that author and policy memory is
    // still owned; do not read either saved module pointer before this test.
    if (definition != effect->native_author) return NULL;
    bool owns_policy = false;
    for (unsigned i = 0; i < definition->model_effect_count; ++i)
        if (effect->native_policy == &definition->model_effects[i]) {
            owns_policy = true;
            break;
        }
    if (!owns_policy) return NULL;
    const int part = effect->native_attachment_part;
    HSD_JObj* origin;
    if (part == -1) origin = object->hsd_obj;
    else {
        if (part < 0 || part >= definition->joint_count || fighter->parts == NULL)
            return NULL;
        origin = fighter->parts[part].joint;
    }
    const EF_ModelRecipe* recipe = efSync_ModelRecipe(effect->native_policy->gfx_id);
    if (recipe == NULL || origin == NULL || origin != effect->native_origin_joint ||
        (recipe->family == EF_MODEL_JOINT && origin != effect->attach_jobj) ||
        effect->native_member_count != recipe->model_count ||
        effect->native_member_index >= effect->native_member_count) return NULL;
    for (unsigned i = 0; i < effect->native_member_count; ++i) {
        EF_Effect* member = find_effect(effect->native_members[i].object, NULL);
        if (member == NULL || member->native_cancelled ||
            member->native_generation != effect->native_members[i].generation ||
            member->native_author != effect->native_author ||
            member->native_policy != effect->native_policy ||
            member->parent_gobj != object ||
            member->native_creator_generation != effect->native_creator_generation ||
            member->native_attachment_part != part ||
            member->native_origin_joint != origin ||
            (recipe->family == EF_MODEL_JOINT && member->attach_jobj != origin) ||
            member->native_member_count != effect->native_member_count ||
            member->native_member_index != i) return NULL;
        for (unsigned j = 0; j < effect->native_member_count; ++j)
            if (member->native_members[j].object != effect->native_members[j].object ||
                member->native_members[j].generation != effect->native_members[j].generation)
                return NULL;
    }
    return object;
}

EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object, uint32_t generation)
{
    if (nativeScriptEffect_IsAttached(object))
        return nativeScriptEffect_Resolve(object,generation);
    EF_Effect* effect = find_effect(object, NULL);
    if (effect == NULL || generation == 0 ||
        effect->native_generation != generation || effect->native_cancelled)
        return NULL;
    if (effect->native_policy != NULL && bound_creator(effect) == NULL)
        return NULL;
    return effect;
}

bool nativeModelEffect_Snapshot(HSD_GObj* object, uint32_t* generation)
{
    if (generation == NULL) return false;
    *generation = 0;
    if (nativeScriptEffect_IsAttached(object))
        return nativeScriptEffect_Snapshot(object,generation);
    EF_Effect* effect = find_effect(object, NULL);
    if (effect == NULL || effect->native_generation == 0 ||
        effect->native_cancelled ||
        (effect->native_policy != NULL && bound_creator(effect) == NULL))
        return false;
    *generation = effect->native_generation;
    return true;
}

Fighter_GObj* nativeModelEffect_Creator(EF_Effect* address)
{
    EF_Effect* effect = find_effect(NULL, address);
    if (effect && nativeScriptEffect_IsAttached(effect->gobj))
        return nativeScriptEffect_Creator(effect->gobj,effect->native_generation);
    return effect != NULL ? bound_creator(effect) : NULL;
}

bool nativeModelEffect_Validate(const NativeFighterDefinition* definition)
{
    if (definition == NULL ||
        definition->model_effect_count > NATIVE_FIGHTER_MODEL_EFFECT_LIMIT ||
        ((definition->model_effect_count == 0) !=
         (definition->model_effects == NULL))) return false;
    uint32_t previous = 0;
    for (unsigned i = 0; i < definition->model_effect_count; ++i) {
        const NativeFighterModelEffect* policy = &definition->model_effects[i];
        if (policy->name_hash <= previous || policy->update == NULL ||
            efSync_ModelRecipe(policy->gfx_id) == NULL) return false;
        previous = policy->name_hash;
    }
    return true;
}

void nativeModelEffect_Remove(EF_Effect* effect)
{
    if (effect == NULL || effect->native_cancelled) return;
    EF_ModelMember members[EF_MODEL_GROUP_MAX];
    memcpy(members, effect->native_members, sizeof(members));
    unsigned count = effect->native_member_count;
    if (count > EF_MODEL_GROUP_MAX) count = EF_MODEL_GROUP_MAX;
    effect->native_cancelled = true;
    effect->update = NULL;
    // Invalidate the whole group before any destructor can reenter it.
    for (unsigned i = 0; i < count; ++i) {
        EF_Effect* member = find_effect(members[i].object, NULL);
        if (member != NULL && member->native_generation == members[i].generation) {
            member->native_cancelled = true;
            member->update = NULL;
        }
    }
    HSD_GObj* source = effect->gobj;
    for (unsigned i = 0; i < count; ++i) {
        EF_Effect* member = find_effect(members[i].object, NULL);
        if (member != NULL && member->gobj != source &&
            member->native_generation == members[i].generation)
            HSD_GObjFree(member->gobj);
    }
}

static void cancel_effect(EF_Effect* effect)
{
    if (effect == NULL || effect->native_cancelled) return;
    HSD_GObj* object = effect->gobj;
    nativeModelEffect_Remove(effect);
    HSD_GObjFree(object);
}

HSD_JObj* nativeModelEffect_Joint(EF_Effect* address, unsigned index)
{
    EF_Effect* effect = find_effect(NULL, address);
    if (effect && nativeScriptEffect_IsAttached(effect->gobj))
        return nativeScriptEffect_Joint(effect->gobj,effect->native_generation,index);
    if (effect == NULL || bound_creator(effect) == NULL ||
        index >= effect->native_member_count) return NULL;
    EF_Effect* member = find_effect(effect->native_members[index].object, NULL);
    return member != NULL ? member->gobj->hsd_obj : NULL;
}

void nativeModelEffect_Cancel(EF_Effect* address)
{
    cancel_effect(find_effect(NULL, address));
}

void nativeModelEffect_CancelGeneration(HSD_GObj* object, uint32_t generation)
{
    EF_Effect* effect = find_effect(object, NULL);
    if (effect != NULL && effect->native_generation == generation)
        cancel_effect(effect);
}

void nativeModelEffect_Spawn(Fighter_GObj* object,
                             const NativeFighterModelEffect* requested, s32 part,
                             Vec3* position, bool deferred)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(object, NativeCallbackSource_Fighter,
                                       &generation)) return;
    Fighter* fighter = object->user_data;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(fighter->kind);
    if (definition == NULL || !nativeModelEffect_Validate(definition)) return;
    const NativeFighterModelEffect* policy = NULL;
    for (unsigned i = 0; i < definition->model_effect_count; ++i)
        if (&definition->model_effects[i] == requested) { policy = requested; break; }
    if (policy == NULL || part < -1 || part >= definition->joint_count) return;
    const EF_ModelRecipe* recipe = efSync_ModelRecipe(policy->gfx_id);
    if (recipe->model_count == 0 || recipe->model_count > EF_MODEL_GROUP_MAX) return;
    HSD_JObj* sample = part == -1 ? object->hsd_obj
        : fighter->parts != NULL ? fighter->parts[part].joint : NULL;
    if (sample == NULL) return;
    if (deferred) {
        if (position != NULL || recipe->family != EF_MODEL_POSITION) return;
        EF_QueuedEffect* queued = efAsync_AllocateRequest();
        if (queued == NULL) return;
        *queued = (EF_QueuedEffect){ .spawn_kind = EF_SPAWN_POS,
            .gfx_id = policy->gfx_id, .jobj = sample, .native_policy = policy,
            .native_author = definition, .native_creator_generation = generation,
            .native_sample_part = (s16)part };
        if (HSD_GObj_CurrentInvokedProc != NULL &&
            HSD_GObj_CurrentInvokedProc->s_link < 9U) {
            EF_QueuedEffect* head = (EF_QueuedEffect*)&fighter->x60C;
            queued->next = head->next;
            head->next = queued;
        } else efAsync_QueueProcessDeferred(object, queued);
        return;
    }
    if ((recipe->family == EF_MODEL_POSITION) != (position != NULL)) return;
    HSD_JObj* origin = recipe->family == EF_MODEL_POSITION ? object->hsd_obj : sample;
    if (origin == NULL) return;
    EF_SpawnResult result = recipe->family == EF_MODEL_POSITION
        ? efSync_Spawn(policy->gfx_id, object, position)
        : efSync_Spawn(policy->gfx_id, object, origin);
    if (result.kind != EF_SPAWN_MODEL) return;
    EF_Effect* members[EF_MODEL_GROUP_MAX] = { 0 };
    bool valid = result.kind == EF_SPAWN_MODEL && result.member_count == recipe->model_count;
    for (unsigned i = 0; i < result.member_count && i < EF_MODEL_GROUP_MAX; ++i) {
        members[i] = find_effect(result.members[i].object, NULL);
        if (members[i] == NULL || members[i]->native_cancelled ||
            members[i]->native_generation != result.members[i].generation ||
            members[i]->parent_gobj != object ||
            (recipe->family == EF_MODEL_JOINT && members[i]->attach_jobj != origin)) valid = false;
        for (unsigned j = 0; j < i; ++j)
            if (result.members[i].object == result.members[j].object) valid = false;
    }
    if (!nativeCallbackSource_Alive(object, generation, NativeCallbackSource_Fighter)) valid = false;
    else {
        fighter = object->user_data;
        if (nativeFighterRuntime_Definition(fighter->kind) != definition ||
            (recipe->family == EF_MODEL_POSITION ? object->hsd_obj
                : part == -1 ? object->hsd_obj
                : fighter->parts != NULL ? fighter->parts[part].joint : NULL) != origin) valid = false;
    }
    if (!valid) {
        for (unsigned i = 0; i < result.member_count && i < EF_MODEL_GROUP_MAX; ++i) {
            EF_Effect* member = find_effect(result.members[i].object, NULL);
            if (member != NULL && member->native_generation == result.members[i].generation)
                cancel_effect(member);
        }
        return;
    }
    for (unsigned i = 0; i < result.member_count; ++i) {
        EF_Effect* member = members[i];
        member->native_creator_generation = generation;
        member->native_author = definition;
        member->native_policy = policy;
        member->native_attachment_part = recipe->family == EF_MODEL_POSITION ? -1 : (int16_t)part;
        member->native_origin_joint = origin;
        member->native_member_count = (u8)result.member_count;
        member->native_member_index = (u8)i;
        memcpy(member->native_members, result.members, sizeof(result.members));
        member->update = i == 0 ? policy->update : NULL;
    }
}

void nativeModelEffect_Clear(void)
{
    if (HSD_GObjPLinkHead == NULL) return;
    const unsigned links[] = { HSD_GOBJ_PLINK_EFFECT_UNK0, HSD_GOBJ_PLINK_EFFECT_UNK1 };
    for (unsigned i = 0; i < 2; ++i) {
        for (;;) {
            EF_Effect* selected = NULL;
            for (HSD_GObj* object = HSD_GObjPLinkHead[links[i]]; object; object = object->next) {
                EF_Effect* effect = find_effect(object, NULL);
                if (effect != NULL && effect->native_policy != NULL && !effect->native_cancelled) {
                    selected = effect; break;
                }
            }
            if (selected == NULL) break;
            nativeModelEffect_Cancel(selected);
        }
    }
}
#endif
