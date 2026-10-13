static unsigned normal_pairs,entry_pairs,landing_pairs;
static int accessory_kind(unsigned s){
 HSD_GObjEvent cb=fighters[s].accessory4_cb;if(!cb)return 0;if(cb==dummy_callback)return 1;
 HSD_GObjEvent old[]={ftGw_Attack11_ItemGreenhouseSetup,ftGw_Attack11_DecideAction,ftGw_AttackLw3_ItemManholeSetup,ftGw_ItemTorchSetup,ftGw_AttackAirN_ItemParachuteSetup,ftGw_AttackAirN_ItemTurtleSetup,ftGw_AttackAirN_ItemSparkySetup,ftGw_AttackAirN_ItemParachuteOnLand,ftGw_AttackAirN_ItemTurtleOnLand,ftGw_AttackAirN_ItemSparkyOnLand};
 HSD_GObjEvent owned[]={script_fn_GreenhouseSetup,script_fn_GreenhouseDecide,script_fn_ManholeSetup,script_fn_TorchSetup,script_fn_ParachuteSetup,script_fn_TurtleSetup,script_fn_SparkySetup,script_fn_ParachuteLand,script_fn_TurtleLand,script_fn_SparkyLand};
 for(unsigned i=0;i<10;i++)if(cb==(s?owned[i]:old[i]))return 2+i;
 check(false,"unclassified accessory binding");return -1;
}
static void phase_reset(int motion,unsigned mask,unsigned flags){
 reset(motion,mask,true,flags,4);
 for(unsigned s=0;s<2;s++){
  Fighter*f=fighters+s;f->facing_dir=flags&1?-1:1;f->allow_interrupt=(flags>>1)&1;f->cmd_vars[0]=(flags>>2)&1;f->throw_flags_b3=(flags>>3)&1;f->co_attrs.landingairn_lag=9;f->co_attrs.landingairb_lag=17;f->co_attrs.jab_2_input_window=7;f->hitlag_mul=99;f->unk_msid=99;f->x2218_b1=true;f->x2218_b2=true;f->x1A54=88;f->cur_anim_frame=(flags>>4)&1?0:4;f->frame_speed_mul=1;f->mv.co.attack100.x0=(flags>>5)&1;f->mv.co.attack100.x4=(flags>>6)&1;
 }
 frames_remaining=(flags>>4)&1;ground_lost=(flags>>5)&1;landing_contact=(flags>>6)&1;pickup_consumed=(flags>>7)&1;
 items[0][8].xDD4_itemVar.gamewatchrescue.fighter_owner=owners;
}
static void phase_equal(void){
 equal();Fighter*a=fighters,*b=fighters+1;
 check(a->motion_id==b->motion_id,"final motion");check(a->facing_dir==b->facing_dir,"facing/cue");check(a->throw_flags==b->throw_flags,"throw flags");check(a->allow_interrupt==b->allow_interrupt,"interrupt latch");check(a->cmd_vars[0]==b->cmd_vars[0],"command0");check(a->ground_or_air==b->ground_or_air,"grounded status");check(a->cur_anim_frame==b->cur_anim_frame&&a->frame_speed_mul==b->frame_speed_mul,"frame/rate");check(a->hitlag_mul==b->hitlag_mul&&a->unk_msid==b->unk_msid,"jab common state");check(a->x2218_b1==b->x2218_b1&&a->x2218_b2==b->x2218_b2&&a->x1A54==b->x1A54,"jab latches");check(accessory_kind(0)==accessory_kind(1),"exact authored accessory category");
}
static void normal_phases(void){
 HSD_GObjEvent old[12][4]={
 {ftGw_Attack11_Anim,ftGw_Attack11_IASA,ftGw_Attack11_Phys,ftGw_Attack11_Coll},
 {ftGw_Attack100Start_Anim,ftGw_Attack100Start_IASA,ftGw_Attack100Start_Phys,ftGw_Attack100Start_Coll},
 {ftGw_Attack100Loop_Anim,ftGw_Attack100Loop_IASA,ftGw_Attack100Loop_Phys,ftGw_Attack100Loop_Coll},
 {ftGw_Attack100End_Anim,ftGw_Attack100End_IASA,ftGw_Attack100End_Phys,ftGw_Attack100End_Coll},
 {ftGw_AttackLw3_Anim,ftGw_AttackLw3_IASA,ftGw_AttackLw3_Phys,ftGw_AttackLw3_Coll},
 {ftGw_AttackS4_Anim,ftGw_AttackS4_IASA,ftGw_AttackS4_Phys,ftGw_AttackS4_Coll},
 {ftGw_AttackAirN_Anim,ftGw_AttackAirN_IASA,ftGw_AttackAirN_Phys,ftGw_AttackAirN_Coll},
 {ftGw_AttackAirB_Anim,ftGw_AttackAirB_IASA,ftGw_AttackAirB_Phys,ftGw_AttackAirB_Coll},
 {ftGw_AttackAirHi_Anim,ftGw_AttackAirHi_IASA,ftGw_AttackAirHi_Phys,ftGw_AttackAirHi_Coll},
 {ftGw_LandingAirN_Anim,ftGw_LandingAirN_IASA,ftGw_LandingAirN_Phys,ftGw_LandingAirN_Coll},
 {ftGw_LandingAirB_Anim,ftGw_LandingAirB_IASA,ftGw_LandingAirB_Phys,ftGw_LandingAirB_Coll},
 {ftGw_LandingAirHi_Anim,ftGw_LandingAirHi_IASA,ftGw_LandingAirHi_Phys,ftGw_LandingAirHi_Coll}};
 unsigned masks[]={0,1,2,4,8,16,32,63,64,128,256,511};
 for(unsigned motion=0;motion<12;motion++)for(unsigned phase=0;phase<4;phase++)for(unsigned m=0;m<12;m++)for(unsigned flags=0;flags<256;flags++){
  phase_reset(341+motion,masks[m],flags);label="normal motion phase";accepted_input=flags%13-1;
  select(0);old[motion][phase](owners);select(1);const MotionState*state=definition->motion_states+motion;HSD_GObjEvent next[]={state->anim_cb,state->input_cb,state->phys_cb,state->coll_cb};check(next[phase]!=NULL,"phase present");next[phase](owners+1);phase_equal();++normal_pairs;
 }
 for(unsigned entry=0;entry<4;entry++)for(unsigned flags=0;flags<256;flags++){
  phase_reset(14,511,flags);label="normal entry";select(0);if(entry==0)ftGw_Attack11_Enter(owners);else if(entry==1)ftGw_Attack100Start_Enter(owners);else if(entry==2)ftGw_AttackLw3_Enter(owners);else ftGw_AttackS4_Enter(owners);
  select(1);if(entry==0)definition->on_jab_enter(owners+1);else if(entry==1)definition->on_rapid_jab_enter(owners+1);else if(entry==2)definition->on_down_tilt_enter(owners+1);else definition->on_forward_smash_enter(owners+1,0.75f);phase_equal();++entry_pairs;
 }
 for(int air=65;air<=69;air++)for(unsigned flags=0;flags<256;flags++){
  phase_reset(29,511,flags);label="aerial selection+entry";selected_air_motion=air;select(0);ftGw_AttackAirN_DecideAction(owners);
  select(1);int selected=definition->on_aerial_attack_motion(owners+1,air);ftCo_AttackAir_EnterFromMsid(owners+1,selected);definition->on_aerial_attack_enter(owners+1);phase_equal();++entry_pairs;
 }
 HSD_GObjEvent landing[]={ftGw_LandingAirN_Enter,ftGw_LandingAirB_Enter,ftGw_LandingAirHi_Enter};HSD_GObjEvent scripted[]={script_fn_NeutralLandingEnter,script_fn_BackLandingEnter,script_fn_UpLandingEnter};
 for(unsigned variant=0;variant<3;variant++)for(unsigned mask=0;mask<512;mask++)for(unsigned flags=0;flags<16;flags++){
  phase_reset(347+variant,mask,flags);label="aerial landing entry";select(0);landing[variant](owners);select(1);scripted[variant](owners+1);phase_equal();++landing_pairs;
 }
 printf("Normal new phases: %u motion pairs, %u entry pairs, %u landing pairs\n",normal_pairs,entry_pairs,landing_pairs);
}
