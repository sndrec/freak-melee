#include "fixture.h"
#include "boundary.h"
#include <melee/ft/native_fighter_contact.h>
#include <melee/ft/native_script_binding_contact.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum { N=512,I=2048,F=32,V=32 };
typedef struct Buffers {
    char source[32768];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
    NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
    NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];
    NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
    NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} Buffers;
NativeSourcePackage packages[3];static Buffers buffers[3];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static Fighter fighters[512];static unsigned allocated,checks,cases,calls,observations;
static unsigned hazard;static HSD_GObj *source,*target,*replacement;static bool mutate_caller;
static NativeScriptDiagnostic diagnostic;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"contact %u %s (%s)\n",__LINE__,#v,diagnostic.message);exit(1);}}while(0)
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind>=Ft_Kind_CustomFirst&&kind<Ft_Kind_CustomFirst+3?&packages[kind-Ft_Kind_CustomFirst]:NULL;}
static void configure(NativeScriptCompileBank* bank,Buffers* b)
{
    *bank=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=sizeof(b->source),.tree={.nodes=b->nodes,.capacity=N},
        .expressions={.nodes=b->ir,.node_capacity=I,.arguments=b->arguments,.argument_capacity=I},
        .declarations={.functions=b->functions,.function_capacity=F,.parameters=b->parameters,.parameter_capacity=V,.values=b->values,.value_capacity=V},
        .locals=b->locals,.local_capacity=V,.instructions=b->instructions,.instruction_capacity=I,.function_code=b->codes,.function_code_capacity=F,
        .execution_functions=b->entries,.execution_function_capacity=F,.graph=b->graph,.graph_capacity=F,
        .storage_layout={.fields=b->fields,.field_capacity=V,.owners=b->owners,.owner_capacity=V},.parameter_slots=b->slots,.parameter_slot_capacity=V};
}

static void prepare(unsigned n)
{
 NativeSourcePackage* p=&packages[n];configure(&p->compiler.bank,&buffers[n]);
 const char* code="fighter:\n    behavior independent\nstate bool allow\nstate bool result\nstate int mode\nstate int calls\nstate int tail\nstate int zero\naction Decisions:\n    on tick:\n        position.x = 999\n    on body_rebound:\n        state.calls = state.calls + 1\n        if state.mode == 1:\n            position.x = 1\n        if state.mode == 2:\n            return collision.contact_rebound_allowed()\n        if state.mode == 3:\n            return 1 / state.zero\n        return state.allow\nfunc Query():\n    state.result = collision.contact_rebound_allowed()\n    state.tail = state.tail + 1\n";
 if(n==2)code="fighter:\n    behavior independent\nstate int calls\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=code,.source_bytes=(uint32_t)strlen(code),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&p->compiler.bank,&request,&accepted,&diagnostic));
 CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst+n,accepted,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->target=Ft_Kind_CustomFirst+n;p->ready=p->assets.ready=true;p->assets.facts.moves=layout;
 gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=p->assets.motions;
 nativeScriptFighter_PublishProgram(p->target,&p->program);
}
static void remove_data(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* create(unsigned bank,bool attached)
{
 CHECK(allocated<ARRAY_SIZE(fighters));HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
 Fighter* f=&fighters[allocated++];*f=(Fighter){.gobj=o,.kind=attached?packages[bank].target:Ft_Kind_Mars,.behavior_kind=attached?packages[bank].target:Ft_Kind_Mars,.x8_spawnNum=allocated,.ft_data=attached?&packages[bank].assets.character.data:NULL};
 GObj_InitUserData(o,4,remove_data,f);if(attached)CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
 NativeScriptReceiverView v;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&v));
 for(unsigned i=0;i<v.program->value_count;++i)if(!v.program->values[i].owner&&nativeScriptSource_SpanEquals(v.program->tree,v.program->values[i].name,name)){
  NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&result,&diagnostic));return result;
 }CHECK(false);return(NativeScriptValue){0};
}
static void integer(HSD_GObj* o,const char* name,int n){NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=n};field(o,name,&v);}
static void boolean(HSD_GObj* o,const char* name,bool n){NativeScriptValue v={.type=NativeScriptType_Bool,.as.u64=n};field(o,name,&v);}
static NativeScriptRuntimeContext context(HSD_GObj* o){return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptVMStatus query(HSD_GObj* o)
{
 const NativeScriptProgramIndex* p=&nativeScriptFighter_Program(((Fighter*)o->user_data)->kind)->bank->declarations;
 for(unsigned i=0;i<p->function_count;++i)if(nativeScriptSource_SpanEquals(p->tree,p->functions[i].name,"Query")){
  NativeScriptValue result;++calls;return nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,i,NULL,0,&result,&diagnostic);
 }CHECK(false);return NativeScriptVM_Error;
}
void contact_observe(HSD_GObj* running)
{
 ++observations;if(!hazard)return;
 HSD_GObj* o=mutate_caller?source:running;unsigned mode=hazard;hazard=0;
 switch(mode){
 case 1:HSD_GObjFree(o);break;
 case 2:scheduler_fail_write=true;{NativeScriptValue result;CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_Tick,NULL,0, NULL,&result)==NativeScriptVM_Error);}scheduler_fail_write=false;break;
 case 3:o->user_data=NULL;break;
 case 4:++((Fighter*)o->user_data)->x8_spawnNum;break;
 case 5:o->classifier=HSD_GOBJ_CLASS_ITEM;break;
 case 6:HSD_GObjFree(o);replacement=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(replacement==o);break;
 case 7:((Fighter*)o->user_data)->ft_data=NULL;break;
 case 8:((Fighter*)source->user_data)->unk_gobj=source;break;
 case 9:((Fighter*)source->user_data)->x221C_b5=false;break;
 default:CHECK(false);
 }
}
static void pair(void)
{source=create(0,true);target=create(1,true);((Fighter*)source->user_data)->unk_gobj=target;((Fighter*)source->user_data)->x221C_b5=true;hazard=observations=0;replacement=NULL;}
static void close_pair(Fighter* a,Fighter* b,unsigned mode,bool caller)
{
 HSD_GObj* changed=caller?source:target;
 if(mode!=1&&mode!=6){changed->classifier=HSD_GOBJ_CLASS_FIGHTER;changed->user_data=caller?(void*)a:(void*)b;}
 if(source!=changed || (mode!=1&&mode!=6))HSD_GObjFree(source);
 if(target!=changed || (mode!=1&&mode!=6))HSD_GObjFree(target);
 if(replacement)HSD_GObjFree(replacement);
}
void ContactPeachContinuation(HSD_GObj*);
bool ContactReferenceReboundAllowed(HSD_GObj*);
static void healthy(void)
{
 for(unsigned result=0;result<2;++result){pair();boolean(target,"allow",result);CHECK(query(source)==NativeScriptVM_Ok);CHECK(field(source,"result",NULL).as.u64==result);CHECK(field(source,"tail",NULL).as.i64==1);CHECK(field(target,"calls",NULL).as.i64==1);CHECK(!observations);++cases;close_pair(source->user_data,target->user_data,0,false);}
 pair();((Fighter*)source->user_data)->unk_gobj=source;boolean(source,"allow",true);CHECK(query(source)==NativeScriptVM_Ok);CHECK(field(source,"calls",NULL).as.i64==1);++cases;close_pair(source->user_data,target->user_data,0,false);
 source=create(0,true);target=create(2,true);((Fighter*)source->user_data)->unk_gobj=target;((Fighter*)source->user_data)->x221C_b5=true;
 CHECK(packages[2].program.hooks[NativeScriptFighter_BodyRebound]==UINT32_MAX);CHECK(query(source)==NativeScriptVM_Ok&&!field(source,"result",NULL).as.u64);CHECK(field(target,"calls",NULL).as.i64==0);++cases;close_pair(source->user_data,target->user_data,0,false);
 for(unsigned kind=0;kind<3;++kind)for(int motion=340;motion<381;++motion){
  source=create(0,true);target=create(0,false);Fighter* other=target->user_data;other->kind=other->behavior_kind=kind==0?Ft_Kind_Mars:kind==1?Ft_Kind_Emblem:Ft_Kind_Peach;other->motion_id=motion;
  Fighter* f=source->user_data;f->unk_gobj=target;f->x221C_b5=true;bool expected=ContactReferenceReboundAllowed(source);bool allowed=!expected;
  CHECK(nativeFighterContact_ReboundAllowed(source,&allowed)==NativeScriptVM_Ok&&allowed==expected);CHECK(query(source)==NativeScriptVM_Ok&&field(source,"result",NULL).as.u64==expected);++cases;HSD_GObjFree(source);HSD_GObjFree(target);
 }
}
static void failures(void)
{
 for(unsigned caller=0;caller<2;++caller)for(unsigned mode=1;mode<=9;++mode)for(unsigned value=0;value<2;++value){
  pair();Fighter* a=source->user_data;Fighter* b=target->user_data;boolean(target,"allow",value);integer(target,"mode",1);mutate_caller=caller;hazard=mode;
  NativeScriptRuntimeContext c=context(source);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_C(0xfeedface11223344)},before=out;
  CHECK(!nativeScriptBindingContact_Invoke(&c,NativeScriptBinding_CollisionContactReboundAllowed,NULL,0,&out,&diagnostic));CHECK(!memcmp(&out,&before,sizeof(out)));CHECK(!hazard&&observations==1);++cases;close_pair(a,b,mode,caller);
 }
 pair();integer(target,"mode",3);bool allowed=true;CHECK(nativeFighterContact_ReboundAllowed(source,&allowed)==NativeScriptVM_Error&&allowed);CHECK(nativeScriptFighter_Status(target)==NativeScriptVM_Error);++cases;close_pair(source->user_data,target->user_data,0,false);
 pair();integer(target,"mode",1);hazard=2;mutate_caller=false;CHECK(query(source)==NativeScriptVM_Error);CHECK(field(source,"tail",NULL).as.i64==0);++cases;close_pair(source->user_data,target->user_data,0,false);
 pair();((Fighter*)source->user_data)->unk_gobj=source;integer(source,"mode",2);CHECK(query(source)==NativeScriptVM_Error);CHECK(nativeScriptRuntime_Idle());++cases;close_pair(source->user_data,target->user_data,0,false);
}
static void native_tail(void)
{
 for(unsigned result=0;result<2;++result){pair();boolean(target,"allow",result);Fighter* a=source->user_data;a->self_vel=(Vec3){3,2,1};a->cmd_vars[2]=17;a->motion_id=21;ContactPeachContinuation(source);CHECK(a->self_vel.x==(result?0:3)&&a->self_vel.y==(result?0:2));CHECK(a->cmd_vars[2]==(result?1:17)&&a->motion_id==(result?888:21));++cases;close_pair(source->user_data,target->user_data,0,false);}
 pair();Fighter* a=source->user_data;Fighter* b=target->user_data;a->self_vel=(Vec3){3,2,1};a->cmd_vars[2]=17;integer(target,"mode",1);boolean(target,"allow",false);mutate_caller=true;hazard=2;ContactPeachContinuation(source);CHECK(a->self_vel.x==3&&a->self_vel.y==2&&a->cmd_vars[2]==17);++cases;close_pair(a,b,2,true);
}
int main(void)
{
 prepare(0);prepare(1);prepare(2);HSD_GObjLibInitDataType c={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&c);
 healthy();failures();native_tail();CHECK(nativeScriptRuntime_Idle()&&!nativeScriptFighter_ReferencesProgram(&packages[0].compiler.bank)&&!nativeScriptFighter_ReferencesProgram(&packages[1].compiler.bank));
 printf("{\"status\":\"PASS contact rebound\",\"checks\":%u,\"cases\":%u,\"source_calls\":%u}\n",checks,cases,calls);return 0;
}
