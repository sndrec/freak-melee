#include <melee/ft/native_fighter_target.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/fighter.h>
#include <melee/ft/types.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

static void airborne_lifetime_cases(void)
{
    HSD_GObj** saved = HSD_GObjPLinkHead;
    HSD_GObj* heads[32] = {0};
    Fighter first_data = { .x8_spawnNum = 7, .ground_or_air = GA_Air };
    Fighter second_data = { .x8_spawnNum = 19, .ground_or_air = GA_Ground };
    HSD_GObj second = { .user_data = &second_data };
    HSD_GObj first = { .user_data = &first_data, .next = &second };
    heads[HSD_GOBJ_PLINK_FIGHTER] = &first;
    HSD_GObjPLinkHead = heads;
    assert(nativeFighterTarget_Airborne(7));
    assert(!nativeFighterTarget_Airborne(19));
    second_data.ground_or_air = GA_Air;
    assert(nativeFighterTarget_Airborne(19));
    first_data.ground_or_air = GA_Ground;
    assert(!nativeFighterTarget_Airborne(7));
    assert(!nativeFighterTarget_Airborne(0));
    assert(!nativeFighterTarget_Airborne(777));
    // Removed addresses are not dereferenced, even with poisoned userdata.
    heads[HSD_GOBJ_PLINK_FIGHTER] = &second;
    first.user_data = (void*)1;
    assert(!nativeFighterTarget_Airborne(7));
    assert(nativeFighterTarget_Airborne(19));
    // A live registry node without userdata is skipped safely.
    heads[HSD_GOBJ_PLINK_FIGHTER] = &first;
    first.user_data = NULL;
    assert(!nativeFighterTarget_Airborne(7));
    assert(nativeFighterTarget_Airborne(19));
    first.user_data = &first_data;
    const uint32_t identities[] = {UINT32_C(16777217), UINT32_C(0x80000001), UINT32_MAX};
    for (unsigned i = 0; i < sizeof(identities)/sizeof(*identities); ++i) {
        // Address reuse must stop saved IDs from following the replacement.
        first_data.x8_spawnNum = (s32)identities[i];
        assert(!nativeFighterTarget_Airborne(7));
        for (unsigned air = 0; air < 2; ++air) {
            first_data.ground_or_air = air ? GA_Air : GA_Ground;
            assert(nativeFighterTarget_Airborne(identities[i]) == (air != 0));
            if (i) assert(!nativeFighterTarget_Airborne(identities[i - 1]));
        }
    }
    HSD_GObjPLinkHead = NULL;
    assert(!nativeFighterTarget_Airborne(UINT32_MAX));
    assert(!nativeFighterTarget_Airborne(0));
    HSD_GObjPLinkHead = saved;
}


static void controller_colors(void)
{
    HSD_GObj** saved_heads=HSD_GObjPLinkHead;
    ftCommonData* saved_common=p_ftCommonData;
    HSD_GObj* heads[32]={0};ftCommonData common={0};
    Fighter f={.x8_spawnNum=(s32)UINT32_C(0x80000001),.x61A_controller_index=1};
    HSD_GObj object={.user_data=&f};heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    HSD_GObjPLinkHead=heads;p_ftCommonData=&common;
    common.x6DC_colorsByPlayer[0]=(GXColor){254,1,2,255};GXColor color;
    assert(nativeFighterTarget_ControllerColor(UINT32_C(0x80000001),&color));
    assert(color.r==254 && color.g==1 && color.b==2 && color.a==255);
    assert(!nativeFighterTarget_ControllerColor(7,&color) && color.r==0 && color.a==0);
    f.x61A_controller_index=0;
    assert(!nativeFighterTarget_ControllerColor(UINT32_C(0x80000001),&color));
    f.x61A_controller_index=5;
    assert(!nativeFighterTarget_ControllerColor(UINT32_C(0x80000001),&color));
    f.x61A_controller_index=1;p_ftCommonData=NULL;
    assert(!nativeFighterTarget_ControllerColor(UINT32_C(0x80000001),&color));
    p_ftCommonData=&common;heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;object.user_data=(void*)1;
    assert(!nativeFighterTarget_ControllerColor(UINT32_C(0x80000001),&color));
    assert(!nativeFighterTarget_ControllerColor(0,&color));
    assert(!nativeFighterTarget_ControllerColor(7,NULL));
    HSD_GObjPLinkHead=saved_heads;p_ftCommonData=saved_common;
}

int main(void)
{
    controller_colors();
    airborne_lifetime_cases();
    HSD_GObj* heads[32] = { 0 };
    HSD_GObj** saved = HSD_GObjPLinkHead;
    HSD_GObjPLinkHead = heads;
    Fighter a = { .x8_spawnNum = 7, .motion_id = ftCo_MS_Guard };
    Fighter b = { .x8_spawnNum = 19, .motion_id = ftCo_MS_Wait };
    a.x34_scale.y = 1.5f;
    a.co_attrs.model_scaling = 2.0f;
    HSD_GObj second = { .user_data = &b };
    HSD_GObj first = { .user_data = &a, .next = &second };
    Item item = { .xCF4_fighterGObjUnk = &first };
    HSD_GObj source = { .user_data = &item };
    heads[HSD_GOBJ_PLINK_FIGHTER] = &first;
    assert(nativeFighterTarget_Contact(&source) == 7);
    assert(nativeFighterTarget_IsFighter(7));
    assert(nativeFighterTarget_Motion(7) == ftCo_MS_Guard);
    assert(nativeFighterTarget_Motion(19) == ftCo_MS_Wait);
    assert(nativeFighterTarget_Motion(0) == -1);
    assert(nativeFighterTarget_Motion(12345) == -1);
    assert(nativeFighterTarget_GuardActive(7));
    assert(!nativeFighterTarget_GuardActive(19));
    assert(nativeFighterTarget_ModelScale(7) == 3.0f);
    assert(nativeFighterTarget_InstanceScaleY(7) == 1.5f);
    b.x34_scale.y = 0.75f;
    assert(nativeFighterTarget_InstanceScaleY(19) == 0.75f);
    assert(nativeFighterTarget_InstanceScaleY(0) == 1.0f);
    item.owner = &first;
    item.xC64_reflectGObj = &second;
    assert(nativeFighterTarget_CurrentOwner(&source) == 7);
    assert(nativeFighterTarget_InstanceScaleY(nativeFighterTarget_CurrentOwner(&source)) == 1.5f);
    item.owner = &second;
    assert(nativeFighterTarget_InstanceScaleY(nativeFighterTarget_CurrentOwner(&source)) == 0.75f);
    item.owner = &first;
    assert(nativeFighterTarget_Reflector(&source) == 19);
    item.owner = (HSD_GObj*)1;
    item.xC64_reflectGObj = &source;
    assert(nativeFighterTarget_CurrentOwner(&source) == 0);
    assert(nativeFighterTarget_Motion(nativeFighterTarget_CurrentOwner(&source)) == -1);
    assert(nativeFighterTarget_InstanceScaleY(nativeFighterTarget_CurrentOwner(&source)) == 1.0f);
    assert(nativeFighterTarget_Reflector(&source) == 0);
    item.xCF4_fighterGObjUnk = &source;
    assert(nativeFighterTarget_Contact(&source) == 0);
    item.xCF4_fighterGObjUnk = NULL;
    assert(nativeFighterTarget_Contact(&source) == 0);
    assert(nativeFighterTarget_Contact(NULL) == 0);
    item.xCF4_fighterGObjUnk = &first;
    heads[HSD_GOBJ_PLINK_FIGHTER] = &second;
    // The removed address may already contain invalid userdata. Resolution
    // must inspect only the live registry, never a retained owner pointer.
    first.user_data = (void*)1;
    item.owner = &first;
    assert(nativeFighterTarget_Motion(7) == -1);
    assert(nativeFighterTarget_Motion(nativeFighterTarget_CurrentOwner(&source)) == -1);
    assert(nativeFighterTarget_Contact(&source) == 0);
    assert(!nativeFighterTarget_IsFighter(7));
    assert(!nativeFighterTarget_GuardActive(7));
    assert(nativeFighterTarget_ModelScale(7) == 0);
    assert(nativeFighterTarget_InstanceScaleY(7) == 1.0f);
    Vec3 center = { 1, 2, 3 };
    assert(nativeFighterTarget_Shield(7, &center) == 0);
    assert(center.x == 0 && center.y == 0 && center.z == 0);
    center = (Vec3) { 1, 2, 3 };
    assert(!nativeFighterTarget_CameraPosition(7, &center));
    assert(center.x == 0 && center.y == 0 && center.z == 0);
    assert(!nativeFighterTarget_CameraPosition(19, NULL));
    // A recycled object address has a new creation number; saved targets do
    // not silently follow the replacement, even across signed-u32 boundaries.
    heads[HSD_GOBJ_PLINK_FIGHTER] = &first;
    first.user_data = &a;
    a.x8_spawnNum = (s32)0xFFFFFFF1u;
    a.motion_id = ftCo_MS_Fall;
    assert(nativeFighterTarget_Motion(7) == -1);
    assert(nativeFighterTarget_Motion(0xFFFFFFF1u) == ftCo_MS_Fall);
    assert(!nativeFighterTarget_IsFighter(7));
    assert(nativeFighterTarget_Contact(&source) == 0xFFFFFFF1u);
    assert(nativeFighterTarget_IsFighter(0xFFFFFFF1u));
    assert(nativeFighterTarget_InstanceScaleY(7) == 1.0f);
    assert(nativeFighterTarget_InstanceScaleY(0xFFFFFFF1u) == 1.5f);
    assert(!nativeFighterTarget_IsFighter(0));
    assert(nativeFighterTarget_Shield(0, NULL) == 0);
    // IDs above binary32's exact range, the high bit, and all bits set are
    // ordinary integer identities here; none may round or become a pointer.
    const uint32_t ids[] = { UINT32_C(16777217), UINT32_C(2147483649), UINT32_MAX };
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        a.x8_spawnNum = (s32)ids[i];
        a.motion_id = ftCo_MS_Wait + (int)i;
        assert(nativeFighterTarget_Motion(ids[i]) == a.motion_id);
        assert(nativeFighterTarget_Motion(0xFFFFFFF1u) == -1);
        for (int mode = 0; mode < 5; ++mode) {
            a.smash_attrs.state = mode;
            assert(nativeFighterTarget_SmashChargeActive(ids[i]) == (mode == 2));
        }
        assert(!nativeFighterTarget_SmashChargeActive(0xFFFFFFF1u));
    }
    first.user_data = NULL;
    assert(!nativeFighterTarget_SmashChargeActive(UINT32_MAX));
    first.user_data = &a;
    assert(!nativeFighterTarget_SmashChargeActive(0));
    // The held slot is observed independently of creator or current damage
    // owner. Retained addresses must be found in the item registry first.
    Item held_item = { .x1C = UINT32_C(2147483649), .kind = It_Kind_Capsule,
                       .msid = 2, .x5CC_currentAnimFrame = 16.5f };
    HSD_GObj held_object = { .user_data = &held_item };
    heads[HSD_GOBJ_PLINK_ITEM] = &held_object;
    a.item_gobj = &held_object;
    assert(nativeFighterTarget_HeldId(UINT32_MAX) == (int32_t)held_item.x1C);
    assert(nativeFighterTarget_HeldKind(UINT32_MAX) == It_Kind_Capsule);
    assert(nativeFighterTarget_HeldMotion(UINT32_MAX) == 2);
    assert(nativeFighterTarget_HeldFrame(UINT32_MAX) == 16.5f);
    held_item.x5CC_currentAnimFrame = 15.99f;
    assert(nativeFighterTarget_HeldFrame(UINT32_MAX) == 15.99f);
    held_item.native_removal_in_progress = true;
    assert(nativeFighterTarget_HeldId(UINT32_MAX) == -1);
    held_item.native_removal_in_progress = false;
    heads[HSD_GOBJ_PLINK_ITEM] = NULL;
    held_object.user_data = (void*)1;
    assert(nativeFighterTarget_HeldKind(UINT32_MAX) == -1);
    assert(nativeFighterTarget_HeldMotion(UINT32_MAX) == -1);
    assert(nativeFighterTarget_HeldFrame(UINT32_MAX) == 0);
    a.item_gobj = NULL;
    assert(nativeFighterTarget_HeldId(UINT32_MAX) == -1);
    assert(nativeFighterTarget_HeldId(0) == -1);
    assert(nativeFighterTarget_HeldMotion(7) == -1);
    HSD_GObjPLinkHead = NULL;
    assert(!nativeFighterTarget_SmashChargeActive(UINT32_MAX));
    assert(nativeFighterTarget_HeldFrame(UINT32_MAX) == 0);
    assert(nativeFighterTarget_Motion(UINT32_MAX) == -1);
    HSD_GObjPLinkHead = saved;
    puts("fighter target lifetime and shared query checks passed");
    return 0;
}
