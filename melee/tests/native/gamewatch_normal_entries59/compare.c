// Actual generated normal policies versus unchanged source-derived decisions.
#include "support.h"
#include "boundaries.h"
#include "original.h"
#include "matrix.h"
#include "unused_host_functions.h"
int main(void)
{
 NativeFighterHost host={0};
#include "unused_hosts.h"
 host.callback_source_alive=nativeCallbackSource_Alive;host.item_creator=creator;
 host.item_current_owner=current_owner;host.target_motion=target_motion;
 host.target_is_fighter=target_is_fighter;host.target_smash_charge_active=target_charge;
 host.signal_owned_article=signal;host.remove_owned_article=remove_owned;
 host.item_change_state_flags=Item_80268E5C;host.item_advance_animation=Item_802694CC;
 host.item_pause=it_8026B724;host.item_resume=it_8026B73C;
 host.item_set_joint_value=joint_set;host.model_animation_active=animation_active;
 host.model_animation_rate=lb_8000BA0C;host.item_interact=interaction;
 host.item_remove=Item_8026A8EC;host.held_item_id=held_id;host.bind_existing_item=bind_held;
 host.set_item_visible=visible;host.set_item_paused=paused;host.notify_item_pickup=ftpickupitem_80094818;
 host.set_visibility_group=ftParts_80074A4C;host.fighter_item_pickup=Fighter_OnItemPickup;
 host.fighter_item_drop=Fighter_OnItemDrop;host.fighter_item_visible=Fighter_OnItemVisible;
 host.fighter_item_invisible=Fighter_OnItemInvisible;host.start_color=ftCo_800BFFD0;
 host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.change_motion_state=Fighter_ChangeMotionState;
 host.start_animation=ftAnim_8006EBA4;host.finish_ground_action=ft_8008A2BC;
 host.enter_fall=ftCo_Fall_Enter;host.enter_crouch_wait=ftCo_800D638C;
 host.ground_motion=ft_80084FA8;host.ground_friction=ft_80084F3C;host.air_drift=ft_80084DB0;
 host.ground_standard_collision=ft_80084104;host.ground_nudge_collision=ft_80084280;
 host.air_aerial_landing_collision=ft_80082C74;host.try_ground_pickup=ftpickupitem_80094790;
 host.enter_jab=ftCo_Attack11_EnterMotion;host.enter_rapid_jab=ftCo_800D6B00;
 host.update_rapid_jab=ftCo_800D6C60;host.enter_landing_aerial=ftCo_LandingAir_EnterWithMsidLag;
 host.enter_landing_basic=ftCo_Landing_Enter_Basic;host.ground_actions=ftCo_Wait_IASA;
 host.check_side_smash=ftCo_AttackS4_CheckInput;host.check_up_smash=ftCo_AttackHi4_CheckInput;
 host.check_down_smash=ftCo_AttackLw4_CheckInput;host.check_side_tilt=ftCo_AttackS3_CheckInput;
 host.check_up_tilt=ftCo_AttackHi3_CheckInput;host.check_down_tilt=ftCo_AttackLw3_CheckInput;
 host.check_jab=ftCo_Attack1_CheckInput;host.check_ground_jump=ftCo_Jump_CheckInput;
 host.check_dash=ftCo_Dash_CheckInput;host.check_crouch=ftCo_Squat_CheckInput;
 host.check_turn=ftCo_Turn_CheckInput;host.check_walk=ftCo_Walk_CheckInput;
 host.bone_count=bone_count;host.joint_position=lb_8000B1CC;host.create_article_at=create;host.notify_item_drop=drop;
 host.item_joint_world_position=world_position;host.spawn_wind=lb_800119DC;
 static const DbLKind debug_level=DbLKind_Master;host.debug_level=&debug_level;
 static MotionState common[341];host.common_motion_states=common;
 common[44].input_cb=ftCo_Attack11_IASA;common[48].input_cb=ftCo_Attack100Loop_IASA;
 common[65].input_cb=ftCo_AttackAirN_IASA;common[67].input_cb=ftCo_AttackAirB_IASA;
 common[68].input_cb=ftCo_AttackAirHi_IASA;
 base.source_kind=Ft_Kind_GameWatch;base.joint_count=53;base.motion_state_first=341;
 base.motion_state_count=40;base.motion_states=motions;
 base.special_ground_state[0]=353;base.special_air_state[0]=354;
 base.special_ground_state[1]=355;base.special_air_state[1]=364;
 base.special_ground_state[2]=373;base.special_air_state[2]=374;
 base.special_ground_state[3]=375;base.special_air_state[3]=378;
 definition=FreakMelee_GetFighterModule(&base,&host);check(definition!=NULL,"generated complete normal module");
 if(!definition)return 2;
 check(FreakMelee_SetFighterParameters(fixture_parameter_bits,FIXTURE_PARAMETER_COUNT,FIXTURE_PARAMETER_SCHEMA),"parameters");
 phases();entries();setups();wind();
 printf("Normal entries: %u checks, %u failures, %u pairs; phases%u entries%u setup%u wind%u\n",
        checks,failures,pairs,phase_pairs,entry_pairs,setup_pairs,wind_pairs);
 return failures?1:0;
}
