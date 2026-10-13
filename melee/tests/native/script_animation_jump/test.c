#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include "boundary.h"
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0C8C.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftwalljump.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftanim.h>
#include <melee/it/it_26B1.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=1024,F=32,V=32};
NativeSourcePackage animation_package;
NativeScriptCompileBank presentation_bank;unsigned presentation_checks;
static char storage[16384],script[16384];static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[N];static uint32_t arguments[N];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[32];static NativeScriptInstruction instructions[2048];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction execution[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[4];static NativeScriptParameterSlot slots[V];
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=2};
static FtSFX sfx={.x14=777};static ftCommonData common;ftCommonData* p_ftCommonData=&common;
static struct Fighter_804D6520_t bunny={.x14=1.375f};struct Fighter_804D6520_t* Fighter_804D6520=&bunny;
static struct Fighter_804D6524_t scale;struct Fighter_804D6524_t* Fighter_804D6524=&scale;
FighterPartsTable** ftPartsTable;
static unsigned stages[8],stage_count,stop_stage,stop_mode,cases,comparisons;
static bool observe_phase;static uint8_t snapshot[32768],tampered[32768];
static uint32_t function(const char*);
static NativeScriptVMStatus helper(HSD_GObj*,const char*,NativeScriptValue*);
void presentation_check(bool ok,const char* text,unsigned line){++presentation_checks;if(!ok){fprintf(stderr,"animation jump %u: %s\n",line,text);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind==Ft_Kind_CustomFirst?&animation_package:NULL;}
static void point(Fighter* fighter,unsigned stage)
{
 CHECK(stage_count<ARRAY_SIZE(stages));stages[stage_count++]=stage;
 if(observe_phase&&nativeScriptFighter_IsAttached(fighter->gobj))CHECK(nativeScriptFighter_RunMotion(fighter->gobj,NativeScriptMotion_Physics));
 if(stop_stage==stage){if(stop_mode==1)HSD_GObjFree(fighter->gobj);else {presentation_retire_mode=7;presentation_retire_at=presentation_callback_writes+1;NativeScriptValue result;CHECK(helper(fighter->gobj,"Fail",&result)==NativeScriptVM_Error);}}
}
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId motion,u32 flags,float frame,float rate,float blend,HSD_GObj* donor)
{CHECK(flags==Ft_MF_SkipNametagVis&&frame==0&&rate==1&&blend==0&&!donor);Fighter* fighter=object->user_data;fighter->motion_id=motion;nativeScriptFighter_SelectMotion(object,(uint32_t)motion);point(fighter,1);}
void ft_80088770(Fighter* fighter){CHECK(fighter->x2221_b7&&fighter->x1968_jumpsUsed==2&&fighter->x671_timer_lstick_tilt_y==254);point(fighter,2);}
void ft_80088328(Fighter* fighter,int sound,u8 volume,u8 pan){CHECK(sound==777&&volume==SFX_VOLUME_MAX&&pan==SFX_PAN_MID);point(fighter,3);}
void ft_PlaySFX(Fighter* fighter,enum_t sound,u8 volume,u8 pan){CHECK(sound==0x11B&&volume==SFX_VOLUME_MAX&&pan==SFX_PAN_MID);point(fighter,4);}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint){(void)joint;abort();}
void ftCo_JumpAerial_IASA(Fighter_GObj* object){(void)object;abort();}
float ftCo_800D0EC8(Fighter* fighter){(void)fighter;abort();}
bool ftWallJump_8008169C(HSD_GObj* object){(void)object;abort();}
bool ftCliffCommon_80081298(Fighter_GObj* object){(void)object;abort();}
bool ftCommon_ResolveAirLanding(Fighter_GObj* object,bool(*callback)(Fighter_GObj*,int)){(void)object;(void)callback;abort();}
bool ftCo_80096CC8(Fighter_GObj* object,int mode){(void)object;(void)mode;abort();}
bool itIsHeavy(Item_GObj* object){(void)object;abort();}
Item_HoldKinds itGetHoldKind(Item_GObj* object){(void)object;abort();}
void ftAnim_80070C48(Fighter_GObj* object,s32 value){(void)object;(void)value;abort();}
void ftAnim_800704F0(Fighter_GObj* object,int index,float value){(void)object;(void)index;(void)value;abort();}
void ftAnim_80070FB4(Fighter_GObj* object,s32 a,s32 b){(void)object;(void)a;(void)b;abort();}
void ftAnim_80070CC4(Fighter_GObj* object,int value){(void)object;(void)value;abort();}
#include "actual_jump.inc"
#define bank animation_package.compiler.bank
static void prepare(bool hook)
{
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
 bank=(NativeScriptCompileBank){.source_storage=storage,.source_capacity=sizeof(storage),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=2048,.function_code=code,.function_code_capacity=F,.execution_functions=execution,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=4},.parameter_slots=slots,.parameter_slot_capacity=V};
 char candidate[16384];strcpy(candidate,script);
 if(!hook){char* begin=strstr(candidate,"    on animation_driven_jump_physics:");char* end=strstr(begin,"action Ordinary:");CHECK(begin&&end);memmove(begin,end,strlen(end)+1);}
 NativeScriptCompileRequest request={.source=candidate,.source_bytes=(uint32_t)strlen(candidate),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic diagnostic={0};
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&animation_package.program,&diagnostic)){fprintf(stderr,"prepare %u:%u %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}
 animation_package.address=&animation_package;animation_package.ready=true;animation_package.target=Ft_Kind_CustomFirst;animation_package.assets.ready=true;animation_package.assets.facts.moves=layout;animation_package.assets.character.data.x4C_sfx=&sfx;
 gFtDataList[Ft_Kind_CustomFirst]=&animation_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=animation_package.assets.motions;
 nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&animation_package.program);
}
static HSD_GObj* create(bool attached)
{
 HSD_GObj* object=presentation_create(Ft_Kind_CustomFirst,false);Fighter* fighter=object->user_data;
 fighter->ft_data=&animation_package.assets.character.data;fighter->behavior_kind=Ft_Kind_CustomFirst;
 gFtDataList[Ft_Kind_CustomFirst]=&animation_package.assets.character.data;
 fighter->motion_id=-1;fighter->x1968_jumpsUsed=1;fighter->x671_timer_lstick_tilt_y=13;fighter->x197C=object;
 fighter->co_attrs.air_drift_stick_mul=0.07f;fighter->co_attrs.aerial_drift_base=0.025f;fighter->co_attrs.air_drift_max=1.1f;fighter->co_attrs.air_max_horizontal_velocity=1.5f;fighter->co_attrs.aerial_friction=0.035f;
 if(attached)CHECK(nativeScriptFighter_Attach(object));return object;
}
static uint32_t function(const char* name){for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name))return i;CHECK(false);return UINT32_MAX;}
static NativeScriptVMStatus helper(HSD_GObj* object,const char* name,NativeScriptValue* result){NativeScriptDiagnostic d={0};return nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function(name),NULL,0,result,&d);}
static int state(HSD_GObj* object,const char* name){return (int)presentation_state(object,name,NULL).as.i64;}
static void reset(void){stage_count=stop_stage=stop_mode=0;observe_phase=false;presentation_retire_mode=0;presentation_callback_writes=0;}
static NativeScriptFighterSnapshotRecord* record(uint8_t* bytes){return (void*)(bytes+sizeof(NativeScriptFighterSnapshotHeader));}
static void healthy(void)
{
 HSD_GObj* object=create(true);HSD_GObj* reference=create(false);Fighter* fighter=object->user_data;Fighter* native=reference->user_data;
 for(unsigned animated=0;animated<2;++animated){reset();observe_phase=true;fighter->x1968_jumpsUsed=1;NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=123};
  CHECK(helper(object,animated?"BeginTrue":"BeginFalse",&output)==NativeScriptVM_Ok);CHECK(output.type==NativeScriptType_Void&&!fighter->phys_cb&&stage_count==4);
  for(unsigned i=0;i<4;++i)CHECK(stages[i]==i+1);CHECK(state(object,"ordinary")==4*(int)(animated+1));
  reset();native->x1968_jumpsUsed=1;Vec3 velocity={-0.8125f,0,-0.125f};ftCo_JumpAerial_Begin(reference,341,&velocity,true,animated!=0,true);
  CHECK(!memcmp(&fighter->self_vel,&native->self_vel,sizeof(Vec3))&&fighter->x2221_b7==native->x2221_b7&&fighter->x1968_jumpsUsed==native->x1968_jumpsUsed&&fighter->x671_timer_lstick_tilt_y==native->x671_timer_lstick_tilt_y);++comparisons;
  CHECK(native->phys_cb==(animated?ftCo_JumpAerial_Phys_Cb:NULL));
 }
 const float sticks[]={-1,-0.0f,0,0.25f,1};
 for(unsigned bunny_item=0;bunny_item<2;++bunny_item)for(unsigned i=0;i<ARRAY_SIZE(sticks);++i){fighter->input.lstick[0].x=native->input.lstick[0].x=sticks[i];fighter->self_vel.x=native->self_vel.x=-0.8125f;fighter->x6A4_transNOffset.y=native->x6A4_transNOffset.y=0.333333343f;fighter->x197C=bunny_item?object:NULL;native->x197C=bunny_item?reference:NULL;
  ftCo_JumpAerial_Phys_Cb(reference);CHECK(nativeScriptFighter_RunMotion(object,NativeScriptMotion_Physics));CHECK(!memcmp(&fighter->self_vel,&native->self_vel,sizeof(Vec3))&&!memcmp(&fighter->x74_self_accel,&native->x74_self_accel,sizeof(Vec3)));++comparisons;
 }
 CHECK(state(object,"animated")==10);size_t bytes;CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&bytes));CHECK(record(snapshot)->motions[NativeScriptMotion_Physics]==animation_package.program.hooks[NativeScriptFighter_AnimationDrivenJumpPhysics]);
 memcpy(tampered,snapshot,bytes);record(tampered)->motions[NativeScriptMotion_Animation]=record(tampered)->motions[NativeScriptMotion_Physics];CHECK(!nativeScriptFighter_ValidateSnapshot(tampered,bytes));
 memcpy(tampered,snapshot,bytes);record(tampered)->motions[NativeScriptMotion_Physics]=animation_package.program.hooks[NativeScriptFighter_Load];CHECK(!nativeScriptFighter_ValidateSnapshot(tampered,bytes));
 CHECK(!nativeScriptFighter_BindMotionPhase(object,NativeScriptMotion_Animation,animation_package.program.hooks[NativeScriptFighter_AnimationDrivenJumpPhysics],false));
 CHECK(!nativeScriptFighter_BindMotionPhase(object,NativeScriptMotion_Physics,animation_package.program.hooks[NativeScriptFighter_AnimationDrivenJumpPhysics],true));
 NativeScriptValue result;CHECK(helper(object,"ArmOverride",&result)==NativeScriptVM_Ok);CHECK(nativeScriptFighter_RunMotion(object,NativeScriptMotion_Physics));CHECK(state(object,"override")==1);
 CHECK(helper(object,"Clear",&result)==NativeScriptVM_Ok&&!nativeScriptFighter_MotionBound(object,NativeScriptMotion_Physics));
 CHECK(nativeScriptFighter_Load(snapshot,bytes));CHECK(nativeScriptFighter_RunMotion(object,NativeScriptMotion_Physics)&&state(object,"animated")==11);
 ++fighter->x8_spawnNum;nativeScriptFighter_NewLife(object,fighter->x8_spawnNum);CHECK(nativeScriptFighter_RunMotion(object,NativeScriptMotion_Physics)&&state(object,"animated")==12);
 fighter->motion_id=342;nativeScriptFighter_SelectMotion(object,342);CHECK(nativeScriptFighter_RunMotion(object,NativeScriptMotion_Physics)&&state(object,"ordinary")==18&&state(object,"animated")==12);
 HSD_GObjFree(reference);HSD_GObjFree(object);++cases;
}
static void failures(void)
{
 for(unsigned mode=1;mode<3;++mode)for(unsigned stage=1;stage<5;++stage){reset();HSD_GObj* object=create(true);NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=123};stop_mode=mode;stop_stage=stage;
  NativeScriptVMStatus status=helper(object,"BeginTrue",&result);CHECK(status==(mode==1?NativeScriptVM_SourceRetired:NativeScriptVM_Error));CHECK(result.type==NativeScriptType_U64&&result.as.u64==123&&stage_count==stage&&nativeScriptRuntime_Idle());
  if(mode==2){CHECK(state(object,"after")==0);HSD_GObjFree(object);}++cases;
 }
 reset();prepare(false);HSD_GObj* object=create(true);Fighter before=*(Fighter*)object->user_data;NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=123};
 CHECK(helper(object,"BeginTrue",&result)==NativeScriptVM_Error);CHECK(stage_count==0&&!memcmp(&before,object->user_data,sizeof(before))&&result.type==NativeScriptType_U64&&result.as.u64==123);HSD_GObjFree(object);
 reset();object=create(true);CHECK(helper(object,"BeginFalse",&result)==NativeScriptVM_Ok&&stage_count==4);CHECK(nativeScriptFighter_RunMotion(object,NativeScriptMotion_Physics)&&state(object,"ordinary")==1);HSD_GObjFree(object);++cases;
}
int main(int argc,char** argv)
{
 CHECK(argc==2);FILE* file=fopen(argv[1],"rb");CHECK(file);size_t size=fread(script,1,sizeof(script)-1,file);CHECK(!ferror(file)&&size<sizeof(script)-1);fclose(file);script[size]=0;
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare(true);healthy();failures();
 printf("{\"status\":\"PASS source animation jump\",\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u}\n",presentation_checks,cases,comparisons);return 0;
}
