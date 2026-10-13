#include "boundary.h"
#include <melee/ft/native_script_binding_hitlag.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_relations.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum { N=2048,F=64,V=64 };
static char bytes[32768];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeSourcePackage package;
#define bank package.compiler.bank
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=10,.ground={343,345,347,349},.air={344,346,348,350}};
static Fighter fighters[32];static MotionState motions[351];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static unsigned allocated,checks,cases,pairs,vm_calls,negative_cases;
static uint8_t snapshot[65536];
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"hitlag line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&package:NULL;}
#include "native_hitlag.inc"
static void userdata_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* bad[]={"hitlag.link(1.5)","hitlag.link()","hitlag.link(1, 2)","hitlag.unlink(1)","hitlag.request_release(1)","hitlag.active = false","hitlag.linked_id = 1","hitlag.unknown()"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){
  char invalid[512];snprintf(invalid,sizeof(invalid),"fighter:\n    behavior independent\nfunc Bad():\n    %s\n",bad[i]);
  NativeScriptCompileRequest request={.source=invalid,.source_bytes=(uint32_t)strlen(invalid),.motion_layout=&layout};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
  CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)&&!accepted);++cases;++negative_cases;
 }
 const char* source="fighter:\n    behavior independent\nfunc Link(uint peer) -> bool:\n    return hitlag.link(peer)\nfunc Unlink():\n    hitlag.unlink()\nfunc Release():\n    hitlag.request_release()\nfunc Active() -> bool:\n    return hitlag.active\nfunc Id() -> int:\n    return hitlag.linked_id\nexport func linked_release_and_unlink():\n    hitlag.request_release()\n    hitlag.unlink()\nfunc Fault():\n    let int zero = 0\n    let int crash = 1 / zero\n";
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&package.program,&d)){
  fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);
 }
 package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.motions=motions;package.assets.facts.moves=layout;
 gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&package.program);
}
static HSD_GObj* create(bool interpreted,uint32_t generation)
{
 CHECK(allocated<32);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);Fighter* f=&fighters[allocated++];memset(f,0,sizeof(*f));
 f->gobj=object;f->kind=f->behavior_kind=interpreted?Ft_Kind_CustomFirst:Ft_Kind_Mario;f->ft_data=&package.assets.character.data;f->x8_spawnNum=(int32_t)generation;
 GObj_InitUserData(object,4,userdata_remove,f);if(interpreted)CHECK(nativeScriptFighter_Attach(object));return object;
}
static uint32_t function_id(const char* name)
{for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name))return i;CHECK(false);return UINT32_MAX;}
static bool helper(HSD_GObj* object,const char* name,const NativeScriptValue* args,unsigned count,NativeScriptValue* out)
{
 ++vm_calls;NativeScriptDiagnostic d={0};bool result=nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function_id(name),args,count,out,&d)==NativeScriptVM_Ok;
 if(!result&&strcmp(name,"Fault"))fprintf(stderr,"helper %s: %s\n",name,d.message);return result;
}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)object->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x123456789abcdef)};}
static void references(HSD_GObj* object,HSD_GObj* peer)
{
 Fighter* f=object->user_data;Fighter* p=peer->user_data;static Fighter before,expected;NativeScriptValue out=sentinel();
 uint32_t ids[]={0,(uint32_t)f->x8_spawnNum,(uint32_t)p->x8_spawnNum,UINT32_C(0x7fffffee)};
 for(unsigned i=0;i<4;++i){
  f->x1A5C=peer;f->native_hitlag_link_generation=(uint32_t)p->x8_spawnNum;before=*f;
  bool linked=nativeFighterRelations_Link(object,ids[i]);expected=*f;*f=before;
  NativeScriptValue arg={.type=NativeScriptType_U32,.as.u64=ids[i]};
  CHECK(helper(object,"Link",&arg,1,&out)&&out.type==NativeScriptType_Bool&&out.as.u64==linked);
  CHECK(!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 }
 for(unsigned active=0;active<2;++active)for(unsigned entered=0;entered<2;++entered)for(unsigned pending=0;pending<2;++pending){
  f->x2219_b5=active;f->x2219_b7=entered;f->x221A_b1=pending;before=*f;
  Fighter_UnkSetFlag_8006CFBC(object);expected=*f;*f=before;
  CHECK(helper(object,"Release",NULL,0,&out)&&out.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f)));
  CHECK(helper(object,"Active",NULL,0,&out)&&out.type==NativeScriptType_Bool&&out.as.u64==active);++cases;++pairs;
 }
 f->x1A5C=peer;f->native_hitlag_link_generation=(uint32_t)p->x8_spawnNum;before=*f;
 uint32_t linked=nativeFighterRelations_LinkedId(object);CHECK(helper(object,"Id",NULL,0,&out)&&out.type==NativeScriptType_I32&&out.as.i64==(int32_t)linked);CHECK(!memcmp(f,&before,sizeof(*f)));++cases;++pairs;
 expected=before;expected.x1A5C=NULL;expected.native_hitlag_link_generation=0;
 CHECK(helper(object,"Unlink",NULL,0,&out)&&out.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 // The authored two-operation helper composes native request then unlink.
 f->x1A5C=peer;f->native_hitlag_link_generation=(uint32_t)p->x8_spawnNum;f->x2219_b7=true;before=*f;
 Fighter_UnkSetFlag_8006CFBC(object);CHECK(nativeFighterRelations_Link(object,0));expected=*f;*f=before;
 CHECK(helper(object,"linked_release_and_unlink",NULL,0,&out)&&!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
}
static void relation_defaults(HSD_GObj* object,HSD_GObj* peer)
{
 Fighter* f=object->user_data;Fighter* p=peer->user_data;NativeScriptValue out=sentinel();NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d={0};
 for(unsigned which=0;which<4;++which){
  f->x1A5C=which==0?NULL:peer;f->native_hitlag_link_generation=which==2?(uint32_t)p->x8_spawnNum+1:which==3?0:(uint32_t)p->x8_spawnNum;
  Fighter_GObj* pointer=f->x1A5C;uint32_t generation=f->native_hitlag_link_generation;
  CHECK(nativeScriptBindingHitlag_Read(&c,NativeScriptBinding_HitlagLinkedId,&out,&d));CHECK(out.as.i64==(which==1?(int32_t)p->x8_spawnNum:0));
  CHECK(f->x1A5C==pointer&&f->native_hitlag_link_generation==generation);++cases;
 }
 f->x1A5C=peer;f->native_hitlag_link_generation=(uint32_t)p->x8_spawnNum;uint32_t id=f->native_hitlag_link_generation;
 NativeMatchPoolReference prior;CHECK(nativeMatchPool_Reference(&nativeGObjPool,peer,&prior));HSD_GObjFree(peer);
 HSD_GObj* replacement=create(false,id+1);CHECK(nativeMatchPool_Resolve(prior)==NULL);
 CHECK(helper(object,"Id",NULL,0,&out)&&out.as.i64==0);CHECK(f->x1A5C==peer&&f->native_hitlag_link_generation==id);++cases;
 HSD_GObjFree(replacement);
}
static void negatives(HSD_GObj* object)
{
 Fighter* f=object->user_data;static Fighter before;NativeScriptDiagnostic d={0};
 NativeScriptValue bad[]={{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_Object,.as.u64=1},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000001)},{.type=NativeScriptType_I64,.as.i64=(int64_t)INT32_MIN-1},{.type=NativeScriptType_Bool,.as.u64=2},{.type=NativeScriptType_I32,.as.i64=(int64_t)INT32_MAX+1},{.type=NativeScriptType_U32,.as.u64=UINT64_C(0x100000001)}};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){NativeScriptRuntimeContext c=context(object);NativeScriptValue out=sentinel(),saved=out;before=*f;
  CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_HitlagLink,NULL,0,&bad[i],1,&out,&d));CHECK(!memcmp(f,&before,sizeof(*f))&&!memcmp(&out,&saved,sizeof(out)));++cases;++negative_cases;
 }
 for(unsigned which=0;which<10;++which)for(unsigned read=0;read<2;++read){
  NativeScriptRuntimeContext c=context(object);NativeScriptValue out=sentinel(),saved=out;void* data=object->user_data;
  switch(which){case 0:++c.generation;break;case 1:++c.scene_epoch;break;case 2:object->classifier=HSD_GOBJ_CLASS_ITEM;break;case 3:object->user_data=NULL;break;case 4:c.source_class=NativeCallbackSource_Item;break;case 5:package.ready=false;break;case 6:ftData_CharacterStateTables[f->kind]=NULL;break;case 7:f->ft_data=NULL;break;case 8:package.assets.facts.moves.count=9;break;case 9:c.source=NULL;break;}
  before=*f;bool ok=read?nativeScriptBindingHitlag_Read(&c,NativeScriptBinding_HitlagActive,&out,&d):nativeScriptBindingHitlag_Invoke(&c,NativeScriptBinding_HitlagUnlink,NULL,0,&out,&d);
  CHECK(!ok&&!memcmp(f,&before,sizeof(*f))&&!memcmp(&out,&saved,sizeof(out)));
  object->classifier=HSD_GOBJ_CLASS_FIGHTER;object->user_data=data;package.ready=true;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;f->ft_data=&package.assets.character.data;package.assets.facts.moves=layout;++cases;++negative_cases;
 }
 HSD_GObj* fault=create(true,90);NativeScriptValue out=sentinel();CHECK(!helper(fault,"Fault",NULL,0,&out));CHECK(nativeScriptFighter_Status(fault)==NativeScriptVM_Error);
 f->x1A5C=fault;f->native_hitlag_link_generation=90;NativeScriptRuntimeContext c=context(object);
 CHECK(nativeScriptBindingHitlag_Read(&c,NativeScriptBinding_HitlagLinkedId,&out,&d)&&out.as.i64==0);
 NativeScriptValue arg={.type=NativeScriptType_U32,.as.u64=90},saved=sentinel();out=saved;before=*f;
 CHECK(!nativeScriptBindingHitlag_Invoke(&c,NativeScriptBinding_HitlagLink,&arg,1,&out,&d));CHECK(!memcmp(f,&before,sizeof(*f))&&!memcmp(&out,&saved,sizeof(out)));++cases;++negative_cases;
 c=context(fault);out=saved;CHECK(!nativeScriptBindingHitlag_Read(&c,NativeScriptBinding_HitlagActive,&out,&d)&&!memcmp(&out,&saved,sizeof(out)));++cases;++negative_cases;HSD_GObjFree(fault);
}
static void snapshot_case(HSD_GObj* object,HSD_GObj* peer)
{
 Fighter* f=object->user_data;NativeScriptValue out=sentinel(),arg={.type=NativeScriptType_U32,.as.u64=(uint32_t)((Fighter*)peer->user_data)->x8_spawnNum};
 CHECK(helper(object,"Link",&arg,1,&out)&&out.as.u64);static Fighter retained;retained=*f;size_t used;
 CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&used));CHECK(helper(object,"Unlink",NULL,0,&out));
 // Native relation fields belong to the native Fighter snapshot; actor packed
 // state alone cannot restore them. Restore both peers explicitly here.
 *f=retained;CHECK(nativeScriptFighter_Load(snapshot,used));CHECK(helper(object,"Id",NULL,0,&out)&&out.as.i64==(int32_t)arg.as.u64);++cases;
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();
 HSD_GObj* object=create(true,1);HSD_GObj* peer=create(false,UINT32_C(0x80000011));references(object,peer);snapshot_case(object,peer);relation_defaults(object,peer);
 HSD_GObj* interpreted=create(true,30);references(object,interpreted);HSD_GObjFree(interpreted);negatives(object);
 HSD_GObjFree(object);CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_helper_calls\":%u,\"negative_cases\":%u}\n",checks,cases,pairs,vm_calls,negative_cases);return 0;
}
