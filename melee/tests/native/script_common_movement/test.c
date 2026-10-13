#include <melee/ft/native_script_binding_article_reference_resolve.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
#include <melee/ft/native_script_binding_common_movement.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_transition.h>
#include <melee/ft/native_script_binding_presentation.h>
#include <melee/ft/native_script_binding_item.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_binding_effect.h>
#include <melee/ft/native_script_binding_ledge.h>
#include <melee/ft/native_script_binding_jump.h>
#include <melee/ft/native_script_binding_forms.h>
#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_math.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/types.h>
#include <melee/ef/native_model_effect.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/pl/plbonuslib.h>
#include <melee/sfx/crowdsfx.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,cases,comparisons,mutations;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"movement %u %s: %s\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static NativeSourcePackage package;
static NativeScriptFighterProgram replacement_program;
static NativeScriptStorageInstance storage,replacement_storage;
static NativeScriptStorageParameters replacement_parameters;
static Fighter fighter;
static MotionState motions[1];
static ftCommonData common_data;ftCommonData* p_ftCommonData=&common_data;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_OnLanding[Ft_Kind_RuntimeCapacity];
static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
NativeMatchPool nativeGObjPool;
static _Alignas(32) uint8_t payload[4*256];static NativeMatchPoolSlot slots[4];static uint32_t free_order[4];
static HSD_GObj* object;static NativeMatchPoolReference reference;
static NativeScriptRuntimeContext context;
static NativeScriptVMStatus actor_status;static unsigned view_available,program_available;static bool package_available,knockback_result;
static u32 floor_flags;static unsigned mutation,mutation_stage;
typedef struct MovementEvent {unsigned kind;int a,b;float x,y;} MovementEvent;
static MovementEvent events[64];static unsigned event_count;
enum {Bonus=1,Knockback,Floor,Landing,Platform};
enum {Generation=1,LogicalRetire,Fault,ViewRetire,ProgramIdentity,LayoutIdentity,ParameterIdentity,ReplaceView,
      ReplacePackage,ReplaceProgram,ReplaceParameters,ReusePool,Epoch,NativeData,MotionTable,ContextGeneration};

const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return package_available&&kind==Ft_Kind_CustomFirst?&package:NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{return program_available&&kind==Ft_Kind_CustomFirst?(program_available==2?&replacement_program:&package.program):NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* actor)
{return actor==object?actor_status:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* actor,uint32_t generation,NativeScriptReceiverView* view)
{if(!view_available||actor!=object||generation!=17||program_available==2||package.program.identity!=88||
    package.compiler.bank.storage_layout.identity!=55||package.compiler.bank.storage_parameters.identity!=66)return false;
 *view=(NativeScriptReceiverView){&package.compiler.bank.declarations,view_available==2?&replacement_storage:&storage};return true;}
bool nativeScriptFighter_IsAttached(Fighter_GObj* actor){CHECK(actor==object);return true;}
bool nativeModelEffect_Snapshot(HSD_GObj* actor,uint32_t* generation){(void)actor;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* actor,uint32_t generation){(void)actor;(void)generation;return NULL;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;abort();}
#include "source_contract.inc"

static void change(unsigned stage)
{
    if(!mutation||stage!=mutation_stage)return;++mutations;
    switch(mutation) {
    case Generation:++fighter.x8_spawnNum;break;
    case LogicalRetire:storage.live=false;break;
    case Fault:actor_status=NativeScriptVM_Error;break;
    case ViewRetire:view_available=0;break;
    case ProgramIdentity:++package.program.identity;break;
    case LayoutIdentity:++package.compiler.bank.storage_layout.identity;break;
    case ParameterIdentity:++package.compiler.bank.storage_parameters.identity;break;
    case ReplaceView:replacement_storage=storage;view_available=2;break;
    case ReplacePackage:package_available=false;break;
    case ReplaceProgram:replacement_program=package.program;program_available=2;break;
    case ReplaceParameters:replacement_parameters=*storage.parameters;storage.parameters=&replacement_parameters;break;
    case ReusePool:{HSD_GObj prior=*object;CHECK(nativeMatchPool_Free(&nativeGObjPool,reference));NativeMatchPoolReference next;
        CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&next)==object);*object=prior;CHECK(next.generation!=reference.generation);break;}
    case Epoch:++context.scene_epoch;break;
    case NativeData:fighter.ft_data=NULL;break;
    case MotionTable:ftData_CharacterStateTables[fighter.kind]=NULL;break;
    case ContextGeneration:++context.generation;break;
    default:abort();
    }
    mutation=0;
}
static void event(unsigned kind,int a,int b,float x,float y)
{CHECK(event_count<64);events[event_count++]=(MovementEvent){kind,a,b,x,y};change(kind);}
void pl_8003FC44(int slot,int value){event(Bonus,slot,value,0,0);}
bool un_803224DC(s32 spawn,float x,float kb){event(Knockback,spawn,0,x,kb);return knockback_result;}
bool ft_80084A18(Fighter_GObj* actor){CHECK(actor==object);event(Floor,0,0,0,0);return true;}
NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj* actor,uint32_t hook,const NativeScriptValue* args,uint32_t count, NativeScriptEventRecordIO* record,NativeScriptValue* out)
{(void)record;CHECK(actor==object&&hook==NativeScriptFighter_Landing&&!args&&!count&&!out);event(Landing,0,0,0,0);return actor_status;}
u32 mpLineGetFlags(int line){event(Platform,line,0,0,0);return floor_flags;}
void OSReport(char* format,...){(void)format;abort();}
ATTRIBUTE_NORETURN void __assert(char* file,u32 line,char* text){fprintf(stderr,"assert %s:%lu %s\n",file,line,text);abort();}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#include "native_bodies.inc"
#pragma clang diagnostic pop

static const uint32_t operations[]={NativeScriptBinding_FacingFromInputThreshold,NativeScriptBinding_PhysicsForceAirAndUseJumps,
    NativeScriptBinding_PhysicsSwitchToGround,NativeScriptBinding_CollisionSkipSoftFloor};
static void setup(void)
{
    memset(&package,0,sizeof(package));memset(&nativeGObjPool,0,sizeof(nativeGObjPool));memset(heads,0,sizeof(heads));
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,payload,sizeof(payload),slots,free_order,4,sizeof(HSD_GObj),32,UINT64_C(0x100000002)));
    object=nativeMatchPool_Allocate(&nativeGObjPool,&reference);CHECK(object);memset(object,0,sizeof(*object));
    memset(&fighter,0,sizeof(fighter));fighter.gobj=object;fighter.kind=fighter.behavior_kind=Ft_Kind_CustomFirst;fighter.x8_spawnNum=17;
    object->user_data=&fighter;heads[HSD_GOBJ_PLINK_FIGHTER]=object;
    package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.motions=motions;
    fighter.ft_data=&package.assets.character.data;gFtDataList[fighter.kind]=fighter.ft_data;ftData_CharacterStateTables[fighter.kind]=package.assets.motions;
    package.program.bank=&package.compiler.bank;package.program.identity=88;package.compiler.bank.valid=package.compiler.bank.has_motion_layout=true;
    package.compiler.bank.motion_layout=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};package.assets.facts.moves=package.compiler.bank.motion_layout;
    package.compiler.bank.storage_layout.identity=55;package.compiler.bank.storage_layout.program=&package.compiler.bank.declarations;
    package.compiler.bank.storage_parameters.identity=66;
    storage=(NativeScriptStorageInstance){.layout=&package.compiler.bank.storage_layout,.parameters=&package.compiler.bank.storage_parameters,.generation=17,.identity=17,.owner=0,.live=true};
    context=(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=17,.scene_epoch=reference.lifetime};
    actor_status=NativeScriptVM_Ok;view_available=program_available=package_available=true;mutation=mutation_stage=event_count=0;
    memset(events,0,sizeof(events));memset(&common_data,0,sizeof(common_data));p_ftCommonData=&common_data;
    common_data.horizontal_stick_deadzone=.25f;fighter.facing_dir=-1;fighter.player_id=2;fighter.co_attrs.max_jumps=5;
    fighter.co_attrs.ground_max_horizontal_velocity=3;fighter.x74_self_accel.y=8;fighter.gr_vel=7;fighter.self_vel.x=11;
    fighter.x1968_jumpsUsed=1;fighter.x1969_walljumpUsed=2;fighter.x2227_b0=true;fighter.x221F_b4=true;
    fighter.ground_or_air=GA_Air;fighter.ecb_lock=19;fighter.coll_data.x130_flags=0xFFF;fighter.coll_data.floor.index=7;fighter.coll_data.floor_skip=-1;
    fighter.dmg.x18A4_knockbackMagnitude=19;fighter.cur_pos.x=37;knockback_result=true;floor_flags=LINE_FLAG_PLATFORM;
}
static NativeScriptValue reference_run(unsigned op,Fighter* f)
{
    NativeScriptValue result={.type=NativeScriptType_Void};
    switch(op){case 0:ftCommon_8007DA24(f);break;case 1:ftCommon_8007D60C(f);break;case 2:ftCommon_8007D7FC(f);break;
        case 3:result.type=NativeScriptType_Bool;result.as.u64=ftCo_8009A134(object);break;default:abort();}
    return result;
}
static void compare(unsigned op)
{
    Fighter expected=fighter;NativeScriptValue actual;CHECK(nativeScriptBindingCommonMovement_Invoke(&context,operations[op],NULL,0,&actual,&diagnostic));
    MovementEvent observed[64];memcpy(observed,events,sizeof(events));unsigned count=event_count;event_count=0;memset(events,0,sizeof(events));
    object->user_data=&expected;NativeScriptValue result=reference_run(op,&expected);object->user_data=&fighter;
    CHECK(!memcmp(&fighter,&expected,sizeof(fighter)));CHECK(count==event_count&&!memcmp(observed,events,sizeof(events)));
    CHECK(actual.type==result.type&&actual.as.u64==result.as.u64);++comparisons;
}
static void native_comparisons(void)
{
    const float inputs[]={-INFINITY,-1,-.250001f,-.25f,-0.0f,0,.25f,.250001f,1,INFINITY,NAN};
    for(unsigned i=0;i<sizeof(inputs)/sizeof(*inputs);++i)for(unsigned facing=0;facing<2;++facing){setup();fighter.input.lstick[0].x=inputs[i];fighter.facing_dir=facing?1:-1;compare(0);++cases;}
    for(unsigned used=0;used<4;++used)for(unsigned bonus=0;bonus<2;++bonus)for(unsigned max=0;max<6;++max){setup();fighter.x1968_jumpsUsed=used;fighter.x2227_b0=bonus;fighter.co_attrs.max_jumps=max;compare(1);++cases;}
    for(unsigned air=0;air<2;++air)for(unsigned bonus=0;bonus<2;++bonus)for(unsigned used=0;used<3;++used)
        for(unsigned offset=0;offset<2;++offset)for(unsigned knock=0;knock<2;++knock){setup();fighter.ground_or_air=air?GA_Air:GA_Ground;fighter.x2227_b0=bonus;
            fighter.x1968_jumpsUsed=used;fighter.x594_b0=offset;fighter.x6A4_transNOffset.z=13;fighter.facing_dir=-1;knockback_result=knock;compare(2);++cases;}
    for(unsigned platform=0;platform<2;++platform){setup();floor_flags=platform?LINE_FLAG_PLATFORM:0;compare(3);CHECK(fighter.coll_data.floor_skip==(platform?7:-1));++cases;}
}
static unsigned expected_mutation;
static void reject(unsigned op)
{NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};bool accepted=nativeScriptBindingCommonMovement_Invoke(&context,operations[op],NULL,0,&out,&diagnostic);
 if(accepted)fprintf(stderr,"unexpected acceptance op%u mutation%u stage%u\n",op,expected_mutation,mutation_stage);
 CHECK(!accepted);CHECK(out.type==NativeScriptType_U64&&out.as.u64==UINT64_MAX);}
static void guards(void)
{
    for(unsigned op=0;op<4;++op){setup();Fighter before=fighter;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(!nativeScriptBindingCommonMovement_Invoke(&context,operations[op],NULL,1,&out,&diagnostic));CHECK(!memcmp(&before,&fighter,sizeof(fighter))&&!event_count&&out.as.u64==UINT64_MAX);
        CHECK(!nativeScriptBindingCommonMovement_Invoke(&context,UINT32_MAX,NULL,0,&out,&diagnostic));CHECK(!event_count);
        for(unsigned m=1;m<=ContextGeneration;++m){
            // Equivalent view/parameter replacement or pool reuse before this
            // direct call supplies no earlier retained reference to compare.
            // Their identity changes are tested during the native call below.
            if(m==ReplaceView||m==ReplaceParameters||m==ReusePool)continue;
            setup();expected_mutation=m;mutation=m;mutation_stage=Landing;change(Landing);Fighter prior=fighter;reject(op);CHECK(!event_count&&!memcmp(&prior,&fighter,sizeof(fighter)));++cases;}
        setup();context.source_class=NativeCallbackSource_Item;reject(op);CHECK(!event_count);++cases;
        setup();context.scene_epoch=0;reject(op);CHECK(!event_count);++cases;
    }
    setup();p_ftCommonData=NULL;reject(0);CHECK(!event_count);++cases;
    // Inject identity loss at native subsystem/callback boundaries. Completed
    // native mutations stay observable; output publication must be withheld.
    for(unsigned m=1;m<=ContextGeneration;++m)for(unsigned op=1;op<4;++op){setup();expected_mutation=m;mutation=m;
        mutation_stage=op==1?Bonus:op==2?Landing:Platform;reject(op);CHECK(event_count>0&&!mutation);++cases;}
    setup();fighter.ground_or_air=GA_Ground;compare(2);CHECK(events[0].kind==Floor&&event_count==1);
    setup();compare(2);CHECK(event_count==4&&events[0].kind==Knockback&&events[1].kind==Bonus&&events[2].kind==Floor&&events[3].kind==Landing);
}
int main(void)
{
    for(unsigned op=0;op<4;++op){const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(operations[op]);CHECK(descriptor&&descriptor->contexts==NativeScriptBindings_Fighter&&!descriptor->argument_count);
        CHECK(descriptor->result_type==(op==3?NativeScriptType_Bool:NativeScriptType_Void));CHECK(descriptor->flags&NativeScriptFlag_MayInvalidateSource);}
    native_comparisons();guards();printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u,\"identity_mutations\":%u}\n",checks,cases,comparisons,mutations);return 0;
}
