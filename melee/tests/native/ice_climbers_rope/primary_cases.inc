/* New primary phases; accepted paired inputs stay unchanged. */
static void primary_state_compare(void)
{
    CHECK(get_int(0,"rope_counter")==expected[0].mv.pp.unk_80123954.x0);
    CHECK(get_int(0,"rope_marker")==expected[0].u.pp.x223C);
    same_float(get_float(0,"linked_rope_endpoint_x"),expected[0].u.pp.x2240.x);
    same_float(get_float(0,"linked_rope_endpoint_y"),expected[0].u.pp.x2240.y);
    same_float(get_float(0,"linked_rope_endpoint_z"),expected[0].u.pp.x2240.z);
}
static void primary_entry_cases(void)
{
    for(unsigned air=0;air<2;++air)for(unsigned bits=0;bits<16;++bits){
        for(unsigned mode=0;mode<2;++mode){initialize(mode,14,361);fighters[0].ground_or_air=air;
            fighters[0].gr_vel=bits&1?-2.75f:1.625f;fighters[0].self_vel.x=bits&2?-3.5f:2.75f;
            fighters[0].self_vel.y=bits&4?-1.5f:4.25f;fighters[0].cmd_vars[0]=bits&8;
            if(mode){(air?definitions[0]->special_air_hi:definitions[0]->special_hi)(&objects[0]);compare();primary_state_compare();}
            else{(air?ftPp_SpecialAirHi_Enter:ftPp_SpecialHi_Enter)(&objects[0]);save();}
        }
    }
}
static void primary_phase_cases(void)
{
    const int ids[]={347,352,348,353,349,354};
    HSD_GObjEvent originals[6][4]={
      {ftPp_SpecialHiStart_0_Anim,ftPp_SpecialHiStart_0_IASA,ftPp_SpecialHiStart_0_Phys,ftPp_SpecialHiStart_0_Coll},
      {ftPp_SpecialAirHiStart_0_Anim,ftPp_SpecialAirHiStart_0_IASA,ftPp_SpecialAirHiStart_0_Phys,ftPp_SpecialAirHiStart_0_Coll},
      {ftPp_SpecialHiThrow_0_Anim,ftPp_SpecialHiThrow_0_IASA,ftPp_SpecialHiThrow_0_Phys,ftPp_SpecialHiThrow_0_Coll},
      {ftPp_SpecialAirHiThrow_0_Anim,ftPp_SpecialAirHiThrow_0_IASA,ftPp_SpecialAirHiThrow_0_Phys,ftPp_SpecialAirHiThrow_0_Coll},
      {ftPp_SpecialHiThrow2_Anim,ftPp_SpecialHiThrow2_IASA,ftPp_SpecialHiThrow2_Phys,ftPp_SpecialHiThrow2_Coll},
      {ftPp_SpecialAirHiThrow2_Anim,ftPp_SpecialAirHiThrow2_IASA,ftPp_SpecialAirHiThrow2_Phys,ftPp_SpecialAirHiThrow2_Coll}};
    for(unsigned phase_index=0;phase_index<6;++phase_index)for(unsigned which=0;which<4;++which)
    for(unsigned bits=0;bits<256;++bits){
        for(unsigned mode=0;mode<2;++mode){initialize(mode,ids[phase_index],bits&8?362:361);
            fighters[0].ground_or_air=phase_index%2;remaining[0]=(bits&1)!=0;
            fighters[0].cmd_vars[0]=fighters[0].cmd_vars[1]=fighters[0].cmd_vars[2]=(bits&2)!=0;
            fighters[0].input.lstick[0].x=bits&4?-.875f:.625f;
            fighters[0].self_vel.y=bits&16?-.25f:.75f;grounded_probe=(bits&32)!=0;
            landed_probe=(bits&64)!=0;cliff_probe=(bits&16)!=0;
            fighters[0].coll_data.env_flags=bits&8?0x3f:bits&4?0xfc0:0x6000;
            if(bits&128){present[1]=false;registry();}
            if(mode){phase(0,ids[phase_index],which)(&objects[0]);compare();primary_state_compare();}
            else{originals[phase_index][which](&objects[0]);save();}
        }
    }
}
static void primary_safety_cases(void)
{
    for(unsigned recycle=0;recycle<2;++recycle){
        initialize(true,14,361);invalidation=1+10*recycle;invalidation_role=0;
        definitions[0]->special_hi(&objects[0]);++safety;
        CHECK(trace_count==1);CHECK(get_int(0,"rope_counter")==3);
    }
}

static void primary_launch_cases(void)
{
    const float offsets[]={0,.03125f,.125f,1.375f,2.5f,8.875f,32.125f,127.75f};
    for(unsigned air=0;air<2;++air)for(unsigned facing=0;facing<2;++facing)
    for(unsigned x=0;x<8;++x)for(unsigned y=0;y<8;++y){
        for(unsigned mode=0;mode<2;++mode){initialize(mode,348,362);fighters[0].ground_or_air=air;
            fighters[1].facing_dir=facing?1:-1;fighters[1].cur_pos=fighters[0].cur_pos;
            fighters[1].cur_pos.x+=offsets[x];fighters[1].cur_pos.y+=offsets[y];
            if(mode){phase(0,348,0)(&objects[0]);compare();primary_state_compare();}
            else{ftPp_SpecialHiThrow_0_Anim(&objects[0]);save();}
        }
    }
}
