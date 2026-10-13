#include <melee/ft/inlines.h>
#include <melee/it/inlines.h>
#include <melee/it/it_2725.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <sysdolphin/baselib/jobj.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef NDEBUG
#include <assert.h>

#define OWNER_Y_OFFSET (offsetof(Fighter, x34_scale) + offsetof(Vec3, y))
_Static_assert(offsetof(Item, scl) != OWNER_Y_OFFSET,
               "Regression requires distinct native Item/Fighter offsets");
_Static_assert(offsetof(Item, scl) + sizeof(float) <= sizeof(Fighter),
               "Decoy must fit within the actual Fighter object");

void __assert(char* file, u32 line, char* message)
{
    fprintf(stderr, "%s:%lu: %s\n", file, (unsigned long) line, message);
    abort();
}

#include "hookshot_scale.generated.h"

static void check_owner_scale(float owner_scale, float attribute_scale,
                              float expected, bool independent_transform)
{
    Fighter owner = { 0 };
    HSD_GObj owner_gobj = { 0 };
    Item item = { 0 };
    ItemAttr attributes = { 0 };
    HSD_GObj item_gobj = { 0 };
    HSD_JObj model = { 0 };
    HSD_JObj child = { 0 };
    float decoy = 13.0f;
    float stored_decoy;

    /* Byte-copy a distinct value into the old mistaken Item scale offset.
     * Read it as bytes too: never alias a Fighter object through Item*. */
    memcpy((unsigned char*) &owner + offsetof(Item, scl), &decoy,
           sizeof(decoy));
    owner.x34_scale.y = owner_scale;
    memcpy(&stored_decoy, (unsigned char*) &owner + offsetof(Item, scl),
           sizeof(stored_decoy));
    assert(stored_decoy == decoy && stored_decoy != owner_scale);
    owner_gobj.user_data = &owner;
    item.owner = &owner_gobj;
    item.scl = -3.0f;
    attributes.x60_scale = attribute_scale;
    item.xCC_item_attr = &attributes;
    item_gobj.user_data = &item;
    item_gobj.hsd_obj = &model;
    model.scale = (Vec3) { 9.0f, 8.0f, 7.0f };
    model.child = &child;
    model.flags = independent_transform ? JOBJ_MTX_INDEP_SRT : 0;

    itLinkHookshot_ApplyOwnerScale(&item_gobj);

    assert(item.scl == expected);
    assert(model.scale.x == expected && model.scale.y == expected &&
           model.scale.z == expected);
    assert(HSD_JObjMtxIsDirty(&model) == !independent_transform);
    assert(HSD_JObjMtxIsDirty(&child) == !independent_transform);
}

static void check_missing_owner_userdata(void)
{
    HSD_GObj owner_gobj = { 0 };
    HSD_GObj item_gobj = { 0 };
    Item item = { 0 };
    HSD_JObj model = { 0 };
    item.owner = &owner_gobj;
    item.scl = 2.5f;
    item_gobj.user_data = &item;
    item_gobj.hsd_obj = &model;
    model.scale = (Vec3) { 9.0f, 8.0f, 7.0f };
    /* NULL attributes additionally proves the no-owner path does not read it. */
    itLinkHookshot_ApplyOwnerScale(&item_gobj);
    assert(item.scl == 2.5f);
    assert(model.scale.x == 9.0f && model.scale.y == 8.0f &&
           model.scale.z == 7.0f && model.flags == 0);
}

int main(void)
{
    check_owner_scale(1.5f, 0.5f, 0.75f, false);
    check_owner_scale(0.625f, 2.0f, 1.25f, true);
    check_owner_scale(2.0f, 0.75f, 1.5f, false);
    check_owner_scale(0.0f, 1.0f, 0.0f, false);
    check_owner_scale(-0.5f, 3.0f, -1.5f, false);
    check_missing_owner_userdata();
    printf("Hookshot owner scale: 6 cases passed (Item.scl %zu, Fighter.y %zu)\n",
           offsetof(Item, scl), (size_t) OWNER_Y_OFFSET);
    return 0;
}
