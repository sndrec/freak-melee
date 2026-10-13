
/* Explicit named projection; no union/padding digest or tolerance masking. */
typedef struct TravelObservation { int32_t i[24];float f[18]; } TravelObservation;
static TravelObservation travel_expected;
static TravelObservation travel_observe(bool hi) {
    TravelObservation o={0};int n=0,m=0;
#define I(v) o.i[n++]=(int32_t)(v)
#define F(v) o.f[m++]=(v)
    I(fighter.motion_id);I(fighter.ground_or_air);I(fighter.cmd_vars[0]);I(fighter.cmd_vars[1]);I(fighter.cmd_vars[2]);I(fighter.cmd_vars[3]);
    I(fighter.x1968_jumpsUsed);I(fighter.invisible);I(fighter.x2223_b4);I(fighter.x1988);I(fighter.ecb_lock);I(fighter.coll_data.x130_flags);
    I(fighter.death2_cb!=NULL);I(fighter.take_dmg_cb!=NULL);I(fighter.accessory4_cb!=NULL);I(fighter.x2219_b0);I(fighter.pre_hitlag_cb!=NULL);I(fighter.post_hitlag_cb!=NULL);
    F(fighter.self_vel.x);F(fighter.self_vel.y);F(fighter.self_vel.z);F(fighter.x74_self_accel.x);F(fighter.x74_self_accel.y);F(fighter.gr_vel);
    F(fighter.facing_dir);F(fighter.cur_anim_frame);F(fighter.frame_speed_mul);F(joint.rotate.x);F(fighter.xE4_ground_accel_1);
    if(hi) {
        I(scripted?get_int(fighter_field("teleport_remaining")):fighter.mv.zd.specialhi.x0);
        I(scripted?get_int(fighter_field("teleport_collision_age")):fighter.mv.zd.specialhi.xC);
        float values[5]={0};if(scripted) {const char*names[]={"teleport_stick_x","teleport_stick_y","teleport_saved_x","teleport_saved_y","teleport_saved_ground"};for(int k=0;k<5;++k)memcpy(&values[k],fighter.native_module_state+fighter_field(names[k]),4);}
        else {values[0]=fighter.mv.zd.specialhi.x4.x;values[1]=fighter.mv.zd.specialhi.x4.y;values[2]=fighter.mv.zd.specialhi.x10.x;values[3]=fighter.mv.zd.specialhi.x10.y;values[4]=fighter.mv.zd.specialhi.x18;}
        for(int k=0;k<5;++k)F(values[k]);
    } else {
        /* Four independent signed timers plus the full-bit retained handle. */
        I(scripted?get_int(fighter_field("din_handle")):(fighter.u.zd.x222C?(int32_t)item.x1C:-1));
        const char*names[]={"din_start_remaining","din_hold_remaining","din_gravity_remaining","din_release_remaining"};
        int32_t values[4]={fighter.mv.zd.specials.x0,fighter.mv.zd.specials.x4,fighter.mv.zd.specials.x8,fighter.mv.zd.specials.xC};
        for(int k=0;k<4;++k)I(scripted?get_int(fighter_field(names[k])):values[k]);
    }
#undef I
#undef F
    return o;
}
static void travel_save(bool hi) { travel_expected=travel_observe(hi);expected_count=event_count;memcpy(expected_events,events,event_count*sizeof*events); }
static void travel_compare(bool hi) {
    TravelObservation o=travel_observe(hi);CHECK(event_count==expected_count);
    for(unsigned i=0;i<event_count;++i){CHECK(events[i].op==expected_events[i].op);CHECK(events[i].a==expected_events[i].a);CHECK(events[i].b==expected_events[i].b);CHECK(float_bits(events[i].x)==float_bits(expected_events[i].x));CHECK(float_bits(events[i].y)==float_bits(expected_events[i].y));CHECK(float_bits(events[i].z)==float_bits(expected_events[i].z));CHECK(float_bits(events[i].w)==float_bits(expected_events[i].w));}
    for(unsigned i=0;i<24;++i){if(o.i[i]!=travel_expected.i[i])fprintf(stderr,"hi%d integer%u actual%d expected%d\n",hi,i,o.i[i],travel_expected.i[i]);CHECK(o.i[i]==travel_expected.i[i]);}
    for(unsigned i=0;i<18;++i){if(float_bits(o.f[i])!=float_bits(travel_expected.f[i]))fprintf(stderr,"hi%d float%u actual%.9g expected%.9g\n",hi,i,o.f[i],travel_expected.f[i]);CHECK(float_bits(o.f[i])==float_bits(travel_expected.f[i]));}
    ++pairs;
}
static void travel_seed(bool generated_path,int motion,int timer,bool retained,int owner) {
    initialize(generated_path);fighter.motion_id=motion;fighter.ground_or_air=(motion==346||motion==347||motion==348||motion>=352)?GA_Air:GA_Ground;
    fighter.self_vel=(Vec3){1.2f,-.8f,.3f};fighter.gr_vel=2.3f;fighter.frame_speed_mul=.7f;fighter.xE4_ground_accel_1=.5f;
    fighter.input.lstick[0]=(Vec2){.75f,.25f};item.owner=owner==0?NULL:owner==1?&source:&foreign;
    fighter.coll_data.ceiling.normal=(Vec3){0,-1,0};fighter.coll_data.left_facing_wall.normal=(Vec3){1,0,0};fighter.coll_data.right_facing_wall.normal=(Vec3){-1,0,0};
    if(motion<=348){if(generated_path){const char*names[]={"din_start_remaining","din_hold_remaining","din_gravity_remaining","din_release_remaining"};for(int k=0;k<4;++k)put_int(fighter_field(names[k]),timer);put_int(fighter_field("din_handle"),retained?(int32_t)item.x1C:-1);}
        else {fighter.mv.zd.specials.x0=timer;fighter.mv.zd.specials.x4=timer;fighter.mv.zd.specials.x8=timer;fighter.mv.zd.specials.xC=timer;fighter.u.zd.x222C=retained?&object:NULL;}}
    else if(generated_path){put_int(fighter_field("teleport_remaining"),timer);put_int(fighter_field("teleport_collision_age"),timer);}
    else {fighter.mv.zd.specialhi.x0=timer;fighter.mv.zd.specialhi.xC=timer;}
}
static HSD_GObjEvent old_phase[12][4]={
    {ftZd_SpecialSStart_Anim,ftZd_SpecialSStart_IASA,ftZd_SpecialSStart_Phys,ftZd_SpecialSStart_Coll},
    {ftZd_SpecialSLoop_Anim,ftZd_SpecialSLoop_IASA,ftZd_SpecialSLoop_Phys,ftZd_SpecialSLoop_Coll},
    {ftZd_SpecialSEnd_Anim,ftZd_SpecialSEnd_IASA,ftZd_SpecialSEnd_Phys,ftZd_SpecialSEnd_Coll},
    {ftZd_SpecialAirSStart_Anim,ftZd_SpecialAirSStart_IASA,ftZd_SpecialAirSStart_Phys,ftZd_SpecialAirSStart_Coll},
    {ftZd_SpecialAirSLoop_Anim,ftZd_SpecialAirSLoop_IASA,ftZd_SpecialAirSLoop_Phys,ftZd_SpecialAirSLoop_Coll},
    {ftZd_SpecialAirSEnd_Anim,ftZd_SpecialAirSEnd_IASA,ftZd_SpecialAirSEnd_Phys,ftZd_SpecialAirSEnd_Coll},
    {ftZd_SpecialHiStart_0_Anim,ftZd_SpecialHiStart_0_IASA,ftZd_SpecialHiStart_0_Phys,ftZd_SpecialHiStart_0_Coll},
    {ftZd_SpecialHiStart_1_Anim,ftZd_SpecialHiStart_1_IASA,ftZd_SpecialHiStart_1_Phys,ftZd_SpecialHiStart_1_Coll},
    {ftZd_SpecialHi_Anim,ftZd_SpecialHi_IASA,ftZd_SpecialHi_Phys,ftZd_SpecialHi_Coll},
    {ftZd_SpecialAirHiStart_0_Anim,ftZd_SpecialAirHiStart_0_IASA,ftZd_SpecialAirHiStart_0_Phys,ftZd_SpecialAirHiStart_0_Coll},
    {ftZd_SpecialAirHiStart_1_Anim,ftZd_SpecialAirHiStart_1_IASA,ftZd_SpecialAirHiStart_1_Phys,ftZd_SpecialAirHiStart_1_Coll},
    {ftZd_SpecialAirHi_Anim,ftZd_SpecialAirHi_IASA,ftZd_SpecialAirHi_Phys,ftZd_SpecialAirHi_Coll}
};
static void phase(int state,int which,bool generated) { if(!generated){old_phase[state-343][which](&source);return;}const MotionState*m=&definition->motion_states[state-341];HSD_GObjEvent fn=which==0?m->anim_cb:which==1?m->input_cb:which==2?m->phys_cb:m->coll_cb;CHECK(fn);fn(&source); }

static void override_float(const char*name,float value) {
    uint32_t bits[sizeof parameter_bits/sizeof*parameter_bits];memcpy(bits,parameter_bits,sizeof bits);bool found=false;
    for(unsigned i=0;i<sizeof bits/sizeof*bits;++i)if(!strcmp(parameter_names[i],name)){bits[i]=float_bits(value);found=true;}
    CHECK(found&&FreakMelee_SetFighterParameters(bits,sizeof bits/sizeof*bits,parameter_schema));
}
static void travel_link_cases(void) {
    const uint32_t ids[]={1,UINT32_C(0x80000000),UINT32_C(0xFEDCBA98),UINT32_C(0xFFFFFFFE)};
    const int32_t large_timers[]={16777217,-16777217,INT32_MAX};
    for(unsigned id=0;id<4;++id)for(unsigned item_id=0;item_id<4;++item_id)for(int state=344;state<=347;state+=3)for(int owner=0;owner<3;++owner)for(int t=0;t<3;++t){
        for(int generated=0;generated<2;++generated){travel_seed(generated,state,large_timers[t],true,owner);fighter.x8_spawnNum=ids[id];item.x1C=ids[item_id];if(generated)put_int(fighter_field("din_handle"),(int32_t)item.x1C);phase(state,0,generated);if(generated)travel_compare(false);else travel_save(false);}
    }
    /* An ordered start/hold/release/end/death sequence uses the bound callbacks,
     * including the existing accepted detach-control signal contract. */
    for(int air=0;air<2;++air)for(int failure=0;failure<2;++failure)for(int transferred=0;transferred<2;++transferred){
        for(int generated=0;generated<2;++generated){travel_seed(generated,air?346:343,4,false,1);create_success=!failure;
            HSD_GObjEvent enter=generated?(air?definition->special_air_s:definition->special_s):(air?ftZd_SpecialAirS_Enter:ftZd_SpecialS_Enter);enter(&source);
            fighter.cmd_vars[0]=1;remaining=false;phase(air?346:343,0,generated);
            if(transferred)item.owner=&foreign;
            for(int frame=0;frame<15;++frame){phase(air?347:344,0,generated);fighter.input.held_buttons[0]=frame<14?512:0;phase(air?347:344,1,generated);}
            /* Cleanup is invoked before the ordinary completion reset. */
            if(fighter.death2_cb)fighter.death2_cb(&source);phase(air?348:345,0,generated);
            if(generated)travel_compare(false);else travel_save(false);
        }
    }
    /* Explicit tuning boundaries, through the module parameter loader. */
    for(int zero=0;zero<2;++zero){float saved=zd_attrs.x34;zd_attrs.x34=zero?0:saved;override_float("din_landing_lag",zd_attrs.x34);
        for(int generated=0;generated<2;++generated){travel_seed(generated,348,4,true,1);remaining=false;phase(348,0,generated);if(generated)travel_compare(false);else travel_save(false);}zd_attrs.x34=saved;
    }
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/4,parameter_schema));
    const Vec3 normals[]={{0,1,0},{.6f,.8f,0},{-.6f,.8f,0},{0,-1,0}};
    for(unsigned n=0;n<4;++n)for(int direction=0;direction<2;++direction)for(int soft=0;soft<2;++soft){for(int generated=0;generated<2;++generated){travel_seed(generated,349,4,true,1);fighter.coll_data.floor.normal=normals[n];fighter.input.lstick[0]=(Vec2){direction?-.5f:.5f,0};remaining=false;soft_floor=soft;phase(349,0,generated);if(generated)travel_compare(true);else travel_save(true);}}
    /* Every wall can terminate independently; common normals also force
     * three ordered ends in a single callback, preserving successive momentum. */
    for(int normal=0;normal<3;++normal)for(int masks=0;masks<8;++masks){for(int generated=0;generated<2;++generated){travel_seed(generated,353,4,true,1);fighter.self_vel=(Vec3){normal==0?-3:normal==1?3:0,normal==2?3:0,0};
        if(normal==2){fighter.coll_data.ceiling.normal=(Vec3){0,-1,0};fighter.coll_data.left_facing_wall.normal=(Vec3){0,-1,0};fighter.coll_data.right_facing_wall.normal=(Vec3){0,-1,0};}
        fighter.coll_data.env_flags=(masks&1?Collide_CeilingMask:0)|(masks&2?Collide_LeftWallMask:0)|(masks&4?Collide_RightWallMask:0);phase(353,3,generated);
        if(normal==2&&masks==7){unsigned transitions=0;for(unsigned i=0;i<event_count;++i)transitions+=events[i].op==Motion;CHECK(transitions==3);}
        if(generated)travel_compare(true);else travel_save(true);}}
    /* Retired/foreign/unregistered references are safe authored cases. The
     * old valid-pointer body is intentionally not invoked on poisoned data. */
    for(int invalid=0;invalid<6;++invalid){travel_seed(true,344,0,true,1);
        switch(invalid){case 0:heads[HSD_GOBJ_PLINK_ITEM]=NULL;object.user_data=(void*)1;break;case 1:item.x1C++;break;case 2:item.kind=It_Kind_Zelda_DinFire_Explode;break;case 3:item.xB8_itemLogicTable=NULL;break;case 4:item.native_removal_in_progress=true;break;case 5:item.owner=&foreign;source.next=NULL;foreign.user_data=(void*)1;break;}
        phase(344,0,true);CHECK(get_int(fighter_field("din_handle"))==-1&&fighter.motion_id==344);++safety;
    }
}
static void travel_cases(void) {
    const int timers[]={-1,0,1,2,4};const float speed[]={-3,-.03f,0,.04f,3};
    /* Four complete entry callbacks; both directions and pre-entry velocities. */
    for(int entry=0;entry<4;++entry)for(int direction=0;direction<2;++direction)for(int v=0;v<5;++v){
        for(int generated=0;generated<2;++generated){travel_seed(generated,entry<2?343:349,4,true,1);fighter.facing_dir=direction?1:-1;fighter.self_vel.x=speed[v];fighter.cmd_vars[0]=3;
            if(generated){HSD_GObjEvent f=entry==0?definition->special_s:entry==1?definition->special_air_s:entry==2?definition->special_hi:definition->special_air_hi;f(&source);}
            else {HSD_GObjEvent f=entry==0?ftZd_SpecialS_Enter:entry==1?ftZd_SpecialAirS_Enter:entry==2?ftZd_SpecialHi_Enter:ftZd_SpecialAirHi_Enter;f(&source);}
            if(generated)travel_compare(entry>=2);else travel_save(entry>=2);
        }
    }
    /* Side-B spawn/hold/end: failure cue, no same-tick end on owner change,
     * negative timers and delayed release retained. */
    for(int state=343;state<=348;++state)for(int command=0;command<3;++command)for(int t=0;t<5;++t)for(int retained=0;retained<2;++retained)for(int owner=0;owner<3;++owner)for(int frames=0;frames<2;++frames)for(int allocation=0;allocation<2;++allocation){
        for(int generated=0;generated<2;++generated){travel_seed(generated,state,timers[t],retained,owner);remaining=frames;create_success=allocation;fighter.cmd_vars[0]=command;phase(state,0,generated);if(generated)travel_compare(false);else travel_save(false);}
    }
    for(int state=343;state<=348;++state)for(int t=0;t<5;++t)for(int held=0;held<2;++held){
        for(int generated=0;generated<2;++generated){travel_seed(generated,state,timers[t],true,1);fighter.input.held_buttons[0]=held?512:0;phase(state,1,generated);if(generated)travel_compare(false);else travel_save(false);}
    }
    /* All twelve physics callbacks exercise acceleration/fall/clamp boundaries. */
    for(int state=343;state<=354;++state)for(int t=0;t<5;++t)for(int v=0;v<5;++v)for(int command=0;command<2;++command){
        for(int generated=0;generated<2;++generated){travel_seed(generated,state,timers[t],true,1);fighter.self_vel.x=speed[v];fighter.self_vel.y=v&1?-3:.2f;fighter.cmd_vars[0]=command;phase(state,2,generated);if(generated)travel_compare(state>=349);else travel_save(state>=349);}
    }
    const Vec2 sticks[]={{0,0},{.001f,0},{-.001f,0},{.49999997f,0},{.5f,0},{-.5f,0},{0,.5f},{0,-.5f},{.75f,.25f},{-1,-1},{1,1},{.3f,.4f},{-.3f,-.4f}};
    for(int state=349;state<=354;++state)for(unsigned stick=0;stick<sizeof sticks/sizeof*sticks;++stick)for(int facing=0;facing<2;++facing)for(int soft=0;soft<2;++soft)for(int frames=0;frames<2;++frames)for(int t=0;t<5;++t){
        for(int generated=0;generated<2;++generated){travel_seed(generated,state,timers[t],true,1);fighter.input.lstick[0]=sticks[stick];fighter.facing_dir=facing?1:-1;soft_floor=soft;remaining=frames;phase(state,0,generated);if(generated)travel_compare(true);else travel_save(true);}
    }
    /* Independent ceiling+wall probes, collision-age before platform gate,
     * wall end after ground loss, and startup/recovery ledge routing. */
    for(int state=343;state<=354;++state)for(int land=0;land<2;++land)for(int cliff=0;cliff<2;++cliff)for(int soft=0;soft<2;++soft)for(int masks=0;masks<8;++masks)for(int t=0;t<5;++t){
        for(int generated=0;generated<2;++generated){travel_seed(generated,state,timers[t],true,1);ground_result=land;air_result=land;cliff_result=cliff;soft_floor=soft;
            fighter.coll_data.env_flags=(masks&1?Collide_CeilingMask:0)|(masks&2?Collide_LeftWallMask:0)|(masks&4?Collide_RightWallMask:0);
            fighter.self_vel=(Vec3){-2,3,0};phase(state,3,generated);if(generated)travel_compare(state>=349);else travel_save(state>=349);}
    }
    for(int state=349;state<=354;++state)for(int generated=0;generated<2;++generated){travel_seed(generated,state,4,true,1);phase(state,1,generated);if(generated)travel_compare(true);else travel_save(true);}
    /* Actual bound accessory body sampled for already-active and fresh effects. */
    for(int end=0;end<2;++end)for(int air=0;air<2;++air)for(int active=0;active<2;++active){for(int generated=0;generated<2;++generated){travel_seed(generated,air?352:349,4,true,1);fighter.x2219_b0=active;fighter.accessory4_cb=generated?(end?script_fn_teleport_end_effect:script_fn_teleport_start_effect):(end?ftZd_SpecialHi_8013979C:ftZd_SpecialHi_801396AC);fighter.accessory4_cb(&source);if(generated)travel_compare(true);else travel_save(true);}}
    /* Separate authored lifetime safety, not old-C stale-pointer equivalence. */
    for(int entry=0;entry<4;++entry)for(int change=1;change<=2;++change){travel_seed(true,entry<2?343:349,4,true,1);invalidation=change;HSD_GObjEvent f=entry==0?definition->special_s:entry==1?definition->special_air_s:entry==2?definition->special_hi:definition->special_air_hi;f(&source);CHECK(event_count==1&&events[0].op==Motion);++safety;}
    travel_link_cases();
    /* New phases own precisely 343..354; the transform slots remain untouched. */
    for(unsigned m=2;m<14;++m){CHECK(definition->motion_states[m].anim_cb&&definition->motion_states[m].input_cb&&definition->motion_states[m].phys_cb&&definition->motion_states[m].coll_cb);}
    for(unsigned m=14;m<18;++m){CHECK(!definition->motion_states[m].anim_cb&&!definition->motion_states[m].input_cb&&!definition->motion_states[m].phys_cb&&!definition->motion_states[m].coll_cb);}
}
