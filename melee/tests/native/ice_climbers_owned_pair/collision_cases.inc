/* Each original body is complete; stage probes and transitions are controlled. */
static void owned_census(void)
{
    for(unsigned r=0;r<2;++r){
        CHECK(definitions[r]->motion_state_first==341);CHECK(definitions[r]->motion_state_count==26);
        unsigned count=0;
        for(int id=341;id<=366;++id)for(unsigned which=0;which<4;++which){
            if(id==363||id==366)CHECK(!phase(r,id,which));
            else {CHECK(phase(r,id,which));++count;}
        }
        CHECK(count==96);CHECK(definitions[r]->special_n);CHECK(definitions[r]->special_air_n);
        CHECK(definitions[r]->special_lw);CHECK(definitions[r]->special_air_lw);
        CHECK((definitions[r]->special_s!=NULL)==(r==0));CHECK((definitions[r]->special_air_s!=NULL)==(r==0));
        CHECK((definitions[r]->special_hi!=NULL)==(r==0));CHECK((definitions[r]->special_air_hi!=NULL)==(r==0));
        CHECK(definitions[r]->on_load);CHECK(definitions[r]->on_death);CHECK(definitions[r]->on_load_special_attrs);
        CHECK(definitions[r]->on_item_pickup);CHECK(definitions[r]->on_item_drop);
        CHECK(definitions[r]->on_item_pickup_ext);CHECK(definitions[r]->on_item_drop_ext);
        CHECK(definitions[r]->on_item_visible);CHECK(definitions[r]->on_item_invisible);
        CHECK(definitions[r]->on_knockback_enter);CHECK(definitions[r]->on_knockback_exit);
        CHECK(definitions[r]->article_logic_count==3);
    }
}
static void collision_cases(void)
{
    HSD_GObjEvent originals[]={ftPp_SpecialS1_Coll,ftPp_SpecialS2_Coll,ftPp_SpecialAirS1_Coll,ftPp_SpecialAirS2_Coll};
    const float speeds[]={-8,-2,-1,-.03125f,0,.03125f,1,2,8};
    const unsigned walls[]={0,0x3f,0xfc0,0xfff,0x6000,0x6fff};
    const Vec3 normals[]={{0,1,0},{.6f,.8f,0},{-.8f,.6f,0},{1,0,0}};
    for(unsigned r=0;r<2;++r)for(unsigned id=0;id<4;++id)for(unsigned s=0;s<9;++s)
    for(unsigned w=0;w<6;++w)for(unsigned bits=0;bits<16;++bits){
        for(unsigned mode=0;mode<2;++mode){
            initialize(mode,343+id,343+id);Fighter*f=&fighters[r];
            f->ground_or_air=id>=2;f->gr_vel=f->self_vel.x=speeds[s];
            f->self_vel.y=bits&8?3.25f:-2.25f;f->facing_dir=bits&2?1:-1;
            f->cmd_vars[3]=bits&4?3:0;f->coll_data.floor.normal=normals[bits/4];
            f->coll_data.env_flags=walls[w];grounded_probe=landed_probe=(bits&1)!=0;
            if(mode){phase(r,343+id,3)(&objects[r]);compare();
                CHECK(fighters[r].native_module_state[field(r,"linked_squall_on_floor")]==expected[r].mv.pp.specials.xC);
                same_float(joints[r][0].rotate.x,expected_root_x);
            }else{originals[id](&objects[r]);expected_root_x=joints[r][0].rotate.x;save();}
        }
    }
}

/* Authored safety, separate from healthy retail comparisons. */
static void collision_safety(void)
{
    for(unsigned r=0;r<2;++r)for(unsigned air=0;air<2;++air)for(unsigned recycle=0;recycle<2;++recycle){
        initialize(true,air?345:343,air?345:343);
        fighters[r].ground_or_air=air;grounded_probe=false;landed_probe=true;
        invalidation=1+10*recycle;invalidation_role=r;
        phase(r,air?345:343,3)(&objects[r]);++safety;
        CHECK(trace_count==3);CHECK(trace[2].op==Change);
        CHECK(fighters[r].native_module_state[field(r,"linked_squall_on_floor")]==0);
        same_float(joints[r][0].rotate.x,.125f+r);
    }
}
