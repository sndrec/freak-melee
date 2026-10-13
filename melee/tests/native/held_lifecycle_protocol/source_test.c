#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftwaitanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftPeach/forward.h>
#include <melee/it/types.h>
#include <melee/it/kinds/forward.h>
#include <melee/it/kinds/types.h>
#include <melee/it/native_held_article.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjobject.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned checks,pairs,authored_cases,safety_cases,timer_tail;
#define CHECK(x) do { ++checks;assert(x); }while(0)
HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static Fighter fighter;
static Item item;
static HSD_GObj object,held;
static ftCommonData common;
ftCommonData* p_ftCommonData=&common;
static NativeFighterDefinition definition,replacement;
static const NativeFighterDefinition* active;
static unsigned invalid_stage,invalid_mode,count,domain;
static int events[32];
static float transition_start,transition_rate,transition_blend,policy_blend;
static uint32_t transition_flags;
static bool hammer,camera,camera_data,claimed,notify_success,timer_handled;
static int32_t mapping;
static struct Fighter_WaitAnimData animation;
static void invalidate(unsigned stage) {
 if(stage!=invalid_stage)return;
 if(invalid_mode==1)heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
 if(invalid_mode==2)++fighter.x8_spawnNum;
 if(invalid_mode==3)fighter.kind=Ft_Kind_CustomFirst+1;
 if(invalid_mode==4)active=&replacement;
}
static void event(int value,unsigned stage) { CHECK(count<32);events[count++]=value;invalidate(stage); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k) { (void)k;return active; }
void ftCo_DownSpot_Enter(Fighter_GObj*g) { CHECK(g==&object);event(1,0); }
bool ftCo_800C5240(Fighter_GObj*g) { CHECK(g==&object);return hammer; }
void ftCo_800C4ED8(Fighter_GObj*g) { CHECK(g==&object);event(2,0); }
void ftCommon_8007D7FC(Fighter*fp) { CHECK(fp==&fighter);fp->ground_or_air=GA_Ground;event(3,1); }
ItemKind itGetKind(Item_GObj*g) { CHECK(g==&held);return item.kind; }
void it_802BDB94(Item_GObj*g) { CHECK(g==&held);event(4,2); }
bool nativeHeldArticle_NotifyNeutral(Fighter_GObj*g,bool* output) { CHECK(g==&object);if(output)*output=claimed;if(claimed)event(14,2);return notify_success; }
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId motion,MotionFlags flags,f32 start,f32 rate,f32 blend,Fighter_GObj*donor) {
 CHECK(g==&object&&!donor);event(5,3);fighter.motion_id=motion;transition_start=start;transition_rate=rate;transition_blend=blend;transition_flags=(uint32_t)flags;
}
bool ftCo_8008A698(Fighter*fp) { CHECK(fp==&fighter);return camera; }
struct Fighter_WaitAnimData* ftData_80085FD4(Fighter*fp,FtMotionId motion) { CHECK(fp==&fighter&&motion==ftCo_MS_DeadUpFall);animation.x8=camera_data;return &animation; }
void ftCo_8008A6D8(Fighter_GObj*g,s32 motion) { CHECK(g==&object&&motion==ftCo_MS_DeadUpFall);event(6,4); }
void ftCommon_8007EFC0(Fighter*fp,u32 value) { CHECK(fp==&fighter&&value==common.x5F0);event(7,0); }
void ftLk_AttackAir_800EB3BC(Fighter_GObj*g) { CHECK(g==&object);event(8,0); }
void ftCl_Init_8014919C(Fighter_GObj*g) { CHECK(g==&object);event(9,0); }
void ftPe_8011D598(HSD_GObj*g) { CHECK(g==&object);event(10,2); }
void ftCo_FallAerial_Enter(Fighter_GObj*g) { (void)g; }
void ftCo_800968C8(Fighter_GObj*g) { CHECK(g==&object);event(11,5); }
void ftCo_Fall_Enter(Fighter_GObj*g) { (void)g; }
void ftCo_80095744(Fighter_GObj*g,bool* flag,struct NativeItemLifetime* retained) { CHECK(g==&object&&!flag&&!retained);event(12,5); }
static void neutral(Fighter_GObj*g,float blend) { CHECK(g==&object);policy_blend=blend;event(13,2); }
static int32_t resume(Fighter_GObj*g,int32_t previous) { CHECK(g==&object);(void)previous;event(15,2);return mapping; }
static bool expired(Fighter_GObj*g) { CHECK(g==&object);CHECK(!fighter.x2221_b4&&fighter.x2104==0);event(16,2);return timer_handled; }
static void landing(Fighter_GObj*g) { CHECK(g==&object);event(17,6); }
static void wait_enter(Fighter_GObj*g) { CHECK(g==&object);event(18,0); }
#include "original_entries.inc"
#include "candidate_entries.inc"
typedef struct Result {int motion,timer;uint32_t motion_flags;unsigned air,active,blocked,interrupt,tail,count;int fighter_policy;void(*resume)(Fighter_GObj*);float start,rate,blend;int events[32];}Result;
static Result result(void) {Result r={0};r.motion=fighter.motion_id;r.motion_flags=transition_flags;r.timer=fighter.x2104;r.air=fighter.ground_or_air;r.active=fighter.x2221_b4;r.blocked=fighter.x2221_b5;r.interrupt=domain==2?fighter.mv.co.landing.allow_interrupt:0;r.tail=timer_tail;r.resume=domain==1?fighter.mv.co.mushroom.resume_air:NULL;r.count=count;r.start=transition_start;r.rate=transition_rate;r.blend=transition_blend;if(domain==2) { switch(fighter.kind) {case Ft_Kind_Mario:case Ft_Kind_DrMario:r.fighter_policy=fighter.u.mr.x2234_tornadoCharge|(fighter.u.mr.x2238_isCapeBoost<<1);break;case Ft_Kind_Peach:r.fighter_policy=fighter.u.pe.specialairn_used;break;case Ft_Kind_Mars:case Ft_Kind_Emblem:r.fighter_policy=fighter.u.ms.x222C;break;case Ft_Kind_GameWatch:r.fighter_policy=fighter.u.gw.x2234;break;case Ft_Kind_Popo:case Ft_Kind_Nana:r.fighter_policy=fighter.u.pp.x224C;break;case Ft_Kind_Kirby:r.fighter_policy=fighter.u.kb.xCC|(fighter.u.kb.xC4<<1)|(fighter.u.kb.x64<<2);break;case Ft_Kind_Mewtwo:r.fighter_policy=fighter.u.mt.x223C_isConfusionBoost;break;} }memcpy(r.events,events,sizeof(events));return r;}
static void equal(Result a,Result b) {
 CHECK(a.fighter_policy==b.fighter_policy);CHECK(a.motion_flags==b.motion_flags);CHECK(a.motion==b.motion);CHECK(a.timer==b.timer);CHECK(a.air==b.air);CHECK(a.active==b.active);CHECK(a.blocked==b.blocked);CHECK(a.interrupt==b.interrupt);CHECK(a.tail==b.tail);CHECK(a.resume==b.resume);CHECK(a.count==b.count);
 CHECK(!memcmp(&a.start,&b.start,4));CHECK(!memcmp(&a.rate,&b.rate,4));CHECK(!memcmp(&a.blend,&b.blend,4));for(unsigned i=0;i<a.count;++i)CHECK(a.events[i]==b.events[i]);++pairs;
}
static void reset(int kind,unsigned flags,int held_kind,int timer) {
 memset(&fighter,0,sizeof(fighter));memset(&item,0,sizeof(item));memset(&object,0,sizeof(object));memset(&held,0,sizeof(held));memset(events,0,sizeof(events));memset(heads,0,sizeof(heads));
 object.user_data=&fighter;object.classifier=HSD_GOBJ_CLASS_FIGHTER;fighter.kind=kind;fighter.behavior_kind=kind;fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);fighter.motion_id=77;fighter.ground_or_air=(flags&1)?GA_Air:GA_Ground;fighter.x2224_b2=(flags>>1)&1;fighter.x2221_b4=(flags>>2)&1;fighter.x2104=timer;switch(kind) {case Ft_Kind_Mario:case Ft_Kind_DrMario:fighter.u.mr.x2234_tornadoCharge=1;fighter.u.mr.x2238_isCapeBoost=1;break;case Ft_Kind_Peach:fighter.u.pe.specialairn_used=1;break;case Ft_Kind_Mars:case Ft_Kind_Emblem:fighter.u.ms.x222C=17;break;case Ft_Kind_GameWatch:fighter.u.gw.x2234=17;break;case Ft_Kind_Popo:case Ft_Kind_Nana:fighter.u.pp.x224C=17;break;case Ft_Kind_Kirby:fighter.u.kb.xCC=1;fighter.u.kb.xC4=1;fighter.u.kb.x64=1;break;case Ft_Kind_Mewtwo:fighter.u.mt.x223C_isConfusionBoost=1;break;}
 held.user_data=&item;held.classifier=HSD_GOBJ_CLASS_ITEM;item.x1C=UINT32_C(0x89ABCDEF);item.kind=held_kind;if(held_kind>=0)fighter.item_gobj=&held;
 heads[HSD_GOBJ_PLINK_FIGHTER]=&object;heads[HSD_GOBJ_PLINK_ITEM]=&held;HSD_GObjPLinkHead=heads;active=NULL;count=timer_tail=0;transition_start=transition_rate=transition_blend=policy_blend=0;transition_flags=0;invalid_stage=invalid_mode=0;hammer=camera=camera_data=claimed=notify_success=timer_handled=false;mapping=0;common.x5F0=123;
}
static unsigned index_event(int value) { for(unsigned i=0;i<count;++i)if(events[i]==value)return i;return 99; }
static void healthy(void) {
 domain=0; int kinds[]={Ft_Kind_Mario,Ft_Kind_Peach,Ft_Kind_CustomFirst,Ft_Kind_Link,Ft_Kind_CLink};int held_kinds[]={-1,It_Kind_Capsule,It_Kind_Peach_Parasol};float blends[]={-1,0,10};
 for(unsigned k=0;k<5;++k)for(unsigned h=0;h<3;++h)for(unsigned flags=0;flags<8;++flags)for(unsigned b=0;b<3;++b)for(unsigned gate=0;gate<4;++gate) {
  reset(kinds[k],flags,held_kinds[h],0);hammer=gate==1;camera=gate>=2;camera_data=gate==3;original54_ft_8008A348(&object,blends[b]);Result before=result();
  reset(kinds[k],flags,held_kinds[h],0);hammer=gate==1;camera=gate>=2;camera_data=gate==3;ft_8008A348(&object,blends[b]);equal(before,result());
 }
 domain=1;int previous[]={-1,ftCo_MS_Fall,ftCo_MS_FallAerial,ftCo_MS_FallSpecial,ftCo_MS_ItemParasolFallSpecial,370,INT32_MAX};
 for(unsigned k=0;k<5;++k)for(unsigned p=0;p<7;++p) {reset(kinds[k],0,-1,0);original54_fn_800D2890(&object,previous[p]);Result before=result();reset(kinds[k],0,-1,0);CHECK(ftCo_Mushroom_SelectAirResume(&object,previous[p]));equal(before,result());}
 domain=2;int landing_kinds[]={Ft_Kind_Mario,Ft_Kind_DrMario,Ft_Kind_Peach,Ft_Kind_Mars,Ft_Kind_Emblem,Ft_Kind_GameWatch,Ft_Kind_Popo,Ft_Kind_Nana,Ft_Kind_Kirby,Ft_Kind_Mewtwo,Ft_Kind_CustomFirst};
 for(unsigned k=0;k<11;++k)for(unsigned allow=0;allow<2;++allow)for(unsigned speed=0;speed<3;++speed) {
  reset(landing_kinds[k],1,-1,0);original54_ftCo_Landing_Enter(&object,ftCo_MS_Landing,allow,Ft_MF_None,(float)speed,(float)speed*0.5f);Result before=result();
  reset(landing_kinds[k],1,-1,0);ftCo_Landing_Enter(&object,ftCo_MS_Landing,allow,Ft_MF_None,(float)speed,(float)speed*0.5f);equal(before,result());
 }
 domain=3;for(unsigned k=0;k<5;++k)for(unsigned h=0;h<3;++h)for(unsigned flag=0;flag<2;++flag)for(int timer=0;timer<4;++timer) {
  reset(kinds[k],flag<<2,held_kinds[h],timer);original54_timer(&object);Result before=result();reset(kinds[k],flag<<2,held_kinds[h],timer);candidate_timer(&object);equal(before,result());
 }
}
static void contracts(void) {
 definition.motion_state_first=369;definition.motion_state_count=2;
 for(unsigned air=0;air<2;++air)for(unsigned allow=0;allow<2;++allow) {
  reset(Ft_Kind_CustomFirst,air,-1,0);active=&definition;definition.on_neutral_enter=neutral;definition.on_wait_enter=wait_enter;ft_8008A348(&object,10);CHECK(policy_blend==10);CHECK(index_event(13)<index_event(5)&&index_event(5)<index_event(18));++authored_cases;
  reset(Ft_Kind_CustomFirst,air,-1,0);active=&definition;definition.on_landing_action=landing;ftCo_Landing_Enter(&object,ftCo_MS_Landing,allow,Ft_MF_None,0,1);CHECK(fighter.mv.co.landing.allow_interrupt==allow&&index_event(17)>index_event(5));++authored_cases;
 }
 int32_t ids[]={INT32_MIN,-1,ftCo_MS_FallAerial,ftCo_MS_FallSpecial,ftCo_MS_Count,369,370,371,INT32_MAX};
 for(unsigned i=0;i<9;++i) {
  reset(Ft_Kind_CustomFirst,0,-1,0);active=&definition;definition.on_air_resume_motion=resume;mapping=ids[i];CHECK(ftCo_Mushroom_SelectAirResume(&object,ftCo_MS_FallAerial));bool valid=ids[i]>=0&&(ids[i]<ftCo_MS_Count||ids[i]==369||ids[i]==370);void(*expected)(Fighter_GObj*)=!valid||ids[i]==ftCo_MS_FallAerial?ftCo_FallAerial_Enter:ids[i]==ftCo_MS_FallSpecial||ids[i]==ftCo_MS_ItemParasolFallSpecial?ftCo_800968C8:ftCo_Fall_Enter;CHECK(fighter.mv.co.mushroom.resume_air==expected);++authored_cases;
 }
 for(unsigned handled=0;handled<2;++handled) {
  reset(Ft_Kind_CustomFirst,4,It_Kind_Peach_Parasol,1);active=&definition;definition.on_held_timer_expired=expired;timer_handled=handled;candidate_timer(&object);CHECK(index_event(16)!=99);CHECK(index_event(12)==(handled?99:1));CHECK(!fighter.x2221_b5&&index_event(11)==99);CHECK(timer_tail==1);++authored_cases;
 }
 memset(&definition,0,sizeof(definition));reset(Ft_Kind_Peach,0,It_Kind_Peach_Parasol,0);claimed=true;notify_success=false;ft_8008A348(&object,0);CHECK(index_event(14)!=99&&index_event(4)==99&&index_event(5)!=99);++authored_cases;
}
static void safety(void) {
 for(unsigned operation=0;operation<4;++operation)for(unsigned mode=1;mode<=4;++mode)for(unsigned stage=1;stage<=5;++stage) {
  bool relevant=operation==0?(stage<=4):operation==1?(stage==1||stage==3):operation==2?(stage==2):(stage==2||stage==5);
  if(!relevant)continue;
  reset(Ft_Kind_CustomFirst,operation==0||operation==1?1:4,It_Kind_Peach_Parasol,1);active=&definition;memset(&definition,0,sizeof(definition));definition.on_neutral_enter=neutral;definition.on_landing_action=landing;definition.on_air_resume_motion=resume;definition.on_held_timer_expired=expired;mapping=ftCo_MS_FallSpecial;timer_handled=stage!=5;camera=camera_data=true;invalid_stage=stage;invalid_mode=mode;
  if(operation==0) {ft_8008A348(&object,0);CHECK(index_event(7)==99);}
  else if(operation==1) {ftCo_Landing_Enter(&object,ftCo_MS_Landing,true,Ft_MF_None,0,1);CHECK(index_event(17)==99);}
  else if(operation==2) CHECK(!ftCo_Mushroom_SelectAirResume(&object,77));
  else {candidate_timer(&object);CHECK(timer_tail==0);}
  ++safety_cases;
 }
 for(unsigned mode=1;mode<=4;++mode) {
  reset(Ft_Kind_Peach,1,It_Kind_Peach_Parasol,0);active=&definition;memset(&definition,0,sizeof(definition));definition.on_landing_action=landing;invalid_stage=2;invalid_mode=mode;
  ftCo_Landing_Enter(&object,ftCo_MS_Landing,true,Ft_MF_None,0,1);CHECK(index_event(10)!=99&&index_event(17)==99);++safety_cases;
 }
 for(unsigned operation=0;operation<3;++operation) {
  reset(Ft_Kind_CustomFirst,4,-1,1);heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;object.user_data=NULL;
  if(operation==0)ft_8008A348(&object,0);if(operation==1)ftCo_Landing_Enter(&object,ftCo_MS_Landing,true,Ft_MF_None,0,1);if(operation==2)CHECK(!ftCo_Mushroom_SelectAirResume(&object,77));
  CHECK(count==0);++safety_cases;
 }
 for(unsigned operation=0;operation<3;++operation) {
  reset(Ft_Kind_CustomFirst,4,-1,1);active=&definition;definition.on_neutral_enter=neutral;definition.on_air_resume_motion=resume;definition.on_held_timer_expired=expired;
  for(unsigned i=0;i<64;++i)CHECK(nativeCallbackDispatch_Enter());CHECK(!nativeCallbackDispatch_Enter());
  if(operation==0)ft_8008A348(&object,0);if(operation==1)CHECK(!ftCo_Mushroom_SelectAirResume(&object,77));if(operation==2)candidate_timer(&object);
  CHECK(count==0);if(operation==2)CHECK(timer_tail==0);for(unsigned i=0;i<64;++i)nativeCallbackDispatch_Leave();CHECK(nativeCallbackDispatch_Enter());nativeCallbackDispatch_Leave();++safety_cases;
 }
}
int main(void) {healthy();contracts();safety();printf("Lifecycle source PASS %u pairs %u authored %u safety %u checks\n",pairs,authored_cases,safety_cases,checks);return 0;}
