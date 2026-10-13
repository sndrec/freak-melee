#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
#include <melee/ft/native_script_binding_article_reference_resolve.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_effect_dispatch.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/ftdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/inlines.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/lb/types.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"feedback %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static Fighter fighter;static Item item;static HSD_GObj object,item_object;
static NativeSourcePackage source;static NativeScriptFighterProgram selected;
static NativeScriptStorageInstance storage;static MotionState owned_motions[8];
static NativeMatchPoolSlot pool_slots[1];static uint32_t free_order[1];NativeMatchPool nativeGObjPool;static uint64_t epoch=UINT64_C(0x100000000);
static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static FighterBone bones[140];static HSD_JObj joints[3];static DynamicBoneTable dynamic;static ItemModelDesc model;static Article article;
static int effect_id,wind_frames;static unsigned async_mode;static HSD_JObj* effect_joint;static Vec3 wind_position;static float wind_values[3];static bool retire;static float effect_scalar;static unsigned effect_calls;
static Vec3 effect_position;static float effect_second_scalar;static unsigned retire_position;
static NativeScriptRuntimeContext context;
// Unselected creator-root and owned Item wind paths are outside this fixture.
HSD_JObjInfo hsdJObj;u8 HSD_GObj_JObjKind;
bool hsdObjIsDescendantOf(HSD_Obj* o,HSD_ClassInfo* c){(void)o;(void)c;abort();}
bool nativeScriptItem_IsAttached(Item_GObj* o){(void)o;abort();}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* o){(void)o;abort();}
bool nativeScriptItem_View(Item_GObj* o,uint32_t g,NativeScriptReceiverView* v){(void)o;(void)g;(void)v;abort();}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* o){(void)o;abort();}
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind==source.target?&source:NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){return kind==source.target?&selected:NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* actor){return actor==&object?NativeScriptVM_Ok:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_BindCallback(Fighter_GObj* actor,uint32_t slot,uint32_t function)
{CHECK(actor==&object&&slot==NativeScriptFighterCallback_ReflectHit&&function==UINT32_MAX);fighter.reflect_hit_cb=NULL;return true;}
bool nativeScriptFighter_View(Fighter_GObj* actor,uint32_t generation,NativeScriptReceiverView* view){
    if(actor!=&object||generation!=17||!storage.live)return false;
    *view=(NativeScriptReceiverView){.program=&source.compiler.bank.declarations,.storage=&storage};return true;
}
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* actor,...){
    CHECK(actor==&object);va_list v;va_start(v,actor);
    if(nativeScriptEffectDispatch_Arguments(id)==NativeScriptEffectArguments_PositionTwoScalar){
        effect_position=*va_arg(v,Vec3*);effect_scalar=*va_arg(v,float*);effect_second_scalar=*va_arg(v,float*);
    }else{effect_joint=va_arg(v,HSD_JObj*);if(nativeScriptEffectDispatch_Arguments(id)==NativeScriptEffectArguments_JointScalar)effect_scalar=*va_arg(v,float*);}
    va_end(v);++effect_calls;effect_id=id;if(retire)++fighter.x8_spawnNum;
    switch(retire_position){
    case 1:++fighter.x8_spawnNum;break;
    case 2:context.scene_epoch+=UINT64_C(0x100000000);break;
    case 3:++selected.identity;break;
    case 4:storage.live=false;break;
    case 5:++source.compiler.bank.storage_layout.identity;break;
    case 6:++source.compiler.bank.storage_parameters.identity;break;
    case 7:fighter.parts=NULL;break;
    case 8:object.user_data=NULL;break;
    case 9:source.assets.parts.part_to_joint=NULL;break;
    }
    return (EF_SpawnResult){0};
}
void efAsync_Spawn(HSD_GObj* o,void* pointer,u32 mode,u32 id,HSD_JObj* joint,...){(void)o;(void)pointer;effect_joint=joint;effect_id=(int)id;async_mode=mode;if(retire)++item.x1C;}
void lb_8000B1CC(HSD_JObj* j,Vec3* offset,Vec3* out){(void)offset;*out=j->translate;}
void lb_800119DC(Vec3* p,int frames,float a,float b,float c){wind_position=*p;wind_frames=frames;wind_values[0]=a;wind_values[1]=b;wind_values[2]=c;}
#include "source_contract.inc"
#include "dispatch_expected.inc"
#include "actual_feedback.h"
static NativeScriptValue floating(float v){return (NativeScriptValue){.type=NativeScriptType_F32,.as.f32=v};}
static NativeScriptValue integer(unsigned v){return (NativeScriptValue){.type=NativeScriptType_U32,.as.u64=v};}
static NativeScriptDiagnostic diagnostic;
static void setup(void)
{
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,&object,sizeof(object),pool_slots,free_order,1,sizeof(HSD_GObj),_Alignof(HSD_GObj),++epoch));
    NativeMatchPoolReference reference;CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&reference)==&object);
    source=(NativeSourcePackage){0};source.address=&source;source.ready=source.assets.ready=true;source.target=Ft_Kind_CustomFirst;
    source.compiler.bank.valid=source.compiler.bank.has_motion_layout=true;source.compiler.bank.motion_layout=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=8};
    source.assets.facts.moves=source.compiler.bank.motion_layout;source.assets.facts.joints=3;source.assets.motions=owned_motions;
    source.compiler.bank.storage_layout.identity=123;source.compiler.bank.storage_parameters.identity=456;
    storage=(NativeScriptStorageInstance){.layout=&source.compiler.bank.storage_layout,.parameters=&source.compiler.bank.storage_parameters,.owner=0,.generation=17,.identity=17,.live=true};
    source.program.bank=&source.compiler.bank;source.program.identity=789;selected=source.program;
    fighter=(Fighter){0};item=(Item){0};object=(HSD_GObj){.user_data=&fighter};item_object=(HSD_GObj){.user_data=&item,.hsd_obj=&joints[0]};
    fighter.kind=fighter.behavior_kind=Ft_Kind_CustomFirst;fighter.ft_data=&source.assets.character.data;gFtDataList[fighter.kind]=fighter.ft_data;ftData_CharacterStateTables[fighter.kind]=owned_motions;fighter.gobj=&object;fighter.x8_spawnNum=17;fighter.parts=bones;fighter.facing_dir=-1;item.entity=&item_object;item.x1C=18;
    heads[HSD_GOBJ_PLINK_FIGHTER]=&object;heads[HSD_GOBJ_PLINK_ITEM]=&item_object;
    source.assets.parts=(FighterPartsTable){.parts_num=3,.part_to_joint=source.assets.part_to_joint,.joint_to_part=source.assets.joint_to_part};tables[fighter.kind]=&source.assets.parts;
    for(unsigned i=0;i<54;++i)source.assets.part_to_joint[i]=(i+2)%3;for(unsigned i=0;i<3;++i){bones[i].joint=&joints[i];joints[i].translate=(Vec3){i+2,i+4,i+6};dynamic.bones[i]=&joints[i];joints[i].child=i<2?&joints[i+1]:NULL;}
    model=(ItemModelDesc){.x4_bone_count=3};article=(Article){.x10_modelDesc=&model};item.xC4_article_data=&article;item.xBBC_dynamicBoneTable=&dynamic;
    context=(NativeScriptRuntimeContext){.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17,.scene_epoch=epoch};effect_joint=NULL;effect_scalar=0;effect_calls=0;effect_id=wind_frames=-1;async_mode=99;retire=false;
    effect_position=(Vec3){0};effect_second_scalar=0;retire_position=0;
}
static void position_two_scalar(void)
{
    NativeScriptValue args[]={integer(0x3FF),floating(-4.5f),floating(3.25f),floating(-0.0f),floating(-1),floating(1.0471976f)};
    const uint32_t semantic=NativeScriptBinding_EffectsSpawnWithTwoScalarsAtPosition;
    CHECK(expected_arguments[0x3FF]==NativeScriptEffectArguments_PositionTwoScalar);
    for(unsigned i=0;i<32;++i){setup();args[1]=floating(i*.125f-2);args[4]=floating(i&1?-1:1);args[5]=floating(i*.25f-4);
        Fighter before=fighter;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(nativeScriptBindingFeedback_Invoke(&context,semantic,args,6,&out,&diagnostic));
        Vec3 expected={args[1].as.f32,args[2].as.f32,args[3].as.f32};
        CHECK(effect_calls==1&&effect_id==0x3FF&&out.type==NativeScriptType_Void);
        CHECK(!memcmp(&effect_position,&expected,sizeof(expected))&&effect_scalar==args[4].as.f32&&effect_second_scalar==args[5].as.f32);
        CHECK(!memcmp(&fighter,&before,sizeof(fighter)));
    }
    // Reject malformed values and unrelated recipes before the variadic leaf.
    for(unsigned mismatch=0;mismatch<17;++mismatch){setup();NativeScriptValue invalid[6];memcpy(invalid,args,sizeof(invalid));unsigned count=6;
        if(mismatch==0)count=5;
        else if(mismatch==1)count=7;
        else if(mismatch==2)invalid[0]=floating(1023);
        else if(mismatch==3)invalid[0]=(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=-1};
        else if(mismatch==4)invalid[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100003FF)};
        else if(mismatch==5)invalid[0]=integer(0x3F1);
        else if(mismatch==6)invalid[0]=integer(0x3F7);
        else if(mismatch<12)invalid[mismatch-6]=floating(mismatch&1?NAN:INFINITY);
        else invalid[mismatch-11]=integer(1);
        Fighter before=fighter;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingFeedback_Invoke(&context,semantic,invalid,count,&out,&diagnostic));
        CHECK(!effect_calls&&!memcmp(&fighter,&before,sizeof(fighter))&&out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    }
    for(unsigned mismatch=1;mismatch<=9;++mismatch){
        setup();retire_position=mismatch;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingFeedback_Invoke(&context,semantic,args,6,&out,&diagnostic));
        CHECK(effect_calls==1&&out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    }
    for(unsigned mismatch=0;mismatch<6;++mismatch){setup();
        if(mismatch==0)source.ready=false;
        else if(mismatch==1)storage.live=false;
        else if(mismatch==2)source.assets.parts.part_to_joint=NULL;
        else if(mismatch==3)context.scene_epoch+=UINT64_C(0x100000000);
        else if(mismatch==4)context.generation=16;
        else context.source_class=NativeCallbackSource_Item;
        NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingFeedback_Invoke(&context,semantic,args,6,&out,&diagnostic));
        CHECK(!effect_calls&&out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    }
}
int main(void)
{
    position_two_scalar();
    CHECK(expected_arguments[1276]==NativeScriptEffectArguments_Joint&&nativeScriptEffectDispatch_Arguments(1276)==expected_arguments[1276]);
    CHECK(expected_arguments[1146]==NativeScriptEffectArguments_JointScalar&&nativeScriptEffectDispatch_Arguments(1146)==expected_arguments[1146]);
    for(unsigned sample=0;sample<64;++sample){setup();NativeScriptValue args[9]={integer(sample%3),{.type=NativeScriptType_I32,.as.i64=sample*4},floating(sample*.25f),floating(-2),floating(3),floating(4.5f),floating(1.25f),floating(0.75f),{.type=NativeScriptType_Bool,.as.u64=sample&1}};
        ReflectDesc shape={.x0_bone_id=sample%3,.x4_max_damage=sample*4,.x8_offset={sample*.25f,-2,3},.x14_size=4.5f,.x18_damage_mul=1.25f,.x1C_speed_mul=.75f,.x20_behavior=sample&1};
        Fighter initial=fighter;ftColl_CreateReflectHit(&object,&shape,NULL);Fighter expected=fighter;fighter=initial;NativeScriptValue out={.type=NativeScriptType_Void};CHECK(nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_ReflectorCreate,args,9,&out,&diagnostic));CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));
        CHECK(nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_ReflectorDisable,NULL,0,&out,&diagnostic));CHECK(!fighter.reflecting);CHECK(nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_ReflectorRetain,NULL,0,&out,&diagnostic));CHECK(fighter.reflecting);fighter.reflect_hit.skip_update_pos=true;CHECK(nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_ReflectorUpdate,NULL,0,&out,&diagnostic));CHECK(!fighter.reflect_hit.skip_update_pos);
        NativeScriptValue wind[]={integer(sample%3),floating(3),{.type=NativeScriptType_I32,.as.i64=120},floating(.9f),floating(.02f),floating(1.0471976f)};
        CHECK(nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_EffectsWind,wind,6,&out,&diagnostic));CHECK(wind_position.x==joints[source.assets.part_to_joint[sample%3]].translate.x-3&&wind_frames==120&&wind_values[0]==.9f&&wind_values[1]==.02f&&wind_values[2]==1.0471976f);
    }
    for(unsigned indexed=0;indexed<2;++indexed)for(unsigned mode=0;mode<2;++mode)for(unsigned bone=0;bone<3;++bone){setup();model.x4_bone_count=indexed?3:0;context.source=&item_object;context.source_class=NativeCallbackSource_Item;context.generation=18;
        item_spawn_effect_on_bone(&item_object,1147,bone,mode);HSD_JObj* expected_joint=effect_joint;effect_joint=NULL;
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=1147},integer(bone),integer(mode)},out={.type=NativeScriptType_Void};CHECK(nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_ItemSpawnEffectOnBoneMode,args,3,&out,&diagnostic));CHECK(effect_id==1147&&effect_joint==expected_joint&&async_mode==mode);
    }
    setup();NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=1276},integer(1)},out={.type=NativeScriptType_Void};CHECK(nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_EffectsSpawn,args,2,&out,&diagnostic));CHECK(effect_joint==&joints[source.assets.part_to_joint[1]]&&effect_id==1276);
    setup();args[0].as.i64=1146;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(!nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_EffectsSpawn,args,2,&out,&diagnostic));
    CHECK(effect_calls==0&&out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    NativeScriptValue scalar_args[]={args[0],integer(1),floating(-0.375f)};
    CHECK(nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_EffectsSpawnWithScalar,scalar_args,3,&out,&diagnostic));
    CHECK(effect_calls==1&&effect_id==1146&&effect_scalar==-0.375f&&effect_joint==&joints[source.assets.part_to_joint[1]]);
    setup();args[0].as.i64=1276;retire=true;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_EffectsSpawn,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    // Exact source ownership is required even for field-only reflector controls.
    for(unsigned mismatch=0;mismatch<4;++mismatch){setup();
        if(mismatch==0)source.ready=false;
        else if(mismatch==1)storage.live=false;
        else if(mismatch==2)source.assets.parts.part_to_joint=NULL;
        else context.scene_epoch+=UINT64_C(0x100000000);
        out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingFeedback_Invoke(&context,NativeScriptBinding_ReflectorRetain,NULL,0,&out,&diagnostic));
        CHECK(!fighter.reflecting&&effect_calls==0&&out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);
    }
    printf("{\"status\":\"PASS actual feedback bodies\",\"checks\":%u}\n",checks);return 0;
}
