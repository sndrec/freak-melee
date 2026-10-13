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
#include <melee/mp/mpcoll.h>
#include <melee/ft/native_fighter_relations.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftwalljump.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>


#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_fighter_geometry.h>
#include <melee/gr/stage.h>



static NativeScriptDiagnostic diagnostic;static Fighter fighters[16];static MotionState motions[342];static unsigned allocated,checks,cases,pairs,vm_calls;
void presentation_check(bool ok,const char* expression,unsigned line)
{++checks;if(!ok){fprintf(stderr,"collision copy %u: %s (%u:%u %s)\n",line,expression,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return source.ready&&source.target==kind?&source:NULL;}
static void configure(void)
{
 source.compiler.bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=call_graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void prepare(void)
{
 configure();const char* authored="fighter:\n    behavior independent\nstate int samples\n"
 "func Copy(int id) -> bool:\n    return collision.copy_from(id)\n"
 "func Sample(int id) -> int:\n    state.samples = state.samples + 1\n    return id\n"
 "func SampleCopy(int id) -> bool:\n    return collision.copy_from(Sample(id))\n"
 "func Discard(int id):\n    collision.copy_from(int(id))\n"
 "func Local(int id) -> bool:\n    let bool copied = collision.copy_from(id)\n    return copied\n"
 "func StandardBox(float top, float bottom, float left_x, float left_y, float right_x, float right_y):\n    collision.ground_standard_box(top, bottom, left_x, left_y, right_x, right_y)\n"
 "func Fault():\n    let int zero = 0\n    let int crash = 1 / zero\n";
 source.assets.facts.moves=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=authored,.source_bytes=(uint32_t)strlen(authored),.motion_layout=&source.assets.facts.moves};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&source.compiler.bank,&request,&accepted,&diagnostic));source.target=Ft_Kind_CustomFirst;
 CHECK(nativeScriptFighter_PrepareProgram(source.target,accepted,&source.assets.facts.moves,NULL,&source.program,&diagnostic));
 source.address=&source;source.ready=source.assets.ready=true;source.assets.motions=motions;gFtDataList[source.target]=&source.assets.character.data;ftData_CharacterStateTables[source.target]=source.assets.motions;
 CHECK(nativeScriptFighter_CanPublish(source.target));nativeScriptFighter_PublishProgram(source.target,&source.program);
}
static void remove_fighter(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* create(bool owned,uint32_t id)
{
 CHECK(allocated<16);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);Fighter* f=&fighters[allocated++];
 *f=(Fighter){.gobj=o,.kind=owned?source.target:Ft_Kind_Mario,.behavior_kind=owned?source.target:Ft_Kind_Mario,.x8_spawnNum=(int32_t)id,.ft_data=&source.assets.character.data};
 GObj_InitUserData(o,4,remove_fighter,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue integer(int64_t id){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=id};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xabcdef123456789)};}
static uint32_t function(const char* name)
{for(unsigned n=0;n<source.compiler.bank.declarations.function_count;++n)if(nativeScriptSource_SpanEquals(&source.compiler.bank.tree,declarations[n].name,name))return n;CHECK(false);return UINT32_MAX;}
static NativeScriptVMStatus helper(HSD_GObj* o,const char* name,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out)
{++vm_calls;return nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,function(name),args,count,out,&diagnostic);}
static void compare(HSD_GObj* caller,HSD_GObj* target)
{
 Fighter* from=target->user_data;Fighter* to=caller->user_data;NativeScriptRuntimeContext c=context(caller);
 for(unsigned seed=1;seed<=256;++seed){CollData* src=&from->coll_data;CollData* dst=&to->coll_data;
  memset(src,0x20+seed%64,sizeof(*src));memset(dst,0x80+seed%64,sizeof(*dst));src->cur_pos=(Vec3){seed,seed*-0.5f,NAN};src->floor.index=(int)seed;src->left_facing_wall.index=-1;src->right_facing_wall.index=37;src->ceiling.index=INT32_MAX;
  CollData retained=*dst,source_before=*src;Fighter original=*to;CHECK(nativeFighterRelations_CopyCollision(caller,(uint32_t)from->x8_spawnNum));Fighter expected=*to;*to=original;
  NativeScriptValue arg=integer(from->x8_spawnNum),out=sentinel();CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,NULL,0,&arg,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==1);
  CHECK(!memcmp(to,&expected,sizeof(expected))&&!memcmp(src,&source_before,sizeof(source_before)));
#define COPIED(field) CHECK(!memcmp(&dst->field,&src->field,sizeof(dst->field)))
  COPIED(cur_pos);COPIED(prev_pos);COPIED(last_pos);COPIED(facing_dir);COPIED(x38);COPIED(floor_skip);COPIED(ledge_id_left);COPIED(ledge_id_right);COPIED(joint_id_skip);COPIED(lstick_x);
  COPIED(x64_ecb);COPIED(desired_ecb);COPIED(ecb);COPIED(prev_ecb);COPIED(xE4_ecb);COPIED(x130_flags);COPIED(env_flags);COPIED(prev_env_flags);COPIED(x13C);COPIED(contact);COPIED(floor);COPIED(left_facing_wall);COPIED(right_facing_wall);COPIED(ceiling);
#undef COPIED
  CHECK(dst->x34_flags.b0==src->x34_flags.b0&&dst->x34_flags.b1234==src->x34_flags.b1234&&dst->x34_flags.b5==src->x34_flags.b5&&dst->x34_flags.b6==src->x34_flags.b6&&dst->x34_flags.b7==retained.x34_flags.b7);
#define RETAINED(field) CHECK(!memcmp(&dst->field,&retained.field,sizeof(dst->field)))
  RETAINED(x0_gobj);RETAINED(x28_vec);RETAINED(x35_flags);RETAINED(joint_id_only);RETAINED(x50);RETAINED(ledge_snap_x);RETAINED(ledge_snap_y);RETAINED(ledge_snap_height);RETAINED(ecb_source);
#undef RETAINED
  ++pairs;++cases;
 }
 const char* names[]={"Copy","SampleCopy","Local","Discard"};
 for(unsigned n=0;n<4;++n){Fighter original=*to;memset(&to->coll_data,0x81+n,sizeof(CollData));Fighter expected=*to;mpCopyCollData(&from->coll_data,&expected.coll_data,2);NativeScriptValue arg=integer(from->x8_spawnNum),out=sentinel();
  CHECK(helper(caller,names[n],&arg,1,&out)==NativeScriptVM_Ok);CHECK(n==3?out.type==NativeScriptType_Void:out.type==NativeScriptType_Bool&&out.as.u64==1);CHECK(!memcmp(to,&expected,sizeof(expected)));++pairs;++cases;*to=original;
 }
 static unsigned samples;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(caller,c.generation,&view));NativeScriptValue sampled;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC,NULL,0,&sampled,&diagnostic)&&sampled.as.i64==++samples);
}
static void defaults_and_ids(HSD_GObj* caller,HSD_GObj* target)
{
 Fighter* from=target->user_data;Fighter* to=caller->user_data;const uint32_t ids[]={1,16777217,0x80000001,UINT32_MAX};NativeScriptRuntimeContext c=context(caller);
 for(unsigned n=0;n<4;++n){from->x8_spawnNum=(int32_t)ids[n];Fighter expected=*to;mpCopyCollData(&from->coll_data,&expected.coll_data,2);NativeScriptValue arg=integer((int32_t)ids[n]),out=sentinel();
  CHECK(nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,&arg,1,&out,&diagnostic)&&out.as.u64==1&&!memcmp(to,&expected,sizeof(expected)));++pairs;++cases;
 }
 // Caller, zero and absent ID have exact native self/missing semantics.
 const uint32_t missing[]={0,0x76543210,(uint32_t)to->x8_spawnNum};
 for(unsigned n=0;n<3;++n){Fighter before=*to;NativeScriptValue arg=integer((int32_t)missing[n]),out=sentinel();CHECK(helper(caller,"Copy",&arg,1,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Bool&&out.as.u64==(n==2)&&!memcmp(to,&before,sizeof(before)));++cases;}
 // Address reuse does not make an old ID current.
 NativeScriptValue stale=integer(from->x8_spawnNum);++from->x8_spawnNum;NativeScriptValue out=sentinel();Fighter before=*to;
 CHECK(nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,&stale,1,&out,&diagnostic)&&!out.as.u64&&!memcmp(to,&before,sizeof(before)));++cases;
}
static void rejected(HSD_GObj* caller,HSD_GObj* target)
{
 Fighter* to=caller->user_data;Fighter* from=target->user_data;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(caller,(uint32_t)to->x8_spawnNum,&view));NativeScriptStorageInstance storage=*view.storage;
 for(unsigned bad=0;bad<26;++bad){NativeScriptRuntimeContext c=context(caller);NativeScriptValue arg=integer(from->x8_spawnNum),out=sentinel(),old=out;Fighter saved=*to;uint64_t epoch=nativeGObjPool.lifetime;uint32_t count=1;
  switch(bad){case 0:arg.type=NativeScriptType_Bool;break;case 1:arg.type=NativeScriptType_F32;break;case 2:arg.type=NativeScriptType_U32;break;case 3:arg.type=NativeScriptType_I64;break;case 4:arg.type=NativeScriptType_U64;break;case 5:arg.as.i64=(int64_t)INT32_MAX+1;break;case 6:arg.as.i64=(int64_t)INT32_MIN-1;break;case 7:count=0;break;case 8:count=2;break;case 9:++c.generation;break;case 10:++c.scene_epoch;break;case 11:c.source_class=NativeCallbackSource_Item;break;case 12:c.source=NULL;break;case 13:caller->classifier=HSD_GOBJ_CLASS_ITEM;break;case 14:caller->user_data=NULL;break;case 15:to->gobj=target;break;case 16:to->ft_data=NULL;break;case 17:view.storage->live=false;break;case 18:++view.storage->owner;break;case 19:++view.storage->generation;break;case 20:view.storage->layout=NULL;break;case 21:view.storage->parameters=NULL;break;case 22:source.ready=false;break;case 23:ftData_CharacterStateTables[source.target]=NULL;break;case 24:++nativeGObjPool.lifetime;break;case 25:to->kind=Ft_Kind_Mario;break;}
  Fighter before=*to,peer_before=*from;bool accepted=nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,&arg,count,&out,&diagnostic);if(accepted)fprintf(stderr,"Accepted invalid source case %u\n",bad);CHECK(!accepted);CHECK(!memcmp(&out,&old,sizeof(out))&&!memcmp(to,&before,sizeof(before))&&!memcmp(from,&peer_before,sizeof(peer_before)));
  *to=saved;caller->user_data=to;caller->classifier=HSD_GOBJ_CLASS_FIGHTER;*view.storage=storage;nativeGObjPool.lifetime=epoch;source.ready=true;ftData_CharacterStateTables[source.target]=source.assets.motions;++cases;
 }
 const uint8_t wrong[]={NativeScriptType_Bool,NativeScriptType_F32,NativeScriptType_U32,NativeScriptType_I64};
 for(unsigned n=0;n<4;++n){NativeScriptRuntimeContext c=context(caller);NativeScriptValue arg=integer(from->x8_spawnNum),out=sentinel(),old=out;arg.type=wrong[n];Fighter before=*to;
  CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,NULL,0,&arg,1,&out,&diagnostic)&&!memcmp(&out,&old,sizeof(out))&&!memcmp(to,&before,sizeof(before)));++cases;
 }
 NativeScriptRuntimeContext c=context(caller);NativeScriptValue arg=integer(from->x8_spawnNum),out=sentinel();CHECK(!nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,NULL,1,&out,&diagnostic));CHECK(!nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,&arg,1,NULL,&diagnostic));++cases;
}
static void peer_failures(HSD_GObj* caller,HSD_GObj* peer)
{
 Fighter* target=peer->user_data;Fighter* to=caller->user_data;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(peer,(uint32_t)target->x8_spawnNum,&view));NativeScriptStorageInstance storage=*view.storage;
 for(unsigned n=0;n<6;++n){NativeScriptRuntimeContext c=context(caller);NativeScriptValue arg=integer(target->x8_spawnNum),out=sentinel(),old=out;Fighter saved=*target;
  switch(n){case 0:target->gobj=caller;break;case 1:view.storage->live=false;break;case 2:++view.storage->generation;break;case 3:++view.storage->owner;break;case 4:view.storage->layout=NULL;break;case 5:view.storage->parameters=NULL;break;}
  Fighter before=*to;CHECK(!nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,&arg,1,&out,&diagnostic)&&!memcmp(&out,&old,sizeof(out))&&!memcmp(to,&before,sizeof(before)));*target=saved;*view.storage=storage;++cases;
 }
 NativeScriptValue out=sentinel();CHECK(helper(peer,"Fault",NULL,0,&out)==NativeScriptVM_Error);NativeScriptRuntimeContext c=context(caller);NativeScriptValue arg=integer(target->x8_spawnNum),old=sentinel();out=old;Fighter before=*to;
 CHECK(!nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,&arg,1,&out,&diagnostic)&&!memcmp(&out,&old,sizeof(out))&&!memcmp(to,&before,sizeof(before)));++cases;
}
static void retired(HSD_GObj* caller)
{
 HSD_GObj* peer=create(false,0x12345678);NativeScriptValue arg=integer(0x12345678),out=sentinel();HSD_GObjFree(peer);Fighter before=*(Fighter*)caller->user_data;NativeScriptRuntimeContext c=context(caller);
 CHECK(nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,&arg,1,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&!out.as.u64&&!memcmp(caller->user_data,&before,sizeof(before)));++cases;
 // Unregistered poisoned memory must be rejected without inspecting userdata.
 HSD_GObj* dead=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(dead);DWORD old;CHECK(VirtualProtect(dead,4096,PAGE_NOACCESS,&old));c.source=dead;out=sentinel();NativeScriptValue saved=out;
 CHECK(!nativeScriptBindingCollision_Invoke(&c,NativeScriptBinding_CollisionCopyFrom,&arg,1,&out,&diagnostic)&&!memcmp(&out,&saved,sizeof(out)));CHECK(VirtualFree(dead,0,MEM_RELEASE));++cases;
}
static void compiler_negatives(void)
{
 const char* bad[]={"collision.copy_from(true)","collision.copy_from(1.5)","collision.copy_from()","collision.copy_from(1, 2)","collision.copy_from(\"bad\")"};NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
 for(unsigned n=0;n<5;++n){char invalid[512];snprintf(invalid,sizeof(invalid),"fighter:\n    behavior independent\nfunc Bad() -> bool:\n    return %s\n",bad[n]);NativeScriptCompileRequest request={.source=invalid,.source_bytes=(uint32_t)strlen(invalid)};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&!scratch.bank.valid);++cases;}nativeSourceBank_Release(&scratch);
}
// Unselected collision leaves abort. History copy and custom standard ground
// response execute their unchanged native bodies in this cohort.
void HSD_JObjSetMtxDirtySub(HSD_JObj* j){(void)j;abort();}
u32 HSD_JObjGetFlags(HSD_JObj* j){(void)j;abort();}
void HSD_JObjClearFlags(HSD_JObj* j,u32 flags){(void)j;(void)flags;abort();}
void lb_8000B1CC(HSD_JObj* j,Vec3* a,Vec3* b){(void)j;(void)a;(void)b;abort();}
void ft_80084280(Fighter_GObj* o){(void)o;abort();}
void ft_80084104(Fighter_GObj* o){(void)o;abort();}
void ft_80083B68(Fighter_GObj* o){(void)o;abort();}
bool ft_80081A00(Fighter_GObj* o){(void)o;abort();}
bool ftWallJump_8008169C(HSD_GObj* o){(void)o;abort();}
bool ftCliffCommon_80081298(Fighter_GObj* o){(void)o;abort();}
void ftCo_LandingFallSpecial_Enter(Fighter_GObj* o,bool b,float f){(void)o;(void)b;(void)f;abort();}
bool ftCo_80096CC8(Fighter_GObj* o,int i){(void)o;(void)i;abort();}
bool mpColl_80047AC8(CollData* c,bool (*f)(Fighter_GObj*,enum_t),Fighter_GObj* o){(void)c;(void)f;(void)o;abort();}
bool mpColl_80047E14(CollData* c,bool (*f)(Fighter_GObj*,enum_t),Fighter_GObj* o){(void)c;(void)f;(void)o;abort();}
void ftCo_AirCatchHit_Coll(Fighter_GObj* o){(void)o;abort();}
bool nativeFighterGeometry_ProbeLedge(Fighter_GObj* o,float x,float y){(void)o;(void)x;(void)y;abort();}
Fighter_GObj* ft_80082E3C(Fighter_GObj* o){(void)o;abort();}
StKind Stage_80225194(void){abort();}
#include "standard_box_cases.h"
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();compiler_negatives();
 HSD_GObj* caller=create(true,0x11223344);HSD_GObj* stock=create(false,0x80000001);standard_box_cases(caller,stock);compare(caller,stock);defaults_and_ids(caller,stock);rejected(caller,stock);retired(caller);
 HSD_GObj* peer=create(true,0x66778899);compare(caller,peer);peer_failures(caller,peer);HSD_GObjFree(peer);HSD_GObjFree(stock);HSD_GObjFree(caller);CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS collision copy\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"actor_vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
