#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_storage.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ft_0D4D.h>
#include <melee/ft/ft_0D31.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/kinds/ftCommon/ftCo_Throw.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageIce.h>
#include <melee/ft/ft_0C31.h>
#include <melee/ft/ftmetal.h>
#include <melee/ft/ft_0C88.h>
#include <stdarg.h>
#include <melee/cm/types.h>
#include <melee/ef/efsync.h>
#include <melee/gm/gm_1601.h>
#include <melee/it/item.h>
#include <melee/pl/player.h>
#include <melee/pl/plbonuslib.h>
#include <melee/pl/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""

enum { N=1024,I=4096,F=64,V=64 };
static char bytes[8192];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
NativeSourcePackage sleep_package;
#define bank sleep_package.compiler.bank
static MotionState motions[342];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static CmSubject cameras[64];static unsigned serial,checks,cases,callbacks,native_callbacks,motion_calls,effects,status_calls,death_tail,rumble_tail,reset_calls,normal_tail,sound_resets;
static HSD_GObj* peer;static HSD_GObj* primary;static HSD_GObj* mutate_target;static HSD_GObj* replacement;
static unsigned probe_values[8],probe_count;
static unsigned mutation;static bool reset_success=true;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"sleep case%u line%u %s (%s)\n",cases,__LINE__,#x,diagnostic.message[0]?diagnostic.message:nativeScriptRuntime_VM()->diagnostic.message);exit(1);}}while(0)

static void remove_data(void* data)
{
 Fighter* f=data;NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeFighterPool,f,&ref));
 nativeScriptFighter_Detach(f->gobj);CHECK(nativeMatchPool_Free(&nativeFighterPool,ref));
}
static HSD_GObj* create(bool owned)
{
 HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
 NativeMatchPoolReference ref;NativeFighterPoolSlot* slot=nativeMatchPool_Allocate(&nativeFighterPool,&ref);CHECK(slot);
 Fighter* f=&slot->actor;f->gobj=o;f->kind=f->behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Mario;
 f->ft_data=&sleep_package.assets.character.data;f->x34_scale.y=2;f->x8_spawnNum=++serial;f->player_id=2;f->x2180=6;
 f->x890_cameraBox=&cameras[ref.slot];*f->x890_cameraBox=(CmSubject){.state=CmSubjectState_Active};
 GObj_InitUserData(o,4,remove_data,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
void sleep_probe(HSD_GObj* source)
{
 ++callbacks;CHECK(probe_count<8);probe_values[probe_count++]=((Fighter*)source->user_data)->cmd_vars[0];if(!mutation)return;unsigned mode=mutation;mutation=0;HSD_GObj* target=mutate_target?mutate_target:source;Fighter* f=target->user_data;
 if(mode==1)HSD_GObjFree(target);
 else if(mode==2){HSD_GObjFree(target);replacement=create(true);CHECK(replacement==target&&replacement->user_data==f);}
 else if(mode==3)f->x8_spawnNum++;
 else if(mode==4)bank.storage_layout.identity++;
 else if(mode==5)nativeScriptFighter_Detach(target);
 else abort();
}
static void foreign_callback(HSD_GObj* o){++native_callbacks;sleep_probe(o);}
static uint32_t function(const char* name)
{for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[i].name,name))return i;CHECK(false);return 0;}
static void bind(HSD_GObj* o,unsigned slot,bool fault)
{CHECK(nativeScriptFighter_BindCallback(o,slot,function(fault?"Fault":"Probe")));}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId id,MotionFlags flags,float start,float speed,float blend,Fighter_GObj* target)
{
 CHECK(id==ftCo_MS_Sleep&&flags==Ft_MF_None&&start==0&&speed==1&&blend==0&&!target);++motion_calls;
 Fighter* f=o->user_data;f->motion_id=id;
 if(nativeScriptFighter_CallbackBound(o,NativeScriptFighterCallback_StateChange))nativeScriptFighter_RunCallback(o,NativeScriptFighterCallback_StateChange);
 else if(f->x21EC)f->x21EC(o);
}
HSD_GObj* Player_GetEntityAtIndex(int player,int index){CHECK(player==2&&index==1);return peer;}
void gm_80167320(int player,bool alternate){CHECK(player==2&&!alternate);++status_calls;}
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* object,...){CHECK(id==0x43F&&!object);va_list args;va_start(args,object);Vec3* position=va_arg(args,Vec3*);float* scale=va_arg(args,float*);CHECK(position&&scale&&*scale==2.5f);va_end(args);++effects;return(EF_SpawnResult){0};}
void ft_800880D8(Fighter* f){CHECK(f);++sound_resets;f->x2164=0;}
void ftCommon_8007ED2C(Fighter* f){CHECK(f);++rumble_tail;}
bool Fighter_UnkProcessDeath_80068354(Fighter_GObj* o)
{
 ++reset_calls;if(!reset_success)return false;Fighter* f=o->user_data;
 nativeScriptFighter_Detach(o);f->x8_spawnNum++;CHECK(nativeScriptFighter_Attach(o));return true;
}
void ftCommon_8007D92C(Fighter_GObj* o){CHECK(o);++normal_tail;}
void ftCo_800DD100(Fighter_GObj* o){CHECK(o);++death_tail;}
void ftCommon_8007E2FC(Fighter_GObj* o){CHECK(o);++death_tail;}
void ftCommon_8007DB24(Fighter_GObj* o){CHECK(o);++death_tail;}
void pl_8004065C(int p,int alt){(void)p;(void)alt;++death_tail;}
void pl_8003EA08(int p,int alt){(void)p;(void)alt;abort();}
void pl_8003EA74(int p,int alt){(void)p;(void)alt;abort();}
u8 ftLib_80086BE0(Fighter_GObj* o){(void)o;abort();}
s32 ftLib_800874BC(Fighter_GObj* o){(void)o;abort();}
void Item_8026A8EC(Item_GObj* o){(void)o;abort();}
void ftCommon_8007F8E8(Fighter_GObj* o){(void)o;abort();}
void ftCommon_8007F9B4(Fighter_GObj* o){(void)o;abort();}
void ftCommon_8007FF74(Fighter_GObj* o){(void)o;abort();}
void ftCo_800C8540(Fighter_GObj* o){(void)o;abort();}
void ftCo_800C8A64(Fighter_GObj* o){(void)o;abort();}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "sleep_bodies.inc"
#pragma clang diagnostic pop

static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N,.arguments=arguments,.argument_capacity=I},.declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate int zero\nfunc Probe():\n    command(0) = 1\nfunc Probe2():\n    command(0) = 2\nfunc Probe3():\n    command(0) = 3\nfunc Fault():\n    let int invalid = 1 / state.zero\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&sleep_package.program,&diagnostic));
 sleep_package.assets.character.data.x0=&sleep_package.assets.character.common_attributes;sleep_package.assets.character.common_attributes.x168=1.25f;
 sleep_package.address=&sleep_package;sleep_package.ready=sleep_package.assets.ready=true;sleep_package.target=Ft_Kind_CustomFirst;sleep_package.assets.motions=motions;sleep_package.assets.facts.moves=layout;
 gFtDataList[Ft_Kind_CustomFirst]=&sleep_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&sleep_package.program);
}
static void counters(void)
{probe_count=0;mutation=0;mutate_target=NULL;replacement=NULL;peer=NULL;motion_calls=effects=status_calls=death_tail=rumble_tail=reset_calls=normal_tail=sound_resets=0;reset_success=true;}
static bool asleep(Fighter* f)
{return f->invisible&&f->x221E_b1&&f->x221E_b2&&f->x2219_b1&&f->x221F_b3&&f->x221F_b1&&f->x890_cameraBox->state==CmSubjectState_Inactive;}
static void healthy(void)
{
 for(unsigned owned=0;owned<2;++owned)for(unsigned operation=0;operation<4;++operation){++cases;counters();HSD_GObj* o=create(owned);Fighter* f=o->user_data;
  if(owned)bind(o,NativeScriptFighterCallback_StateChange,false);else f->x21EC=foreign_callback;
  if(operation==0){Fighter expected=*f;expected.motion_id=ftCo_MS_Sleep;expected.invisible=expected.x221E_b1=expected.x221E_b2=expected.x2219_b1=expected.x221F_b3=expected.x221F_b1=true;if(owned)expected.cmd_vars[0]=1;
   ftCo_800BFD04(o);CHECK(!memcmp(f,&expected,sizeof(expected)));}
  if(operation==1){f->x2222_b5=true;peer=create(owned);if(owned)bind(peer,NativeScriptFighterCallback_Death1,false);else ((Fighter*)peer->user_data)->death1_cb=foreign_callback;ftCo_800BFD9C(o);CHECK(status_calls==1&&effects==1&&asleep(peer->user_data));HSD_GObjFree(peer);}
  if(operation==2){if(owned)bind(o,NativeScriptFighterCallback_Death1,false);else f->death1_cb=foreign_callback;f->x221D_b6=true;f->x2004=7;ftCo_800D4F24(o,1);CHECK(death_tail==3&&effects==1&&sound_resets==1&&!f->x2004);}
  if(operation==3){if(owned)bind(o,NativeScriptFighterCallback_Death1,false);else f->death1_cb=foreign_callback;ftLib_80087140(o);CHECK(rumble_tail==1&&reset_calls==1&&normal_tail==1&&(!owned||nativeScriptFighter_Status(o)==NativeScriptVM_Ok));}
  if(operation!=3)CHECK(asleep(f));CHECK(motion_calls==(operation==1?2u:1u));HSD_GObjFree(o);
 }
}
static void failures(void)
{
 for(unsigned operation=0;operation<4;++operation)for(unsigned mode=1;mode<=6;++mode){++cases;counters();HSD_GObj* o=create(true);Fighter* f=o->user_data;CmSubject* camera=f->x890_cameraBox;uint64_t layout=bank.storage_layout.identity;
  bool motion=operation==0||operation==1;bind(o,motion?NativeScriptFighterCallback_StateChange:NativeScriptFighterCallback_Death1,mode==6);
  mutation=mode==6?0:mode;
  if(operation==0)ftCo_800BFD04(o);
  if(operation==1){f->x2222_b5=true;peer=create(true);ftCo_800BFD9C(o);CHECK(!status_calls&&!effects);HSD_GObjFree(peer);}
  if(operation==2)ftCo_800D4F24(o,1);
  if(operation==3)ftLib_80087140(o);
  CHECK(!mutation&&!camera->state);CHECK(!rumble_tail&&!reset_calls&&!normal_tail);
  if(operation==2||operation==3)CHECK(!motion_calls&&death_tail==(mode==4?3u:0u));
  bank.storage_layout.identity=layout;
  if(mode==2){CHECK(!asleep(replacement->user_data));HSD_GObjFree(replacement);}else if(mode!=1)HSD_GObjFree(o);
 }
 // Peer callback controls either peer or primary. Peer loss alone preserves primary status.
 for(unsigned target=0;target<2;++target)for(unsigned mode=1;mode<=6;++mode){++cases;counters();HSD_GObj* o=create(true);primary=o;Fighter* f=o->user_data;f->x2222_b5=true;peer=create(true);HSD_GObj* p=peer;uint64_t layout=bank.storage_layout.identity;
  bind(p,NativeScriptFighterCallback_Death1,mode==6);mutation=mode==6?0:mode;mutate_target=target?o:p;
  ftCo_800BFD9C(o);
  // Fault belongs to the peer callback, regardless of the unused selected probe target.
  bool primary_current=mode!=4&&(!target||mode==6);CHECK(status_calls==(unsigned)primary_current);CHECK(effects==1);
  if(primary_current)CHECK(asleep(f));bank.storage_layout.identity=layout;
  if(mode==2)HSD_GObjFree(replacement);
  if(mode!=1&&mode!=2){HSD_GObjFree(p);HSD_GObjFree(o);}else if(target)HSD_GObjFree(p);else HSD_GObjFree(o);
 }
 // Missing peer, disabled retirement and self peer preserve native primary bookkeeping.
 for(unsigned mode=0;mode<3;++mode){++cases;counters();HSD_GObj* o=create(true);((Fighter*)o->user_data)->x2222_b5=mode!=0;if(mode==2)peer=o;ftCo_800BFD9C(o);CHECK(asleep(o->user_data)&&status_calls==1&&!effects);HSD_GObjFree(o);}
 // Healthy triple dispatch preserves native Death2 -> Death3 -> Death1 order.
 ++cases;counters();HSD_GObj* ordered=create(true);
 CHECK(nativeScriptFighter_BindCallback(ordered,NativeScriptFighterCallback_Death2,function("Probe2")));
 CHECK(nativeScriptFighter_BindCallback(ordered,NativeScriptFighterCallback_Death3,function("Probe3")));
 bind(ordered,NativeScriptFighterCallback_Death1,false);ftCo_800D4F24(ordered,0);
 CHECK(probe_count==3&&probe_values[0]==2&&probe_values[1]==3&&probe_values[2]==1&&death_tail==3&&asleep(ordered->user_data));
 counters();ftCo_800D4F24(ordered,1);CHECK(!effects&&!motion_calls&&!death_tail);HSD_GObjFree(ordered);
 // Independent callback slots execute in native Death2, Death3, Death1 order.
 for(unsigned slot=NativeScriptFighterCallback_Death1;slot<=NativeScriptFighterCallback_Death3;++slot){++cases;counters();HSD_GObj* o=create(true);bind(o,slot,true);ftCo_800D4F24(o,0);CHECK(!effects&&!motion_calls&&!death_tail);HSD_GObjFree(o);}
 ++cases;counters();HSD_GObj* o=create(true);reset_success=false;ftLib_80087140(o);CHECK(rumble_tail==1&&reset_calls==1&&!normal_tail);HSD_GObjFree(o);
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();healthy();failures();CHECK(nativeScriptRuntime_Idle());CHECK(!nativeFighterPool.used&&!nativeGObjPool.used);
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"authored_callbacks\":%u,\"native_callbacks\":%u}\n",checks,cases,callbacks,native_callbacks);return 0;
}
