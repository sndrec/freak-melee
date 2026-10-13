#include <melee/ft/native_script_fighter.h>
#include <melee/ef/efsync.h>
// This actor/motion fixture supplies no effect assets; preparation must reject any effect declaration.
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/controller.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SourceBytes=65536, Nodes=2048, ExpressionNodes=4096, Functions=128,
       Parameters=256, ValueFields=256, Instructions=8192 };
typedef struct Buffers {
    char source[SourceBytes];NativeScriptSourceNode nodes[Nodes];
    NativeScriptIRNode expressions[ExpressionNodes];uint32_t arguments[ExpressionNodes];
    NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[Parameters];
    NativeScriptValueDeclaration values[ValueFields];NativeScriptLocal locals[128];
    NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];
    NativeScriptExecutionFunction executions[Functions];NativeScriptCompileFunctionScratch graph[Functions];
    NativeScriptStorageField fields[ValueFields];NativeScriptStorageOwner owners[32];NativeScriptParameterSlot slots[ValueFields];
} Buffers;
static Buffers buffers[2];static NativeScriptCompileBank banks[2];
static char input[SourceBytes];static unsigned checks;
#define CHECK(value)do{++checks;if(!(value)){fprintf(stderr,"line %u: %s\n",__LINE__,#value);exit(1);}}while(0)

static void setup(unsigned at)
{
    Buffers* b=&buffers[at];
    banks[at]=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=SourceBytes,
        .tree={.nodes=b->nodes,.capacity=Nodes},
        .expressions={.nodes=b->expressions,.node_capacity=ExpressionNodes,.arguments=b->arguments,.argument_capacity=ExpressionNodes},
        .declarations={.functions=b->functions,.function_capacity=Functions,.parameters=b->parameters,.parameter_capacity=Parameters,
            .values=b->values,.value_capacity=ValueFields},.locals=b->locals,.local_capacity=128,
        .instructions=b->instructions,.instruction_capacity=Instructions,.function_code=b->code,.function_code_capacity=Functions,
        .execution_functions=b->executions,.execution_function_capacity=Functions,.graph=b->graph,.graph_capacity=Functions,
        .storage_layout={.fields=b->fields,.field_capacity=ValueFields,.owners=b->owners,.owner_capacity=32},
        .parameter_slots=b->slots,.parameter_slot_capacity=ValueFields};
}

// Engine presentation boundaries are controlled; actor registry, compilation,
// bytecode, event payload, owned storage and callback slots are production code.
static HSD_GObj object;static Fighter fighter;
NativeMatchPool nativeGObjPool;static NativeMatchPoolSlot object_slot[1];static uint32_t object_free[1];static HSD_GObj* links[32];
HSD_GObj** HSD_GObjPLinkHead=links;
static unsigned reports, writes;static bool retire_on_write;
void OSReport(char* format,...){(void)format;++reports;}
bool nativeModelEffect_Snapshot(void* object,uint32_t* generation){return false;}
void* nativeModelEffect_Resolve(void* object,uint32_t generation){return NULL;}
static void nativeCallback(HSD_GObj* object){(void)object;}
static bool installed_definition;static MotionState owned_states[8];
static NativeFighterDefinition owned_definition;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{(void)kind;return installed_definition?&owned_definition:NULL;}


bool nativeScriptBindings_Context(void* source,uint32_t kind,uint32_t generation,
    NativeScriptRuntimeContext* context,NativeScriptDiagnostic* diagnostic)
{
    if(!nativeCallbackSource_Alive(source,generation,kind))return false;
    *context=(NativeScriptRuntimeContext){.source=source,.source_class=kind,.generation=generation,.actor_id=generation};return true;
}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* context)
{return nativeCallbackSource_Alive(context->source,context->generation,context->source_class);}
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{return false;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    if(semantic!=NativeScriptBinding_PositionX || count || value->type!=NativeScriptType_F32)return false;
    CHECK(!nativeScriptFighter_CanActivateForm(&object)); // This VM frame is suspended in the native write.
    fighter.cur_pos.x=value->as.f32;++writes;
    if(retire_on_write)links[HSD_GOBJ_PLINK_FIGHTER]=NULL;
    return true;
}
bool nativeScriptBindingKinetics_Read(NativeScriptRuntimeContext* c,uint32_t s,NativeScriptValue* r,NativeScriptDiagnostic* d){return false;}
bool nativeScriptBindingKinetics_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,NativeScriptDiagnostic* d){return false;}
bool nativeScriptBindingKinetics_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){return false;}
bool nativeScriptBindingState_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){return false;}
bool nativeScriptBindingSetup_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d){return false;}

#define GET_FIGHTER(object) ((Fighter*)(object)->user_data)
static void ft_80088770(Fighter* object){(void)object;}
static void ft_800887CC(Fighter* object){(void)object;}
static void Fighter_8006CFE0(Fighter_GObj* object){(void)object;}
static ftCommonData common_data;
ftCommonData* p_ftCommonData=&common_data;
HSD_GObjEvent ftData_SpecialN[Ft_Kind_RuntimeCapacity],ftData_SpecialS[Ft_Kind_RuntimeCapacity],
    ftData_SpecialHi[Ft_Kind_RuntimeCapacity],ftData_SpecialLw[Ft_Kind_RuntimeCapacity],
    ftData_SpecialAirN[Ft_Kind_RuntimeCapacity],ftData_SpecialAirS[Ft_Kind_RuntimeCapacity],
    ftData_SpecialAirHi[Ft_Kind_RuntimeCapacity],ftData_SpecialAirLw[Ft_Kind_RuntimeCapacity];
void ftCommon_UpdateFacing(Fighter* fighter){fighter->facing_dir=-fighter->facing_dir;}
float ft_GetGroundFrictionMultiplier(Fighter* fighter){(void)fighter;return 1;}
#include "actual_callsites.h"

int main(void)
{
    // Admission and execution share the same typed ABI. Check it against the
    // independently source-audited event descriptors for every migrated hook.
    for(unsigned hook=0;hook<NativeScriptFighter_HookCount;++hook) {
        const NativeScriptFighterHookContract* contract=&nativeScriptFighterHookContracts[hook];
        CHECK(contract->name&&contract->argument_count<=2);
        for(unsigned previous=0;previous<hook;++previous)
            CHECK(strcmp(contract->name,nativeScriptFighterHookContracts[previous].name)!=0);
        const NativeScriptEventDescriptor* event=NULL;
        for(unsigned id=0;id<nativeScriptEvents_Count();++id) {
            const NativeScriptEventDescriptor* candidate=nativeScriptEvents_Descriptor(id);
            if(candidate&&candidate->context==NativeScriptEvent_Fighter&&
               candidate->kind==NativeScriptEvent_FighterHook&&!strcmp(candidate->name,contract->name)) {
                CHECK(event==NULL);event=candidate;
            }
        }
        CHECK(event&&event->result_type==contract->result_type&&event->argument_count==contract->argument_count);
        for(unsigned argument=0;argument<contract->argument_count;++argument)
            CHECK(event->arguments[argument].type==contract->arguments[argument]);
    }
    static NativeScriptFighterProgram prepared;
    const char* source="fighter:\n    behavior independent\nstate int counter\n"
        "func Count():\n    state.counter = state.counter + 1\n    position.x = state.counter\n"
        "func WithArgument(int value):\n    pass\n"
        "action Lifecycle:\n    on load:\n        Count()\n    on tick:\n        Count()\n"
        "    on death:\n        Count()\n    on late_update:\n        Count()\n    on form_activated:\n        pass\n"
        "    on damage:\n        Count()\n    on death1:\n        Count()\n"
        "    on death2:\n        Count()\n    on pre_hitlag:\n        Count()\n"
        "    on post_hitlag:\n        Count()\n    on deal_damage:\n        Count()\n"
        "    on state_change:\n        Count()\n    on user_data_remove:\n        Count()\n"
        "    on landing:\n        pass\n";
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,&object,sizeof(object),object_slot,object_free,1,sizeof(object),_Alignof(HSD_GObj),1));
    NativeMatchPoolReference object_reference;
    CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&object_reference)==&object);
    CHECK(nativeScriptFighter_BeginScene(1));
    setup(0);NativeScriptDiagnostic diagnostic={0};const NativeScriptCompileBank* accepted=NULL;
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source)};
    if(!nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic)){
        fprintf(stderr,"%u: %s\n",diagnostic.source.line,diagnostic.message);return 1;}
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,accepted,NULL,NULL,&prepared,&diagnostic));
    CHECK(prepared.hooks[NativeScriptFighter_Landing]!=UINT32_MAX);
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&prepared);
    fighter.kind=Ft_Kind_Mario;fighter.x8_spawnNum=11;fighter.gobj=&object;
    object.classifier=HSD_GOBJ_CLASS_FIGHTER;object.user_data=&fighter;links[HSD_GOBJ_PLINK_FIGHTER]=&object;
    CHECK(nativeScriptFighter_Attach(&object));CHECK(fighter.cur_pos.x==1);
    CHECK(nativeScriptFighter_CanActivateForm(&object));
    CHECK(nativeScriptFighter_HasHook(&object,NativeScriptFighter_FormActivated));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_FormActivated,NULL,0, NULL,NULL)==NativeScriptVM_Ok);
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Landing,NULL,0, NULL,NULL)==NativeScriptVM_Ok);
    CHECK(fighter.cur_pos.x==1);
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,NULL,NULL,NULL,&prepared,&diagnostic));
    CHECK(!nativeScriptFighter_CanPublish(Ft_Kind_Mario));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0, NULL,NULL)==NativeScriptVM_Ok);CHECK(fighter.cur_pos.x==2);
    for(uint32_t slot=0;slot<NativeScriptFighterCallback_ReflectHit;++slot){
        uint32_t fn=nativeScriptFighter_CallbackFunction(&object,slot);CHECK(fn!=UINT32_MAX);
        CHECK(nativeScriptFighter_BindCallback(&object,slot,fn));CHECK(nativeScriptFighter_CallbackBound(&object,slot));
        CHECK(nativeScriptFighter_RunCallback(&object,slot));CHECK(fighter.cur_pos.x==(float)(3+slot));
    }
    CHECK(!nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_StateChange));
    for(uint32_t slot=NativeScriptFighterCallback_ReflectHit;slot<NativeScriptFighterCallback_Count;++slot) {
        CHECK(nativeScriptFighter_CallbackFunction(&object,slot)==UINT32_MAX);
        CHECK(!nativeScriptFighter_CallbackBound(&object,slot));
    }
    CHECK(!nativeScriptFighter_BindCallback(&object,NativeScriptFighterCallback_Damage,1)); // helper has arguments
    CHECK(!nativeScriptFighter_BindCallback(&object,NativeScriptFighterCallback_Damage,
        nativeScriptFighter_CallbackFunction(&object,NativeScriptFighterCallback_PreHitlag)));
    CHECK(nativeScriptFighter_BindCallback(&object,NativeScriptFighterCallback_Damage,0)); // generic Void helper
    CHECK(nativeScriptFighter_RunCallback(&object,NativeScriptFighterCallback_Damage));CHECK(fighter.cur_pos.x==11);
    fighter.take_dmg_cb=nativeCallback;
    CHECK(!nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_Damage));
    CHECK(nativeScriptFighter_BindCallback(&object,NativeScriptFighterCallback_Damage,0));CHECK(fighter.take_dmg_cb==NULL);
    nativeScriptFighter_ResetMotionCallbacks(&object);
    CHECK(!nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_Damage));
    CHECK(!nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_Death2));
    CHECK(!nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_PreHitlag));
    CHECK(nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_Death1));
    CHECK(nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_Death3));
    CHECK(nativeScriptFighter_BindCallback(&object,NativeScriptFighterCallback_Damage,0));
    ftCommon_8007DB58(&object);CHECK(fighter.cur_pos.x==13); // Damage then persistent death1.
    CHECK(nativeScriptFighter_BindCallback(&object,NativeScriptFighterCallback_PreHitlag,
        nativeScriptFighter_CallbackFunction(&object,NativeScriptFighterCallback_PreHitlag)));
    Fighter_UnkRecursiveFunc_8006D044(&object);CHECK(fighter.cur_pos.x==14);CHECK(fighter.x2219_b5);
    CHECK(nativeScriptFighter_BindCallback(&object,NativeScriptFighterCallback_PostHitlag,
        nativeScriptFighter_CallbackFunction(&object,NativeScriptFighterCallback_PostHitlag)));
    Fighter_8006D10C(&object);CHECK(fighter.cur_pos.x==15);CHECK(!fighter.x2219_b5);
    uint8_t packed[16384],bad[16384];size_t packed_bytes=0;
    CHECK(nativeScriptFighter_Save(packed,sizeof(packed),&packed_bytes));
    CHECK(packed_bytes==sizeof(NativeScriptFighterSnapshotHeader)+sizeof(NativeScriptFighterSnapshotRecord)+4);
    CHECK(nativeScriptFighter_ValidateSnapshot(packed,packed_bytes));
    // Copy the resident interpreted owner, including bound execution and inline
    // variables. Neither restore below reconstructs storage/execution bindings.
    _Alignas(64) uint8_t arena_memory[64];static uint8_t history_memory[1024*1024];
    NativeMatchArena arena;NativeMatchArenaFrame history;
    CHECK(nativeMatchArena_Begin(&arena,arena_memory,sizeof(arena_memory),1));
    CHECK(nativeScriptFighter_AdmitHistory(&arena));
    CHECK(nativeMatchArena_AddRoot(&arena,&fighter.cur_pos,sizeof(fighter.cur_pos)));
    CHECK(nativeMatchArena_BindFrame(&arena,&history,history_memory,sizeof(history_memory)));
    CHECK(nativeMatchArena_Save(&history));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0,NULL,NULL)==NativeScriptVM_Ok);
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0,NULL,NULL)==NativeScriptVM_Ok);
    CHECK(fighter.cur_pos.x==17);
    CHECK(nativeMatchArena_Load(&history));CHECK(fighter.cur_pos.x==15);
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0,NULL,NULL)==NativeScriptVM_Ok);
    CHECK(fighter.cur_pos.x==16);
    CHECK(nativeMatchArena_Load(&history));
    size_t restored_bytes;CHECK(nativeScriptFighter_Save(bad,sizeof(bad),&restored_bytes));
    CHECK(restored_bytes==packed_bytes&&!memcmp(packed,bad,packed_bytes));

    CHECK(!nativeScriptFighter_Save(bad,sizeof(bad),(size_t*)bad));
    CHECK(!nativeScriptFighter_ValidateSnapshot(packed,packed_bytes-1));
    for(unsigned invalid=0;invalid<6;++invalid) {
        memcpy(bad,packed,packed_bytes);NativeScriptFighterSnapshotRecord changed;
        memcpy(&changed,bad+sizeof(NativeScriptFighterSnapshotHeader),sizeof(changed));
        switch(invalid) {
        case 0:changed.layout_identity^=1;break;
        case 1:changed.parameter_identity^=1;break;
        case 2:changed.callbacks[NativeScriptFighterCallback_Damage]=nativeScriptFighter_CallbackFunction(&object,NativeScriptFighterCallback_PreHitlag);break;
        case 3:changed.motions[NativeScriptMotion_Animation]=1;break; // WithArgument is not a zero-argument phase helper.
        case 4:changed.state_bytes++;break;
        case 5:changed.object.generation++;break; // Shape is valid, peer lifetime is not.
        }
        memcpy(bad+sizeof(NativeScriptFighterSnapshotHeader),&changed,sizeof(changed));
        CHECK(!nativeScriptFighter_Load(bad,packed_bytes));
        CHECK(fighter.cur_pos.x==15);
    }
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0, NULL,NULL)==NativeScriptVM_Ok);CHECK(fighter.cur_pos.x==16);
    CHECK(nativeScriptFighter_Load(packed,packed_bytes));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0, NULL,NULL)==NativeScriptVM_Ok);CHECK(fighter.cur_pos.x==16);
    memcpy(bad,packed,packed_bytes);NativeScriptFighterSnapshotRecord record;
    memcpy(&record,bad+sizeof(NativeScriptFighterSnapshotHeader),sizeof(record));
    record.program_identity^=1;memcpy(bad+sizeof(NativeScriptFighterSnapshotHeader),&record,sizeof(record));
    CHECK(!nativeScriptFighter_Load(bad,packed_bytes));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0, NULL,NULL)==NativeScriptVM_Ok);CHECK(fighter.cur_pos.x==17);
    CHECK(nativeScriptFighter_Load(packed,packed_bytes));
    nativeScriptFighter_Detach(&object);
    CHECK(nativeScriptFighter_Load(packed,packed_bytes)); // Restore the retired actor view with its peer still alive.
    CHECK(nativeScriptFighter_IsAttached(&object));
    retire_on_write=true;ftCommon_8007DB58(&object);
    CHECK(fighter.cur_pos.x==16); // Retired damage must not invoke death1.
    links[HSD_GOBJ_PLINK_FIGHTER]=&object;
    fighter.x2219_b5=false;Fighter_UnkRecursiveFunc_8006D044(&object);
    CHECK(fighter.cur_pos.x==17);CHECK(!fighter.x2219_b5); // No post-retirement flag mutation.
    links[HSD_GOBJ_PLINK_FIGHTER]=&object;retire_on_write=false;
    fighter.x8_spawnNum=12;nativeScriptFighter_NewLife(&object,12);
    CHECK(nativeScriptFighter_CanActivateForm(&object));
    CHECK(!nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_Death1));
    CHECK(!nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_Death3));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Death,NULL,0, NULL,NULL)==NativeScriptVM_Ok);CHECK(fighter.cur_pos.x==18);
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_LateUpdate,NULL,0, NULL,NULL)==NativeScriptVM_Ok);CHECK(fighter.cur_pos.x==19);
    links[HSD_GOBJ_PLINK_FIGHTER]=NULL;unsigned old=writes;
    CHECK(!nativeScriptFighter_CanActivateForm(&object));
    CHECK(!nativeScriptFighter_BindCallback(&object,NativeScriptFighterCallback_Damage,0));
    CHECK(!nativeScriptFighter_CallbackBound(&object,NativeScriptFighterCallback_Damage));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0, NULL,NULL)!=NativeScriptVM_Ok);CHECK(writes==old);
    links[HSD_GOBJ_PLINK_FIGHTER]=&object;
    nativeScriptFighter_Detach(&object);CHECK(fighter.cur_pos.x==20);
    CHECK(!nativeScriptFighter_IsAttached(&object));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,NULL,NULL,NULL,&prepared,&diagnostic));
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&prepared);
    const char* motions="fighter:\n    behavior independent\nstate int counter\n"
        "func Count():\n    state.counter = state.counter + 1\n    position.x = state.counter\n"
        "func TakesArgument(int value):\n    pass\nfunc ReturnsValue() -> int:\n    return 7\n"
        "move Projectile:\n    on special_n:\n        position.x = NeutralSpecial\n"
        "    on special_air_n:\n        position.x = NeutralSpecial\n"
        "    on ground_animation:\n        Count()\n    on air_animation:\n        Count()\n"
        "    on ground_physics:\n        Count()\n    on air_physics:\n        Count()\n"
        "    on ground_collision:\n        Count()\n    on air_collision:\n        Count()\n"
        "    on neutral_special_cue:\n        Count()\n"
        "move Side:\n    on special_s:\n        position.x = SideSpecial\n    on special_air_s:\n        position.x = SideSpecial\n"
        "move Up:\n    on special_hi:\n        position.x = UpSpecial\n    on special_air_hi:\n        position.x = UpSpecial\n"
        "move Down:\n    on special_lw:\n        position.x = DownSpecial\n    on special_air_lw:\n        position.x = DownSpecial\n"
        "action Idle:\n    on state_14_physics:\n        Count()\n";
    NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=8,
        .ground={341,343,345,347},.air={342,344,346,348}};
    setup(0);accepted=NULL;request=(NativeScriptCompileRequest){.source=motions,.source_bytes=(uint32_t)strlen(motions),.motion_layout=&layout};
    if(!nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic)){
        fprintf(stderr,"motions %u: %s\n",diagnostic.source.line,diagnostic.message);return 1;}
    CHECK(!nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,accepted,NULL,NULL,&prepared,&diagnostic)); // No owned numeric motion layout.
    owned_definition=(NativeFighterDefinition){.source_kind=Ft_Kind_None,.motion_states=owned_states,
        .motion_state_first=341,.motion_state_count=8,.special_ground_state={341,343,345,347},.special_air_state={342,344,346,348}};
    installed_definition=true;
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,accepted,&layout,NULL,&prepared,&diagnostic));
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&prepared);
    CHECK(nativeScriptFighter_Program(Ft_Kind_Mario)->motion_count==3);
    fighter.x8_spawnNum=21;CHECK(nativeScriptFighter_Attach(&object));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_SpecialN,NULL,0, NULL,NULL)==NativeScriptVM_Ok);CHECK(fighter.cur_pos.x==341);
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_SpecialAirN,NULL,0, NULL,NULL)==NativeScriptVM_Ok);CHECK(fighter.cur_pos.x==342);
    common_data.x21C=common_data.x218=0.5f;common_data.x220=0.3f;
    fighter.facing_dir=1;fighter.x676_x=255;fighter.input.pressed_buttons=HSD_PAD_B;
    fighter.gr_vel=8;fighter.co_attrs.specials_ground_speed_retention=0.5f;
    CHECK(ftCo_800D6824(&object));CHECK(fighter.cur_pos.x==341);
    CHECK(ftCo_SpecialS_CheckInput(&object));CHECK(fighter.cur_pos.x==343);CHECK(fighter.gr_vel==4);
    CHECK(ftCo_Attack100_CheckInput(&object));CHECK(fighter.cur_pos.x==345);
    CHECK(ftCo_800D68C0(&object));CHECK(fighter.cur_pos.x==347);
    for(unsigned special=0;special<4;++special) {
        fighter.facing_dir=1;fighter.x2227_b5=false;
        fighter.input.lstick[0].x=special==1?-1:0;
        fighter.input.lstick[0].y=special==2?1:special==3?-1:0;
        CHECK(ftCo_SpecialAir_CheckInput(&object));CHECK(fighter.cur_pos.x==(float)(342+2*special));
        CHECK(fighter.x2227_b5);
        if(special==1)CHECK(fighter.facing_dir==-1);
    }
    retire_on_write=true;fighter.x2227_b5=false;fighter.input.lstick[0].x=fighter.input.lstick[0].y=0;
    CHECK(ftCo_SpecialAir_CheckInput(&object));CHECK(!fighter.x2227_b5);
    links[HSD_GOBJ_PLINK_FIGHTER]=&object;retire_on_write=false;
    unsigned delivered=0;
    for(unsigned plane=0;plane<2;++plane){
        fighter.anim_cb=fighter.phys_cb=fighter.coll_cb=fighter.accessory4_cb=nativeCallback;
        fighter.input_cb=nativeCallback;nativeScriptFighter_SelectMotion(&object,341+plane);
        CHECK(fighter.anim_cb==NULL && fighter.phys_cb==NULL && fighter.coll_cb==NULL && fighter.accessory4_cb==NULL);
        CHECK(!nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Interrupt));CHECK(fighter.input_cb==nativeCallback);
        for(unsigned phase=0;phase<NativeScriptMotion_Count;++phase){
            if(phase==NativeScriptMotion_Interrupt)continue;
            CHECK(nativeScriptFighter_MotionBound(&object,phase));
            if(phase==NativeScriptMotion_Accessory){
                float previous=fighter.cur_pos.x;fighter.throw_flags_b0=false;
                CHECK(nativeScriptFighter_RunMotion(&object,phase));CHECK(fighter.cur_pos.x==previous);
                fighter.throw_flags_b0=true;
            }
            CHECK(nativeScriptFighter_RunMotion(&object,phase));
            if(phase==NativeScriptMotion_Accessory)CHECK(!fighter.throw_flags_b0);
            CHECK(fighter.cur_pos.x==(float)++delivered);
        }
    }
    fighter.phys_cb=nativeCallback;CHECK(!nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Physics));
    nativeScriptFighter_SelectMotion(&object,14);CHECK(fighter.phys_cb==NULL);
    CHECK(nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Physics));
    CHECK(nativeScriptFighter_RunMotion(&object,NativeScriptMotion_Physics));CHECK(fighter.cur_pos.x==(float)++delivered);
    nativeScriptFighter_SelectMotion(&object,15);CHECK(!nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Physics));
    uint32_t helper=UINT32_MAX,argument_helper=UINT32_MAX,value_helper=UINT32_MAX;
    for(unsigned i=0;i<banks[0].declarations.function_count;++i) {
        const NativeScriptFunctionDeclaration* function=&banks[0].declarations.functions[i];
        if(nativeScriptSource_SpanEquals(&banks[0].tree,function->name,"Count"))helper=i;
        if(nativeScriptSource_SpanEquals(&banks[0].tree,function->name,"TakesArgument"))argument_helper=i;
        if(nativeScriptSource_SpanEquals(&banks[0].tree,function->name,"ReturnsValue"))value_helper=i;
    }
    CHECK(helper!=UINT32_MAX&&argument_helper!=UINT32_MAX&&value_helper!=UINT32_MAX);
    CHECK(!nativeScriptFighter_BindMotionPhase(&object,NativeScriptMotion_Physics,argument_helper,false));
    CHECK(!nativeScriptFighter_BindMotionPhase(&object,NativeScriptMotion_Physics,value_helper,false));
    CHECK(!nativeScriptFighter_BindMotionPhase(&object,NativeScriptMotion_Physics,prepared.hooks[NativeScriptFighter_SpecialN],false));
    CHECK(!nativeScriptFighter_BindMotionPhase(&object,NativeScriptMotion_Physics,helper,true));
    for(unsigned phase=0;phase<NativeScriptMotion_Count;++phase) {
        CHECK(nativeScriptFighter_BindMotionPhase(&object,phase,helper,false));
        CHECK(nativeScriptFighter_MotionBound(&object,phase));
    }
    CHECK(nativeScriptFighter_Save(packed,sizeof(packed),&packed_bytes));
    CHECK(nativeScriptFighter_ValidateSnapshot(packed,packed_bytes));
    for(unsigned phase=0;phase<NativeScriptMotion_Count;++phase) {
        CHECK(nativeScriptFighter_RunMotion(&object,phase));CHECK(fighter.cur_pos.x==(float)++delivered);
    }
    CHECK(nativeScriptFighter_Load(packed,packed_bytes));delivered-=NativeScriptMotion_Count;
    fighter.cur_pos.x=(float)delivered; // Restore the separate native Fighter peer.
    fighter.phys_cb=nativeCallback;CHECK(!nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Physics));
    fighter.phys_cb=NULL; // The separate Fighter snapshot owns native pointers.
    CHECK(nativeScriptFighter_Load(packed,packed_bytes));CHECK(nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Physics));
    memcpy(bad,packed,packed_bytes);memcpy(&record,bad+sizeof(NativeScriptFighterSnapshotHeader),sizeof(record));
    record.motions[NativeScriptMotion_Physics]=prepared.hooks[NativeScriptFighter_SpecialN];
    memcpy(bad+sizeof(NativeScriptFighterSnapshotHeader),&record,sizeof(record));CHECK(!nativeScriptFighter_ValidateSnapshot(bad,packed_bytes));
    record.motions[NativeScriptMotion_Physics]=argument_helper;
    memcpy(bad+sizeof(NativeScriptFighterSnapshotHeader),&record,sizeof(record));CHECK(!nativeScriptFighter_ValidateSnapshot(bad,packed_bytes));
    CHECK(nativeScriptFighter_BindMotionPhase(&object,NativeScriptMotion_Accessory,helper,true));
    fighter.throw_flags_b0=false;CHECK(nativeScriptFighter_RunMotion(&object,NativeScriptMotion_Accessory));CHECK(fighter.cur_pos.x==(float)delivered);
    fighter.throw_flags_b0=true;CHECK(nativeScriptFighter_RunMotion(&object,NativeScriptMotion_Accessory));CHECK(!fighter.throw_flags_b0&&fighter.cur_pos.x==(float)++delivered);
    CHECK(nativeScriptFighter_Save(packed,sizeof(packed),&packed_bytes));CHECK(nativeScriptFighter_ValidateSnapshot(packed,packed_bytes));
    CHECK(nativeScriptFighter_BindMotionPhase(&object,NativeScriptMotion_Accessory,UINT32_MAX,true));CHECK(!nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Accessory));
    CHECK(nativeScriptFighter_Load(packed,packed_bytes));CHECK(nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Accessory));
    nativeScriptFighter_SelectMotion(&object,14);CHECK(!nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Accessory));
    CHECK(nativeScriptFighter_MotionBound(&object,NativeScriptMotion_Physics));
    nativeScriptFighter_Detach(&object);
    CHECK(!nativeScriptFighter_BindMotionPhase(&object,NativeScriptMotion_Physics,helper,false));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,NULL,NULL,NULL,&prepared,&diagnostic));
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&prepared);
    // Reject aliases that would compete for the same actual phase before publishing.
    const char* duplicate="fighter:\n    behavior independent\nmove A:\n    on special_n:\n        pass\n    on ground_physics:\n        pass\naction B:\n    on state_341_physics:\n        pass\n";
    setup(1);request=(NativeScriptCompileRequest){.source=duplicate,.source_bytes=(uint32_t)strlen(duplicate),.motion_layout=&layout};
    CHECK(!nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));CHECK(strstr(diagnostic.message,"Duplicate callback target")!=NULL);
    CHECK(accepted==&banks[0]);
    // Fault through actual actor execution. The controlled position reader is
    // unavailable; the valid authored Tick therefore fails in the real VM.
    const char* fault_source="fighter:\n    behavior independent\n"
        "func Recipient():\n    position.x = 99\n"
        "action Fault:\n    on load:\n        pass\n"
        "    on tick:\n        position.x = position.y\n";
    setup(1);request=(NativeScriptCompileRequest){.source=fault_source,.source_bytes=(uint32_t)strlen(fault_source)};
    CHECK(nativeScriptCompile_Build(&banks[1],&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,accepted,NULL,NULL,&prepared,&diagnostic));
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&prepared);
    uint32_t recipient=UINT32_MAX;
    for(unsigned i=0;i<accepted->declarations.function_count;++i)
        if(nativeScriptSource_SpanEquals(&accepted->tree,accepted->declarations.functions[i].name,"Recipient"))recipient=i;
    CHECK(recipient!=UINT32_MAX);CHECK(nativeScriptFighter_Attach(&object));
    CHECK(nativeScriptFighter_Run(&object,NativeScriptFighter_Tick,NULL,0, NULL,NULL)==NativeScriptVM_Error);
    CHECK(nativeScriptFighter_Status(&object)==NativeScriptVM_Error);
    CHECK(nativeScriptRuntime_VM()->diagnostic.message[0]!=0);
    unsigned fault_writes=writes;
    for(unsigned unrelated=0;unrelated<2;++unrelated) {
        memset(&nativeScriptRuntime_VM()->diagnostic,0,sizeof(NativeScriptDiagnostic));
        if(unrelated)strcpy(nativeScriptRuntime_VM()->diagnostic.message,"Unrelated shared VM diagnostic");
        memset(&diagnostic,0,sizeof(diagnostic));
        NativeScriptValue untouched={.type=NativeScriptType_I32,.as.i64=73};
        CHECK(nativeScriptFighter_Helper(&object,fighter.x8_spawnNum,recipient,NULL,0,&untouched,&diagnostic)==NativeScriptVM_Error);
        CHECK(strcmp(diagnostic.message,"Fighter helper recipient is faulted")==0);
        CHECK(untouched.type==NativeScriptType_I32&&untouched.as.i64==73);
        CHECK(writes==fault_writes);
    }
    nativeScriptFighter_Detach(&object);
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,NULL,NULL,NULL,&prepared,&diagnostic));
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&prepared);
    CHECK(nativeScriptFighter_RetireScene());CHECK(!nativeScriptFighter_ValidateSnapshot(packed,packed_bytes));
    nativeGObjPool.lifetime=2;CHECK(nativeScriptFighter_BeginScene(2));
    CHECK(!nativeScriptFighter_Load(packed,packed_bytes));
    printf("%u interpreter fighter callback/lifetime/motion/snapshot checks; %u writes.\n",checks,writes);return 0;
}

bool nativeScriptBindingCombat_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{(void)context;(void)semantic;(void)values;(void)count;(void)result;(void)diagnostic;return false;}

bool nativeScriptBindingTransition_Invoke(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* r,NativeScriptDiagnostic* d)
{(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
