#include "native_article_refs.h"

#include "types.h"
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <string.h>

static const NativeFighterDefinition* caller_definition(HSD_GObj* caller)
{
    if (caller == NULL || HSD_GObjPLinkHead == NULL) return NULL;
    // Check live membership before dereferencing an address which may have
    // been removed by an earlier synchronous callback.
    for (HSD_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         live != NULL; live = live->next) {
        if (live != caller) continue;
        Fighter* fighter = live->user_data;
        return fighter != NULL && fighter->kind >= Ft_Kind_CustomFirst &&
                       fighter->kind < Ft_Kind_RuntimeCapacity
            ? nativeFighterRuntime_Definition(fighter->kind) : NULL;
    }
    for (HSD_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         live != NULL; live = live->next) {
        if (live != caller) continue;
        Item* item = live->user_data;
        return item != NULL &&
                       item->native_article_author_kind >= Ft_Kind_CustomFirst &&
                       item->native_article_author_kind < Ft_Kind_RuntimeCapacity
            ? nativeFighterRuntime_Definition(item->native_article_author_kind)
            : NULL;
    }
    return NULL;
}

static Item* resolve(HSD_GObj* caller, uint32_t handle, ItemKind kind,
                     const NativeFighterArticleLogic** logic)
{
    if (handle == UINT32_MAX) return NULL;
    const NativeFighterDefinition* definition = caller_definition(caller);
    if (definition == NULL || definition->article_logics == NULL) return NULL;
    const NativeFighterArticleLogic* expected = NULL;
    for (unsigned i = 0; i < definition->article_logic_count; ++i) {
        if (definition->article_logics[i].kind == kind) {
            expected = &definition->article_logics[i];
            break;
        }
    }
    if (expected == NULL || expected->logic == NULL) return NULL;
    for (HSD_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         live != NULL; live = live->next) {
        Item* item = live->user_data;
        if (item == NULL || item->x1C != handle || item->kind != kind ||
            item->native_article_author_kind < Ft_Kind_CustomFirst ||
            item->native_article_author_kind >= Ft_Kind_RuntimeCapacity ||
            nativeFighterRuntime_Definition(item->native_article_author_kind) !=
                definition || item->xB8_itemLogicTable != expected->logic)
            continue;
        *logic = expected;
        return item;
    }
    return NULL;
}

static const uint8_t* field_address(HSD_GObj* caller, uint32_t handle,
                                    ItemKind kind, unsigned field_index,
                                    int32_t index, NativeFighterStateType type)
{
    const NativeFighterArticleLogic* logic;
    Item* item = resolve(caller, handle, kind, &logic);
    if (item == NULL || logic->state_fields == NULL ||
        field_index >= logic->state_field_count || index < 0)
        return NULL;
    const NativeFighterStateField* field = &logic->state_fields[field_index];
    unsigned width = type == NativeFighterState_Bool ? 1 : 4;
    if (field->type != type || field->stride != width || field->count == 0 ||
        (unsigned) index >= field->count || (!field->array && index != 0) ||
        logic->local_state_bytes >
            sizeof(item->xDD4_itemVar.native_article.state))
        return NULL;
    unsigned offset = field->offset + (unsigned) index * field->stride;
    if (offset > logic->local_state_bytes ||
        width > logic->local_state_bytes - offset)
        return NULL;
    return item->xDD4_itemVar.native_article.state + offset;
}

bool nativeArticleRef_Exists(HSD_GObj* caller, uint32_t handle, ItemKind kind)
{
    const NativeFighterArticleLogic* logic;
    return resolve(caller, handle, kind, &logic) != NULL;
}

uint32_t nativeArticleRef_CurrentOwner(HSD_GObj* caller, uint32_t handle,
                                       ItemKind kind)
{
    const NativeFighterArticleLogic* logic;
    const Item* item = resolve(caller, handle, kind, &logic);
    if (item == NULL || item->native_removal_in_progress || item->owner == NULL)
        return 0;
    // Ownership may transfer without changing the article's immutable author.
    // Compare retained addresses against the live registry before reading them.
    for (HSD_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         object != NULL; object = object->next) {
        if (object == item->owner && object->user_data != NULL)
            return (uint32_t)((Fighter*)object->user_data)->x8_spawnNum;
    }
    return 0;
}

bool nativeArticleRef_PrimaryModelTranslation(HSD_GObj* caller, uint32_t handle,
                                               ItemKind kind, Vec3* position)
{
    if (position == NULL) return false;
    uint32_t generation, source_class = NativeCallbackSource_Fighter;
    if (!nativeCallbackSource_Snapshot(caller, source_class, &generation)) {
        source_class = NativeCallbackSource_Item;
        if (!nativeCallbackSource_Snapshot(caller, source_class, &generation))
            return false;
    }
    void* source_data = caller->user_data;
    FighterKind source_kind = source_class == NativeCallbackSource_Fighter
        ? ((Fighter*)source_data)->kind
        : ((Item*)source_data)->native_article_author_kind;
    ftData* source_fighter_data = source_class == NativeCallbackSource_Fighter
        ? ((Fighter*)source_data)->ft_data : NULL;
    ItemKind source_item_kind = source_class == NativeCallbackSource_Item
        ? ((Item*)source_data)->kind : 0;
    ItemLogicTable* source_logic = source_class == NativeCallbackSource_Item
        ? ((Item*)source_data)->xB8_itemLogicTable : NULL;
    const NativeFighterDefinition* definition = caller_definition(caller);
    if (definition == NULL || definition->article_logics == NULL ||
        !nativeCallbackSource_Alive(caller, generation, source_class) ||
        caller->user_data != source_data) return false;
    if (source_class == NativeCallbackSource_Item) {
        Item* source = source_data;
        bool authored_logic = false;
        if (source->native_removal_in_progress) return false;
        for (unsigned i = 0; i < definition->article_logic_count; ++i) {
            const NativeFighterArticleLogic* candidate = &definition->article_logics[i];
            if (candidate->kind == source->kind && candidate->logic != NULL &&
                candidate->logic == source->xB8_itemLogicTable) {
                authored_logic = true;
                break;
            }
        }
        if (!authored_logic) return false;
    }
    const NativeFighterArticleLogic* logic;
    Item* item = resolve(caller, handle, kind, &logic);
    if (!nativeCallbackSource_Alive(caller, generation, source_class) ||
        caller->user_data != source_data) return false;
    if (item == NULL || item->native_removal_in_progress) return false;
    HSD_GObj* object = NULL;
    for (HSD_GObj* live = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         live != NULL; live = live->next) {
        if (live->user_data == item && live == item->entity) {
            object = live;
            break;
        }
    }
    if (object == NULL || object->hsd_obj == NULL) return false;
    HSD_JObj* root = object->hsd_obj;
    Vec3 result;
    HSD_JObjGetTranslation(root, &result);
    // Do not commit a partially observed query if synchronous engine work
    // replaced either entity, its immutable author or the selected model.
    const NativeFighterDefinition* current_definition = caller_definition(caller);
    const NativeFighterArticleLogic* current_logic;
    Item* current_item = resolve(caller, handle, kind, &current_logic);
    if (!nativeCallbackSource_Alive(caller, generation, source_class) ||
        caller->user_data != source_data ||
        (source_class == NativeCallbackSource_Fighter
            ? ((Fighter*)source_data)->kind
            : ((Item*)source_data)->native_article_author_kind) != source_kind ||
        (source_class == NativeCallbackSource_Fighter
            ? ((Fighter*)source_data)->ft_data != source_fighter_data
            : (((Item*)source_data)->native_removal_in_progress ||
               ((Item*)source_data)->kind != source_item_kind ||
               ((Item*)source_data)->xB8_itemLogicTable != source_logic)) ||
        current_definition != definition || current_item != item ||
        current_logic != logic ||
        !nativeCallbackSource_Alive(object, handle, NativeCallbackSource_Item) ||
        object->user_data != item || object->hsd_obj != root ||
        item->native_removal_in_progress)
        return false;
    *position = result;
    return true;
}

float nativeArticleRef_Float(HSD_GObj* caller, uint32_t handle, ItemKind kind,
                            unsigned field_index, int32_t index)
{
    float value = 0;
    const uint8_t* field = field_address(caller, handle, kind, field_index, index,
                                       NativeFighterState_Float);
    if (field != NULL) memcpy(&value, field, sizeof(value));
    return value;
}

int32_t nativeArticleRef_Int(HSD_GObj* caller, uint32_t handle, ItemKind kind,
                            unsigned field_index, int32_t index)
{
    int32_t value = 0;
    const uint8_t* field = field_address(caller, handle, kind, field_index, index,
                                       NativeFighterState_Int);
    if (field != NULL) memcpy(&value, field, sizeof(value));
    return value;
}

bool nativeArticleRef_Bool(HSD_GObj* caller, uint32_t handle, ItemKind kind,
                          unsigned field_index, int32_t index)
{
    const uint8_t* field = field_address(caller, handle, kind, field_index, index,
                                       NativeFighterState_Bool);
    return field != NULL && *field != 0;
}
