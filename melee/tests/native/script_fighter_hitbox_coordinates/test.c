#include "boundary.h"
#include <melee/ft/native_script_binding_fighter_hitboxes.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ft_0DF0.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/fighter.h>
#include <melee/it/types.h>
#include <melee/db/db.h>
#include <melee/pl/player.h>
#include <melee/pl/types.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=2048,F=64,V=64};
static char bytes[32768];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeSourcePackage package;
#define bank package.compiler.bank
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=10,.ground={343,345,347,349},.air={344,346,348,350}};
static Fighter fighters[32];static MotionState motions[351];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static struct Fighter_804D6524_t scale_data;struct Fighter_804D6524_t* Fighter_804D6524=&scale_data;
static float weights[9];float* Fighter_804D6548=weights;DbLKind DbLevel;
static StaleMoveTable stale;static bool missing_table;
static unsigned allocated,checks,cases,pairs,vm_calls,negative_cases,table_reads;
static uint8_t snapshot[65536];
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"hitbox line%u %s\n",__LINE__,#x);exit(1);}}while(0)
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&package:NULL;}
StaleMoveTable* Player_GetStaleMoveTableIndexPtr(s32 player)
{CHECK(player==2);++table_reads;return missing_table?NULL:&stale;}
#include "native_hitbox.inc"
static void userdata_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
#include "coordinate_source.inc"
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* bad[]={"let float x = hitbox.current(0.5).x","let float x = hitbox.previous().y","let float x = hitbox.current(0, 1).z","let float x = hitbox.current.x","let float x = hitbox.previous.x","let float x = hitbox.current(0)","hitbox.previous(0) = 1","let float x = hitbox.current(0).w","hitbox.current(0)","hitbox.set_world_position(0.5, 1, 2, 3)"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){
  char invalid[512];snprintf(invalid,sizeof(invalid),"fighter:\n    behavior independent\nfunc Bad():\n    %s\n",bad[i]);
  NativeScriptCompileRequest r={.source=invalid,.source_bytes=(uint32_t)strlen(invalid),.motion_layout=&layout};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
  bool valid=nativeScriptCompile_Build(&bank,&r,&accepted,&d);if(valid)fprintf(stderr,"negative compiled: %s\n",bad[i]);CHECK(!valid&&accepted==NULL);++cases;++negative_cases;
 }
 const char* foreign="fighter:\n    behavior independent\narticle It_Kind_Link_Arrow states 1:\n    func Bad():\n        let float x = hitbox.previous(0).x\n";
 static ItemAttr article_attributes;static ItemModelDesc model;
 static Article article={.x0_common_attr=&article_attributes,.x10_modelDesc=&model};
 NativeScriptItemAsset asset={.declaration="It_Kind_Link_Arrow",.kind=It_Kind_Link_Arrow,.article=&article};
 NativeScriptItemMetadataContext metadata={.motions=&layout,.assets=&asset,.asset_count=1};
 NativeScriptCompileRequest foreign_request={.source=foreign,.source_bytes=(uint32_t)strlen(foreign),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};NativeScriptDiagnostic foreign_d={0};const NativeScriptCompileBank* foreign_bank=NULL;
 CHECK(!nativeScriptCompile_Build(&bank,&foreign_request,&foreign_bank,&foreign_d)&&foreign_bank==NULL);CHECK(foreign_d.source.line==5);++cases;++negative_cases;
 const char* source=script_source;
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&package.program,&d)){
  fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);
 }
 package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.motions=motions;package.assets.facts.moves=layout;
 gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&package.program);
}
static HSD_GObj* create(void)
{
 CHECK(allocated<32);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);Fighter* f=&fighters[allocated++];memset(f,0,sizeof(*f));
 f->gobj=object;f->kind=f->behavior_kind=Ft_Kind_CustomFirst;f->ft_data=&package.assets.character.data;f->x8_spawnNum=allocated;f->player_id=2;f->x34_scale.y=1;
 GObj_InitUserData(object,4,userdata_remove,f);CHECK(nativeScriptFighter_Attach(object));return object;
}
static uint32_t function_id(const char* name)
{for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name))return i;CHECK(false);return UINT32_MAX;}
static bool helper(HSD_GObj* object,const char* name,const NativeScriptValue* args,unsigned count,NativeScriptValue* out)
{
 ++vm_calls;NativeScriptDiagnostic d={0};bool result=nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function_id(name),args,count,out,&d)==NativeScriptVM_Ok;
 if(!result&&strcmp(name,"Invalid"))fprintf(stderr,"helper %s %s\n",name,d.message);return result;
}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)object->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x123456789abcdef)};}
static float owned(HSD_GObj* object,const char* name,unsigned slot)
{
 NativeScriptReceiverView view;NativeScriptValue selector={.type=NativeScriptType_I32,.as.i64=slot},out={0};NativeScriptDiagnostic d={0};
 CHECK(nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view));
 for(unsigned i=0;i<view.program->value_count;++i)if(nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,name)){
  CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,&selector,1,&out,&d)&&out.type==NativeScriptType_F32);return out.as.f32;
 }
 CHECK(false);return 0;
}

static const uint32_t coordinates[]={NativeScriptBinding_HitboxCurrentX,NativeScriptBinding_HitboxCurrentY,NativeScriptBinding_HitboxCurrentZ,NativeScriptBinding_HitboxPreviousX,NativeScriptBinding_HitboxPreviousY,NativeScriptBinding_HitboxPreviousZ};
static void scalar_cases(HSD_GObj* object)
{
 Fighter* f=object->user_data;NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};
 const uint32_t bits[]={0,0x80000000,0x3f800000,0xbf400000,0x7f800000,0xff800000,0x7fc01234};
 for(unsigned slot=0;slot<4;++slot)for(unsigned axis=0;axis<6;++axis)for(unsigned b=0;b<sizeof(bits)/sizeof(*bits);++b){
  NativeScriptValue selector={.type=NativeScriptType_I32,.as.i64=slot},v={.type=NativeScriptType_F32},out=sentinel();memcpy(&v.as.f32,&bits[b],4);
  static Fighter before,expected;before=*f;expected=before;
  float* fields[]={&expected.x914[slot].x4C.x,&expected.x914[slot].x4C.y,&expected.x914[slot].x4C.z,&expected.x914[slot].x58.x,&expected.x914[slot].x58.y,&expected.x914[slot].x58.z};
  memcpy(fields[axis],&bits[b],4);
  CHECK(nativeScriptBindingFighterHitboxes_Write(&c,coordinates[axis],&selector,1,&v,&d));CHECK(!memcmp(f,&expected,sizeof(*f)));
  CHECK(nativeScriptBindingFighterHitboxes_Read(&c,coordinates[axis],&selector,1,&out,&d));CHECK(out.type==NativeScriptType_F32&&!memcmp(&out.as.f32,&bits[b],4));++cases;++pairs;
 }
}
static void native_cases(HSD_GObj* object)
{
 Fighter* f=object->user_data;static Fighter before,expected;NativeScriptValue out=sentinel();
 memset(f->x914,0x5a,sizeof(f->x914));before=*f;ftSk_SpecialS_ZeroHitboxPositions(object);expected=*f;*f=before;
 CHECK(helper(object,"chain_zero_capsules",NULL,0,&out));CHECK(!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 const Vec3 points[]={{0,0,0},{0,0,7},{3,-2,9},{0,4,-1},{NAN,0,INFINITY}};
 for(unsigned slot=0;slot<4;++slot)for(unsigned enabled=0;enabled<2;++enabled)for(unsigned previous=0;previous<2;++previous)for(unsigned p=0;p<5;++p){
  memset(f->x914,0x5a,sizeof(f->x914));f->cmd_vars[0]=enabled;f->x914[slot].x58=previous?(Vec3){11,12,13}:(Vec3){0};before=*f;
  Vec3 stored={owned(object,"chain_hit_x",slot),owned(object,"chain_hit_y",slot),owned(object,"chain_hit_z",slot)};
  Vec3 point=points[p];ftSk_SpecialS_UpdateHitboxes(object,&point,slot);expected=*f;*f=before;
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=slot},{.type=NativeScriptType_F32,.as.f32=point.x},{.type=NativeScriptType_F32,.as.f32=point.y},{.type=NativeScriptType_F32,.as.f32=point.z}};
  CHECK(helper(object,"chain_endpoint",args,4,&out));CHECK(!memcmp(f->x914,expected.x914,sizeof(f->x914)));
  Vec3 observed={owned(object,"chain_hit_x",slot),owned(object,"chain_hit_y",slot),owned(object,"chain_hit_z",slot)};
  if(enabled)stored=point;CHECK(!memcmp(&stored,&observed,sizeof(stored)));
  // Native Sheik writes its private union. Source preserves that behavior in
  // typed owned arrays, deliberately outside the native private union.
  expected.u=before.u;CHECK(!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 }
 NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};
 for(unsigned slot=0;slot<4;++slot){
  before=*f;Vec3 point={4,5,6};ftColl_8007B8A8(&f->x914[slot],&point);expected=*f;*f=before;
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=slot},{.type=NativeScriptType_F32,.as.f32=4},{.type=NativeScriptType_F32,.as.f32=5},{.type=NativeScriptType_F32,.as.f32=6}};
  CHECK(nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxSetWorldPosition,args,4,&out,&d));CHECK(out.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 }
}
static void once(HSD_GObj* object)
{
 Fighter* f=object->user_data;f->x914[1].x58.y=29;NativeScriptValue v={.type=NativeScriptType_F32,.as.f32=31},out=sentinel();
 CHECK(helper(object,"WriteOnce",&v,1,&out)&&f->x914[1].x4C.x==31);
 CHECK(helper(object,"ReadOnce",NULL,0,&out)&&out.type==NativeScriptType_F32&&out.as.f32==29);
 NativeScriptReceiverView view;NativeScriptDiagnostic d={0};CHECK(nativeScriptFighter_View(object,f->x8_spawnNum,&view));
 uint32_t field=UINT32_MAX;for(unsigned i=0;i<view.program->value_count;++i)if(nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,"calls"))field=i;
 CHECK(field!=UINT32_MAX&&nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|field,NULL,0,&out,&d)&&out.as.i64==2);++cases;
}
static void negatives(HSD_GObj* object)
{
 Fighter* f=object->user_data;static Fighter before;NativeScriptDiagnostic d={0};
 for(unsigned i=0;i<15;++i){NativeScriptRuntimeContext c=context(object);NativeScriptValue selector={.type=NativeScriptType_I32},input={.type=NativeScriptType_F32,.as.f32=8},out=sentinel(),old=out;unsigned count=1;uint32_t semantic=coordinates[0];void* userdata=object->user_data;
  switch(i){case 0:selector.as.i64=-1;break;case 1:selector.as.i64=4;break;case 2:selector.type=NativeScriptType_U64;selector.as.u64=UINT64_MAX;break;case 3:selector.type=NativeScriptType_F32;break;case 4:count=0;break;case 5:count=2;break;case 6:semantic=NativeScriptBinding_HitboxCurrent;break;case 7:++c.generation;break;case 8:++c.scene_epoch;break;case 9:object->classifier=HSD_GOBJ_CLASS_ITEM;break;case 10:object->user_data=NULL;break;case 11:c.source_class=NativeCallbackSource_Item;break;case 12:package.ready=false;break;case 13:ftData_CharacterStateTables[f->kind]=NULL;break;case 14:package.assets.facts.moves.count=9;break;}
  before=*f;CHECK(!nativeScriptBindingFighterHitboxes_Read(&c,semantic,&selector,count,&out,&d));CHECK(!memcmp(&out,&old,sizeof(out)));
  CHECK(!nativeScriptBindingFighterHitboxes_Write(&c,semantic,&selector,count,&input,&d));CHECK(!memcmp(f,&before,sizeof(*f)));object->classifier=HSD_GOBJ_CLASS_FIGHTER;object->user_data=userdata;package.ready=true;ftData_CharacterStateTables[f->kind]=motions;package.assets.facts.moves=layout;++cases;++negative_cases;
 }
 HSD_GObj* fault=create();Fighter* fault_data=fault->user_data;before=*fault_data;NativeScriptValue invalid={.type=NativeScriptType_I32,.as.i64=4},result=sentinel(),saved=result;
 CHECK(!helper(fault,"Invalid",&invalid,1,&result)&&nativeScriptFighter_Status(fault)==NativeScriptVM_Error);CHECK(!memcmp(&result,&saved,sizeof(result))&&!memcmp(fault_data,&before,sizeof(before)));
 NativeScriptRuntimeContext fault_context=context(fault);invalid.as.i64=0;
 CHECK(!nativeScriptBindingFighterHitboxes_Read(&fault_context,coordinates[0],&invalid,1,&result,&d)&&!memcmp(&result,&saved,sizeof(result)));HSD_GObjFree(fault);++cases;++negative_cases;
 for(unsigned i=0;i<4;++i){NativeScriptRuntimeContext c=context(object);NativeScriptValue args[]={{.type=NativeScriptType_I32},{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_F32,.as.f32=2},{.type=NativeScriptType_F32,.as.f32=3}},out=sentinel(),old=out;unsigned count=4;
  switch(i){case 0:args[0].type=NativeScriptType_U64;args[0].as.u64=UINT64_MAX;break;case 1:args[0].type=NativeScriptType_F32;break;case 2:count=3;break;case 3:args[2].type=NativeScriptType_Text;break;}
  before=*f;CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxSetWorldPosition,args,count,&out,&d));CHECK(!memcmp(&out,&old,sizeof(out))&&!memcmp(f,&before,sizeof(*f)));++cases;++negative_cases;
 }
 NativeScriptRuntimeContext stale_context=context(object);HSD_GObjFree(object);HSD_GObj* reused=create();CHECK(reused==object);
 NativeScriptValue selector={.type=NativeScriptType_I32},out=sentinel(),old=out;
 CHECK(!nativeScriptBindingFighterHitboxes_Read(&stale_context,coordinates[0],&selector,1,&out,&d));CHECK(!memcmp(&out,&old,sizeof(out)));HSD_GObjFree(reused);++cases;++negative_cases;
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();HSD_GObj* object=create();
 scalar_cases(object);native_cases(object);once(object);negatives(object);CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_helper_calls\":%u,\"negative_cases\":%u}\n",checks,cases,pairs,vm_calls,negative_cases);return 0;
}
