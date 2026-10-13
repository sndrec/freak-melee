#include <dolphin/pad.h>
static void fighter_rumble_cases(HSD_GObj* object,HSD_GObj* article_object)
{
    Fighter* fp=object->user_data;Fighter original=*fp;StaticPlayer original_slot=player_slots[2];
    NativeScriptValue args[2]={{.type=NativeScriptType_I32},{.type=NativeScriptType_I32}},out;
    const int32_t durations[]={0,1,10,-1,-2};
    for(unsigned mask=0;mask<32;++mask)for(unsigned pattern=0;pattern<3;++pattern)for(unsigned d=0;d<5;++d){
        fp->x221F_b4=(mask&1)!=0;fp->x221F_b3=(mask&2)!=0;fp->x2224_b2=(mask&4)!=0;
        player_slots[2].flags.b0=(mask&8)!=0;player_slots[2].player_state=(mask&16)?2:0;
        args[0].as.i64=pattern;args[1].as.i64=durations[d];Fighter before=*fp;
        unsigned start=hardware_calls;
        CHECK(invoke(object,NativeScriptBinding_ControllerRumble,args,2,&out)&&out.type==NativeScriptType_Void);
        unsigned scripted=hardware_calls-start;int controller=last_controller,id=last_id,duration=last_duration,priority=last_priority;const void* list=last_commands;
        start=hardware_calls;ftCommon_8007EBAC(fp,pattern,(uint32_t)durations[d]);
        CHECK(hardware_calls-start==scripted);
        if(scripted)CHECK(last_controller==controller&&last_id==id&&last_duration==duration&&last_priority==priority&&last_commands==list);
        CHECK(!memcmp(fp,&before,sizeof(*fp)));++comparisons;
    }
    *fp=original;player_slots[2]=original_slot;
    // Preparation and real Fighter helper execution must route to the new API.
    uint32_t function=UINT32_MAX;
    for(unsigned n=0;n<bank.declarations.function_count;++n)
        if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[n].name,"Rumble"))function=n;
    CHECK(function!=UINT32_MAX);unsigned start=hardware_calls;
    CHECK(nativeScriptFighter_Helper(object,fp->x8_spawnNum,function,NULL,0,&out,&diagnostic)==NativeScriptVM_Ok);
    CHECK(out.type==NativeScriptType_Void&&hardware_calls==start+1&&last_controller==1&&last_id==3&&last_priority==6&&last_duration==-2);
    args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000001)};
    args[1]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(invoke(object,NativeScriptBinding_ControllerRumble,args,2,&out)&&last_duration==-1&&last_id==3);
    for(unsigned bad=0;bad<7;++bad){
        *fp=original;NativeScriptValue invalid[2]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_I32,.as.i64=10}};unsigned count=2;
        if(bad==0)invalid[0].as.i64=-1;
        if(bad==1)invalid[0].as.i64=3;
        if(bad==2)fp->x618_player_id=PAD_MAX_CONTROLLERS;
        if(bad==3)fp->player_id=Gm_Player_NumMax;
        if(bad==4)invalid[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};
        if(bad==5)invalid[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=10};
        if(bad==6)count=1;
        start=hardware_calls;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!invoke(object,NativeScriptBinding_ControllerRumble,invalid,count,&out)&&hardware_calls==start&&out.type==NativeScriptType_U64&&out.as.u64==99);
    }
    *fp=original;args[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=1};args[1]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=10};
    start=hardware_calls;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!invoke(article_object,NativeScriptBinding_ControllerRumble,args,2,&out)&&hardware_calls==start&&out.type==NativeScriptType_U64&&out.as.u64==99);
    const unsigned retirements[]={2,10,13,14,15,16,18};
    for(unsigned i=0;i<sizeof(retirements)/sizeof(*retirements);++i){
        feedback_source=fighter(true);Fighter* data=feedback_source->user_data;Fighter before=*data;
        uint64_t program=feedback_package.program.identity,layout=bank.storage_layout.identity,parameters=bank.storage_parameters.identity;
        feedback_mutation=retirements[i];start=hardware_calls;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!invoke(feedback_source,NativeScriptBinding_ControllerRumble,args,2,&out)&&feedback_mutation==0&&hardware_calls==start+1);
        CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
        feedback_package.program.identity=program;bank.storage_layout.identity=layout;bank.storage_parameters.identity=parameters;
        if(retirements[i]!=2){feedback_source->user_data=data;feedback_source->classifier=HSD_GOBJ_CLASS_FIGHTER;*data=before;HSD_GObjFree(feedback_source);}
        feedback_source=NULL;
    }
}
