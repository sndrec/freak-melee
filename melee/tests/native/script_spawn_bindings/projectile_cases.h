#include <melee/it/native_script_item_spawn.h>
#include <melee/ft/native_script_math.h>
#include <math.h>

static void projectile_cases(void)
{
    reject("func Start():\n    articles.launch_at_position(CustomBolt, 0, 0, 0, 0, 0, 0, 0, 1.5, 1)\narticle CustomBolt states 2:\n    state int unused\n","collision group requires an integral",false);
    reject("func Start():\n    articles.launch_at_position(CustomBolt, 0, 0, 0, 0, 0, 0, 0, 0)\narticle CustomBolt states 2:\n    state int unused\n","argument count",false);
    reject("state int choice\nfunc Start():\n    articles.launch_projectile(state.choice, 0, 0, 0, 0, 0, 1, 0)\narticle CustomBolt states 2:\n    state int unused\n","identifier constant",false);
    // Stock ItemKind constants remain numeric outside declaration calls.
    CHECK(read("native_kind").as.i64==It_Kind_Fox_Laser);
    CHECK(root.items.programs[3].owner!=It_Kind_Fox_Laser);
    set_action(15);CHECK(run());HSD_GObj* shot=by_kind(It_Kind_Mario_Fire);CHECK(shot);
    Item* data=shot->user_data;
    CHECK(nativeScriptItem_Spec(shot)==&root.items.programs[3]&&data->msid==1&&data->facing_dir==-1);
    CHECK(data->x40_vel.x==3&&data->x40_vel.y==0&&data->xD44_lifeTimer==60);
    CHECK(positioned_spawn.prev_pos.x==21&&positioned_spawn.prev_pos.y==22&&positioned_spawn.prev_pos.z==0);
    CHECK(!memcmp(&positioned_spawn.pos,&actor.cur_pos,sizeof(Vec3))&&positioned_spawn.x40==UINT32_C(0x80000000));
    CHECK(projectile_appearance==0&&projectile_variant==4&&item_value(shot,"launches")==1);
    Item_8026A8EC(shot);++cases;
    set_action(16);CHECK(run());shot=by_kind(It_Kind_Mario_Fire);CHECK(shot);data=shot->user_data;
    CHECK(projectile_joint==&joint&&projectile_offset.x==1&&projectile_offset.y==2&&projectile_offset.z==3);
    CHECK(positioned_spawn.prev_pos.x==11&&positioned_spawn.prev_pos.y==13&&positioned_spawn.prev_pos.z==0);
    CHECK(positioned_spawn.x40==0&&projectile_variant==0&&data->facing_dir==-1&&data->msid==1);
    CHECK(data->x40_vel.x==3*cosf(3.1415927410125732f)&&item_value(shot,"launches")==1);
    Item_8026A8EC(shot);++cases;

    NativeScriptRuntimeContext context={.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,
        .generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
    uint32_t owner=root.items.programs[3].owner;
    uint32_t world=NATIVE_SCRIPT_SPAWN_DOMAIN|(NativeScriptSpawn_LaunchAtPosition<<20)|owner;
    NativeScriptValue arguments[10]={{.type=NativeScriptType_I32,.as.i64=It_Kind_Fox_Laser},
        {.type=NativeScriptType_F32,.as.f32=21},{.type=NativeScriptType_F32,.as.f32=22},
        {.type=NativeScriptType_F32,.as.f32=23},{.type=NativeScriptType_F32,.as.f32=0},
        {.type=NativeScriptType_F32,.as.f32=3},{.type=NativeScriptType_I32,.as.i64=1},
        {.type=NativeScriptType_I32,.as.i64=4},{.type=NativeScriptType_I32,.as.i64=-1},
        {.type=NativeScriptType_F32,.as.f32=-1}};
    const float angles[]={-6.2831854820251465f,-3.1415927410125732f,-1.5707963705062866f,0,
        1.5707963705062866f,3.1415927410125732f,6.2831854820251465f};
    for(unsigned velocity=0;velocity<2;++velocity)for(unsigned n=0;n<sizeof(angles)/sizeof(angles[0]);++n) {
        projectile.initial_velocity=velocity!=0;spawn_order_count=0;
        NativeScriptItemLaunchData launch={.angle=angles[n],.speed=n%2?-3:3,
            .state=n%2,.collision_group=UINT32_MAX,.preserve_facing=true};
        Vec3 previous={21,22,23};Item_GObj* direct=NULL;
        CHECK(nativeScriptItemSpawn_Projectile(&context,owner,&previous,-1,4,&launch,&direct,&diagnostic)==NativeScriptVM_Ok&&direct);
        Item expected=*(Item*)direct->user_data;SpawnItem expected_spawn=positioned_spawn;
        unsigned expected_order[128],expected_count=spawn_order_count;
        memcpy(expected_order,spawn_order,expected_count*sizeof(unsigned));Item_8026A8EC(direct);
        arguments[4].as.f32=launch.angle;arguments[5].as.f32=launch.speed;arguments[6].as.i64=launch.state;
        spawn_order_count=0;NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(nativeScriptBindingSpawn_Invoke(&context,world,arguments,10,&output,&diagnostic)&&output.type==NativeScriptType_Void);
        shot=by_kind(It_Kind_Mario_Fire);CHECK(shot);data=shot->user_data;
        CHECK(!memcmp(&positioned_spawn,&expected_spawn,sizeof(SpawnItem))&&spawn_order_count==expected_count&&
            !memcmp(spawn_order,expected_order,expected_count*sizeof(unsigned)));
        CHECK(!memcmp(&data->x40_vel,&expected.x40_vel,sizeof(Vec3))&&
            !memcmp(&data->pos,&expected.pos,sizeof(Vec3))&&data->facing_dir==expected.facing_dir&&data->msid==expected.msid&&
            data->xD44_lifeTimer==expected.xD44_lifeTimer);
        CHECK(!memcmp(&data->xDD4_itemVar.native_article.launch_angle,&expected.xDD4_itemVar.native_article.launch_angle,sizeof(float))&&
            data->xDD4_itemVar.native_article.launch_speed==expected.xDD4_itemVar.native_article.launch_speed&&
            !memcmp(&data->xDD4_itemVar.native_article.launch_origin,&expected.xDD4_itemVar.native_article.launch_origin,sizeof(Vec3)));
        CHECK(item_value(shot,"launches")==1&&nativeScriptItem_Creator(shot)==spawn_caller);
        Item_8026A8EC(shot);++cases;
    }
    projectile.initial_velocity=true;
    NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    for(unsigned n=0;n<10;++n) {
        NativeScriptValue saved=arguments[n];unsigned models=scheduler_models;
        if(saved.type==NativeScriptType_F32)arguments[n].as.f32=__builtin_nanf("");
        else if(n==8)arguments[n]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)};
        else arguments[n].as.i64=INT64_MAX;
        NativeScriptValue output=sentinel;
        CHECK(!nativeScriptBindingSpawn_Invoke(&context,world,arguments,10,&output,&diagnostic)&&
            !memcmp(&output,&sentinel,sizeof(output))&&scheduler_models==models);
        arguments[n]=saved;++cases;
    }
    NativeScriptValue output=sentinel;arguments[6].as.i64=2;unsigned models=scheduler_models;
    CHECK(!nativeScriptBindingSpawn_Invoke(&context,world,arguments,10,&output,&diagnostic)&&
        !memcmp(&output,&sentinel,sizeof(output))&&scheduler_models==models);arguments[6].as.i64=1;++cases;

    // The actual constructor/state/appearance/debug/update leaves can retire
    // the logical caller. Created/Launched are real interpreted hooks; their
    // stored result is checked above, not substituted with a successful stub.
    const unsigned stages[]={3,4,6,7};
    for(unsigned n=0;n<sizeof(stages)/sizeof(stages[0]);++n) {
        spawn_retire_stage=stages[n];spawn_order_count=0;output=sentinel;
        CHECK(!nativeScriptBindingSpawn_Invoke(&context,world,arguments,10,&output,&diagnostic)&&
            !memcmp(&output,&sentinel,sizeof(output))&&nativeScriptRuntime_Idle());
        CHECK(spawn_order_count&&spawn_order[spawn_order_count-1]==stages[n]);
        shot=by_kind(It_Kind_Mario_Fire);CHECK(shot&&nativeScriptItem_IsAttached(shot));Item_8026A8EC(shot);
        caller();context.source=spawn_caller;++cases;
    }
}
