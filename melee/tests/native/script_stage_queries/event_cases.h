#include <melee/gr/stage_events.h>

static unsigned event_calls;
static uint32_t event_channel;
static float event_payload;
static bool event_result,event_detach;
static HSD_GObj* event_source;

static bool event_provider(u32 channel,float payload)
{
    ++event_calls;event_channel=channel;event_payload=payload;
    if(event_detach)nativeScriptFighter_Detach(event_source);
    return event_result;
}

static void event_cases(HSD_GObj* object)
{
    NativeScriptRuntimeContext c=context(object);
    NativeScriptDiagnostic d={0};
    NativeScriptValue args[2]={{.type=NativeScriptType_U32,.as.u64=StageEvent_Conversation_start},
                             {.type=NativeScriptType_F32,.as.f32=1}};
    NativeScriptValue out=sentinel(),saved=out;
    stage_info.on_event=NULL;
    CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageEvent,args,2,&out,&d));
    CHECK(out.type==NativeScriptType_Bool&&!out.as.u64&&!event_calls);++cases;
    stage_info.on_event=event_provider;
    const uint32_t channels[]={0,StageEvent_Conversation_available,StageEvent_Conversation_active,
        StageEvent_Conversation_start,StageEvent_Conversation_cancel,UINT32_MAX};
    const float payloads[]={0,-0.0f,1,-1,0.5f,FLT_MAX};
    for(unsigned i=0;i<sizeof(channels)/sizeof(*channels);++i)
        for(unsigned j=0;j<sizeof(payloads)/sizeof(*payloads);++j)
            for(unsigned accepted=0;accepted<2;++accepted) {
                args[0].as.u64=channels[i];args[1].as.f32=payloads[j];event_result=accepted;
                CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageEvent,args,2,&out,&d));
                CHECK(out.type==NativeScriptType_Bool&&out.as.u64==accepted);
                CHECK(event_channel==channels[i]&&!memcmp(&event_payload,&payloads[j],sizeof(float)));
                ++cases;++pairs;
            }
    unsigned calls=event_calls;
    const NativeScriptValue bad[]={
        {.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I32,.as.i64=INT64_MAX},
        {.type=NativeScriptType_I64,.as.i64=-1},{.type=NativeScriptType_U32,.as.u64=UINT64_MAX},
        {.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)},
        {.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_Bool,.as.u64=1}};
    args[1].as.f32=1;
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i) {
        args[0]=bad[i];out=saved;
        CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageEvent,args,2,&out,&d));
        CHECK(!memcmp(&out,&saved,sizeof(out))&&event_calls==calls);++cases;++negative_cases;
    }
    args[0]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=StageEvent_Conversation_start};
    args[1].as.f32=NAN;out=saved;
    CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageEvent,args,2,&out,&d));
    CHECK(!memcmp(&out,&saved,sizeof(out))&&event_calls==calls);++cases;++negative_cases;
    event_result=true;
    args[1].as.f32=1;
    uint32_t function_id=UINT32_MAX;
    for(unsigned i=0;i<bank.declarations.function_count;++i)
        if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[i].name,"EmitEvent"))function_id=i;
    CHECK(function_id!=UINT32_MAX);out=saved;
    CHECK(helper(object,function_id,&out)&&out.type==NativeScriptType_Bool&&out.as.u64);
    CHECK(event_channel==StageEvent_Conversation_start&&event_payload==1);++cases;
    HSD_GObj* retired=create();event_source=retired;event_detach=true;c=context(retired);out=saved;
    CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageEvent,args,2,&out,&d));
    CHECK(!memcmp(&out,&saved,sizeof(out)));++cases;++negative_cases;
    event_detach=false;event_source=NULL;HSD_GObjFree(retired);stage_info.on_event=NULL;
}
