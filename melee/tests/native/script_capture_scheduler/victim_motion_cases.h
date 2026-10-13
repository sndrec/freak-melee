// Real common entry bodies and interpreter; controlled motion/pose leaves.
static void reciprocal_pair(Fighter_GObj* owner,Fighter_GObj* victim)
{
    link_pair(owner,victim);Fighter* h=owner->user_data;Fighter* v=victim->user_data;
    v->victim_gobj=v->x1A5C=owner;v->native_hitlag_link_generation=h->x8_spawnNum;
}

static void victim_motion_cases(void)
{
    ftCommonData common={0};common.x4A0=0.25f;common.x4A4=19.5f;
    ftCommonData* previous=p_ftCommonData;p_ftCommonData=&common;
    NativeScriptDiagnostic diagnostic;
    for(unsigned owned=0;owned<2;++owned)for(unsigned operation=0;operation<2;++operation)
        for(unsigned freeze=0;freeze<2;++freeze)for(unsigned unchanged=0;unchanged<2;++unchanged){
            reset();capture_motion_case=true;
            Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(owned!=0);
            reciprocal_pair(owner,victim);Fighter* h=owner->user_data;Fighter* v=victim->user_data;
            v->x2222_b6=freeze;v->motion_id=unchanged?ftCo_MS_ShoulderedWait:ftCo_MS_Wait;
            v->self_vel=(Vec3){1,2,3};v->dmg.x1830_percent=33;
            v->mv.co.shouldered.x0=7;v->mv.co.shouldered.x4=8;
            v->x1A51=11;v->x1A50=12;v->x2224_b6=true;
            Fighter holder_before=*h,start=*v;
            FtMotionId motion=operation?ftCo_MS_ShoulderedWait:ftCo_MS_CaptureDamageKoopa;
            trace_count=0;
            if(operation)reference_initialize_hold(victim,motion);else reference_victim_motion(victim,motion);
            Fighter expected=*v;unsigned expected_trace[32],expected_count=trace_count;
            u32 expected_flags=capture_motion_flags;
            memcpy(expected_trace,trace,sizeof(trace));HSD_GObj* expected_donor=animation_donor;
            *v=start;trace_count=0;animation_donor=NULL;
            NativeScriptRuntimeContext context=victim_policy_context(owner);
            uint32_t semantic=operation?NativeScriptBinding_CaptureInitializeHold:NativeScriptBinding_CaptureSetVictimCaptureMotion;
            NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=motion},output;
            CHECK(nativeScriptBindings_Invoke(&context,semantic,NULL,0,&input,1,&output,&diagnostic));
            CHECK(output.type==NativeScriptType_Void&&!memcmp(v,&expected,sizeof(expected))&&!memcmp(h,&holder_before,sizeof(holder_before)));
            CHECK(trace_count==expected_count&&!memcmp(trace,expected_trace,trace_count*sizeof(*trace)));
            CHECK(!expected_count || capture_motion_flags==expected_flags);
            CHECK(animation_donor==expected_donor || !expected_count);
            presentation_state(owner,"items",&input);*v=start;trace_count=0;
            CHECK(helper(owner,operation?"HoldVictim":"VictimMotion"));
            CHECK(!memcmp(v,&expected,sizeof(expected))&&trace_count==expected_count);
            HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
        }
    for(unsigned operation=0;operation<2;++operation){
        uint32_t semantic=operation?NativeScriptBinding_CaptureInitializeHold:NativeScriptBinding_CaptureSetVictimCaptureMotion;
        for(unsigned bad=0;bad<14;++bad){
            reset();capture_motion_case=true;Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(true);
            reciprocal_pair(owner,victim);Fighter* v=victim->user_data;Fighter* h=owner->user_data;
            NativeScriptRuntimeContext context=victim_policy_context(owner);
            NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=ftCo_MS_ShoulderedWait};uint32_t count=1;
            if(bad==0)input.as.i64=-1;if(bad==1)input.as.i64=65536;if(bad==2)input.as.i64=347;
            if(bad==3)input.type=NativeScriptType_F32;if(bad==4)input.type=NativeScriptType_U32;
            if(bad==5)count=0;if(bad==6)count=2;if(bad==7)++context.generation;
            if(bad==8)++context.scene_epoch;if(bad==9)v->victim_gobj=NULL;
            if(bad==10)v->native_hitlag_link_generation++;if(bad==11)v->x1C_actionStateList=NULL;
            if(bad==12)v->x20_actionStateList=NULL;if(bad==13)v->x18++;
            Fighter before=*v,holder_before=*h;trace_count=0;
            NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
            CHECK(!nativeScriptBindingCapture_Invoke(&context,semantic,&input,count,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!trace_count);
            CHECK(!memcmp(v,&before,sizeof(before))&&!memcmp(h,&holder_before,sizeof(holder_before)));
            HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
        }
        reset();Fighter_GObj* owner=create(true);NativeScriptRuntimeContext context=victim_policy_context(owner);
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=ftCo_MS_ShoulderedWait},out;
        CHECK(nativeScriptBindings_Invoke(&context,semantic,NULL,0,&input,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!trace_count);HSD_GObjFree(owner);++cases;
        // Stop after the first authored motion callback when either actor retires.
        for(unsigned stage=0;stage<(operation?2u:1u);++stage)for(unsigned target=0;target<4;++target){
            reset();capture_motion_case=true;Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(true);
            reciprocal_pair(owner,victim);Fighter* v=victim->user_data;v->motion_id=ftCo_MS_Wait;
            context=victim_policy_context(owner);retire_step=stage?8:5;retire_target=target==1;
            victim_motion_retire_mode=target>=2?target-1:0;
            NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(v->kind);
            uint64_t identity=program->identity;
            out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
            CHECK(!nativeScriptBindingCapture_Invoke(&context,semantic,&input,1,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&trace_count==stage+1&&trace[0]==5&&trace[trace_count-1]==retire_step);
            CHECK(v->accessory1_cb==NULL&&v->x1A6A!=511);
            program->identity=identity;
            HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
        }
    }
    // Invalid float-to-int mash timers must not begin changing the victim.
    for(unsigned bad=0;bad<5;++bad){
        reset();capture_motion_case=true;Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(false);
        reciprocal_pair(owner,victim);Fighter* v=victim->user_data;
        common.x4A0=0.25f;common.x4A4=19.5f;v->dmg.x1830_percent=33;
        if(bad==0)p_ftCommonData=NULL;if(bad==1)v->dmg.x1830_percent=NAN;
        if(bad==2)common.x4A0=INFINITY;if(bad==3)common.x4A4=-100;if(bad==4)common.x4A4=0x1p31f;
        NativeScriptRuntimeContext context=victim_policy_context(owner);Fighter before=*v;
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=ftCo_MS_ShoulderedWait};
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureInitializeHold,&input,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!trace_count&&!memcmp(v,&before,sizeof(before)));
        p_ftCommonData=&common;HSD_GObjFree(victim);HSD_GObjFree(owner);++cases;
    }
    p_ftCommonData=previous;reset();
}
