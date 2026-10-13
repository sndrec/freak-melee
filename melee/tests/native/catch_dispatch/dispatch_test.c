// Full extracted common bodies; controlled engine boundaries are named below.
#include <melee/ft/fighter.h>
#include <melee/ft/inlines.h>
#include <melee/ft/types.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_capture.h>
#include <melee/ft/kinds/ftYoshi/types.h>
#include <melee/ft/kinds/ftYoshi/ftyoshi.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftCo_CapturePulled.h>
#include <melee/ft/kinds/ftCommon/ftCo_Throw.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/it/inlines.h>
#include <melee/it/types.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/ef/efasync.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <windows.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fighter *fighter, *victim;
static HSD_GObj *object, *victim_object;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static struct ftCommonData common;
struct ftCommonData* p_ftCommonData=&common;
static NativeFighterDefinition definition, victim_definition, replacement_definition;
static const NativeFighterDefinition *active_definition, *active_victim_definition;
static FighterKind definition_kind,victim_definition_kind;
static struct ftData data;
static struct ftData_x8 data8;
static ftYoshiAttributes yattrs;
static Item article;
static HSD_GObj article_object;
static HSD_JObj root_joint, endpoint_joint;
static FighterBone fighter_parts[1],victim_parts[1];
static Vec3 anchor_position, victim_position;
static size_t fighter_bytes, object_bytes;
static bool item_priority, frames, legacy_link_frame, legacy_samus_frame, directional_throw;
static bool callback_result, capture_callbacks;
static float callback_frame;
static int hook_id, hazard, boundary_stage;
static void mutate(void);
static unsigned checks, cases, hook_calls, invalidation_cases, baseline_rejections;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"check failed: %s stage=%s kind=%u mask=%u id=%u old=%u hook=%d hazard=%d boundary=%d\n",#x,stage,diagnostic_kind,diagnostic_mask,diagnostic_id,diagnostic_old,hook_id,hazard,boundary_stage);abort(); } } while(0)
enum { ItemPriority=1, LinkFrame, SamusFrame, LegacyLinkExit, LegacySamusExit,
       LegacyLinkRelease, LegacySamusRelease, Motion, RegisterCapture, Wait,
       Fall, Pummel, DirectionalThrow, HookBase=32, Effect=64, CapturedSetup,
       CapturedEnter, CaptureFlags, HurtFlags, RetractLink, RetractSamus,
       WorldRead, CutFollow, PickupTarget, PickupFinish, AnimationRate, AnimationStart };
static struct Trace { int events[64]; unsigned count; int motion;
    float frame, rate; } trace;
static unsigned char reused_fighter[sizeof(Fighter)], reused_object[sizeof(HSD_GObj)];
static unsigned char reused_victim[sizeof(Fighter)], reused_victim_object[sizeof(HSD_GObj)];

static const char* stage="init";
static unsigned diagnostic_kind,diagnostic_mask,diagnostic_id,diagnostic_old;
static LONG WINAPI fault(EXCEPTION_POINTERS* info)
{
    fprintf(stderr,"fault code=%08lx stage=%s kind=%u mask=%u id=%u old=%u hook=%d hazard=%d boundary=%d\n",
        info->ExceptionRecord->ExceptionCode,stage,diagnostic_kind,diagnostic_mask,
        diagnostic_id,diagnostic_old,hook_id,hazard,boundary_stage);
    return EXCEPTION_EXECUTE_HANDLER;
}
static void record(int event)
{ CHECK(trace.count<64); trace.events[trace.count++]=event; }
static void alive(HSD_GObj* g)
{ CHECK(nativeCallbackSource_Alive(g,g==object?77:177,NativeCallbackSource_Fighter)); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    if (kind==definition_kind) return active_definition;
    if (kind==victim_definition_kind) return active_victim_definition;
    return NULL;
}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n) { (void)g;(void)n;abort(); }
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n) { (void)g;(void)n;abort(); }
bool ftCo_800951D0(Fighter_GObj* g) { alive(g);record(ItemPriority);return item_priority; }
bool ftCo_80095254(Fighter_GObj* g) { alive(g);record(ItemPriority);return item_priority; }
bool ftCo_800952DC(Fighter_GObj* g) { alive(g);record(ItemPriority);return item_priority; }
bool fn_800D8EC8(Fighter_GObj* g) { alive(g);record(LinkFrame);return legacy_link_frame; }
bool fn_800D9228(Fighter_GObj* g) { alive(g);record(LinkFrame);return legacy_link_frame; }
bool fn_800D9558(Fighter_GObj* g) { alive(g);record(SamusFrame);return legacy_samus_frame; }
bool fn_800D9930(Fighter_GObj* g) { alive(g);record(SamusFrame);return legacy_samus_frame; }
void fn_800D949C(Fighter_GObj* g) { alive(g);record(LegacyLinkExit); }
void fn_800D9C64(Fighter_GObj* g) { alive(g);record(LegacySamusExit); }
void it_802A7AAC(Fighter_GObj* g) { alive(g);record(LegacyLinkRelease); }
void it_802BAC3C(Fighter_GObj* g) { alive(g);record(LegacySamusRelease); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* g) { alive(g);return frames; }
void ft_8008A2BC(Fighter_GObj* g) { alive(g);record(Wait); }
void ftCo_Fall_Enter(Fighter_GObj* g) { alive(g);record(Fall); }
void ftAnim_SetAnimRate(Fighter_GObj* g,float rate) { alive(g);record(AnimationRate);trace.rate=rate; }
void ftAnim_8006EBA4(Fighter_GObj* g) { alive(g);record(AnimationStart); }
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId motion,MotionFlags flags,
    float frame,float rate,float blend,Fighter_GObj* other)
{
    alive(g); record(Motion); trace.motion=motion;trace.frame=frame;trace.rate=rate;
    Fighter* f=g->user_data;f->motion_id=motion;f->cur_anim_frame=frame;
    (void)flags;(void)blend;(void)other;
    if(boundary_stage==1)mutate();
}
void ftCommon_8007E2D0(Fighter* f,s16 mode,HSD_GObjEvent grab,
    HSD_GObjEvent pickup_cb,void (*caught)(HSD_GObj*,HSD_GObj*))
{ CHECK(f==fighter&&mode==1&&grab&&pickup_cb&&caught);alive(object);record(RegisterCapture); }
void fn_800DAADC(Fighter_GObj* a,Fighter_GObj* b) { (void)a;(void)b;abort(); }
void ftpickupitem_800948A8(Fighter_GObj* g,Item_GObj* target)
{ alive(g);(void)target;record(PickupTarget); }
void ftpickupitem_80094694(Fighter_GObj* g,FtMotionId kind,bool flags)
{ alive(g);CHECK(kind==0x5D&&flags==1);record(PickupFinish); }
void it_802A7840(Item_GObj* g) { CHECK(g==&article_object);record(RetractLink); }
void it_802BAA94(Item_GObj* g) { CHECK(g==&article_object);record(RetractSamus); }
void fn_800DA4FC(Fighter_GObj* g) { alive(g);record(Pummel);if(boundary_stage==2)mutate(); }
bool ftCo_800DD1E4(Fighter_GObj* g) { alive(g);record(DirectionalThrow);if(boundary_stage==3)mutate();return directional_throw; }
void ftCo_800DA698(Fighter_GObj* g,bool forced) { alive(g);CHECK(forced);record(CutFollow); }
Fighter_Part ftParts_GetBoneIndex(Fighter* f,Fighter_Part p)
{ CHECK(f==victim&&p==FtPart_XRotN);return 0; }
void lb_8000B1CC(HSD_JObj* j,Vec3* offset,Vec3* out)
{ CHECK(!offset&&out);record(WorldRead);*out=j==&endpoint_joint?anchor_position:victim_position; }
void efAsync_Spawn(HSD_GObj* g,void* queue,u32 kind,u32 id,HSD_JObj* joint,...)
{ alive(g);CHECK(queue&&kind==1&&id==0x41D&&joint==&root_joint);record(Effect); }
void ftCommon_8007EBAC(Fighter* f,u32 mode,u32 value)
{ CHECK(f==fighter||f==victim);CHECK(mode==3&&!value);alive(f==fighter?object:victim_object);record(CaptureFlags); }
void ftCommon_8007E2F4(Fighter* f,s16 bits)
{ CHECK(f==fighter||f==victim);CHECK(bits==0x1FF);alive(f==fighter?object:victim_object);record(HurtFlags); }
float ftYs_Init_8012BAC0(Fighter* f) { CHECK(f==fighter);return 13.0f; }
void nativeCapture_EncloseCaught(Fighter_GObj* g,float radius)
{ CHECK(g==object&&radius==13);record(CapturedSetup); }
void ftCo_800DB368(Fighter* holder,Fighter* caught)
{ CHECK(holder==fighter&&caught==victim);record(CapturedEnter); }

#include "new_bodies.h"
#include "old_bodies.h"

static void protect(Fighter* f,HSD_GObj* g,DWORD mode)
{ DWORD prior;CHECK(VirtualProtect(f,fighter_bytes,mode,&prior));CHECK(VirtualProtect(g,object_bytes,mode,&prior)); }
static void mutate(void)
{
    if(hazard==1) { heads[HSD_GOBJ_PLINK_FIGHTER]=victim_object;protect(fighter,object,PAGE_NOACCESS); }
    if(hazard==2) {
        memset(fighter,0xA5,sizeof(*fighter));fighter->x8_spawnNum=78;
        memset(object,0x5A,sizeof(*object));object->user_data=fighter;object->next=victim_object;
        memcpy(reused_fighter,fighter,sizeof(*fighter));memcpy(reused_object,object,sizeof(*object));
    }
    if(hazard==3) fighter->kind++;
    if(hazard==4) active_definition=&replacement_definition;
    if(hazard==5) { object->next=NULL;protect(victim,victim_object,PAGE_NOACCESS); }
    if(hazard==6) {
        memset(victim,0xA5,sizeof(*victim));victim->x8_spawnNum=178;
        memset(victim_object,0x5A,sizeof(*victim_object));victim_object->user_data=victim;victim_object->next=NULL;
        memcpy(reused_victim,victim,sizeof(*victim));memcpy(reused_victim_object,victim_object,sizeof(*victim_object));
    }
    if(hazard==7) { if(hook_id>=10) victim->victim_gobj=NULL; else fighter->victim_gobj=NULL; }
}
static bool hook(Fighter_GObj* g)
{
    CHECK(g==object);alive(g);++hook_calls;record(HookBase+hook_id);
    if(hook_id==2) CHECK(fighter->x74_self_accel.x==0&&fighter->x74_self_accel.y==0&&fighter->x74_self_accel.z==0);
    if(hook_id==6) { CHECK(fighter->motion_id==ftCo_MS_Catch||fighter->motion_id==ftCo_MS_CatchDash);fighter->mv.co.capturedamage.x18=&endpoint_joint; }
    if(hook_id==8) CHECK(fighter->motion_id==ftCo_MS_CatchPull||fighter->motion_id==ftCo_MS_CatchDashPull);
    if(!boundary_stage)mutate();return callback_result;
}
static void void_hook(Fighter_GObj* g) { (void)hook(g); }
static float frame_hook(Fighter_GObj* g) { (void)hook(g);return callback_frame; }
static void capture_hook(Fighter_GObj* holder,Fighter_GObj* caught)
{ CHECK(holder==object&&caught==victim_object);(void)hook(holder); }
static void reset(FighterKind kind,bool present)
{
    protect(fighter,object,PAGE_READWRITE);protect(victim,victim_object,PAGE_READWRITE);
    memset(fighter,0,sizeof(*fighter));memset(victim,0,sizeof(*victim));
    memset(object,0,sizeof(*object));memset(victim_object,0,sizeof(*victim_object));
    memset(&definition,0,sizeof(definition));memset(&victim_definition,0,sizeof(victim_definition));
    memset(&replacement_definition,0,sizeof(replacement_definition));memset(&trace,0,sizeof(trace));
    fighter->kind=kind;fighter->x8_spawnNum=77;victim->kind=Ft_Kind_CustomFirst+3;victim->x8_spawnNum=177;
    object->user_data=fighter;object->next=victim_object;victim_object->user_data=victim;
    heads[HSD_GOBJ_PLINK_FIGHTER]=object;fighter->victim_gobj=victim_object;victim->victim_gobj=object;
    data.x8=&data8;data8.x11=0;fighter->ft_data=&data;victim->ft_data=&data;
    fighter->parts=fighter_parts;victim->parts=victim_parts;
    fighter->parts[0].joint=&root_joint;victim->parts[0].joint=&root_joint;
    fighter->motion_id=ftCo_MS_Catch;fighter->cur_anim_frame=3;fighter->gr_vel=6;
    fighter->x74_self_accel=(Vec3){1,2,3};fighter->x221B_b7=true;
    fighter->mv.co.catch.x0=11;fighter->mv.co.common.x0=3;
    fighter->throw_flags=0x123;fighter->co_attrs.walk_max_vel=2;
    fighter->dat_attrs=&yattrs;fighter->facing_dir=1;
    yattrs.x124=2;yattrs.x128=8;yattrs.x12C[1]=7;
    memset(&article,0,sizeof(article));
    memset(&article_object,0,sizeof(article_object));
    article_object.user_data=&article;
    article.xDD4_itemVar.linkhookshot.xC=(void*)&endpoint_joint;
    article.xDD4_itemVar.samusgrapple.xC=(void*)&endpoint_joint;
    definition_kind=kind;victim_definition_kind=victim->kind;
    active_definition=present?&definition:NULL;active_victim_definition=NULL;
    item_priority=legacy_link_frame=legacy_samus_frame=directional_throw=false;frames=true;
    callback_result=false;callback_frame=9.5f;hook_id=hazard=boundary_stage=0;hook_calls=0;
    anchor_position=(Vec3){0,0,0};victim_position=(Vec3){-1,0,0};
}
static void install(int id)
{
    hook_id=id;active_definition=&definition;
    switch(id) {
    case 1:definition.on_catch_allowed=hook;break;
    case 2:definition.on_catch_start=void_hook;break;
    case 3:definition.on_catch_frame=hook;break;
    case 4:definition.on_catch_exit=void_hook;break;
    case 5:definition.on_catch_wait_release=void_hook;break;
    case 6:definition.on_catch_pull_anchor=hook;break;
    case 7:definition.on_catch_pull_ready=hook;break;
    case 8:definition.on_catch_pull_follow=hook;break;
    case 9:definition.on_catch_pull_frame=frame_hook;break;
    case 10:definition.on_capture_wait_setup=capture_hook;break;
    case 11:definition.on_capture_wait_enter=capture_hook;break;
    default:abort();
    }
}
static bool input(int path,bool old)
{
    diagnostic_id=path;diagnostic_old=old;
    if(path==0)return old?old_ftCo_Catch_CheckInput(object):new_ftCo_Catch_CheckInput(object);
    if(path==1)return old?old_ftCo_800D8A38(object):new_ftCo_800D8A38(object);
    if(path==2)return old?old_ftCo_800D8AE0(object):new_ftCo_800D8AE0(object);
    if(path==3)return old?old_ftCo_800D8B9C(object):new_ftCo_800D8B9C(object);
    abort();
}
static void phase(int id,bool old)
{
    diagnostic_id=id;diagnostic_old=old;
    switch(id) {
    case 2:if(old)old_ftCo_800D8C54(object,ftCo_MS_Catch);else new_ftCo_800D8C54(object,ftCo_MS_Catch);break;
    case 3:if(old)old_ftCo_Catch_Anim(object);else new_ftCo_Catch_Anim(object);break;
    case 4:if(old)old_fn_800D8E30(object);else new_fn_800D8E30(object);break;
    case 5:if(old)old_ftCo_CatchWait_IASA(object);else new_ftCo_CatchWait_IASA(object);break;
    case 6:case 8:case 9:if(old)old_fn_800D9CE8(object);else new_fn_800D9CE8(object);break;
    case 7:if(old)old_ftCo_CatchPull_Anim(object);else new_ftCo_CatchPull_Anim(object);break;
    case 10:if(old)old_ftCo_CaptureWait_Setup(victim_object);else new_ftCo_CaptureWait_Setup(victim_object);break;
    case 11:if(old)old_fn_800DB6C8(victim_object);else new_fn_800DB6C8(victim_object);break;
    case 12:if(old)old_ftCo_CatchDash_Anim(object);else new_ftCo_CatchDash_Anim(object);break;
    case 13:if(old)old_fn_800DA2B0(object);else new_fn_800DA2B0(object);break;
    default:abort();
    }
}
static int callback_kind(HSD_GObjEvent cb)
{
    if(!cb)return 0;
    if(cb==new_fn_800DA190||cb==old_fn_800DA190)return 1;
    if(cb==new_fn_800DA054||cb==old_fn_800DA054)return 2;
    if(cb==new_fn_800DA4A0||cb==old_fn_800DA4A0)return 3;
    if(cb==new_fn_800DA490||cb==old_fn_800DA490)return 4;
    abort();
}
struct State { FtMotionId motion; float frame,velocity,counter;
    Vec3 accel;u32 flags;FtMotionId victim_motion;float victim_frame;
    HSD_JObj* anchor;int accessory,damage;bool follow; };
static void state(struct State* out)
{
    memset(out,0,sizeof(*out));
    out->motion=fighter->motion_id;out->frame=fighter->cur_anim_frame;
    out->velocity=fighter->gr_vel;out->counter=fighter->mv.co.catch.x0;
    out->accel=fighter->x74_self_accel;out->flags=fighter->throw_flags;
    out->victim_motion=victim->motion_id;out->victim_frame=victim->cur_anim_frame;
    out->anchor=fighter->mv.co.capturedamage.x18;
    out->accessory=callback_kind(fighter->accessory1_cb);
    out->damage=callback_kind(fighter->take_dmg_cb);out->follow=fighter->x221B_b7;
}
static void configure(unsigned mask)
{
    fighter->motion_id=(mask&256)?ftCo_MS_CatchDash:ftCo_MS_Catch;
    victim->motion_id=(mask&128)?ftCo_MS_CapturePulledHi:ftCo_MS_CapturePulledLw;
    fighter->mv.co.guard.x24=(mask&8)?0:2;
    item_priority=mask&1;fighter->input.held_buttons[0]=(mask&2)?HSD_PAD_LR:0;
    fighter->input.pressed_buttons=(mask&4)?HSD_PAD_A:0;
    if(mask&8)fighter->mv.co.common.x0=0;
    if(mask&16){if(fighter->kind==Ft_Kind_Samus)fighter->u.ss.x223C=&article_object;else fighter->u.lk.xC=&article_object;}
    frames=(mask&32)!=0;legacy_link_frame=mask&64;legacy_samus_frame=mask&128;directional_throw=mask&256;
    article.xDD4_itemVar.linkhookshot.x14=(mask&512)!=0;
    article.xDD4_itemVar.samusgrapple.x14=(mask&512)!=0;
}
static void fallback_matrix(void)
{
    FighterKind kinds[]={Ft_Kind_CustomFirst,Ft_Kind_Link,Ft_Kind_CLink,Ft_Kind_Samus,Ft_Kind_Yoshi};
    for(unsigned k=0;k<5;k++)for(unsigned mask=0;mask<1024;mask++)for(int present=0;present<2;present++) {
        diagnostic_kind=k;diagnostic_mask=mask;
        for(int path=0;path<4;path++) {
            reset(kinds[k],present);configure(mask);bool a=input(path,true);struct Trace t=trace;struct State s;state(&s);
            reset(kinds[k],present);configure(mask);bool b=input(path,false);struct State n;state(&n);
            CHECK(a==b&&!memcmp(&trace,&t,sizeof(t))&&!memcmp(&n,&s,sizeof(n))&&!hook_calls);++cases;
        }
        for(int id=2;id<=13;id++) {
            // Link/Samus anchor entry requires a live source item. Stock NULL is UB.
            if((id==6||id==8||id==9)&&(k==1||k==2||k==3)&&!(mask&16))continue;
            reset(kinds[k],present);configure(mask);phase(id,true);struct Trace t=trace;struct State s;state(&s);
            reset(kinds[k],present);configure(mask);phase(id,false);struct State n;state(&n);
            CHECK(!memcmp(&trace,&t,sizeof(t))&&!memcmp(&n,&s,sizeof(n))&&!hook_calls);++cases;
        }
    }
}
static void authored_matrix(void)
{
    for(int id=1;id<=11;id++)for(int result=0;result<2;result++) {
        reset(Ft_Kind_CustomFirst,true);install(id);callback_result=result;
        fighter->input.held_buttons[0]=HSD_PAD_LR;fighter->input.pressed_buttons=HSD_PAD_A;
        if(id==1)CHECK(input(0,false)==(bool)result);else phase(id,false);
        CHECK(hook_calls==1);++cases;++baseline_rejections;
        if(id==3){CHECK(trace.events[0]==HookBase+3);for(unsigned j=0;j<trace.count;j++)CHECK(trace.events[j]!=LinkFrame&&trace.events[j]!=SamusFrame);}
        if(id==6)CHECK(fighter->mv.co.capturedamage.x18==(result?&endpoint_joint:&root_joint));
        if(id==8)CHECK(fighter->accessory1_cb==(result?new_fn_800DA054:NULL));
        if(id==9)CHECK(trace.frame==callback_frame);
        if(id==5)CHECK(trace.events[0]==Pummel&&trace.events[1]==HookBase+5);
        if(id==4)CHECK(trace.events[0]==HookBase+4&&trace.events[1]==Fall);
    }
    // Item input priority precedes even an authored veto.
    for(int path=0;path<3;path++){reset(Ft_Kind_CustomFirst,true);install(1);item_priority=true;CHECK(input(path,false)&&!hook_calls&&trace.count==1);++cases;}
    // Actual common no-follow policy remains for original tether kinds.
    FighterKind kinds[]={Ft_Kind_Link,Ft_Kind_CLink,Ft_Kind_Samus,Ft_Kind_CustomFirst};
    for(unsigned k=0;k<4;k++) {
        reset(kinds[k],false);fighter->mv.co.capturedamage.x18=&endpoint_joint;
        new_fn_800DA190(object);
        CHECK(trace.count==(k==3?2u:0u));++cases;
    }
    // Pummel success short-circuits directional_throw; otherwise directional_throw result selects release.
    for(int pummel=0;pummel<2;pummel++)for(int throw_result=0;throw_result<2;throw_result++) {
        reset(Ft_Kind_CustomFirst,true);install(5);directional_throw=throw_result;
        fighter->input.pressed_buttons=pummel?HSD_PAD_A:0;
        new_ftCo_CatchWait_IASA(object);CHECK(hook_calls==(unsigned)(pummel||throw_result));
        if(pummel){CHECK(trace.count==2&&trace.events[0]==Pummel&&trace.events[1]==HookBase+5);}
        else CHECK(trace.events[0]==DirectionalThrow);++cases;
    }
}
static float ordered_frame(Fighter_GObj* g)
{
    alive(g);CHECK(fighter->motion_id==ftCo_MS_Catch);record(HookBase+9);
    return 6.25f;
}
static bool ordered_anchor(Fighter_GObj* g)
{
    alive(g);CHECK(trace.count==1&&trace.events[0]==HookBase+9);
    CHECK(fighter->motion_id==ftCo_MS_Catch);record(HookBase+6);
    fighter->mv.co.capturedamage.x18=&endpoint_joint;return true;
}
static bool ordered_follow(Fighter_GObj* g)
{
    alive(g);CHECK(trace.count==3&&trace.events[1]==HookBase+6&&trace.events[2]==Motion);
    CHECK(fighter->motion_id==ftCo_MS_CatchPull&&fighter->cur_anim_frame==6.25f);
    record(HookBase+8);return callback_result;
}
static void continuation_matrix(void)
{
    for(int follow=0;follow<2;follow++) {
        reset(Ft_Kind_CustomFirst,true);callback_result=follow;
        definition.on_catch_pull_frame=ordered_frame;
        definition.on_catch_pull_anchor=ordered_anchor;
        definition.on_catch_pull_follow=ordered_follow;
        new_fn_800D9CE8(object);
        CHECK(trace.count==4&&fighter->accessory1_cb==(follow?new_fn_800DA054:NULL));
        CHECK(!fighter->x221B_b7&&fighter->mv.co.capturedamage.x18==&endpoint_joint);++cases;
    }
    // Authored anchor/follow do not require a victim for isolated entry.
    for(int id=6;id<=9;id++) {
        reset(Ft_Kind_CustomFirst,true);install(id);fighter->victim_gobj=NULL;
        callback_result=false;phase(id,false);CHECK(hook_calls==1);++cases;
    }
    // An unlinked retained victim blocks the callback before it can run.
    for(int id=6;id<=9;id++) {
        reset(Ft_Kind_CustomFirst,true);install(id);object->next=NULL;
        phase(id,false);CHECK(!hook_calls&&!trace.count);++cases;
    }
    // Capture registration's pickup callback retains its actual cleanup order.
    for(int old=0;old<2;old++) {
        reset(Ft_Kind_CustomFirst,false);fighter->target_item_gobj=&article_object;
        if(old)old_fn_800D8BFC(object);else new_fn_800D8BFC(object);
        CHECK(trace.count==2&&trace.events[0]==PickupTarget&&trace.events[1]==PickupFinish);
        CHECK(!fighter->target_item_gobj&&!fighter->x1A64);++cases;
    }
    // Both animation callbacks use replacement policy for tether donor kinds.
    FighterKind kinds[]={Ft_Kind_Link,Ft_Kind_CLink,Ft_Kind_Samus};
    for(unsigned k=0;k<3;k++)for(int dash=0;dash<2;dash++)for(int handled=0;handled<2;handled++) {
        reset(kinds[k],true);install(3);callback_result=handled;frames=false;
        if(dash)new_ftCo_CatchDash_Anim(object);else new_ftCo_Catch_Anim(object);
        CHECK(hook_calls==1&&trace.events[0]==HookBase+3);
        CHECK(trace.count==(handled?1u:2u));if(!handled)CHECK(trace.events[1]==Wait);++cases;
    }
    // Replacement veto allows held-item throw priority but suppresses donor gates.
    for(unsigned k=0;k<3;k++)for(int path=0;path<3;path++) {
        reset(kinds[k],true);install(1);callback_result=false;
        fighter->input.held_buttons[0]=HSD_PAD_LR;fighter->input.pressed_buttons=HSD_PAD_A;
        CHECK(!input(path,false)&&hook_calls==1&&trace.count==2);++cases;
    }
    // The actual follow body is still executed for non-tether kinds.
    for(int hidden=0;hidden<2;hidden++)for(int outside=0;outside<2;outside++) {
        reset(Ft_Kind_CustomFirst,false);fighter->mv.co.capturedamage.x18=&endpoint_joint;
        victim->x2226_b2=hidden;victim_position.x=outside?7:-1;
        new_fn_800DA190(object);
        CHECK(trace.count==(hidden?0u:(outside?3u:2u)));
        if(!hidden&&!outside)CHECK(fighter->gr_vel==-1);++cases;
    }
}
static void invalidation_matrix(void)
{
    for(int id=1;id<=11;id++)for(int result=0;result<2;result++)for(int h=1;h<=7;h++) {
        // Only Pull/capture paths retain a victim across the authored callback.
        if(h>=5&&id<6)continue;
        reset(Ft_Kind_CustomFirst,true);install(id);callback_result=result;
        fighter->input.held_buttons[0]=HSD_PAD_LR;fighter->input.pressed_buttons=HSD_PAD_A;
        hazard=h;if(id==1)CHECK(input(0,false));else phase(id,false);
        if(h==1)protect(fighter,object,PAGE_READWRITE);
        if(h==5)protect(victim,victim_object,PAGE_READWRITE);
        CHECK(hook_calls==1&&trace.events[trace.count-1]==HookBase+id);
        if(h==2)CHECK(!memcmp(fighter,reused_fighter,sizeof(*fighter))&&!memcmp(object,reused_object,sizeof(*object)));
        if(h==6)CHECK(!memcmp(victim,reused_victim,sizeof(*victim))&&!memcmp(victim_object,reused_victim_object,sizeof(*victim_object)));
        ++invalidation_cases;
    }
}
static void nested_capture_matrix(void)
{
    // Setup is nested inside the captured fighter's motion-entry function.
    // The outer entry must revalidate the same holder and caught generations.
    for(int result=0;result<2;result++)for(int h=1;h<=7;h++) {
        reset(Ft_Kind_CustomFirst,true);install(10);callback_result=result;hazard=h;
        new_fn_800DB6C8(victim_object);
        if(h==1)protect(fighter,object,PAGE_READWRITE);
        if(h==5)protect(victim,victim_object,PAGE_READWRITE);
        CHECK(hook_calls==1&&trace.events[trace.count-1]==HookBase+10);
        if(h==2)CHECK(!memcmp(fighter,reused_fighter,sizeof(*fighter))&&!memcmp(object,reused_object,sizeof(*object)));
        if(h==6)CHECK(!memcmp(victim,reused_victim,sizeof(*victim))&&!memcmp(victim_object,reused_victim_object,sizeof(*victim_object)));
        ++invalidation_cases;
    }
}
static void boundary_matrix(void)
{
    // Actual motion-entry can call x21EC and retire its source. The engine
    // operation itself is controlled; production continuation guards are real.
    for(int phase_id=0;phase_id<5;phase_id++)for(int h=1;h<=7;h++) {
        if((phase_id==0||phase_id==4)&&h>=5)continue;
        reset(Ft_Kind_CustomFirst,true);install(6);hook_id=phase_id==3?10:6;
        diagnostic_id=phase_id;diagnostic_old=0;
        boundary_stage=1;hazard=h;
        if(phase_id==0)new_ftCo_800D8C54(object,ftCo_MS_Catch);
        if(phase_id==1)new_fn_800D9CE8(object);
        if(phase_id==2)new_fn_800DA1D8(object);
        if(phase_id==3)new_fn_800DB6C8(victim_object);
        if(phase_id==4)new_fn_800DA2B0(object);
        if(h==1)protect(fighter,object,PAGE_READWRITE);
        if(h==5)protect(victim,victim_object,PAGE_READWRITE);
        // The nested captured entry owns the caught fighter. A change to
        // the still-live holder's kind/Definition is noticed by DB6C8 after
        // Setup; no final capture continuation may follow that boundary.
        CHECK(trace.events[trace.count-1]==((phase_id==3&&(h==3||h==4))?HurtFlags:Motion));
        if(h==2)CHECK(!memcmp(fighter,reused_fighter,sizeof(*fighter))&&!memcmp(object,reused_object,sizeof(*object)));
        if(h==6)CHECK(!memcmp(victim,reused_victim,sizeof(*victim))&&!memcmp(victim_object,reused_victim_object,sizeof(*victim_object)));
        ++invalidation_cases;
    }
    // Accepted pummel and directional throw can retire their caller.
    // Rejected paths are pure input reads and are not retirement boundaries.
    for(int operation=2;operation<=3;operation++)for(int h=1;h<=4;h++) {
        reset(Ft_Kind_CustomFirst,true);install(5);boundary_stage=operation;hazard=h;
        fighter->input.pressed_buttons=operation==2?HSD_PAD_A:0;directional_throw=true;
        new_ftCo_CatchWait_IASA(object);
        if(h==1)protect(fighter,object,PAGE_READWRITE);
        CHECK(!hook_calls&&trace.count==1&&trace.events[0]==(operation==2?Pummel:DirectionalThrow));
        if(h==2)CHECK(!memcmp(fighter,reused_fighter,sizeof(*fighter))&&!memcmp(object,reused_object,sizeof(*object)));
        ++invalidation_cases;
    }
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(fault);
    SYSTEM_INFO info;GetSystemInfo(&info);
    fighter_bytes=(sizeof(Fighter)+info.dwPageSize-1)&~((size_t)info.dwPageSize-1);
    object_bytes=(sizeof(HSD_GObj)+info.dwPageSize-1)&~((size_t)info.dwPageSize-1);
    fighter=VirtualAlloc(NULL,fighter_bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    victim=VirtualAlloc(NULL,fighter_bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    object=VirtualAlloc(NULL,object_bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    victim_object=VirtualAlloc(NULL,object_bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    CHECK(fighter&&victim&&object&&victim_object);common.x34C=5;common.x350=5;
    if(argc==2&&strcmp(argv[1],"--legacy-frame-retirement")==0) {
        reset(Ft_Kind_CustomFirst,true);install(9);hazard=1;
        old_fn_800D9CE8(object);return 99;
    }
    stage="fallback";fallback_matrix();stage="authored";authored_matrix();
    stage="continuation";continuation_matrix();stage="invalidation";invalidation_matrix();
    stage="nested-capture";nested_capture_matrix();stage="boundaries";boundary_matrix();
    printf("{\"checks\":%u,\"cases\":%u,\"invalidation_cases\":%u,\"authored_cases\":%u}\n",checks,cases,invalidation_cases,baseline_rejections);
    protect(fighter,object,PAGE_READWRITE);protect(victim,victim_object,PAGE_READWRITE);
    VirtualFree(fighter,0,MEM_RELEASE);VirtualFree(victim,0,MEM_RELEASE);
    VirtualFree(object,0,MEM_RELEASE);VirtualFree(victim_object,0,MEM_RELEASE);return 0;
}
