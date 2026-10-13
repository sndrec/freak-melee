/* Actual original travel/held bodies versus the full composed generated module.
 * Allocation, motion/animation engine, collision internals and recipient lookup
 * are controlled boundaries; production generation-aware lifetime is linked. */
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftwaitanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialhi.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/kinds/ftPeach/types.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ef/native_model_effect.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/item.h>
#include <melee/it/kinds/itpeachparasol.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, pairs, safety;
static float maximum;
#define CHECK(x) do { checks++; if (!(x)) { fprintf(stderr,"pair%u line%d %s\n",pairs,__LINE__,#x); exit(1); } } while (0)
static Fighter fighter, transferred_holder;
static Item item, previous_item;
static HSD_GObj source, object, previous, holder_object, *heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static ftPe_DatAttrs data;
static HSD_JObj root;
static FighterBone bones[MAX_FT_PARTS];
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic* parasol;
static bool scripted, remaining, contact, recipient_export;
static int mutation;
static bool hammer;
static ftCommonData common_data;
ftCommonData* p_ftCommonData=&common_data;
static const uint32_t handle=UINT32_C(0xfedcba98), previous_handle=UINT32_C(0xfedcba99);
enum { Change, Start, MotionAir, MotionGround, Quick, AirCollision, GroundCollision,
       Landing, Freefall, Open, Rotate, Sound, ItemChange, Advance, Pause, Visible,
       Pickup, Remove, Interaction, GroundSetup, Throw, NeutralBookkeeping };
typedef struct ParasolEvent { int op,a,b; float x,y,z,w; } ParasolEvent;
static ParasolEvent events[64], expected[64];
static unsigned count, expected_count;
static void record(int op,int a,int b,float x,float y,float z,float w)
{ CHECK(count<64);events[count++]=(ParasolEvent){op,a,b,x,y,z,w}; }
static void close_float(float a,float b)
{ float delta=fabsf(a-b);if(delta>maximum)maximum=delta;CHECK(isfinite(a)&&isfinite(b)&&delta<=.00001f); }
static void save(void) { expected_count=count;memcpy(expected,events,sizeof events);count=0; }
static void compare(void)
{ CHECK(count==expected_count);for(unsigned i=0;i<count;i++){CHECK(events[i].op==expected[i].op&&events[i].a==expected[i].a&&events[i].b==expected[i].b);close_float(events[i].x,expected[i].x);close_float(events[i].y,expected[i].y);close_float(events[i].z,expected[i].z);close_float(events[i].w,expected[i].w);}pairs++; }
static void unexpected(void) { fprintf(stderr,"Uncontrolled Host operation\n");exit(1); }
void __assert(char* f,u32 l,char* s) { fprintf(stderr,"%s:%lu %s\n",f,l,s);exit(1); }
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* id) { (void)g;*id=0;return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t id) { (void)g;(void)id;return NULL; }
static void retire(void)
{ if(mutation==1)heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;else if(mutation==2)fighter.x8_spawnNum++; }
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId id,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* donor)
{ CHECK(g==&source&&!donor);record(Change,id,flags,frame,rate,blend,0);fighter.motion_id=id;fighter.cur_anim_frame=frame;retire(); }
void ftAnim_8006EBA4(Fighter_GObj* g) { CHECK(g==&source);record(Start,0,0,0,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* g) { CHECK(g==&source);return remaining; }
void ft_80085154(Fighter_GObj* g)
{ CHECK(g==&source);record(MotionAir,0,0,0,0,0,0);fighter.self_vel.x+=.25f;fighter.self_vel.y-=.5f;fighter.self_vel.z+=.125f; }
void ft_80084FA8(Fighter_GObj* g)
{ CHECK(g==&source);record(MotionGround,0,0,0,0,0,0);fighter.self_vel.x+=.625f; }
bool ftCommon_CalcSelfAccel_DeaccelQuickAir(Fighter* f)
{ CHECK(f==&fighter);record(Quick,0,0,0,0,0,0);f->self_vel.x*=.75f;return true; }
void ftCommon_UpdateFacing(Fighter* f) { CHECK(f==&fighter);f->facing_dir=f->input.lstick[0].x>=0?1:-1; }
void ftPartSetRotY(Fighter* f,int bone,float angle)
{ CHECK(f==&fighter&&bone==0);record(Rotate,bone,0,angle,0,0,0); }
void ft_80083B68(Fighter_GObj* g) { CHECK(g==&source);record(AirCollision,0,0,0,0,0,0); }
void ft_80084104(Fighter_GObj* g) { CHECK(g==&source);record(GroundCollision,0,0,0,0,0,0); }
bool ftCo_80096CC8(Fighter_GObj* g,int arg) { (void)g;(void)arg;unexpected();return false; }
void ft_800831CC(Fighter_GObj* g,bool(*check)(Fighter_GObj*,int),HSD_GObjEvent cb)
{ CHECK(g==&source&&check==ftCo_80096CC8&&cb);record(AirCollision,1,0,0,0,0,0);if(contact)cb(g); }
void ftCo_LandingFallSpecial_Enter(Fighter_GObj* g,bool interrupt,float lag)
{ CHECK(g==&source);record(Landing,interrupt,0,lag,0,0,0);fighter.motion_id=ftCo_MS_LandingFallSpecial;retire(); }
void ftCo_80096900(Fighter_GObj* g,int a,int b,bool interrupt,float mobility,float lag)
{ CHECK(g==&source);record(Freefall,a,b,interrupt,mobility,lag,0);fighter.motion_id=ftCo_MS_FallSpecial;retire(); }
void ftCo_ItemParasolOpen_Enter(Fighter_GObj* g,float blend)
{ CHECK(g==&source);record(Open,fighter.motion_id,0,blend,0,0,0);fighter.motion_id=scripted&&definition->on_held_motion?definition->on_held_motion(g,ftCo_MS_ItemParasolOpen,fighter.motion_id):(fighter.u.pe.parasol_gobj_0?369:ftCo_MS_ItemParasolOpen);retire(); }
static void ftPe_SpecialHi_8011D424(HSD_GObj* g);
static void ftPe_SpecialHi_8011D620(HSD_GObj* g);
static void ftPe_SpecialHi_8011D650(HSD_GObj* g);
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* out) { (void)joint;(void)offset;(void)out;unexpected(); }
Item_GObj* it_802BDA64(HSD_GObj* parent,Vec3* pos,Fighter_Part part,float facing)
{ (void)parent;(void)pos;(void)part;(void)facing;unexpected();return NULL; }
void ftCommon_8007E6DC(Fighter_GObj* g,Item_GObj* i,s32 flag) { (void)g;(void)i;(void)flag;unexpected(); }
void it_8026BB44(Item_GObj* g) { (void)g;unexpected(); }
void ftPe_Init_OnDeath2(Fighter_GObj* g) { (void)g;unexpected(); }
ItemKind itGetKind(Item_GObj* g) { CHECK(g==&object||g==&previous);return ((Item*)g->user_data)->kind; }
void it_8026BB20(Item_GObj* g) { CHECK(g==&previous);record(Visible,1,0,0,0,0,0); }
void it_8026B724(Item_GObj* g) { CHECK(g==&object||g==&previous);record(Pause,g==&previous,1,0,0,0,0); }
void it_8026B73C(Item_GObj* g) { CHECK(g==&object||g==&previous);record(Pause,g==&previous,0,0,0,0,0); }
void ftpickupitem_80094818(Fighter_GObj* g,bool flag)
{ CHECK(g==&source&&flag&&fighter.item_gobj==&previous);record(Pickup,1,0,0,0,0,0); }
void Item_8026A8EC(Item_GObj* g) { CHECK(g==&object);record(Remove,0,0,0,0,0,0);heads[HSD_GOBJ_PLINK_ITEM]=&previous; }
void Item_80268E5C(Item_GObj* g,int state,Item_StateChangeFlags flags)
{ CHECK(g==&object);record(ItemChange,state,flags,item.x5D0_animFrameSpeed,0,0,0);item.msid=state;item.on_accessory=NULL;
  if(mutation==3)heads[HSD_GOBJ_PLINK_ITEM]=&previous;else if(mutation==4)item.x1C++; }
void Item_802694CC(Item_GObj* g) { CHECK(g==&object);record(Advance,0,0,0,0,0,0); }
void Item_8026AE84(Item* i,enum_t sound,u8 pan,u8 volume)
{ CHECK(i==&item);record(Sound,sound,pan,volume,0,0,0); }
bool it_8026B894(Item_GObj* g,HSD_GObj* other)
{ CHECK(g==&object);record(Interaction,other==&source,0,0,0,0,0);return false; }
#include "actual.inc"
#include "generated.inc"
#include "parameters.inc"
#include "attributes.inc"
static unsigned field(const char* name)
{ for(unsigned i=0;i<definition->state_field_count;i++)if(!strcmp(name,definition->state_fields[i].name))return definition->state_fields[i].offset;CHECK(false);return 0; }
static void integer(const char* n,int32_t value) { script_state_set_int(&fighter,field(n),value); }
static int32_t get_integer(const char* n) { return script_state_get_int(&fighter,field(n)); }
static int32_t held_id(Fighter_GObj* g,bool auxiliary)
{ CHECK(g==&source&&!auxiliary);return fighter.item_gobj?((Item*)fighter.item_gobj->user_data)->x1C:-1; }
static uint32_t current_owner(Item_GObj* g)
{ CHECK(g==&object);for(HSD_GObj* live=heads[HSD_GOBJ_PLINK_FIGHTER];live;live=live->next)if(live==item.owner)return ((Fighter*)live->user_data)->x8_spawnNum;return 0; }
static Fighter* target(uint32_t id)
{ for(HSD_GObj* live=heads[HSD_GOBJ_PLINK_FIGHTER];live;live=live->next)if(((Fighter*)live->user_data)->x8_spawnNum==id)return live->user_data;return NULL; }
static int32_t target_motion(uint32_t id)
{ Fighter* f=target(id);return f?f->motion_id:-1; }
static int32_t held_motion(uint32_t id)
{ Fighter* f=target(id);return f&&f->item_gobj?((Item*)f->item_gobj->user_data)->msid:-1; }
static float held_frame(uint32_t id)
{ Fighter* f=target(id);return f&&f->item_gobj?((Item*)f->item_gobj->user_data)->x5CC_currentAnimFrame:0; }
static bool target_bool(uint32_t id,uint32_t hash)
{ if(id!=fighter.x8_spawnNum||!recipient_export||heads[HSD_GOBJ_PLINK_FIGHTER]!=&source)return false;
  for(unsigned i=0;i<definition->callback_export_count;i++)if(definition->callback_exports[i].name_hash==hash&&definition->callback_exports[i].bool_callback){bool result=definition->callback_exports[i].bool_callback(&source);
    if(mutation==5)heads[HSD_GOBJ_PLINK_ITEM]=&previous;else if(mutation==6)item.x1C++;
    return result;}
  CHECK(false);return false; }
static bool bind_existing(Fighter_GObj* g,uint32_t id,bool aux)
{ CHECK(g==&source&&!aux);if(id!=previous_handle||heads[HSD_GOBJ_PLINK_ITEM]==NULL)return false;fighter.item_gobj=&previous;return true; }
static bool visible(uint32_t id,bool value) { CHECK(id==previous_handle&&value);it_8026BB20(&previous);return true; }
static bool paused(uint32_t id,bool value)
{ HSD_GObj* g=id==handle?&object:id==previous_handle?&previous:NULL;if(!g)return false;if(value)it_8026B724(g);else it_8026B73C(g);return true; }
static void play_sound(Item_GObj* g,s32 sound,u8 p,u8 v) { CHECK(g==&object);Item_8026AE84(&item,sound,p,v); }
static bool notify_neutral(Fighter_GObj* g,bool* claimed)
{ CHECK(g==&source);if(claimed)*claimed=false;if(fighter.item_gobj!=&object)return false;if(claimed)*claimed=true;parasol->on_holder_neutral(&object);return true; }
bool nativeHeldArticle_NotifyNeutral(Fighter_GObj* g,bool* claimed) { return notify_neutral(g,claimed); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k)
{ return scripted&&k==fighter.kind?definition:NULL; }
void ftCo_DownSpot_Enter(Fighter_GObj* g) { CHECK(g==&source);record(NeutralBookkeeping,1,0,0,0,0,0); }
bool ftCo_800C5240(Fighter_GObj* g) { CHECK(g==&source);return hammer; }
void ftCo_800C4ED8(Fighter_GObj* g) { CHECK(g==&source);record(NeutralBookkeeping,2,0,0,0,0,0); }
void ftCommon_8007D7FC(Fighter* f) { CHECK(f==&fighter);f->ground_or_air=GA_Ground;record(GroundSetup,0,0,0,0,0,0); }
bool ftCo_8008A698(Fighter* f) { CHECK(f==&fighter);return false; }
struct Fighter_WaitAnimData* ftData_80085FD4(Fighter* f,FtMotionId id) { (void)f;(void)id;unexpected();return NULL; }
void ftCo_8008A6D8(Fighter_GObj* g,s32 id) { (void)g;(void)id;unexpected(); }
void ftCommon_8007EFC0(Fighter* f,u32 value) { CHECK(f==&fighter&&value==common_data.x5F0);record(NeutralBookkeeping,3,0,0,0,0,0); }
void ftLk_AttackAir_800EB3BC(Fighter_GObj* g) { (void)g;unexpected(); }
void ftCl_Init_8014919C(Fighter_GObj* g) { (void)g;unexpected(); }
void ftCo_FallAerial_Enter(Fighter_GObj* g) { (void)g;unexpected(); }
void ftCo_Fall_Enter(Fighter_GObj* g) { (void)g;unexpected(); }
void ftCo_800968C8(Fighter_GObj* g) { ftCo_80096900(g,1,0,true,1,fighter.x2EC); }
void ftCo_80095744(Fighter_GObj* g,bool* flag,struct NativeItemLifetime* retained) { CHECK(g==&source&&flag==NULL&&retained==NULL);record(Throw,0,0,0,0,0,0); }
static bool signal_article(HSD_GObj* g,uint32_t id,uint32_t hash,float value)
{ CHECK(g==&source&&value==0);if(id!=handle||heads[HSD_GOBJ_PLINK_ITEM]!=&object)return false;
  for(unsigned i=0;i<parasol->signal_count;i++)if(parasol->signals[i].name_hash==hash){parasol->signals[i].callback(&object,value);return true;}CHECK(false);return false; }
#include "entries.inc"
static void common_policy(Fighter_GObj* g) { (void)g;unexpected(); }
static void seed(bool script,unsigned v,unsigned motion)
{ scripted=script;count=0;mutation=0;recipient_export=true;hammer=false;
  memset(&fighter,0,sizeof fighter);memset(&transferred_holder,0,sizeof transferred_holder);memset(&item,0,sizeof item);memset(&previous_item,0,sizeof previous_item);memset(heads,0,sizeof heads);
  source=(HSD_GObj){.user_data=&fighter};object=(HSD_GObj){.user_data=&item};previous=(HSD_GObj){.user_data=&previous_item};holder_object=(HSD_GObj){.user_data=&transferred_holder};object.next=&previous;source.next=&holder_object;
  transferred_holder.gobj=&holder_object;transferred_holder.kind=Ft_Kind_Mars;transferred_holder.x8_spawnNum=UINT32_C(0xf0000002);
  heads[HSD_GOBJ_PLINK_FIGHTER]=&source;heads[HSD_GOBJ_PLINK_ITEM]=&object;
  fighter.gobj=&source;fighter.kind=Ft_Kind_Peach;fighter.behavior_kind=script?Ft_Kind_None:Ft_Kind_Peach;fighter.x8_spawnNum=UINT32_C(0xf0000001);fighter.dat_attrs=&data;fighter.parts=bones;bones[0].joint=&root;
  memcpy(&data,dat_attribute_bits,sizeof dat_attribute_bits);
  fighter.motion_id=motion;fighter.ground_or_air=(v&1)?GA_Ground:GA_Air;
  fighter.self_vel=(Vec3){(v&2)?-3.25f:2.75f,(v&4)?-2.5f:1.75f,.125f};
  fighter.co_attrs.gravity=.09375f;fighter.co_attrs.terminal_velocity=2.75f;
  fighter.input.lstick[0].x=((int)(v%17)-8)/8.f;fighter.facing_dir=(v&8)?-1:1;
  fighter.lstick_angle=(v&16)?-.3125f:.125f;fighter.cmd_vars[0]=(v&32)?1:0;fighter.cmd_vars[2]=(v&64)?7:0;
  fighter.throw_flags=(v&128)?8:0;fighter.x2EC=19.75f;
  fighter.item_gobj=(v&256)?&object:NULL;fighter.u.pe.parasol_gobj_0=(v&512)?&object:NULL;
  integer("parasol_handle",fighter.u.pe.parasol_gobj_0?(int32_t)handle:-1);integer("saved_handle",-1);integer("toad_handle",-1);integer("vegetable_handle",-1);
  item.entity=&object;item.x1C=handle;item.kind=(v&1024)?It_Kind_Parasol:It_Kind_Peach_Parasol;item.owner=&source;
  previous_item.entity=&previous;previous_item.x1C=previous_handle;previous_item.kind=It_Kind_Sword;
  remaining=(v&4)!=0;contact=(v&8)!=0;
}
typedef struct Projection { Vec3 velocity;float angle,facing,rate;int motion,commands[3],request,item_state;unsigned flags;bool immune,accessory,owner,held_previous,damage,death; } Projection;
static Projection project(void)
{ Projection p={0};p.velocity=fighter.self_vel;p.angle=fighter.lstick_angle;p.facing=fighter.facing_dir;p.rate=item.x5D0_animFrameSpeed;p.motion=fighter.motion_id;
  for(unsigned i=0;i<3;i++)p.commands[i]=fighter.cmd_vars[i];p.request=scripted?get_integer("parasol_request_kind"):fighter.mv.pe.specialhi.kind;
  p.item_state=item.msid;p.flags=fighter.throw_flags;p.immune=fighter.x2222_b2;p.accessory=fighter.accessory4_cb!=NULL;p.owner=item.owner!=NULL;p.held_previous=fighter.item_gobj==&previous;p.damage=fighter.take_dmg_cb!=NULL;p.death=fighter.death3_cb!=NULL;return p; }
static void compare_projection(Projection a,Projection b)
{ close_float(a.velocity.x,b.velocity.x);close_float(a.velocity.y,b.velocity.y);close_float(a.velocity.z,b.velocity.z);close_float(a.angle,b.angle);close_float(a.facing,b.facing);close_float(a.rate,b.rate);
  a.velocity=b.velocity=(Vec3){0};a.angle=b.angle=a.facing=b.facing=a.rate=b.rate=0;if(memcmp(&a,&b,sizeof a))fprintf(stderr,"projection motion%d/%d request%d/%d flags%u/%u accessory%d/%d owner%d/%d held%d/%d\n",a.motion,b.motion,a.request,b.request,a.flags,b.flags,a.accessory,b.accessory,a.owner,b.owner,a.held_previous,b.held_previous);CHECK(!memcmp(&a,&b,sizeof a)); }
static HSD_GObjEvent retail(unsigned id,unsigned phase)
{ static HSD_GObjEvent callbacks[4][4]={{ftPe_SpecialHiStart_Anim,ftPe_SpecialHiStart_IASA,ftPe_SpecialHiStart_Phys,ftPe_SpecialHiStart_Coll},{ftPe_SpecialHiEnd_Anim,ftPe_SpecialHiEnd_IASA,ftPe_SpecialHiEnd_Phys,ftPe_SpecialHiEnd_Coll},{ftPe_SpecialAirHiStart_Anim,ftPe_SpecialAirHiStart_IASA,ftPe_SpecialAirHiStart_Phys,ftPe_SpecialAirHiStart_Coll},{ftPe_SpecialAirHiEnd_Anim,ftPe_SpecialAirHiEnd_IASA,ftPe_SpecialAirHiEnd_Phys,ftPe_SpecialAirHiEnd_Coll}};return callbacks[id-361][phase]; }
static HSD_GObjEvent authored(unsigned id,unsigned phase)
{ const MotionState* m=&definition->motion_states[id-341];switch(phase){case 0:return m->anim_cb;case 1:return m->input_cb;case 2:return m->phys_cb;default:return m->coll_cb;} }
int main(void)
{ CHECK(ftCo_MS_ItemParasolOpen==144 && ftCo_MS_ItemParasolFallSpecial==146 && ftCo_MS_FallSpecial==35 && ftCo_MS_FallSpecialB==37 && ftCo_MS_LandingFallSpecial==43); MotionState states[30]={0},common[341]={0};for(unsigned i=0;i<30;i++){states[i].anim_id=400+i;states[i].x4_flags=0x100+i;}
  for(unsigned id=144;id<=146;id++)common[id]=(MotionState){.anim_cb=common_policy,.input_cb=common_policy,.phys_cb=common_policy,.coll_cb=common_policy};
  NativeFighterDefinition base={.fighter_root="ftDataPeach",.source_kind=Ft_Kind_Peach,.motion_states=states,.motion_state_first=341,.motion_state_count=30,.joint_count=114};NativeFighterHost host={0};
#include "hosts.inc"
  host.common_motion_states=common;host.callback_source_alive=nativeCallbackSource_Alive;
  host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;host.animation_frames_remaining=ftAnim_IsFramesRemaining;
  host.air_motion=ft_80085154;host.ground_motion=ft_80084FA8;host.fall=ftCommon_Fall;host.quick_air_decel=ftCommon_CalcSelfAccel_DeaccelQuickAir;
  host.set_root_rotation_y=ftPartSetRotY;host.air_standard_collision=ft_80083B68;host.ground_standard_collision=ft_80084104;
  host.air_special_landing_collision=ft_800831CC;host.fall_special_ledge_check=ftCo_80096CC8;host.enter_landing_fall_special=ftCo_LandingFallSpecial_Enter;
  host.enter_freefall=ftCo_80096900;host.enter_held_open=ftCo_ItemParasolOpen_Enter;host.held_item_id=held_id;host.held_item_kind=itGetKind;
  host.item_current_owner=current_owner;host.target_motion=target_motion;host.target_held_motion=held_motion;host.target_held_frame=held_frame;host.target_call_bool=target_bool;
  host.bind_existing_item=bind_existing;host.set_item_visible=visible;host.set_item_paused=paused;host.notify_item_pickup=ftpickupitem_80094818;
  host.item_play_sound=play_sound;host.item_change_state_flags=Item_80268E5C;host.item_advance_animation=Item_802694CC;host.item_remove=Item_8026A8EC;host.item_interact=it_8026B894;host.notify_held_neutral=notify_neutral;
  host.signal_article=signal_article;
  CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof parameter_bits/4,parameter_schema));definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);
  for(unsigned i=0;i<definition->article_logic_count;i++)if(definition->article_logics[i].kind==It_Kind_Peach_Parasol)parasol=&definition->article_logics[i];CHECK(parasol);
  CHECK(definition->special_hi&&definition->special_air_hi&&definition->on_held_motion&&definition->on_held_timer_expired&&definition->on_neutral_enter&&definition->on_landing_action&&definition->on_air_resume_motion);
  CHECK(definition->motion_states[28].phys_cb==common[144].phys_cb&&definition->motion_states[29].phys_cb==common[146].phys_cb);
  for(unsigned id=361;id<=364;id++)for(unsigned phase=0;phase<4;phase++)for(unsigned v=0;v<2048;v++){
    seed(false,v,id);retail(id,phase)(&source);Projection e=project();save();seed(true,v,id);authored(id,phase)(&source);compare_projection(e,project());compare();
  }
  for(unsigned v=0;v<2048;v++)for(unsigned air=0;air<2;air++){
    seed(false,v,29);(air?ftPe_SpecialAirHi_Enter:ftPe_SpecialHi_Enter)(&source);Projection e=project();save();seed(true,v,29);(air?definition->special_air_hi:definition->special_hi)(&source);compare_projection(e,project());compare();
  }
  /* Dense tilt points exercise the original double denominator near its strict threshold. */
  for(unsigned v=0;v<8192;v++){
    seed(false,v,361);fighter.input.lstick[0].x=((int)v-4096)/4096.f;fighter.cmd_vars[0]=0;fighter.lstick_angle=0;ftPe_SpecialHiStart_IASA(&source);Projection e=project();save();
    seed(true,v,361);fighter.input.lstick[0].x=((int)v-4096)/4096.f;fighter.cmd_vars[0]=0;fighter.lstick_angle=0;authored(361,1)(&source);compare_projection(e,project());compare();
  }
  /* Execute full actual common entries with the generated policies in this same module. */
  for(unsigned operation=0;operation<3;operation++)for(unsigned v=0;v<128;v++){
    Projection e={0};bool old_active=false,old_blocked=false,old_interrupt=false;int old_timer=0;
    for(unsigned s=0;s<2;s++){
      seed(s,0,361);fighter.ground_or_air=(v&1)?GA_Air:GA_Ground;fighter.x2224_b2=(v&2)!=0;hammer=(v&4)!=0;
      fighter.item_gobj=(v&8)?&object:(v&16)?&previous:NULL;
      fighter.u.pe.parasol_gobj_0=(v&32)?&object:NULL;integer("parasol_handle",(v&32)?(int32_t)handle:-1);
      fighter.u.pe.parasol_gobj_1=(v&64)?&previous:NULL;integer("saved_handle",(v&64)?(int32_t)previous_handle:-1);
      fighter.x2221_b4=true;fighter.x2104=1;fighter.u.pe.specialairn_used=true;fighter.native_module_state[field("toad_air_boost_used")]=1;
      if(operation==0){if(!s)original_ft_8008A348(&source,10);else ft_8008A348(&source,10);}
      else if(operation==1){if(!s)original_ftCo_Landing_Enter(&source,ftCo_MS_Landing,(v&2)!=0,0,0,1);else ftCo_Landing_Enter(&source,ftCo_MS_Landing,(v&2)!=0,0,0,1);}
      else {if(!s)original_timer(&source);else candidate_timer(&source);}
      if(!s){e=project();old_active=fighter.x2221_b4;old_blocked=fighter.x2221_b5;old_interrupt=fighter.mv.co.landing.allow_interrupt;old_timer=fighter.x2104;save();}
      else{Projection a=project();a.request=e.request=0; /* Inactive special union aliases landing fields; not a surviving request. */
        compare_projection(e,a);if(operation==1)CHECK(!fighter.native_module_state[field("toad_air_boost_used")]);CHECK(old_active==fighter.x2221_b4&&old_blocked==fighter.x2221_b5&&old_interrupt==fighter.mv.co.landing.allow_interrupt&&old_timer==fighter.x2104);compare();}
    }
  }
  for(unsigned id=0;id<371;id++){
    seed(false,0,id);original_fn_800D2890(&source,id);HSD_GObjEvent e=fighter.mv.co.mushroom.resume_air;save();
    seed(true,0,id);CHECK(ftCo_Mushroom_SelectAirResume(&source,id));CHECK(e==fighter.mv.co.mushroom.resume_air);compare();
  }
  /* Producer completion and direct callback bindings; no constructor branch runs here. */
  for(unsigned held=0;held<2;held++)for(unsigned requested=0;requested<2;requested++){
    if(requested&&!held)continue;
    seed(false,held?512:0,361);fighter.mv.pe.specialhi.kind=requested?It_Kind_Capsule:It_Kind_Peach_Parasol;
    ftPe_SpecialHi_8011D424(&source);CHECK(fighter.pre_hitlag_cb==ftPe_SpecialHi_8011D620&&fighter.post_hitlag_cb==ftPe_SpecialHi_8011D650);
    fighter.pre_hitlag_cb(&source);fighter.post_hitlag_cb(&source);Projection e=project();save();
    seed(true,held?512:0,361);integer("parasol_request_kind",requested?It_Kind_Capsule:It_Kind_Peach_Parasol);
    script_fn_create_parasol(&source);CHECK(fighter.pre_hitlag_cb==script_fn_pause_parasol&&fighter.post_hitlag_cb==script_fn_resume_parasol);
    fighter.pre_hitlag_cb(&source);fighter.post_hitlag_cb(&source);compare_projection(e,project());compare();
  }
  for(unsigned state=0;state<3;state++)for(unsigned motion=0;motion<371;motion++)for(unsigned held=0;held<3;held++)for(unsigned frame=0;frame<4;frame++){
    bool e,a;Projection ep;for(unsigned s=0;s<2;s++){
      seed(s,0,motion);item.msid=state;item.x5CC_currentAnimFrame=frame==0?0:frame==1?15.999f:frame==2?16:17;
      fighter.item_gobj=held==0?&object:held==1?&previous:NULL;previous_item.msid=2;previous_item.x5CC_currentAnimFrame=item.x5CC_currentAnimFrame;
      if(motion==ftCo_MS_LandingFallSpecial&&held==2)continue; /* Native NULL held dereference is not a defined pair. */
      if(!s){e=itPeachparasol_UnkMotion2_Anim(&object);ep=project();save();}else{a=parasol->logic->states[state].animated(&object);if(e!=a)fprintf(stderr,"expiry state%u motion%u held%u frame%u old%d new%d\n",state,motion,held,frame,e,a);CHECK(e==a);compare_projection(ep,project());compare();}
    }
  }
  /* Healthy retain phases observe a transferred holder, never the creator's motion/item. */
  const int retained[]={361,362,363,364,369,370,ftCo_MS_ItemParasolOpen,ftCo_MS_ItemParasolFall,ftCo_MS_ItemParasolFallSpecial,ftCo_MS_FallSpecial,ftCo_MS_FallSpecialF,ftCo_MS_FallSpecialB,ftCo_MS_LandingFallSpecial};
  for(unsigned state=0;state<3;state++)for(unsigned m=0;m<sizeof retained/sizeof *retained;m++){
    bool e;for(unsigned s=0;s<2;s++){
      seed(s,0,29);item.owner=&holder_object;item.msid=state;transferred_holder.motion_id=retained[m];transferred_holder.item_gobj=&previous;previous_item.msid=1;previous_item.x5CC_currentAnimFrame=99;
      if(!s){e=itPeachparasol_UnkMotion2_Anim(&object);CHECK(!e);save();}
      else{CHECK(parasol->logic->states[state].animated(&object)==e);CHECK(get_integer("parasol_handle")==-1);compare();}
    }
  }
  const float durations[]={0,.125f,1,10,15,16,100},rates[]={.125f,1,2.75f};
  for(unsigned state=0;state<3;state++)for(unsigned action=4;action<=6;action+=2)for(unsigned d=0;d<7;d++)for(unsigned r=0;r<3;r++){
    seed(false,0,361);item.msid=state;float length=action==4?it_802BDA40(&object,1):it_802BDA40(&object,2);float rate=durations[d]==0?rates[r]:rates[r]*(length/durations[d]);
    (action==4?it_802BDD40:it_802BDDB4)(&object,rate);Projection e=project();save();seed(true,0,361);item.msid=state;CHECK(parasol->on_held_request(&object,action,rates[r],durations[d]));compare_projection(e,project());compare();
  }
  for(unsigned v=0;v<32;v++){
    seed(false,v,361);itPeachParasol_Logic60_PickedUp(&object);Projection e=project();save();seed(true,v,361);parasol->logic->picked_up(&object);compare_projection(e,project());compare();
  }
  for(int state=0;state<3;state++){seed(true,0,361);item.msid=state;CHECK(parasol->on_held_status(&object)==(state==1?4:6));}
  for(int action=-2;action<9;action++){if(action==4||action==6)continue;seed(true,0,361);CHECK(!parasol->on_held_request(&object,action,1,10)&&count==0);safety++;}
  for(int change=1;change<=2;change++)for(int air=0;air<2;air++){
    seed(true,0,29);mutation=change;fighter.cmd_vars[0]=19;(air?definition->special_air_hi:definition->special_hi)(&source);CHECK(count==1&&fighter.cmd_vars[0]==19);safety++;
  }
  for(int change=3;change<=4;change++){
    seed(true,0,361);mutation=change;parasol->logic->picked_up(&object);CHECK(count==1&&item.x5D0_animFrameSpeed==16&&item.on_accessory==NULL);safety++;
  }
  for(int change=5;change<=6;change++)for(unsigned state=0;state<3;state++){
    seed(true,0,29);mutation=change;CHECK(!parasol->logic->states[state].animated(&object));CHECK(item.owner==&source);safety++;
  }
  /* No owner, missing recipient export, and missing held instance use explicit safe policy. */
  seed(true,0,29);item.owner=NULL;CHECK(parasol->logic->states[0].animated(&object));safety++;
  seed(true,0,29);recipient_export=false;CHECK(parasol->logic->states[0].animated(&object)&&item.owner==&source);safety++;
  seed(true,0,361);item.owner=&holder_object;transferred_holder.motion_id=29;CHECK(parasol->logic->states[0].animated(&object)&&item.owner==&holder_object);CHECK(transferred_holder.item_gobj==NULL);safety++;
  seed(true,0,ftCo_MS_LandingFallSpecial);fighter.item_gobj=NULL;CHECK(!parasol->logic->states[0].animated(&object));safety++;
  printf("PASS %u original/generated comparisons, %u checks, %u authored safety cases, maxfloat %.9g\n",pairs,checks,safety,maximum);return 0;
}
