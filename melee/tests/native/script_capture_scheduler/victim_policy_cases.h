// Actual ftColl_8007B62C is mechanically extracted into actual_capture.inc.
// Its color-animation leaf is controlled; no rendering is claimed here.
static NativeScriptRuntimeContext victim_policy_context(Fighter_GObj* owner)
{
    return (NativeScriptRuntimeContext){.source=owner,.source_class=NativeCallbackSource_Fighter,
        .generation=((Fighter*)owner->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
}

static void victim_policy_cases(void)
{
    NativeScriptDiagnostic diagnostic;
    for(unsigned scripted=0;scripted<2;++scripted){
        reset();Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(scripted!=0);
        link_pair(owner,victim);Fighter* holder=owner->user_data;Fighter* caught=victim->user_data;
        NativeScriptRuntimeContext context=victim_policy_context(owner);
        for(unsigned visible=0;visible<2;++visible){
            Fighter expected=*caught,before=*holder;expected.invisible=!visible;
            NativeScriptValue input={.type=NativeScriptType_Bool,.as.u64=visible},output;
            CHECK(nativeScriptBindingCapture_Write(&context,NativeScriptBinding_CaptureVictimVisible,&input,&diagnostic));
            CHECK(!memcmp(caught,&expected,sizeof(expected))&&!memcmp(holder,&before,sizeof(before)));
            CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureVictimVisible,&output,&diagnostic));
            CHECK(output.type==NativeScriptType_Bool&&output.as.u64==visible);++cases;
        }
        for(int status=0;status<3;++status){
            Fighter start=*caught;victim_color_calls=0;trace_count=0;
            ftColl_8007B62C(victim,status);Fighter expected=*caught;
            enum_t expected_color=victim_color;CHECK(victim_color_calls==1&&trace_count==1&&trace[0]==11);
            *caught=start;victim_color_calls=trace_count=0;
            NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=status},output={.type=NativeScriptType_U64,.as.u64=99};
            CHECK(nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureSetVictimBodyStatus,&input,1,&output,&diagnostic));
            CHECK(output.type==NativeScriptType_Void&&!memcmp(caught,&expected,sizeof(expected)));
            CHECK(victim_color_calls==1&&victim_color==expected_color&&trace_count==1&&trace[0]==11);++cases;
        }
        CHECK(helper(owner,"HideVictim")&&caught->invisible&&caught->x1988==2);
        CHECK(presentation_state(owner,"items",NULL).as.i64==0);
        CHECK(helper(owner,"ShowVictim")&&!caught->invisible&&caught->x1988==0);
        CHECK(presentation_state(owner,"items",NULL).as.i64==1);cases+=2;
        for(unsigned bad=0;bad<6;++bad){
            Fighter before=*caught;unsigned calls=victim_color_calls;
            NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=0},output={.type=NativeScriptType_U64,.as.u64=99};
            uint32_t count=1;
            if(bad==0)input.as.i64=-1;if(bad==1)input.as.i64=3;
            if(bad==2)input.type=NativeScriptType_F32;if(bad==3)input.type=NativeScriptType_Bool;
            if(bad==4)count=0;if(bad==5)count=2;
            CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureSetVictimBodyStatus,&input,count,&output,&diagnostic));
            CHECK(!memcmp(caught,&before,sizeof(before))&&victim_color_calls==calls);
            CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++cases;
        }
        NativeScriptValue wrong={.type=NativeScriptType_I32,.as.i64=1};Fighter before=*caught;
        CHECK(!nativeScriptBindingCapture_Write(&context,NativeScriptBinding_CaptureVictimVisible,&wrong,&diagnostic));
        CHECK(!memcmp(caught,&before,sizeof(before)));++cases;
        // A stale captured actor cannot be observed or changed, even while the
        // holder's own interpreter program remains alive.
        ++caught->x8_spawnNum;before=*caught;
        NativeScriptValue visible={.type=NativeScriptType_Bool,.as.u64=0},output={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureVictimVisible,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);
        CHECK(!nativeScriptBindingCapture_Write(&context,NativeScriptBinding_CaptureVictimVisible,&visible,&diagnostic));
        CHECK(!memcmp(caught,&before,sizeof(before)));++cases;
        HSD_GObjFree(victim);HSD_GObjFree(owner);
    }
    for(unsigned target=0;target<2;++target){
        reset();Fighter_GObj* owner=create(true);Fighter_GObj* victim=create(false);link_pair(owner,victim);
        NativeScriptRuntimeContext context=victim_policy_context(owner);retire_step=11;retire_target=target;
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=2},output={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureSetVictimBodyStatus,&input,1,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++cases;
        HSD_GObjFree(victim);HSD_GObjFree(owner);
    }
    reset();Fighter_GObj* owner=create(true);NativeScriptRuntimeContext context=victim_policy_context(owner);
    Fighter before=*(Fighter*)owner->user_data;unsigned calls=victim_color_calls;
    NativeScriptValue visible={.type=NativeScriptType_Bool,.as.u64=1},output;
    CHECK(nativeScriptBindingCapture_Write(&context,NativeScriptBinding_CaptureVictimVisible,&visible,&diagnostic));
    CHECK(nativeScriptBindingCapture_Read(&context,NativeScriptBinding_CaptureVictimVisible,&output,&diagnostic));
    CHECK(output.type==NativeScriptType_Bool&&!output.as.u64);
    NativeScriptValue status={.type=NativeScriptType_I32,.as.i64=2};
    CHECK(nativeScriptBindingCapture_Invoke(&context,NativeScriptBinding_CaptureSetVictimBodyStatus,&status,1,&output,&diagnostic));
    CHECK(output.type==NativeScriptType_Void&&victim_color_calls==calls);
    CHECK(!memcmp(owner->user_data,&before,sizeof(before)));++cases;HSD_GObjFree(owner);
}
