/* Actual old/current bodies. Engine allocation, animation, pickup and private
 * family dispatch boundaries are controls, never substitute decision bodies. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattack11.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattack100.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattackair.h>
#include <melee/ft/kinds/ftGameWatch/ftgamewatchattacklw3.h>
#include <melee/ft/kinds/ftLink/ftlinkattackair.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/it_26B1.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>

static unsigned checks,pairs,safety;
#define CHECK(x) do { ++checks; if(!(x)){fprintf(stderr,"pair%u safety%u line%d %s\n",pairs,safety,__LINE__,#x);exit(1);} } while(0)
enum { Pickup=1, Motion, Start, Recount, HitReset, GroundEvent, Length, Rate, Finish,
       FamilyJab, FamilyRapid, FamilyAir, FamilyTilt, Throw, Swing, Gun, Policy, AerialPolicy };
typedef struct AttackEvent { int op,a,b;float x,y,z; } AttackEvent;
static AttackEvent events[32],saved_events[32];static unsigned count,saved_count;
static Fighter fighter;static HSD_GObj object,item_object,*heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static NativeFighterDefinition policy, replacement;static const NativeFighterDefinition* current;
static int fail_at,fail_mode,pickup,use_kind,chosen;
static bool cstick;static float stick_angle;
static void record(int op,int a,int b,float x,float y,float z) {
    CHECK(count<32);events[count++]=(AttackEvent){op,a,b,x,y,z};
    if(op!=fail_at)return;
    switch(fail_mode){case 1:heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;object.user_data=(void*)1;break;
      case 2:fighter.x8_spawnNum++;break;case 3:fighter.kind=Ft_Kind_Donkey;break;case 4:current=&replacement;break;}
}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind) { (void)kind;return current; }
bool nativeModelEffect_Snapshot(HSD_GObj*g,uint32_t*id){(void)g;*id=0;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj*g,uint32_t id){(void)g;(void)id;return NULL;}
bool ftpickupitem_80094790(Fighter_GObj*g){CHECK(g==&object);record(Pickup,pickup,0,0,0,0);return pickup;}
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj*donor){CHECK(g==&object&&donor==NULL);record(Motion,motion,(int)flags,frame,rate,blend);fighter.motion_id=motion;}
void ftAnim_8006EBA4(Fighter_GObj*g){CHECK(g==&object);record(Start,0,0,0,0,0);}
void ft_800892A0(Fighter_GObj*g){CHECK(g==&object);record(Recount,0,0,0,0,0);}
void ft_80089824(Fighter_GObj*g){CHECK(g==&object);record(HitReset,0,0,0,0,0);}
void ftCommon_8007D7FC(Fighter*f){CHECK(f==&fighter);fighter.ground_or_air=GA_Ground;record(GroundEvent,0,0,0,0,0);}
float ftAnim_8006F484(Fighter_GObj*g){CHECK(g==&object);record(Length,0,0,0,0,0);return 23.75f;}
void ftAnim_SetAnimRate(Fighter_GObj*g,float value){CHECK(g==&object);fighter.frame_speed_mul=value;record(Rate,0,0,value,0,0);}
void ftGw_Attack11_Enter(Fighter_GObj*g){CHECK(g==&object);record(FamilyJab,0,0,0,0,0);}
void ftGw_Attack100Start_Enter(Fighter_GObj*g){CHECK(g==&object);record(FamilyRapid,0,0,0,0,0);}
void ftGw_AttackAirN_DecideAction(Fighter_GObj*g){CHECK(g==&object);record(FamilyAir,1,0,0,0,0);}
void ftLk_AttackAir_Enter(Fighter_GObj*g){CHECK(g==&object);record(FamilyAir,2,0,0,0,0);}
void ftGw_AttackLw3_Enter(Fighter_GObj*g){CHECK(g==&object);record(FamilyTilt,0,0,0,0,0);}
void ftCo_800957F4(Fighter_GObj*g,FtMotionId motion){CHECK(g==&object);record(Throw,motion,0,0,0,0);}
void ftCo_Attack_800CCF58(Fighter_GObj*g,bool flag){CHECK(g==&object&&!flag);record(Swing,0,0,0,0,0);}
void ftCo_Attack_800CDD14(Fighter_GObj*g){CHECK(g==&object);record(Gun,0,0,0,0,0);}
void ftCo_800CDDA0(Fighter_GObj*g){CHECK(g==&object);record(Gun,1,0,0,0,0);}
int itGetUseKind(Item_GObj*g){CHECK(g==&item_object);return use_kind;}
bool ftCo_800DF478(Fighter*f){CHECK(f==&fighter);return cstick;}
float ftCo_GetCStickAngle(Fighter*f){CHECK(f==&fighter);return stick_angle;}
float ftCo_GetLStickAngle(Fighter*f){CHECK(f==&fighter);return stick_angle;}
static void finish(Fighter_GObj*g){CHECK(g==&object);record(Finish,0,0,0,0,0);}
static void entry_policy(Fighter_GObj*g){CHECK(g==&object);record(Policy,0,0,0,0,0);}
static int32_t aerial_policy(Fighter_GObj*g,int32_t selected){CHECK(g==&object);record(AerialPolicy,selected,chosen,0,0,0);return chosen;}
static int32_t jab_flags(Fighter_GObj*g){CHECK(g==&object);record(Policy,1,0,0,0,0);return Ft_MF_SkipAttackCount;}
static bool restart(Fighter_GObj*g,int32_t motion){CHECK(g==&object);record(Policy,2,motion,0,0,0);return false;}
static ftCommonData common;ftCommonData* p_ftCommonData=&common;
enum {cmd_unk0_bool=0};
#include "old_attack.inc"
#include "new_attack.inc"

typedef struct State {int motion,unk,allow,b1,b2,a0,a4,throw_flags,press_count,air,callback;float window,rate;} State;
static State saved;
static State state(void){return (State){fighter.motion_id,fighter.unk_msid,fighter.allow_interrupt,fighter.x2218_b1,fighter.x2218_b2,fighter.mv.co.attack100.x0,fighter.mv.co.attack100.x4,fighter.throw_flags,fighter.x1A54,fighter.ground_or_air,fighter.x21EC!=NULL,fighter.hitlag_mul,fighter.frame_speed_mul};}
static uint32_t bits(float f){uint32_t v;memcpy(&v,&f,4);return v;}
static void save(void){saved=state();saved_count=count;memcpy(saved_events,events,count*sizeof(*events));}
static void compare(void){State now=state();CHECK(count==saved_count);for(unsigned i=0;i<count;++i){CHECK(events[i].op==saved_events[i].op&&events[i].a==saved_events[i].a&&events[i].b==saved_events[i].b);CHECK(bits(events[i].x)==bits(saved_events[i].x)&&bits(events[i].y)==bits(saved_events[i].y)&&bits(events[i].z)==bits(saved_events[i].z));}
#define MATCH(f) CHECK(now.f==saved.f)
    MATCH(motion);MATCH(unk);MATCH(allow);MATCH(b1);MATCH(b2);MATCH(a0);MATCH(a4);MATCH(throw_flags);MATCH(press_count);MATCH(air);MATCH(callback);CHECK(bits(now.window)==bits(saved.window));CHECK(bits(now.rate)==bits(saved.rate));
#undef MATCH
    ++pairs;
}
static void init(FighterKind kind){memset(&fighter,0,sizeof fighter);memset(&object,0,sizeof object);memset(heads,0,sizeof heads);memset(&policy,0,sizeof policy);current=NULL;
    object.user_data=&fighter;object.classifier=HSD_GOBJ_CLASS_FIGHTER;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;fighter.gobj=&object;fighter.kind=kind;fighter.x8_spawnNum=(s32)UINT32_C(0xFEDCBA98);
    fighter.motion_id=14;fighter.unk_msid=99;fighter.allow_interrupt=true;fighter.x2218_b1=true;fighter.x2218_b2=true;fighter.mv.co.attack100.x0=7;fighter.mv.co.attack100.x4=9;fighter.x1A54=13;fighter.throw_flags=UINT32_C(0xAB);fighter.hitlag_mul=11;fighter.co_attrs.jab_2_input_window=6;fighter.co_attrs.jab_3_input_window=9;fighter.frame_speed_mul=1.25f;fighter.ground_or_air=GA_Air;
    common.xDC=.2f;common.xE0=.2f;common.x20_radians=.7f;count=0;fail_at=0;fail_mode=0;pickup=0;cstick=false;stick_angle=0;chosen=65;
}
int main(void){
    const FighterKind kinds[]={Ft_Kind_Mario,Ft_Kind_Pikachu,Ft_Kind_Pichu,Ft_Kind_GameWatch,Ft_Kind_Link,Ft_Kind_CLink,Ft_Kind_Mars};
    for(unsigned k=0;k<7;++k)for(pickup=0;pickup<2;){int picked=pickup;
        init(kinds[k]);pickup=picked;old_jab_checkAttack11(&object);save();init(kinds[k]);pickup=picked;CHECK(new_jab_ftCo_Attack11_EnterMotion(&object,ftCo_MS_Attack11)==!picked);compare();pickup=picked+1;
    }
    for(unsigned k=0;k<7;++k)for(unsigned dispatch=0;dispatch<7;++dispatch){
        init(kinds[k]);switch(dispatch){case 0:old_jab_decideAttack11(&object);break;case 1:old_jab_doAttack12Rapid(&object);break;case 2:old_rapid_fn_800D6AC4(&object);break;case 3:old_tilt_decideFighter(&object);break;case 4:old_air_decideFighter(&object);break;case 5:old_jab_doAttack12(&object);break;case 6:old_jab_doAttack13(&object);break;}save();
        init(kinds[k]);switch(dispatch){case 0:new_jab_decideAttack11(&object);break;case 1:new_jab_doAttack12Rapid(&object);break;case 2:new_rapid_fn_800D6AC4(&object);break;case 3:new_tilt_decideFighter(&object);break;case 4:new_air_decideFighter(&object);break;case 5:new_jab_doAttack12(&object);break;case 6:new_jab_doAttack13(&object);break;}compare();
    }
    for(unsigned picked=0;picked<2;++picked){init(Ft_Kind_Mario);pickup=picked;old_rapid_ftCo_800D6B00(&object,ftCo_MS_Attack100Start);save();init(Ft_Kind_Mario);pickup=picked;new_rapid_ftCo_800D6B00(&object,ftCo_MS_Attack100Start);compare();}
    const float frames[]={-1,0,.5f,1.25f,2};
    for(unsigned f=0;f<5;++f)for(unsigned ready=0;ready<2;++ready)for(unsigned again=0;again<2;++again)for(unsigned flag=0;flag<2;++flag)for(unsigned picked=0;picked<2;++picked){
        init(Ft_Kind_Mario);fighter.cur_anim_frame=frames[f];fighter.mv.co.attack100.x0=ready;fighter.mv.co.attack100.x4=again;fighter.throw_flags_b3=flag;pickup=picked;old_rapid_ftCo_800D6C60(&object,finish);save();
        init(Ft_Kind_Mario);fighter.cur_anim_frame=frames[f];fighter.mv.co.attack100.x0=ready;fighter.mv.co.attack100.x4=again;fighter.throw_flags_b3=flag;pickup=picked;if(new_rapid_ftCo_RapidJab_ShouldFinish(&object))finish(&object);compare();
    }
    const float lag[]={1,4.5f,12};for(unsigned l=0;l<3;++l){init(Ft_Kind_Mario);old_landing_ftCo_LandingAir_EnterWithMsidLag(&object,ftCo_MS_LandingAirN,lag[l]);save();init(Ft_Kind_Mario);new_landing_ftCo_LandingAir_EnterWithMsidLag(&object,ftCo_MS_LandingAirN,lag[l]);compare();}
    for(unsigned k=0;k<7;++k)for(unsigned n=0;n<3;++n)for(unsigned enabled=0;enabled<2;++enabled)for(unsigned input=0;input<4;++input){
        init(kinds[k]);fighter.co_attrs.rapid_jab_window=3;fighter.x1A54=n==0?0:n==1?2:3;fighter.x2218_b2=enabled;fighter.input.pressed_buttons=input&1?HSD_PAD_A:0;fighter.input.released_buttons=input&2?HSD_PAD_A:0;bool original=old_rapid_ftCo_Attack_800D6A50(&object);save();
        init(kinds[k]);fighter.co_attrs.rapid_jab_window=3;fighter.x1A54=n==0?0:n==1?2:3;fighter.x2218_b2=enabled;fighter.input.pressed_buttons=input&1?HSD_PAD_A:0;fighter.input.released_buttons=input&2?HSD_PAD_A:0;CHECK(new_rapid_ftCo_Attack_800D6A50(&object)==original);compare();
    }
    for(unsigned kind=0;kind<4;++kind)for(unsigned held=0;held<2;++held){init(Ft_Kind_Mario);fighter.input.pressed_buttons=HSD_PAD_A;fighter.input.held_buttons[0]=held?HSD_PAD_LR:0;fighter.item_gobj=&item_object;use_kind=kind;old_jab_ftCo_Attack1_CheckInput(&object);save();init(Ft_Kind_Mario);fighter.input.pressed_buttons=HSD_PAD_A;fighter.input.held_buttons[0]=held?HSD_PAD_LR:0;fighter.item_gobj=&item_object;use_kind=kind;new_jab_ftCo_Attack1_CheckInput(&object);compare();}
    /*58-only authored contracts are separate from unclaimed raw fallback.*/
    for(unsigned route=0;route<4;++route){init(Ft_Kind_GameWatch);current=&policy;policy.on_jab_enter=entry_policy;policy.on_rapid_jab_enter=entry_policy;policy.on_down_tilt_enter=entry_policy;
        switch(route){case 0:new_jab_decideAttack11(&object);break;case 1:new_jab_doAttack12Rapid(&object);break;case 2:new_rapid_fn_800D6AC4(&object);break;case 3:new_tilt_decideFighter(&object);break;}CHECK(count==1&&events[0].op==Policy);++safety;}
    const int remaps[]={-1,0,65,340,341,345,352,353,INT32_MAX,INT32_MIN};
    for(unsigned selected=0;selected<5;++selected)for(unsigned edge=0;edge<2;++edge)for(unsigned r=0;r<10;++r){
        init(Ft_Kind_Mario);current=&policy;policy.motion_state_first=341;policy.motion_state_count=12;policy.on_aerial_attack_motion=aerial_policy;chosen=remaps[r];
        const float x[]={0,1,-1,0,0},y[]={0,0,0,1,-1};fighter.facing_dir=1;fighter.input.lstick[0]=(Vec2){x[selected],y[selected]};fighter.input.cstick[0]=fighter.input.lstick[0];cstick=edge;stick_angle=selected==3?1:selected==4?-1:0;
        new_air_decideFighter(&object);int ordinary=ftCo_MS_AttackAirN+selected;CHECK(count==3&&events[0].op==AerialPolicy&&events[0].a==ordinary&&events[1].op==Motion&&events[2].op==Start);CHECK(fighter.motion_id==(chosen==ordinary||(chosen>=341&&chosen<353)?chosen:ordinary));++safety;
    }
    /*Every nonterminal cached continuation stops for retirement/recycle,
     * changed kind or changed Definition, using the real full-width registry.*/
    const int operations[]={Pickup,Motion,Start,Recount,HitReset,GroundEvent,Policy,AerialPolicy};
    for(unsigned mode=1;mode<=4;++mode)for(unsigned op=0;op<8;++op){
        init(Ft_Kind_Mario);current=&policy;policy.on_jab_start=jab_flags;policy.on_aerial_attack_motion=aerial_policy;policy.motion_state_first=341;policy.motion_state_count=12;fail_mode=mode;fail_at=operations[op];
        switch(fail_at){case Pickup:case Motion:case Start:case Policy:CHECK(!new_jab_ftCo_Attack11_EnterMotion(&object,341));break;
          case Recount:case HitReset:fighter.cur_anim_frame=0;fighter.throw_flags_b3=true;if(new_rapid_ftCo_RapidJab_ShouldFinish(&object))finish(&object);break;
          case GroundEvent:new_landing_ftCo_LandingAir_EnterWithMsidLag(&object,350,8);break;
          case AerialPolicy:new_air_decideFighter(&object);break;}
        CHECK(count&&events[count-1].op==fail_at);++safety;
    }
    for(unsigned mode=1;mode<=4;++mode)for(unsigned route=0;route<3;++route)for(unsigned boundary=0;boundary<2;++boundary){
        if(route==1&&boundary==1)continue; /* Landing has no Start callback. */
        init(Ft_Kind_Mario);current=&policy;fail_mode=mode;fail_at=boundary?Start:Motion;
        if(route==0)new_rapid_ftCo_800D6B00(&object,342);else if(route==1)new_landing_ftCo_LandingAir_EnterWithMsidLag(&object,350,8);else new_air_ftCo_AttackAir_EnterFromMsid(&object,347);
        CHECK(count&&events[count-1].op==fail_at);++safety;
    }
    /* Return-true pickup and terminal finish callbacks may invalidate, but
     * these branches retain no cached continuation after the callback. */
    for(unsigned mode=1;mode<=4;++mode){init(Ft_Kind_Mario);current=&policy;fail_mode=mode;fail_at=Pickup;pickup=true;CHECK(!new_jab_ftCo_Attack11_EnterMotion(&object,341));CHECK(count==1);++safety;
        init(Ft_Kind_Mario);current=&policy;fail_mode=mode;fail_at=Finish;fighter.cur_anim_frame=2;fighter.mv.co.attack100.x0=true;fighter.mv.co.attack100.x4=false;fighter.throw_flags_b3=true;if(new_rapid_ftCo_RapidJab_ShouldFinish(&object))finish(&object);CHECK(count==1&&events[0].op==Finish);++safety;}
    printf("PASS %u old/current pairs, %u checks, %u authored/safety cases\n",pairs,checks,safety);return 0;
}
