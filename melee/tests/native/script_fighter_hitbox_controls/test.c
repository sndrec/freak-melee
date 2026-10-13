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
#include <melee/lb/lbcollision.h>
#include "control_native.inc"
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* bad[]={"hitbox.set_enabled(0.5, true)","hitbox.clear_victims(0.5)","hitbox.set_enabled(0, 1)","hitbox.set_enabled(0)","hitbox.clear_victims()","hitbox.clear_victims(0, 1)","let float x = hitbox.set_enabled(0, true)","hitbox.clear_all(0)","let bool value = hitbox.clear_all()"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){
  char invalid[512];snprintf(invalid,sizeof(invalid),"fighter:\n    behavior independent\nfunc Bad():\n    %s\n",bad[i]);
  NativeScriptCompileRequest r={.source=invalid,.source_bytes=(uint32_t)strlen(invalid),.motion_layout=&layout};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
  bool valid=nativeScriptCompile_Build(&bank,&r,&accepted,&d);if(valid)fprintf(stderr,"negative compiled: %s\n",bad[i]);CHECK(!valid&&accepted==NULL);++cases;++negative_cases;
 }
 const char* foreign="fighter:\n    behavior independent\narticle It_Kind_Link_Arrow states 1:\n    func Bad():\n        hitbox.set_enabled(0, true)\n";
 static ItemAttr article_attributes;static ItemModelDesc model;
 static Article article={.x0_common_attr=&article_attributes,.x10_modelDesc=&model};
 NativeScriptItemAsset asset={.declaration="It_Kind_Link_Arrow",.kind=It_Kind_Link_Arrow,.article=&article};
 NativeScriptItemMetadataContext metadata={.motions=&layout,.assets=&asset,.asset_count=1};
 NativeScriptCompileRequest foreign_request={.source=foreign,.source_bytes=(uint32_t)strlen(foreign),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};NativeScriptDiagnostic foreign_d={0};const NativeScriptCompileBank* foreign_bank=NULL;
 CHECK(!nativeScriptCompile_Build(&bank,&foreign_request,&foreign_bank,&foreign_d)&&foreign_bank==NULL);CHECK(foreign_d.source.line==5);++cases;++negative_cases;
 foreign="fighter:\n    behavior independent\narticle It_Kind_Link_Arrow states 1:\n    func Bad():\n        hitbox.clear_all()\n";
 foreign_request.source=foreign;foreign_request.source_bytes=(uint32_t)strlen(foreign);foreign_bank=NULL;
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
 if(!result&&strcmp(name,"Fault"))fprintf(stderr,"helper %s %s\n",name,d.message);return result;
}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)object->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x123456789abcdef)};}

static void clear_all(HSD_GObj* object)
{
 Fighter* f=object->user_data;NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};static Fighter before,expected;
 for(unsigned pattern=0;pattern<=HitCapsule_Max;++pattern)for(unsigned marker=0;marker<2;++marker)for(unsigned vm=0;vm<2;++vm){
  memset(f->x914,0x5a,sizeof(f->x914));for(unsigned i=0;i<4;++i)f->x914[i].state=(pattern+i)%(HitCapsule_Max+1);
  f->x2219_b3=marker;before=*f;ftColl_8007AFF8(object);expected=*f;*f=before;NativeScriptValue out=sentinel();
  CHECK(vm?helper(object,"ClearAll",NULL,0,&out):nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxClearAll,NULL,0,&out,&d));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 }
 for(unsigned which=0;which<14;++which){
  c=context(object);NativeScriptValue arg={.type=NativeScriptType_I32},out=sentinel(),saved=out;unsigned count=0;NativeScriptValue* result=&out;void* data=object->user_data;
  switch(which){case 0:count=1;break;case 1:count=UINT32_MAX;break;case 2:result=NULL;break;case 3:++c.generation;break;case 4:++c.scene_epoch;break;case 5:object->classifier=HSD_GOBJ_CLASS_ITEM;break;case 6:object->user_data=NULL;break;case 7:c.source_class=NativeCallbackSource_Item;break;case 8:package.ready=false;break;case 9:ftData_CharacterStateTables[f->kind]=NULL;break;case 10:package.assets.facts.moves.count=9;break;case 11:f->ft_data=NULL;break;case 12:f->kind=Ft_Kind_Mario;break;case 13:c.source=NULL;break;}
  before=*f;CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxClearAll,&arg,count,result,&d));CHECK(!memcmp(&out,&saved,sizeof(out))&&!memcmp(f,&before,sizeof(*f)));
  object->classifier=HSD_GOBJ_CLASS_FIGHTER;object->user_data=data;package.ready=true;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;package.assets.facts.moves=layout;f->ft_data=&package.assets.character.data;f->kind=Ft_Kind_CustomFirst;++cases;++negative_cases;
 }
}

static void controls(HSD_GObj* object)
{
 Fighter* f=object->user_data;NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};
 static Fighter before,expected;
 for(unsigned slot=0;slot<4;++slot)for(unsigned marker=0;marker<2;++marker)for(unsigned enabled=0;enabled<2;++enabled){
  memset(f->x914,0x5a,sizeof(f->x914));f->x2219_b3=marker;before=*f;
  if(enabled)lbColl_80008434(&f->x914[slot]);else lbColl_80008428(&f->x914[slot]);
  if(enabled)f->x2219_b3=true;expected=*f;*f=before;
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=slot},{.type=NativeScriptType_Bool,.as.u64=enabled}},out=sentinel();
  CHECK(nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxSetEnabled,args,2,&out,&d));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 }
 for(unsigned slot=0;slot<4;++slot)for(unsigned state=0;state<3;++state){
  memset(f->x914,0x5a,sizeof(f->x914));f->x914[slot].state=state;before=*f;
  lbColl_80008440(&f->x914[slot]);expected=*f;*f=before;
  NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=slot},out=sentinel();
  CHECK(helper(object,"Clear",&arg,1,&out));CHECK(!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 }
 for(unsigned active=0;active<2;++active){
  memset(f->x914,0x5a,sizeof(f->x914));f->x2219_b3=active;before=*f;
  if(active)ftSeakSpecialS_LoopChainHitActivate(object);else ftSeakSpecialS_LoopChainHitCollisions(object);
  expected=*f;*f=before;NativeScriptValue out=sentinel();
  CHECK(helper(object,active?"chain_activate":"chain_disable",NULL,0,&out));
  CHECK(!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 }
 NativeScriptValue value={.type=NativeScriptType_Bool,.as.u64=1},out=sentinel();
 CHECK(helper(object,"EnableOnce",&value,1,&out));CHECK(helper(object,"ClearOnce",NULL,0,&out));
 CHECK(helper(object,"Calls",NULL,0,&out)&&out.type==NativeScriptType_I32&&out.as.i64==2);++cases;
}

static void cleanup(HSD_GObj* object)
{
 Fighter* f=object->user_data;static Fighter before,stock;NativeScriptValue out=sentinel();
 for(unsigned command=0;command<2;++command)for(unsigned timer=0;timer<3;++timer)for(unsigned marker=0;marker<2;++marker){
  memset(f->x914,0x5a,sizeof(f->x914));f->x914[0].state=HitCapsule_Disabled;f->cmd_vars[0]=command;f->x2219_b3=marker;
  f->mv.ns.attackhi4.yoyoRehitTimer=timer;before=*f;
  ftNs_AttackHi4_YoyoCheckTimedRehit(object);stock=*f;*f=before;
  NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=timer};CHECK(helper(object,"Timer",&v,1,&out));
  CHECK(helper(object,"YoyoCheckTimedRehit",NULL,0,&out));
  CHECK(!memcmp(f->x914,stock.x914,sizeof(f->x914)));
  CHECK(helper(object,"TimerValue",NULL,0,&out)&&out.as.i64==stock.mv.ns.attackhi4.yoyoRehitTimer);
  bool rehit=!command&&timer==1;
  CHECK(f->x2219_b3==(rehit?true:marker));
  stock.mv=before.mv;if(rehit)stock.x2219_b3=true;CHECK(!memcmp(f,&stock,sizeof(*f)));++cases;++pairs;
  if(rehit&&!marker){
   for(unsigned skip=0;skip<2;++skip){
    static Fighter corrected;corrected=*f;
    reference_cleanup(object,skip?Ft_MF_SkipHit:0);
    CHECK(f->x914[0].state==(skip?HitCapsule_Enabled:HitCapsule_Disabled));CHECK(f->x2219_b3==skip);
    // Negative control: raw native delayed rehit leaves cleanup unlatched.
    *f=before;ftNs_AttackHi4_YoyoCheckTimedRehit(object);reference_cleanup(object,skip?Ft_MF_SkipHit:0);
    CHECK(f->x914[0].state==HitCapsule_Enabled&&!f->x2219_b3);*f=corrected;++cases;
   }
  }
 }
}

static void absent_indices(HSD_GObj* object)
{
 Fighter* f=object->user_data;static Fighter before;NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};
 NativeScriptValue indices[]={{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I32,.as.i64=4},{.type=NativeScriptType_I64,.as.i64=INT64_MIN},{.type=NativeScriptType_U32,.as.u64=UINT32_MAX},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000001)},{.type=NativeScriptType_U64,.as.u64=UINT64_MAX}};
 for(unsigned i=0;i<sizeof(indices)/sizeof(*indices);++i)for(unsigned clear=0;clear<2;++clear){
  NativeScriptValue args[]={indices[i],{.type=NativeScriptType_Bool,.as.u64=1}},out=sentinel();before=*f;
  CHECK(nativeScriptBindingFighterHitboxes_Invoke(&c,clear?NativeScriptBinding_HitboxClearVictims:NativeScriptBinding_HitboxSetEnabled,args,clear?1:2,&out,&d));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(f,&before,sizeof(*f)));++cases;
 }
}

static void rejection(HSD_GObj* object)
{
 Fighter* f=object->user_data;static Fighter before;NativeScriptDiagnostic d={0};
 for(unsigned which=0;which<16;++which)for(unsigned clear=0;clear<2;++clear){
  NativeScriptRuntimeContext c=context(object);NativeScriptValue args[]={{.type=NativeScriptType_I32},{.type=NativeScriptType_Bool,.as.u64=1}},out=sentinel(),saved=out;
  unsigned count=clear?1:2;void* data=object->user_data;
  switch(which){case 0:args[0].type=NativeScriptType_F32;break;case 1:args[0].type=NativeScriptType_Text;break;case 2:count=0;break;case 3:count=3;break;case 4:++c.generation;break;case 5:++c.scene_epoch;break;case 6:object->classifier=HSD_GOBJ_CLASS_ITEM;break;case 7:object->user_data=NULL;break;case 8:c.source_class=NativeCallbackSource_Item;break;case 9:package.ready=false;break;case 10:ftData_CharacterStateTables[f->kind]=NULL;break;case 11:package.assets.facts.moves.count=9;break;case 12:f->ft_data=NULL;break;case 13:f->kind=Ft_Kind_Mario;break;case 14:c.source=NULL;break;case 15:++c.generation;args[0].type=NativeScriptType_U64;args[0].as.u64=UINT64_MAX;break;}
  before=*f;CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,clear?NativeScriptBinding_HitboxClearVictims:NativeScriptBinding_HitboxSetEnabled,args,count,&out,&d));
  CHECK(!memcmp(&out,&saved,sizeof(out))&&!memcmp(f,&before,sizeof(*f)));
  object->classifier=HSD_GOBJ_CLASS_FIGHTER;object->user_data=data;package.ready=true;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;package.assets.facts.moves=layout;f->ft_data=&package.assets.character.data;f->kind=Ft_Kind_CustomFirst;++cases;++negative_cases;
 }
 for(unsigned type=0;type<2;++type){
  NativeScriptRuntimeContext c=context(object);NativeScriptValue args[]={{.type=NativeScriptType_I32},{.type=type?NativeScriptType_F32:NativeScriptType_I32,.as.i64=1}},out=sentinel(),saved=out;before=*f;
  CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxSetEnabled,args,2,&out,&d));CHECK(!memcmp(f,&before,sizeof(*f))&&!memcmp(&out,&saved,sizeof(out)));++cases;++negative_cases;
 }
 for(unsigned which=0;which<6;++which)for(unsigned clear=0;clear<2;++clear){
  NativeScriptRuntimeContext c=context(object);NativeScriptValue args[]={{.type=NativeScriptType_I32},{.type=NativeScriptType_Bool,.as.u64=1}},out=sentinel(),saved=out;
  switch(which){case 0:args[0].as.i64=(int64_t)INT32_MAX+1;break;case 1:args[0].as.i64=(int64_t)INT32_MIN-1;break;case 2:args[0].type=NativeScriptType_U32;args[0].as.u64=UINT64_C(0x100000001);break;case 3:args[0].type=NativeScriptType_Bool;args[0].as.u64=2;break;case 4:args[0].type=NativeScriptType_Bool;args[0].as.u64=UINT64_MAX;break;case 5:if(clear)continue;args[1].as.u64=2;break;}
  before=*f;CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,clear?NativeScriptBinding_HitboxClearVictims:NativeScriptBinding_HitboxSetEnabled,args,clear?1:2,&out,&d));
  CHECK(!memcmp(f,&before,sizeof(*f))&&!memcmp(&out,&saved,sizeof(out)));++cases;++negative_cases;
 }
 HSD_GObj* fault=create();NativeScriptValue result=sentinel(),saved=result;CHECK(!helper(fault,"Fault",NULL,0,&result)&&nativeScriptFighter_Status(fault)==NativeScriptVM_Error);
 NativeScriptRuntimeContext c=context(fault);NativeScriptValue args[]={{.type=NativeScriptType_I32},{.type=NativeScriptType_Bool,.as.u64=1}};
 before=*(Fighter*)fault->user_data;CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxSetEnabled,args,2,&result,&d));CHECK(!memcmp(&result,&saved,sizeof(result))&&!memcmp(fault->user_data,&before,sizeof(before)));++cases;++negative_cases;
 CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxClearAll,NULL,0,&result,&d));CHECK(!memcmp(&result,&saved,sizeof(result))&&!memcmp(fault->user_data,&before,sizeof(before)));++cases;++negative_cases;HSD_GObjFree(fault);
 c=context(object);HSD_GObjFree(object);HSD_GObj* reuse=create();CHECK(reuse==object);before=*(Fighter*)reuse->user_data;
 CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxClearVictims,args,1,&result,&d));CHECK(!memcmp(&result,&saved,sizeof(result))&&!memcmp(reuse->user_data,&before,sizeof(before)));++cases;++negative_cases;
 CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxClearAll,NULL,0,&result,&d));CHECK(!memcmp(&result,&saved,sizeof(result))&&!memcmp(reuse->user_data,&before,sizeof(before)));++cases;++negative_cases;HSD_GObjFree(reuse);
}

int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();HSD_GObj* object=create();
 clear_all(object);controls(object);cleanup(object);absent_indices(object);rejection(object);CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_helper_calls\":%u,\"negative_cases\":%u}\n",checks,cases,pairs,vm_calls,negative_cases);return 0;
}
