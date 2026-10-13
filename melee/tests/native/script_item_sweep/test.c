#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_binding_presentation.h>

#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_pool.h>

#include <melee/ft/ftdata.h>
#include <sysdolphin/baselib/gobjplink.h>




#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=1024,F=32,V=64};
static char text[16384];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N];static uint32_t arguments[N];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch call_graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
NativeScriptCompileBank presentation_bank;unsigned presentation_checks;static NativeSourcePackage source;
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/itgroundcoll.h>
#include <melee/mp/mplib.h>
#include <melee/ft/native_script_binding_item_collision.h>
#include <melee/ft/native_script_binding_stage.h>
#include <melee/mp/mpcoll.h>
static Article articles[3];static ItemAttr attributes[3];static ItemModelDesc models[3];
static NativeScriptDiagnostic diagnostic;static Item items[256];static unsigned item_count,checks,cases,pairs,vm_calls;
void presentation_check(bool ok,const char* expression,unsigned line)
{++checks;if(!ok){fprintf(stderr,"item sweep %u: %s (%u:%u %s)\n",line,expression,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return source.ready&&source.target==kind?&source:NULL;}
static void configure(void)
{
 source.compiler.bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=call_graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void prepare(void)
{
 const char* authored="fighter:\n    behavior independent\n"
 "func LineExists() -> bool:\n    return stage.line_exists(0)\n"
 "func NormalX() -> float:\n    let float x = 0\n    let float y = 0\n    let float z = 0\n    stage.line_normal(0, x, y, z)\n    return x\n"
 "article Probe states 1:\n    state float nx\n    state float ny\n    state float nz\n    state int line\n    state int flags\n    state bool hit\n    state int samples\n"
 "    func Previous() -> float:\n        item.state.samples = item.state.samples + 1\n        return 7\n"
 "    on picked_up:\n        item.sweep_segment(Previous(), 8, 9, item.state.nx, item.state.ny, item.state.nz, item.state.line, item.state.flags, item.state.hit)\n"
 "    on dropped:\n        let float nx = 12\n        let float ny = 13\n        let float nz = 14\n        let int line = -2\n        let int flags = -2147483648\n        let bool hit = false\n"
 "        item.sweep_segment(7, 8, 9, nx, ny, nz, line, flags, hit)\n"
 "        item.state.nx = nx\n        item.state.ny = ny\n        item.state.nz = nz\n        item.state.line = line\n        item.state.flags = flags\n        item.state.hit = hit\n"
 "    on signal Fault:\n        item.state.line = 1 / item.state.line\n"
 "    on spawned:\n        item.state.hit = stage.line_exists(item.state.line)\n        if item.state.hit:\n            stage.line_normal(item.state.line, item.state.nx, item.state.ny, item.state.nz)\n            item.bind_stage_line(item.state.line)\n            item.state.hit = item.read_stage_line_speed(item.state.line)\n"
 "    on destroyed:\n        item.sweep_segment(7, 8, 9, item.state.nx, item.state.ny, item.state.nz, item.state.line, item.state.flags, item.state.hit)\n    state 0 anim -1:\n"
 "article Second states 1:\n    state float nx\n    state float ny\n    state float nz\n    state int line\n    state int flags\n    state bool hit\n    on picked_up:\n        item.sweep_segment(7, 8, 9, item.state.nx, item.state.ny, item.state.nz, item.state.line, item.state.flags, item.state.hit)\n    state 0 anim -1:\n"
 "item Holdable damage 4 angle 45 lifetime 90:\n    on tick:\n        return\n";
 configure();for(unsigned i=0;i<3;++i)articles[i]=(Article){.x0_common_attr=&attributes[i],.x10_modelDesc=&models[i]};
 NativeScriptItemAsset assets[]={{.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&articles[0]},{.declaration="Second",.kind=It_Kind_GameWatch_Panic,.article=&articles[1]},{.declaration="Holdable",.kind=It_Kind_NativeHoldable,.article=&articles[2]}};
 NativeScriptItemMetadataContext metadata={.assets=assets,.asset_count=3};
 source.assets.facts.moves=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=authored,.source_bytes=(uint32_t)strlen(authored),.motion_layout=&source.assets.facts.moves,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
 const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&source.compiler.bank,&request,&accepted,&diagnostic));
 source.target=Ft_Kind_CustomFirst;CHECK(nativeScriptFighter_PrepareProgram(source.target,&source.compiler.bank,&source.assets.facts.moves,&metadata,&source.program,&diagnostic));
 source.address=&source;source.ready=source.assets.ready=true;CHECK(nativeScriptFighter_CanPublish(source.target));nativeScriptFighter_PublishProgram(source.target,&source.program);
 CHECK(nativeScriptFighter_Program(source.target)->items.count==3);
}
static void remove_item(void* data){nativeScriptItem_Detach(((Item*)data)->entity);}
static const NativeScriptItemProgram* spec(unsigned index){return &nativeScriptFighter_Program(source.target)->items.programs[index];}
static HSD_GObj* make(unsigned which)
{
 CHECK(item_count<256);const NativeScriptItemProgram* p=spec(which);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(o);Item* i=&items[item_count++];
 *i=(Item){.entity=o,.kind=p->kind,.x1C=1000+item_count,.msid=0,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.xC4_article_data=(Article*)p->article,.xCC_item_attr=p->article?p->article->x0_common_attr:NULL,.pos={1,-0.0f,3},.x40_vel={4,5,6}};
 if(p->recipe){i->xDD4_itemVar.native_holdable.owner_kind=p->author_kind;i->xDD4_itemVar.native_holdable.definition_index=p->recipe_index;}
 GObj_InitUserData(o,6,remove_item,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Item,.generation=((Item*)o->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue state(HSD_GObj* object,const char* name)
{
 NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,((Item*)object->user_data)->x1C,&view));
 for(unsigned n=0;n<view.program->value_count;++n)if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
  NativeScriptValue result;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
 }CHECK(false);return(NativeScriptValue){0};
}
static unsigned segments,mutation;
static HSD_GObj* sweep_source;
static bool collision_hit;
static Vec3 collision_position={101,102,103},collision_normal={-1,0.5f,-0.0f};
static int collision_line;
static u32 collision_flags;
static float observed[4];
// Independent box sweeps belong to the separate stage query cohort.
void mpColl_80041EE4(CollData* collision){(void)collision;abort();}
bool mpColl_8004730C(CollData* collision,ftCollisionBox* box){(void)collision;(void)box;abort();}
// Controlled stage-intersection leaf. The actual Item and map wrappers execute.
bool mpCheckMultiple(float x0,float y0,float x1,float y1,Vec3* p,int* line,u32* flags,Vec3* normal,u32 mask,int skip,int only)
{
 CHECK(p&&line&&flags&&normal&&mask==0x1f&&skip==-1&&only==-1);++segments;
 observed[0]=x0;observed[1]=y0;observed[2]=x1;observed[3]=y1;
 if(mutation){NativeScriptReceiverView view;CHECK(nativeScriptItem_View(sweep_source,((Item*)sweep_source->user_data)->x1C,&view));
  if(mutation==1)view.storage->live=false;
  if(mutation==2)++source.compiler.bank.storage_layout.identity;
  if(mutation==3)++source.compiler.bank.storage_parameters.identity;
  if(mutation==4)++view.storage->generation;
  if(mutation==5)++view.storage->owner;
  mutation=0;
 }
 if(collision_hit){*p=collision_position;*line=collision_line;*flags=collision_flags;*normal=collision_normal;}
 return collision_hit;
}
static void inputs(NativeScriptValue* a)
{
 const float initial[]={7,8,9,12,13,14};
 for(unsigned n=0;n<6;++n)a[n]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=initial[n]};
 a[6]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-2};
 a[7]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=INT32_MIN};
 a[8]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};
}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xabcdef123456789)};}
static void set_state(HSD_GObj* o,const char* name,NativeScriptValue value)
{
 NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&view));
 for(unsigned n=0;n<view.program->value_count;++n)if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
  CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&value,&diagnostic));return;
 }CHECK(false);
}
static void initialize_state(HSD_GObj* o,const NativeScriptValue* a)
{const char* names[]={"nx","ny","nz","line","flags","hit"};for(unsigned n=0;n<6;++n)set_state(o,names[n],a[3+n]);}
static void compare(void)
{
 const u32 masks[]={0,1,0x1f,0x80000000,0xffffffff};
 for(unsigned which=0;which<2;++which)for(unsigned hit=0;hit<2;++hit)for(unsigned m=0;m<5;++m){
  HSD_GObj* o=make(which);Item* item=o->user_data;NativeScriptRuntimeContext c=context(o);NativeScriptValue a[9];inputs(a);
  collision_hit=hit;collision_flags=masks[m];collision_line=m==4?-1:(int)m;
  Item expected=*item;Vec3 previous={7,8,9},normal={12,13,14};int line=-2;u32 flags=0x80000000;
  CHECK(it_8026EA20(o,&previous,&expected.pos,&normal,&line,&flags)==hit);unsigned before=segments;
  NativeScriptValue out=sentinel();CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSweepSegment,a,9,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&segments==before+1&&!memcmp(item,&expected,sizeof(expected)));
  CHECK(a[3].as.f32==normal.x&&a[4].as.f32==normal.y&&memcmp(&a[5].as.f32,&normal.z,sizeof(float))==0);
  int32_t bits;memcpy(&bits,&flags,sizeof(bits));CHECK(a[6].as.i64==line&&a[7].as.i64==bits&&a[8].as.u64==hit);
  CHECK(observed[0]==7&&observed[1]==8&&observed[2]==1&&observed[3]==0);++pairs;++cases;HSD_GObjFree(o);
 }
 // A miss carries even unusual normal bits through unchanged.
 HSD_GObj* o=make(0);NativeScriptRuntimeContext c=context(o);NativeScriptValue a[9];inputs(a);a[3].as.f32=NAN;a[4].as.f32=-0.0f;a[5].as.f32=INFINITY;
 NativeScriptValue saved[9];memcpy(saved,a,sizeof(a));collision_hit=false;NativeScriptValue out=sentinel();CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSweepSegment,a,9,&out,&diagnostic));
 for(unsigned n=3;n<8;++n)CHECK(!memcmp(&saved[n].as,&a[n].as,sizeof(a[n].as)));++cases;HSD_GObjFree(o);
}
static void vm_comparisons(void)
{
 for(unsigned which=0;which<2;++which)for(unsigned hit=0;hit<2;++hit)for(unsigned local=0;local<(which?1u:2u);++local){
  HSD_GObj* o=make(which);Item* item=o->user_data;NativeScriptValue a[9];inputs(a);initialize_state(o,a);
  collision_hit=hit;collision_flags=0xfedcba98;collision_line=37;Item expected=*item;Vec3 previous={7,8,9},normal={12,13,14};int line=-2;u32 flags=0x80000000;
  CHECK(it_8026EA20(o,&previous,&expected.pos,&normal,&line,&flags)==hit);unsigned before=segments;
  NativeScriptValue out=sentinel();CHECK(nativeScriptItem_RunHook(o,local?NativeScriptItemHook_Dropped:NativeScriptItemHook_PickedUp,NULL,0,&out)==NativeScriptVM_Ok);++vm_calls;
  CHECK(out.type==NativeScriptType_Void&&segments==before+1&&!memcmp(item,&expected,sizeof(expected)));
  CHECK(state(o,"nx").as.f32==normal.x&&state(o,"ny").as.f32==normal.y&&state(o,"nz").as.f32==normal.z);
  int32_t bits;memcpy(&bits,&flags,sizeof(bits));CHECK(state(o,"line").as.i64==line&&state(o,"flags").as.i64==bits&&state(o,"hit").as.u64==hit);
  if(!which&&!local)CHECK(state(o,"samples").as.i64==1);++pairs;++cases;HSD_GObjFree(o);
 }
 HSD_GObj* o=make(0);Item* i=o->user_data;NativeScriptValue a[9];inputs(a);initialize_state(o,a);i->native_removal_in_progress=true;collision_hit=false;
 NativeScriptValue out=sentinel();CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Destroyed,NULL,0,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Void);++cases;++vm_calls;HSD_GObjFree(o);
}
static void rejected(void)
{
 for(unsigned bad=0;bad<25;++bad){
  HSD_GObj* o=make(0);Item* i=o->user_data;NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,i->x1C,&view));NativeScriptStorageInstance storage=*view.storage;
  NativeScriptRuntimeContext c=context(o);NativeScriptValue a[9];inputs(a);uint64_t epoch=nativeGObjPool.lifetime;Item before=*i;
  unsigned count=9;uint32_t semantic=NativeScriptBinding_ItemSweepSegment;
  switch(bad){case 0:a[0].type=NativeScriptType_F64;break;case 1:a[3].type=NativeScriptType_I32;break;case 2:a[6].type=NativeScriptType_I64;break;case 3:a[6].as.i64=(int64_t)INT32_MAX+1;break;case 4:a[7].as.i64=(int64_t)INT32_MIN-1;break;case 5:a[8].type=NativeScriptType_I32;break;case 6:a[0].as.f32=NAN;break;case 7:a[2].as.f32=INFINITY;break;case 8:i->pos.x=NAN;break;case 9:count=8;break;case 10:semantic=NativeScriptBinding_ItemGrounded;break;case 11:++c.generation;break;case 12:++c.scene_epoch;break;case 13:c.source_class=NativeCallbackSource_Fighter;break;case 14:view.storage->live=false;break;case 15:++view.storage->owner;break;case 16:++view.storage->generation;break;case 17:++i->native_script_owner;break;case 18:++i->native_article_author_kind;break;case 19:i->xC4_article_data=&articles[1];break;case 20:o->classifier=HSD_GOBJ_CLASS_FIGHTER;break;case 21:++nativeGObjPool.lifetime;break;case 22:view.storage->layout=NULL;break;case 23:view.storage->parameters=NULL;break;case 24:i->kind=It_Kind_Coin;break;}
  NativeScriptValue saved[9];memcpy(saved,a,sizeof(a));NativeScriptValue out=sentinel(),expected=out;Item invalid=*i;unsigned calls=segments;
  CHECK(!nativeScriptBindingItemCollision_Invoke(&c,semantic,a,count,&out,&diagnostic));CHECK(!memcmp(saved,a,sizeof(a))&&!memcmp(&out,&expected,sizeof(out))&&!memcmp(i,&invalid,sizeof(*i))&&segments==calls);
  *i=before;*view.storage=storage;nativeGObjPool.lifetime=epoch;o->classifier=HSD_GOBJ_CLASS_ITEM;++cases;HSD_GObjFree(o);
 }
 HSD_GObj* o=make(0);NativeScriptRuntimeContext c=context(o);NativeScriptValue a[9];inputs(a);HSD_GObjFree(o);NativeScriptValue out=sentinel();CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSweepSegment,a,9,&out,&diagnostic));++cases;
 o=make(0);c=context(o);CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSweepSegment,NULL,9,&out,&diagnostic));CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSweepSegment,a,9,NULL,&diagnostic));++cases;HSD_GObjFree(o);
 o=make(0);const NativeScriptItemProgram* p=spec(0);CHECK(p->signal_count==1);CHECK(nativeScriptItem_RunSignal(o,p->signals[0].signal_id,0,&out)==NativeScriptVM_Error);++vm_calls;c=context(o);CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSweepSegment,a,9,&out,&diagnostic));++cases;HSD_GObjFree(o);
}
static void publication_probes(void)
{
 for(unsigned mode=1;mode<=5;++mode){HSD_GObj* o=make(0);NativeScriptRuntimeContext c=context(o);NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,c.generation,&view));NativeScriptStorageInstance storage=*view.storage;
  uint64_t layout=view.storage->layout->identity,parameters=view.storage->parameters->identity;NativeScriptValue a[9];inputs(a);NativeScriptValue saved[9];memcpy(saved,a,sizeof(a));NativeScriptValue out=sentinel(),old=out;
  sweep_source=o;mutation=mode;collision_hit=true;CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSweepSegment,a,9,&out,&diagnostic));
  CHECK(mutation==0&&!memcmp(a,saved,sizeof(a))&&!memcmp(&out,&old,sizeof(out)));CHECK(!memcmp(&((Item*)o->user_data)->pos,&collision_position,sizeof(Vec3)));
  *view.storage=storage;source.compiler.bank.storage_layout.identity=layout;source.compiler.bank.storage_parameters.identity=parameters;++cases;HSD_GObjFree(o);
 }
 // The shared VM also keeps authored state destinations unmodified on failure.
 HSD_GObj* o=make(0);NativeScriptValue a[9];inputs(a);initialize_state(o,a);NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&view));uint64_t layout=view.storage->layout->identity;
 sweep_source=o;mutation=2;collision_hit=true;NativeScriptValue out=sentinel(),old=out;CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_PickedUp,NULL,0,&out)==NativeScriptVM_Error);++vm_calls;
 CHECK(!memcmp(&out,&old,sizeof(out)));source.compiler.bank.storage_layout.identity=layout;CHECK(state(o,"nx").as.f32==12&&state(o,"ny").as.f32==13&&state(o,"line").as.i64==-2&&state(o,"flags").as.i64==INT32_MIN);++cases;HSD_GObjFree(o);
}
static void compiler_negatives(void)
{
 const char* calls[]={
 "item.sweep_segment(0, 0, 0, item.state.x, item.state.x, item.state.z, item.state.line, item.state.flags, item.state.hit)",
 "item.sweep_segment(0, 0, 0, item.state.x, item.state.y, item.state.z, item.state.line, item.state.line, item.state.hit)",
 "item.sweep_segment(0, 0, 0, item.state.x, item.state.y, item.state.z, item.state.line, item.state.flags, item.state.line)",
 "item.sweep_segment(0, 0, 0, 1.0, item.state.y, item.state.z, item.state.line, item.state.flags, item.state.hit)",
 "item.sweep_segment(0, 0, 0, item.state.x, item.state.y, item.state.z, item.state.line, item.state.flags)",
 "item.sweep_segment(0, 0, 0, item.state.x, item.state.y, item.state.z, item.state.line, item.state.flags, item.state.hit, 1)",
 "item.sweep_segment(\"bad\", 0, 0, item.state.x, item.state.y, item.state.z, item.state.line, item.state.flags, item.state.hit)"};
 NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&articles[0]};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 for(unsigned n=0;n<8;++n){char bad[2048];
  if(n<7)snprintf(bad,sizeof(bad),"fighter:\n    behavior independent\narticle Probe states 1:\n    state float x\n    state float y\n    state float z\n    state int line\n    state int flags\n    state bool hit\n    on picked_up:\n        %s\n    state 0 anim -1:\n",calls[n]);
  else snprintf(bad,sizeof(bad),"fighter:\n    behavior independent\nfunc Bad():\n    let float x = 0\n    let float y = 0\n    let float z = 0\n    let int line = 0\n    let int flags = 0\n    let bool hit = false\n    item.sweep_segment(0, 0, 0, x, y, z, line, flags, hit)\n");
  NativeScriptCompileRequest request={.source=bad,.source_bytes=(uint32_t)strlen(bad),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));CHECK(!scratch.bank.valid);++cases;
 }nativeSourceBank_Release(&scratch);
}
MapCollData* mpLib_804D64B4;
CollLine* groundCollLine;
CollVtx* groundCollVtx;
CollJoint* groundCollJoint;
static MapCollData geometry,other_geometry;
static MapLine geometry_lines[3];
static CollLine collision_lines[3];
static CollVtx geometry_vertices[6];
static MapJoint map_joints[3];
static CollJoint collision_joints[3];
static unsigned contact_calls,contact_mutation,contact_kind;
static int contact_amount;
static CollData contact_record;
static HSD_GObj* contact_source;
static void contact_callback(void* data,int joint,CollData* collision,int amount,mpLib_GroundEnum kind,float dy)
{
 CHECK(data==&geometry&&joint>=0&&joint<2&&dy==0);++contact_calls;
 contact_record=*collision;contact_kind=kind;contact_amount=amount;
 unsigned mutate=contact_mutation;contact_mutation=0;
 if(mutate==1)++source.compiler.bank.storage_layout.identity;
 if(mutate==2)mpLib_804D64B4=&other_geometry;
 if(mutate==3)collision_lines[joint].flags|=LINE_FLAG_HIDDEN;
 if(mutate==4)nativeScriptItem_Detach(contact_source);
}
static void geometry_prepare(void)
{
 geometry=(MapCollData){.lines=geometry_lines,.line_count=3,.vert_count=6,.joints=map_joints,.joint_count=3};other_geometry=geometry;
 mpLib_804D64B4=&geometry;groundCollLine=collision_lines;groundCollVtx=geometry_vertices;groundCollJoint=collision_joints;
 const u32 kinds[]={CollLine_Floor,CollLine_Ceiling,CollLine_LeftWall};
 for(unsigned line=0;line<3;++line){
  geometry_lines[line]=(MapLine){.v0_idx=(u16)(line*2),.v1_idx=(u16)(line*2+1)};
  collision_lines[line]=(CollLine){.x0=&geometry_lines[line],.flags=LINE_FLAG_ENABLED|kinds[line]};
  map_joints[line]=(MapJoint){.vtx_start=(s16)(line*2),.vtx_count=2};
  collision_joints[line]=(CollJoint){.inner=&map_joints[line],.cb_0=contact_callback,.cb_data_0=(void*)&geometry,.cb_1=contact_callback,.cb_data_1=(void*)&geometry};
  geometry_vertices[line*2]=(CollVtx){.pos={(float)line+2,(float)line+3},.x10=(float)line,.x14=(float)line};
  geometry_vertices[line*2+1]=(CollVtx){.pos={(float)line+12,(float)line+8},.x10=(float)line+10,.x14=(float)line+5};
 }
 contact_calls=contact_mutation=0;
}
static NativeScriptValue index_value(int line){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=line};}
static void normal_arguments(int line,NativeScriptValue* args)
{args[0]=index_value(line);for(unsigned n=1;n<4;++n)args[n]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=-99};}
static void line_comparisons(void)
{
 for(unsigned which=0;which<2;++which)for(int line=0;line<3;++line){
  geometry_prepare();HSD_GObj* o=make(which);Item* item=o->user_data;NativeScriptRuntimeContext c=context(o);
  item->x378_itemColl.x0_gobj=o;item->x378_itemColl.x34_flags.b1234=2;item->x378_itemColl.x50=17.75f;item->x378_itemColl.cur_pos=(Vec3){4,5,6};
  NativeScriptValue a[4],out=sentinel();normal_arguments(line,a);Vec3 normal;mpLineGetNormal(line,&normal);
  CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineExists,a,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==mpLib_80054ED8(line));++pairs;
  CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineNormal,a,4,&out,&diagnostic)&&out.type==NativeScriptType_Void);
  CHECK(!memcmp(&a[1].as.f32,&normal.x,sizeof(float))&&!memcmp(&a[2].as.f32,&normal.y,sizeof(float))&&!memcmp(&a[3].as.f32,&normal.z,sizeof(float)));++pairs;
  Item expected=*item;mpColl_80043558(&expected.x378_itemColl,line);unsigned calls=contact_calls;CollData observed=contact_record;unsigned kind=contact_kind;int amount=contact_amount;
  CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,a,1,&out,&diagnostic)&&out.type==NativeScriptType_Void);
  CHECK(!memcmp(item,&expected,sizeof(expected)));CHECK(contact_calls==calls+(line<2));
  if(line<2)CHECK(!memcmp(&observed,&contact_record,sizeof(observed))&&contact_kind==kind&&kind==(line?0:2)&&contact_amount==amount&&amount==17);++pairs;
  expected=*item;bool speed=mpGetSpeed(line,&expected.pos,&expected.x40_vel);
  CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemReadStageLineSpeed,a,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==speed&&!memcmp(item,&expected,sizeof(expected)));++pairs;
  CHECK(item->x40_vel.z==0&&item->pos.x==1&&item->pos.z==3);++cases;HSD_GObjFree(o);
 }
 // Native remapping includes clamping and the zero-length previous-line branch.
 for(unsigned location=0;location<3;++location)for(unsigned collapsed=0;collapsed<2;++collapsed){
  geometry_prepare();HSD_GObj* o=make(0);Item* item=o->user_data;item->pos=(Vec3){location==0?-100:location==1?5:100,2,9};
  if(collapsed){geometry_vertices[1].x10=geometry_vertices[0].x10;geometry_vertices[1].x14=geometry_vertices[0].x14;}
  Item expected=*item;CHECK(mpGetSpeed(0,&expected.pos,&expected.x40_vel));NativeScriptRuntimeContext c=context(o);NativeScriptValue a=index_value(0),out=sentinel();
  CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemReadStageLineSpeed,&a,1,&out,&diagnostic)&&out.as.u64==1&&!memcmp(item,&expected,sizeof(expected)));++pairs;++cases;HSD_GObjFree(o);
 }
 // Actual Item VM executes all four operations against its declared state.
 geometry_prepare();HSD_GObj* o=make(0);Item* i=o->user_data;i->x378_itemColl.x0_gobj=o;i->x378_itemColl.x34_flags.b1234=3;i->x378_itemColl.x50=12;
 NativeScriptValue args[9];inputs(args);args[6]=index_value(0);initialize_state(o,args);Item expected=*i;CHECK(mpGetSpeed(0,&expected.pos,&expected.x40_vel));Vec3 normal;mpLineGetNormal(0,&normal);
 NativeScriptValue out=sentinel();CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Void);++vm_calls;
 CHECK(!memcmp(i,&expected,sizeof(expected))&&contact_calls==1&&contact_kind==2&&contact_record.x0_gobj==o&&contact_record.x34_flags.b1234==3);
 CHECK(state(o,"nx").as.f32==normal.x&&state(o,"ny").as.f32==normal.y&&state(o,"nz").as.f32==normal.z&&state(o,"hit").as.u64==1);++cases;HSD_GObjFree(o);
}
static void line_failures(void)
{
 geometry_prepare();HSD_GObj* o=make(0);Item* item=o->user_data;item->x378_itemColl.x0_gobj=o;item->x378_itemColl.x34_flags.b1234=2;NativeScriptRuntimeContext c=context(o);
 const int missing[]={-1,-2,3,INT32_MAX};
 for(unsigned n=0;n<8;++n){NativeScriptValue a[4];normal_arguments(n<4?missing[n]:0,a);geometry_prepare();
  if(n==4)collision_lines[0].flags=0;if(n==5)collision_lines[0].flags|=LINE_FLAG_HIDDEN;if(n==6)mpLib_804D64B4=NULL;if(n==7)groundCollLine=NULL;
  NativeScriptValue out=sentinel();CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineExists,a,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&!out.as.u64);
  NativeScriptValue saved[4];memcpy(saved,a,sizeof(a));out=sentinel();NativeScriptValue old=out;Item before=*item;
  CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineNormal,a,4,&out,&diagnostic)&&!memcmp(a,saved,sizeof(a))&&!memcmp(&old,&out,sizeof(out)));
  CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,a,1,&out,&diagnostic)&&!memcmp(&old,&out,sizeof(out))&&!memcmp(item,&before,sizeof(before)));
  CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemReadStageLineSpeed,a,1,&out,&diagnostic)&&!memcmp(&old,&out,sizeof(out))&&!memcmp(item,&before,sizeof(before)));++cases;
 }
 const NativeScriptValue malformed[]={ {.type=NativeScriptType_F32,.as.f32=0},{.type=NativeScriptType_Bool,.as.u64=2},{.type=NativeScriptType_I32,.as.i64=(int64_t)INT32_MAX+1},{.type=NativeScriptType_I64,.as.i64=INT64_MIN},{.type=NativeScriptType_U32,.as.u64=UINT32_MAX},{.type=NativeScriptType_U64,.as.u64=UINT64_MAX} };
 geometry_prepare();
 for(unsigned n=0;n<6;++n){NativeScriptValue a[4];normal_arguments(0,a);a[0]=malformed[n];NativeScriptValue out=sentinel(),old=out;unsigned calls=contact_calls;
  CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineExists,a,1,&out,&diagnostic));CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineNormal,a,4,&out,&diagnostic));
  CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,a,1,&out,&diagnostic));CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemReadStageLineSpeed,a,1,&out,&diagnostic));CHECK(!memcmp(&out,&old,sizeof(out))&&calls==contact_calls);++cases;
 }
 const unsigned integral_types[]={NativeScriptType_Bool,NativeScriptType_I32,NativeScriptType_I64,NativeScriptType_U32,NativeScriptType_U64};
 for(unsigned n=0;n<5;++n){NativeScriptValue a={.type=(u8)integral_types[n],.as.u64=0},out=sentinel();
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StageLineExists,NULL,0,&a,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==1);
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemBindStageLine,NULL,0,&a,1,&out,&diagnostic)&&out.type==NativeScriptType_Void);
  CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemReadStageLineSpeed,NULL,0,&a,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==1);++cases;
 }
 for(unsigned n=0;n<6;++n){NativeScriptValue a=malformed[n],out=sentinel(),old=out;
  CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_StageLineExists,NULL,0,&a,1,&out,&diagnostic));CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemBindStageLine,NULL,0,&a,1,&out,&diagnostic));CHECK(!memcmp(&out,&old,sizeof(out)));++cases;
 }
 for(unsigned n=0;n<5;++n){geometry_prepare();Item before=*item;NativeScriptValue a=index_value(0),out=sentinel(),old=out;
  if(n==0)item->x378_itemColl.x0_gobj=NULL;if(n==1)item->x378_itemColl.x34_flags.b1234=1;if(n==2)item->x378_itemColl.x50=NAN;if(n==3)item->x378_itemColl.x50=2147483648.0f;if(n==4)item->x378_itemColl.x50=-INFINITY;
  CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,&a,1,&out,&diagnostic)&&!memcmp(&out,&old,sizeof(out))&&!contact_calls);*item=before;++cases;
 }
 // Native no-callback path performs no float-to-int conversion.
 geometry_prepare();collision_joints[0].cb_0=NULL;item->x378_itemColl.x50=NAN;NativeScriptValue a=index_value(0),out=sentinel();CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,&a,1,&out,&diagnostic)&&out.type==NativeScriptType_Void);++cases;
 item->x378_itemColl.x50=0;geometry_prepare();nativeScriptItem_Detach(o);out=sentinel();CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineExists,&a,1,&out,&diagnostic));CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,&a,1,&out,&diagnostic));++cases;HSD_GObjFree(o);
 // Native lookup stops at the first matching range. An unread later joint is
 // irrelevant; a missing descriptor on the traversed prefix must reject.
 geometry_prepare();o=make(0);item=o->user_data;item->x378_itemColl.x0_gobj=o;item->x378_itemColl.x34_flags.b1234=2;c=context(o);collision_joints[1].inner=NULL;a=index_value(0);out=sentinel();
 Item expected=*item;mpColl_80043558(&expected.x378_itemColl,0);CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,&a,1,&out,&diagnostic)&&out.type==NativeScriptType_Void&&!memcmp(item,&expected,sizeof(expected)));++pairs;++cases;
 a=index_value(1);out=sentinel();NativeScriptValue old=out;unsigned calls=contact_calls;CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,&a,1,&out,&diagnostic)&&!memcmp(&out,&old,sizeof(out))&&calls==contact_calls);++cases;HSD_GObjFree(o);geometry_prepare();
}
static void contact_publication(void)
{
 for(unsigned mode=1;mode<=4;++mode){geometry_prepare();HSD_GObj* o=make(0);Item* item=o->user_data;item->x378_itemColl.x0_gobj=o;item->x378_itemColl.x34_flags.b1234=2;NativeScriptRuntimeContext c=context(o);uint64_t layout=source.compiler.bank.storage_layout.identity;
  contact_source=o;contact_mutation=mode;NativeScriptValue a=index_value(0),out=sentinel(),old=out;
  bool success=nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemBindStageLine,&a,1,&out,&diagnostic);
  CHECK(success==(mode==3));CHECK(contact_calls==1&&!contact_mutation);if(mode==3)CHECK(out.type==NativeScriptType_Void);else CHECK(!memcmp(&out,&old,sizeof(out)));
  source.compiler.bank.storage_layout.identity=layout;mpLib_804D64B4=&geometry;++cases;HSD_GObjFree(o);
 }
 geometry_prepare();
}
static void line_compiler_negatives(void)
{
 const char* calls[]={"stage.line_exists(0.0)","stage.line_normal(0.0, item.state.x, item.state.y, item.state.z)","stage.line_normal(0, item.state.x, item.state.x, item.state.z)","stage.line_normal(0, 1.0, item.state.y, item.state.z)","item.bind_stage_line(0.0)","item.read_stage_line_speed(0.0)","stage.line_normal(0, item.state.x, item.state.y)"};
 NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&articles[0]};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 for(unsigned n=0;n<7;++n){char bad[1024];snprintf(bad,sizeof(bad),"fighter:\n    behavior independent\narticle Probe states 1:\n    state float x\n    state float y\n    state float z\n    on picked_up:\n        %s\n    state 0 anim -1:\n",calls[n]);NativeScriptCompileRequest request={.source=bad,.source_bytes=(uint32_t)strlen(bad),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&!scratch.bank.valid);++cases;
 }nativeSourceBank_Release(&scratch);
}
static void fighter_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void fighter_line_queries(void)
{
 geometry_prepare();static Fighter fighter;HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
 fighter=(Fighter){.gobj=o,.kind=source.target,.behavior_kind=source.target,.x8_spawnNum=4001,.ft_data=&source.assets.character.data};
 gFtDataList[source.target]=&source.assets.character.data;ftData_CharacterStateTables[source.target]=source.assets.motions;
 GObj_InitUserData(o,4,fighter_remove,&fighter);CHECK(nativeScriptFighter_Attach(o));
 NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=4001,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptValue a[4],out=sentinel();normal_arguments(0,a);Vec3 normal;mpLineGetNormal(0,&normal);Fighter before=fighter;
 CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineExists,a,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==1);++pairs;
 CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageLineNormal,a,4,&out,&diagnostic)&&out.type==NativeScriptType_Void&&a[1].as.f32==normal.x&&a[2].as.f32==normal.y&&a[3].as.f32==normal.z&&!memcmp(&fighter,&before,sizeof(before)));++pairs;
 const char* helpers[]={"LineExists","NormalX"};
 for(unsigned n=0;n<2;++n){uint32_t function=UINT32_MAX;const NativeScriptProgramIndex* p=&source.compiler.bank.declarations;
  for(unsigned f=0;f<p->function_count;++f)if(!p->functions[f].owner&&nativeScriptSource_SpanEquals(p->tree,p->functions[f].name,helpers[n]))function=f;
  CHECK(function!=UINT32_MAX);CHECK(nativeScriptFighter_Helper(o,4001,function,NULL,0,&out,&diagnostic)==NativeScriptVM_Ok);++vm_calls;
  CHECK(n?(out.type==NativeScriptType_F32&&out.as.f32==normal.x):(out.type==NativeScriptType_Bool&&out.as.u64==1));++cases;
 }
 HSD_GObjFree(o);
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();compiler_negatives();line_compiler_negatives();compare();vm_comparisons();rejected();publication_probes();line_comparisons();line_failures();contact_publication();fighter_line_queries();CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS Item sweep and stage lines\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"actor_vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
