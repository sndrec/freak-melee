#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_article_combat.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/itcoll.h>
#include <melee/it/it_2725.h>
#include <melee/mp/forward.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage combat_package;StaleMoveTable combat_stale;
#define bank combat_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static Fighter fighters[32],alternate_fighter;static Item items[128],alternate_item;static HSD_JObj joints[4];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned combat_probe_calls,combat_mutation;HSD_GObj* combat_source,*combat_owner;HSD_GObj*(*combat_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons;static NativeScriptDiagnostic diagnostic;static HSD_GObj* replacement;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"Article combat %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="fighter:\n    behavior independent\narticle Probe states 1:\n    state bool enabled\n    state float radius\n    state bool floor\n    state bool ceiling\n    state bool left\n    state bool right\n    on signal Inspect:\n        item.state.enabled = hitbox.enabled(0)\n        item.state.radius = hitbox.radius(0)\n        item.state.floor = collision.floor_contact\n        item.state.ceiling = collision.ceiling_contact\n        item.state.left = collision.left_wall\n        item.state.right = collision.right_wall\n    on signal Edit:\n        hitbox.set_damage(1, 12.75)\n        hitbox.set_radius(1, 2.5)\n    on signal Clear:\n        hitboxes.disable_all()\n    on signal Single:\n        item.disable_hitbox(1)\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Zelda_DinFire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,NULL,&metadata,&combat_package.program,&diagnostic));
    combat_package.address=&combat_package;combat_package.target=Ft_Kind_CustomFirst;combat_package.ready=combat_package.assets.ready=true;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&combat_package.program);
    for(unsigned i=0;i<4;++i)joints[i].translate=(Vec3){10+(float)i,20+(float)i,30+(float)i};
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(void)
{
    CHECK(fighter_count<32);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=Ft_Kind_Link,.behavior_kind=Ft_Kind_Link,.x8_spawnNum=20+fighter_count,.player_id=2};f->x34_scale.y=1;GObj_InitUserData(o,4,fighter_remove,f);return o;
}
static HSD_GObj* item(void)
{
    CHECK(item_count<128);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100+item_count,.xC4_article_data=&article};
    GObj_InitUserData(o,6,item_remove,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static void signal(HSD_GObj* o,const char* name){NativeScriptValue out;CHECK(nativeScriptItem_RunSignal(o,hash(name),0,&out)==NativeScriptVM_Ok);}
static NativeScriptValue field(HSD_GObj* o,const char* name)
{
    NativeScriptReceiverView v;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&v));
    for(unsigned n=0;n<bank.declarations.value_count;++n)if(values[n].owner==v.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
        NativeScriptValue out;CHECK(nativeScriptStorage_Read(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&out,&diagnostic));return out;
    }CHECK(false);return(NativeScriptValue){0};
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Item,.generation=((Item*)o->user_data)->x1C,.actor_id=((Item*)o->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};}
static bool invoke(HSD_GObj* o,uint32_t id,const NativeScriptValue* a,uint32_t n,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(o);return nativeScriptBindingArticleCombat_Invoke(&c,id,a,n,out,&diagnostic);}
static uint32_t bits(float f){uint32_t u;memcpy(&u,&f,4);return u;}
static void initialize(Item* i,unsigned pattern)
{
    for(unsigned h=0;h<4;++h)i->x5D4_hitboxes[h].hit=(HitCapsule){.state=(HitCapsuleState)((pattern+h)%5),.damage=3.25f,.scale=4.5f,.unk_count=3,.jobj=&joints[h],.b_offset={1,2,3},.x4C={4,5,6},.x58={7,8,9}};
    i->xDAA_flag.b2=1;i->xDC8_word.flags.x16=1;
}
static HSD_GObj* reuse(void){replacement=item();return replacement;}
void combat_native_completed(HSD_GObj* source)
{
    if(source!=combat_source||!combat_mutation)return;++combat_probe_calls;
    unsigned mode=combat_mutation;combat_mutation=0;
    if(mode==1){NativeScriptValue out;CHECK(nativeScriptItem_RunHook(combat_source,NativeScriptItemHook_Destroyed,NULL,0,&out)==NativeScriptVM_Ok);}
    if(mode==2)HSD_GObjFree(combat_source);
    if(mode==3){HSD_GObjFree(combat_source);if(combat_reuse)combat_reuse();}
    if(mode==4){alternate_item=*(Item*)combat_source->user_data;combat_source->user_data=&alternate_item;}
    if(mode==5)((Item*)combat_source->user_data)->x1C++;
    if(mode==6){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==combat_source)nativeScriptItemActors[i].faulted=true;}
    if(mode==7)HSD_GObjFree(combat_owner);
    if(mode==8)((Fighter*)combat_owner->user_data)->kind=Ft_Kind_Zelda;
    if(mode==9){alternate_fighter=*(Fighter*)combat_owner->user_data;combat_owner->user_data=&alternate_fighter;}
}
int main(void)
{
    prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
    HSD_GObj* o=item();Item* data=o->user_data;initialize(data,1);data->x378_itemColl.env_flags=Collide_FloorMask|Collide_RightWallMask;signal(o,"Inspect");
    CHECK(field(o,"enabled").as.u64&&field(o,"radius").as.f32==4.5f&&field(o,"floor").as.u64&&field(o,"right").as.u64&&!field(o,"ceiling").as.u64&&!field(o,"left").as.u64);
    signal(o,"Edit");CHECK(data->x5D4_hitboxes[1].hit.unk_count==12&&data->x5D4_hitboxes[1].hit.damage==12&&data->x5D4_hitboxes[1].hit.scale==2.5f);
    signal(o,"Single");CHECK(data->x5D4_hitboxes[1].hit.state==HitCapsule_Disabled);signal(o,"Clear");for(unsigned h=0;h<4;++h)CHECK(data->x5D4_hitboxes[h].hit.state==HitCapsule_Disabled);CHECK(!data->xDC8_word.flags.x16&&!data->xDAA_flag.b2);
    NativeScriptValue a[2]={{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_F32,.as.f32=1}},out;
    NativeScriptRuntimeContext c=context(o);const uint32_t ids[]={NativeScriptBinding_CollisionFloorContact,NativeScriptBinding_CollisionCeilingContact,NativeScriptBinding_CollisionLeftWall,NativeScriptBinding_CollisionRightWall};const uint32_t masks[]={Collide_FloorMask,Collide_CeilingMask,Collide_LeftWallMask,Collide_RightWallMask};
    for(unsigned flags=0;flags<65536;++flags){data->x378_itemColl.env_flags=flags;for(unsigned p=0;p<4;++p)CHECK(nativeScriptBindingArticleCombat_Read(&c,ids[p],NULL,0,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==((flags&masks[p])!=0));}
    for(unsigned state=0;state<5;++state){data->x5D4_hitboxes[0].hit.state=(HitCapsuleState)state;CHECK(invoke(o,NativeScriptBinding_HitboxEnabled,a,1,&out)&&out.as.u64==(state!=HitCapsule_Disabled));}
    static const float radii[]={0,-0.0f,1.25f,-1,NAN,INFINITY};
    for(unsigned r=0;r<6;++r){data->x5D4_hitboxes[0].hit.state=HitCapsule_Disabled;data->x5D4_hitboxes[0].hit.scale=3.5f;a[1].as.f32=radii[r];CHECK(invoke(o,NativeScriptBinding_HitboxSetRadius,a,2,&out));CHECK(bits(data->x5D4_hitboxes[0].hit.scale)==bits(r<3?radii[r]:3.5f));CHECK(invoke(o,NativeScriptBinding_HitboxRadius,a,1,&out)&&bits(out.as.f32)==bits(data->x5D4_hitboxes[0].hit.scale));}
    HSD_GObj* owner=fighter();Fighter* fp=owner->user_data;data->owner=owner;data->xD88_attackID=4;data->xD8C_attack_instance=7;
    combat_stale.current_index=2;combat_stale.StaleMoves[1].move_id=4;combat_stale.StaleMoves[0].move_id=4;
    static const float scales[]={0.5f,1,2,3};
    for(unsigned n=0;n<64;++n){uint32_t damage=n*5;fp->x34_scale.y=scales[n%4];a[0].as.i64=n%4;a[1]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=(double)damage+0.75};
        HitCapsule expected={0};it_80272460(&expected,damage,o);CHECK(invoke(o,NativeScriptBinding_HitboxSetDamage,a,2,&out));HitCapsule* got=&data->x5D4_hitboxes[n%4].hit;
        CHECK(got->unk_count==expected.unk_count&&bits(got->damage)==bits(expected.damage));++comparisons;
    }
    // Defined Inf scale with zero common factor is accepted, not sanitized.
    float factor=Fighter_804D6524->x4;Fighter_804D6524->x4=0;fp->x34_scale.y=INFINITY;a[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=10};CHECK(invoke(o,NativeScriptBinding_HitboxSetDamage,a,2,&out));Fighter_804D6524->x4=factor;fp->x34_scale.y=1;
    data->owner=NULL;static const NativeScriptValue amounts[]={{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_U64,.as.u64=UINT64_MAX},{.type=NativeScriptType_F64,.as.f64=-0.75},{.type=NativeScriptType_F32,.as.f32=12.75f}};
    static const uint32_t damages[]={UINT32_MAX,UINT32_MAX,0,12};a[0].as.i64=0;
    for(unsigned n=0;n<4;++n){a[1]=amounts[n];CHECK(invoke(o,NativeScriptBinding_HitboxSetDamage,a,2,&out)&&data->x5D4_hitboxes[0].hit.unk_count==damages[n]);}
    for(unsigned n=0;n<3;++n){a[1]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=n==0?NAN:n==1?-1.0:4294967296.0};out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};uint32_t before=data->x5D4_hitboxes[0].hit.unk_count;CHECK(!invoke(o,NativeScriptBinding_HitboxSetDamage,a,2,&out)&&out.type==NativeScriptType_U64&&out.as.u64==99&&data->x5D4_hitboxes[0].hit.unk_count==before);}
    data->owner=owner;a[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=12};
    for(unsigned bad=0;bad<3;++bad){fp->x34_scale.y=bad==0?0.0f:bad==1?INFINITY:2.0f;struct Fighter_804D6524_t* saved_scale=Fighter_804D6524;if(bad==2)Fighter_804D6524=NULL;
        uint32_t before=data->x5D4_hitboxes[0].hit.unk_count;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(o,NativeScriptBinding_HitboxSetDamage,a,2,&out)&&out.as.u64==99&&data->x5D4_hitboxes[0].hit.unk_count==before);Fighter_804D6524=saved_scale;
    }fp->x34_scale.y=1;data->owner=NULL;
    a[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(o,NativeScriptBinding_ItemDisableHitbox,a,1,&out)&&out.as.u64==99);
    a[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};CHECK(!invoke(o,NativeScriptBinding_HitboxEnabled,a,1,&out)&&out.as.u64==99);
    a[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=0};a[1]=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=1};CHECK(!invoke(o,NativeScriptBinding_HitboxSetRadius,a,2,&out)&&out.as.u64==99);
    NativeScriptRuntimeContext invalid=c;invalid.scene_epoch=0;CHECK(!nativeScriptBindingArticleCombat_Read(&invalid,NativeScriptBinding_CollisionFloorContact,NULL,0,&out,&diagnostic)&&out.as.u64==99);
    invalid=c;invalid.source_class=NativeCallbackSource_Fighter;CHECK(!nativeScriptBindingArticleCombat_Read(&invalid,NativeScriptBinding_CollisionFloorContact,NULL,0,&out,&diagnostic)&&out.as.u64==99);
    HSD_GObj* faulted=item();for(unsigned slot=0;slot<NATIVE_SCRIPT_ITEM_ACTORS;++slot)if(nativeScriptItemActors[slot].active&&nativeScriptItemActors[slot].object==faulted)nativeScriptItemActors[slot].faulted=true;
    CHECK(!invoke(faulted,NativeScriptBinding_HitboxesDisableAll,NULL,0,&out)&&out.as.u64==99);HSD_GObjFree(faulted);
    for(unsigned n=0;n<20;++n){HSD_GObj* s=item(),*r=item();Item* scripted=s->user_data,*reference=r->user_data;initialize(scripted,n);initialize(reference,n);a[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=n%4};
        if(n<16){CHECK(invoke(s,NativeScriptBinding_ItemDisableHitbox,a,1,&out));it_80272560(r,n%4);}else{CHECK(invoke(s,NativeScriptBinding_HitboxesDisableAll,NULL,0,&out));it_802725D4(r);}
        for(unsigned h=0;h<4;++h)CHECK(memcmp(&scripted->x5D4_hitboxes[h],&reference->x5D4_hitboxes[h],sizeof(scripted->x5D4_hitboxes[h]))==0);CHECK(scripted->xDAA_flag.b2==reference->xDAA_flag.b2&&scripted->xDC8_word.flags.x16==reference->xDC8_word.flags.x16);++comparisons;HSD_GObjFree(s);HSD_GObjFree(r);
    }
    for(unsigned n=0;n<3;++n){a[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=n==0?-1:n==1?4:INT32_MAX};CHECK(invoke(o,NativeScriptBinding_ItemDisableHitbox,a,1,&out));out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(o,NativeScriptBinding_HitboxRadius,a,1,&out)&&out.as.u64==99);}
    initialize(data,0);data->x5D4_hitboxes[0].hit.state=HitCapsule_Enabled;data->x5D4_hitboxes[0].hit.jobj=NULL;a[0].as.i64=0;CHECK(invoke(o,NativeScriptBinding_ItemDisableHitbox,a,1,&out));
    initialize(data,0);data->x5D4_hitboxes[1].hit.state=HitCapsule_Enabled;data->x5D4_hitboxes[1].hit.jobj=NULL;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(o,NativeScriptBinding_ItemDisableHitbox,a,1,&out)&&out.as.u64==99&&data->x5D4_hitboxes[0].hit.state==HitCapsule_Disabled);
    HSD_GObjFree(owner);
    for(unsigned mode=1;mode<=9;++mode){combat_source=item();combat_owner=fighter();Item* i=combat_source->user_data;i->owner=combat_owner;a[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=0};a[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=12};out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        combat_probe_calls=0;combat_mutation=mode;combat_reuse=mode==3?reuse:NULL;CHECK(!invoke(combat_source,NativeScriptBinding_HitboxSetDamage,a,2,&out)&&combat_mutation==0&&combat_probe_calls==1&&out.type==NativeScriptType_U64&&out.as.u64==99);
        if(mode==3){CHECK(replacement==combat_source);HSD_GObjFree(replacement);}else if(mode!=2)HSD_GObjFree(combat_source);if(mode!=7)HSD_GObjFree(combat_owner);combat_source=NULL;combat_reuse=NULL;
    }
    HSD_GObjFree(o);CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS Article combat\",\"checks\":%u,\"native_comparisons\":%u}\n",checks,comparisons);return 0;
}
