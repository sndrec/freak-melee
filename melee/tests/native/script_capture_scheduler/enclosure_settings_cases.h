#include <stddef.h>

static void enclosure_integer_cases(void)
{
    const struct {uint32_t semantic;size_t offset;} fields[]={
        {NativeScriptBinding_CaptureEnclosureModelSlot,offsetof(Fighter,capture_enclosure_settings.model_slot)},
        {NativeScriptBinding_CaptureEnclosureEscapeInvincibilityFrames,offsetof(Fighter,capture_enclosure_settings.escape_invincibility_frames)},
    };
    const int32_t values[]={INT32_MIN,-1,0,1,3,7,INT32_MAX};
    reset();Fighter_GObj* object=create(true);Fighter* fighter=object->user_data;
    NativeScriptRuntimeContext context={.source=object,.source_class=NativeCallbackSource_Fighter,
        .generation=fighter->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptDiagnostic diagnostic;
    for(unsigned field=0;field<2;++field)for(unsigned n=0;n<sizeof(values)/sizeof(*values);++n){
        Fighter expected=*fighter;
        memcpy((uint8_t*)&expected+fields[field].offset,&values[n],sizeof(int32_t));
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=values[n]},output;
        CHECK(nativeScriptBindingCapture_Write(&context,fields[field].semantic,&input,&diagnostic));
        CHECK(!memcmp(fighter,&expected,sizeof(expected)));
        CHECK(nativeScriptBindingCapture_Read(&context,fields[field].semantic,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_I32&&output.as.i64==values[n]);++cases;
    }
    for(unsigned field=0;field<2;++field)for(unsigned bad=0;bad<5;++bad){
        Fighter before=*fighter;
        NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=0};
        if(bad==0)input.as.i64=(int64_t)INT32_MAX+1;
        if(bad==1)input.as.i64=(int64_t)INT32_MIN-1;
        if(bad==2)input.type=NativeScriptType_Bool;
        if(bad==3)input.type=NativeScriptType_F32;
        if(bad==4)input.type=NativeScriptType_I64;
        CHECK(!nativeScriptBindingCapture_Write(&context,fields[field].semantic,&input,&diagnostic));
        CHECK(!memcmp(fighter,&before,sizeof(before)));++cases;
    }
    NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,fighter->x8_spawnNum,&view));
    for(unsigned bad=0;bad<8;++bad){
        NativeScriptRuntimeContext invalid=context;
        uint32_t generation=fighter->x8_spawnNum;
        uint64_t identity=capture_package.program.identity;
        NativeScriptStorageInstance storage=*view.storage;uint16_t classifier=object->classifier;
        if(bad==0)++invalid.generation;
        if(bad==1)++invalid.scene_epoch;
        if(bad==2)invalid.source_class=NativeCallbackSource_Item;
        if(bad==3)object->classifier=HSD_GOBJ_CLASS_STAGE;
        if(bad==4)++capture_package.program.identity;
        if(bad==5)view.storage->live=false;
        if(bad==6)++view.storage->generation;
        if(bad==7)view.storage->parameters=NULL;
        Fighter before=*fighter;
        for(unsigned field=0;field<2;++field){
            NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=1},output={.type=NativeScriptType_U64,.as.u64=99};
            CHECK(!nativeScriptBindingCapture_Write(&invalid,fields[field].semantic,&input,&diagnostic));
            CHECK(!memcmp(fighter,&before,sizeof(before)));
            CHECK(!nativeScriptBindingCapture_Read(&invalid,fields[field].semantic,&output,&diagnostic));
            CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++cases;
        }
        fighter->x8_spawnNum=generation;object->classifier=classifier;
        capture_package.program.identity=identity;*view.storage=storage;
    }
    CHECK(helper(object,"ConfigureEnclosure")&&presentation_state(object,"items",NULL).as.i64==70);
    CHECK(fighter->capture_enclosure_settings.model_slot==3&&fighter->capture_enclosure_settings.escape_invincibility_frames==1);
    HSD_GObjFree(object);
}
