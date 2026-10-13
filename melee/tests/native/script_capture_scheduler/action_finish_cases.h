// Native finishing bodies pinned before this change; release ordering is the
// original exporter's apply_throw_damage + selected launch sequence.
static NativeScriptValue finish_saved_value(const NativeScriptReceiverView* view,const char* name)
{
    // These injections retire identity only, not physical storage. Inspect the
    // retained storage directly instead of asking for a new live actor view.
    NativeScriptDiagnostic diagnostic;NativeScriptValue result={0};
    for(unsigned i=0;i<view->program->value_count;++i)
        if(!view->program->values[i].owner&&nativeScriptSource_SpanEquals(view->program->tree,view->program->values[i].name,name)){
            CHECK(nativeScriptStorage_Read(view->storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&result,&diagnostic));return result;
        }
    CHECK(false);return result;
}
static void action_finish_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    for(unsigned air=0;air<2;++air){
        reset();Fighter_GObj* h=create(true);Fighter* fp=h->user_data;
        fp->ground_or_air=air?GA_Air:GA_Ground;Fighter before=*fp;
        reference_ftCommon_8007D92C(h);Fighter expected=*fp;unsigned n=trace_count;
        *fp=before;trace_count=0;NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
        CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_StateFinish,NULL,0,NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Void&&!memcmp(fp,&expected,sizeof(expected))&&trace_count==n);
        CHECK(trace[0]==(air?24:25));*fp=before;trace_count=0;
        CHECK(helper(h,"Finish")&&!memcmp(fp,&expected,sizeof(expected)));
        CHECK(presentation_state(h,"items",NULL).as.i64==99);
        HSD_GObjFree(h);++cases;
    }
    for(unsigned owned=0;owned<2;++owned)for(unsigned has=0;has<2;++has)
        for(unsigned fall=0;fall<2;++fall)for(unsigned credit=0;credit<2;++credit)
        for(unsigned upward=0;upward<2;++upward)for(unsigned air=0;air<2;++air){
            if(fall&&(credit||upward))continue;
            reset();throw_case=true;Fighter_GObj* h=create(true);Fighter_GObj* v=has?create(owned!=0):NULL;
            if(v)reciprocal_pair(h,v);Fighter* holder=h->user_data;Fighter* victim=v?v->user_data:NULL;
            if(v)victim->ground_or_air=air?GA_Air:GA_Ground;
            Fighter hs=*holder,vs={0};if(v)vs=*victim;
            if(v){ftCo_800DE2A8(h,v);if(fall)reference_ftCo_80090780(v);else ftCo_800DE7C0(v,credit?h:NULL,upward!=0);}
            Fighter he=*holder,ve={0};if(v)ve=*victim;
            unsigned expected[32],n=trace_count;memcpy(expected,trace,sizeof(trace));
            HSD_GObj* c=release_credit;bool u=release_upward;
            *holder=hs;if(v)*victim=vs;trace_count=0;release_credit=NULL;release_upward=false;
            NativeScriptRuntimeContext context=victim_policy_context(h);NativeScriptValue out;
            NativeScriptValue args[]={{.type=NativeScriptType_Bool,.as.u64=credit},{.type=NativeScriptType_Bool,.as.u64=upward}};
            uint32_t semantic=fall?NativeScriptBinding_CaptureReleaseDamageFall:NativeScriptBinding_CaptureReleaseDamage;
            CHECK(nativeScriptBindings_Invoke(&context,semantic,NULL,0,fall?NULL:args,fall?0:2,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_Void&&!memcmp(holder,&he,sizeof(he))&&(!v||!memcmp(victim,&ve,sizeof(ve))));
            CHECK(trace_count==n&&!memcmp(trace,expected,n*sizeof(*trace))&&release_credit==c&&release_upward==u);
            *holder=hs;if(v)*victim=vs;trace_count=0;
            NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=credit};presentation_state(h,"items",&value);
            value.as.i64=upward;presentation_state(h,"catches",&value);
            CHECK(helper(h,fall?"ReleaseDamageFall":"ReleaseDamage"));
            CHECK(!memcmp(holder,&he,sizeof(he))&&(!v||!memcmp(victim,&ve,sizeof(ve)))&&trace_count==n);
            CHECK(presentation_state(h,"victims",NULL).as.i64==99);
            if(v)HSD_GObjFree(v);HSD_GObjFree(h);++cases;
        }
    // Native continuations must not publish or resume the following script
    // statement after either actor generation or a published program retires.
    const unsigned points[]={9,10,26,5};
    for(unsigned point=0;point<4;++point)for(unsigned target=0;target<3;++target){
        reset();throw_case=true;Fighter_GObj* h=create(true);Fighter_GObj* v=create(true);reciprocal_pair(h,v);
        ((Fighter*)v->user_data)->ground_or_air=GA_Air;
        retirement_holder=h;retire_target=target==1;victim_motion_retire_mode=target==2?2:0;retire_step=points[point];
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(((Fighter*)h->user_data)->kind);uint64_t identity=program->identity;
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(h,((Fighter*)h->user_data)->x8_spawnNum,&view));
        CHECK(!helper(h,point<2?"ReleaseDamage":"ReleaseDamageFall"));
        CHECK(finish_saved_value(&view,"victims").as.i64!=99);
        // Damage-fall owns only the victim; retiring the holder does not stop
        // the victim's independent native transition, but does stop the VM.
        CHECK(trace[trace_count-1]==((point>=2&&target==0)?28:points[point]));
        program->identity=identity;HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
    for(unsigned air=0;air<2;++air)for(unsigned program_change=0;program_change<2;++program_change){
        reset();Fighter_GObj* h=create(true);((Fighter*)h->user_data)->ground_or_air=air?GA_Air:GA_Ground;
        retirement_holder=h;retire_step=air?24:25;victim_motion_retire_mode=program_change?2:0;
        NativeScriptFighterProgram* program=(NativeScriptFighterProgram*)nativeScriptFighter_Program(((Fighter*)h->user_data)->kind);uint64_t identity=program->identity;
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(h,((Fighter*)h->user_data)->x8_spawnNum,&view));
        CHECK(!helper(h,"Finish")&&finish_saved_value(&view,"items").as.i64!=99);
        program->identity=identity;HSD_GObjFree(h);++cases;
    }
    for(unsigned invalid=0;invalid<5;++invalid){
        reset();Fighter_GObj* h=create(true);Fighter_GObj* v=create(false);reciprocal_pair(h,v);
        NativeScriptRuntimeContext context=victim_policy_context(h);
        NativeScriptValue args[]={{.type=NativeScriptType_Bool},{.type=NativeScriptType_Bool}},out={.type=NativeScriptType_U64,.as.u64=99};
        if(invalid==0)args[0].type=NativeScriptType_I32;
        if(invalid==1)args[1].type=NativeScriptType_F32;
        if(invalid==2)((Fighter*)v->user_data)->victim_gobj=NULL;
        Fighter hb=*(Fighter*)h->user_data,vb=*(Fighter*)v->user_data;
        bool ok=invalid==3?nativeScriptBindingState_Invoke(&context,NativeScriptBinding_StateFinish,args,1,&out,&diagnostic):
            nativeScriptBindingCapture_Invoke(&context,invalid==4?NativeScriptBinding_CaptureReleaseDamageFall:NativeScriptBinding_CaptureReleaseDamage,args,2,&out,&diagnostic);
        CHECK(!ok&&out.type==NativeScriptType_U64&&out.as.u64==99&&trace_count==0);
        CHECK(!memcmp(h->user_data,&hb,sizeof(hb))&&!memcmp(v->user_data,&vb,sizeof(vb)));
        HSD_GObjFree(v);HSD_GObjFree(h);++cases;
    }
    reset();
}
