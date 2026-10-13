#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/native_holdable.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/cm/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

// Same production constants/context macros as it/inlines.h and MSL/math.h.
#define GET_ITEM(gobj) ((Item*) HSD_GObjGetUserData(gobj))
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 (M_PI / 2)
#endif

void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "%s:%lu: %s\n", file, (unsigned long)line, expression); fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort(); }
void OSReport(char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    va_end(arguments);
}

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static Item item;
static Fighter fighter, victim, replacement;
static HSD_GObj objects[4];
#define fixture_object objects[0]
#define fg objects[1]
#define vg objects[2]
#define rg objects[3]
NativeMatchPool nativeGObjPool;
static NativeMatchPoolSlot object_slots[4];static uint32_t object_free[4];static uint64_t object_epoch;

static HSD_JObj item_joint;
static ItemLogicTable logic;
static unsigned continuation, destroyed, freed, pickup_cleanup, second_grab;
static unsigned effect, integrations, victim_action;
static unsigned callback_calls;
static bool invalidate_owner;
static bool callback_result;
static bool refresh_physics_invalidates;
static unsigned invalidation;
enum LifeStep { LifeReset, LifeCommon, LifeCamera, LifeDeath,
                LifeCpu, LifeQueue, LifeTail1, LifeTail2 };
static enum LifeStep life_steps[8];
static unsigned life_count, life_invalidation_step;
static NativeFighterDefinition life_definition, other_life_definition;
static const NativeFighterDefinition* current_life_definition;
static unsigned cpu_replacement;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ (void)kind; return current_life_definition; }
bool nativeScriptItem_IsAttached(Item_GObj* o){(void)o;return false;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* o){(void)o;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* o){(void)o;return NULL;}
bool nativeScriptItem_View(Item_GObj* o,uint32_t g,NativeScriptReceiverView* v){(void)o;(void)g;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeScriptItem_RunPhase(Item_GObj* o,uint32_t p,NativeScriptValue* v){(void)o;(void)p;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeScriptItem_RunHook(Item_GObj* o,uint32_t h,const NativeScriptValue* a,uint32_t n,NativeScriptValue* v){(void)o;(void)h;(void)a;(void)n;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
bool nativeScriptItem_HookBound(Item_GObj* o,uint32_t h){(void)o;(void)h;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeHoldable_PickedUp(Item_GObj* o,NativeScriptValue* v){(void)o;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeHoldable_Hit(Item_GObj* o,NativeScriptValue* v){(void)o;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeHoldable_Clanked(Item_GObj* o,NativeScriptValue* v){(void)o;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeHoldable_Reflected(Item_GObj* o,NativeScriptValue* v){(void)o;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeHoldable_ShieldBounced(Item_GObj* o,NativeScriptValue* v){(void)o;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeHoldable_Collision(Item_GObj* o,NativeScriptValue* v){(void)o;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
bool nativeScriptFighter_IsAttached(Fighter_GObj* o){(void)o;return false;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* o){(void)o;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind k){(void)k;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
bool nativeScriptFighter_View(Fighter_GObj* o,uint32_t g,NativeScriptReceiverView* v){(void)o;(void)g;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj* o,uint32_t h,const NativeScriptValue* a,uint32_t n, NativeScriptEventRecordIO* record,NativeScriptValue* v){(void)record;(void)o;(void)h;(void)a;(void)n;(void)v;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
bool nativeScriptFighter_CallbackBound(Fighter_GObj* o,uint32_t h){(void)o;(void)h;return false;}
bool nativeScriptFighter_RunCallback(Fighter_GObj* o,uint32_t h){(void)o;(void)h;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
bool nativeScriptFighter_MotionBound(Fighter_GObj* o,uint32_t h){(void)o;(void)h;return false;}
bool nativeScriptFighter_RunMotion(Fighter_GObj* o,uint32_t h){(void)o;(void)h;fprintf(stderr,"Unselected raw fixture leaf: %s\n",__func__);abort();}
HSD_GObjEvent ftData_OnDeath[Ft_Kind_RuntimeCapacity];
Fighter_ItemEvent ftData_OnItemPickupExt[Ft_Kind_RuntimeCapacity];
static bool pickup_argument;

void pl_8003E17C(int player, int port, Item_GObj* held)
{
    assert(player == fighter.player_id && port == fighter.x221F_b4);
    assert(held == &fixture_object);
    ++continuation;
}

void Item_8026A8EC(Item_GObj*);
static void Item_8026A158(HSD_GObj*);
static void Item_8026A1E8(HSD_GObj*);

static void invalidate(HSD_GObj* source)
{
    unsigned link = source == &fixture_object ? HSD_GOBJ_PLINK_ITEM : HSD_GOBJ_PLINK_FIGHTER;
    if (invalidation == 1) {
        heads[link] = NULL;
        source->user_data = (void*)(uintptr_t)1;
    } else if (invalidation == 2) {
        if (source == &fixture_object) ++item.x1C;
        else ++fighter.x8_spawnNum;
    }
}

static bool predicate(HSD_GObj* source)
{ invalidate(source); return callback_result; }
static void event(HSD_GObj* source) { ++callback_calls; invalidate(source); }
static void pickup_event(HSD_GObj* source, bool normal)
{ assert(normal == pickup_argument); ++callback_calls; invalidate(source); }
void Item_802697D4(HSD_GObj* source)
{ ++callback_calls; if (refresh_physics_invalidates) invalidate(source); }
static void destroy_recursive(HSD_GObj* source)
{ ++destroyed; Item_8026A8EC(source); }
static void destroy_foreign(HSD_GObj* source)
{ ++destroyed; invalidate(source); }

// Real dispatch bodies below retain their engine calls. These stubs observe
// continuations and lifecycle order without a game or rendering context.
void it_8027574C(HSD_GObj* g) { (void)g; ++continuation; }
void it_8026C368(HSD_GObj* g) { (void)g; ++continuation; }
void efAsync_QueueFlush(HSD_GObj* g, void* queue)
{ (void)g; (void)queue; }
void it_80271A58(HSD_GObj* g) { (void)g; ++continuation; }
void it_80272280(HSD_GObj* g) { (void)g; ++continuation; }
void it_802722B0(HSD_GObj* g) { (void)g; ++continuation; }
void it_80273168(HSD_GObj* g) { (void)g; }
void it_802742F4(HSD_GObj* g, HSD_GObj* owner, Fighter_Part part)
{ (void)g; (void)owner; (void)part; }
void Item_8026B074(Item* data) { assert(data == &item); ++pickup_cleanup; }
bool it_80272D1C(HSD_GObj* g) { return g == &fixture_object && heads[HSD_GOBJ_PLINK_ITEM] == g; }
void it_802725D4(HSD_GObj* g) { (void)g; }
static void Item_802675A8(HSD_GObj* g) { (void)g; }
static void func_8026A8EC_inline1(HSD_GObj* g) { (void)g; }
void efLib_DestroyAll(HSD_GObj* g) { (void)g; ++effect; }
static void ItemSwitch(HSD_GObj* g) { (void)g; }
bool ftLib_80086960(HSD_GObj* g) { return g != NULL; }
static void DestroyItemInline(HSD_GObj* g, Item* data)
{ (void)g; (void)data; }
static void func_8026A8EC_inline3(HSD_GObj* g) { (void)g; }
void lb_8000FD18(void* desc) { (void)desc; }
void Item_8026B0B4(HSD_GObj* g) { (void)g; }
void nativeArticleModel_Destroy(HSD_GObj* g, unsigned slot) { (void)g; (void)slot; }
void HSD_GObjFree(HSD_GObj* g)
{ assert(g == &fixture_object); NativeMatchPoolReference reference;assert(nativeMatchPool_Reference(&nativeGObjPool,g,&reference)&&nativeMatchPool_Free(&nativeGObjPool,reference)); heads[HSD_GOBJ_PLINK_ITEM] = NULL; g->user_data = (void*)(uintptr_t)1; ++freed; }
void ft_80088770(Fighter* data) { (void)data; }
void ft_800887CC(Fighter* data) { (void)data; }
void ftCo_800C0A98(HSD_GObj* g) { (void)g; }
void ftCommon_IntegrateMovement(Fighter* data) { (void)data; ++integrations; }
void ftColl_GetWindOffsetVec(HSD_GObj* g, Vec3* out)
{ (void)g; *out = (Vec3){1,2,3}; }
void ftColl_80076528(HSD_GObj* g) { (void)g; }
bool mpGetSpeed(int line, Vec3* position, Vec3* out)
{ (void)line; (void)position; *out = (Vec3){0}; return false; }
void C_VECAdd(Vec* a, Vec* b, Vec* out)
{ out->x=a->x+b->x; out->y=a->y+b->y; out->z=a->z+b->z; }
void it_802701BC(HSD_GObj* g) { (void)g; }
bool gm_8016B1C4(void) { return false; }
void ftColl_8007BA0C(HSD_GObj* g) { (void)g; }
void ftColl_80078A2C(HSD_GObj* g) { (void)g; }
void ft_PlaySFX(Fighter* data, int sound, int volume, int pan)
{ (void)data; (void)sound; (void)volume; (void)pan; }
void ftColl_80078754(HSD_GObj* g, HSD_GObj* caught, int mode)
{ (void)g; (void)caught; (void)mode; }
void ftColl_8007BC90(HSD_GObj* g) { (void)g; }
void it_8027B4A4(HSD_GObj* g, HSD_GObj* target)
{ (void)g; (void)target; }
void it_80275BC8(HSD_GObj* g, HSD_GObj* owner) { (void)g; (void)owner; }
void it_80274DAC(HSD_GObj* g) { (void)g; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { (void)joint; }
void Item_8026A848(HSD_GObj* g, HSD_GObj* owner)
{ invalidate(invalidate_owner ? owner : g); }
void it_8027B070(HSD_GObj* g, HSD_GObj* owner)
{ (void)g; (void)owner; ++continuation; }
void mpCollSetFacingDir(CollData* collision, s32 facing)
{ (void)collision; (void)facing; ++continuation; }
void it_802762BC(Item* data) { (void)data; }
static void it_2725_JObjSetTranslate(HSD_JObj* joint, Vec3* position)
{ (void)joint; (void)position; }
float it_802758D4(HSD_GObj* g) { (void)g; return 0; }
void it_8027B378(HSD_GObj* owner, HSD_GObj* g, float value)
{ (void)owner; (void)g; (void)value; }
bool it_80274C78(HSD_GObj* g) { (void)g; return false; }
float it_8026B424(int damage) { (void)damage; return 1.0f; }
static bool Item_80269DC8(HSD_GObj* g) { (void)g; return false; }
static bool Item_80269F14(HSD_GObj* g) { (void)g; return false; }
static void Item_80269CC4(HSD_GObj* g) { (void)g; ++continuation; }
void nativeReplay_LogItemEvent(ItemKind kind, const char* name) { (void)kind; (void)name; }

// Reset intentionally changes the identity, just as production starts a new
// life before the callbacks. The extracted body must snapshot that new life.
static void life_step(enum LifeStep step)
{ assert(life_count < sizeof(life_steps)/sizeof(life_steps[0])); life_steps[life_count++]=step; }
void Fighter_UnkInitReset_80067C98(Fighter* data)
{ assert(data==&fighter); ++data->x8_spawnNum; life_step(LifeReset); }
void ftCo_800D105C(HSD_GObj* g) { (void)g; }
void ftCo_800C09B4(HSD_GObj* g) { (void)g; }
void ftCommon_8007E2FC(HSD_GObj* g) { (void)g; }
void ft_80088A50(Fighter* data) { (void)data; }
void ft_800890BC(Fighter* data) { (void)data; }
void ft_800892D4(Fighter* data) { (void)data; }
void ft_80081B38(HSD_GObj* g) { (void)g; }
void ft_80081938(HSD_GObj* g) { (void)g; }
bool ft_80082A68(HSD_GObj* g) { assert(g==&fg); return true; }
void ftCommon_8007D6A4(Fighter* data)
{
    assert(data==&fighter); life_step(LifeCommon);
    if (life_invalidation_step==LifeCommon) invalidate(&fg);
}
void ftCommon_8007D5D4(Fighter* data) { assert(data==&fighter); }
void ftCamera_80076064(Fighter* data)
{ assert(data==&fighter); life_step(LifeCamera); }
void Fighter_UnkApplyTransformation_8006C0F0(HSD_GObj* g) { (void)g; }
void Fighter_UpdateModelScale(HSD_GObj* g) { (void)g; }
void ftCo_800BFFAC(Fighter* data) { (void)data; }
void ftCo_800C0074(Fighter* data) { (void)data; }
void ftCo_800C8438(HSD_GObj* g) { (void)g; }
void ftCo_800C89A0(HSD_GObj* g) { (void)g; }
void ftCo_800C8FC4(HSD_GObj* g) { (void)g; }
void ftColl_8007AFF8(HSD_GObj* g) { (void)g; }
void ftColl_8007B0C0(HSD_GObj* g, HurtCapsuleState state)
{ assert(g==&fg && state==HurtCapsule_Enabled); }
static void life_death(HSD_GObj* g)
{
    assert(g==&fg); life_step(LifeDeath);
    if (life_invalidation_step==LifeDeath) invalidate(g);
}
int Player_GetCpuType(int player) { assert(player==0); return 4; }
int Player_GetCpuLevel(int player) { assert(player==0); return 9; }
void ftCo_800A101C(Fighter* data, int type, int level, int extra)
{
    assert(data==&fighter && type==4 && level==9 && extra==0);
    life_step(LifeCpu); data->cpu.xC=type; data->cpu.level=level;
    if (cpu_replacement == 1 || cpu_replacement == 2) invalidate(&fg);
    else if (cpu_replacement == 3) { replacement.x8_spawnNum=data->x8_spawnNum; fg.user_data=&replacement; }
    else if (cpu_replacement == 4) data->kind=Ft_Kind_Luigi;
    else if (cpu_replacement == 5) data->ft_data=(ftData*)(uintptr_t)1;
    else if (cpu_replacement == 6) current_life_definition=&other_life_definition;
}
void efAsync_QueueClear(void* queue)
{ if (queue==&fighter.x60C) life_step(LifeQueue); }
void ft_8007C17C(HSD_GObj* g) { assert(g==&fg); life_step(LifeTail1); }
void ft_8007C630(HSD_GObj* g) { assert(g==&fg); life_step(LifeTail2); }
#ifndef HSD_ASSERTREPORT
#define HSD_ASSERTREPORT(line, condition, message) assert(condition)
#endif

#include "callback_engine_fixture.h"

static void grab_first(HSD_GObj* source)
{
    if (victim_action == 1) {
        if (source == &fixture_object) item.grab_victim = &rg;
        else fighter.victim_gobj = &rg;
    } else if (victim_action == 2) ++victim.x8_spawnNum;
    else if (victim_action == 3) {
        fg.next = NULL; vg.user_data = (void*)(uintptr_t)1;
    }
}

static void grabbed(HSD_GObj* target, HSD_GObj* source)
{
    (void)source; assert(target == (victim_action == 1 ? &rg : &vg));
    ++second_grab;
}

static void reset(void)
{
    memset(&item,0,sizeof(item)); memset(&fighter,0,sizeof(fighter));
    memset(&victim,0,sizeof(victim)); memset(&replacement,0,sizeof(replacement));
    memset(&logic,0,sizeof(logic)); memset(heads,0,sizeof(heads));
    assert(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),object_slots,object_free,4,sizeof(objects[0]),_Alignof(HSD_GObj),++object_epoch));
    for(unsigned i=0;i<4;++i){NativeMatchPoolReference allocated;assert(nativeMatchPool_Allocate(&nativeGObjPool,&allocated)==&objects[i]);}
    fixture_object=(HSD_GObj){.user_data=&item,.classifier=HSD_GOBJ_CLASS_ITEM}; fg=(HSD_GObj){.user_data=&fighter,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    item_joint=(HSD_JObj){.flags=JOBJ_MTX_INDEP_SRT}; fixture_object.hsd_obj=&item_joint;
    fg.hsd_obj=&item_joint;
    vg=(HSD_GObj){.user_data=&victim,.classifier=HSD_GOBJ_CLASS_FIGHTER}; rg=(HSD_GObj){.user_data=&replacement,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    fighter.gobj=&fg;victim.gobj=&vg;replacement.gobj=&rg;
    fg.next=&vg; vg.next=&rg;
    heads[HSD_GOBJ_PLINK_ITEM]=&fixture_object; heads[HSD_GOBJ_PLINK_FIGHTER]=&fg;
    HSD_GObjPLinkHead=heads;
    item.x1C=UINT32_C(0xFEDCBA98); item.entity=&fixture_object; item.xB8_itemLogicTable=&logic;
    fighter.x8_spawnNum=UINT32_C(0x80000001); victim.x8_spawnNum=99; replacement.x8_spawnNum=100;
    continuation=destroyed=freed=pickup_cleanup=second_grab=effect=integrations=0;
    callback_calls=0;
    invalidation=0; callback_result=false; victim_action=0;
    refresh_physics_invalidates=false;
    invalidate_owner=false;
    memset(ftData_OnDeath,0,sizeof(ftData_OnDeath));
    memset(ftData_OnItemPickupExt,0,sizeof(ftData_OnItemPickupExt));
    life_count=0; life_invalidation_step=UINT32_MAX;
    cpu_replacement=0; current_life_definition=&life_definition;
}

int main(void)
{
    for (unsigned normal=0; normal<2; ++normal) {
        reset(); pickup_argument=normal; fighter.item_gobj=&fixture_object;
        ftData_OnItemPickupExt[fighter.kind]=pickup_event;
        ftpickupitem_80094818(&fg,normal);
        assert(callback_calls==1 && continuation==1);
        for (unsigned action=1; action<=2; ++action) {
            reset(); pickup_argument=normal; fighter.item_gobj=&fixture_object;
            invalidation=action; ftData_OnItemPickupExt[fighter.kind]=pickup_event;
            ftpickupitem_80094818(&fg,normal);
            assert(callback_calls==1 && !continuation);
        }
    }
    reset(); fighter.item_gobj=&fixture_object; ftpickupitem_80094818(&fg,true);
    assert(!callback_calls && continuation==1);
    reset(); fighter.x1978=&fixture_object; ftData_OnItemPickupExt[fighter.kind]=pickup_event;
    ftpickupitem_80094818(&fg,true);
    assert(!callback_calls && continuation==1);
    reset(); ftpickupitem_80094818(&fg,true); assert(!callback_calls && !continuation);
    reset(); it_802750F8(&fixture_object);
    assert(callback_calls==1 && item.xDCC_flag.b3 && continuation==2);
    for (unsigned action=1; action<=2; ++action) {
        reset(); invalidation=action; refresh_physics_invalidates=true;
        it_802750F8(&fixture_object);
        assert(callback_calls==1 && !continuation && !item.xDCC_flag.b3);
        reset(); invalidation=action; item.collided=predicate;
        it_802750F8(&fixture_object);
        assert(callback_calls==1 && !continuation && !item.xDCC_flag.b3);
    }
    const enum LifeStep healthy_life[]={LifeReset,LifeCommon,LifeCamera,
        LifeDeath,LifeCpu,LifeQueue,LifeTail1,LifeTail2};
    reset(); fighter.smash_attrs.x2135=-1; ftData_OnDeath[fighter.kind]=life_death;
    const uint32_t old_generation=(uint32_t)fighter.x8_spawnNum;
    Fighter_UnkProcessDeath_80068354(&fg);
    assert((uint32_t)fighter.x8_spawnNum==old_generation+1);
    assert(!nativeCallbackSource_Alive(&fg,old_generation,NativeCallbackSource_Fighter));
    assert(nativeCallbackSource_Alive(&fg,old_generation+1,NativeCallbackSource_Fighter));
    assert(life_count==8 && memcmp(life_steps,healthy_life,sizeof(healthy_life))==0);
    assert(fighter.cpu.xC==4 && fighter.cpu.level==9);
    for (unsigned action=1; action<=2; ++action)
        for (unsigned stage=LifeCommon; stage<=LifeDeath; stage+=LifeDeath-LifeCommon) {
            reset(); fighter.smash_attrs.x2135=-1; ftData_OnDeath[fighter.kind]=life_death;
            invalidation=action; life_invalidation_step=stage;
            assert(!Fighter_UnkProcessDeath_80068354(&fg));
            const unsigned expected=stage==LifeCommon ? 2 : 4;
            assert(life_count==expected);
            assert(memcmp(life_steps,healthy_life,expected*sizeof(life_steps[0]))==0);
            assert(!fighter.cpu.xC && !fighter.cpu.level);
        }
    // Also keep the no-callback path healthy: a new generation is ordinary
    // initialization, not evidence that the reset destroyed the fighter.
    reset(); assert(Fighter_UnkProcessDeath_80068354(&fg));
    assert(life_count==6 && life_steps[0]==LifeReset && life_steps[2]==LifeCpu);
    assert(fighter.cpu.xC==4 && fighter.cpu.level==9);
    // Early CPU authoring can retire or replace this new life without allowing
    // reset's effect queue/model tail, or its caller, to keep using stale state.
    for (unsigned changed=1; changed<=6; ++changed) {
        reset(); fighter.smash_attrs.x2135=-1; ftData_OnDeath[fighter.kind]=life_death;
        cpu_replacement=changed; invalidation=changed;
        assert(!Fighter_UnkProcessDeath_80068354(&fg));
        assert(life_count==5 && life_steps[4]==LifeCpu);
    }
    for (unsigned action=1; action<=2; ++action)
        for (unsigned result=0; result<2; ++result) {
            reset(); invalidation=action; callback_result=result;
            assert(processCallback(predicate,&fixture_object,&item));
            assert(!freed && !item.destroy_type);
            reset(); invalidation=action; callback_result=result; item.collided=predicate;
            Item_80269978(&fixture_object); assert(!continuation && !freed);
        }
    reset(); item.pos.x=12; item.collided=predicate; Item_80269978(&fixture_object);
    assert(continuation==2 && item_joint.translate.x==12);
    reset(); callback_result=true; item.collided=predicate;
    Item_80269978(&fixture_object); assert(freed==1 && item.destroy_type==1);
    for (unsigned action=1; action<=2; ++action) {
        reset(); invalidation=action; item.on_accessory=event;
        Item_80269A9C(&fixture_object); assert(!continuation);
        reset(); invalidation=action; logic.picked_up=event;
        Item_8026AB54(&fixture_object,&fg,0); assert(!pickup_cleanup);
        reset(); invalidation=action; logic.entered_air=event;
        Item_8026ADC0(&fixture_object); assert(!item.xDC8_word.flags.x1F);
        reset(); invalidation=action; fighter.take_dmg_cb=event; fighter.death1_cb=event;
        ftCommon_8007DB58(&fg); assert(callback_calls==1);
        reset(); invalidation=action; fighter.phys_cb=event;
        Fighter_UpdatePosition(&fg); assert(!integrations && !fighter.cur_pos.x);
        reset(); invalidation=action; fighter.hitlag_cb=event;
        Fighter_UpdatePosition(&fg); assert(integrations==1 && !fighter.cur_pos.x);
        reset(); invalidation=action; item.entered_hitlag=event;
        Item_8026A158(&fixture_object); assert(!item.xDC8_word.flags.x9 && callback_calls==1);
        reset(); invalidation=action; item.exited_hitlag=event; item.xDC8_word.flags.x9=true;
        Item_8026A1E8(&fixture_object); assert(item.xDC8_word.flags.x9 && callback_calls==1);
        reset(); invalidation=action; item.exited_hitlag=event; item.xDC8_word.flags.x8=true;
        func_8026A1E8_inline(&fixture_object); assert(item.xDC8_word.flags.x8 && callback_calls==1);
        reset(); invalidation=action; item.entered_hitlag=event; item.xCA8=1;
        Item_8026A294(&fixture_object); assert(!continuation && !item.xDC8_word.flags.x9);
        for (unsigned result=0; result<2; ++result) {
            reset(); invalidation=action; callback_result=result; item.xCA0=1;
            logic.dmg_received=predicate; Item_8026A294(&fixture_object);
            assert(!continuation && !freed);
        }
    }
    reset(); logic.destroyed=destroy_recursive; Item_8026A8EC(&fixture_object);
    assert(destroyed==1 && freed==1 && effect==1 && item.native_removal_in_progress);
    reset(); logic.destroyed=destroy_foreign; invalidation=1; Item_8026A8EC(&fixture_object);
    assert(destroyed==1 && !freed);
    for (unsigned owner=0; owner<2; ++owner)
        for (unsigned action=1; action<=2; ++action) {
            reset(); invalidate_owner=owner; invalidation=action;
            it_80273F34(&fixture_object,&fg); assert(!continuation && !pickup_cleanup);
        }
    reset(); it_80273F34(&fixture_object,&fg);
    assert(continuation==2 && pickup_cleanup==1 && item.xD54_throwNum==1);
    for (victim_action=0; victim_action<4; ++victim_action) {
        unsigned action=victim_action; reset(); victim_action=action;
        item.xDD0_flag.b5=true; item.grab_victim=&vg;
        item.grab_dealt=grab_first; item.grabbed_for_victim=grabbed;
        Item_80269BE4(&fixture_object); assert(second_grab==(action<2));
        reset(); victim_action=action; fighter.x221E_b6=true;
        fighter.x2225_b1=true; fighter.victim_gobj=&vg;
        fighter.grab_cb=grab_first; fighter.grabbed_cb=grabbed;
        Fighter_UnkProcessGrab_8006CA5C(&fg); assert(second_grab==(action<2));
    }
    puts("Actual engine dispatch bodies stop after removal/recycling, preserve changed live recipients, and destroy once");
    return 0;
}
