#include <stddef.h>
static void capture_settings_cases(void)
{
    const struct {uint32_t semantic;size_t offset;} properties[]={
        {NativeScriptBinding_CaptureTimerDamageMultiplier,offsetof(Fighter,capture_settings.timer_damage_multiplier)},
        {NativeScriptBinding_CaptureTimerDecay,offsetof(Fighter,capture_settings.timer_decay)},
        {NativeScriptBinding_CaptureMashReduction,offsetof(Fighter,capture_settings.mash_reduction)},
        {NativeScriptBinding_CaptureMashPauseFrames,offsetof(Fighter,capture_settings.mash_pause_frames)},
        {NativeScriptBinding_CaptureMashAnimationRate,offsetof(Fighter,capture_settings.mash_animation_rate)},
        {NativeScriptBinding_CaptureWiggleScale,offsetof(Fighter,capture_settings.wiggle_scale)},
        {NativeScriptBinding_CaptureWiggleLimit,offsetof(Fighter,capture_settings.wiggle_limit)},
        {NativeScriptBinding_CaptureEnclosureLaunchX,offsetof(Fighter,capture_enclosure_settings.launch_velocity.x)},
        {NativeScriptBinding_CaptureEnclosureLaunchY,offsetof(Fighter,capture_enclosure_settings.launch_velocity.y)},
        {NativeScriptBinding_CaptureEnclosureLaunchZ,offsetof(Fighter,capture_enclosure_settings.launch_velocity.z)},
        {NativeScriptBinding_CaptureEnclosureDamageBehavior,offsetof(Fighter,capture_enclosure_settings.damage_behavior)},
        {NativeScriptBinding_CaptureEnclosureShrinkFraction,offsetof(Fighter,capture_enclosure_settings.shrink_fraction)},
        {NativeScriptBinding_CaptureEnclosureShrinkFrames,offsetof(Fighter,capture_enclosure_settings.shrink_frames)},
        {NativeScriptBinding_CaptureEnclosureLifetime,offsetof(Fighter,capture_enclosure_settings.lifetime)},
        {NativeScriptBinding_CaptureEnclosureDamageTimeScale,offsetof(Fighter,capture_enclosure_settings.damage_time_scale)},
        {NativeScriptBinding_CaptureEnclosureEscapeX,offsetof(Fighter,capture_enclosure_settings.escape_velocity.x)},
        {NativeScriptBinding_CaptureEnclosureEscapeY,offsetof(Fighter,capture_enclosure_settings.escape_velocity.y)},
        {NativeScriptBinding_CaptureEnclosureEscapeZ,offsetof(Fighter,capture_enclosure_settings.escape_velocity.z)},
    };
    const uint32_t bits[]={0,0x80000000u,0x3f800000u,0xc0200000u,1,0x007fffffu,0x7f7fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u};
    reset();Fighter_GObj* owner=create(true);Fighter* fp=owner->user_data;
    NativeScriptRuntimeContext context={.source=owner,.source_class=NativeCallbackSource_Fighter,.generation=fp->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};NativeScriptDiagnostic d;
    for(unsigned n=0;n<sizeof(properties)/sizeof(*properties);++n)for(unsigned b=0;b<sizeof(bits)/sizeof(*bits);++b){
        Fighter expected=*fp;NativeScriptValue value={.type=NativeScriptType_F32},out;
        memcpy(&value.as.f32,&bits[b],sizeof(float));memcpy((uint8_t*)&expected+properties[n].offset,&bits[b],sizeof(float));
        CHECK(nativeScriptBindingCapture_Write(&context,properties[n].semantic,&value,&d));
        CHECK(!memcmp(fp,&expected,sizeof(*fp)));
        CHECK(nativeScriptBindingCapture_Read(&context,properties[n].semantic,&out,&d)&&out.type==NativeScriptType_F32);
        CHECK(!memcmp(&out.as.f32,&bits[b],sizeof(float))&&!memcmp(fp,&expected,sizeof(*fp)));++cases;
    }
    CHECK(helper(owner,"ConfigureCapture")&&presentation_state(owner,"items",NULL).as.i64==28);
    for(unsigned n=0;n<7;++n){
        float actual;memcpy(&actual,(uint8_t*)fp+properties[n].offset,sizeof(actual));CHECK(actual==(float)(n+1));
    }
    NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(owner,fp->x8_spawnNum,&view));
    for(unsigned bad=0;bad<8;++bad){
        uint32_t generation=fp->x8_spawnNum;uint64_t identity=capture_package.program.identity;
        NativeScriptStorageInstance storage=*view.storage;uint16_t classifier=owner->classifier;
        NativeScriptRuntimeContext invalid=context;
        if(bad==0)++invalid.generation;
        if(bad==1)++invalid.scene_epoch;
        if(bad==2)invalid.source_class=NativeCallbackSource_Item;
        if(bad==3)owner->classifier=HSD_GOBJ_CLASS_STAGE;
        if(bad==4)++capture_package.program.identity;
        if(bad==5)view.storage->live=false;
        if(bad==6)++view.storage->generation;
        if(bad==7)view.storage->parameters=NULL;
        Fighter before=*fp;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99},value={.type=NativeScriptType_F32,.as.f32=13};
        for(unsigned n=0;n<sizeof(properties)/sizeof(*properties);++n){
            CHECK(!nativeScriptBindingCapture_Read(&invalid,properties[n].semantic,&out,&d));
            CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
            CHECK(!nativeScriptBindingCapture_Write(&invalid,properties[n].semantic,&value,&d));
            CHECK(!memcmp(fp,&before,sizeof(*fp)));++cases;
        }
        fp->x8_spawnNum=generation;owner->classifier=classifier;capture_package.program.identity=identity;*view.storage=storage;
    }
    Fighter before=*fp;NativeScriptValue wrong={.type=NativeScriptType_Bool,.as.u64=1};
    for(unsigned n=0;n<sizeof(properties)/sizeof(*properties);++n){CHECK(!nativeScriptBindingCapture_Write(&context,properties[n].semantic,&wrong,&d));CHECK(!memcmp(fp,&before,sizeof(*fp)));}
    HSD_GObjFree(owner);
}
