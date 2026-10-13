#include <melee/gr/types.h>
#include <melee/mp/mplib.h>
StageInfo stage_info;
#include "native_floor_material.inc"

static void floor_material_cases(HSD_GObj* fighter_object,HSD_GObj* item_object)
{
    Fighter* f=fighter_object->user_data;Item* it=item_object->user_data;
    NativeScriptRuntimeContext fc=context(fighter_object),ic=context(item_object);
    const uint32_t upper[]={0,0x100,0xffff0000,0xffffff00};
    for(unsigned stage=0;stage<2;++stage){
        mpLib_803BF248[stage].x4=&material_tables[stage];stage_info.grkind=(GrKind)stage;
        for(unsigned material=0;material<20;++material){
            material_tables[stage][material]=&material_values[stage][material];
            material_values[stage][material].x0=stage*.5f+material*.125f;
            for(unsigned flags=0;flags<4;++flags){
                f->coll_data.floor.flags=it->x378_itemColl.floor.flags=material|upper[flags];
                float expected=mpLib_800569EC(f->coll_data.floor.flags);
                static Fighter before;before=*f;Item item_before=*it;NativeScriptValue value;
                CHECK(nativeScriptBindings_Read(&fc,NativeScriptBinding_FloorFriction,NULL,0,&value,&diagnostic));
                CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&expected,4));++pairs;
                CHECK(nativeScriptBindings_Read(&ic,NativeScriptBinding_FloorFriction,NULL,0,&value,&diagnostic));
                CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&expected,4));++pairs;
                CHECK(!memcmp(f,&before,sizeof(*f))&&!memcmp(it,&item_before,sizeof(*it)));
                CHECK(helper(fighter_object,"ObserveFriction"));value=field(fighter_object,"friction");
                CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&expected,4));
                surface_signal(item_object,"ObserveFriction");value=field(item_object,"friction");
                CHECK(value.type==NativeScriptType_F32&&!memcmp(&value.as.f32,&expected,4));++cases;
            }
        }
    }
    NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},value=sentinel;
    f->coll_data.floor.flags=it->x378_itemColl.floor.flags=20;
    CHECK(!nativeScriptBindings_Read(&fc,NativeScriptBinding_FloorFriction,NULL,0,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
    CHECK(!nativeScriptBindings_Read(&ic,NativeScriptBinding_FloorFriction,NULL,0,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
    f->coll_data.floor.flags=it->x378_itemColl.floor.flags=0;
    ++fc.generation;++ic.generation;
    CHECK(!nativeScriptBindings_Read(&fc,NativeScriptBinding_FloorFriction,NULL,0,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
    CHECK(!nativeScriptBindings_Read(&ic,NativeScriptBinding_FloorFriction,NULL,0,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
    fc=context(fighter_object);ic=context(item_object);++fc.scene_epoch;++ic.scene_epoch;
    CHECK(!nativeScriptBindings_Read(&fc,NativeScriptBinding_FloorFriction,NULL,0,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
    CHECK(!nativeScriptBindings_Read(&ic,NativeScriptBinding_FloorFriction,NULL,0,&value,&diagnostic));CHECK(!memcmp(&value,&sentinel,sizeof(value)));
    fc=context(fighter_object);ic=context(item_object);NativeScriptValue scalar={.type=NativeScriptType_F32,.as.f32=2};
    CHECK(!nativeScriptBindings_Write(&fc,NativeScriptBinding_FloorFriction,NULL,0,&scalar,&diagnostic));
    CHECK(!nativeScriptBindings_Write(&ic,NativeScriptBinding_FloorFriction,NULL,0,&scalar,&diagnostic));cases+=8;
}
