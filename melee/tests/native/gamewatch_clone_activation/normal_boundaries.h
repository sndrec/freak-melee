// Controlled common transitions/physics/input/pose boundaries for normal phases.
static bool frames_remaining,pickup_consumed,landing_contact,ground_lost,rapid_end;
static int accepted_input,selected_air_motion;
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId motion,MotionFlags flags,float frame,float speed,float blend,HSD_GObj*anim){
 check(g==owners+side&&blend==0&&anim==NULL,"motion arguments");
 event('M',motion,flags,frame);fighters[side].motion_id=motion;fighters[side].cur_anim_frame=frame;fighters[side].frame_speed_mul=speed;
}
void ftAnim_8006EBA4(Fighter_GObj*g){check(g==owners+side,"animation owner");event('A',0,0,0);}
bool ftAnim_IsFramesRemaining(Fighter_GObj*g){check(g==owners+side,"animation predicate owner");return frames_remaining;}
bool ftpickupitem_80094790(Fighter_GObj*g){check(g==owners+side,"pickup owner");event('I',0,pickup_consumed,0);return pickup_consumed;}
void ft_8008A2BC(Fighter_GObj*g){check(g==owners+side,"wait owner");event('W',0,0,0);fighters[side].motion_id=14;}
void ftCo_800D638C(Fighter_GObj*g){check(g==owners+side,"crouch owner");event('C',0,0,0);fighters[side].motion_id=40;}
void ftCo_Fall_Enter(Fighter_GObj*g){check(g==owners+side,"fall owner");event('F',0,0,0);fighters[side].motion_id=29;}
void ft_80084FA8(Fighter_GObj*g){check(g==owners+side,"ground motion");event('G',0,0,0);}
void ft_80084F3C(Fighter_GObj*g){check(g==owners+side,"ground friction");event('G',1,0,0);}
void ft_80084DB0(Fighter_GObj*g){check(g==owners+side,"air drift");event('G',2,0,0);}
void ft_80084104(Fighter_GObj*g){check(g==owners+side,"ground collision");event('B',0,0,0);if(ground_lost)fighters[side].ground_or_air=GA_Air;}
void ft_80084280(Fighter_GObj*g){check(g==owners+side,"nudge collision");event('B',1,0,0);if(ground_lost)fighters[side].motion_id=29;}
void ft_80082C74(Fighter_GObj*g,HSD_GObjEvent callback){check(g==owners+side,"air landing owner");event('B',2,0,0);if(landing_contact)callback(g);}
void ftCo_Landing_Enter_Basic(Fighter_GObj*g){check(g==owners+side,"basic landing owner");event('L',0,0,0);fighters[side].motion_id=42;}
void ftCo_LandingAir_EnterWithMsidLag(Fighter_GObj*g,FtMotionId motion,float lag){check(g==owners+side,"aerial landing owner");event('L',motion,0,lag);fighters[side].motion_id=motion;}
FtMotionId ftCo_AttackAir_GetMsidFromCStick(Fighter*f){check(f==fighters+side,"air selection owner");return selected_air_motion;}
void ftCo_AttackAir_EnterFromMsid(Fighter_GObj*g,FtMotionId motion){check(g==owners+side,"common aerial owner");event('E',motion,0,0);fighters[side].motion_id=motion;}
void ftCo_800D6B00(Fighter_GObj*g,enum_t motion){
 if(!ftpickupitem_80094790(g)){fighters[side].throw_flags=0;Fighter_ChangeMotionState(g,motion,0,0,1,0,NULL);ftAnim_8006EBA4(g);fighters[side].mv.co.attack100.x0=false;fighters[side].mv.co.attack100.x4=false;}
}
void ftCo_800D6C60(Fighter_GObj*g,HSD_GObjEvent callback){check(g==owners+side,"rapid update owner");event('R',0,0,0);if(rapid_end)callback(g);}
static bool jab_enter(Fighter_GObj*g,FtMotionId motion){
 if(ftpickupitem_80094790(g))return false;
 Fighter*f=fighters+side;f->allow_interrupt=0;f->x2218_b1=0;Fighter_ChangeMotionState(g,motion,0,0,1,0,NULL);ftAnim_8006EBA4(g);f->hitlag_mul=f->co_attrs.jab_2_input_window;f->unk_msid=44;f->x2218_b2=0;f->mv.co.attack1.x0=0;f->x1A54=0;return true;
}
// Binding identities are compared after entry; setup itself is covered by the
// accepted constructor/prop matrices, and is never invoked at this boundary.
void ftGw_Attack11_ItemGreenhouseSetup(HSD_GObj*g){(void)g;event('Z',0,0,0);unused();}
void ftGw_AttackLw3_ItemManholeSetup(HSD_GObj*g){(void)g;event('Z',1,0,0);unused();}
void ftGw_ItemTorchSetup(HSD_GObj*g){(void)g;event('Z',2,0,0);unused();}
void ftGw_AttackAirN_ItemParachuteSetup(HSD_GObj*g){(void)g;event('Z',3,0,0);unused();}
void ftGw_AttackAirN_ItemTurtleSetup(HSD_GObj*g){(void)g;event('Z',4,0,0);unused();}
void ftGw_AttackAirN_ItemSparkySetup(HSD_GObj*g){(void)g;event('Z',5,0,0);unused();}
#define INPUT(name,n) bool name(Fighter_GObj*g){check(g==owners+side,"input owner");event('Q',n,0,0);return accepted_input==n;}
INPUT(ftCo_AttackS4_CheckInput,0) INPUT(ftCo_AttackHi4_CheckInput,1) INPUT(ftCo_AttackLw4_CheckInput,2)
INPUT(ftCo_AttackS3_CheckInput,3) INPUT(ftCo_AttackHi3_CheckInput,4) INPUT(ftCo_AttackLw3_CheckInput,5)
INPUT(ftCo_Attack1_CheckInput,6) INPUT(ftCo_Jump_CheckInput,7) INPUT(ftCo_Dash_CheckInput,8)
INPUT(ftCo_Squat_CheckInput,9) INPUT(ftCo_Turn_CheckInput,10) INPUT(ftCo_Walk_CheckInput,11)
#undef INPUT
void ftCo_Wait_IASA(Fighter_GObj*g){check(g==owners+side,"ground actions owner");event('Q',20,0,0);}
void ftCo_Attack11_IASA(Fighter_GObj*g){check(g==owners+side,"jab input owner");event('Q',21,0,0);}
void ftCo_Attack100Loop_IASA(Fighter_GObj*g){check(g==owners+side,"rapid input owner");event('Q',22,0,0);}
void ftCo_AttackAirN_IASA(Fighter_GObj*g){check(g==owners+side,"air input owner");event('Q',23,0,0);}
void ftCo_AttackAirB_IASA(Fighter_GObj*g){check(g==owners+side,"air input owner");event('Q',24,0,0);}
void ftCo_AttackAirHi_IASA(Fighter_GObj*g){check(g==owners+side,"air input owner");event('Q',25,0,0);}
static MotionState common_states[341];
static void normal_hosts(NativeFighterHost* h){
 h->animation_frames_remaining=ftAnim_IsFramesRemaining;h->change_motion_state=Fighter_ChangeMotionState;h->start_animation=ftAnim_8006EBA4;h->finish_ground_action=ft_8008A2BC;h->enter_crouch_wait=ftCo_800D638C;h->enter_fall=ftCo_Fall_Enter;
 h->ground_motion=ft_80084FA8;h->ground_friction=ft_80084F3C;h->air_drift=ft_80084DB0;h->ground_standard_collision=ft_80084104;h->ground_nudge_collision=ft_80084280;h->air_aerial_landing_collision=ft_80082C74;h->enter_landing_basic=ftCo_Landing_Enter_Basic;h->enter_landing_aerial=ftCo_LandingAir_EnterWithMsidLag;
 h->try_ground_pickup=ftpickupitem_80094790;h->enter_jab=jab_enter;h->enter_rapid_jab=ftCo_800D6B00;h->update_rapid_jab=ftCo_800D6C60;h->ground_actions=ftCo_Wait_IASA;
 h->check_side_smash=ftCo_AttackS4_CheckInput;h->check_up_smash=ftCo_AttackHi4_CheckInput;h->check_down_smash=ftCo_AttackLw4_CheckInput;h->check_side_tilt=ftCo_AttackS3_CheckInput;h->check_up_tilt=ftCo_AttackHi3_CheckInput;h->check_down_tilt=ftCo_AttackLw3_CheckInput;h->check_jab=ftCo_Attack1_CheckInput;h->check_ground_jump=ftCo_Jump_CheckInput;h->check_dash=ftCo_Dash_CheckInput;h->check_crouch=ftCo_Squat_CheckInput;h->check_turn=ftCo_Turn_CheckInput;h->check_walk=ftCo_Walk_CheckInput;
 common_states[44].input_cb=ftCo_Attack11_IASA;common_states[48].input_cb=ftCo_Attack100Loop_IASA;common_states[65].input_cb=ftCo_AttackAirN_IASA;common_states[67].input_cb=ftCo_AttackAirB_IASA;common_states[68].input_cb=ftCo_AttackAirHi_IASA;h->common_motion_states=common_states;
}
