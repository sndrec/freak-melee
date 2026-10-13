#include "boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_cpu.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/pl/player.h>
#include <melee/gm/native_home.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum {N=2048,F=64,V=64};
static NativeSourcePackage packages[2];
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
static MotionState motions[342];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static StaticPlayer player_slots[Gm_Player_NumMax];
static unsigned checks,cases,factory_calls,vm_calls,free_count,callback_calls;
static uint32_t next_generation=100,freed[16];
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"constructor %u %s: %s\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void Player_CheckSlot(int slot){CHECK(slot>=0&&slot<Gm_Player_NumMax);}
void family_guard_observe(void){}
#include "native_player.inc"
#include "native_family.inc"
#include "native_constructor.inc"
static Fighter* ftCo_800A589C(Fighter* f){(void)f;abort();}
#include "native_initializer.inc"
unsigned nativeHome_WorkshopInitialMember(unsigned slot){CHECK(slot==2);return 0;}
typedef struct SecondaryCompileStorage {
 char bytes[32768];NativeScriptSourceNode syntax[N];NativeScriptIRNode expressions[N*2];uint32_t arguments[N*2];
 NativeScriptFunctionDeclaration declarations[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];NativeScriptLocal locals[64];
 NativeScriptInstruction instructions[4096];NativeScriptFunctionCode code[F];NativeScriptExecutionFunction functions[F];NativeScriptCompileFunctionScratch graph[F];
 NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[8];NativeScriptParameterSlot parameters_slots[V];
} SecondaryCompileStorage;
static SecondaryCompileStorage compile_storage[2];
static void prepare_program(unsigned member)
{
 SecondaryCompileStorage* a=&compile_storage[member];NativeSourcePackage* p=&packages[member];
 const char* source="fighter:\n    behavior independent\nstate int history_hint\nfunc Fault():\n    let int zero = 0\n    let int invalid = 1 / zero\naction CPU:\n    on cpu_prepare:\n        state.history_hint = event.history_source\n        return event.history_source\n";
 p->compiler.bank=(NativeScriptCompileBank){.source_storage=a->bytes,.source_capacity=sizeof(a->bytes),.tree={.nodes=a->syntax,.capacity=N},.expressions={.nodes=a->expressions,.node_capacity=N*2,.arguments=a->arguments,.argument_capacity=N*2},
 .declarations={.functions=a->declarations,.function_capacity=F,.parameters=a->parameters,.parameter_capacity=V,.values=a->values,.value_capacity=V},.locals=a->locals,.local_capacity=64,
 .instructions=a->instructions,.instruction_capacity=4096,.function_code=a->code,.function_code_capacity=F,.execution_functions=a->functions,.execution_function_capacity=F,.graph=a->graph,.graph_capacity=F,
 .storage_layout={.fields=a->fields,.field_capacity=V,.owners=a->owners,.owner_capacity=8},.parameter_slots=a->parameters_slots,.parameter_slot_capacity=V};
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&p->compiler.bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst+member,accepted,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->ready=p->assets.ready=true;p->target=Ft_Kind_CustomFirst+member;p->assets.motions=motions;p->assets.facts.moves=layout;
 gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);CHECK(nativeScriptFighter_Program(p->target)!=&p->program);
}
static void prepare(void)
{
 for(unsigned i=0;i<2;++i)prepare_program(i);
 families[0]=(NativeFighterFamily){.members=slots,.character_kind=ChKind_CustomFirst,.active=true};
 for(unsigned i=0;i<2;++i)slots[i]=(NativeFighterSlot){.family=&families[0],.member_index=i,.source=&packages[i],.character_kind=ChKind_CustomFirst,.fighter_kind=Ft_Kind_CustomFirst+i,.active=true};
}

enum Action { None, Fault, Detach, ReplaceRole, RetireReuse, NewLife, ChangeOwner,
              ChangeCharacter, ChangeMapping, SwapRoles, BadSchema };
static enum Action factory_action,post_action;
static unsigned fail_factory,act_factory,unwind_publish;
static bool factory_targets_returned;
static HSD_GObj* created[2],*replacement;
static Fighter* changed_owner;
static uint32_t created_generation[2];
static HSD_GObj* allocate_actor(unsigned member);
static void remove_actor(void* userdata)
{
 Fighter* f=userdata;CHECK(free_count<16);freed[free_count++]=f->x8_spawnNum;
 nativeScriptFighter_Detach(f->gobj);
 Player_80031FB0(f->player_id,f->x221F_b4,f->gobj);
 if(unwind_publish && f->kind==Ft_Kind_CustomFirst+1){unwind_publish=0;replacement=allocate_actor(1);player_slots[2].player_entity[1]=replacement;}
 NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeFighterPool,f,&ref));CHECK(nativeMatchPool_Free(&nativeFighterPool,ref));
}
static HSD_GObj* allocate_actor(unsigned member)
{
 NativeMatchPoolReference ref;NativeFighterPoolSlot* slot=nativeMatchPool_Allocate(&nativeFighterPool,&ref);CHECK(slot);
 memset(slot,0,sizeof(*slot));HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
 Fighter* f=&slot->actor;*f=(Fighter){.gobj=object,.kind=Ft_Kind_CustomFirst+member,.behavior_kind=Ft_Kind_CustomFirst+member,
  .ft_data=&packages[member].assets.character.data,.x8_spawnNum=next_generation++,.player_id=2};
 f->x221F_b4=member;f->cur_pos=(Vec3){23.25f,-17.5f,-0.0f};f->facing_dir=-1;
 GObj_InitUserData(object,4,remove_actor,f);CHECK(nativeScriptFighter_Attach(object));return object;
}
static unsigned named_function(unsigned member,const char* name)
{
 NativeScriptCompileBank* b=&packages[member].compiler.bank;
 for(unsigned i=0;i<b->declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&b->tree,b->declarations.functions[i].name,name))return i;
 CHECK(false);return 0;
}
static void act(enum Action action,HSD_GObj* object)
{
 ++callback_calls;Fighter* f=object->user_data;unsigned member=f->kind-Ft_Kind_CustomFirst;
 switch(action){
 case None:break;
 case Fault:{NativeScriptValue result;CHECK(nativeScriptFighter_Helper(object,f->x8_spawnNum,named_function(member,"Fault"),NULL,0,&result,&diagnostic)==NativeScriptVM_Error);++vm_calls;break;}
 case Detach:nativeScriptFighter_Detach(object);break;
 case ReplaceRole:replacement=allocate_actor(member);player_slots[2].player_entity[member]=replacement;break;
 case RetireReuse:HSD_GObjFree(object);replacement=allocate_actor(member);CHECK(replacement==object);player_slots[2].player_entity[member]=replacement;break;
 case NewLife:++f->x8_spawnNum;nativeScriptFighter_NewLife(object,f->x8_spawnNum);break;
 case ChangeOwner:changed_owner=f;object->user_data=&nativeFighterPool_Slot(f)->parts[0];break;
 case ChangeCharacter:player_slots[2].player_character=CKind_Mario;break;
 case ChangeMapping:++ftMapping_list[ChKind_CustomFirst].internal_id;break;
 case SwapRoles:Player_SwapTransformedStates(2,0,1);f->x221F_b4=!f->x221F_b4;break;
 case BadSchema:++packages[member].compiler.bank.storage_layout.identity;break;
 }
}
Fighter_GObj* Fighter_Create(struct plAllocInfo* info)
{
 ++factory_calls;unsigned member=info->internal_id-Ft_Kind_CustomFirst;CHECK(member<2 && info->slot==2);
 CHECK(info->b0==member && info->x5==-1);
 if(member){CHECK(player_slots[2].player_entity[0]==created[0]);CHECK(nativeFighterRuntime_MemberId(created[0],0)==created_generation[0]);}
 if(factory_calls==fail_factory)return NULL;
 HSD_GObj* object=allocate_actor(member);created[member]=object;created_generation[member]=((Fighter*)object->user_data)->x8_spawnNum;
 Fighter* f=object->user_data;f->co_attrs.gravity=2;f->co_attrs.jump_v_initial_velocity=4;f->co_attrs.air_jump_v_multiplier=1;
 uint32_t expected=member && families[0].metadata.mode==NativeFighterFamily_SimultaneousCompanion?created_generation[0]:0;
 memset(&f->cpu,0xA5,sizeof(f->cpu));struct Fighter_x1A88_xFC_t records[30];memcpy(records,f->cpu.xFC,sizeof(records));
 for(unsigned i=0;i<30;++i){records[i].x0=0;records[i].lstick=(S8Vec2){0,0};records[i].x4=records[i].x5=0;
  records[i].cur_pos=expected?(Vec3){23.25f,-17.5f,-0.0f}:(Vec3){0,0,0};records[i].facing_dir=expected?-1:1;}
 ftCo_800A101C(f,6,9,0);++vm_calls;
 NativeScriptReceiverView view;NativeScriptValue hint;CHECK(nativeScriptFighter_View(object,f->x8_spawnNum,&view));
 CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC,NULL,0,&hint,&diagnostic));
 CHECK((uint32_t)hint.as.i64==expected);
 for(unsigned i=0;i<30;++i){CHECK(!memcmp(&records[i],&f->cpu.xFC[i],sizeof(records[i])));}
 if(factory_calls==act_factory)act(factory_action,member && !factory_targets_returned?created[0]:object);
 return object;
}
static void post_construct(s32 slot)
{CHECK(slot==2);act(post_action,created[1]?created[1]:created[0]);}
static void reset(unsigned mode)
{
 CHECK(nativeFighterPool.used==0);memset(player_slots,0,sizeof(player_slots));
 player_slots[2].player_character=ChKind_CustomFirst;player_slots[2].transformed[0]=0;player_slots[2].transformed[1]=1;player_slots[2].struct_func=post_construct;
 ftMapping_list[ChKind_CustomFirst]=(ftMapping){Ft_Kind_CustomFirst,mode?Ft_Kind_CustomFirst+1:-1,mode==1};
 families[0].metadata=(NativeFighterPackage){0};
 families[0].metadata.mode=mode==2?NativeFighterFamily_SimultaneousCompanion:mode?NativeFighterFamily_AlternateForm:NativeFighterFamily_Single;
 families[0].metadata.member_count=mode?2:1;slots[1].active=mode!=0;
 factory_calls=free_count=fail_factory=act_factory=unwind_publish=0;factory_action=post_action=None;
 factory_targets_returned=false;
 created[0]=created[1]=replacement=NULL;changed_owner=NULL;
}
static void clean(void)
{
 if(changed_owner){for(HSD_GObj* o=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];o;o=o->next)if(o==changed_owner->gobj)o->user_data=changed_owner;changed_owner=NULL;}
 while(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER])HSD_GObjFree(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]);
 CHECK(nativeFighterPool.used==0);CHECK(!player_slots[2].player_entity[0]&&!player_slots[2].player_entity[1]);
}
static void constructors(void)
{
 for(unsigned mode=0;mode<3;++mode){
  reset(mode);Player_80031AD0(2);CHECK(factory_calls==(mode?2:1));CHECK(player_slots[2].player_state==2 && free_count==0);
  CHECK(player_slots[2].player_entity[0]==created[0]);if(mode)CHECK(player_slots[2].player_entity[1]==created[1]);++cases;clean();
  reset(mode);fail_factory=1;Player_80031AD0(2);CHECK(factory_calls==1&&nativeFighterPool.used==0&&player_slots[2].player_state==0);++cases;
  const enum Action first_invalid[]={Fault,ReplaceRole,ChangeCharacter};
  for(unsigned i=0;i<sizeof(first_invalid)/sizeof(*first_invalid);++i){reset(mode);act_factory=1;factory_action=first_invalid[i];Player_80031AD0(2);
   CHECK(factory_calls==1&&free_count==1&&freed[0]==created_generation[0]);
   if(replacement)CHECK(player_slots[2].player_entity[0]==replacement&&nativeFighterPool.used==1);else CHECK(nativeFighterPool.used==0);++cases;clean();
  }
  if(mode){reset(mode);fail_factory=2;Player_80031AD0(2);CHECK(factory_calls==2&&free_count==1&&freed[0]==created_generation[0]);CHECK(player_slots[2].player_state==0);++cases;clean();}
  reset(mode);post_action=SwapRoles;Player_80031AD0(2);CHECK(free_count==0&&player_slots[2].transformed[0]==1);CHECK(nativeFighterRuntime_MemberId(created[0],0)==created_generation[0]);++cases;clean();
  const enum Action invalid[]={Fault,Detach,ReplaceRole,RetireReuse,NewLife,ChangeOwner,ChangeCharacter,ChangeMapping,BadSchema};
  for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){reset(mode);uint64_t schema[2]={packages[0].compiler.bank.storage_layout.identity,packages[1].compiler.bank.storage_layout.identity};post_action=invalid[i];Player_80031AD0(2);
   if(replacement){CHECK(player_slots[2].player_entity[mode?1:0]==replacement);CHECK(nativeFighterPool.used==1);}
   else if(changed_owner){CHECK(nativeFighterPool.used==1);}
   else CHECK(nativeFighterPool.used==0&&player_slots[2].player_state==0);
   packages[0].compiler.bank.storage_layout.identity=schema[0];packages[1].compiler.bank.storage_layout.identity=schema[1];++cases;clean();
  }
  if(mode){const enum Action middle[]={Fault,Detach,ReplaceRole,RetireReuse,NewLife};
   for(unsigned i=0;i<sizeof(middle)/sizeof(*middle);++i){reset(mode);act_factory=2;factory_action=middle[i];Player_80031AD0(2);
    CHECK(free_count==2);if(middle[i]==RetireReuse)CHECK(freed[1]==created_generation[1]);else CHECK(freed[0]==created_generation[1]);
    if(replacement)CHECK(player_slots[2].player_entity[0]==replacement&&nativeFighterPool.used==1);else CHECK(nativeFighterPool.used==0);++cases;clean();
   }
   const enum Action second_invalid[]={Fault,ReplaceRole};
   for(unsigned i=0;i<sizeof(second_invalid)/sizeof(*second_invalid);++i){reset(mode);act_factory=2;factory_targets_returned=true;factory_action=second_invalid[i];Player_80031AD0(2);
    CHECK(free_count==2&&freed[0]==created_generation[1]&&freed[1]==created_generation[0]);
    if(replacement)CHECK(player_slots[2].player_entity[1]==replacement&&nativeFighterPool.used==1);else CHECK(nativeFighterPool.used==0);++cases;clean();
   }
   reset(mode);post_action=Fault;unwind_publish=1;Player_80031AD0(2);CHECK(free_count==2&&freed[0]==created_generation[1]&&freed[1]==created_generation[0]);
   CHECK(player_slots[2].player_entity[1]==replacement&&player_slots[2].player_state==2);++cases;clean();
  }
 }
}
static void expected_clear(void)
{
 reset(2);HSD_GObj* a=allocate_actor(0),*b=allocate_actor(1);player_slots[2].player_entity[0]=a;player_slots[2].player_entity[1]=b;player_slots[2].player_state=2;
 Player_SwapTransformedStates(2,0,1);Player_80031FB0(2,0,a);CHECK(!player_slots[2].player_entity[0]&&player_slots[2].player_entity[1]==b&&player_slots[2].player_state==2);
 Player_80031FB0(2,1,a);CHECK(player_slots[2].player_entity[1]==b);Player_80031FB0(2,1,NULL);CHECK(player_slots[2].player_entity[1]==b);++cases;clean();
 reset(0);replacement=allocate_actor(0);player_slots[2].player_entity[1]=replacement;player_slots[2].player_state=2;
 HSD_GObj* original=allocate_actor(0);player_slots[2].player_entity[0]=original;Player_80031FB0(2,0,original);CHECK(player_slots[2].player_state==2&&player_slots[2].player_entity[1]==replacement);++cases;clean();
 reset(2);replacement=allocate_actor(0);player_slots[2].player_entity[0]=replacement;Player_80031AD0(2);CHECK(factory_calls==0&&player_slots[2].player_entity[0]==replacement);++cases;clean();
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();constructors();expected_clear();CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"vm_calls\":%u,\"controlled_callbacks\":%u}\n",checks,cases,vm_calls,callback_calls);return 0;
}
