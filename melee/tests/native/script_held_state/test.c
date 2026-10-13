#include "fixture.h"
#include <melee/ft/native_script_binding_held_state.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemParasolOpen.h>
#include <math.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum {N=1024,I=4096,F=32,V=64};
typedef struct BankData {
 char source_bytes[4096];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
 NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
 NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
 NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} BankData;
static BankData banks[2];static unsigned active;
NativeSourcePackage held_packages[3];
#define bank held_packages[active].compiler.bank
static Fighter fighters[16],substitute;static unsigned allocated,checks,cases,pairs,vm_calls;
static MotionState motions[40],common_motions[341];static FighterBone parts[MAX_FT_PARTS];static HSD_JObj joints[MAX_FT_PARTS];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"held-state %u case %u: %s (%s)\n",__LINE__,cases,#x,diagnostic.message);exit(1);}}while(0)
static void configure(void)
{
 BankData* data=&banks[active];
 bank=(NativeScriptCompileBank){.source_storage=data->source_bytes,.source_capacity=sizeof(data->source_bytes),.tree={.nodes=data->nodes,.capacity=N},.expressions={.nodes=data->ir,.node_capacity=I,.arguments=data->arguments,.argument_capacity=I},.declarations={.functions=data->functions,.function_capacity=F,.parameters=data->parameters,.parameter_capacity=V,.values=data->values,.value_capacity=V},.locals=data->locals,.local_capacity=V,.instructions=data->instructions,.instruction_capacity=I,.function_code=data->codes,.function_code_capacity=F,.execution_functions=data->entries,.execution_function_capacity=F,.graph=data->graph,.graph_capacity=F,.storage_layout={.fields=data->fields,.field_capacity=V,.owners=data->owners,.owner_capacity=V},.parameter_slots=data->slots,.parameter_slot_capacity=V};
}
static uint32_t function(const char* name)
{for(unsigned n=0;n<bank.declarations.function_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[n].name,name))return n;CHECK(false);return 0;}
static NativeScriptValue field(HSD_GObj* o,const char* name)
{
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));
 for(unsigned n=0;n<bank.declarations.value_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.values[n].name,name)){
  NativeScriptValue r;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&r,&diagnostic));return r;}
 CHECK(false);return(NativeScriptValue){0};
}
static bool helper(HSD_GObj* o,const char* name)
{
 NativeScriptValue r={.type=NativeScriptType_U64,.as.u64=99};++vm_calls;NativeScriptVMStatus s=nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,function(name),NULL,0,&r,&diagnostic);
 CHECK(s==NativeScriptVM_Ok?r.type==NativeScriptType_Void:r.type==NativeScriptType_U64&&r.as.u64==99);return s==NativeScriptVM_Ok;
}

static const char* source="fighter:\n    behavior independent\nstate int mode\nstate int selected\nstate int seen_motion\nstate int seen_previous\nstate int zero\nstate int calls\nfunc Enter():\n    motion.id = Motion.fall_special\n    state.held_open(10.0)\nfunc Fault():\n    state.zero = 1 / state.zero\nmove SharedHeldPolicy:\n    on held_motion:\n        state.calls = state.calls + 1\n        state.seen_motion = event.motion\n        state.seen_previous = event.previous_motion\n        if state.mode == 1:\n            return state.selected\n        if state.mode == 2:\n            return 1 / state.zero\n        return event.motion\naction Placeholder:\n    on tick:\n        pass\n";
static void prepare(bool hook)
{
 const char* input=hook?source:"fighter:\n    behavior independent\nfunc Enter():\n    motion.id = Motion.fall_special\n    state.held_open(10.0)\naction Placeholder:\n    on tick:\n        pass\n";
 configure();NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=40};NativeScriptCompileRequest request={.source=input,.source_bytes=(uint32_t)strlen(input),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(accepted==&bank);
 NativeSourcePackage* p=&held_packages[active];CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst+active,&bank,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->ready=p->assets.ready=true;p->target=Ft_Kind_CustomFirst+active;p->assets.facts.moves=layout;p->assets.facts.joints=MAX_FT_PARTS;p->assets.parts.parts_num=MAX_FT_PARTS;p->assets.parts.part_to_joint=p->assets.part_to_joint;p->assets.parts.joint_to_part=p->assets.joint_to_part;p->assets.motions=motions;p->assets.common_motions=common_motions;
 static struct Fighter_WaitAnimData animation[8];p->assets.character.data.xC=animation;p->assets.facts.actions=8;
 tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 for(unsigned n=0;n<MAX_FT_PARTS;++n)parts[n].joint=&joints[n];
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);
}
static void remove_data(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static HSD_GObj* actor(void)
{
 CHECK(allocated<16);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);Fighter* f=&fighters[allocated++];
 *f=(Fighter){.gobj=o,.kind=Ft_Kind_CustomFirst+active,.behavior_kind=Ft_Kind_CustomFirst+active,.x8_spawnNum=100+allocated,.ft_data=&held_packages[active].assets.character.data,.parts=parts,.x18=341,.x1C_actionStateList=common_motions,.x20_actionStateList=motions,.x24=held_packages[active].assets.character.data.xC,.x58C=8};
 GObj_InitUserData(o,4,remove_data,f);CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static void set_field(HSD_GObj* o,const char* name,int32_t value)
{
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));
 for(unsigned n=0;n<bank.declarations.value_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.values[n].name,name)){
  NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=value};CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&v,&diagnostic));return;}
 CHECK(false);
}
static struct Trace {unsigned order[3],count;int32_t selected,previous,timer;uint32_t flags;float start,speed,blend;} trace;
static unsigned mutation;
void held_transition(Fighter_GObj* o,FtMotionId motion,MotionFlags flags,float start,float speed,float blend,Fighter_GObj* peer)
{
 Fighter* f=o->user_data;CHECK(trace.count<3&&peer==NULL);trace.order[trace.count++]=1;trace.selected=motion;trace.previous=f->mv.co.parasol_open.prev_msid;trace.timer=f->x2104;trace.flags=flags;trace.start=start;trace.speed=speed;trace.blend=blend;
 f->motion_id=motion;f->x2221_b4=false;
 if(mutation==1)CHECK(!helper(o,"Fault"));
 else if(mutation==2)nativeScriptFighter_Detach(o);
 else if(mutation==3){substitute=*f;o->user_data=&substitute;}
 else if(mutation==4)p_ftCommonData=NULL;
}
void held_animation(Fighter_GObj* o)
{CHECK(trace.count<3);trace.order[trace.count++]=2;if(mutation==5)CHECK(!helper(o,"Fault"));if(mutation==6){NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));nativeScriptStorage_Destroy(view.storage);}}
static bool enter(NativeScriptRuntimeContext* c,float blend,NativeScriptValue* r)
{NativeScriptValue argument={.type=NativeScriptType_F32,.as.f32=blend};return nativeScriptBindingHeldState_Invoke(c,NativeScriptBinding_StateHeldOpen,&argument,1,r,&diagnostic);}
static void healthy(HSD_GObj* o)
{
 Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);const int prev[]={ftCo_MS_Fall,ftCo_MS_FallSpecial,369};const float timers[]={5.75f,-2.75f,0.0f},blends[]={0,10,3.75f,-0.0f};
 for(unsigned mode=0;mode<3;++mode)for(unsigned p=0;p<3;++p)for(unsigned bits=0;bits<8;++bits)for(unsigned t=0;t<3;++t)for(unsigned b=0;b<4;++b){
  set_field(o,"mode",mode==0?0:1);set_field(o,"selected",mode==1?369:65535);held_common.x59C=timers[t];f->motion_id=prev[p];f->x2104=27;f->x2221_b7=bits&1;f->x2221_b4=(bits>>1)&1;f->x2221_b6=(bits>>2)&1;
  int64_t called=field(o,"calls").as.i64;
  static Fighter before,expected;before=*f;trace=(struct Trace){0};reference_enter(o,blends[b]);expected=*f;struct Trace expected_trace=trace;*f=before;trace=(struct Trace){0};NativeScriptValue r;
  CHECK(enter(&c,blends[b],&r));CHECK(r.type==NativeScriptType_Void);CHECK(!memcmp(f,&expected,sizeof(*f)));CHECK(!memcmp(&trace,&expected_trace,sizeof(trace)));CHECK(trace.count==2&&trace.order[0]==1&&trace.order[1]==2);
  CHECK(field(o,"seen_motion").as.i64==ftCo_MS_ItemParasolOpen&&field(o,"seen_previous").as.i64==prev[p]);CHECK(field(o,"calls").as.i64==called+2);vm_calls+=2;++cases;++pairs;
 }
 set_field(o,"mode",1);set_field(o,"selected",369);f->motion_id=0;trace=(struct Trace){0};CHECK(helper(o,"Enter"));CHECK(trace.previous==ftCo_MS_FallSpecial&&trace.selected==369&&trace.blend==10.0f);++cases;
}
static void setters(HSD_GObj* o)
{
 NativeScriptRuntimeContext c=context(o);Fighter* f=o->user_data;const int32_t valid[]={0,340,341,380};
 for(unsigned n=0;n<4;++n){static Fighter expected;expected=*f;expected.motion_id=valid[n];NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=valid[n]};trace=(struct Trace){0};CHECK(nativeScriptBindingHeldState_Write(&c,NativeScriptBinding_MotionID,&v,&diagnostic));CHECK(!memcmp(f,&expected,sizeof(*f))&&trace.count==0);++cases;++pairs;}
 const NativeScriptValue invalid[]={{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_U64,.as.u64=65536},{.type=NativeScriptType_I32,.as.i64=381},{.type=NativeScriptType_F32,.as.f32=35}};
 for(unsigned n=0;n<4;++n){static Fighter before;before=*f;CHECK(!nativeScriptBindingHeldState_Write(&c,NativeScriptBinding_MotionID,&invalid[n],&diagnostic));CHECK(!memcmp(f,&before,sizeof(*f)));++cases;}
}
static void preflight(HSD_GObj* o)
{
 NativeScriptRuntimeContext c=context(o);Fighter* f=o->user_data;const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(NativeScriptBinding_MotionID);
 CHECK(descriptor&&descriptor->result_type==NativeScriptType_I32&&(descriptor->flags&NativeScriptFlag_Writable));
 for(unsigned n=0;n<13;++n){static Fighter before;before=*f;NativeScriptRuntimeContext bad=c;float timer=held_common.x59C;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,c.generation,&view));NativeScriptStorageParameters alternate=bank.storage_parameters;
  switch(n){case 0:bad.source_class=NativeCallbackSource_Item;break;case 1:++bad.generation;break;case 2:++bad.scene_epoch;break;case 3:f->x1C_actionStateList=NULL;break;case 4:f->x24=NULL;break;case 5:++f->x58C;break;case 6:p_ftCommonData=NULL;break;case 7:held_common.x59C=NAN;break;case 8:held_common.x59C=2147483648.0f;break;case 9:f->behavior_kind=Ft_Kind_Mario;break;case 10:view.storage->parameters=&alternate;break;case 11:o->classifier=HSD_GOBJ_CLASS_ITEM;break;case 12:f->gobj=NULL;break;}
  static Fighter unchanged;unchanged=*f;NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=88},r=sentinel;trace=(struct Trace){0};CHECK(!enter(&bad,10,&r));CHECK(!memcmp(&r,&sentinel,sizeof(r))&&!memcmp(f,&unchanged,sizeof(*f))&&trace.count==0);
  *f=before;o->classifier=HSD_GOBJ_CLASS_FIGHTER;p_ftCommonData=&held_common;held_common.x59C=timer;view.storage->parameters=&bank.storage_parameters;++cases;
 }
 NativeScriptValue r={.type=NativeScriptType_U64,.as.u64=88},wrong={.type=NativeScriptType_I32,.as.i64=10};CHECK(!nativeScriptBindingHeldState_Invoke(&c,NativeScriptBinding_StateHeldOpen,&wrong,1,&r,&diagnostic)&&r.as.u64==88);CHECK(!nativeScriptBindingHeldState_Invoke(&c,NativeScriptBinding_StateHeldOpen,NULL,0,&r,&diagnostic)&&r.as.u64==88);cases+=2;
}
static void faults(void)
{
 for(unsigned m=1;m<=6;++m){HSD_GObj* o=actor();Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);set_field(o,"mode",0);f->motion_id=ftCo_MS_FallSpecial;f->x2104=99;f->x2221_b7=true;f->x2221_b6=true;trace=(struct Trace){0};mutation=m;
  NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=88},r=sentinel;CHECK(!enter(&c,10,&r));CHECK(!memcmp(&r,&sentinel,sizeof(r)));CHECK(f->x2104==99&&f->x2221_b7);CHECK(trace.count==(m<5?1u:2u));
  mutation=0;o->user_data=f;p_ftCommonData=&held_common;HSD_GObjFree(o);++cases;
 }
 HSD_GObj* o=actor();Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);set_field(o,"mode",2);trace=(struct Trace){0};NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=88},r=sentinel;CHECK(!enter(&c,10,&r));CHECK(!memcmp(&r,&sentinel,sizeof(r))&&trace.count==0);HSD_GObjFree(o);++cases;(void)f;
}
int main(void)
{
 prepare(true);HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);HSD_GObj* o=actor();healthy(o);setters(o);preflight(o);HSD_GObjFree(o);faults();active=1;prepare(false);o=actor();CHECK(!nativeScriptFighter_HasHook(o,NativeScriptFighter_HeldMotion));trace=(struct Trace){0};CHECK(helper(o,"Enter"));CHECK(trace.selected==ftCo_MS_ItemParasolOpen&&trace.previous==ftCo_MS_FallSpecial&&trace.blend==10.0f);HSD_GObjFree(o);++cases;CHECK(nativeGObjPool_RetireScene());printf("{\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
