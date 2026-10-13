static unsigned phase_pairs,entry_pairs,setup_pairs,wind_pairs;
static void prepare(int motion,unsigned mask,unsigned bits)
{
 reset(motion,mask,true,0,0);finished=bits&1;collision_trigger=bits&2;pickup=bits&4;spawn_success=bits&8;
 memset(world_reads,0,sizeof(world_reads));memset(fighter_joints,0,sizeof(fighter_joints));memset(children,0,sizeof(children));
 for(unsigned s=0;s<2;s++){
  Fighter*f=fighters+s;f->parts=fighter_parts[s];f->facing_dir=bits&16?-1:1;f->allow_interrupt=bits&32;
  f->throw_flags_b3=bits&16;f->mv.co.attack100.x0=bits&64;f->mv.co.attack100.x4=bits&128;
  f->x2218_b1=f->x2218_b2=true;f->hitlag_mul=9.25f;f->unk_msid=99;f->x1A54=73;
  f->co_attrs.jab_2_input_window=5.75f;f->co_attrs.landingairn_lag=13.5f;f->co_attrs.landingairb_lag=19.25f;
  f->co_attrs.landingairhi_lag=37;f->cmd_vars[0]=(bits>>2)&1;f->cur_anim_frame=(bits&1)?0:1.25f;f->frame_speed_mul=1;
  for(unsigned i=0;i<53;i++)fighter_parts[s][i].joint=&fighter_joints[s][i];
  roots[s][1].child=children+s;
 }
}
static int accessory_tag(HSD_GObjEvent cb)
{
 HSD_GObjEvent originals[]={ftGw_Attack11_ItemGreenhouseSetup,ftGw_Attack11_DecideAction,
  ftGw_AttackLw3_ItemManholeSetup,ftGw_ItemTorchSetup,ftGw_AttackAirN_ItemParachuteSetup,
  ftGw_AttackAirN_ItemTurtleSetup,ftGw_AttackAirN_ItemSparkySetup,ftGw_AttackAirN_ItemParachuteOnLand,
  ftGw_AttackAirN_ItemTurtleOnLand,ftGw_AttackAirN_ItemSparkyOnLand};
 HSD_GObjEvent authored[]={script_fn_GreenhouseSetup,script_fn_GreenhouseDecide,
  script_fn_ManholeSetup,script_fn_TorchSetup,script_fn_ParachuteSetup,script_fn_TurtleSetup,
  script_fn_SparkySetup,script_fn_ParachuteLand,script_fn_TurtleLand,script_fn_SparkyLand};
 if(!cb)return 0;for(unsigned i=0;i<10;i++)if(cb==originals[i]||cb==authored[i])return i+1;
 return 99;
}
static int binding_tag(HSD_GObjEvent cb)
{
 HSD_GObjEvent original[]={ftGw_Init_OnDamage,ftGw_Attack11_ItemGreenhouseEnterHitlag,
  ftGw_Attack11_ItemGreenhouseExitHitlag,ftGw_AttackLw3_ItemManholeEnterHitlag,
  ftGw_AttackLw3_ItemManholeExitHitlag,ftGw_AttackS4_ItemTorchEnterHitlag,
  ftGw_AttackS4_ItemTorchExitHitlag,ftGw_AttackAirN_EnterItemHitlag,ftGw_AttackAirN_ExitItemHitlag};
 HSD_GObjEvent authored[]={script_fn_PropDamage,script_fn_GreenhousePause,script_fn_GreenhouseResume,
  script_fn_ManholePause,script_fn_ManholeResume,script_fn_TorchPause,script_fn_TorchResume,
  script_fn_AirPropsPause,script_fn_AirPropsResume};
 if(!cb)return 0;for(unsigned i=0;i<9;i++)if(cb==original[i]||cb==authored[i])return i+1;
 return 99;
}
static void complete_equal(void)
{
 equal();Fighter*a=fighters,*b=fighters+1;
 check(a->motion_id==b->motion_id,"result motion");check(a->allow_interrupt==b->allow_interrupt,"interrupt latch");
 check(a->x2218_b1==b->x2218_b1&&a->x2218_b2==b->x2218_b2,"jab flags");
 check(a->hitlag_mul==b->hitlag_mul&&a->unk_msid==b->unk_msid&&a->x1A54==b->x1A54,"jab bookkeeping");
 check(a->mv.co.attack100.x0==b->mv.co.attack100.x0&&a->mv.co.attack100.x4==b->mv.co.attack100.x4,"common rapid state");
 check(a->throw_flags==b->throw_flags,"throw/flip flags");check(a->facing_dir==b->facing_dir,"facing");
 check(a->cmd_vars[0]==b->cmd_vars[0],"entry command");check(a->cur_anim_frame==b->cur_anim_frame&&a->frame_speed_mul==b->frame_speed_mul,"transition frame/rate");
 check(a->ground_or_air==b->ground_or_air,"ground/air");
 check(accessory_tag(a->accessory4_cb)==accessory_tag(b->accessory4_cb),"exact accessory decision");
 check(binding_tag(a->pre_hitlag_cb)==binding_tag(b->pre_hitlag_cb)&&binding_tag(a->post_hitlag_cb)==binding_tag(b->post_hitlag_cb),"exact hitlag bindings");
 check(binding_tag(a->death2_cb)==binding_tag(b->death2_cb)&&binding_tag(a->take_dmg_cb)==binding_tag(b->take_dmg_cb),"exact damage/death bindings");
 check(world_reads[0]==world_reads[1],"single world read count");
}
#define PHASES(NAME) {NAME##_Anim,NAME##_IASA,NAME##_Phys,NAME##_Coll}
static HSD_GObjEvent originals[12][4]={
 PHASES(ftGw_Attack11),PHASES(ftGw_Attack100Start),PHASES(ftGw_Attack100Loop),PHASES(ftGw_Attack100End),
 PHASES(ftGw_AttackLw3),PHASES(ftGw_AttackS4),PHASES(ftGw_AttackAirN),PHASES(ftGw_AttackAirB),PHASES(ftGw_AttackAirHi),
 PHASES(ftGw_LandingAirN),PHASES(ftGw_LandingAirB),PHASES(ftGw_LandingAirHi)};
#undef PHASES
static HSD_GObjEvent phase(unsigned motion,unsigned phase)
{const MotionState*m=definition->motion_states+motion;return phase==0?m->anim_cb:phase==1?m->input_cb:phase==2?m->phys_cb:m->coll_cb;}
static void phases(void)
{
 unsigned masks[]={0,1,8,56,511};
 for(unsigned motion=0;motion<12;motion++)for(unsigned p=0;p<4;p++)for(unsigned mask=0;mask<5;mask++)for(unsigned bits=0;bits<256;bits+=3){
  prepare(341+motion,masks[mask],bits);interrupt_stop=((int)bits%13)-1;label="all48 phases";
  select(0);originals[motion][p](owners);select(1);HSD_GObjEvent cb=phase(motion,p);check(cb!=NULL,"authored/common phase present");if(cb)cb(owners+1);
  complete_equal();++phase_pairs;
 }
 // Every ordered DownTilt interruption boundary, enabled and disabled.
 for(int stop=-1;stop<12;stop++)for(unsigned allow=0;allow<2;allow++){
  prepare(345,0,allow?32:0);interrupt_stop=stop;label="DownTilt interrupt order";
  select(0);ftGw_AttackLw3_IASA(owners);select(1);phase(4,1)(owners+1);complete_equal();++phase_pairs;
 }
}
static void entries(void)
{
 HSD_GObjEvent old[]={ftGw_Attack11_Enter,ftGw_Attack100Start_Enter,ftGw_Attack100Loop_Enter,ftGw_Attack100End_Enter,ftGw_AttackLw3_Enter};
 HSD_GObjEvent authored[]={definition->on_jab_enter,definition->on_rapid_jab_enter,script_fn_RapidLoopEnter,script_fn_RapidEndEnter,definition->on_down_tilt_enter};
 for(unsigned e=0;e<6;e++)for(unsigned bits=0;bits<256;bits++){
  prepare(21,0,bits);label="ground entry";select(0);if(e<5)old[e](owners);else ftGw_AttackS4_Enter(owners);
  select(1);if(e<5)authored[e](owners+1);else definition->on_forward_smash_enter(owners+1,0.5f);
  complete_equal();++entry_pairs;
 }
 for(int selected=65;selected<=69;selected++)for(unsigned bits=0;bits<2;bits++){
  prepare(21,0,bits*16);aerial_selected=selected;label="aerial mapping and post-entry";
  select(0);ftGw_AttackAirN_DecideAction(owners);
  select(1);int motion=definition->on_aerial_attack_motion(owners+1,selected);
  ftCo_AttackAir_EnterFromMsid(owners+1,motion);definition->on_aerial_attack_enter(owners+1);
  complete_equal();++entry_pairs;
 }
 HSD_GObjEvent landing[]={ftGw_LandingAirN_Enter,ftGw_LandingAirB_Enter,ftGw_LandingAirHi_Enter};
 HSD_GObjEvent authored_land[]={script_fn_NeutralLandingEnter,script_fn_BackLandingEnter,script_fn_UpLandingEnter};
 for(unsigned e=0;e<3;e++)for(unsigned mask=0;mask<512;mask++)for(unsigned command=0;command<2;command++){
  prepare(347+e,mask,command?4:0);label="three aerial landing entries";
  for(unsigned s=0;s<2;s++)fighters[s].ground_or_air=GA_Air;
  select(0);landing[e](owners);select(1);authored_land[e](owners+1);complete_equal();++entry_pairs;
 }
}
static void setups(void)
{
 HSD_GObjEvent old[]={ftGw_Attack11_ItemGreenhouseSetup,ftGw_AttackLw3_ItemManholeSetup,ftGw_ItemTorchSetup,
  ftGw_AttackAirN_ItemParachuteSetup,ftGw_AttackAirN_ItemTurtleSetup,ftGw_AttackAirN_ItemSparkySetup};
 HSD_GObjEvent authored[]={script_fn_GreenhouseSetup,script_fn_ManholeSetup,script_fn_TorchSetup,
  script_fn_ParachuteSetup,script_fn_TurtleSetup,script_fn_SparkySetup};
 for(unsigned e=0;e<6;e++)for(unsigned mask=0;mask<64;mask++)for(unsigned bits=0;bits<64;bits++){
  prepare(341+bits%12,mask,bits);label="prop setup at controlled construction boundary";
  if(bits&4)for(unsigned s=0;s<2;s++)fighters[s].item_gobj=objects[s]+9;
  if(bits&2)for(unsigned s=0;s<2;s++)fighters[s].death2_cb=fighters[s].take_dmg_cb=NULL;
  select(0);old[e](owners);select(1);authored[e](owners+1);complete_equal();++setup_pairs;
 }
}
static void wind(void)
{
 int commands[]={0,1,2,-1,16777217};
 for(int motion=340;motion<350;motion++)for(unsigned present=0;present<2;present++)for(unsigned c=0;c<5;c++){
  int command=commands[c];
  prepare(motion,511,0);label="Manhole child-world wind";
  for(unsigned s=0;s<2;s++){items[s][1].xDAC_itcmd_var0=command;if(!present)items[s][1].owner=NULL;}
  select(0);bool a=itGamewatchmanhole_UnkMotion0_Anim(objects[0]+1);
  select(1);bool b=logic(1)->logic->states[0].animated(objects[1]+1);
  check(a==b,"Manhole remove result");complete_equal();check(world_reads[1]==(command?1:0),"exactly one child Vec3 read");++wind_pairs;
 }
}
