#include "boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_binding_capture.h>
#include <melee/ft/native_script_binding_common_properties.h>
#include <melee/ft/native_fighter_capture.h>
#include <melee/ft/native_fighter_capture_rig.h>
#include <melee/ft/native_capture.h>
#include <melee/ft/kinds/ftCommon/ftCo_CaptureWait.h>
#include <melee/ft/kinds/ftCommon/ftCo_Thrown.h>
#include <melee/ft/kinds/ftCommon/ftCo_CaptureDamageKoopa.h>
#include <melee/ft/kinds/ftCommon/ftCo_CaptureKoopa.h>
#include <melee/ft/kinds/ftCommon/ftCo_Shouldered.h>
#include <melee/ft/kinds/ftCommon/ftCo_ThrownKoopa.h>
#include <melee/ft/kinds/ftCommon/ftCo_Throw.h>
#include <melee/pl/plbonuslib.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/ft/kinds/ftYoshi/ftyoshi.h>
#include <melee/ft/kinds/ftCommon/ftCo_YoshiEgg.h>
#include <sysdolphin/baselib/jobj.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/kinds/ftCommon/ftCo_Damage.h>
#include <melee/it/item.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_0881.h>
#include <melee/pl/plstale.h>
#include <melee/pl/pltrick.h>
#include <melee/it/types.h>
#include <melee/it/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack1.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftdynamics.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ft_0CD1.h>
#include <melee/ft/ftlipstickswing.h>
#include <melee/ft/ftstarrodswing.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/it/it_26B1.h>
#include <melee/lb/lb_00B0.h>
#include <melee/ef/eflib.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/debug.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/fighter.h>
#include <melee/gm/gmvs.h>
#include <melee/it/it_279C.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/class.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <melee/mp/mplib.h>

// Stage-material lookup is outside the capture scheduler cohort.
float mpLib_800569EC(u32 flags){(void)flags;abort();}

enum { N=1024,F=128,V=16 };
static char text[12288];static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[N];static uint32_t arguments[N];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[V];
static NativeScriptInstruction instructions[2048];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[4];
static NativeScriptParameterSlot slots[V];
NativeSourcePackage capture_package;
unsigned presentation_checks;
static bool accessory_case;
static HSD_JObj* accessory_load(HSD_Joint*);
static void accessory_remove(HSD_JObj*);
static void accessory_follow(HSD_JObj*,HSD_JObj*);
HSD_JObjInfo hsdJObj;
bool hsdObjIsDescendantOf(HSD_Obj* object,HSD_ClassInfo* parent)
{if(accessory_case)return object && parent==HSD_CLASS_INFO(&hsdJObj);(void)object;(void)parent;abort();}
void ftCommon_8007F76C(HSD_GObj* object){(void)object;abort();}
static Fighter_GObj* candidate;static Item_GObj* item_candidate;
static unsigned native_catches,native_victims,native_items,cases;
static unsigned retire_peer;static unsigned order;
static MotionState owned_common[341],owned_special[6];
static unsigned trace[32],trace_count,retire_step,retire_target,retire_skip;
static Fighter_GObj* animation_donor;
static Fighter_GObj* retirement_holder;
static bool capture_motion_case;
static bool damage_motion_case;
static unsigned damage_retire_mode;
static unsigned victim_motion_retire_mode;
static bool throw_case;
static HSD_GObj* release_credit;static bool release_upward;
static unsigned release_di_calls;
static float capture_motion_frame,capture_motion_rate;
static u32 capture_motion_flags;
static unsigned capture_hook_peer_change;
static bool throw_presentation_probe,throw_entry_case;
static unsigned throw_presentation_observations,throw_presentation_invincibility,throw_presentation_stock_accessories;
static void step(unsigned id,Fighter_GObj* object);
void capture_hook_observe(Fighter* fp)
{
 if(throw_presentation_probe){
  Fighter* v=fp->victim_gobj->user_data;
  CHECK(!throw_presentation_invincibility);
  CHECK(v->grab_timer==0&&v->x1A6A==511&&v->accessory1_cb==ftCo_800DE508);
  CHECK(v->motion_id==(FtMotionId)(ftCo_MS_ThrownF+fp->motion_id-ftCo_MS_ThrowF));
  CHECK(trace_count&&trace[trace_count-1]==7);
  ++throw_presentation_observations;step(23,fp->gobj);
 }
 if(!capture_hook_peer_change)return;
 if(capture_hook_peer_change==1)++((Fighter*)candidate->user_data)->x8_spawnNum;
 if(capture_hook_peer_change==2)fp->victim_gobj=NULL;
 if(capture_hook_peer_change==3)((Fighter*)candidate->user_data)->victim_gobj=NULL;
}
void fn_800DA054(Fighter_GObj* g){(void)g;abort();}
void fn_800DA190(Fighter_GObj* g){(void)g;abort();}
void ftCo_800DE508(Fighter_GObj* g){(void)g;abort();}
void fn_800DE798(Fighter_GObj* g){(void)g;abort();}
void ftCo_8008E5A4(Fighter* fp){CHECK(fp);++release_di_calls;}
void it_802A7840(HSD_GObj* g){(void)g;abort();}
void it_802BAA94(Item_GObj* g){(void)g;abort();}
void ftAnim_SetAnimRate(Fighter_GObj* g,float rate)
{CHECK(throw_case);((Fighter*)g->user_data)->frame_speed_mul=rate;step(18,g);}
void ft_80085134(Fighter_GObj* g){CHECK(throw_case);step(19,g);((Fighter*)g->user_data)->self_vel.y-=0.125f;}
void ft_80085004(Fighter_GObj* g){CHECK(throw_case);step(20,g);((Fighter*)g->user_data)->gr_vel+=0.25f;}
void pl_80040614(int player,bool team,float timer)
{(void)player;(void)team;CHECK(timer==17);step(16,candidate);}
void ftCommon_8007E358(Fighter_GObj* g){step(17,g);}

static unsigned metadata_counters;static bool metadata_item;
static unsigned segment_records;static u16 segment_instance;
u16 plStale_IncrementAttackInstance(void){return ++segment_instance;}
void pl_80037C60(Fighter_GObj* actor,volatile s32 previous)
{CHECK(actor&&previous==0);++segment_records;}
u16 plAttack_80037B08(void){++metadata_counters;return 77;}
bool it_8026B6C8(Item_GObj* item){CHECK(item);return metadata_item;}
static void step(unsigned id,Fighter_GObj* object){CHECK(trace_count<32);trace[trace_count++]=id;
 if(id==retire_step){
  if(retire_skip){--retire_skip;return;}
  if(victim_motion_retire_mode==1)((Fighter*)candidate->user_data)->victim_gobj=NULL;
  else if(victim_motion_retire_mode==2)
   ++((NativeScriptFighterProgram*)nativeScriptFighter_Program(((Fighter*)object->user_data)->kind))->identity;
  else if(damage_motion_case&&damage_retire_mode==1)
   ++((NativeScriptFighterProgram*)nativeScriptFighter_Program(((Fighter*)object->user_data)->kind))->identity;
  else ++((Fighter*)(retire_target?candidate:retirement_holder)->user_data)->x8_spawnNum;}}
uint32_t nativeFighterTarget_Id(HSD_GObj* actor){return actor?((Fighter*)actor->user_data)->x8_spawnNum:0;}
void ftCommon_8007DB58(Fighter_GObj* actor){step(1,actor);}
void ftCo_8009750C(Fighter_GObj* actor){step(2,actor);}
void ftCo_800DD168(Fighter_GObj* actor){step(3,actor);}
void ftCo_800DB368(Fighter* anchor,Fighter* attached){CHECK(anchor&&attached);step(4,attached->gobj);attached->x2226_b2=true;}
void ftCo_800DB464(Fighter_GObj* actor){(void)actor;abort();}
void ftCommon_8007D5D4(Fighter* actor){step(6,actor->gobj);actor->ground_or_air=GA_Air;}
void ftAnim_8006EBA4(Fighter_GObj* actor){step(7,actor);}
void ftSs_Init_CreateThrowGrappleBeam(HSD_GObj* actor,s32 motion,float rate)
{CHECK(motion==((Fighter*)actor->user_data)->motion_id&&rate==capture_motion_rate);
 ++throw_presentation_stock_accessories;step(22,actor);}
void ftCommon_8007E2FC(Fighter_GObj* actor){step(8,actor);((Fighter*)actor->user_data)->self_vel=(Vec3){0};}
void ftCo_800DE2A8(Fighter_GObj* holder,Fighter_GObj* victim){step(9,holder);
 Fighter* h=holder->user_data;Fighter* v=victim->user_data;h->victim_gobj=h->x1A5C=NULL;h->native_hitlag_link_generation=0;
 v->victim_gobj=v->x1A5C=NULL;v->native_hitlag_link_generation=0;}
void ftCo_800DE7C0(Fighter_GObj* victim,Fighter_GObj* other,bool flag)
{if(!throw_case)CHECK(!other&&!flag);release_credit=other;release_upward=flag;step(10,victim);}
void Fighter_ChangeMotionState(Fighter_GObj* actor,FtMotionId motion,u32 flags,float frame,float rate,float blend,HSD_GObj* other){
 if(damage_motion_case)CHECK(flags==0x40&&frame==0&&rate==1&&blend==0&&!other);
 else if(throw_case){CHECK(blend==0);capture_motion_frame=frame;capture_motion_rate=rate;capture_motion_flags=flags;}
 else if(capture_motion_case){CHECK((flags==0||flags==0x4000||flags==Ft_MF_FreezeState)&&blend==0);capture_motion_frame=frame;capture_motion_rate=rate;capture_motion_flags=flags;}
 else CHECK((flags==0||flags==Ft_MF_Unk06)&&frame==0&&rate==1&&blend==0);
 step(5,actor);animation_donor=other;
 ++presentation_common_motion;((Fighter*)actor->user_data)->motion_id=motion;nativeScriptFighter_SelectMotion(actor,motion);}
static const NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=6};

static float swing_rates[6][5];float (*Fighter_804D654C)[5]=swing_rates;
static unsigned swing_entries;static int swing_motion,swing_variant,swing_kind;
static unsigned jab_restarts;
ftCommonData* p_ftCommonData;
u32 it_804D6D14;
HSD_GObjEvent ftData_OnItemVisible[Ft_Kind_RuntimeCapacity],ftData_OnItemInvisible[Ft_Kind_RuntimeCapacity];
static unsigned visibility_calls,visibility_retire;
static void visible_native(Fighter_GObj* actor)
{++visibility_calls;if(visibility_retire)++((Fighter*)actor->user_data)->x8_spawnNum;}
static void invisible_native(Fighter_GObj* actor)
{visibility_calls+=10;if(visibility_retire)++((Fighter*)actor->user_data)->x8_spawnNum;}
static unsigned shield_entries,shield_reflect_entries,shield_retire;
static unsigned dynamic_calls,dynamic_flag,dynamic_resets,wind_queries;
static int wind_direction;
static unsigned damage_continuations,native_secondary_calls;
static ftDynamics dynamic_definition={.dynamicsNum=1};
static uint8_t callback_snapshot[65536];
static void ftCo_800924C0(Fighter_GObj* actor)
{++shield_entries;if(shield_retire)++((Fighter*)actor->user_data)->x8_spawnNum;}
static void ftCo_80093A50(Fighter_GObj* actor)
{++shield_reflect_entries;if(shield_retire)++((Fighter*)actor->user_data)->x8_spawnNum;}
void ftYs_Init_8012BECC(Fighter_GObj* actor){(void)actor;abort();}
void ftYs_Shield_8012C914(Fighter_GObj* actor){(void)actor;abort();}
int lb_80011ABC(void){++wind_queries;return wind_direction;}
static void ftCo_8009CB40(Fighter* actor,ssize_t index,bool flag,FigaTree* tree)
{CHECK(actor&&index==0&&!tree);++dynamic_calls;dynamic_flag=flag;}
static void ftCo_8009E4A8(Fighter* actor){CHECK(actor);++dynamic_resets;}
void ftAnim_8006EBE8(Fighter_GObj* actor,float frame,float speed,float blend)
{(void)actor;(void)frame;(void)speed;(void)blend;abort();}
void ftAnim_8006E9B4(Fighter_GObj* actor){(void)actor;abort();}
static void native_secondary(Fighter_GObj* actor){CHECK(actor);++native_secondary_calls;}
bool ftCo_Attack11_EnterMotion(Fighter_GObj* actor,FtMotionId motion)
{CHECK(motion==ftCo_MS_Attack11);++jab_restarts;((Fighter*)actor->user_data)->motion_id=motion;return true;}
static void doAttack12Rapid(Fighter_GObj* actor){(void)actor;abort();}
static float swing_rate;
ItemKind itGetKind(Item_GObj* actor){return ((Item*)actor->user_data)->kind;}
bool ftpickupitem_80094790(Fighter_GObj* actor){(void)actor;return false;}
static void swing_entry(Fighter_GObj* actor,int motion,int variant,float rate,int kind)
{CHECK(actor);++swing_entries;swing_motion=motion;swing_variant=variant;swing_rate=rate;swing_kind=kind;}
#define SWING(name,kind) void name(Fighter_GObj*g,int m,int v,float r){swing_entry(g,m,v,r,kind);}
SWING(ftCo_800CD350,0) SWING(ftCo_800CD418,1) SWING(ftCo_800CD4E0,2)
SWING(ftCo_800CD5A8,3) SWING(ftCo_800CD82C,4) SWING(ftCo_800CDAB4,5)
#undef SWING

// JObj animation leaves are controlled; traversal/blending and the binding
// itself below are extracted/linked production code, not a substitute.
static unsigned pose_count;static HSD_JObj* posed[8];static HSD_AnimJoint* pose_anims[8];
void HSD_JObjAddAnim(HSD_JObj*j,HSD_AnimJoint*a,HSD_MatAnimJoint*m,HSD_ShapeAnimJoint*s)
{CHECK(!m&&!s&&pose_count<8);posed[pose_count]=j;pose_anims[pose_count++]=a;}
void HSD_JObjReqAnimByFlags(HSD_JObj*j,u32 flags,float frame){CHECK(j&&flags==1&&frame==0);}
void HSD_JObjAnim(HSD_JObj*j){CHECK(j);}
void ftParts_80074A4C(Fighter_GObj*g,int i,int v){(void)g;(void)i;(void)v;abort();}
void ftParts_80074B0C(Fighter_GObj*g,int i,int v){(void)g;(void)i;(void)v;abort();}
uint8_t nativeCostumeRuntime_ModelVariant(Fighter* fp){(void)fp;return 0;}
void efLib_PauseAll(HSD_GObj*g){CHECK(g);}
void efLib_ResumeAll(HSD_GObj*g){CHECK(g);}
void efLib_DestroyAll(HSD_GObj*g){(void)g;abort();}
void ftCommon_8007DB24(Fighter_GObj*g){(void)g;abort();}
float HSD_Randf(void){abort();}
int HSD_Randi(int n){(void)n;abort();}
void nativeFighterDamage_Self(Fighter_GObj*g,float v){(void)g;(void)v;abort();}
void nativeFighterDamage_Heal(Fighter_GObj*g,float v){(void)g;(void)v;abort();}
s32 ftColl_8007B868(Fighter_GObj*g){(void)g;abort();}
void ftColl_8007B0C0(Fighter_GObj*g,HurtCapsuleState s){CHECK(s==HurtCapsule_Intangible);step(15,g);}
static enum_t victim_color;
static unsigned victim_color_calls;
static int throw_overlay;
static unsigned throw_overlay_clears;
int ftCo_800C0694(Fighter* fp){CHECK(fp);return throw_overlay;}
void timer_color_reset_observer(Fighter* fp,int color)
{CHECK(fp&&color==9&&throw_overlay==9);throw_overlay=0;++throw_overlay_clears;}
bool ftCo_800BFFD0(Fighter* fp,enum_t color,bool flag)
{CHECK(fp&&!flag);++presentation_overlay_calls;victim_color=color;++victim_color_calls;
 if(throw_entry_case&&color==9){++throw_presentation_invincibility;step(24,fp->gobj);}else step(11,fp->gobj);return true;}
void ftCo_800BFFAC(Fighter* fp){(void)fp;abort();}
static unsigned feedback_calls;
bool nativeScriptBindingFeedback_Invoke(NativeScriptRuntimeContext*c,uint32_t semantic,const NativeScriptValue* a,uint32_t n,NativeScriptValue* out,NativeScriptDiagnostic*d)
{
 (void)out;(void)d;CHECK(c);
 if(semantic==NativeScriptBinding_EffectsSpawn){CHECK(n==2);
  CHECK(a[0].type==NativeScriptType_I32&&a[0].as.i64==1290);
  CHECK(a[1].type==NativeScriptType_U32&&a[1].as.u64==FtPart_HipN);
 } else {CHECK(semantic==NativeScriptBinding_EffectsWind&&n==6);
  CHECK(a[0].type==NativeScriptType_U32&&a[0].as.u64==FtPart_HipN);
  for(unsigned i=1;i<6;++i)if(i!=2)CHECK(a[i].type==NativeScriptType_F32);
  CHECK(a[2].type==NativeScriptType_I32&&a[2].as.i64==10);
  CHECK(a[1].as.f32==0&&a[3].as.f32==0.5f&&a[4].as.f32==0.05f);
  CHECK(fabsf(a[5].as.f32-1.0471975511965976f)<1e-6f);
 }
 ++feedback_calls;return false;
}

void presentation_check(bool ok,const char* message,unsigned line)
{++presentation_checks;if(!ok){fprintf(stderr,"capture %u: %s\n",line,message);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&capture_package:NULL;}
bool gm_8016B1C4(void){return false;}
void ftColl_8007BA0C(Fighter_GObj* actor){(void)actor;}
void ftColl_80078A2C(Fighter_GObj* actor){Fighter* fp=actor->user_data;fp->victim_gobj=fp->x1A5C=candidate;
 fp->native_hitlag_link_generation=candidate?((Fighter*)candidate->user_data)->x8_spawnNum:0;}
void ftColl_8007BC90(Fighter_GObj* actor){((Fighter*)actor->user_data)->target_item_gobj=item_candidate;}
void ftColl_80078754(Fighter_GObj* actor,Fighter_GObj* victim,bool flag)
{CHECK(victim==candidate&&!flag);CHECK(((Fighter*)actor->user_data)->cur_pos.x==0);}
void it_8027B4A4(Fighter_GObj* actor,Item_GObj* item){(void)actor;CHECK(item==item_candidate);++order;}

static void native_catch(Fighter_GObj* actor)
{++native_catches;CHECK(!order++);CHECK(((Fighter*)actor->user_data)->cur_pos.x==0);
 if(retire_peer){++((Fighter*)candidate->user_data)->x8_spawnNum;return;}
 ((Fighter*)actor->user_data)->cur_pos.x=3;}
static void native_victim(Fighter_GObj* victim,Fighter_GObj* owner)
{++native_victims;CHECK(order++==1&&victim==candidate);CHECK(((Fighter*)owner->user_data)->cur_pos.x==3);}
static void native_item(Fighter_GObj* owner){(void)owner;++native_items;CHECK(order++==1);}

static HSD_JObj enclosure_accessory,enclosure_child;
struct Fighter_ShakeTable_t* Fighter_GrabMashShake;
void ftCo_800DE2CC(Fighter_GObj* holder,Fighter_GObj* victim)
{step(12,holder);Fighter* h=holder->user_data;Fighter* v=victim->user_data;
 h->victim_gobj=h->x1A5C=NULL;h->native_hitlag_link_generation=0;
 v->victim_gobj=v->x1A5C=NULL;v->native_hitlag_link_generation=0;}
void ftCommon_SetAccessory(Fighter* fp,HSD_Joint* model)
{if(accessory_case){CHECK(!fp->x20A0_accessory);fp->x20A0_accessory=accessory_load(model);return;}CHECK(model&&fp->x20A0_accessory==NULL);step(13,fp->gobj);
 enclosure_accessory=(HSD_JObj){.flags=JOBJ_MTX_INDEP_SRT,.child=&enclosure_child};
 fp->x20A0_accessory=&enclosure_accessory;}
void lb_8000C2F8(HSD_JObj* joint,HSD_JObj* target)
{if(accessory_case){accessory_follow(joint,target);return;}CHECK(joint==&enclosure_child&&target);step(14,candidate);}
void ftCo_800BC438(Fighter_GObj* object){(void)object;abort();}
void ftCo_CaptureEnclosure_Damage(Fighter_GObj* object){(void)object;abort();}
void Fighter_UpdateModelScale(Fighter_GObj* object){(void)object;abort();}
float ftYs_Init_8012BAC0(Fighter* fp){(void)fp;abort();}

// Extracted production functions, including the entire collision dispatch.
static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}
#include "damage_entry_leaves.h"
#include "shield_break_leaves.h"
#include "action_finish_leaves.h"
#include "drop_leaves.h"
#include "walk_leaves.h"
#include "input_query_leaves.h"
#include "damage_tick_leaves.h"
#include "jump_leaves.h"
#include "multi_jump_leaves.h"
#include "attachment_leaves.h"
#include "accessory_leaves.h"
#include "actual_capture.inc"
#include "caught_reference.inc"
#include "damage_reference.inc"
#include "victim_motion_reference.inc"
#include "throw_reference.inc"
#include "action_finish_reference.inc"
#include "drop_reference.inc"
#include "walk_reference.inc"
#include "input_reference.inc"
#include "jump_reference.inc"
#include "attached_capture_reference.inc"
#include "damage_tick_reference.inc"
#include "shield_break_reference.inc"

static void prepare(void)
{
 NativeScriptCompileBank* bank=&capture_package.compiler.bank;
 *bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=N},
  .expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},
  .declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},
  .locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=2048,.function_code=code,.function_code_capacity=F,
  .execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
  .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=4},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* script="fighter:\n    behavior independent\nstate int catches\nstate int victims\nstate int items\nstate float victim_frame\n"
  "table float recoil = [0, -0.51, -1.02, -1.53]\n"
  "func TableRead():\n    state.victim_frame = table.recoil[state.items]\n"
  "func ConfigureMultiJump():\n    state.items = int(jumps.configure_multi(3, 0.3, 0.5, 0.8, 0.9, 4.0, 3.5, 3.0, 2.5, 2.0, 5, 341, -1))\n"
  "func MultiSpeed():\n    state.victim_frame = jumps.multi_vertical_speed(state.items)\n"
  "func BoundedDrift():\n    physics.air_drift_with_limits(0.3, 0.5, 1.0)\n"
  "func DriftThreshold():\n    state.victim_frame = air.drift_threshold\n"
  "func LedgeBox():\n    state.catches = int(collision.air_box_ledge(state.items, 8, 0, -4, 4, 3, 5))\n"
  "func MakeHat():\n    if attachments.has_root(\"Accessory_TopN_joint\"):\n        attachments.load(0, \"Accessory_TopN_joint\", 6, -1, 0)\n        attachments.add_dynamics(0, 1, 2)\n        state.items = 1\n"
  "func MakeVisibleHat():\n    attachments.load(0, \"Accessory_TopN_joint\", 6, 1, 4)\n"
  "func ClearHat():\n    attachments.clear_all()\n"
  "func SelectPart():\n    visuals.set_part_animation(3, 2)\n"
  "func ShowPart():\n    visuals.show_part_animation(3)\n"
  "func HidePart():\n    visuals.hide_part_animation(3)\n"
  "func Arm():\n    capture.arm(2)\nfunc Mask():\n    capture.mask(511)\n"
  "func Begin():\n    capture.begin(275, false, false, true, true)\n"
  "func Attach():\n    capture.attach_to_victim()\n    capture.follow_victim()\n    capture.ground_link = true\n"
  "func Release():\n    capture.release_throw()\n"
  "func EncloseCaught():\n    capture.enclose_caught(7)\n    state.items = 99\n"
  "func ConstrainCaught():\n    capture.constrain_caught()\n    state.items = 99\n"
  "action CaughtPolicy:\n    on capture_wait_setup:\n        capture.enclose_caught(7)\n        position.x = 1\n"
  "action CaptureHooks:\n    on catch_pull_frame:\n        position.x = 1\n        return float(state.items) / 4\n"
  "    on throw_victim_setup:\n        capture.set_victim_body_status(2)\n        position.x = 1\n        state.catches = state.catches + 1\n"
  "    on throw_presentation:\n        state.items = event.motion\n        state.victim_frame = event.rate\n        position.x = 2\n        state.victims = state.victims + 1\n"

  "func ConfigureCapture():\n    capture.timer_damage_multiplier = 1\n    capture.timer_decay = 2\n    capture.mash_reduction = 3\n    capture.mash_pause_frames = 4\n    capture.mash_animation_rate = 5\n    capture.wiggle_scale = 6\n    capture.wiggle_limit = 7\n"
  "    state.items = int(capture.timer_damage_multiplier + capture.timer_decay + capture.mash_reduction + capture.mash_pause_frames + capture.mash_animation_rate + capture.wiggle_scale + capture.wiggle_limit)\n"
  "func HideVictim():\n    capture.victim_visible = false\n    capture.set_victim_body_status(2)\n    state.items = int(capture.victim_visible)\n"
  "func ShowVictim():\n    capture.victim_visible = true\n    capture.set_victim_body_status(0)\n    state.items = int(capture.victim_visible)\n"
  "func VictimMotion():\n    capture.set_victim_capture_motion(state.items)\n"
  "func HoldVictim():\n    capture.initialize_hold(state.items)\n"
  "func HoldPose():\n    capture.set_victim_hold_motion(state.items)\n"
  "func ThrowPose():\n    capture.set_victim_throw_motion(state.items, 0.5)\n"
  "func AttachedThrow():\n    capture.set_victim_attached_throw(state.items, 0x0c4c5080, 3.5, true)\n"
  "func SyncFacing():\n    capture.sync_victim_facing()\n"
  "func VictimFrame():\n    state.victim_frame = capture.victim_frame\n"
  "func ThrowTick():\n    throws.update()\n"
  "func ThrowMotion():\n    physics.throw_motion()\n"
  "func Finish():\n    state.finish()\n    state.items = 99\n"
  "func ReleaseDamage():\n    capture.release_damage(state.items != 0, state.catches != 0)\n    state.victims = 99\n"
  "func ReleaseDamageFall():\n    capture.release_damage_fall()\n    state.victims = 99\n"
  "func DropVictim():\n    capture.drop_victim()\n    state.victims = 99\n"
  "func BeginAttached():\n    capture.begin_attached(state.items)\n    state.victims = 99\n"
  "func VictimFreezeState():\n    state.catches = int(capture.victim_freeze_state)\n"
  "func DamageTimer():\n    damage.update_timer()\n    state.items = 99\n"
  "func DamagePhysics():\n    physics.damage_motion()\n    state.items = 99\n"
  "func AnimationFacing():\n    state.victim_frame = facing.animation_direction\n"
  "func WalkEnter():\n    walk.enter(state.items, 0x40, 3.5, 7, 11, 13, 0.5, 1.25, 2, 1)\n"
  "func WalkRate():\n    walk.update_animation()\n"
  "func WalkPhysics():\n    physics.walk()\n"
  "func GroundFacingMotion():\n    physics.ground_animation_facing_motion()\n"
  "func WalkType():\n    state.items = walk.type\n"
  "func WalkRequested():\n    state.items = int(controls.walk_requested())\n"
  "func WalkStopRequested():\n    state.items = int(controls.walk_stop_requested())\n"
  "func TurnRequested():\n    state.items = int(controls.turn_requested())\n"
  "func PlatformDropRequested():\n    state.items = int(controls.platform_drop_requested())\n"
  "func JumpInput():\n    state.items = controls.jump_input()\n"
  "func VictimFamily():\n    state.items = capture.victim_animation_family\n"
  "func ThrowHorizontalThreshold():\n    state.victim_frame = controls.throw_horizontal_threshold\n"
  "func ThrowUpThreshold():\n    state.victim_frame = controls.throw_up_threshold\n"
  "func ThrowDownThreshold():\n    state.victim_frame = controls.throw_down_threshold\n"
  "func PrepareJump():\n    jump.prepare(state.items, false)\n"
  "func PrepareShortHop():\n    jump.prepare(state.items, true)\n"
  "func CheckShortHop():\n    jump.check_short_hop()\n"
  "func LaunchJump():\n    jump.launch(true, state.victim_frame)\n"
  "func LaunchSilentJump():\n    jump.launch(false, state.victim_frame)\n"
  "func JumpPhysics():\n    physics.jump_motion()\n"
  "func PlatformDrop():\n    state.platform_drop(state.items, uint(state.catches), state.victim_frame)\n    state.victims = 99\n"
  "func ReleaseEnclosure():\n    capture.release_into_enclosure()\n    state.items = 99\n"
  "func ConfigureEnclosure():\n    capture.enclosure_launch_x = 1\n    capture.enclosure_launch_y = 2\n    capture.enclosure_launch_z = 3\n    capture.enclosure_damage_behavior = 4\n    capture.enclosure_shrink_fraction = 5\n    capture.enclosure_shrink_frames = 6\n    capture.enclosure_lifetime = 7\n    capture.enclosure_damage_time_scale = 8\n    capture.enclosure_escape_x = 9\n    capture.enclosure_escape_y = 10\n    capture.enclosure_escape_z = 11\n    capture.enclosure_model_slot = 3\n    capture.enclosure_escape_invincibility_frames = 1 != 0\n    state.items = int(capture.enclosure_launch_x + capture.enclosure_launch_y + capture.enclosure_launch_z + capture.enclosure_damage_behavior + capture.enclosure_shrink_fraction + capture.enclosure_shrink_frames + capture.enclosure_lifetime + capture.enclosure_damage_time_scale + capture.enclosure_escape_x + capture.enclosure_escape_y + capture.enclosure_escape_z) + capture.enclosure_model_slot + capture.enclosure_escape_invincibility_frames\n"
  "func MarkEffect():\n    state.catches = state.catches + 1\n    effects.active = true\n    effects.pause_during_hitlag()\n    callbacks.accessory(None)\n"
  "func InstallEffect():\n    callbacks.accessory(MarkEffect)\n"
  "func UnlockECB():\n    collision.unlock_ecb()\n"
  "func ConsumeCue():\n    state.catches = int(cue.consume(state.items))\n"
  "func DamageObservations():\n    state.catches = int(damage.reaction_suppressed)\n    state.victims = damage.knockback_level\n"
  "func NewHitGroup():\n    state.items = articles.new_collision_group()\n"
  "func DamageEnter():\n    damage.enter(state.items, 0)\n"
  "func DamageDefault():\n    damage.default_reaction(0)\n"
  "action DamagePolicy:\n    on knockback_enter:\n        if state.items == 901: position.x = position.x + 1\n"
  "func MaskBone():\n    hurtboxes.set_bone(state.items, 2)\n"
  "func ClearBone():\n    hurtboxes.set_bone(state.items, 0)\n"
  "func SpawnCue():\n    effects.spawn(1290, Hip)\n    effects.active = true\n"
  "func RefreshSegment():\n    stats.refresh_attack()\n"
  "func RecordSegment():\n    stats.record_attack()\n"
  "func WindCue():\n    effects.wind(Hip, 0, 10, 0.5, 0.05, 1.0471975511965976)\n    effects.active = true\n"
  "func Secondary():\n    state.catches = state.catches + 1\n    if state.items == 2: position.x = 1\n"
  "func ShowHeld():\n    items.set_held_visible(true)\n    state.catches = state.catches + 1\n"
  "func HideHeld():\n    items.set_held_visible(false)\n    state.catches = state.catches + 1\n"
  "func ShowAttached():\n    articles.attached_visible = true\n"
  "func HideAttached():\n    articles.attached_visible = false\n"
  "func ReadAttached():\n    state.items = int(articles.attached_visible)\n"
  "action HeldPresentation:\n    on item_visible:\n        state.victims = state.victims + 1\n        if state.items == 2: position.x = 1\n"
  "    on item_invisible:\n        state.victims = state.victims + 10\n        if state.items == 2: position.x = 1\n"
  "func InstallSecondary():\n    callbacks.secondary_damage(Secondary)\n"
  "func ClearSecondary():\n    callbacks.secondary_damage(None)\n"
  "action ShieldBreakPolicy:\n    on shield_break:\n        damage.force_top_blast_ko = state.items != 0\n"
  "        state.victims = state.victims + 1\n        if state.items == 2: position.x = 1\n"
  "action ShieldPresentation:\n    on shield_start:\n        state.victims = state.victims + 1\n"
  "    on shield_reflect_start:\n        state.victims = state.victims + 10\n"
  "action DynamicPolicy:\n    on dynamics_wind:\n"
  "        if state.items == 2: position.x = 1\n"
  "        return state.items != 0\n"
  "action Swing:\n    on item_swing_motion:\n        if event.swing_type == 5:\n            position.x = 1\n"
  "        return 341 + event.swing_type + state.items\n"
#include "accessory_script.inc"
  "func Contact():\n    if target.exists:\n        state.items = target.kind\n"
  "        if target.is_fighter:\n            state.catches = state.catches + 1\n"
  "        if target.is_item:\n            state.victims = state.victims + 1\n"
  "action Classification:\n    on action_metadata:\n"
  "        if action.move_id(event.metadata) == 113:\n            return action.metadata(99, 2, 4, 0)\n"
  "        if action.move_id(event.metadata) == 114:\n            position.x = 1\n"
  "action JabPolicy:\n    on jab_restart:\n"
  "        if state.items == 2: position.x = 1\n"
  "        if state.items == 3: return true\n"
  "        return state.items == 1 and event.motion == 46\n"
  "action Grab:\n    on grab_catch:\n        if victim.exists:\n            state.catches = state.catches + 1\n        position.x = 1\n"
  "        if victim.airborne:\n            state.items = state.items + 1\n"
  "    on grabbed:\n        if victim.exists:\n            state.victims = state.victims + 1\n            position.x = 2\n"
  "    on item_catch:\n        state.items = state.items + 1\n";
 const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic diagnostic;
 NativeScriptCompileRequest request={.source=script,.source_bytes=(uint32_t)strlen(script),.motion_layout=&layout};
 if(!nativeScriptCompile_Build(bank,&request,&accepted,&diagnostic)||
    !nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&capture_package.program,&diagnostic))
 {fprintf(stderr,"prepare: %u:%u: %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
 capture_package.address=&capture_package;capture_package.ready=capture_package.assets.ready=true;
 capture_package.target=Ft_Kind_CustomFirst;capture_package.assets.facts.moves=layout;
 capture_package.assets.common_motions=owned_common;capture_package.assets.motions=owned_special;
}
static Fighter_GObj* create(bool scripted)
{
 Fighter_GObj* actor=presentation_create(scripted?Ft_Kind_CustomFirst:Ft_Kind_Mario,scripted);
 Fighter* fp=actor->user_data;fp->x2225_b1=true;
 if(scripted){fp->ft_data=&capture_package.assets.character.data;gFtDataList[fp->kind]=fp->ft_data;
  ftData_CharacterStateTables[fp->kind]=capture_package.assets.motions;fp->x1C_actionStateList=owned_common;fp->x20_actionStateList=owned_special;}
 return actor;
}
static bool helper(Fighter_GObj* actor,const char* name)
{
 NativeScriptCompileBank* bank=&capture_package.compiler.bank;
 for(uint32_t i=0;i<bank->declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank->tree,declarations[i].name,name)){
  NativeScriptDiagnostic d;NativeScriptValue output;
  return nativeScriptFighter_Helper(actor,((Fighter*)actor->user_data)->x8_spawnNum,i,NULL,0,&output,&d)==NativeScriptVM_Ok;}
 CHECK(false);return false;
}
static void reset(void){candidate=NULL;item_candidate=NULL;native_catches=native_victims=native_items=order=retire_peer=0;
 throw_presentation_probe=throw_entry_case=false;throw_presentation_observations=throw_presentation_invincibility=throw_presentation_stock_accessories=0;
 jump_case=false;
 damage_tick_case=false;damage_exit_calls=0;
 victim_motion_retire_mode=0;
 throw_case=false;release_credit=NULL;release_upward=false;
 capture_motion_case=false;capture_motion_frame=capture_motion_rate=0;capture_motion_flags=capture_hook_peer_change=0;trace_count=retire_step=retire_target=retire_skip=0;animation_donor=retirement_holder=NULL;presentation_reset();}

static void link_pair(Fighter_GObj* owner,Fighter_GObj* victim){candidate=victim;retirement_holder=owner;ftColl_80078A2C(owner);}
static void capture_cases(void)
{
 // Unchanged native Falcon capture entry and the new generic entry see the
 // same controlled leaves. Compare their observable fields and call order.
 for(unsigned source=0;source<2;++source)for(unsigned air=0;air<2;++air)for(unsigned left=0;left<2;++left){
  reset();Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(source!=0);link_pair(owner,victim);
  Fighter* h=owner->user_data;Fighter* v=victim->user_data;h->facing_dir=left?-1:1;v->ground_or_air=air?GA_Air:GA_Ground;
  v->self_vel=(Vec3){1,2,3};ftCo_8009CA0C(victim,owner);
  unsigned expected[32],expected_count=trace_count;memcpy(expected,trace,sizeof(trace));
  CHECK(v->victim_gobj==owner&&v->x1A5C==owner&&v->native_hitlag_link_generation==h->x8_spawnNum);
  CHECK(v->motion_id==275&&v->facing_dir==-h->facing_dir&&v->x221B_b7==!air);
  CHECK(v->accessory1_cb==(air?ftCo_800DB464:NULL)&&v->x1A6A==511&&animation_donor==owner);
  trace_count=0;v->victim_gobj=v->x1A5C=NULL;v->native_hitlag_link_generation=0;v->x2226_b2=false;v->accessory1_cb=NULL;
  v->self_vel=(Vec3){1,2,3};CHECK(helper(owner,"Begin"));
  CHECK(trace_count==expected_count&&!memcmp(trace,expected,trace_count*sizeof(trace[0])));
  CHECK(v->motion_id==275&&v->facing_dir==-h->facing_dir&&v->x221B_b7==!air&&animation_donor==owner);
  CHECK(v->accessory1_cb==(air?ftCo_800DB464:NULL)&&v->x1A6A==511);
  CHECK(v->self_vel.x==0&&v->self_vel.y==0&&v->self_vel.z==0);
  HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
 }
 // Options are general engine choices rather than Falcon-specific policy.
 for(unsigned options=0;options<16;++options)for(unsigned source=0;source<2;++source)for(unsigned air=0;air<2;++air){
  reset();Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(source!=0);link_pair(owner,victim);
  Fighter* h=owner->user_data;Fighter* v=victim->user_data;h->facing_dir=-1;v->ground_or_air=air?GA_Air:GA_Ground;
  bool same=options&1,force=options&2,track=options&4,donor=options&8;
  CHECK(nativeFighterCapture_Begin(victim,owner,(FtMotionId)275,same,force,track,donor));
  CHECK(v->ground_or_air==(air||force?GA_Air:GA_Ground)&&v->x221B_b7==!(air||force));
  CHECK(v->facing_dir==(same?-1:1)&&animation_donor==(donor?owner:NULL));
  CHECK(v->accessory1_cb==((air||force)&&track?ftCo_800DB464:NULL));
  CHECK(v->victim_gobj==owner&&v->x1A5C==owner&&!v->x221B_b5);
  HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
 }
 for(unsigned target=0;target<2;++target)for(unsigned stage=1;stage<=8;++stage){
  if(stage==6)continue; // force-air is covered by the option matrix above
  reset();Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(false);link_pair(owner,victim);
  Fighter* v=victim->user_data;v->ground_or_air=GA_Air;retire_step=stage;retire_target=target;
  // Invalidate either holder or victim inside the controlled native callback.
  CHECK(!nativeFighterCapture_Begin(victim,owner,(FtMotionId)275,false,false,true,true));
  CHECK(trace[trace_count-1]==stage);
  HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
 }
 reset();Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(false);link_pair(owner,victim);
 Fighter* h=owner->user_data;Fighter* v=victim->user_data;v->cur_pos=(Vec3){4,5,6};v->dmg.x1830_percent=42;
 CHECK(helper(owner,"Attach")&&h->accessory4_cb==nativeFighterCapture_FollowVictim&&h->x221B_b7&&h->x2226_b2);
 h->accessory4_cb(owner);CHECK(h->cur_pos.x==4&&h->cur_pos.y==5&&h->cur_pos.z==6);
 NativeScriptRuntimeContext context={.source=owner,.source_class=NativeCallbackSource_Fighter,.generation=h->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptValue out;NativeScriptDiagnostic d;
 CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureHasVictim,&out,&d)&&out.as.u64==1);
 CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureVictimDamage,&out,&d)&&out.type==NativeScriptType_F32&&out.as.f32==42);
 CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureGroundLink,&out,&d)&&out.as.u64==1);
 trace_count=0;CHECK(helper(owner,"Release")&&trace_count==2&&trace[0]==9&&trace[1]==10&&!h->victim_gobj);
 CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureHasVictim,&out,&d)&&out.as.u64==0);
 CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureVictimDamage,&out,&d)&&out.as.f32==0);
 HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
 for(unsigned target=0;target<2;++target){reset();owner=create(true);victim=create(false);link_pair(owner,victim);
  retire_step=9;retire_target=target;CHECK(!nativeFighterCapture_ReleaseThrow(owner)&&trace_count==1);
  HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;}
 reset();owner=create(true);victim=create(false);link_pair(owner,victim);h=owner->user_data;v=victim->user_data;
 NativeFighterCapturePair pair;CHECK(nativeFighterCapture_Retain(owner,&pair));++v->x8_spawnNum;
 CHECK(!nativeFighterCapture_Current(&pair,true)&&!nativeFighterCapture_Retain(owner,&pair));
 CHECK(!nativeFighterCapture_Begin(victim,owner,(FtMotionId)275,false,false,true,true)&&!trace_count);
 HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
 for(unsigned scripted=0;scripted<2;++scripted){reset();owner=create(true);candidate=create(scripted!=0);
  ((Fighter*)candidate->user_data)->ground_or_air=GA_Air;CHECK(helper(owner,"Arm"));Fighter_UnkProcessGrab_8006CA5C(owner);
  CHECK(presentation_state(owner,"items",NULL).as.i64==1&&presentation_state(owner,"catches",NULL).as.i64==1);
  HSD_GObjFree(candidate);HSD_GObjFree(owner);++cases;}
 for(unsigned invalid=0;invalid<3;++invalid){reset();owner=create(true);victim=create(true);link_pair(owner,victim);v=victim->user_data;
  if(invalid==0)v->x1C_actionStateList=NULL;if(invalid==1)v->x20_actionStateList=NULL;
  CHECK(!nativeFighterCapture_Begin(victim,owner,(FtMotionId)(invalid==2?347:275),false,false,true,true)&&!trace_count);
  HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;}
}

static void item_destroy(void* data){(void)data;}
static void contact_cases(void)
{
 for(unsigned scripted=0;scripted<2;++scripted){
  reset();Fighter_GObj* owner=create(true);Fighter_GObj* peer=create(scripted!=0);
  Fighter* fp=owner->user_data;Fighter* other=peer->user_data;
  CHECK(helper(owner,"Contact")&&presentation_state(owner,"catches",NULL).as.i64==0);
  fp->unk_gobj=peer;CHECK(helper(owner,"Contact"));
  CHECK(presentation_state(owner,"items",NULL).as.i64==other->kind);
  CHECK(presentation_state(owner,"catches",NULL).as.i64==1&&presentation_state(owner,"victims",NULL).as.i64==0);
  NativeScriptRuntimeContext context={.source=owner,.source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
  NativeScriptReceiverRecord records[8];NativeScriptReceiverFrame frames[2];
  NativeScriptReceiverPool pool={.records=records,.capacity=8,.frames=frames,.frame_capacity=2};
  NativeScriptReceiverScope scope={.pool=&pool};NativeScriptDiagnostic d;NativeScriptValue token,out;
  CHECK(nativeScriptReceiver_Push(&pool,&context,context.scene_epoch,&capture_package.compiler.bank.declarations,
       fp->kind,nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope.frame,&d));
  CHECK(nativeScriptReceiver_Capture(&scope,&context,NativeScriptReceiver_ContactTarget,true,false,&token,&d));
  fp->unk_gobj=NULL; // The token retains the selected peer rather than following a later relation.
  CHECK(nativeScriptBindingReceiver_Read(&scope,&context,NATIVE_SCRIPT_RECEIVER_IDENTITY|NativeScriptReceiverIdentity_Kind,&token,1,&out,&d)&&out.as.i64==other->kind);
  ++other->x8_spawnNum;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=999};
  CHECK(!nativeScriptBindingReceiver_Read(&scope,&context,NATIVE_SCRIPT_RECEIVER_IDENTITY|NativeScriptReceiverIdentity_Kind,&token,1,&out,&d));
  CHECK(out.type==NativeScriptType_U64&&out.as.u64==999);
  CHECK(nativeScriptBindingReceiver_Read(&scope,&context,NATIVE_SCRIPT_RECEIVER_EXISTS,&token,1,&out,&d)&&!out.as.u64);
  CHECK(nativeScriptReceiver_Pop(&pool,scope.frame,&d));HSD_GObjFree(peer);HSD_GObjFree(owner);++cases;
 }
 reset();Fighter_GObj* owner=create(true);Fighter* fp=owner->user_data;
 Item data={0};Item_GObj* item=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(item);
 data.entity=item;data.x1C=41;data.kind=(ItemKind)7;data.native_article_author_kind=Ft_Kind_None;data.native_creator_kind=Ft_Kind_None;
 GObj_InitUserData(item,6,item_destroy,&data);fp->unk_gobj=item;
 CHECK(helper(owner,"Contact")&&presentation_state(owner,"items",NULL).as.i64==7);
 CHECK(presentation_state(owner,"catches",NULL).as.i64==0&&presentation_state(owner,"victims",NULL).as.i64==1);
 NativeScriptRuntimeContext context={.source=owner,.source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptReceiverRecord records[8];NativeScriptReceiverFrame frames[2];
 NativeScriptReceiverPool pool={.records=records,.capacity=8,.frames=frames,.frame_capacity=2};
 NativeScriptReceiverScope scope={.pool=&pool};NativeScriptDiagnostic d;NativeScriptValue token,out;
 CHECK(nativeScriptReceiver_Push(&pool,&context,context.scene_epoch,&capture_package.compiler.bank.declarations,
      fp->kind,nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope.frame,&d));
 CHECK(nativeScriptReceiver_Capture(&scope,&context,NativeScriptReceiver_ContactTarget,true,false,&token,&d));
 CHECK(!nativeScriptBindingReceiver_Write(&scope,&context,NATIVE_SCRIPT_RECEIVER_IDENTITY|NativeScriptReceiverIdentity_Kind,&token,1,&(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=8},&d)&&data.kind==7);
 CHECK(!nativeScriptBindingReceiver_Read(&scope,&context,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_PositionX,&token,1,&out,&d));
 HSD_GObjFree(item); // Physical slot reuse cannot revive this retained token.
 item=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(item);GObj_InitUserData(item,6,item_destroy,&data);
 CHECK(nativeScriptBindingReceiver_Read(&scope,&context,NATIVE_SCRIPT_RECEIVER_EXISTS,&token,1,&out,&d)&&!out.as.u64);
 fp->unk_gobj=(HSD_GObj*)(uintptr_t)1;
 CHECK(!nativeScriptReceiver_Capture(&scope,&context,NativeScriptReceiver_ContactTarget,false,false,&out,&d));
 fp->unk_gobj=item;item->classifier=0;
 CHECK(!nativeScriptReceiver_Capture(&scope,&context,NativeScriptReceiver_ContactTarget,false,false,&out,&d));
 item->classifier=HSD_GOBJ_CLASS_ITEM;
 CHECK(nativeScriptReceiver_Pop(&pool,scope.frame,&d));HSD_GObjFree(item);HSD_GObjFree(owner);++cases;
}

static void metadata_cases(void)
{
 const uint32_t words[]={0,113,114,98,0x12345671,0xfedcba70,0xfff0ff62,0x100ff};
 for(unsigned held=0;held<2;++held)for(unsigned prior=0;prior<2;++prior)for(unsigned i=0;i<sizeof(words)/sizeof(*words);++i){
  reset();Fighter_GObj* stock=create(false);Fighter_GObj* source=create(true);
  Fighter* a=stock->user_data;Fighter* b=source->user_data;a->behavior_kind=b->behavior_kind=Ft_Kind_Luigi;
  a->kind=Ft_Kind_Luigi;a->x2070.x2073=b->x2070.x2073=prior?words[i]&255u:0;
  memset(&a->x2074,0xa5,sizeof(a->x2074));memset(&b->x2074,0xa5,sizeof(b->x2074));
  a->dmg.x1830_percent=b->dmg.x1830_percent=12.75f;a->is_metal=b->is_metal=true;
  a->x2220_b5=b->x2220_b5=true;a->x221D_b6=b->x221D_b6=true;
  metadata_item=held!=0;a->item_gobj=b->item_gobj=held?(Item_GObj*)stock:NULL;
  metadata_counters=0;ft_800895E0(a,(int32_t)words[i]);unsigned expected=metadata_counters;
  metadata_counters=0;ft_800895E0(b,(int32_t)words[i]);CHECK(nativeScriptFighter_Status(source)==NativeScriptVM_Ok);
  CHECK(metadata_counters==expected&&a->x2070.x2070_int==b->x2070.x2070_int);
  CHECK(!memcmp(&a->x2074,&b->x2074,sizeof(a->x2074)));
  HSD_GObjFree(source);HSD_GObjFree(stock);++cases;
 }
 reset();Fighter_GObj* owner=create(true);Fighter* fp=owner->user_data;
 // Retirement inside the query must not publish classification/statistics.
 fp->x2070.x2073=114;uint32_t old=fp->x2070.x2070_int;
 presentation_retire_mode=1;presentation_retire_at=1;ft_800895E0(fp,114);
 CHECK(nativeScriptFighter_Status(owner)==NativeScriptVM_SourceRetired&&fp->x2070.x2070_int==old);
 presentation_restore();HSD_GObjFree(owner);++cases;
 // No query hook means preserve the incoming word, with no stock Luigi rewrite.
 uint32_t hook=capture_package.program.hooks[NativeScriptFighter_ActionMetadata];
 capture_package.program.hooks[NativeScriptFighter_ActionMetadata]=UINT32_MAX;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
 owner=create(true);fp=owner->user_data;fp->behavior_kind=Ft_Kind_Luigi;ft_800895E0(fp,113);
 CHECK(fp->x2070.x2073==113);HSD_GObjFree(owner);
 capture_package.program.hooks[NativeScriptFighter_ActionMetadata]=hook;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);++cases;
 owner=create(true);fp=owner->user_data;
 NativeScriptRuntimeContext context={.source=owner,.source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptValue args[4],out;NativeScriptDiagnostic d;
 for(unsigned i=0;i<4;++i)args[i]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1};
 CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ActionMetadata,NULL,0,args,4,&out,&d)&&out.type==NativeScriptType_I32&&out.as.i64==0xffffff);
 CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_ActionMoveId,NULL,0,&out,1,&out,&d)&&out.as.i64==255);
 for(unsigned i=0;i<4;++i){
  NativeScriptValue original=args[i];args[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};
  out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=999};
  CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_ActionMetadata,NULL,0,args,4,&out,&d)&&out.type==NativeScriptType_U64&&out.as.u64==999);
  args[i]=original;++cases;
 }
 HSD_GObjFree(owner);++cases;
}

static void segment_cases(void)
{
 for(unsigned record=0;record<2;++record)for(unsigned flags=0;flags<16;++flags){
  reset();Fighter_GObj* stock=create(false);Fighter_GObj* source=create(true);
  Fighter* a=stock->user_data;Fighter* b=source->user_data;
  a->x2068_attackID=b->x2068_attackID=flags+1;
  a->x206C_attack_instance=b->x206C_attack_instance=123;
  memset(&a->x2074,0xa5,sizeof(a->x2074));b->x2074=a->x2074;
  a->x2070.x2070_int=b->x2070.x2070_int=0x12345678;
  a->dmg.x1830_percent=b->dmg.x1830_percent=12.75f;
  a->is_metal=b->is_metal=(flags&1)!=0;a->x221D_b6=b->x221D_b6=(flags&2)!=0;
  a->x2220_b5=b->x2220_b5=(flags&4)!=0;a->x2220_b6=b->x2220_b6=(flags&8)!=0;
  segment_records=metadata_counters=0;segment_instance=500;
  if(record)ft_800892A0(stock);else ft_80089824(stock);
  unsigned expected_records=segment_records,expected_counters=metadata_counters;
  u16 expected_instance=segment_instance;
  segment_records=metadata_counters=0;segment_instance=500;
  CHECK(helper(source,record?"RecordSegment":"RefreshSegment"));
  CHECK(segment_records==expected_records&&metadata_counters==expected_counters&&segment_instance==expected_instance);
  CHECK(a->x2068_attackID==b->x2068_attackID&&a->x206C_attack_instance==b->x206C_attack_instance);
  CHECK(a->x2070.x2070_int==b->x2070.x2070_int&&!memcmp(&a->x2074,&b->x2074,sizeof(a->x2074)));
  HSD_GObjFree(source);HSD_GObjFree(stock);++cases;
 }
}

static void jab_cases(void)
{
 for(unsigned restart=0;restart<2;++restart){
  reset();Fighter_GObj* stock=create(false);Fighter_GObj* source=create(true);
  Fighter* a=stock->user_data;Fighter* b=source->user_data;
  a->kind=restart?Ft_Kind_Mars:Ft_Kind_Mario;
  NativeScriptValue choice={.type=NativeScriptType_I32,.as.i64=restart};presentation_state(source,"items",&choice);
  a->allow_interrupt=b->allow_interrupt=true;a->x2218_b1=b->x2218_b1=true;
  a->motion_id=b->motion_id=ftCo_MS_Attack12;
  jab_restarts=0;doAttack13(stock);unsigned expected=jab_restarts;
  jab_restarts=0;doAttack13(source);
  CHECK(jab_restarts==expected&&a->motion_id==b->motion_id&&a->allow_interrupt==b->allow_interrupt&&a->x2218_b1==b->x2218_b1);
  HSD_GObjFree(source);HSD_GObjFree(stock);++cases;
 }
 reset();Fighter_GObj* source=create(true);Fighter* fp=source->user_data;
 NativeScriptValue choice={.type=NativeScriptType_I32,.as.i64=1};presentation_state(source,"items",&choice);
 // Marth's query is false for jab two, so the actual normal jab-two path runs.
 fp->co_attrs.jab_3_input_window=7;fp->allow_interrupt=fp->x2218_b1=true;jab_restarts=0;
 doAttack12(source);CHECK(!jab_restarts&&fp->motion_id==ftCo_MS_Attack12&&fp->hitlag_mul==7&&fp->unk_msid==ftCo_MS_Attack12&&!fp->allow_interrupt&&!fp->x2218_b1);
 HSD_GObjFree(source);++cases;
 reset();source=create(true);fp=source->user_data;
 choice.as.i64=3;presentation_state(source,"items",&choice);jab_restarts=0;
 doAttack12(source);CHECK(jab_restarts==1&&fp->motion_id==ftCo_MS_Attack11);
 HSD_GObjFree(source);++cases;
 for(unsigned third=0;third<2;++third){
  reset();source=create(true);fp=source->user_data;fp->motion_id=99;
  choice.as.i64=2;presentation_state(source,"items",&choice);
  presentation_retire_mode=1;presentation_retire_at=1;jab_restarts=0;
  if(third)doAttack13(source);else doAttack12(source);
  CHECK(!jab_restarts&&fp->motion_id==99&&nativeScriptFighter_Status(source)==NativeScriptVM_SourceRetired);
  presentation_restore();HSD_GObjFree(source);++cases;
 }
 uint32_t hook=capture_package.program.hooks[NativeScriptFighter_JabRestart];
 capture_package.program.hooks[NativeScriptFighter_JabRestart]=UINT32_MAX;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
 reset();source=create(true);fp=source->user_data;jab_restarts=0;doAttack13(source);
 CHECK(!jab_restarts&&fp->motion_id==ftCo_MS_Attack13);HSD_GObjFree(source);++cases;
 capture_package.program.hooks[NativeScriptFighter_JabRestart]=hook;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
}

static void held_visibility_cases(void)
{
 ftData_OnItemVisible[Ft_Kind_Mario]=visible_native;ftData_OnItemInvisible[Ft_Kind_Mario]=invisible_native;
 for(unsigned held=0;held<2;++held)for(unsigned previous=0;previous<2;++previous)for(unsigned next=0;next<2;++next){
  reset();Fighter_GObj* stock=create(false);Fighter_GObj* source=create(true);
  HSD_GObj* item=held?GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0):NULL;
  HSD_GObj* article=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(article);
  Fighter* a=stock->user_data;Fighter* b=source->user_data;
  a->item_gobj=b->item_gobj=item;a->x221E_b3=b->x221E_b3=previous;
  visibility_calls=0;ftCommon_8007F5CC(stock,next);
  CHECK(helper(source,next?"ShowHeld":"HideHeld"));
  CHECK(a->x221E_b3==b->x221E_b3&&b->x221E_b3==next);
  CHECK(presentation_state(source,"victims",NULL).as.i64==visibility_calls);
  CHECK(presentation_state(source,"catches",NULL).as.i64==1);
  b->x197C=article;b->x221E_b4=false;CHECK(!ftLib_800868D4(source,article));
  CHECK(helper(source,"ShowAttached")&&b->x221E_b4&&ftLib_800868D4(source,article));
  CHECK(helper(source,"ReadAttached")&&presentation_state(source,"items",NULL).as.i64==1);
  CHECK(helper(source,"HideAttached")&&!b->x221E_b4&&!ftLib_800868D4(source,article));
  CHECK(helper(source,"ReadAttached")&&!presentation_state(source,"items",NULL).as.i64);
  CHECK(b->x221E_b3==next);
  a->item_gobj=b->item_gobj=b->x197C=NULL;if(item)HSD_GObjFree(item);HSD_GObjFree(article);
  HSD_GObjFree(source);HSD_GObjFree(stock);++cases;
 }
 for(unsigned next=0;next<2;++next){
  reset();Fighter_GObj* stock=create(false);Fighter_GObj* source=create(true);
  HSD_GObj* item=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);
  Fighter* a=stock->user_data;Fighter* b=source->user_data;
  a->item_gobj=b->item_gobj=item;a->x221E_b3=b->x221E_b3=!next;
  visibility_retire=1;ftCommon_8007F5CC(stock,next);visibility_retire=0;
  NativeScriptValue flag={.type=NativeScriptType_I32,.as.i64=2};presentation_state(source,"items",&flag);
  presentation_retire_mode=1;presentation_retire_at=1;
  CHECK(!helper(source,next?"ShowHeld":"HideHeld"));
  CHECK(nativeScriptFighter_Status(source)==NativeScriptVM_SourceRetired&&a->x221E_b3==b->x221E_b3&&b->x221E_b3==!next);
  presentation_restore();a->item_gobj=b->item_gobj=NULL;HSD_GObjFree(item);HSD_GObjFree(source);HSD_GObjFree(stock);++cases;
 }
}

static void shield_cases(void)
{
 for(unsigned reflect=0;reflect<2;++reflect)for(unsigned retired=0;retired<2;++retired){
  reset();Fighter_GObj* owner=create(true);shield_entries=shield_reflect_entries=0;shield_retire=retired;
  if(reflect)ftCo_800939B4(owner);else ftCo_800923B4(owner);
  CHECK(shield_entries==!reflect&&shield_reflect_entries==reflect);
  if(retired){CHECK(nativeScriptFighter_Status(owner)==NativeScriptVM_SourceRetired);}
  else CHECK(presentation_state(owner,"victims",NULL).as.i64==(reflect?10:1));
  HSD_GObjFree(owner);shield_retire=0;++cases;
 }
}

static void wind_cases(void)
{
 for(unsigned path=0;path<2;++path)for(unsigned wind=0;wind<2;++wind)for(int direction=-1;direction<=1;++direction){
  reset();Fighter_GObj* stock=create(false);Fighter_GObj* source=create(true);
  Fighter* a=stock->user_data;Fighter* b=source->user_data;
  a->kind=wind?Ft_Kind_Mars:Ft_Kind_Mario;
  a->ft_data->x2C=b->ft_data->x2C=&dynamic_definition;
  a->anim_id=b->anim_id=0;a->dynamics_num=b->dynamics_num=1;
  NativeScriptValue choice={.type=NativeScriptType_I32,.as.i64=wind};presentation_state(source,"items",&choice);
  dynamic_calls=dynamic_flag=dynamic_resets=wind_queries=0;wind_direction=direction;
  if(path)ftCo_8009E7B4(a,NULL);else ftCo_8009E614(a);
  unsigned calls=dynamic_calls,flag=dynamic_flag,resets=dynamic_resets,queries=wind_queries;
  dynamic_calls=dynamic_flag=dynamic_resets=wind_queries=0;
  if(path)ftCo_8009E7B4(b,NULL);else ftCo_8009E614(b);
  CHECK(nativeScriptFighter_Status(source)==NativeScriptVM_Ok);
  CHECK(calls==dynamic_calls&&flag==dynamic_flag&&resets==dynamic_resets&&queries==wind_queries);
  HSD_GObjFree(source);HSD_GObjFree(stock);++cases;
 }
 for(unsigned path=0;path<2;++path){
  reset();Fighter_GObj* source=create(true);Fighter* fp=source->user_data;
  fp->ft_data->x2C=&dynamic_definition;fp->anim_id=0;fp->dynamics_num=1;
  NativeScriptValue choice={.type=NativeScriptType_I32,.as.i64=2};presentation_state(source,"items",&choice);
  presentation_retire_mode=1;presentation_retire_at=1;dynamic_calls=dynamic_resets=wind_queries=0;
  if(path)ftCo_8009E7B4(fp,NULL);else ftCo_8009E614(fp);
  CHECK(!dynamic_calls&&!dynamic_resets&&!wind_queries&&nativeScriptFighter_Status(source)==NativeScriptVM_SourceRetired);
  presentation_restore();HSD_GObjFree(source);++cases;
 }
}

static void secondary_damage_cases(void)
{
 reset();Fighter_GObj* owner=create(true);Fighter* fp=owner->user_data;
 CHECK(helper(owner,"InstallSecondary")&&nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_SecondaryDamage)&&!fp->take_dmg_2_cb);
 size_t bytes=0;CHECK(nativeScriptFighter_Save(callback_snapshot,sizeof(callback_snapshot),&bytes));
 damage_continuations=native_secondary_calls=0;dispatch_secondary_damage(owner);
 CHECK(damage_continuations==1&&!native_secondary_calls&&presentation_state(owner,"catches",NULL).as.i64==1);
 CHECK(helper(owner,"ClearSecondary"));dispatch_secondary_damage(owner);
 CHECK(damage_continuations==2&&presentation_state(owner,"catches",NULL).as.i64==1);
 CHECK(nativeScriptFighter_Load(callback_snapshot,bytes));dispatch_secondary_damage(owner);
 CHECK(damage_continuations==3&&presentation_state(owner,"catches",NULL).as.i64==1);
 fp->take_dmg_2_cb=native_secondary;dispatch_secondary_damage(owner);
 CHECK(native_secondary_calls==1&&damage_continuations==4&&!nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_SecondaryDamage));
 CHECK(helper(owner,"InstallSecondary"));nativeScriptFighter_ResetMotionCallbacks(owner);
 CHECK(!nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_SecondaryDamage));HSD_GObjFree(owner);++cases;
 reset();owner=create(true);fp=owner->user_data;
 NativeScriptValue choice={.type=NativeScriptType_I32,.as.i64=2};presentation_state(owner,"items",&choice);
 CHECK(helper(owner,"InstallSecondary"));presentation_retire_mode=1;presentation_retire_at=1;damage_continuations=0;
 dispatch_secondary_damage(owner);CHECK(!damage_continuations&&nativeScriptFighter_Status(owner)==NativeScriptVM_SourceRetired);
 presentation_restore();HSD_GObjFree(owner);++cases;
}

static void swing_cases(void)
{
 const int kinds[]={12,11,13,24,22,23};
 for(int swing=0;swing<6;++swing)for(int variant=0;variant<5;++variant){
  reset();Fighter_GObj* stock=create(false);Fighter_GObj* owner=create(true);
  Fighter* a=stock->user_data;Fighter* b=owner->user_data;a->kind=Ft_Kind_Captain;
  Item item={.kind=(ItemKind)kinds[swing]};HSD_GObj held={.user_data=&item};a->item_gobj=b->item_gobj=&held;
  swing_rates[swing][variant]=(float)(swing*5+variant)+0.5f;
  int expected=get_anim_id(a,swing,variant);CHECK(get_anim_id(b,swing,variant)==expected);
  swing_entries=0;ftCo_Attack_800CCF58(owner,variant);
  CHECK(swing_entries==1&&swing_motion==expected&&swing_variant==variant&&swing_kind==swing&&swing_rate==swing_rates[swing][variant]);
  HSD_GObjFree(owner);HSD_GObjFree(stock);++cases;
 }
 reset();Fighter_GObj* owner=create(true);Fighter* fp=owner->user_data;
 CHECK(get_anim_id(fp,-1,3)==-1&&get_anim_id(fp,6,3)==-1&&get_anim_id(fp,0,-1)==-1&&get_anim_id(fp,0,5)==-1);
 CHECK(helper(owner,"Contact")); // no target, unchanged offset
 NativeScriptValue one={.type=NativeScriptType_I32,.as.i64=6};
 CHECK(presentation_state(owner,"items",&one).as.i64==6);
 CHECK(get_anim_id(fp,0,3)==-1);HSD_GObjFree(owner);++cases;
 // A missing source hook never falls through to the native Captain hook.
 uint32_t hook=capture_package.program.hooks[NativeScriptFighter_ItemSwingMotion];
 capture_package.program.hooks[NativeScriptFighter_ItemSwingMotion]=UINT32_MAX;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
 owner=create(true);fp=owner->user_data;
 CHECK(get_anim_id(fp,0,3)==-1&&get_anim_id(fp,0,0)==120);HSD_GObjFree(owner);++cases;
 capture_package.program.hooks[NativeScriptFighter_ItemSwingMotion]=hook;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);
 owner=create(true);fp=owner->user_data;MotionState* table=fp->x20_actionStateList;fp->x20_actionStateList=NULL;
 CHECK(get_anim_id(fp,0,3)==-1&&get_anim_id(fp,0,0)==-1);fp->x20_actionStateList=table;HSD_GObjFree(owner);++cases;
 // Retiring inside the integer query must not call a swing entry afterwards.
 owner=create(true);fp=owner->user_data;Item item={.kind=(ItemKind)23};HSD_GObj held={.user_data=&item};fp->item_gobj=&held;
 presentation_retire_mode=1;presentation_retire_at=1;swing_entries=0;
 ftCo_Attack_800CCF58(owner,3);CHECK(!swing_entries&&nativeScriptFighter_Status(owner)==NativeScriptVM_SourceRetired);
 presentation_restore();HSD_GObjFree(owner);++cases;
}

#include "part_animation_cases.h"

static void deferred_effect_case(void)
{
 reset();Fighter_GObj* owner=create(true);Fighter* fp=owner->user_data;
 fp->accessory4_cb=native_catch;
 CHECK(!fp->throw_flags_b0&&!fp->x2219_b0&&helper(owner,"InstallEffect")&&!fp->x2219_b0&&!fp->accessory4_cb);
 CHECK(nativeScriptFighter_MotionBound(owner,NativeScriptMotion_Accessory));
 CHECK(nativeScriptFighter_RunMotion(owner,NativeScriptMotion_Accessory)&&fp->x2219_b0);
 CHECK(fp->pre_hitlag_cb==efLib_PauseAll&&fp->post_hitlag_cb==efLib_ResumeAll);
 // Normal accessory helpers do not wait for a throw cue. Self-clearing must
 // survive returning through the VM, and a later transition can rearm them.
 CHECK(!nativeScriptFighter_MotionBound(owner,NativeScriptMotion_Accessory)&&!fp->accessory4_cb);
 CHECK(presentation_state(owner,"catches",NULL).as.i64==1);
 CHECK(helper(owner,"InstallEffect")&&nativeScriptFighter_MotionBound(owner,NativeScriptMotion_Accessory));
 CHECK(nativeScriptFighter_RunMotion(owner,NativeScriptMotion_Accessory));
 CHECK(presentation_state(owner,"catches",NULL).as.i64==2);
 CHECK(!nativeScriptFighter_MotionBound(owner,NativeScriptMotion_Accessory)&&!fp->throw_flags_b0);
 HSD_GObjFree(owner);++cases;
 owner=create(true);fp=owner->user_data;feedback_calls=0;
 // Exercise the real compiler/VM and top-level routing. Feedback is an
 // explicitly unavailable leaf here: failure must prevent the flag write.
 CHECK(!helper(owner,"SpawnCue")&&feedback_calls==1&&!fp->x2219_b0);
 CHECK(nativeScriptFighter_Status(owner)==NativeScriptVM_Error);HSD_GObjFree(owner);++cases;
 owner=create(true);fp=owner->user_data;feedback_calls=0;
 CHECK(!helper(owner,"WindCue")&&feedback_calls==1&&!fp->x2219_b0);
 CHECK(nativeScriptFighter_Status(owner)==NativeScriptVM_Error);HSD_GObjFree(owner);++cases;
}

#include "capture_settings_cases.h"
#include "enclosure_settings_cases.h"
#include "victim_policy_cases.h"
#include "enclosure_release_cases.h"
#include "caught_policy_cases.h"
#include "capture_hook_cases.h"
#include "bone_status_cases.h"
#include "cue_cases.h"
#include "reaction_cases.h"
#include "damage_entry_cases.h"
#include "victim_motion_cases.h"
#include "throw_cases.h"
#include "throw_presentation_cases.h"
#include "throw_invincibility_cases.h"
#include "action_finish_cases.h"
#include "drop_cases.h"
#include "walk_cases.h"
#include "input_query_cases.h"
#include "jump_cases.h"
#include "multi_jump_cases.h"
#include "attachment_cases.h"
#include "accessory_cases.h"
#include "shield_break_cases.h"
#include "attached_capture_cases.h"
#include "damage_tick_cases.h"
#include "table_cases.h"
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();
 table_cases();
 reaction_cases();
 damage_entry_cases();
 victim_motion_cases();
 throw_cases();
 throw_presentation_cases();
 throw_invincibility_cases();
 action_finish_cases();
 drop_cases();
 walk_cases();
 input_query_cases();
 jump_cases();
 multi_jump_cases();
 attachment_cases();
 accessory_cases();
 shield_break_cases();
 attached_capture_cases();
 damage_tick_cases();
 for(unsigned scripted_peer=0;scripted_peer<2;++scripted_peer){reset();Fighter_GObj* owner=create(true);candidate=create(scripted_peer!=0);
  Fighter* fp=owner->user_data;Fighter* peer=candidate->user_data;peer->cur_pos.x=41;
  CHECK(helper(owner,"Arm")&&fp->x221E_b6&&fp->x1A68==2&&!fp->grab_cb&&!fp->grabbed_cb&&!fp->x2194);
  CHECK(helper(owner,"Mask")&&fp->x1A6A==511);Fighter_UnkProcessGrab_8006CA5C(owner);
  CHECK(presentation_state(owner,"catches",NULL).as.i64==1&&presentation_state(owner,"victims",NULL).as.i64==1);
  CHECK(fp->cur_pos.x==2&&peer->cur_pos.x==41&&!native_catches&&!native_victims);
  if(scripted_peer)CHECK(presentation_state(candidate,"catches",NULL).as.i64==0&&presentation_state(candidate,"victims",NULL).as.i64==0);
  HSD_GObjFree(candidate);HSD_GObjFree(owner);++cases;
 }
 reset();Fighter_GObj* owner=create(true);candidate=create(false);Fighter* fp=owner->user_data;
 CHECK(helper(owner,"Arm"));ftCommon_8007E2D0(fp,7,native_catch,NULL,native_victim);Fighter_UnkProcessGrab_8006CA5C(owner);
 CHECK(native_catches==1&&native_victims==1&&order==2&&presentation_state(owner,"catches",NULL).as.i64==0);
 HSD_GObjFree(candidate);HSD_GObjFree(owner);++cases;
 reset();owner=create(true);candidate=create(false);fp=owner->user_data;retire_peer=1;
 ftCommon_8007E2D0(fp,7,native_catch,NULL,native_victim);Fighter_UnkProcessGrab_8006CA5C(owner);
 CHECK(native_catches==1&&!native_victims);HSD_GObjFree(candidate);HSD_GObjFree(owner);++cases;
 reset();owner=create(false);candidate=create(true);fp=owner->user_data;
 ftCommon_8007E2D0(fp,7,native_catch,NULL,native_victim);Fighter_UnkProcessGrab_8006CA5C(owner);
 CHECK(native_catches==1&&native_victims==1&&presentation_state(candidate,"victims",NULL).as.i64==0);
 HSD_GObjFree(candidate);HSD_GObjFree(owner);++cases;
 reset();owner=create(true);candidate=create(false);fp=owner->user_data;CHECK(helper(owner,"Arm"));
 fp->x221F_b3=true;Fighter_UnkProcessGrab_8006CA5C(owner);
 CHECK(fp->cur_pos.x==0&&presentation_state(owner,"catches",NULL).as.i64==0);fp->x221F_b3=false;
 presentation_retire_mode=1;presentation_retire_at=1;Fighter_UnkProcessGrab_8006CA5C(owner);
 CHECK(fp->cur_pos.x==1&&nativeScriptFighter_Status(owner)==NativeScriptVM_SourceRetired);
 CHECK(((Fighter*)candidate->user_data)->cur_pos.x==0);
 presentation_restore();HSD_GObjFree(candidate);HSD_GObjFree(owner);++cases;
 reset();owner=create(true);fp=owner->user_data;CHECK(helper(owner,"Arm"));item_candidate=(Item_GObj*)owner;
 Fighter_UnkProcessGrab_8006CA5C(owner);CHECK(order==1&&presentation_state(owner,"items",NULL).as.i64==1);
 fp->x2194=native_item;order=0;Fighter_UnkProcessGrab_8006CA5C(owner);CHECK(native_items==1&&presentation_state(owner,"items",NULL).as.i64==1);
 HSD_GObjFree(owner);++cases;
 reset();owner=create(true);fp=owner->user_data;
 NativeScriptRuntimeContext context={.source=owner,.source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 for(unsigned invalid=0;invalid<4;++invalid){NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=invalid==0?-1:invalid==1?32768:2};
  if(invalid==2)arg.type=NativeScriptType_U32;if(invalid==3)arg.type=NativeScriptType_F32;
  NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=99};NativeScriptDiagnostic diagnostic;
  CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureArm,&arg,1,&output,&diagnostic));
  CHECK(output.type==NativeScriptType_U64&&output.as.u64==99&&!fp->x221E_b6);++cases;}
 HSD_GObjFree(owner);
 const uint32_t required[]={NativeScriptFighter_GrabCatch,NativeScriptFighter_Grabbed};
 for(unsigned h=0;h<2;++h){CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
  uint32_t original=capture_package.program.hooks[required[h]];capture_package.program.hooks[required[h]]=UINT32_MAX;
  nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);owner=create(true);fp=owner->user_data;
  CHECK(!helper(owner,"Arm")&&!fp->x221E_b6&&!fp->grab_cb&&!fp->grabbed_cb);
  CHECK(nativeScriptFighter_Status(owner)==NativeScriptVM_Error);HSD_GObjFree(owner);
  capture_package.program.hooks[required[h]]=original;CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
  nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&capture_package.program);++cases;
 }
 cue_cases();bone_status_cases();capture_cases();capture_settings_cases();enclosure_integer_cases();victim_policy_cases();enclosure_release_cases();caught_policy_cases();capture_hook_cases();contact_cases();metadata_cases();segment_cases();jab_cases();held_visibility_cases();shield_cases();wind_cases();secondary_damage_cases();swing_cases();part_cases();part_control_cases();deferred_effect_case();CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS capture owner scheduler\",\"checks\":%u,\"cases\":%u,\"throw_presentation_cases\":%u,\"throw_invincibility_cases\":%u,\"accessory_cases\":%u}\n",presentation_checks,cases,throw_presentation_cases_count,throw_invincibility_cases_count,accessory_cases_count);return 0;
}
