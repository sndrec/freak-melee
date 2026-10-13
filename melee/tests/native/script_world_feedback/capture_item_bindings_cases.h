#include <melee/ft/native_script_binding_capture.h>

static unsigned capture_binding_cases;
static uint32_t capture_helper(const char* name)
{
    for(unsigned n=0;n<bank.declarations.function_count;++n)
        if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[n].name,name))return n;
    CHECK(false);return UINT32_MAX;
}

static void capture_item_bindings_cases(void)
{
    uint32_t attach=capture_helper("CaptureAttach"),release=capture_helper("CaptureRelease");
    for(unsigned owned=0;owned<2;++owned){
        capture_fixture_pair(owned);
        Fighter* holder=capture_holder->user_data;Item* target=capture_target->user_data;
        NativeScriptRuntimeContext c=context(capture_holder);
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(nativeScriptBindingCapture_Read(&c,NativeScriptBinding_CaptureHasItem,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Bool&&out.as.u64==1);
        CHECK(nativeScriptFighter_Helper(capture_holder,holder->x8_spawnNum,attach,NULL,0,&out,&diagnostic)==NativeScriptVM_Ok);
        CHECK(out.type==NativeScriptType_Void&&target->native_capture_inert&&target->grab_victim==capture_holder);
        CHECK(nativeScriptFighter_Helper(capture_holder,holder->x8_spawnNum,release,NULL,0,&out,&diagnostic)==NativeScriptVM_Ok);
        CHECK(out.type==NativeScriptType_Void&&!holder->target_item_gobj&&capture_destroy_type==0);
        CHECK(nativeScriptBindingCapture_Read(&c,NativeScriptBinding_CaptureHasItem,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_Bool&&!out.as.u64);
        unsigned calls=capture_trace_count;
        CHECK(nativeScriptFighter_Helper(capture_holder,holder->x8_spawnNum,release,NULL,0,&out,&diagnostic)==NativeScriptVM_Ok);
        CHECK(capture_trace_count==calls); // missing target release is a native no-op
        NativeScriptValue bone={.type=NativeScriptType_I32,.as.i64=0};
        out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Invoke(&c,NativeScriptBinding_CaptureAttachItem,&bone,1,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&capture_trace_count==calls);
        HSD_GObjFree(capture_holder);++capture_binding_cases;
    }

    capture_fixture_pair(true);NativeScriptRuntimeContext c=context(capture_holder);
    const uint32_t operations[]={NativeScriptBinding_CaptureAttachItem,NativeScriptBinding_CaptureReleaseItem};
    for(unsigned op=0;op<2;++op)for(unsigned bad=0;bad<7;++bad){
        NativeScriptValue arguments[3]={{.type=NativeScriptType_I32,.as.i64=0},
            {.type=NativeScriptType_Bool,.as.u64=1},{.type=NativeScriptType_I32,.as.i64=2}};
        uint32_t count=op?2:1;
        if(bad==0)arguments[0].type=NativeScriptType_F32;
        if(bad==1)arguments[0].as.i64=-1;
        if(bad==2)arguments[0].as.i64=MAX_FT_PARTS;
        if(bad==3)arguments[0].as.i64=INT64_MAX;
        if(bad==4)count=0;
        if(bad==5)++count;
        if(bad==6){if(op)arguments[1].type=NativeScriptType_I32;else arguments[0].type=NativeScriptType_Bool;}
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingCapture_Invoke(&c,operations[op],arguments,count,&out,&diagnostic));
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!capture_trace_count);
        ++capture_binding_cases;
    }
    NativeScriptValue arguments[2]={{.type=NativeScriptType_I32,.as.i64=0},
        {.type=NativeScriptType_Bool,.as.u64=0}};
    for(unsigned bad=0;bad<5;++bad){
        NativeScriptRuntimeContext wrong=c;
        if(bad==0)wrong.scene_epoch=0;
        if(bad==1)++wrong.generation;
        if(bad==2)wrong.source_class=NativeCallbackSource_Item;
        if(bad==3)wrong=context(capture_target);
        if(bad==4)((Item*)capture_target->user_data)->native_removal_in_progress=true;
        for(unsigned op=0;op<3;++op){
            NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
            bool success=op==2?nativeScriptBindingCapture_Read(&wrong,NativeScriptBinding_CaptureHasItem,&out,&diagnostic):
                nativeScriptBindingCapture_Invoke(&wrong,operations[op],arguments,op?2:1,&out,&diagnostic);
            CHECK(!success&&out.type==NativeScriptType_U64&&out.as.u64==99&&!capture_trace_count);
            ++capture_binding_cases;
        }
    }
    ((Item*)capture_target->user_data)->native_removal_in_progress=false;
    CHECK(!nativeScriptBindingCapture_Write(&c,NativeScriptBinding_CaptureHasItem,&arguments[1],&diagnostic));
    HSD_GObjFree(capture_target);HSD_GObjFree(capture_holder);

    capture_fixture_pair(true);c=context(capture_holder);
    NativeScriptValue out;
    CHECK(nativeScriptBindingCapture_Invoke(&c,NativeScriptBinding_CaptureAttachItem,arguments,1,&out,&diagnostic));
    capture_trace_count=0;capture_stop=1;capture_mutation=3;
    out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!nativeScriptBindingCapture_Invoke(&c,NativeScriptBinding_CaptureReleaseItem,arguments,2,&out,&diagnostic));
    CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&capture_trace_count==1);
    HSD_GObjFree(capture_target);HSD_GObjFree(capture_holder);
    capture_stop=capture_mutation=0;++capture_binding_cases;
}
