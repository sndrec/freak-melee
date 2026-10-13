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
void ftCommon_8007F76C(Fighter_GObj* object){(void)object;abort();}
static uint8_t snapshot[65536];
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"hitbox line%u %s\n",__LINE__,#x);exit(1);}}while(0)
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&package:NULL;}
StaleMoveTable* Player_GetStaleMoveTableIndexPtr(s32 player)
{CHECK(player==2);++table_reads;return missing_table?NULL:&stale;}
#include "native_hitbox.inc"
static void userdata_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate int seen\nstate int tail\nfunc State(int slot) -> int:\n    return hitbox.state(slot)\nfunc IsDisabled(int slot) -> bool:\n    return hitbox.state(slot) == Hitbox.disabled\nfunc ReadDamage(int slot) -> float:\n    return hitbox.damage(slot)\nfunc Stamp():\n    state.seen = hitbox.state(0)\nfunc Damage(int slot, uint value):\n    hitbox.set_damage(slot, value)\nfunc DamageSigned(int slot, int value):\n    hitbox.set_damage(slot, value)\nfunc DamageFloat(int slot, float value):\n    hitbox.set_damage(slot, value)\nfunc Panic(int value):\n    if hitbox.state(0) == Hitbox.enabled: hitbox.set_damage(0, value)\n    if hitbox.state(1) == Hitbox.enabled: hitbox.set_damage(1, value)\n    if hitbox.state(2) == Hitbox.enabled: hitbox.set_damage(2, value)\n    if hitbox.state(3) == Hitbox.enabled: hitbox.set_damage(3, value)\nfunc Invalid():\n    hitbox.set_damage(4, 7)\n    state.tail = state.tail + 1\n";
 static char complete[4096];
 const char* groups="func ReadGroup(int slot) -> uint:\n    return hitbox.group(slot)\nfunc SetGroup(int slot, uint group):\n    hitbox.set_group(slot, group)\nfunc CycleGroup(int slot):\n    if hitbox.state(slot) != Hitbox.disabled:\n        hitbox.set_group(slot, (hitbox.group(slot) + 1) % 2)\n";
 int length=snprintf(complete,sizeof(complete),"%s%s",source,groups);CHECK(length>0&&(unsigned)length<sizeof(complete));
 NativeScriptCompileRequest request={.source=complete,.source_bytes=(uint32_t)length,.motion_layout=&layout};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
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
static void reset(Fighter* f,unsigned mode)
{
 f->player_id=2;f->x34_scale.y=1;f->x2068_attackID=41;f->x206C_attack_instance=567;
 f->smash_attrs.state=SmashState_None;f->smash_attrs.x2118_frames=17;f->smash_attrs.x211C_holdFrame=60;f->smash_attrs.x2120_damageMul=1.4f;
 scale_data.x4=1;DbLevel=DbLKind_NoDebugRom;memset(&stale,0,sizeof(stale));stale.current_index=3;
 for(unsigned i=0;i<9;++i)weights[i]=.025f+(float)i*.001f;
 if(mode&1){for(unsigned i=0;i<10;++i){stale.StaleMoves[i].move_id=(i&1)?41:12;stale.StaleMoves[i].attack_instance=i+1;}}
 for(unsigned i=0;i<4;++i){memset(&f->x914[i],0xA5,sizeof(f->x914[i]));f->x914[i].state=(i+mode)%5;}
 missing_table=false;Fighter_804D6524=&scale_data;Fighter_804D6548=weights;
}
static void state_cases(HSD_GObj* object)
{
 Fighter* f=object->user_data;
 for(unsigned slot=0;slot<4;++slot)for(unsigned state=0;state<=HitCapsule_Max;++state){++cases;f->x914[slot].state=state;NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=slot},out=sentinel();
  CHECK(helper(object,"State",&arg,1,&out)&&out.type==NativeScriptType_I32&&out.as.i64==(int)state);
  CHECK(helper(object,"IsDisabled",&arg,1,&out)&&out.type==NativeScriptType_Bool&&out.as.u64==(state==HitCapsule_Disabled));
 }
}
static void pair(HSD_GObj* object,unsigned slot,NativeScriptValue value,const char* helper_name)
{
 Fighter* f=object->user_data;static Fighter before,expected;static StaleMoveTable saved_stale;
 NativeScriptDiagnostic d={0};uint32_t word;
 // Independent exact native C conversion, exercised only in its defined
 // Float conversion domain; the backend uses the actual typed converter.
 switch(value.type){case NativeScriptType_I32:case NativeScriptType_I64:word=(uint32_t)value.as.i64;break;
  case NativeScriptType_U32:case NativeScriptType_U64:word=(uint32_t)value.as.u64;break;
  case NativeScriptType_F32:word=(uint32_t)value.as.f32;break;case NativeScriptType_F64:word=(uint32_t)value.as.f64;break;default:abort();}
 before=*f;saved_stale=stale;ftColl_8007ABD0(&f->x914[slot],word,object);expected=*f;*f=before;
 NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=slot},value},out=sentinel();
 if(helper_name)CHECK(helper(object,helper_name,args,2,&out));else {NativeScriptRuntimeContext c=context(object);CHECK(nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxSetDamage,args,2,&out,&d));}
 CHECK(out.type==NativeScriptType_Void&&!memcmp(f,&expected,sizeof(*f))&&!memcmp(&stale,&saved_stale,sizeof(stale)));++pairs;
}
static void damage_cases(HSD_GObj* object)
{
 Fighter* f=object->user_data;const float values[]={-.75f,0,1.75f,19.5f,100};const float scales[]={.25f,.75f,1,1.5f,2};
 const float scale_weights[]={-1,.5f,1,2};
 for(unsigned slot=0;slot<4;++slot)for(unsigned s=0;s<5;++s)for(unsigned w=0;w<4;++w)for(unsigned mode=0;mode<8;++mode)for(unsigned v=0;v<5;++v){
  ++cases;reset(f,mode);f->x34_scale.y=scales[s];scale_data.x4=scale_weights[w];
  if(mode&2)f->smash_attrs.state=SmashState_Release;if(mode&4)DbLevel=DbLKind_DebugRom;
  pair(object,slot,(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=values[v]},"DamageFloat");
 }
 // Integral narrowing remains modulo32, including negative bit patterns. A
 // native scale reduction makes these high-word examples a defined s32 count.
 const NativeScriptValue words[]={{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_U32,.as.u64=UINT32_MAX},
   {.type=NativeScriptType_I64,.as.i64=-INT64_C(4294967297)},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0xabcdef0100000007)}};
 for(unsigned i=0;i<4;++i){++cases;reset(f,0);f->x34_scale.y=.25f;pair(object,i,words[i],i==0?"DamageSigned":i==1?"Damage":NULL);}
 // Common attack ID1 bypasses stale weights, and DebugRom bypasses all tables.
 ++cases;reset(f,1);f->x2068_attackID=1;Fighter_804D6548=NULL;pair(object,0,(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=9},"Damage");
 ++cases;reset(f,1);DbLevel=DbLKind_DebugRom;Fighter_804D6548=NULL;missing_table=true;pair(object,1,(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=9},"Damage");reset(f,0);
}
static void panic_policy(HSD_GObj* object)
{
 ++cases;Fighter* f=object->user_data;reset(f,1);for(unsigned i=0;i<4;++i)f->x914[i].state=i;
 static Fighter before,expected;before=*f;for(unsigned i=0;i<4;++i)if(f->x914[i].state==HitCapsule_Enabled)ftColl_8007ABD0(&f->x914[i],29,object);expected=*f;*f=before;
 NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=29},out=sentinel();CHECK(helper(object,"Panic",&arg,1,&out)&&!memcmp(f,&expected,sizeof(*f)));
 ++pairs;
}
static void negative(HSD_GObj* object)
{
 Fighter* f=object->user_data;static Fighter before;NativeScriptDiagnostic d={0};
 for(unsigned test=0;test<21;++test){++cases;++negative_cases;reset(f,0);NativeScriptRuntimeContext c=context(object);
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_F32,.as.f32=10}},out=sentinel(),original=out;unsigned count=2;uint32_t semantic=NativeScriptBinding_HitboxSetDamage;
  switch(test){case 0:args[0].as.i64=-1;break;case 1:args[0].as.i64=4;break;case 2:args[0].type=NativeScriptType_U64;args[0].as.u64=UINT64_MAX;break;
   case 3:args[0].type=NativeScriptType_F32;args[0].as.f32=0;break;case 4:args[1].as.f32=NAN;break;case 5:args[1].as.f32=INFINITY;break;case 6:args[1].as.f32=-1.5f;break;
   case 7:args[1].type=NativeScriptType_I32;args[1].as.i64=-1;break;case 8:args[1].type=NativeScriptType_U32;args[1].as.u64=UINT32_MAX;break;
   case 9:f->x34_scale.y=0;break;case 10:f->x34_scale.y=.5f;Fighter_804D6524=NULL;break;
   case 11:f->smash_attrs.state=SmashState_Release;f->smash_attrs.x211C_holdFrame=0;break;case 12:stale.current_index=10;break;case 13:missing_table=true;break;case 14:Fighter_804D6548=NULL;break;
   case 15:f->player_id=6;break;case 16:++c.generation;break;case 17:++c.scene_epoch;break;case 18:object->classifier=HSD_GOBJ_CLASS_ITEM;break;case 19:count=1;break;case 20:semantic=NativeScriptBinding_HitboxState;args[0].as.i64=4;count=1;break;}
  before=*f;CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,semantic,args,count,&out,&d));CHECK(!memcmp(&out,&original,sizeof(out))&&!memcmp(f,&before,sizeof(*f)));object->classifier=HSD_GOBJ_CLASS_FIGHTER;
 }
 reset(f,0);HSD_GObj* fault=create();NativeScriptValue out=sentinel(),original=out;CHECK(!helper(fault,"Invalid",NULL,0,&out));
 CHECK(!memcmp(&out,&original,sizeof(out))&&nativeScriptFighter_Status(fault)==NativeScriptVM_Error);++cases;++negative_cases;
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(fault,((Fighter*)fault->user_data)->x8_spawnNum,&view));
 for(unsigned i=0;i<view.program->value_count;++i)if(nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,"tail")){
  CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&out,&d)&&!out.as.i64);
 }
 NativeScriptRuntimeContext c=context(fault);NativeScriptValue index={.type=NativeScriptType_I32};out=original;
 CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&c,NativeScriptBinding_HitboxState,&index,1,&out,&d)&&!memcmp(&out,&original,sizeof(out)));HSD_GObjFree(fault);
}
static void damage_observation_cases(HSD_GObj* object)
{
 static const uint32_t bits[]={0,0x80000000,1,0x3fa00000,0x7f800000,0xff800000,0x7fc12345,0xbf800000};
 Fighter* fighter=object->user_data;static Fighter before;
 for(unsigned slot=0;slot<4;++slot)for(unsigned state=0;state<5;++state)
  for(unsigned sample=0;sample<sizeof(bits)/sizeof(bits[0]);++sample){
   reset(fighter,0);fighter->x914[slot].state=state;
   memcpy(&fighter->x914[slot].damage,&bits[sample],sizeof(float));before=*fighter;
   NativeScriptValue index={.type=NativeScriptType_I32,.as.i64=slot},out=sentinel();
   CHECK(helper(object,"ReadDamage",&index,1,&out)&&out.type==NativeScriptType_F32);
   CHECK(!memcmp(&out.as.f32,&fighter->x914[slot].damage,sizeof(float))&&!memcmp(fighter,&before,sizeof(*fighter)));
   ++cases;++pairs;
  }
 for(unsigned test=0;test<9;++test){
  reset(fighter,0);before=*fighter;NativeScriptRuntimeContext source=context(object);
  NativeScriptValue index={.type=NativeScriptType_I32},out=sentinel(),original=out;NativeScriptDiagnostic d={0};
  unsigned count=1;
  switch(test){
  case 0:index.as.i64=-1;break;
  case 1:index.as.i64=4;break;
  case 2:index.type=NativeScriptType_U64;index.as.u64=UINT64_C(0x100000000);break;
  case 3:index.type=NativeScriptType_F32;break;
  case 4:++source.generation;break;
  case 5:++source.scene_epoch;break;
  case 6:source.source_class=NativeCallbackSource_Item;break;
  case 7:count=0;break;
  case 8:object->classifier=HSD_GOBJ_CLASS_ITEM;break;
  }
  CHECK(!nativeScriptBindingFighterHitboxes_Invoke(&source,NativeScriptBinding_HitboxDamage,&index,count,&out,&d));
  CHECK(!memcmp(&out,&original,sizeof(out))&&!memcmp(fighter,&before,sizeof(*fighter)));
  object->classifier=HSD_GOBJ_CLASS_FIGHTER;++cases;++negative_cases;
 }
}

static void snapshot_case(HSD_GObj* object)
{
 ++cases;Fighter* f=object->user_data;reset(f,1);NativeScriptValue out=sentinel();CHECK(helper(object,"Stamp",NULL,0,&out));static Fighter peer;peer=*f;size_t used;CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&used));
 NativeScriptValue args[]={{.type=NativeScriptType_I32},{.type=NativeScriptType_U32,.as.u64=19}};CHECK(helper(object,"Damage",args,2,&out));CHECK(memcmp(f,&peer,sizeof(*f)));
 f->x914[0].state=HitCapsule_Unk4;CHECK(helper(object,"Stamp",NULL,0,&out));
 NativeScriptReceiverView view;NativeScriptDiagnostic d={0};CHECK(nativeScriptFighter_View(object,f->x8_spawnNum,&view));uint32_t seen=UINT32_MAX;
 for(unsigned i=0;i<view.program->value_count;++i)if(nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,"seen"))seen=i;
 CHECK(seen!=UINT32_MAX&&nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|seen,NULL,0,&out,&d)&&out.as.i64==HitCapsule_Unk4);
 *f=peer;CHECK(nativeScriptFighter_Load(snapshot,used));CHECK(!memcmp(f,&peer,sizeof(*f)));
 CHECK(nativeScriptFighter_View(object,f->x8_spawnNum,&view)&&nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|seen,NULL,0,&out,&d)&&out.as.i64==peer.x914[0].state);
 CHECK(helper(object,"Damage",args,2,&out));
}
static void group_cases(HSD_GObj* object)
{
 const uint32_t tags[]={0,1,2,7,0x80000000,0x80000001,0xfffffffe,UINT32_MAX};
 Fighter* f=object->user_data;static Fighter expected;
 for(unsigned slot=0;slot<4;++slot)for(unsigned state=0;state<5;++state)for(unsigned n=0;n<sizeof(tags)/sizeof(*tags);++n){
  reset(f,0);f->x914[slot].state=state;f->x914[slot].x4=tags[n];expected=*f;
  NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=slot},{.type=NativeScriptType_U32,.as.u64=tags[n]}},out=sentinel();
  CHECK(helper(object,"ReadGroup",args,1,&out)&&out.type==NativeScriptType_U32&&out.as.u64==tags[n]);
  CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
  args[1].as.u64=tags[(n+1)%(sizeof(tags)/sizeof(*tags))];expected.x914[slot].x4=(uint32_t)args[1].as.u64;
  CHECK(helper(object,"SetGroup",args,2,&out)&&out.type==NativeScriptType_Void);
  CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
  if(expected.x914[slot].state!=HitCapsule_Disabled)expected.x914[slot].x4=(expected.x914[slot].x4+1)%2;
  CHECK(helper(object,"CycleGroup",args,1,&out)&&out.type==NativeScriptType_Void);
  CHECK(!memcmp(f,&expected,sizeof(*f)));++cases;++pairs;
 }
 NativeScriptDiagnostic d={0};NativeScriptRuntimeContext source=context(object);
 // Group tags retain the native U32 assignment semantics; they are equality
 // tags, never array indices, and changing them must not clear contact history.
 NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=2},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000007)}},out=sentinel();
 expected=*f;expected.x914[2].x4=7;
 CHECK(nativeScriptBindings_Invoke(&source,NativeScriptBinding_HitboxSetGroup,NULL,0,args,2,&out,&d));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
 args[1]=(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=-1};expected.x914[2].x4=UINT32_MAX;
 CHECK(nativeScriptBindings_Invoke(&source,NativeScriptBinding_HitboxSetGroup,NULL,0,args,2,&out,&d));CHECK(!memcmp(f,&expected,sizeof(*f)));++pairs;
 for(unsigned invalid=0;invalid<7;++invalid){source=context(object);args[0]=(NativeScriptValue){.type=NativeScriptType_I32};args[1]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=4};
  switch(invalid){case 0:args[0].as.i64=-1;break;case 1:args[0].as.i64=4;break;
   case 2:args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)};break;
   case 3:args[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};break;
   case 4:args[1]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=2};break;
   case 5:++source.generation;break;case 6:++source.scene_epoch;break;}
  expected=*f;out=sentinel();NativeScriptValue original=out;
  CHECK(!nativeScriptBindings_Invoke(&source,NativeScriptBinding_HitboxSetGroup,NULL,0,args,2,&out,&d));
  CHECK(!memcmp(f,&expected,sizeof(*f))&&!memcmp(&out,&original,sizeof(out)));++cases;++negative_cases;
 }
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();HSD_GObj* object=create();
 state_cases(object);damage_cases(object);panic_policy(object);negative(object);damage_observation_cases(object);group_cases(object);snapshot_case(object);HSD_GObjFree(object);CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_helper_calls\":%u,\"negative_cases\":%u}\n",checks,cases,pairs,vm_calls,negative_cases);return 0;
}
