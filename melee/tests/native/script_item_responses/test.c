#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_article_lifecycle.h>
#include <melee/ft/native_script_binding_item.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/itmaplib.h>
#include <melee/mp/forward.h>
#include <sysdolphin/baselib/jobj.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/it/it_3F14.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum { N=4096,I=16384,F=128,V=256 };
typedef struct Buffers {
    char source[32768];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
    NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
    NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];
    NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
    NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} Buffers;
NativeSourcePackage target_packages[3];static Buffers buffers[4];static NativeScriptCompileBank scratch;
static Article article;static ItemAttr attributes;static ItemModelDesc model;
static Fighter fighters[128],alternate;HSD_JObjInfo hsdJObj;static HSD_JObj roots[64];static Item items[64];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static unsigned checks,cases,vm_calls,fighter_count,item_count,probe_mode,probe_calls;
static HSD_GObj *caller,*recipient,*replacement;static unsigned relation_mode,relation_calls;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"exports %u: %s (%s)\n",__LINE__,#x,diagnostic.message);fprintf(stderr,"diagnostic at %u:%u offset%u\n",diagnostic.source.line,diagnostic.source.column,diagnostic.source.offset);exit(1);}}while(0)
static void configure(NativeScriptCompileBank* bank,Buffers* b)
{
    *bank=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=sizeof(b->source),.tree={.nodes=b->nodes,.capacity=N},
        .expressions={.nodes=b->ir,.node_capacity=I,.arguments=b->arguments,.argument_capacity=I},
        .declarations={.functions=b->functions,.function_capacity=F,.parameters=b->parameters,.parameter_capacity=V,.values=b->values,.value_capacity=V},
        .locals=b->locals,.local_capacity=V,.instructions=b->instructions,.instruction_capacity=I,.function_code=b->codes,.function_code_capacity=F,
        .execution_functions=b->entries,.execution_function_capacity=F,.graph=b->graph,.graph_capacity=F,
        .storage_layout={.fields=b->fields,.field_capacity=V,.owners=b->owners,.owner_capacity=V},.parameter_slots=b->slots,.parameter_slot_capacity=V};
}

static void prepare(void)
{
 NativeSourcePackage* p=&target_packages[0];NativeScriptCompileBank* bank=&p->compiler.bank;configure(bank,&buffers[0]);
 const char* source="fighter:\n    behavior independent\nstate int unused\naction Placeholder:\n    on tick:\n        pass\narticle Probe states 1:\n    state int mask\n    state int tail\n    state bool same\n    on signal Bounce:\n        item.bounce()\n        item.state.tail = item.state.tail + 1\n    on signal Stage:\n        item.respond_stage_mask(item.state.mask)\n        item.state.tail = item.state.tail + 1\n    on signal Fault:\n        item.state.tail = 1 / item.state.tail\n    on interaction:\n        item.state.same = item.attachment_owner_is(other)\n        item.state.tail = item.state.tail + 1\n    state 0 anim -1:\n";
 article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
 NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
 const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(bank,&request,&accepted,&diagnostic));
 CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,bank,&layout,&metadata,&p->program,&diagnostic));
 p->address=p;p->target=Ft_Kind_CustomFirst;p->ready=p->assets.ready=true;p->assets.facts.moves=layout;p->assets.parts.parts_num=4;p->assets.facts.joints=4;
 tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=p->assets.motions;nativeScriptFighter_PublishProgram(p->target,&p->program);
 HSD_ClassInfo* cls=HSD_CLASS_INFO(&hsdJObj);cls->head.flags=1;cls->head.obj_size=sizeof(HSD_JObj);
}
static void compile_rejections(void)
{
 const char* invalid[]={"item.respond_stage_mask(1.5)","item.attachment_owner_is(1)","item.bounce(1)"};
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 for(unsigned n=0;n<4;++n){char source[512];if(n<3)snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle Probe states 1:\n    on created:\n        %s\n    state 0 anim -1:\n",invalid[n]);else snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc Bad():\n    item.bounce()\n");
  configure(&scratch,&buffers[3]);NativeScriptCompileRequest r={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=&target_packages[0].compiler.bank;
  CHECK(!nativeScriptCompile_Build(&scratch,&r,&accepted,&diagnostic));CHECK(accepted==&target_packages[0].compiler.bank);++cases;
 }
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(unsigned bank,uint32_t id)
{
    CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
    Fighter* f=&fighters[fighter_count++];NativeSourcePackage* p=&target_packages[bank];
    *f=(Fighter){.gobj=o,.kind=p->target,.behavior_kind=p->target,.x8_spawnNum=id,.ft_data=&p->assets.character.data};
    GObj_InitUserData(o,4,fighter_remove,f);CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* foreign_fighter(void)
{
 CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];*f=(Fighter){.gobj=o,.kind=Ft_Kind_Link,.behavior_kind=Ft_Kind_Link,.x8_spawnNum=999,.x34_scale={1,0.5f,1}};GObj_InitUserData(o,4,fighter_remove,f);return o;
}
static HSD_GObj* item(Fighter_GObj* creator)
{
    CHECK(item_count<64);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(target_packages[0].target)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(o);Item* i=&items[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=1000+item_count,.xC4_article_data=&article,.xCC_item_attr=&attributes};
    GObj_InitUserData(o,6,item_remove,i);nativeItem_CaptureCreator(o,creator);CHECK(nativeScriptItem_Attach(o,p));o->obj_kind=HSD_GObj_JObjKind;o->hsd_obj=&roots[item_count-1];roots[item_count-1].object.parent.class_info=HSD_CLASS_INFO(&hsdJObj);return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?(uint32_t)((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    const NativeScriptProgramIndex* index=view.program;
    for(unsigned n=0;n<index->value_count;++n)if(index->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(index->tree,index->values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));
        CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static void set(HSD_GObj* o,const char* name,int32_t value){NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=value};field(o,name,&v);}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?(uint32_t)((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
 return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};}

static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
void target_guard_probe(NativeScriptRuntimeContext* c){(void)c;}
void response_kernel_probe(Item_GObj* object)
{
 if(!probe_mode||object!=caller)return;unsigned mode=probe_mode;probe_mode=0;++probe_calls;
 if(mode==1)((Item*)object->user_data)->x1C++;
 if(mode==2){object->hsd_obj=NULL;object->obj_kind=HSD_GOBJ_OBJ_NONE;HSD_GObjFree(object);}
 if(mode==3)nativeScriptItem_RunHook(object,NativeScriptItemHook_Destroyed,NULL,0,NULL);
 if(mode==4){NativeScriptValue out;CHECK(nativeScriptItem_RunSignal(object,hash("Fault"),0,&out)==NativeScriptVM_Error);}
 if(mode==5)object->hsd_obj=&roots[63];
 if(mode==6)++target_packages[0].compiler.bank.storage_parameters.identity;
 if(mode==7)object->classifier=HSD_GOBJ_CLASS_FIGHTER;
 if(mode==8)((Fighter*)recipient->user_data)->x8_spawnNum++;
 if(mode==9)it_804D6D28=NULL;
 if(mode==10){object->hsd_obj=NULL;object->obj_kind=HSD_GOBJ_OBJ_NONE;HSD_GObjFree(object);replacement=item(NULL);}
 if(mode==11){alternate=*(Fighter*)recipient->user_data;recipient->user_data=&alternate;}

}
void response_relation_probe(Item_GObj* object)
{
 if(object!=caller||!relation_mode||++relation_calls!=2)return;unsigned mode=relation_mode;relation_mode=0;
 if(mode==1)HSD_GObjFree(recipient);
 if(mode==2){HSD_GObjFree(recipient);replacement=fighter(0,987);}
 if(mode==3)((Fighter*)recipient->user_data)->x8_spawnNum++;
 if(mode==4){alternate=*(Fighter*)recipient->user_data;recipient->user_data=&alternate;}
 if(mode==5)recipient->classifier=HSD_GOBJ_CLASS_ITEM;
 if(mode==6)++((Item*)object->user_data)->x1C;
}
static void close_item(HSD_GObj* object){object->hsd_obj=NULL;object->obj_kind=HSD_GOBJ_OBJ_NONE;HSD_GObjFree(object);}
static void reset_events(void){response_sound=response_effect=response_damage=response_geometry=0;response_order=0;}
static void seed(Item* i)
{
 i->x40_vel=(Vec3){-3.25f,4.5f,-0.0f};i->pos=(Vec3){1,2,3};i->xDCF_flag.b0=false;i->xDCD_flag.b2=false;
 i->x378_itemColl=(CollData){.facing_dir=1};i->x378_itemColl.left_facing_wall.normal=(Vec3){1,0,0};i->x378_itemColl.right_facing_wall.normal=(Vec3){-1,0,0};i->x378_itemColl.ceiling.normal=(Vec3){0,-1,0};i->x378_itemColl.floor.normal=(Vec3){0,1,0};
 i->x378_itemColl.ecb.top=(Vec2){0,3};i->x378_itemColl.ecb.bottom=(Vec2){0,-3};i->x378_itemColl.ecb.right=(Vec2){2,0};i->x378_itemColl.ecb.left=(Vec2){-2,0};
 for(unsigned n=0;n<4;++n)i->x5D4_hitboxes[n].hit=(HitCapsule){.state=HitCapsule_Enabled,.damage=5.5f+n};
}
int main(void)
{
 prepare();compile_rejections();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 HSD_GObj* owner=fighter(0,123),*source=item(owner);Item* data=source->user_data;static Item reference;HSD_GObj raw={.classifier=HSD_GOBJ_CLASS_ITEM,.user_data=&reference};
 response_common.x58_float=-0.75f;response_common.x5C_float=0.25f;response_common.x60_float=1.5f;attributes.x58=0.5f;
 const float inputs[]={-5.25f,-0.0f,0.0f,1.5f,100.25f};NativeScriptValue out;
 for(unsigned x=0;x<5;++x)for(unsigned y=0;y<5;++y){data->x40_vel=(Vec3){inputs[x],inputs[y],7.0f};reference=*data;
  response_reference_bounce(&raw);NativeScriptRuntimeContext c=context(source);CHECK(nativeScriptBindingItem_Invoke(&c,NativeScriptBinding_ItemBounceVictim,NULL,0,&out,&diagnostic));
  CHECK(!memcmp(&data->x40_vel,&reference.x40_vel,sizeof(Vec3)));++cases;
 }
 const uint32_t envs[]={0,Collide_LeftWallMask,Collide_RightWallMask,Collide_CeilingMask,Collide_FloorMask,Collide_LeftWallMask|Collide_RightWallMask};
 for(unsigned e=0;e<6;++e)for(unsigned previous=0;previous<6;++previous)for(unsigned mask=0;mask<16;++mask){seed(data);data->x378_itemColl.env_flags=envs[e];data->x378_itemColl.prev_env_flags=envs[previous];reference=*data;reference.owner=NULL;data->owner=NULL;
  reset_events();response_reference_stage(&raw,(s32)mask);unsigned sound=response_sound,effect=response_effect,damage=response_damage,geometry=response_geometry;uint32_t order=response_order;
  reset_events();NativeScriptRuntimeContext c=context(source);NativeScriptValue input={.type=NativeScriptType_U32,.as.u64=mask};CHECK(nativeScriptBindingItem_Invoke(&c,NativeScriptBinding_ItemRespondStageMask,&input,1,&out,&diagnostic));
  CHECK(!memcmp(&data->x40_vel,&reference.x40_vel,sizeof(Vec3))&&!memcmp(&data->pos,&reference.pos,sizeof(Vec3))&&!memcmp(&data->xC1C,&reference.xC1C,sizeof(itECB)));
  CHECK(response_sound==sound&&response_effect==effect&&response_damage==damage&&response_geometry==geometry&&response_order==order);
  for(unsigned n=0;n<4;++n)CHECK(data->x5D4_hitboxes[n].hit.damage==reference.x5D4_hitboxes[n].hit.damage);++cases;
 }
 CHECK(nativeScriptItem_Creator(source)==owner&&nativeScriptItem_AttachmentOwner(source)==owner);
 NativeScriptValue object;CHECK(nativeScriptReceiver_CaptureExternal(owner,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&object,&diagnostic));
 CHECK(nativeScriptItem_RunHook(source,NativeScriptItemHook_Interaction,&object,1,&out)==NativeScriptVM_Ok);++vm_calls;CHECK(field(source,"same",NULL).as.u64);++cases;
 HSD_GObj* foreign=foreign_fighter();NativeScriptValue foreign_token;CHECK(nativeScriptReceiver_CaptureExternal(foreign,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&foreign_token,&diagnostic));data->owner=foreign;
 CHECK(nativeScriptItem_RunHook(source,NativeScriptItemHook_Interaction,&foreign_token,1,&out)==NativeScriptVM_Ok);++vm_calls;CHECK(!field(source,"same",NULL).as.u64&&nativeScriptItem_Creator(source)==owner);++cases;
 CHECK(nativeScriptItem_SetAttachmentOwner(source,foreign));CHECK(nativeScriptItem_RunHook(source,NativeScriptItemHook_Interaction,&foreign_token,1,&out)==NativeScriptVM_Ok);++vm_calls;CHECK(field(source,"same",NULL).as.u64&&nativeScriptItem_Creator(source)==owner);++cases;
 seed(data);reference=*data;reset_events();response_reference_stage(&raw,1);NativeScriptRuntimeContext foreign_source=context(source);NativeScriptValue foreign_mask={.type=NativeScriptType_U32,.as.u64=1};CHECK(nativeScriptBindingItem_Invoke(&foreign_source,NativeScriptBinding_ItemRespondStageMask,&foreign_mask,1,&out,&diagnostic));
 for(unsigned n=0;n<4;++n)CHECK(data->x5D4_hitboxes[n].hit.damage==reference.x5D4_hitboxes[n].hit.damage&&data->x5D4_hitboxes[n].hit.unk_count==reference.x5D4_hitboxes[n].hit.unk_count);++cases;
 data->owner=source;seed(data);reference=*data;response_reference_stage(&raw,1);CHECK(nativeScriptBindingItem_Invoke(&foreign_source,NativeScriptBinding_ItemRespondStageMask,&foreign_mask,1,&out,&diagnostic));
 for(unsigned n=0;n<4;++n)CHECK(data->x5D4_hitboxes[n].hit.damage==reference.x5D4_hitboxes[n].hit.damage);++cases;data->owner=NULL;
 CHECK(nativeScriptItem_SetAttachmentOwner(source,owner));
 CHECK(nativeScriptItem_ClearAttachmentOwner(source));CHECK(nativeScriptItem_RunHook(source,NativeScriptItemHook_Interaction,&object,1,&out)==NativeScriptVM_Ok);++vm_calls;CHECK(!field(source,"same",NULL).as.u64&&nativeScriptItem_Creator(source)==owner);++cases;
 for(unsigned stage=0;stage<2;++stage){seed(data);NativeScriptValue mask={.type=NativeScriptType_U32,.as.u64=0};set(source,"mask",0);
  CHECK(nativeScriptItem_RunSignal(source,hash(stage?"Stage":"Bounce"),0,&out)==NativeScriptVM_Ok);++vm_calls;++cases;(void)mask;}

 NativeScriptRuntimeContext c=context(source);
 const NativeScriptValue bad[]={{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)},{.type=NativeScriptType_I64,.as.i64=INT64_MIN},{.type=NativeScriptType_Bool,.as.u64=2}};
 for(unsigned n=0;n<4;++n){Vec3 velocity=data->x40_vel;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingItem_Invoke(&c,NativeScriptBinding_ItemRespondStageMask,&bad[n],1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(&velocity,&data->x40_vel,sizeof(velocity)));++cases;}
 out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};it_804D6D28=NULL;CHECK(!nativeScriptBindingItem_Invoke(&c,NativeScriptBinding_ItemBounceVictim,NULL,0,&out,&diagnostic));CHECK(out.as.u64==99);it_804D6D28=&response_common;++cases;
 NativeScriptValue mask={.type=NativeScriptType_I32,.as.i64=-1};seed(data);data->native_removal_in_progress=true;CHECK(nativeScriptBindingItem_Invoke(&c,NativeScriptBinding_ItemRespondStageMask,&mask,1,&out,&diagnostic));data->native_removal_in_progress=false;++cases;
 for(unsigned n=0;n<4;++n){NativeScriptValue input=n==0?(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=123}:object;if(n==1)input.as.object.kind=0;if(n==2)input.as.object.identity=UINT32_MAX;if(n==3)input.as.object.generation++;
  out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingArticleLifecycle_Invoke(&c,NativeScriptBinding_ItemAttachmentOwnerIs,&input,1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;}
 NativeScriptValue other_item;CHECK(nativeScriptReceiver_CaptureExternal(source,NativeCallbackSource_Item,nativeGObjPool.lifetime,&other_item,&diagnostic));CHECK(nativeScriptBindingArticleLifecycle_Invoke(&c,NativeScriptBinding_ItemAttachmentOwnerIs,&other_item,1,&out,&diagnostic)&&!out.as.u64);++cases;
 NativeScriptValue null_object={.type=NativeScriptType_Object,.as.object.kind=NATIVE_SCRIPT_RECEIVER_ENGINE_KIND|NativeCallbackSource_Fighter};CHECK(nativeScriptBindingArticleLifecycle_Invoke(&c,NativeScriptBinding_ItemAttachmentOwnerIs,&null_object,1,&out,&diagnostic)&&!out.as.u64);++cases;
 // Separate retained records export the same physical target; token equality
 // must not replace the actual native attachment relation comparison.
 CHECK(nativeScriptItem_SetAttachmentOwner(source,owner));NativeScriptReceiverPool* pool=nativeScriptRuntime_Receivers();NativeScriptReceiverFrameHandle frame;
 CHECK(nativeScriptReceiver_Push(pool,&c,nativeGObjPool.lifetime,&target_packages[0].compiler.bank.declarations,Ft_Kind_CustomFirst,nativeScriptActors_Lookup,NULL,NULL,&frame,&diagnostic));NativeScriptReceiverScope scope={pool,frame};NativeScriptValue left,right,exported;
 CHECK(nativeScriptReceiver_Import(&scope,&c,&object,nativeGObjPool.lifetime,&left,&diagnostic));
 CHECK(nativeScriptReceiver_Export(&scope,&c,&left,&exported,&diagnostic));CHECK(nativeScriptBindingArticleLifecycle_Invoke(&c,NativeScriptBinding_ItemAttachmentOwnerIs,&exported,1,&out,&diagnostic)&&out.as.u64);
 NativeScriptReceiverFrameHandle nested;CHECK(nativeScriptReceiver_Push(pool,&c,nativeGObjPool.lifetime,&target_packages[0].compiler.bank.declarations,Ft_Kind_CustomFirst,nativeScriptActors_Lookup,NULL,NULL,&nested,&diagnostic));NativeScriptReceiverScope second={pool,nested};
 CHECK(nativeScriptReceiver_Import(&second,&c,&object,nativeGObjPool.lifetime,&right,&diagnostic));CHECK(left.as.object.identity!=right.as.object.identity);
 CHECK(nativeScriptReceiver_Export(&second,&c,&right,&exported,&diagnostic));CHECK(nativeScriptBindingArticleLifecycle_Invoke(&c,NativeScriptBinding_ItemAttachmentOwnerIs,&exported,1,&out,&diagnostic)&&out.as.u64);CHECK(nativeScriptReceiver_Pop(pool,nested,&diagnostic));CHECK(nativeScriptReceiver_Pop(pool,frame,&diagnostic));++cases;
 // A queued removal keeps the owned view valid until Destroyed completes.
 data->native_removal_in_progress=true;
 CHECK(nativeScriptBindingArticleLifecycle_Invoke(&c,NativeScriptBinding_ItemAttachmentOwnerIs,&object,1,&out,&diagnostic)&&out.as.u64);
 CHECK(nativeScriptItem_RunHook(source,NativeScriptItemHook_Interaction,&object,1,&out)==NativeScriptVM_Ok);++vm_calls;CHECK(field(source,"same",NULL).as.u64);++cases;
 CHECK(nativeScriptItem_RunHook(source,NativeScriptItemHook_Destroyed,NULL,0,&out)==NativeScriptVM_Ok);++vm_calls;
 out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingArticleLifecycle_Invoke(&c,NativeScriptBinding_ItemAttachmentOwnerIs,&object,1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 close_item(source);HSD_GObjFree(owner);HSD_GObjFree(foreign);
 for(unsigned stage=0;stage<2;++stage)for(unsigned mode=1;mode<=11;++mode){if(!stage&&(mode==8||mode==11))continue;if(stage&&mode==9)continue;
  recipient=fighter(0,700+fighter_count);caller=item(NULL);Item* d=caller->user_data;seed(d);d->owner=recipient;((Fighter*)recipient->user_data)->x34_scale.y=1.0f;
  uint64_t parameters=target_packages[0].compiler.bank.storage_parameters.identity;probe_mode=mode;replacement=NULL;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptItem_RunSignal(caller,hash(stage?"Stage":"Bounce"),0,&out)!=NativeScriptVM_Ok);++vm_calls;CHECK(!probe_mode&&out.type==NativeScriptType_U64&&out.as.u64==99);CHECK(nativeScriptRuntime_Idle());
  target_packages[0].compiler.bank.storage_parameters.identity=parameters;it_804D6D28=&response_common;
  if(mode==10){CHECK(replacement==caller);close_item(replacement);}else if(mode!=2)close_item(caller);HSD_GObjFree(recipient);++cases;
 }
 for(unsigned mode=1;mode<=6;++mode){recipient=fighter(0,987);caller=item(recipient);NativeScriptRuntimeContext cc=context(caller);CHECK(nativeScriptReceiver_CaptureExternal(recipient,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&object,&diagnostic));relation_mode=mode;relation_calls=0;replacement=NULL;
  out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingArticleLifecycle_Invoke(&cc,NativeScriptBinding_ItemAttachmentOwnerIs,&object,1,&out,&diagnostic));CHECK(!relation_mode&&out.type==NativeScriptType_U64&&out.as.u64==99);
  if(mode==2){CHECK(replacement==recipient);HSD_GObjFree(replacement);}else if(mode!=1)HSD_GObjFree(recipient);close_item(caller);++cases;
 }
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS Item responses\",\"checks\":%u,\"cases\":%u,\"vm_calls\":%u}\n",checks,cases,vm_calls);return 0;
}
