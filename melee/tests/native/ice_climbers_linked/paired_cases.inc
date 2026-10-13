/* New paired cases; accepted solo case files are not modified. */
static void recruitment_cases(void)
{
    const float distances[]={0,.0625f,1,4,64};
    for(unsigned category=0;category<16;++category)for(unsigned air=0;air<2;++air)
    for(unsigned secondary_air=0;secondary_air<2;++secondary_air)for(unsigned distance=0;distance<5;++distance){
        bool expected_accept=false;
        for(unsigned mode=0;mode<2;++mode){
            initialize(mode,344,359);fighters[1].x2070.x2071_b0_3=category;fighters[1].ground_or_air=secondary_air;
            fighters[1].cur_pos=fighters[0].cur_pos;fighters[1].cur_pos.x+=distances[distance];
            if(mode){bool accepted=target_bool((uint32_t)fighters[1].x8_spawnNum,
                name_hash(air?"linked_join_squall_air":"linked_join_squall_ground"));CHECK(accepted==expected_accept);compare();
                CHECK(get_int(1,"linked_follow_delay")==expected[1].mv.pp.unk_80123954.x0);
            }else{expected_accept=!ftNn_Init_80123954(&objects[1],air?GA_Air:GA_Ground);save();}
        }
    }
}

static void secondary_phase_cases(void)
{
    HSD_GObjEvent originals[2][4]={
        {ftPp_SpecialS_0_Anim,ftPp_SpecialS_0_IASA,ftPp_SpecialS_0_Phys,ftPp_SpecialS_0_Coll},
        {ftPp_SpecialS_1_Anim,ftPp_SpecialS_1_IASA,ftPp_SpecialS_1_Phys,ftPp_SpecialS_1_Coll}};
    for(unsigned air=0;air<2;++air)for(unsigned which=0;which<4;++which)for(unsigned bits=0;bits<64;++bits){
        for(unsigned mode=0;mode<2;++mode){
            initialize(mode,bits&1?344:346,359+air);if(bits&2)fighters[0].motion_id=14;
            fighters[0].ground_or_air=bits&4?GA_Air:GA_Ground;fighters[1].ground_or_air=air;
            fighters[0].cmd_vars[1]=bits&8?1:0;grounded_probe=bits&16;landed_probe=bits&32;
            fighters[0].x1A5C=&objects[1];fighters[1].x1A5C=&objects[0];
            fighters[0].native_hitlag_link_generation=(uint32_t)fighters[1].x8_spawnNum;
            fighters[1].native_hitlag_link_generation=(uint32_t)fighters[0].x8_spawnNum;
            fighters[0].x2219_b7=bits&4;fighters[1].x2219_b7=bits&8;
            if(mode)set_int(1,"linked_follow_delay",4);else fighters[1].mv.pp.unk_80123954.x0=4;
            if(mode){phase(1,359+air,which)(&objects[1]);compare();CHECK(get_int(1,"linked_follow_delay")==expected[1].mv.pp.unk_80123954.x0);}
            else{originals[air][which](&objects[1]);save();}
        }
    }
    HSD_GObjEvent belay[4][4]={
        {ftPp_SpecialHi_0_Anim,ftPp_SpecialHi_0_IASA,ftPp_SpecialHi_0_Phys,ftPp_SpecialHi_0_Coll},
        {ftPp_SpecialHi_1_Anim,ftPp_SpecialHi_1_IASA,ftPp_SpecialHi_1_Phys,ftPp_SpecialHi_1_Coll},
        {ftPp_SpecialHi_3_Anim,ftPp_SpecialHi_3_IASA,ftPp_SpecialHi_3_Phys,ftPp_SpecialHi_3_Coll},
        {ftPp_SpecialHi_4_Anim,ftPp_SpecialHi_4_IASA,ftPp_SpecialHi_4_Phys,ftPp_SpecialHi_4_Coll}};
    const int motions[]={361,362,364,365};
    for(unsigned state=0;state<4;++state)for(unsigned which=0;which<4;++which)for(unsigned bits=0;bits<128;++bits){
        for(unsigned mode=0;mode<2;++mode){
            initialize(mode,bits&1?347:352,motions[state]);if(bits&2)fighters[0].motion_id=353;
            fighters[0].x2219_b5=bits&4;fighters[0].frame_speed_mul=bits&8?fighters[1].frame_speed_mul:.625f;
            fighters[1].input.lstick[0].x=bits&16?1:0;fighters[1].self_vel.y=bits&32?.5f:-.5f;
            remaining[1]=bits&64;grounded_probe=bits&16;landed_probe=bits&32;
            fighters[1].coll_data.env_flags=bits&4?Collide_LeftWallMask:bits&8?Collide_RightWallMask:Collide_CeilingMask;
            fighters[1].self_vel.x=bits&16?2:-2;
            if(mode){phase(1,motions[state],which)(&objects[1]);compare();}
            else{belay[state][which](&objects[1]);save();}
        }
    }
}

static void belay_offer_cases(void)
{
    for(unsigned category=0;category<16;++category)for(unsigned bits=0;bits<8;++bits){
        bool accepted=false;
        for(unsigned mode=0;mode<2;++mode){
            initialize(mode,347,14);fighters[1].x2070.x2071_b0_3=category;
            fighters[1].x221F_b3=bits&1;fighters[1].x2219_b5=bits&2;fighters[1].ground_or_air=bits&4?GA_Air:GA_Ground;
            if(mode){CHECK(target_bool((uint32_t)fighters[1].x8_spawnNum,name_hash("linked_belay_offer"))==accepted);compare();}
            else{accepted=ftNn_Init_8012300C(&objects[1]);save();}
        }
    }
    for(int motion=360;motion<=367;++motion)for(unsigned inactive=0;inactive<2;++inactive){
        initialize(false,347,motion);fighters[1].x221F_b3=inactive;bool expected_result=ftNn_Init_8012309C(&objects[1]);
        initialize(true,347,motion);fighters[1].x221F_b3=inactive;
        CHECK(target_bool((uint32_t)fighters[1].x8_spawnNum,name_hash("linked_belay_can_throw"))==expected_result);++pairs;
    }
}

static void primary_squall_cases(void)
{
    for(unsigned air=0;air<2;++air)for(unsigned category=0;category<16;++category)
    for(unsigned distance=0;distance<3;++distance)for(unsigned secondary_air=0;secondary_air<2;++secondary_air){
        for(unsigned mode=0;mode<2;++mode){
            initialize(mode,14,14);fighters[0].ground_or_air=air?GA_Air:GA_Ground;
            fighters[1].ground_or_air=secondary_air?GA_Air:GA_Ground;fighters[1].x2070.x2071_b0_3=category;
            fighters[1].cur_pos=fighters[0].cur_pos;fighters[1].cur_pos.x+=distance*16;
            if(mode){(air?definitions[0]->special_air_s:definitions[0]->special_s)(&objects[0]);compare();
                CHECK(get_int(0,"squall_pending_pumps")==expected[0].mv.pp.specials.x10);
                CHECK(get_int(0,"squall_pump_timer")==expected[0].mv.pp.specials.x14);
                CHECK(get_int(0,"squall_elapsed")==expected[0].mv.pp.specials.x18);
                same_float(get_float(0,"squall_input_accel"),expected[0].mv.pp.specials.x1C);
            }else{(air?ftPp_SpecialAirS_Enter:ftPp_SpecialS_Enter)(&objects[0]);save();}
        }
    }
    HSD_GObjEvent animations[]={ftPp_SpecialS1_Anim,ftPp_SpecialS2_Anim,ftPp_SpecialAirS1_Anim,ftPp_SpecialAirS2_Anim};
    for(unsigned state=0;state<4;++state)for(unsigned bits=0;bits<16;++bits){
        for(unsigned mode=0;mode<2;++mode){
            initialize(mode,343+state,bits&1?359:14);remaining[0]=bits&2;
            fighters[0].x1A5C=&objects[1];fighters[1].x1A5C=&objects[0];
            fighters[0].native_hitlag_link_generation=(uint32_t)fighters[1].x8_spawnNum;
            fighters[1].native_hitlag_link_generation=(uint32_t)fighters[0].x8_spawnNum;
            fighters[0].x2219_b7=bits&4;fighters[1].x2219_b7=bits&8;
            if(mode){phase(0,343+state,0)(&objects[0]);compare();}else{animations[state](&objects[0]);save();}
        }
    }
    const float velocities[]={-4,-.25f,0,.25f,4};
    for(unsigned paired=0;paired<2;++paired)for(unsigned velocity=0;velocity<5;++velocity)
    for(unsigned accel=0;accel<3;++accel)for(unsigned bits=0;bits<32;++bits){
        for(unsigned mode=0;mode<2;++mode){
            initialize(mode,343+paired,359);fighters[0].gr_vel=velocities[velocity];fighters[0].cmd_vars[0]=bits&1;
            fighters[0].cmd_vars[2]=bits&2;fighters[0].input.pressed_buttons=bits&4?HSD_PAD_B:0;
            float acceleration=(int)accel-1;int timer=attributes[0].x68-1+(bits>>3);
            if(mode){set_float(0,"squall_input_accel",acceleration);set_int(0,"squall_pending_pumps",bits&8?1:0);
                set_int(0,"squall_pump_timer",timer);set_int(0,"squall_elapsed",7);
                phase(0,343+paired,2)(&objects[0]);compare();
                CHECK(get_int(0,"squall_pending_pumps")==expected[0].mv.pp.specials.x10);
                CHECK(get_int(0,"squall_pump_timer")==expected[0].mv.pp.specials.x14);
                CHECK(get_int(0,"squall_elapsed")==expected[0].mv.pp.specials.x18);
            }else{fighters[0].mv.pp.specials.x1C=acceleration;fighters[0].mv.pp.specials.x10=bits&8?1:0;
                fighters[0].mv.pp.specials.x14=timer;fighters[0].mv.pp.specials.x18=7;
                (paired?ftPp_SpecialS2_Phys:ftPp_SpecialS1_Phys)(&objects[0]);save();}
        }
    }
}

static void safety_cases(void)
{
    /* Missing source has no defined stock paired dereference contract. These
     * safety cases are explicitly authored/registry checks, not parity pairs. */
    for(unsigned recycle=0;recycle<2;++recycle){
        initialize(true,347,361);Vec3 before=fighters[1].cur_pos;
        invalidation=3+10*recycle;invalidation_role=0;
        phase(1,361,2)(&objects[1]);++safety;
        CHECK(!memcmp(&before,&fighters[1].cur_pos,sizeof(before)));CHECK(trace_count==2);
        CHECK(trace[0].op==RotationY&&trace[1].op==WorldPosition);
    }
    for(unsigned r=0;r<2;++r){
        initialize(true,344,359);present[1-r]=false;registry();
        if(r)phase(1,359,2)(&objects[1]);else phase(0,344,0)(&objects[0]);++safety;
        CHECK(r?trace_count==0:fighters[0].motion_id==14);
    }
    for(unsigned recycle=0;recycle<2;++recycle){
        initialize(true,344,359);fighters[0].x1A5C=&objects[1];
        fighters[0].native_hitlag_link_generation=(uint32_t)fighters[1].x8_spawnNum;
        if(recycle)++fighters[1].x8_spawnNum;else{present[1]=false;objects[1].user_data=NULL;registry();}
        CHECK(nativeFighterRelations_LinkedId(&objects[0])==0);++safety;
        target_void((uint32_t)fighters[0].x8_spawnNum,name_hash("linked_release_and_unlink"));
        CHECK(fighters[0].x1A5C==NULL);CHECK(fighters[1].x221A_b1==0);
    }
}
