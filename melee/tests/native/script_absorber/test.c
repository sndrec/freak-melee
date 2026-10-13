#include "boundary.h"
#include <melee/ft/native_script_binding_absorber.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_damage.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdat.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
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
static Fighter fighters[64];static FighterBone parts[MAX_FT_PARTS];static HSD_JObj joints[MAX_FT_PARTS];static MotionState motions[351];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
HSD_GObjEvent ftData_OnAbsorb[Ft_Kind_RuntimeCapacity];
static unsigned allocated,checks,cases,pairs,vm_calls,completed_tails,stock_calls;
static HSD_GObj* mutation_actor;static unsigned mutation;
static int hp_slot,hp_port,hp_value,stat_slot,stat_port,stat_value,hp_calls,stat_calls;
static uint8_t snapshot[65536];
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"absorber line%u %s\n",__LINE__,#x);exit(1);}}while(0)

const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&package:NULL;}
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
 (void)s;const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(id);
 if(n||!descriptor||!(descriptor->flags&NativeScriptFlag_Readable))return false;
 return nativeScriptBindingAbsorber_Read(c,id,out,d);
}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* s,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d)
{
 (void)s;if(n)return false;
 if(id!=NativeScriptBinding_PositionX)return nativeScriptBindingAbsorber_Write(c,id,v,d);
 if(v->type!=NativeScriptType_F32)return false;
 ((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x=v->as.f32;
 if(mutation_actor==c->source) {
  HSD_GObj* object=mutation_actor;mutation_actor=NULL;
  if(mutation==1)return false;
  if(mutation==2)HSD_GObjFree(object);
  if(mutation==3){Fighter* f=object->user_data;++f->x8_spawnNum;nativeScriptFighter_NewLife(object,f->x8_spawnNum);}
  if(mutation==4)++bank.storage_layout.identity;
 }
 return true;
}
void Player_SetHPByIndex(s32 slot,s32 port,s32 value){hp_slot=slot;hp_port=port;hp_value=value;++hp_calls;}
void pl_80040B8C(int slot,int port,int value){stat_slot=slot;stat_port=port;stat_value=value;++stat_calls;}
void Fighter_TakeDamage_8006CC7C(Fighter* fighter,float amount){(void)fighter;(void)amount;abort();}
#include "heal_dispatch.inc"
#include "native_absorber.inc"
#include "native_absorb_schedule.inc"
static void stock_absorb(HSD_GObj* object){(void)object;++stock_calls;}
static void userdata_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void prepare(unsigned mode)
{
 memset(&package,0,sizeof(package));bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* common="fighter:\n    behavior independent\nstate int seen_hits\nstate int seen_damage\nstate float seen_direction\nstate int tail\nstate bool active\nstate bool hit\nstate bool skip\nfunc Create(int bone, float radius):\n    absorber.create(bone, -0.0, 2.25, -3.5, radius)\nfunc Query():\n    state.active = absorber.active\n    state.hit = absorber.hit\n    state.skip = absorber.skip_position_update\n    state.seen_hits = absorber.hits\n    state.seen_damage = absorber.damage\n    state.seen_direction = absorber.direction\nfunc Flags():\n    absorber.active = false\n    absorber.hit = true\n    absorber.skip_position_update = false\nfunc Heal(float amount):\n    damage.heal(amount)\n";
 const char* hook="    on absorb:\n        state.seen_hits = absorber.hits\n        state.seen_damage = absorber.damage\n        state.seen_direction = absorber.direction\n        position.x = 1\n        state.tail = state.tail + 1\n";
 const char* recipient="action Recipient:\n    on cpu_threat:\n        state.seen_damage = other.absorber.damage\n        other.absorber.active = false\n        return other.absorber.hit\n";
 char source[8192];snprintf(source,sizeof(source),"%s%s%s%s",common,recipient,mode==1?"action Absorption:\n":mode==2?"move Fireball:\n":"",mode?hook:"");
 NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&package.program,&d)){
  fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);
 }
 package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;
 package.assets.motions=motions;package.assets.facts.moves=layout;package.assets.facts.joints=MAX_FT_PARTS;
 package.assets.parts.parts_num=MAX_FT_PARTS;package.assets.parts.part_to_joint=package.assets.part_to_joint;package.assets.parts.joint_to_part=package.assets.joint_to_part;
 gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
 tables[Ft_Kind_CustomFirst]=&package.assets.parts;
 for(unsigned i=0;i<MAX_FT_PARTS;++i)parts[i].joint=&joints[i];
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&package.program);
 ftData_OnAbsorb[Ft_Kind_CustomFirst]=stock_absorb;
}
static HSD_GObj* create(void)
{
 CHECK(allocated<64);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
 Fighter* f=&fighters[allocated++];memset(f,0,sizeof(*f));f->gobj=object;f->kind=f->behavior_kind=Ft_Kind_CustomFirst;f->ft_data=&package.assets.character.data;f->x8_spawnNum=allocated;
 f->parts=parts;f->player_id=2;f->x221F_b4=1;GObj_InitUserData(object,4,userdata_remove,f);CHECK(nativeScriptFighter_Attach(object));return object;
}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{return (NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)object->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue state(HSD_GObj* object,const char* name)
{
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view));
 for(unsigned i=0;i<view.program->value_count;++i)if(!view.program->values[i].owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,name)){
  NativeScriptValue result;NativeScriptDiagnostic d={0};CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&result,&d));return result;
 }CHECK(false);return(NativeScriptValue){0};
}
static bool helper(HSD_GObj* object,const char* name,const NativeScriptValue* args,uint32_t count)
{
 ++vm_calls;for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name)){
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=123};NativeScriptDiagnostic d={0};
  bool ok=nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,i,args,count,&out,&d)==NativeScriptVM_Ok;
  if(ok)CHECK(out.type==NativeScriptType_Void);else CHECK(out.type==NativeScriptType_U64&&out.as.u64==123);return ok;
 }CHECK(false);return false;
}
static void create_pairs(HSD_GObj* object)
{
 Fighter* f=object->user_data;static Fighter before,expected;
 const int bones[]={-1,0,43,104,MAX_FT_PARTS-1,MAX_FT_PARTS,INT32_MAX};
 const float radii[]={-0.0f,-7.25f,4.5f,INFINITY,-INFINITY,NAN};
 for(unsigned b=0;b<sizeof(bones)/sizeof(*bones);++b)for(unsigned r=0;r<sizeof(radii)/sizeof(*radii);++r){
  ++cases;f->x2218_b6=0;f->x2218_b7=1;f->absorb_hit.skip_update_pos=true;f->AbsorbAttr.x1A44_damageTaken=71;f->AbsorbAttr.x1A48_hitsTaken=2;
  before=*f;AbsorbDesc shape={.x0_bone_id=bones[b],.x4_offset={-0.0f,2.25f,-3.5f},.x10_size=radii[r]};
  if(bones[b]>=0&&bones[b]<MAX_FT_PARTS)ftColl_CreateAbsorbHit(object,&shape);expected=*f;*f=before;
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=bones[b]},{.type=NativeScriptType_F32,.as.f32=radii[r]}};
  CHECK(helper(object,"Create",args,2));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
 }
}
static void queries(HSD_GObj* object)
{
 ++cases;Fighter* f=object->user_data;f->x2218_b6=true;f->x2218_b7=true;f->absorb_hit.skip_update_pos=true;
 f->AbsorbAttr.x1A44_damageTaken=INT32_MIN;f->AbsorbAttr.x1A48_hitsTaken=INT32_MAX;f->AbsorbAttr.x1A40_absorbHitDirection=-1;
 CHECK(helper(object,"Query",NULL,0));CHECK(state(object,"active").as.u64==1&&state(object,"hit").as.u64==1&&state(object,"skip").as.u64==1);
 CHECK(state(object,"seen_damage").as.i64==INT32_MIN&&state(object,"seen_hits").as.i64==INT32_MAX&&state(object,"seen_direction").as.f32==-1);
 CHECK(helper(object,"Flags",NULL,0));CHECK(!f->x2218_b6&&f->x2218_b7&&!f->absorb_hit.skip_update_pos);
 NativeScriptRuntimeContext c=context(object);NativeScriptValue v={.type=NativeScriptType_Bool,.as.u64=2};NativeScriptDiagnostic d;
 static Fighter before;before=*f;CHECK(!nativeScriptBindingAbsorber_Write(&c,NativeScriptBinding_AbsorberActive,&v,&d));CHECK(!memcmp(f,&before,sizeof(*f)));
 v.as.u64=0;CHECK(!nativeScriptBindingAbsorber_Write(&c,NativeScriptBinding_AbsorberDamage,&v,&d));
}
static void healing(HSD_GObj* object)
{
 const float amounts[]={0,2.75f,10,25,-3.5f};Fighter* f=object->user_data;
 for(unsigned i=0;i<5;++i){++cases;f->dmg.x1830_percent=10;hp_calls=stat_calls=0;nativeFighterDamage_Heal(object,amounts[i]);float expected=f->dmg.x1830_percent;int hp=hp_value,stat=stat_value;
  f->dmg.x1830_percent=10;hp_calls=stat_calls=0;NativeScriptValue a={.type=NativeScriptType_F32,.as.f32=amounts[i]};CHECK(helper(object,"Heal",&a,1));
  CHECK(f->dmg.x1830_percent==expected&&hp_value==hp&&stat_value==stat&&hp_calls==1&&stat_calls==1&&hp_slot==2&&stat_slot==2&&hp_port==1&&stat_port==1);++pairs;
 }
}
static void negative(HSD_GObj* object)
{
 NativeScriptRuntimeContext c=context(object);NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=104},{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_F32,.as.f32=2},{.type=NativeScriptType_F32,.as.f32=3},{.type=NativeScriptType_F32,.as.f32=4}};
 Fighter* f=object->user_data;static Fighter before;before=*f;
 for(unsigned kind=0;kind<10;++kind){++cases;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=123};NativeScriptDiagnostic d={0};
  switch(kind){case 0:++c.generation;break;case 1:++c.scene_epoch;break;case 2:c.source_class=NativeCallbackSource_Item;break;case 3:parts[104].joint=NULL;break;
   case 4:tables[f->kind]=NULL;break;case 5:package.assets.parts.parts_num=0;break;case 6:package.assets.facts.joints=1;break;case 7:package.address=NULL;break;case 8:args[0].type=NativeScriptType_F32;break;case 9:args[2].type=NativeScriptType_F64;break;}
  CHECK(!nativeScriptBindingAbsorber_Invoke(&c,NativeScriptBinding_AbsorberCreate,args,5,&out,&d));CHECK(out.type==NativeScriptType_U64&&out.as.u64==123&&!memcmp(f,&before,sizeof(*f)));
  c=context(object);parts[104].joint=&joints[104];tables[f->kind]=&package.assets.parts;package.assets.parts.parts_num=MAX_FT_PARTS;package.assets.facts.joints=MAX_FT_PARTS;package.address=&package;args[0].type=NativeScriptType_I32;args[0].as.i64=104;args[2].type=NativeScriptType_F32;
 }
}
static void scheduler_cases(HSD_GObj* object)
{
 Fighter* f=object->user_data;++cases;f->AbsorbAttr.x1A40_absorbHitDirection=-1;f->AbsorbAttr.x1A44_damageTaken=73;f->AbsorbAttr.x1A48_hitsTaken=3;
 unsigned tails=completed_tails;absorber_schedule(object);CHECK(completed_tails==tails+1&&stock_calls==0);
 CHECK(state(object,"seen_damage").as.i64==73&&state(object,"seen_hits").as.i64==3&&state(object,"seen_direction").as.f32==-1&&state(object,"tail").as.i64==1);
 CHECK(f->AbsorbAttr.x1A40_absorbHitDirection==0&&f->AbsorbAttr.x1A44_damageTaken==0&&f->AbsorbAttr.x1A48_hitsTaken==0);
 size_t size;CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&size));static Fighter peer;peer=*f;
 f->AbsorbAttr.x1A40_absorbHitDirection=1;f->AbsorbAttr.x1A44_damageTaken=2;f->AbsorbAttr.x1A48_hitsTaken=1;absorber_schedule(object);CHECK(state(object,"tail").as.i64==2);
 *f=peer;CHECK(nativeScriptFighter_Load(snapshot,size));CHECK(state(object,"tail").as.i64==1);
 for(unsigned kind=1;kind<=4;++kind){++cases;HSD_GObj* target=create();Fighter* data=target->user_data;data->AbsorbAttr.x1A40_absorbHitDirection=1;data->AbsorbAttr.x1A44_damageTaken=9;data->AbsorbAttr.x1A48_hitsTaken=2;
  mutation_actor=target;mutation=kind;tails=completed_tails;uint64_t identity=bank.storage_layout.identity;absorber_schedule(target);CHECK(completed_tails==tails);bank.storage_layout.identity=identity;
  if(kind==2)continue;
  CHECK(data->AbsorbAttr.x1A40_absorbHitDirection==1&&data->AbsorbAttr.x1A44_damageTaken==9&&data->AbsorbAttr.x1A48_hitsTaken==2);
  // Position is a pure field-write boundary in production. Artificial native
  // generation/schema mutations here prove the retained native continuation,
  // not a MayInvalidate intrinsic's per-instruction termination contract.
  CHECK(state(target,"tail").as.i64==(kind==4?1:0));HSD_GObjFree(target);
 }
}
static void foreign_fields(HSD_GObj* caller)
{
 for(unsigned owned=0;owned<2;++owned){++cases;
  FighterKind kind=owned?Ft_Kind_CustomFirst+1:Ft_Kind_Fox;
  if(owned){CHECK(nativeScriptFighter_CanPublish(kind));nativeScriptFighter_PublishProgram(kind,&package.program);}
  HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);Fighter* f=&fighters[allocated++];memset(f,0,sizeof(*f));
  f->gobj=object;f->kind=f->behavior_kind=kind;f->x8_spawnNum=allocated;GObj_InitUserData(object,4,userdata_remove,f);CHECK(nativeScriptFighter_Attach(object));
  CHECK(nativeScriptFighter_IsAttached(object)==(bool)owned);f->AbsorbAttr.x1A44_damageTaken=-991;f->x2218_b6=f->x2218_b7=true;
  NativeScriptValue token,out={.type=NativeScriptType_U64,.as.u64=123};NativeScriptDiagnostic d={0};
  CHECK(nativeScriptReceiver_CaptureExternal(object,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&token,&d));
  CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_CpuThreat,&token,1, NULL,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Bool&&out.as.u64==1);
  CHECK(!f->x2218_b6&&state(caller,"seen_damage").as.i64==-991);
  NativeScriptRuntimeContext c=context(object);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};
  CHECK(nativeScriptBindingAbsorber_Read(&c,NativeScriptBinding_AbsorberHit,&out,&d)&&out.type==NativeScriptType_Bool&&out.as.u64==1);
  NativeScriptValue flag={.type=NativeScriptType_Bool,.as.u64=0};CHECK(nativeScriptBindingAbsorber_Write(&c,NativeScriptBinding_AbsorberHit,&flag,&d)&&!f->x2218_b7);
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32}};
  out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=123};CHECK(!nativeScriptBindingAbsorber_Invoke(&c,NativeScriptBinding_AbsorberCreate,args,5,&out,&d)&&out.type==NativeScriptType_U64&&out.as.u64==123);
  object->classifier=HSD_GOBJ_CLASS_ITEM;
  CHECK(!nativeScriptBindingAbsorber_Read(&c,NativeScriptBinding_AbsorberHit,&out,&d)&&out.type==NativeScriptType_U64&&out.as.u64==123);
  object->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(object);
 }
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare(1);HSD_GObj* object=create();
 CHECK(nativeScriptFighter_HasHook(object,NativeScriptFighter_Absorb));create_pairs(object);queries(object);healing(object);negative(object);scheduler_cases(object);foreign_fields(object);HSD_GObjFree(object);
 prepare(0);object=create();CHECK(!nativeScriptFighter_HasHook(object,NativeScriptFighter_Absorb));Fighter* f=object->user_data;f->AbsorbAttr.x1A40_absorbHitDirection=1;f->AbsorbAttr.x1A44_damageTaken=4;f->AbsorbAttr.x1A48_hitsTaken=2;
 unsigned tails=completed_tails;absorber_schedule(object);CHECK(completed_tails==tails+1&&stock_calls==0&&f->AbsorbAttr.x1A44_damageTaken==0);HSD_GObjFree(object);++cases;
 prepare(2);object=create();CHECK(nativeScriptFighter_HasHook(object,NativeScriptFighter_Absorb));f=object->user_data;f->AbsorbAttr.x1A40_absorbHitDirection=1;f->AbsorbAttr.x1A44_damageTaken=12;f->AbsorbAttr.x1A48_hitsTaken=1;absorber_schedule(object);CHECK(state(object,"seen_damage").as.i64==12);HSD_GObjFree(object);++cases;
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_helper_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
