#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

HSD_GObj** HSD_GObjPLinkHead;

int main(void)
{
    HSD_GObj* heads[32] = { 0 };
    Fighter fighter = { .x8_spawnNum = (int32_t) 0xFEDCBA98u };
    Item item = { .x1C = 0x80000001u };
    HSD_GObj f = { .user_data = &fighter };
    HSD_GObj i = { .user_data = &item };
    heads[HSD_GOBJ_PLINK_FIGHTER] = &f;
    heads[HSD_GOBJ_PLINK_ITEM] = &i;
    uint32_t generation = 99;
    assert(!nativeCallbackSource_Snapshot(&f, NativeCallbackSource_Fighter, &generation));
    assert(generation == 0);
    HSD_GObjPLinkHead = heads;
    assert(nativeCallbackSource_Snapshot(&f, NativeCallbackSource_Fighter, &generation));
    assert(generation == 0xFEDCBA98u);
    assert(nativeCallbackSource_Alive(&f, generation, NativeCallbackSource_Fighter));
    assert(!nativeCallbackSource_Alive(&f, generation, NativeCallbackSource_Item));
    assert(nativeCallbackSource_Snapshot(&i, NativeCallbackSource_Item, &generation));
    assert(generation == 0x80000001u);
    assert(nativeCallbackSource_Alive(&i, generation, NativeCallbackSource_Item));
    item.x1C = 16777217u;
    assert(!nativeCallbackSource_Alive(&i, generation, NativeCallbackSource_Item));
    assert(nativeCallbackSource_Alive(&i, 16777217u, NativeCallbackSource_Item));
    item.x1C = UINT32_MAX;
    assert(nativeCallbackSource_Alive(&i, UINT32_MAX, NativeCallbackSource_Item));
    item.x1C = 0;
    assert(nativeCallbackSource_Alive(&i, 0, NativeCallbackSource_Item));
    heads[HSD_GOBJ_PLINK_ITEM] = NULL;
    i.user_data = (void*) 1;
    assert(!nativeCallbackSource_Alive(&i, 0, NativeCallbackSource_Item));
    assert(!nativeCallbackSource_Snapshot(&i, NativeCallbackSource_Item, &generation));
    assert(!nativeCallbackSource_Snapshot((HSD_GObj*) 1, NativeCallbackSource_Item, &generation));
    assert(!nativeCallbackSource_Snapshot(NULL, NativeCallbackSource_Fighter, &generation));
    assert(!nativeCallbackSource_Snapshot(&f, 2, &generation));
    assert(!nativeCallbackSource_Snapshot(&f, NativeCallbackSource_Fighter, NULL));
    assert(!nativeCallbackSource_Alive(NULL, 0, NativeCallbackSource_Item));
    assert(!nativeCallbackSource_Alive(&f, 0, UINT32_MAX));
    f.user_data = NULL;
    assert(!nativeCallbackSource_Alive(&f, 0xFEDCBA98u, NativeCallbackSource_Fighter));
    for (unsigned pass = 0; pass < 2; ++pass) {
        for (unsigned depth = 0; depth < 64; ++depth)
            assert(nativeCallbackDispatch_Enter());
        assert(!nativeCallbackDispatch_Enter());
        for (unsigned depth = 0; depth < 64; ++depth)
            nativeCallbackDispatch_Leave();
    }
    nativeCallbackDispatch_Leave();
    assert(nativeCallbackDispatch_Enter());
    nativeCallbackDispatch_Leave();
    puts("Live callback generation and bounded balanced dispatch passed");
    return 0;
}
