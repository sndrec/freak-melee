#include "native_held_article.h"
#include "types.h"
#include "native_script_item_actor.h"
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

typedef struct HeldArticleSnapshot {
    Fighter_GObj* holder;
    Item_GObj* object;
    Fighter* fighter;
    Item* item;
    uint32_t fighter_generation, item_generation;
    FighterKind holder_kind, author_kind;
    const NativeFighterDefinition* holder_definition;
    const NativeFighterDefinition* author_definition;
    const NativeFighterArticleLogic* logic;
    const NativeScriptItemProgram* source_program;
    NativeMatchPoolReference holder_reference, item_reference;
} HeldArticleSnapshot;

// 0: ordinary engine policy; 1: authored policy; 2: invalid retained identity.
static unsigned capture_held(Fighter_GObj* holder, HeldArticleSnapshot* saved,
                             bool neutral_notification)
{
    *saved = (HeldArticleSnapshot){0};
    if (!nativeMatchPool_Reference(&nativeGObjPool, holder, &saved->holder_reference) ||
        holder->classifier != HSD_GOBJ_CLASS_FIGHTER) return 2;
    if (!nativeCallbackSource_Snapshot(holder, NativeCallbackSource_Fighter,
                                       &saved->fighter_generation)) return 2;
    saved->holder = holder;
    saved->fighter = holder->user_data;
    saved->holder_kind = saved->fighter->kind;
    saved->holder_definition = nativeFighterRuntime_Definition(saved->holder_kind);
    saved->object = saved->fighter->item_gobj;
    if (saved->object == NULL) return 0;
    if (!nativeMatchPool_Reference(&nativeGObjPool, saved->object, &saved->item_reference) ||
        saved->holder_reference.lifetime != saved->item_reference.lifetime ||
        saved->object->classifier != HSD_GOBJ_CLASS_ITEM) return 2;
    if (!nativeCallbackSource_Snapshot(saved->object, NativeCallbackSource_Item,
                                       &saved->item_generation)) return 2;
    saved->item = saved->object->user_data;
    if (saved->item->native_removal_in_progress) return 2;
    saved->author_kind = saved->item->native_article_author_kind;
    if (saved->item->native_script_owner) {
        saved->source_program = nativeScriptItem_Spec(saved->object);
        if (saved->source_program == NULL ||
            nativeScriptItem_Status(saved->object) != NativeScriptVM_Ok) return 2;
        return 1;
    }
    saved->author_definition = nativeFighterRuntime_Definition(saved->author_kind);
    if (saved->author_definition == NULL) return 0;
    for (unsigned i = 0; i < saved->author_definition->article_logic_count; ++i) {
        const NativeFighterArticleLogic* logic = &saved->author_definition->article_logics[i];
        if (logic->kind != saved->item->kind) continue;
        if (neutral_notification ? logic->on_holder_neutral == NULL :
            (logic->on_held_status == NULL && logic->on_held_request == NULL)) return 0;
        if (logic->logic != saved->item->xB8_itemLogicTable) return 2;
        saved->logic = logic;
        return 1;
    }
    return 0;
}

static bool held_unchanged(const HeldArticleSnapshot* saved)
{
    if (nativeMatchPool_Resolve(saved->holder_reference) != saved->holder ||
        nativeMatchPool_Resolve(saved->item_reference) != saved->object ||
        saved->holder->classifier != HSD_GOBJ_CLASS_FIGHTER ||
        saved->object->classifier != HSD_GOBJ_CLASS_ITEM ||
        saved->holder->user_data != saved->fighter ||
        saved->object->user_data != saved->item) return false;
    if (!nativeCallbackSource_Alive(saved->holder, saved->fighter_generation, NativeCallbackSource_Fighter) ||
        !nativeCallbackSource_Alive(saved->object, saved->item_generation, NativeCallbackSource_Item)) return false;
    if (saved->fighter->kind != saved->holder_kind ||
        nativeFighterRuntime_Definition(saved->holder_kind) != saved->holder_definition ||
        saved->fighter->item_gobj != saved->object ||
        saved->item->native_removal_in_progress ||
        saved->item->native_article_author_kind != saved->author_kind) return false;
    if (saved->source_program != NULL)
        return nativeScriptItem_Spec(saved->object) == saved->source_program &&
            nativeScriptItem_Status(saved->object) == NativeScriptVM_Ok;
    return nativeFighterRuntime_Definition(saved->author_kind) == saved->author_definition &&
        saved->item->kind == saved->logic->kind &&
        saved->item->xB8_itemLogicTable == saved->logic->logic;
}

bool nativeHeldArticle_Status(Fighter_GObj* holder, int32_t* status)
{
    if (status == NULL) return true;
    *status = -1;
    HeldArticleSnapshot saved;
    unsigned captured = capture_held(holder, &saved, false);
    if (captured != 1) return captured != 0;
    if (saved.source_program != NULL) {
        if (!nativeScriptItem_HookBound(saved.object, NativeScriptItemHook_HeldStatus)) return true;
        NativeScriptValue result;
        if (nativeScriptItem_RunHook(saved.object, NativeScriptItemHook_HeldStatus,
                NULL, 0, &result) == NativeScriptVM_Ok &&
            result.type == NativeScriptType_I32 && held_unchanged(&saved))
            *status = (int32_t) result.as.i64;
        return true;
    }
    if (saved.logic->on_held_status == NULL || !nativeCallbackDispatch_Enter()) return true;
    int32_t result = saved.logic->on_held_status(saved.object);
    nativeCallbackDispatch_Leave();
    if (held_unchanged(&saved)) *status = result;
    return true;
}

bool nativeHeldArticle_Request(Fighter_GObj* holder, int32_t action,
                               float frame_rate, float duration)
{
    HeldArticleSnapshot saved;
    unsigned captured = capture_held(holder, &saved, false);
    if (captured != 1) return captured != 0;
    if (saved.source_program != NULL) {
        const NativeScriptValue arguments[] = {
            { .type=NativeScriptType_I32, .as.i64=action },
            { .type=NativeScriptType_F32, .as.f32=frame_rate },
            { .type=NativeScriptType_F32, .as.f32=duration },
        };
        NativeScriptValue result;
        nativeScriptItem_RunHook(saved.object, NativeScriptItemHook_HeldRequest,
            arguments, 3, &result);
        return true;
    }
    if (saved.logic->on_held_request == NULL || !nativeCallbackDispatch_Enter()) return true;
    saved.logic->on_held_request(saved.object, action, frame_rate, duration);
    nativeCallbackDispatch_Leave();
    // There is no cached engine tail here. Callers must also validate their own
    // retained fighter before continuing after a request that changes the world.
    return true;
}

bool nativeHeldArticle_NotifyNeutral(Fighter_GObj* holder, bool* claimed)
{
    HeldArticleSnapshot saved;
    unsigned captured = capture_held(holder, &saved, true);
    if (claimed != NULL) *claimed = captured != 0;
    if (captured != 1) return false;
    if (saved.source_program != NULL) {
        if (!nativeScriptItem_HookBound(saved.object, NativeScriptItemHook_HolderNeutral)) return false;
        NativeScriptValue result;
        return nativeScriptItem_RunHook(saved.object, NativeScriptItemHook_HolderNeutral,
            NULL, 0, &result) == NativeScriptVM_Ok && held_unchanged(&saved);
    }
    if (!nativeCallbackDispatch_Enter()) return false;
    saved.logic->on_holder_neutral(saved.object);
    nativeCallbackDispatch_Leave();
    return held_unchanged(&saved);
}
