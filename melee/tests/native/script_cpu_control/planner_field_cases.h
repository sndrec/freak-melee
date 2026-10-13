static void planner_field_cases(HSD_GObj* owned,HSD_GObj* foreign,
                                HSD_GObj* stock,HSD_GObj* item)
{
    NativeScriptRuntimeContext c=context(owned);Fighter* f=owned->user_data;
    const int32_t integers[]={INT32_MIN,-1,0,1,38,INT32_MAX};
    const uint32_t ids[]={NativeScriptBinding_CPUTravelFrames,NativeScriptBinding_CPUSelectedCommand};
    for(unsigned field_index=0;field_index<2;++field_index)for(unsigned n=0;n<6;++n){
        static Fighter expected;expected=*f;
        if(!field_index)expected.cpu.x90=integers[n];else expected.cpu.xA4=integers[n];
        NativeScriptValue value=integer(integers[n]),out;
        CHECK(nativeScriptBindings_Write(&c,ids[field_index],NULL,0,&value,&diagnostic));
        CHECK(!memcmp(f,&expected,sizeof(expected)));
        CHECK(nativeScriptBindings_Read(&c,ids[field_index],NULL,0,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_I32&&out.as.i64==integers[n]);++cases;++pairs;
    }
    for(unsigned n=0;n<6;++n){
        f->cpu.x90=integers[n];f->cpu.xA4=integers[5-n];f->cpu.x7C=integers[n];f->cpu.xFA_b5=n&1;
        memcpy(&f->cpu.x564,&scalar_bits[n],4);static Fighter before;before=*f;
        CHECK(helper(owned,"ObservePlannerFields"));
        CHECK(field(owned,"travel").as.i64==f->cpu.x90&&field(owned,"selected").as.i64==f->cpu.xA4);
        CHECK(field(owned,"elapsed").as.i64==f->cpu.x7C&&field(owned,"floor").as.u64==(n&1));
        NativeScriptValue width=field(owned,"width");CHECK(width.type==NativeScriptType_F32&&!memcmp(&width.as.f32,&f->cpu.x564,4));
        CHECK(!memcmp(f,&before,sizeof(before)));++cases;++pairs;
        set_field(owned,"travel",integer(integers[5-n]));set_field(owned,"selected",integer(integers[n]));
        before.cpu.x90=integers[5-n];before.cpu.xA4=integers[n];CHECK(helper(owned,"ApplyPlannerFields"));
        CHECK(!memcmp(f,&before,sizeof(before)));++pairs;
    }
    // Read-only cached fields are rejected before changing any native state.
    const uint32_t readonly[]={NativeScriptBinding_CPUElapsedFrames,NativeScriptBinding_CPUFloorInBounds,NativeScriptBinding_CPUBodyHalfWidth};
    for(unsigned n=0;n<3;++n){static Fighter before;before=*f;NativeScriptValue value=integer(1);
        CHECK(!nativeScriptBindings_Write(&c,readonly[n],NULL,0,&value,&diagnostic));CHECK(!memcmp(f,&before,sizeof(before)));++cases;}
    for(unsigned field_index=0;field_index<2;++field_index)for(unsigned bad=0;bad<5;++bad){
        NativeScriptValue value=integer(1);static Fighter before;before=*f;
        if(bad==0)value.type=NativeScriptType_Bool;if(bad==1)value.type=NativeScriptType_F32;
        if(bad==2)value.type=NativeScriptType_I64;if(bad==3)value.as.i64=(int64_t)INT32_MAX+1;
        if(bad==4)value.as.i64=(int64_t)INT32_MIN-1;
        CHECK(!nativeScriptBindingCPUControl_Write(&c,ids[field_index],&value,&diagnostic));
        CHECK(!memcmp(f,&before,sizeof(before)));++cases;
    }
    // Body width is observational: it is readable on stock and other owned
    // fighters without adopting their script package or allowing CPU writes.
    HSD_GObj* targets[]={foreign,stock};
    NativeScriptRuntimeContext article_context=context(item);
    NativeScriptReceiverScope scope={.pool=nativeScriptRuntime_Receivers()};NativeScriptValue token;
    CHECK(nativeScriptReceiver_Push(scope.pool,&article_context,article_context.scene_epoch,&primary_bank.declarations,
        Ft_Kind_CustomFirst,nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope.frame,&diagnostic));
    for(unsigned target=0;target<2;++target)for(unsigned n=0;n<sizeof(scalar_bits)/sizeof(*scalar_bits);++n){
        Fighter* observed=targets[target]->user_data;memcpy(&observed->cpu.x564,&scalar_bits[n],4);
        static Fighter before;before=*observed;
        CHECK(nativeScriptItem_SetAttachmentOwner(item,targets[target]));
        CHECK(nativeScriptReceiver_Capture(&scope,&article_context,NativeScriptReceiver_AttachmentOwner,true,false,&token,&diagnostic));
        NativeScriptValue out;
        CHECK(nativeScriptBindingReceiver_Read(&scope,&article_context,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_CPUBodyHalfWidth,&token,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,&observed->cpu.x564,4));
        CHECK(nativeScriptItem_RunSignal(item,name_key("ObserveWidth"),0,&out)==NativeScriptVM_Ok);++vm_calls;++receiver_calls;
        out=field(item,"width");CHECK(!memcmp(&out.as.f32,&observed->cpu.x564,4));
        CHECK(!memcmp(observed,&before,sizeof(before)));++cases;++pairs;
    }
    Fighter* observed=stock->user_data;uint32_t saved_generation=observed->x8_spawnNum;++observed->x8_spawnNum;
    NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel;
    CHECK(!nativeScriptBindingReceiver_Read(&scope,&article_context,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_CPUBodyHalfWidth,&token,1,&out,&diagnostic));
    CHECK(!memcmp(&out,&sentinel,sizeof(out)));observed->x8_spawnNum=saved_generation;++cases;
    CHECK(nativeScriptReceiver_Pop(scope.pool,scope.frame,&diagnostic));
}
