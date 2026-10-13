// Actual original Dair/passive-shield/S4 bodies versus composed callbacks.
// Common input acceptance, animation clip and damage scaling are shared explicit
// boundaries. Capsule enable/victim clearing and carried sphere setup are actual.
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackS4.h>
#include <melee/ft/ftattacks4combo.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/lb/lbcollision.h>
#include <melee/lb/lbanim.h>
static unsigned dair_cases,shield_cases,combo_cases;
// These C-reference actors have no interpreted callback slots. Only the actual
// native constructor's slot-clear request is admitted; it remains unavailable.
bool nativeScriptFighter_BindCallback(Fighter_GObj* g,uint32_t slot,uint32_t function)
{CHECK(g&&slot==NativeScriptFighterCallback_ShieldHit&&function==UINT32_MAX);return false;}
struct Fighter_ShakeTable_t* Fighter_SmashChargeShakeTable; // Unused enum channel traps if queried.
static int accepted_control;
static unsigned selected_aerial;
static float clip_end=43.25f;
static FigaTree clip_token;
// Deterministic common input decisions identify ordering independently of
// authored caller; Catch itself remains the actual previously accepted body.
static bool control_check(Fighter_GObj* g,unsigned id)
{CHECK(creator_side(g)==current);event(41,id,0);return accepted_control==(int)id;}
bool ftCo_SpecialS_CheckInput(Fighter_GObj* g){return control_check(g,0);}
bool ftCo_Attack100_CheckInput(Fighter_GObj* g){return control_check(g,1);}
bool ftCo_800D6824(Fighter_GObj* g){return control_check(g,2);}
bool ftCo_800D68C0(Fighter_GObj* g){return control_check(g,3);}
bool ftCo_AttackS4_CheckInput(Fighter_GObj* g){return control_check(g,5);}
bool ftCo_AttackHi4_CheckInput(Fighter_GObj* g){return control_check(g,6);}
bool ftCo_AttackLw4_CheckInput(Fighter_GObj* g){return control_check(g,7);}
bool ftCo_AttackS3_CheckInput(Fighter_GObj* g){return control_check(g,8);}
bool ftCo_AttackHi3_CheckInput(Fighter_GObj* g){return control_check(g,9);}
bool ftCo_AttackLw3_CheckInput(Fighter_GObj* g){return control_check(g,10);}
bool ftCo_Attack1_CheckInput(Fighter_GObj* g){return control_check(g,11);}
bool ftCo_80091A4C(Fighter_GObj* g){return control_check(g,12);}
bool ftCo_800DE9D8(Fighter_GObj* g){return control_check(g,13);}
bool ftCo_Jump_CheckInput(Fighter_GObj* g){return control_check(g,14);}
bool ftCo_Dash_CheckInput(Fighter_GObj* g){return control_check(g,15);}
bool ftCo_800D5FB0(Fighter_GObj* g){return control_check(g,16);}
bool ftCo_Turn_CheckInput(Fighter_GObj* g){return control_check(g,17);}
bool ftCo_Walk_CheckInput(Fighter_GObj* g){return control_check(g,18);}
void ftCo_Wait_IASA(Fighter_GObj* g){CHECK(creator_side(g)==current);event(42,0,0);}
void ft_80084FA8(Fighter_GObj* g){CHECK(creator_side(g)==current);event(43,0,0);}
void ft_80084104(Fighter_GObj* g){CHECK(creator_side(g)==current);event(44,0,0);}
void ftCo_AttackAir_EnterFromCStick(Fighter_GObj* g)
{CHECK(creator_side(g)==current);event(45,selected_aerial,0);((Fighter*)g->user_data)->motion_id=selected_aerial;}
// Damage calculation is not the authored decision under test: same actual
// engine operation boundary receives original owned damage/index in each path.
void ftColl_8007ABD0(HitCapsule* hit,u32 damage,Fighter_GObj* g)
{Fighter* f=g->user_data;CHECK(f==&fighters[current]);unsigned i=hit-f->x914;CHECK(i<4);event(46,i,damage);hit->damage=damage;hit->unk_count=damage;}
static int previous_group(Fighter* f,int group)
{CHECK(f==&fighters[current]&&group==2);return f->x5F4_arr[group].prev;}
#include "closure_originals.h"
static float timing(unsigned i)
{float f;memcpy(&f,fighters[1].native_module_state+field_offset(definition,"dair_timing")+i*4,4);return f;}
static void set_timing(unsigned i,float f)
{memcpy(fighters[1].native_module_state+field_offset(definition,"dair_timing")+i*4,&f,4);}
static void common_reset(void)
{
 reset(0,0,0,0,0,0);animation_remaining=true;accepted_control=-1;
 memset(&phase_attrs,0,sizeof(phase_attrs));
 phase_attrs.attackairlw_hit_vel_y=parameter_float("dair_bounce_y");
 phase_attrs.attackairlw_hit_anim_frame_start=parameter_float("dair_hit_pause");
 phase_attrs.attackairlw_hit_anim_frame_end=clip_end;
 for(unsigned i=0;i<3;i++){char name[32];snprintf(name,sizeof(name),"dair_damage%u",i);phase_attrs.attackairlw_anim_flags[i]=parameter_value(name);}
 phase_attrs.xC4.x0_bone_id=parameter_value("carried_shield_bone");
 phase_attrs.xC4.x4_offset=(Vec3){parameter_float("carried_shield_x"),parameter_float("carried_shield_y"),parameter_float("carried_shield_z")};
 phase_attrs.xC4.x10_size=parameter_float("carried_shield_radius");phase_attrs.xD8=parameter_float("carried_shield_light_amount");
 for(unsigned s=0;s<2;s++){fighters[s].dat_attrs=&phase_attrs;fdata[s].ext_attr=&phase_attrs;fighters[s].parts=parts[s];fighters[s].facing_dir=-1;parts[s][phase_attrs.xC4.x0_bone_id].joint=&common_joints[s];}
 set_timing(1,clip_end);
}
static void common_compare(void)
{
 phase_compare();CHECK(fighters[0].allow_interrupt==fighters[1].allow_interrupt);
 CHECK(fighters[0].cur_anim_frame==fighters[1].cur_anim_frame);
 CHECK(fighters[0].throw_flags==fighters[1].throw_flags);CHECK(fighters[0].facing_dir==fighters[1].facing_dir);
}
static void common_init(NativeFighterHost* host)
{
 host->previous_model_group=previous_group;host->create_guard=ftColl_8007B1B8;host->common_attribute=common_attribute;
 host->set_hitbox_damage=ftColl_8007ABD0;host->clear_hitbox_victims=lbColl_80008440;
 host->start_animation=ftAnim_8006EBA4;host->enter_fall=ftCo_Fall_Enter;host->change_motion_state=Fighter_ChangeMotionState;
 host->check_side_special=ftCo_SpecialS_CheckInput;host->check_up_special=ftCo_Attack100_CheckInput;
 host->check_neutral_special=ftCo_800D6824;host->check_down_special=ftCo_800D68C0;
 host->check_catch=ftCo_Catch_CheckInput;host->check_side_smash=ftCo_AttackS4_CheckInput;host->check_up_smash=ftCo_AttackHi4_CheckInput;
 host->check_down_smash=ftCo_AttackLw4_CheckInput;host->check_side_tilt=ftCo_AttackS3_CheckInput;host->check_up_tilt=ftCo_AttackHi3_CheckInput;
 host->check_down_tilt=ftCo_AttackLw3_CheckInput;host->check_jab=ftCo_Attack1_CheckInput;host->check_guard=ftCo_80091A4C;
 host->check_taunt=ftCo_800DE9D8;host->check_ground_jump=ftCo_Jump_CheckInput;host->check_dash=ftCo_Dash_CheckInput;
 host->check_crouch=ftCo_800D5FB0;host->check_turn=ftCo_Turn_CheckInput;host->check_walk=ftCo_Walk_CheckInput;
 host->ground_actions=ftCo_Wait_IASA;host->ground_motion=ft_80084FA8;host->ground_standard_collision=ft_80084104;
}
static void common_matrix(void)
{
 // Entry selects Dair only; other aerials retain shared ordinary callbacks.
 for(selected_aerial=65;selected_aerial<=69;selected_aerial++){
  common_reset();fighters[0].mv.lk.attackair.lw_frame_start=7;set_timing(0,7);
  current=0;ftLk_AttackAir_Enter(&creators[0]);current=1;ftCo_AttackAir_EnterFromCStick(&creators[1]);definition->on_aerial_attack_enter(&creators[1]);common_compare();
  CHECK((fighters[0].deal_dmg_cb!=NULL)==(fighters[1].deal_dmg_cb!=NULL));
  CHECK((fighters[0].anim_cb!=NULL)==(fighters[1].anim_cb!=NULL));CHECK(fighters[0].mv.lk.attackair.lw_frame_start==timing(0));++dair_cases;
 }
 for(unsigned frame_case=0;frame_case<5;frame_case++)for(unsigned fast=0;fast<2;fast++){
  common_reset();float len=clip_end-phase_attrs.attackairlw_hit_anim_frame_start;
  float frame[]={len-1,len,len+.125f,0,clip_end+3};
  for(unsigned s=0;s<2;s++){fighters[s].motion_id=69;fighters[s].cur_anim_frame=frame[frame_case];fighters[s].fall_fast=fast;fighters[s].self_vel.y=-7;}
  current=0;lwOnHit(&creators[0]);current=1;script_fn_Dair_Hit(&creators[1]);common_compare();
  CHECK(fighters[0].self_vel.y==fighters[1].self_vel.y);CHECK(fighters[0].fall_fast==fighters[1].fall_fast);CHECK(fighters[0].mv.lk.attackair.lw_frame_start==timing(0));++dair_cases;
 }
 float countdowns[]={-1,0,.25f,1,1.5f,2,4};float rates[]={0,.5f,1,2};
 for(unsigned c=0;c<7;c++)for(unsigned r=0;r<4;r++)for(unsigned f=0;f<3;f++)for(unsigned flags=0;flags<4;flags++){
  common_reset();float frames[]={clip_end-.25f,clip_end,clip_end+1};animation_remaining=!(flags&1);
  for(unsigned s=0;s<2;s++){fighters[s].motion_id=69;fighters[s].cur_anim_frame=frames[f];fighters[s].frame_speed_mul=rates[r];fighters[s].mv.lk.attackair.lw_frame_start=countdowns[c];fighters[s].throw_flags=(flags&2)?8:0;
   for(unsigned h=0;h<4;h++){fighters[s].x914[h].state=HitCapsule_Disabled;fighters[s].x914[h].x44=3;fighters[s].x914[h].x45=5;
    for(unsigned v=0;v<12;v++){fighters[s].x914[h].victims_1[v].victim=(void*)(uintptr_t)(v+1);fighters[s].x914[h].victims_2[v].victim=(void*)(uintptr_t)(v+20);}}}
  set_timing(0,countdowns[c]);current=0;lwOnAnim(&creators[0]);current=1;script_fn_Dair_Animation(&creators[1]);common_compare();
  CHECK(fighters[0].mv.lk.attackair.lw_frame_start==timing(0));CHECK(fighters[0].x2219_b3==fighters[1].x2219_b3);
  for(unsigned h=0;h<4;h++)CHECK(!memcmp(&fighters[0].x914[h],&fighters[1].x914[h],sizeof(HitCapsule)));++dair_cases;
 }
 for(unsigned event_kind=0;event_kind<2;event_kind++)for(unsigned previous=0;previous<3;previous++)for(unsigned active=0;active<2;active++){
  common_reset();for(unsigned s=0;s<2;s++){fighters[s].x5F4_arr[2].prev=previous;fighters[s].x5F4_arr[2].idx=2-previous;fighters[s].x221B_b0=active;fighters[s].x221B_b1=true;}
  current=0;if(family==Ft_Kind_Link)ftLk_AttackAir_800EB3BC(&creators[0]);else ftCl_Init_8014919C(&creators[0]);
  current=1;if(event_kind)definition->on_crouch_wait_enter(&creators[1]);else definition->on_wait_enter(&creators[1]);common_compare();
  CHECK(fighters[0].x221B_b0==fighters[1].x221B_b0);CHECK(fighters[0].x221B_b1==fighters[1].x221B_b1);CHECK(fighters[0].x221B_b2==fighters[1].x221B_b2);CHECK(fighters[0].x221B_b3==fighters[1].x221B_b3);CHECK(fighters[0].x221B_b4==fighters[1].x221B_b4);
  CHECK((fighters[0].shield_hit_cb!=NULL)==(fighters[1].shield_hit_cb!=NULL));CHECK(fighters[0].shield_hit.size==fighters[1].shield_hit.size);CHECK(!memcmp(&fighters[0].shield_hit.offset,&fighters[1].shield_hit.offset,sizeof(Vec3)));
  if(!previous)CHECK(fighters[0].shield_hit.bone==&common_joints[0]&&fighters[1].shield_hit.bone==&common_joints[1]);++shield_cases;
 }
 common.x28C=.625f;common.x290=.375f;common.x294=1.25f;common.x2E4=.15f;common.x2E8=.85f;
 float directions[]={-1,0,1};int damage[]={0,1,17,200};
 for(unsigned d=0;d<4;d++)for(unsigned f=0;f<3;f++){
  common_reset();
  for(unsigned s=0;s<2;s++)fighters[s].x5F4_arr[2].prev=0;
  // Deliver contact through the callbacks installed by each actual guard setup.
  current=0;if(family==Ft_Kind_Link)ftLk_AttackAir_800EB3BC(&creators[0]);else ftCl_Init_8014919C(&creators[0]);
  current=1;definition->on_wait_enter(&creators[1]);
  for(unsigned s=0;s<2;s++){CHECK(fighters[s].x221B_b0&&fighters[s].shield_hit_cb);fighters[s].x19A4=damage[d];fighters[s].specialn_facing_dir=directions[f];}
  current=0;fighters[0].shield_hit_cb(&creators[0]);current=1;fighters[1].shield_hit_cb(&creators[1]);common_compare();CHECK(fighters[0].gr_vel==fighters[1].gr_vel);++shield_cases;
 }
 // Every first-swing angle row, full accepted-input priority, held-item Catch
 // priority remains actual. A rejected catch flows to second swing or tail.
 for(unsigned row=58;row<=62;row++)for(unsigned allowed=0;allowed<2;allowed++)for(unsigned command=0;command<2;command++)for(unsigned pressed=0;pressed<2;pressed++)for(int accept=-1;accept<=18;accept++){
  common_reset();accepted_control=accept;
  for(unsigned s=0;s<2;s++){fighters[s].motion_id=row;fighters[s].allow_interrupt=allowed;fighters[s].cmd_vars[0]=command;fighters[s].input.pressed_buttons=pressed?HSD_PAD_A:0;fighters[s].input.held_buttons[0]=0;}
  current=0;ftCo_AttackS4_IASA(&creators[0]);current=1;definition->motion_states[row].input_cb(&creators[1]);common_compare();++combo_cases;
 }
 for(unsigned phase=0;phase<4;phase++)for(unsigned flag=0;flag<2;flag++){
  common_reset();animation_remaining=flag;fighters[0].allow_interrupt=fighters[1].allow_interrupt=flag;fighters[0].motion_id=fighters[1].motion_id=341;
  current=0;if(phase==0)ftCo_AttackS42_Anim(&creators[0]);if(phase==1)ftCo_AttackS42_IASA(&creators[0]);if(phase==2)ftCo_AttackS42_Phys(&creators[0]);if(phase==3)ftCo_AttackS42_Coll(&creators[0]);
  current=1;HSD_GObjEvent cb=phase==0?definition->motion_states[341].anim_cb:phase==1?definition->motion_states[341].input_cb:phase==2?definition->motion_states[341].phys_cb:definition->motion_states[341].coll_cb;CHECK(cb);cb(&creators[1]);common_compare();++combo_cases;
 }
 no_leaks();fprintf(stderr,"closure cases: dair=%u shield=%u combo=%u\n",dair_cases,shield_cases,combo_cases);
}
