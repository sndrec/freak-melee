static void cpu_floor_seed(unsigned mask,int stage,int joint)
{
    terrain_seed();stage_info.grkind=stage;terrain_joint_id=joint;
    for(unsigned n=0;n<4;++n)terrain_intersection[n]=(mask>>n)&1;
    terrain_floor_calls=0;
}

static void cpu_floor_cases(HSD_GObj* object)
{
    // The authored predicate composes two existing generic operations. Compare
    // it with the pinned native CPU helper, including its stage exclusions.
    const int stages[]={Gr_Kind_Battle,Gr_Kind_BigBlue,Gr_Kind_Inishie1,Gr_Kind_Corneria,Gr_Kind_Venom};
    const int joints[]={0,3,6,67};
    for(unsigned stage=0;stage<5;++stage)for(unsigned joint=0;joint<4;++joint)
      for(unsigned mask=0;mask<16;++mask)for(unsigned reverse=0;reverse<2;++reverse){
        float y=reverse?-10:10;Vec3 point,normal;int line;u32 flags;
        cpu_floor_seed(mask,stages[stage],joints[joint]);
        bool expected=original_cpu_floor_segment(&point,&line,&flags,&normal,-1,-1,-1,0,y,0,-y,0);
        unsigned expected_calls=terrain_floor_calls;float expected_bounds[4];memcpy(expected_bounds,bounds,sizeof(bounds));
        cpu_floor_seed(mask,stages[stage],joints[joint]);
        NativeScriptValue input={.type=NativeScriptType_F32,.as.f32=y};
        terrain_field(object,"floor_y",&input);NativeScriptValue out=sentinel();
        CHECK(helper(object,terrain_function("CpuFloor"),&out)&&out.type==NativeScriptType_Bool&&out.as.u64==expected);
        CHECK(terrain_floor_calls==expected_calls&&!memcmp(bounds,expected_bounds,sizeof(bounds)));
        ++pairs;++cases;
      }
    terrain_seed();
}
