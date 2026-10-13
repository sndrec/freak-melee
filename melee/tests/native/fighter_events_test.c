#include <melee/ft/native_fighter_events.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static NativeFighterDefinition definitions[2];
static unsigned calls, recursion;
static uint32_t recursive_target;

HSD_GObj* ftLib_8008741C(u32 target)
{
    for (HSD_GObj* live = heads[HSD_GOBJ_PLINK_FIGHTER]; live; live = live->next)
        if (live->user_data != NULL &&
            (uint32_t) ((Fighter*) live->user_data)->x8_spawnNum == target)
            return live;
    return NULL;
}

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    return kind == Ft_Kind_CustomFirst ? &definitions[0] :
        kind == Ft_Kind_CustomFirst + 1 ? &definitions[1] : NULL;
}

static void first(HSD_GObj* object)
{
    Fighter* fighter = object->user_data;
    fighter->native_module_state[1] = 17;
    ++calls;
}

static void second(HSD_GObj* object)
{
    Fighter* fighter = object->user_data;
    fighter->native_module_state[31] = 29;
    ++calls;
}

static void remove_recipient(HSD_GObj* object)
{
    heads[HSD_GOBJ_PLINK_FIGHTER] = object->next;
    object->user_data = (void*) 1;
    ++calls;
}

static void recursive(HSD_GObj* object)
{
    (void) object;
    ++recursion;
    if (recursion < 64) assert(nativeFighterEvent_Call(recursive_target, 30));
    else assert(!nativeFighterEvent_Call(recursive_target, 30));
}

static bool bool_result(HSD_GObj* object) { (void)object; ++calls; return true; }
static int32_t int_result(HSD_GObj* object) { (void)object; ++calls; return 16777217; }
static float float_result(HSD_GObj* object) { (void)object; ++calls; return -2.25f; }
static bool remove_bool(HSD_GObj* object) { remove_recipient(object); return true; }

int main(void)
{
    NativeFighterCallbackExport exports[3] = {
        { .name_hash=10, .callback=first }, { .name_hash=20, .callback=remove_recipient },
        { .name_hash=30, .callback=recursive } };
    NativeFighterCallbackExport other[] = { { .name_hash=10, .callback=second },
        { .name_hash=20, .bool_callback=bool_result, .result_type=NativeFighterReturn_Bool },
        { .name_hash=30, .int_callback=int_result, .result_type=NativeFighterReturn_Int },
        { .name_hash=40, .float_callback=float_result, .result_type=NativeFighterReturn_Float },
        { .name_hash=50, .bool_callback=remove_bool, .result_type=NativeFighterReturn_Bool } };
    definitions[0].callback_exports = exports;
    definitions[0].callback_export_count = 3;
    definitions[1].callback_exports = other;
    definitions[1].callback_export_count = 5;
    assert(nativeFighterEvents_Validate(&definitions[0]));
    assert(nativeFighterEvents_Validate(&definitions[1]));
    Fighter a = { .kind = Ft_Kind_CustomFirst, .x8_spawnNum = (int32_t) 0xFEDCBA98u };
    Fighter b = { .kind = Ft_Kind_CustomFirst + 1, .x8_spawnNum = 16777217 };
    HSD_GObj second_object = { .user_data = &b };
    HSD_GObj first_object = { .user_data = &a, .next = &second_object };
    heads[HSD_GOBJ_PLINK_FIGHTER] = &first_object;
    HSD_GObjPLinkHead = heads;
    assert(nativeFighterEvent_Call(0xFEDCBA98u, 10));
    assert(nativeFighterEvent_Call(16777217u, 10));
    assert(calls == 2 && a.native_module_state[1] == 17 && b.native_module_state[31] == 29);
    assert(a.native_module_state[31] == 0 && b.native_module_state[1] == 0);
    assert(nativeFighterEvent_CallBool(16777217,20));
    assert(nativeFighterEvent_CallInt(16777217,30)==16777217);
    assert(nativeFighterEvent_CallFloat(16777217,40)==-2.25f);
    unsigned typed_calls=calls;
    assert(!nativeFighterEvent_CallBool(16777217,30));
    assert(nativeFighterEvent_CallInt(16777217,40)==0);
    assert(nativeFighterEvent_CallFloat(16777217,20)==0.0f);
    assert(!nativeFighterEvent_CallBool(0,20));
    assert(nativeFighterEvent_CallInt(16777217,999)==0);
    assert(nativeFighterEvent_CallFloat(16777216,40)==0.0f);
    assert(calls==typed_calls);
    assert(nativeFighterEvent_Call(16777217,30));
    assert(!nativeFighterEvent_Call(0, 10));
    assert(!nativeFighterEvent_Call(16777216u, 10));
    assert(!nativeFighterEvent_Call(0xFEDCBA98u, 0));
    assert(!nativeFighterEvent_Call(0xFEDCBA98u, 11));
    recursive_target = 0xFEDCBA98u;
    assert(nativeFighterEvent_Call(recursive_target, 30));
    assert(recursion == 64);
    assert(nativeFighterEvent_Call(recursive_target, 10)); // Depth fully unwound.
    assert(nativeFighterEvent_Call(recursive_target, 20));
    assert(!nativeCallbackSource_Alive(&first_object, recursive_target, NativeCallbackSource_Fighter));
    assert(!nativeFighterEvent_Call(recursive_target, 10));
    first_object.user_data = &a;
    heads[HSD_GOBJ_PLINK_FIGHTER] = &first_object;
    a.x8_spawnNum = (int32_t) 0xFEDCBA99u;
    assert(!nativeFighterEvent_Call(recursive_target, 10));
    assert(nativeFighterEvent_Call(0xFEDCBA99u, 10));
    a.kind = Ft_Kind_Mario;
    assert(!nativeFighterEvent_Call(0xFEDCBA99u, 10));
    NativeFighterDefinition invalid = { 0 };
    assert(nativeFighterEvents_Validate(&invalid));
    assert(!nativeFighterEvents_Validate(NULL));
    invalid.callback_exports = exports;
    assert(!nativeFighterEvents_Validate(&invalid));
    invalid.callback_export_count = 65;
    assert(!nativeFighterEvents_Validate(&invalid));
    invalid.callback_export_count = 3;
    exports[1].name_hash = 10;
    assert(!nativeFighterEvents_Validate(&invalid));
    exports[1].name_hash = 20;
    exports[0].name_hash = 0;
    assert(!nativeFighterEvents_Validate(&invalid));
    exports[0].name_hash = 10;
    exports[1].callback = NULL;
    assert(!nativeFighterEvents_Validate(&invalid));
    invalid.callback_exports = NULL;
    assert(!nativeFighterEvents_Validate(&invalid));
    heads[HSD_GOBJ_PLINK_FIGHTER]=&second_object;
    assert(nativeFighterEvent_CallBool(16777217,50));
    assert(!nativeFighterEvent_CallBool(16777217,20));
    puts("Recipient-owned named events, removal and dispatch bounds passed");
    return 0;
}
