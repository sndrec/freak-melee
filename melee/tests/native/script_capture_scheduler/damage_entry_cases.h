static void damage_entry_cases(void)
{
    reset();Fighter_GObj* actor=create(true);Fighter* fp=actor->user_data;
    NativeScriptRuntimeContext context={.source=actor,.source_class=NativeCallbackSource_Fighter,
        .generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptDiagnostic diagnostic;
    ftCommonData common={.x154=0.4f,.x158=10,.x15C=20,.x160=30,.x100=0.03f,
        .xFC=10,.x144_radians=0.7853982f,.x148=44,.x14C=10,.x150=20,
        .x190=0.75f,.x23C=1000,.x208=2000,.x20C=2000,.x5E8=2000,
        .x12C=10,.x568=0.5f,.x56C=1,.x570=2,.x574=3,.x578=4,
        .x57C=3,.x580=4,.x584=5,.x588=6,.ledge_cooldown=30};
    ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    const float knockback[]={0,10,30,60,90};const unsigned angles[]={0,45,90,270,361};
    const float facings[]={0,1,-1};
    damage_motion_case=true;
    for(unsigned target=0;target<2;++target)for(unsigned air=0;air<2;++air)for(unsigned index=0;index<5;++index)
        for(unsigned angle=0;angle<5;++angle)for(unsigned face=0;face<3;++face){
            fp->ground_or_air=air?GA_Air:GA_Ground;fp->motion_id=ftCo_MS_Fall;
            fp->dmg.kb_applied=knockback[index];fp->dmg.facing_dir_1=-1;
            fp->dmg.x1848_kb_angle=angles[angle];fp->dmg.x184c_damaged_hurtbox=angle%3;
            fp->dmg.x1860_element=HitElement_Normal;fp->dmg.x18ac_time_since_hit=0;
            fp->coll_data.floor.normal=(Vec3){0,1,0};fp->self_vel=(Vec3){1,2,3};
            fp->x8c_kb_vel=(Vec3){4,5,6};fp->xF0_ground_kb_vel=2;fp->gr_vel=7;
            int requested_motion=target?-1:345;
            Fighter before=*fp;trace_count=0;reference_damage_entry(actor,requested_motion,facings[face]);
            Fighter expected=*fp;unsigned expected_trace[32],expected_count=trace_count;
            memcpy(expected_trace,trace,sizeof(trace));*fp=before;trace_count=0;
            NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=requested_motion},
                {.type=NativeScriptType_F32,.as.f32=facings[face]}},out;
            CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_DamageEnter,NULL,0,args,2,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(*fp)));
            CHECK(trace_count==expected_count&&!memcmp(trace,expected_trace,trace_count*sizeof(*trace)));++cases;
        }
    for(unsigned element=0;element<8;++element)for(unsigned sandbag=0;sandbag<2;++sandbag){
        fp->ground_or_air=GA_Air;fp->motion_id=ftCo_MS_Fall;fp->is_sandbag=sandbag;
        fp->dmg.x1860_element=element;fp->x221D_b7=1;fp->dmg.kb_applied=10;
        fp->dmg.x1848_kb_angle=45;fp->dmg.x184c_damaged_hurtbox=0;
        Fighter before=*fp;trace_count=0;reference_damage_default(actor,0);Fighter expected=*fp;
        *fp=before;trace_count=0;NativeScriptValue facing={.type=NativeScriptType_F32,.as.f32=0},out;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_DamageDefaultReaction,NULL,0,&facing,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(*fp)));++cases;
    }
    fp->is_sandbag=false;fp->dmg.x1860_element=HitElement_Normal;
    fp->ground_or_air=GA_Air;fp->dmg.x1848_kb_angle=45;fp->dmg.x184c_damaged_hurtbox=0;
    NativeScriptValue motion={.type=NativeScriptType_I32,.as.i64=345};
    presentation_state(actor,"items",&motion);trace_count=0;CHECK(helper(actor,"DamageEnter")&&fp->motion_id==345);
    trace_count=0;CHECK(helper(actor,"DamageDefault")&&fp->motion_id!=345);cases+=2;
    const int invalid_motions[]={-2,347,65536};
    for(unsigned bad=0;bad<11;++bad){
        NativeScriptRuntimeContext invalid=context;
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=345},{.type=NativeScriptType_F32,.as.f32=0}};
        if(bad<3)args[0].as.i64=invalid_motions[bad];
        if(bad==3)args[0].type=NativeScriptType_U32;if(bad==4)args[1].type=NativeScriptType_I32;
        if(bad==5)args[1].as.f32=INFINITY;if(bad==6)invalid.generation++;
        if(bad==7)invalid.scene_epoch++;if(bad==8)invalid.source=NULL;
        if(bad==9)invalid.source_class=NativeCallbackSource_Item;if(bad==10)p_ftCommonData=NULL;
        Fighter before=*fp;trace_count=0;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingCombat_Invoke(&invalid,NativeScriptBinding_DamageEnter,args,2,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&!trace_count&&!memcmp(fp,&before,sizeof(*fp)));
        p_ftCommonData=&common;++cases;
    }
    HSD_GObjFree(actor);
    // Both native and binding continuations must stop at each authored boundary.
    const unsigned boundaries[]={16,5,7};
    for(unsigned mode=0;mode<2;++mode)for(unsigned i=0;i<3;++i){
        reset();actor=create(true);fp=actor->user_data;damage_motion_case=true;
        damage_retire_mode=mode;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(fp->kind);
        uint64_t program_identity=program->identity;
        fp->ground_or_air=GA_Air;fp->dmg.kb_applied=10;fp->dmg.x1848_kb_angle=45;
        fp->dmg.facing_dir_1=1;retirement_holder=actor;retire_step=boundaries[i];
        context.source=actor;context.generation=fp->x8_spawnNum;
        context.scene_epoch=nativeGObjPool.lifetime;
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=345},{.type=NativeScriptType_F32,.as.f32=0}};
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingCombat_Invoke(&context,NativeScriptBinding_DamageEnter,args,2,&out,&diagnostic));
        if(out.type!=NativeScriptType_U64||out.as.u64!=UINT64_MAX||!trace_count||trace[trace_count-1]!=boundaries[i])
            fprintf(stderr,"damage boundary mode=%u step=%u type=%u count=%u last=%u: %s\n",mode,boundaries[i],out.type,trace_count,trace_count?trace[trace_count-1]:0,diagnostic.message);
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&trace_count&&trace[trace_count-1]==boundaries[i]);
        program->identity=program_identity;HSD_GObjFree(actor);++cases;
    }
    reset();actor=create(true);fp=actor->user_data;damage_retire_mode=0;
    fp->ground_or_air=GA_Air;fp->dmg.kb_applied=60;fp->dmg.x1848_kb_angle=45;
    fp->dmg.x1860_element=HitElement_Ice;fp->dmg.facing_dir_1=1;
    motion.as.i64=901;presentation_state(actor,"items",&motion);
    presentation_retire_mode=7;presentation_retire_at=1;
    context.source=actor;context.generation=fp->x8_spawnNum;context.scene_epoch=nativeGObjPool.lifetime;
    NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=345},{.type=NativeScriptType_F32,.as.f32=0}};
    NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(!nativeScriptBindingCombat_Invoke(&context,NativeScriptBinding_DamageEnter,args,2,&out,&diagnostic));
    CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX&&presentation_callback_writes==1);
    CHECK(nativeScriptFighter_Status(actor)==NativeScriptVM_Error);
    for(unsigned i=0;i<trace_count;++i)CHECK(trace[i]!=21);
    HSD_GObjFree(actor);++cases;
    damage_motion_case=false;damage_retire_mode=0;p_ftCommonData=previous;reset();
}
