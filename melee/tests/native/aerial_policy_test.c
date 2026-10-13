// Actual production policy units + original Peach bodies, controlled float entry.
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_contact.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_bindings.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <melee/ft/kinds/ftCommon/ftCo_AerialAction.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobj.h>
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, stock_pairs, authored_cases, contact_cases, float_entries;
static unsigned callback_calls, hazard;
static unsigned recursive_cases;
static bool callback_result;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static HSD_GObj objects[2];
#define source objects[0]
#define recipient objects[1]
NativeMatchPool nativeGObjPool;
static NativeMatchPoolSlot slots[2];static uint32_t free_order[2];
static NativeMatchPoolReference source_ref,recipient_ref;static uint64_t epoch;
static Fighter *owner, *other;
static ftCommonData common;
ftCommonData* p_ftCommonData = &common;
static NativeFighterDefinition definition, replacement;
static const NativeFighterDefinition* definitions[Ft_Kind_CustomFirst + 3];
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"check failed %s:%d: %s\n",__FILE__,__LINE__,#x); exit(1); } } while (0)

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    CHECK((unsigned)kind < sizeof(definitions) / sizeof(definitions[0]));
    return definitions[kind];
}
bool nativeModelEffect_Snapshot(HSD_GObj* object, uint32_t* generation)
{ (void)object; (void)generation; CHECK(false); return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object, uint32_t generation)
{ (void)object; (void)generation; CHECK(false); return NULL; }
void ftPe_8011BB6C(HSD_GObj* object, bool initialize)
{
    CHECK(object == &source && initialize);
    CHECK(nativeCallbackSource_Alive(object, (uint32_t) owner->x8_spawnNum,
                                    NativeCallbackSource_Fighter));
    ++float_entries;
    owner->u.pe.has_float = false;
    owner->motion_id = 361;
}

// Extracted unchanged functions; only their two externally visible names differ.
#include "aerial_policy_originals.h"

static void access_page(Fighter* fighter, DWORD access)
{
    DWORD old;
    CHECK(VirtualProtect(fighter, sizeof(*fighter), access, &old));
}
// This legacy C-policy fixture has no interpreted actor or selected source.
bool nativeScriptFighter_IsAttached(Fighter_GObj* o){(void)o;return false;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind k){(void)k;return NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* o){(void)o;return NativeScriptVM_Error;}
bool nativeScriptFighter_View(Fighter_GObj* o,uint32_t g,NativeScriptReceiverView* v){(void)o;(void)g;(void)v;return false;}
NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj* o,uint32_t h,const NativeScriptValue* a,uint32_t n, NativeScriptEventRecordIO* record,NativeScriptValue* r){(void)record;(void)o;(void)h;(void)a;(void)n;(void)r;CHECK(false);return NativeScriptVM_Error;}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k){(void)k;return NULL;}
const NativeScriptMoveLayout* nativeScriptBindings_SourceFighterLayout(NativeScriptRuntimeContext* c,NativeScriptDiagnostic* d){(void)c;(void)d;CHECK(false);return NULL;}
static void reset(void)
{
    access_page(owner, PAGE_READWRITE); access_page(other, PAGE_READWRITE);
    memset(owner,0,sizeof(*owner)); memset(other,0,sizeof(*other));
    memset(heads,0,sizeof(heads)); memset(definitions,0,sizeof(definitions));
    memset(&definition,0,sizeof(definition)); memset(&replacement,0,sizeof(replacement));
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),slots,free_order,2,sizeof(HSD_GObj),_Alignof(HSD_GObj),++epoch));
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&source_ref)==&source);
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&recipient_ref)==&recipient);
    source = (HSD_GObj){ .classifier=HSD_GOBJ_CLASS_FIGHTER,.user_data = owner, .next = &recipient };
    recipient = (HSD_GObj){ .classifier=HSD_GOBJ_CLASS_FIGHTER,.user_data = other };
    owner->gobj=&source;other->gobj=&recipient;
    HSD_GObjPLinkHead = heads; heads[HSD_GOBJ_PLINK_FIGHTER] = &source;
    owner->x8_spawnNum = (int32_t)0xFEDCBA98u;
    other->x8_spawnNum = (int32_t)0x80000001u;
    owner->kind = Ft_Kind_CustomFirst;
    other->kind = Ft_Kind_CustomFirst + 1;
    owner->unk_gobj = &recipient; owner->x221C_b5 = true;
    common.x88 = 0.7f; common.tap_jump_threshold = 0.8f;
    callback_calls = float_entries = 0;
}

static bool decision(HSD_GObj* object)
{
    CHECK(object == &source || object == &recipient); ++callback_calls;
    switch (hazard) {
    case 0: break;
    case 1: // Retire source and make every cached owner field inaccessible.
        CHECK(nativeMatchPool_Free(&nativeGObjPool,source_ref));
        heads[HSD_GOBJ_PLINK_FIGHTER] = &recipient;
        access_page(owner,PAGE_NOACCESS); source.user_data = (void*)1; break;
    case 2: ++owner->x8_spawnNum; break;
    case 3: owner->kind = Ft_Kind_Mario; break;
    case 4: definitions[Ft_Kind_CustomFirst] = &replacement; break;
    case 5: definitions[Ft_Kind_CustomFirst] = NULL; break;
    case 6: source.user_data = NULL; break;
    case 7: // Retire recipient; subsequent registry guard must precede its reads.
        CHECK(nativeMatchPool_Free(&nativeGObjPool,recipient_ref));
        source.next = NULL;
        access_page(other,PAGE_NOACCESS); recipient.user_data = (void*)1; break;
    case 8: ++other->x8_spawnNum; break;
    case 9: other->kind = Ft_Kind_Mario; break;
    case 10: definitions[Ft_Kind_CustomFirst+1] = &replacement; break;
    case 11: definitions[Ft_Kind_CustomFirst+1] = NULL; break;
    case 12: recipient.user_data = NULL; break;
    case 13: owner->unk_gobj = &source; break;
    default: CHECK(false);
    }
    return callback_result;
}

static void stock_aerial(void)
{
    float stick[] = { -INFINITY,-1,-0.7f,nextafterf(-0.7f,INFINITY),0,
        nextafterf(0.8f,-INFINITY),0.8f,1,INFINITY,NAN };
    float speed[] = { -INFINITY,-1,-0.0f,0,nextafterf(0,INFINITY),1,INFINITY,NAN };
    unsigned buttons[] = { 0,HSD_PAD_X,HSD_PAD_Y,HSD_PAD_A,HSD_PAD_XY,
                           HSD_PAD_X|HSD_PAD_A };
    for (unsigned kind=0;kind<Ft_Kind_CustomFirst+2;++kind)
    for (unsigned available=0;available<2;++available)
    for (unsigned y=0;y<sizeof(stick)/sizeof(*stick);++y)
    for (unsigned v=0;v<sizeof(speed)/sizeof(*speed);++v)
    for (unsigned b=0;b<sizeof(buttons)/sizeof(*buttons);++b)
    for (unsigned automatic=0;automatic<2;++automatic) {
        reset(); owner->kind=kind; owner->u.pe.has_float=available;
        owner->self_vel.y=speed[v]; owner->input.lstick[0].y=stick[y];
        owner->input.held_buttons[0]=buttons[b]; owner->motion_id=17;
        Fighter initial=*owner;
        bool old=automatic?original_automatic(&source):original_manual(&source);
        unsigned entries=float_entries; Fighter expected=*owner;
        *owner=initial; float_entries=0;
        bool current=automatic?ftCo_AerialAction_CheckAutomatic(&source):
                               ftCo_AerialAction_CheckManual(&source);
        CHECK(old==current); CHECK(float_entries==entries);
        CHECK(memcmp(owner,&expected,sizeof(expected))==0); ++stock_pairs;
    }
}

static void authored_aerial(void)
{
    for (unsigned automatic=0;automatic<2;++automatic)
    for (unsigned result=0;result<2;++result)
    for (hazard=0;hazard<=6;++hazard) {
        reset(); definitions[owner->kind]=&definition;
        definition.on_aerial_action_manual=decision;
        definition.on_aerial_action_automatic=decision;
        callback_result=result;
        bool consumed=automatic?ftCo_AerialAction_CheckAutomatic(&source):
                                ftCo_AerialAction_CheckManual(&source);
        CHECK(consumed == (result || hazard!=0));
        CHECK(callback_calls==1 && float_entries==0); ++authored_cases;
    }
    // An authored false decision overrides stock Peach input; no fallback entry.
    for (unsigned automatic=0;automatic<2;++automatic) {
        reset(); owner->kind=Ft_Kind_Peach; owner->u.pe.has_float=true;
        owner->input.held_buttons[0]=HSD_PAD_XY;
        owner->input.lstick[0].y=-1; definitions[owner->kind]=&definition;
        definition.on_aerial_action_manual=decision;
        definition.on_aerial_action_automatic=decision;
        hazard=0; callback_result=false;
        CHECK(!(automatic?ftCo_AerialAction_CheckAutomatic(&source):
                            ftCo_AerialAction_CheckManual(&source)));
        CHECK(callback_calls==1 && float_entries==0); ++authored_cases;
    }
    for (unsigned automatic=0;automatic<2;++automatic) {
        reset(); CHECK(nativeMatchPool_Free(&nativeGObjPool,source_ref));
        heads[HSD_GOBJ_PLINK_FIGHTER]=NULL; source.user_data=(void*)1;
        CHECK(automatic?ftCo_AerialAction_CheckAutomatic(&source):
                         ftCo_AerialAction_CheckManual(&source));
        CHECK(float_entries==0); ++authored_cases;
        reset(); source.user_data=NULL;
        CHECK(automatic?ftCo_AerialAction_CheckAutomatic(&source):
                         ftCo_AerialAction_CheckManual(&source)); ++authored_cases;
    }
}

static void contacts(void)
{
    for (unsigned kind=0;kind<Ft_Kind_CustomFirst+2;++kind)
    for (int motion=340;motion<=380;++motion) {
        reset(); other->kind=kind; other->motion_id=motion;
        bool expected=(kind==Ft_Kind_Mars || kind==Ft_Kind_Emblem)?
            (motion==369 || motion==371):kind==Ft_Kind_Peach &&
            (motion==365 || motion==367);
        bool allowed=!expected;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)==NativeScriptVM_Ok&&allowed==expected); ++contact_cases;
    }
    for (unsigned result=0;result<2;++result)
    for (hazard=0;hazard<=13;++hazard) {
        reset(); definitions[owner->kind]=&definition;
        definitions[other->kind]=&definition;
        definition.on_body_rebound=decision; callback_result=result;
        bool allowed=!result;NativeScriptVMStatus state=nativeFighterContact_ReboundAllowed(&source,&allowed);
        CHECK(hazard ? state!=NativeScriptVM_Ok&&allowed==!result : state==NativeScriptVM_Ok&&allowed==result);
        CHECK(callback_calls==1); ++contact_cases;
    }
    reset(); owner->x221C_b5=false; recipient.user_data=(void*)1;
    {bool allowed=false;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)==NativeScriptVM_Ok&&allowed);} ++contact_cases;
    reset(); owner->unk_gobj=NULL;
    {bool allowed=false;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)==NativeScriptVM_Ok&&allowed);} ++contact_cases;
    reset(); source.next=NULL; owner->unk_gobj=(HSD_GObj*)1;
    {bool allowed=true;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)!=NativeScriptVM_Ok&&allowed);} ++contact_cases;
    reset(); CHECK(nativeMatchPool_Free(&nativeGObjPool,source_ref));
    heads[HSD_GOBJ_PLINK_FIGHTER]=NULL; source.user_data=(void*)1;
    {bool allowed=true;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)!=NativeScriptVM_Ok&&allowed);} ++contact_cases;
    reset(); recipient.user_data=NULL;
    {bool allowed=true;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)!=NativeScriptVM_Ok&&allowed);} ++contact_cases;
}

static bool recursive_contact(HSD_GObj* object)
{
    CHECK(object == &recipient); ++callback_calls;
    bool allowed=false;return nativeFighterContact_ReboundAllowed(&source,&allowed)==NativeScriptVM_Ok&&allowed;
}

static void recursive_contacts(void)
{
    // Recursion stops at the shared dispatch bound and leaves depth balanced.
    for (unsigned pass=0;pass<2;++pass) {
        reset(); definitions[other->kind]=&definition;
        definition.on_body_rebound=recursive_contact;
        {bool allowed=true;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)==NativeScriptVM_Ok&&!allowed);}
        CHECK(callback_calls==64); ++recursive_cases;
        for (unsigned depth=0;depth<64;++depth) CHECK(nativeCallbackDispatch_Enter());
        CHECK(!nativeCallbackDispatch_Enter());
        for (unsigned depth=0;depth<64;++depth) nativeCallbackDispatch_Leave();
    }
    // Existing synchronous dispatch nesting shares the same budget.
    reset(); definitions[other->kind]=&definition;
    definition.on_body_rebound=recursive_contact;
    for (unsigned depth=0;depth<63;++depth) CHECK(nativeCallbackDispatch_Enter());
    {bool allowed=true;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)==NativeScriptVM_Ok&&!allowed);} CHECK(callback_calls==1);
    CHECK(nativeCallbackDispatch_Enter()); CHECK(!nativeCallbackDispatch_Enter());
    for (unsigned depth=0;depth<64;++depth) nativeCallbackDispatch_Leave();
    ++recursive_cases;
    definition.on_body_rebound=decision; hazard=0; callback_result=true;
    callback_calls=0;
    {bool allowed=false;CHECK(nativeFighterContact_ReboundAllowed(&source,&allowed)==NativeScriptVM_Ok&&allowed);} CHECK(callback_calls==1);
    ++recursive_cases;
}

int main(void)
{
    owner=VirtualAlloc(NULL,sizeof(*owner),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    other=VirtualAlloc(NULL,sizeof(*other),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    CHECK(owner && other);
    stock_aerial(); authored_aerial(); contacts(); recursive_contacts();
    printf("{\"checks\":%u,\"stock_aerial_pairs\":%u,\"authored_aerial_cases\":%u,\"contact_cases\":%u,\"recursive_contact_cases\":%u}\n",
           checks,stock_pairs,authored_cases,contact_cases,recursive_cases);
    VirtualFree(owner,0,MEM_RELEASE); VirtualFree(other,0,MEM_RELEASE);
    return 0;
}
