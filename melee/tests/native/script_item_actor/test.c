#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <math.h>
#include "boundary.h"
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_match_arena.h>
#include <melee/gm/native_rollback_state.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum { SourceBytes=65536, Nodes=2048, Expressions=4096, Functions=64, Parameters=128, Fields=128, Instructions=8192 };
typedef struct Buffers {
    char source[SourceBytes];NativeScriptSourceNode nodes[Nodes];NativeScriptIRNode expressions[Expressions];uint32_t arguments[Expressions];
    NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[Parameters];NativeScriptValueDeclaration fields[Fields];
    NativeScriptLocal locals[64];NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];NativeScriptExecutionFunction executions[Functions];
    NativeScriptCompileFunctionScratch graph[Functions];NativeScriptStorageField storage[Fields];NativeScriptStorageOwner owners[16];NativeScriptParameterSlot slots[Fields];
} Buffers;
static Buffers buffers;static NativeScriptCompileBank bank;static NativeScriptItemProgram specs[4];
static NativeScriptItemState states[4][4];static NativeScriptItemSignal signals[2];
static NativeItemDefinition recipes[2];static Article assets;
static Item native_items[NATIVE_SCRIPT_ITEM_ACTORS+1];static HSD_GObj* objects[NATIVE_SCRIPT_ITEM_ACTORS+1];
static uint8_t packed[5*1024*1024],bad[5*1024*1024],saved_actors[sizeof(nativeScriptItemActors)];
static uint16_t saved_index[NATIVE_GOBJ_CAPACITY];
static unsigned checks,cases,phase_ticks;static NativeScriptVMStatus delayed_status;
static void shared_void(HSD_GObj* object){(void)object;}
static bool shared_bool(HSD_GObj* object){(void)object;return false;}
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %u: %s\n",(unsigned)__LINE__,#v);exit(1);}}while(0)
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{for(unsigned i=0;i<4;++i)if(specs[i].bank && specs[i].author_kind==author && specs[i].owner==owner)return &specs[i];return NULL;}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t node,NativeScriptDiagnostic* d)
{
    (void)unused;const NativeScriptSourceNode* n=&tree->nodes[node];
    if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 2");
    if(n->kind==NativeScriptSource_Item)return nativeScriptSource_SpanEquals(tree,n->value,"damage 4 angle 45 lifetime 90");
    if(n->kind==NativeScriptSource_ArticleState)return nativeScriptSource_SpanEquals(tree,n->value,"0 anim -1")||nativeScriptSource_SpanEquals(tree,n->value,"1 anim -1");
    if(n->kind==NativeScriptSource_Function||n->kind==NativeScriptSource_State||n->kind==NativeScriptSource_Parameter)return n->parent<tree->count&&(tree->nodes[n->parent].kind==NativeScriptSource_Article||tree->nodes[n->parent].kind==NativeScriptSource_Item);
    if(n->kind==NativeScriptSource_Callback){NativeScriptEventResolution event;return nativeScriptEvents_Resolve(tree,n,&event,d);}
    return nativeScriptCompile_MinimalMetadata(NULL,tree,node,d);
}
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=buffers.source,.source_capacity=SourceBytes,.tree={.nodes=buffers.nodes,.capacity=Nodes},
        .expressions={.nodes=buffers.expressions,.node_capacity=Expressions,.arguments=buffers.arguments,.argument_capacity=Expressions},
        .declarations={.functions=buffers.functions,.function_capacity=Functions,.parameters=buffers.parameters,.parameter_capacity=Parameters,.values=buffers.fields,.value_capacity=Fields},
        .locals=buffers.locals,.local_capacity=64,.instructions=buffers.instructions,.instruction_capacity=Instructions,.function_code=buffers.code,.function_code_capacity=Functions,
        .execution_functions=buffers.executions,.execution_function_capacity=Functions,.graph=buffers.graph,.graph_capacity=Functions,
        .storage_layout={.fields=buffers.storage,.field_capacity=Fields,.owners=buffers.owners,.owner_capacity=16},.parameter_slots=buffers.slots,.parameter_slot_capacity=Fields};
    const char* source="fighter:\n    behavior independent\n"
        "article It_Kind_Mario_Fire states 2:\n    state int count\n    state bool marked\n    state float amount\n"
        "    func Extra():\n        item.state.count = item.state.count + 3\n"
        "    on spawned:\n            item.state.count = 1\n"
        "    on destroyed:\n            item.state.count = item.state.count + 100\n            item.state.marked = true\n"
        "    on accessory:\n            Extra()\n"
        "    on interaction:\n            item.state.marked = true\n"
        "    on signal Ping:\n            item.state.amount = signal.value\n"
        "    on held_request:\n            return event.action == 7\n"
        "    on damage_received:\n            return 1 / item.state.count > 0\n"
        "    state 0 anim -1:\n        on animation:\n            item.state.count = item.state.count + 1\n            position.x = item.state.count\n            item.state.amount = item.state.amount + 1\n            return false\n"
        "        on physics:\n            item.state.amount = item.state.amount + 1\n        on collision:\n            return true\n"
        "    state 1 anim -1:\n        on animation:\n            return false\n"
        "article It_Kind_Ness_PKFire states 2:\n    state int count\n    on spawned:\n            item.state.count = 77\n    state 0 anim -1:\n    state 1 anim -1:\n"
        "item Apple damage 4 angle 45 lifetime 90:\n    state int count\n    on tick:\n        item.state.count = item.state.count + 1\n"
        "item Pear damage 4 angle 45 lifetime 90:\n    state int count\n    on tick:\n        item.state.count = item.state.count + 2\n";
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
    if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)){fprintf(stderr,"compile %u:%u: %s\n",d.source.line,d.source.column,d.message);exit(1);}
    unsigned entity=0;
    for(unsigned node=0;node<bank.tree.count;++node)if(bank.tree.nodes[node].kind==NativeScriptSource_Article||bank.tree.nodes[node].kind==NativeScriptSource_Item){
        CHECK(entity<4);NativeScriptItemProgram* p=&specs[entity];
        *p=(NativeScriptItemProgram){.bank=&bank,.identity=entity+100,.author_kind=Ft_Kind_Mario,.kind=entity<2?It_Kind_Mario_Fire:It_Kind_NativeHoldable,
            .recipe_index=entity<2?UINT32_MAX:entity-2,.owner=node,.article=&assets,.recipe=entity<2?NULL:&recipes[entity-2],.states=states[entity],.state_count=entity<2?2:4};
        p->drawing=bank.tree.nodes[node].kind==NativeScriptSource_Item?NativeScriptItemDrawing_Billboard:NativeScriptItemDrawing_Standard;
        for(unsigned i=0;i<NativeScriptItemHook_Count;++i)p->hooks[i]=UINT32_MAX;
        for(unsigned state=0;state<4;++state){states[entity][state].animation_id=-1;for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase)states[entity][state].functions[phase]=UINT32_MAX;}
        ++entity;
    }
    CHECK(entity==4);
    for(unsigned f=0;f<bank.declarations.function_count;++f){
        const NativeScriptFunctionDeclaration* fn=&bank.declarations.functions[f];if(!fn->callback)continue;
        NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&bank.tree,&bank.tree.nodes[fn->source_node],&e,&d));
        NativeScriptItemProgram* p=NULL;for(unsigned i=0;i<4;++i)if(specs[i].owner==fn->owner)p=&specs[i];CHECK(p);
        const char* name=e.descriptor->name;
        if(e.descriptor->kind==NativeScriptEvent_ArticleState){unsigned phase=!strcmp(name,"animation")?NativeScriptItemPhase_Animation:!strcmp(name,"physics")?NativeScriptItemPhase_Physics:NativeScriptItemPhase_Collision;states[p-specs][e.motion].functions[phase]=f;}
        else if(e.descriptor->kind==NativeScriptEvent_ArticleSignal){signals[0]=(NativeScriptItemSignal){7,f};p->signals=signals;p->signal_count=1;}
        else{unsigned hook=!strcmp(name,"spawned")?NativeScriptItemHook_Spawned:!strcmp(name,"destroyed")?NativeScriptItemHook_Destroyed:
            !strcmp(name,"accessory")?NativeScriptItemHook_Accessory:!strcmp(name,"interaction")?NativeScriptItemHook_Interaction:
            !strcmp(name,"held_request")?NativeScriptItemHook_HeldRequest:!strcmp(name,"damage_received")?NativeScriptItemHook_DamageReceived:NativeScriptItemHook_WorkshopTick;p->hooks[hook]=f;}
    }
    for(unsigned i=0;i<4;++i)CHECK(nativeScriptItem_ProgramValid(&specs[i]));
    uint32_t original=specs[0].hooks[NativeScriptItemHook_Spawned];specs[0].hooks[NativeScriptItemHook_Spawned]=specs[0].hooks[NativeScriptItemHook_Accessory];
    CHECK(!nativeScriptItem_ProgramValid(&specs[0]));specs[0].hooks[NativeScriptItemHook_Spawned]=original;
    recipes[0].on_tick=shared_void;CHECK(!nativeScriptItem_ProgramValid(&specs[2]));recipes[0].on_tick=NULL;
    states[0][0].animation_id=-2;CHECK(!nativeScriptItem_ProgramValid(&specs[0]));states[0][0].animation_id=-1;++cases;
}
static HSD_GObj* create(unsigned slot,unsigned spec)
{
    HSD_GObj* object=GObj_Create(6,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
    Item* item=&native_items[slot];memset(item,0,sizeof(*item));item->entity=object;item->kind=specs[spec].kind;item->x1C=(s32)slot;item->msid=-1;
    item->native_article_author_kind=specs[spec].author_kind;item->native_script_owner=specs[spec].owner;item->xC4_article_data=&assets;
    if(item->kind==It_Kind_NativeHoldable){item->xDD4_itemVar.native_holdable.owner_kind=Ft_Kind_Mario;item->xDD4_itemVar.native_holdable.definition_index=(u16)specs[spec].recipe_index;}
    GObj_InitUserData(object,6,fixture_userdata_remove,item);objects[slot]=object;return object;
}
static NativeScriptValue state(HSD_GObj* object,const char* name)
{
    NativeScriptItemActor* a=NULL;for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==object)a=&nativeScriptItemActors[i];CHECK(a);
    for(unsigned i=0;i<bank.declarations.value_count;++i){const NativeScriptValueDeclaration* d=&bank.declarations.values[i];if(d->owner==a->program->owner&&nativeScriptSource_SpanEquals(&bank.tree,d->name,name)){
        NativeScriptValue out;NativeScriptDiagnostic error;CHECK(nativeScriptStorage_Read(&a->storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&out,&error));return out;}}
    CHECK(false);return(NativeScriptValue){0};
}
static void select_state(HSD_GObj* object,unsigned which)
{int32_t anim=123;CHECK(nativeScriptItem_StateAnimation(object,which,&anim)&&anim==-1);CHECK(nativeScriptItem_SelectState(object,which));((Item*)object->user_data)->msid=(s32)which;}
static void step(HSD_GObj* object)
{nativeScriptItem_BeginTick(object);NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};delayed_status=nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Animation,&result);if(delayed_status!=NativeScriptVM_Ok)CHECK(result.type==NativeScriptType_U64&&result.as.u64==99);++phase_ticks;}
static void reject_unchanged(size_t bytes)
{
    memcpy(saved_actors,nativeScriptItemActors,sizeof(saved_actors));uint64_t epoch=nativeScriptItemSceneEpoch;
    memcpy(saved_index,nativeScriptItemActorByObject,sizeof(saved_index));
    CHECK(!nativeScriptItem_Load(bad,bytes));CHECK(!memcmp(saved_actors,nativeScriptItemActors,sizeof(saved_actors))&&epoch==nativeScriptItemSceneEpoch&&
        !memcmp(saved_index,nativeScriptItemActorByObject,sizeof(saved_index)));++cases;
}
static void copy_history(void)
{
    static uint8_t history_bytes[64*1024*1024];NativeMatchArenaFrame history;
    NativeMatchArena* arena=&nativeMatchSceneArena;
    CHECK(nativeScriptItemInitializedCount==4);
    CHECK(nativeScriptItem_AdmitHistory(arena));
    CHECK(nativeMatchArena_BindFrame(arena,&history,history_bytes,sizeof(history_bytes)));
    CHECK(nativeMatchArena_Save(&history));
    size_t bytes,after;CHECK(nativeScriptItem_Save(packed,sizeof(packed),&bytes));
    HSD_GObj* future=create(4,0);CHECK(nativeScriptItem_Attach(future,&specs[0]));
    NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,future,&ref));
    CHECK(nativeScriptItemInitializedCount==5);
    nativeScriptItemActors[0].bytes[0]^=0xFF;
    nativeScriptItemActors[0].storage.state=NULL;
    nativeScriptItemActors[0].storage.identity^=0x1234;
    nativeScriptItemActorByObject[ref.slot]=UINT16_MAX;
    for(unsigned pass=0;pass<2;++pass){
        CHECK(nativeMatchArena_Load(&history));CHECK(nativeScriptItemInitializedCount==4);
        CHECK(!nativeMatchPool_Resolve(ref));
        CHECK(nativeScriptItem_Save(bad,sizeof(bad),&after)&&bytes==after&&!memcmp(packed,bad,bytes));
        HSD_GObj* replay=create(4,0);CHECK(replay==future&&nativeScriptItem_Attach(replay,&specs[0]));
        NativeMatchPoolReference replay_ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,replay,&replay_ref));
        CHECK(replay_ref.generation==ref.generation&&nativeScriptItemInitializedCount==5);
        CHECK(nativeScriptItemActors[4].storage.state==nativeScriptItemActors[4].bytes);
        CHECK(nativeScriptItemActors[4].bytes[0]==0);
        memset(nativeScriptItemActors[4].bytes,0xFE,sizeof(nativeScriptItemActors[4].bytes));
    }
    CHECK(nativeMatchArena_Load(&history));++cases;
}

int main(void)
{
    prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=2,.gproc_pri_max=8};HSD_GObj_80391304(&config);
    if(!nativeScriptItemSceneEpoch)CHECK(nativeScriptItem_BeginScene(nativeGObjPool.lifetime));
    HSD_GObj* a=create(0,0),*b=create(1,1),*apple=create(2,2),*pear=create(3,3);
    CHECK(nativeScriptItem_Attach(a,&specs[0])&&nativeScriptItem_Attach(b,&specs[1])&&nativeScriptItem_Attach(apple,&specs[2])&&nativeScriptItem_Attach(pear,&specs[3]));
    copy_history();
    float creation=99;CHECK(nativeScriptItem_GetCreationValue(a,&creation)&&creation==0);
    CHECK(nativeScriptItem_SetCreationValue(a,7.125f)&&nativeScriptItem_GetCreationValue(a,&creation)&&creation==7.125f);
    CHECK(!nativeScriptItem_SetCreationValue(a,NAN)&&!nativeScriptItem_SetCreationValue(a,INFINITY));
    CHECK(nativeScriptItem_GetCreationValue(a,&creation)&&creation==7.125f);
    CHECK(!nativeScriptItem_GetCreationValue(NULL,&creation)&&creation==7.125f);
    CHECK(!nativeScriptItem_GetCreationValue(a,NULL));++cases;
    CHECK(!nativeScriptItem_Attach(a,&specs[0])&&nativeScriptItem_HasActors(Ft_Kind_Mario)&&nativeScriptItem_ReferencesProgram(&bank));
    CHECK(nativeScriptItem_Spec(a)==&specs[0]&&nativeScriptItem_Spec(b)==&specs[1]);NativeScriptValue result;
    CHECK(nativeScriptItem_RunHook(a,NativeScriptItemHook_Spawned,NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(nativeScriptItem_RunHook(b,NativeScriptItemHook_Spawned,NULL,0,&result)==NativeScriptVM_Ok);
    HSD_GObj* relation_fighter=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(relation_fighter);
    static Fighter relation_data;relation_data=(Fighter){.gobj=relation_fighter,.kind=Ft_Kind_Mario,.x8_spawnNum=0};
    relation_fighter->user_data=&relation_data;
    CHECK(nativeScriptItem_SetAttachmentOwner(a,relation_fighter));
    CHECK(nativeScriptItem_AttachmentOwner(a)==relation_fighter&&!nativeScriptItem_Creator(a));
    size_t relation_bytes;CHECK(nativeScriptItem_Save(packed,sizeof(packed),&relation_bytes));
    CHECK(nativeScriptItem_ClearAttachmentOwner(a)&&!nativeScriptItem_AttachmentOwner(a));
    CHECK(nativeScriptItem_Load(packed,relation_bytes)&&nativeScriptItem_AttachmentOwner(a)==relation_fighter);
    memcpy(bad,packed,relation_bytes);NativeScriptItemSnapshotRecord* relation_bad=(void*)(bad+sizeof(NativeScriptItemSnapshotHeader));
    relation_bad->attachment_owner.object.lifetime^=1;CHECK(!nativeScriptItem_ValidateSnapshot(bad,relation_bytes));
    relation_data.x8_spawnNum=1;CHECK(!nativeScriptItem_AttachmentOwner(a));relation_data.x8_spawnNum=0;
    HSD_GObjFree(relation_fighter);relation_fighter=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(relation_fighter);
    relation_fighter->user_data=&relation_data;relation_data.gobj=relation_fighter;
    CHECK(!nativeScriptItem_AttachmentOwner(a));
    CHECK(nativeScriptItem_Save(packed,sizeof(packed),&relation_bytes));
    CHECK(nativeScriptItem_Load(packed,relation_bytes)&&!nativeScriptItem_AttachmentOwner(a));
    CHECK(nativeScriptItem_ClearAttachmentOwner(a));HSD_GObjFree(relation_fighter);++cases;
    CHECK(state(a,"count").as.i64==1&&state(b,"count").as.i64==77);++cases;
    CHECK(nativeScriptItem_RunHook(apple,NativeScriptItemHook_WorkshopTick,NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(nativeScriptItem_RunHook(pear,NativeScriptItemHook_WorkshopTick,NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(state(apple,"count").as.i64==1&&state(pear,"count").as.i64==2);++cases;
    CHECK(!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));
    uint32_t accessory=nativeScriptItem_CallbackFunction(a,NativeScriptItemCallback_Accessory);CHECK(accessory!=UINT32_MAX);
    CHECK(nativeScriptItem_BindCallback(a,NativeScriptItemCallback_Accessory,accessory)&&nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));
    CHECK(nativeScriptItem_RunHook(a,NativeScriptItemHook_Accessory,NULL,0,&result)==NativeScriptVM_Ok&&state(a,"count").as.i64==4);
    CHECK(!nativeScriptItem_BindCallback(a,NativeScriptItemCallback_Touched,accessory));nativeScriptItem_ResetStateCallbacks(a);CHECK(!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));++cases;
    uint32_t helper=UINT32_MAX;for(unsigned f=0;f<bank.declarations.function_count;++f)
        if(bank.declarations.functions[f].owner==specs[0].owner&&!bank.declarations.functions[f].callback&&nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[f].name,"Extra"))helper=f;
    CHECK(helper!=UINT32_MAX&&nativeScriptItem_BindCallback(a,NativeScriptItemCallback_GrabDealt,helper));
    CHECK(nativeScriptItem_BindCallback(a,NativeScriptItemCallback_Accessory,helper));native_items[0].on_accessory=shared_void;
    CHECK(!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));native_items[0].on_accessory=NULL;CHECK(!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));
    nativeScriptItem_ResetStateCallbacks(a);CHECK(nativeScriptItem_HookBound(a,NativeScriptItemHook_GrabDealt));
    CHECK(nativeScriptItem_BindCallback(a,NativeScriptItemCallback_GrabDealt,UINT32_MAX));++cases;
    CHECK(nativeScriptItem_RunSignal(a,7,1.25f,&result)==NativeScriptVM_Ok&&state(a,"amount").as.f32==1.25f);
    NativeScriptValue args[3]={{.type=NativeScriptType_I32,.as.i64=7},{.type=NativeScriptType_F32,.as.f32=2},{.type=NativeScriptType_F32,.as.f32=3}};
    CHECK(nativeScriptItem_RunHook(a,NativeScriptItemHook_HeldRequest,args,3,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Bool&&result.as.u64==1);
    args[0].type=NativeScriptType_F32;result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(nativeScriptItem_RunHook(a,NativeScriptItemHook_HeldRequest,args,3,&result)==NativeScriptVM_Error&&result.type==NativeScriptType_U64&&result.as.u64==99);++cases;
    select_state(a,0);fixture_transition=true;step(a);CHECK(delayed_status==NativeScriptVM_Ok&&nativeScriptItem_Revision(a)==2&&state(a,"amount").as.f32==2.25f); // Code continues after state change.
    select_state(a,0);CHECK(nativeScriptItem_RunPhase(a,NativeScriptItemPhase_Collision,&result)==NativeScriptVM_Ok&&result.as.u64==1);++cases;
    size_t bytes;CHECK(nativeScriptItem_Save(packed,sizeof(packed),&bytes));CHECK(nativeScriptItem_ValidateSnapshot(packed,bytes));
    CHECK(nativeScriptItem_SetCreationValue(a,-0.0f));
    CHECK(nativeScriptItem_Load(packed,bytes)&&nativeScriptItem_GetCreationValue(a,&creation)&&creation==7.125f);++cases;
    memcpy(bad,packed,bytes);unsigned bool_offset=0;
    for(unsigned f=0;f<bank.storage_layout.field_count;++f)if(bank.storage_layout.fields[f].owner==specs[0].owner&&bank.storage_layout.fields[f].type==NativeScriptType_Bool)bool_offset=bank.storage_layout.fields[f].offset;
    bad[sizeof(NativeScriptItemSnapshotHeader)+sizeof(NativeScriptItemSnapshotRecord)+bool_offset]=2;reject_unchanged(bytes);
    memcpy(bad,packed,bytes);NativeScriptItemSnapshotRecord first;memcpy(&first,bad+sizeof(NativeScriptItemSnapshotHeader),sizeof(first));
    size_t second=sizeof(NativeScriptItemSnapshotHeader)+sizeof(first)+first.state_bytes;NativeScriptItemSnapshotRecord next;memcpy(&next,bad+second,sizeof(next));next.object=first.object;
    memcpy(bad+second,&next,sizeof(next));reject_unchanged(bytes);
    memcpy(bad,packed,bytes);memcpy(&first,bad+sizeof(NativeScriptItemSnapshotHeader),sizeof(first));first.tick+=777;
    memcpy(bad+sizeof(NativeScriptItemSnapshotHeader),&first,sizeof(first));
    native_items[1].native_script_owner++;reject_unchanged(bytes);native_items[1].native_script_owner--; // Last peer fails before changed first actor mutates.
    for(unsigned error=0;error<12;++error){memcpy(bad,packed,bytes);NativeScriptItemSnapshotRecord r;memcpy(&r,bad+sizeof(NativeScriptItemSnapshotHeader),sizeof(r));
        switch(error){case 0:r.program_identity++;break;case 1:r.layout_identity++;break;case 2:r.parameter_identity++;break;case 3:r.owner++;break;case 4:r.recipe_index=0;break;case 5:r.object.generation++;break;case 6:r.generation++;break;case 7:r.state_bytes++;break;case 8:r.faulted=2;break;case 9:r.phases[0]=accessory;break;case 10:r.callbacks[NativeScriptItemCallback_Touched]=accessory;break;case 11:r.article=NULL;break;}
        memcpy(bad+sizeof(NativeScriptItemSnapshotHeader),&r,sizeof(r));reject_unchanged(bytes);}
    memcpy(bad,packed,bytes);reject_unchanged(bytes-1);
    memcpy(bad,packed,bytes);memcpy(&first,bad+sizeof(NativeScriptItemSnapshotHeader),sizeof(first));first.creation_value=NAN;
    memcpy(bad+sizeof(NativeScriptItemSnapshotHeader),&first,sizeof(first));reject_unchanged(bytes);
    memcpy(bad,packed,bytes);memcpy(&next,bad+second,sizeof(next));next.creation_value=INFINITY;
    memcpy(bad+second,&next,sizeof(next));reject_unchanged(bytes);
    step(a);float endpoint=((Item*)a->user_data)->pos.x;CHECK(nativeScriptItem_Load(packed,bytes));step(a);CHECK(((Item*)a->user_data)->pos.x==endpoint);++cases;
    native_items[0].animated=shared_bool;CHECK(!nativeScriptItem_PhaseBound(a,NativeScriptItemPhase_Animation));native_items[0].animated=NULL;
    CHECK(!nativeScriptItem_PhaseBound(a,NativeScriptItemPhase_Animation));select_state(a,0);CHECK(nativeScriptItem_PhaseBound(a,NativeScriptItemPhase_Animation));++cases;
    Item native_saved=*(Item*)a->user_data;NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,a,&reference));
    HSD_GObj* saved_head=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM],*saved_tail=plinklow_gobjs[HSD_GOBJ_PLINK_ITEM];
    size_t object_bytes;static uint8_t object_snapshot[2*1024*1024];CHECK(nativeMatchSnapshotPool_Save(&nativeGObjPool,object_snapshot,sizeof(object_snapshot),&object_bytes));
    HSD_GObjFree(a);CHECK(!nativeScriptItem_IsAttached(a));HSD_GObj* reused=create(0,0);CHECK(reused==a);CHECK(!nativeScriptItem_Load(packed,bytes));
    CHECK(nativeMatchSnapshotPool_Load(&nativeGObjPool,object_snapshot,object_bytes));native_items[0]=native_saved;
    HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]=saved_head;plinklow_gobjs[HSD_GOBJ_PLINK_ITEM]=saved_tail;CHECK(nativeMatchPool_Resolve(reference)==a);
    CHECK(nativeScriptItem_Load(packed,bytes)&&nativeScriptItem_IsAttached(a));++cases;
    int32_t expected_destroy_count=(int32_t)state(a,"count").as.i64+101;
    HSD_GObj_SetupProc(a,step,1);fixture_remove=true;HSD_GObj_80390CFC();
    CHECK(delayed_status==NativeScriptVM_SourceRetired&&fixture_destroy_writes==1&&fixture_destroy_count==expected_destroy_count&&fixture_destroy_marked&&fixture_deferred_alive&&
        !nativeScriptItem_IsAttached(a)&&nativeMatchPool_Resolve(reference)==NULL&&nativeScriptRuntime_Idle());++cases;
    native_items[2].native_removal_in_progress=true;
    CHECK(nativeScriptItem_RunHook(apple,NativeScriptItemHook_Destroyed,NULL,0,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);
    CHECK(nativeScriptItem_IsAttached(apple)&&nativeScriptItem_Spec(apple)==&specs[2]&&!nativeScriptItem_Save(bad,sizeof(bad),&(size_t){0}));
    CHECK(nativeScriptItem_RunHook(apple,NativeScriptItemHook_WorkshopTick,NULL,0,&result)==NativeScriptVM_SourceRetired);++cases;
    HSD_GObjFree(b);HSD_GObjFree(apple);HSD_GObjFree(pear);CHECK(!nativeScriptItem_HasActors(Ft_Kind_Mario));
    a=create(0,0);CHECK(nativeScriptItem_Attach(a,&specs[0]));result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(nativeScriptItem_RunHook(a,NativeScriptItemHook_DamageReceived,NULL,0,&result)==NativeScriptVM_Error&&result.type==NativeScriptType_U64&&result.as.u64==99);
    CHECK(nativeScriptItem_IsAttached(a)&&nativeScriptItem_RunHook(a,NativeScriptItemHook_Spawned,NULL,0,&result)==NativeScriptVM_Error);
    creation=99;CHECK(!nativeScriptItem_GetCreationValue(a,&creation)&&creation==99&&!nativeScriptItem_SetCreationValue(a,2));++cases;
    CHECK(nativeScriptItem_Save(packed,sizeof(packed),&bytes));CHECK(nativeScriptItem_Load(packed,bytes));
    CHECK(nativeScriptItem_RunHook(a,NativeScriptItemHook_Spawned,NULL,0,&result)==NativeScriptVM_Error);HSD_GObjFree(a);++cases;
    for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i){HSD_GObj* object=create(i,0);CHECK(nativeScriptItem_Attach(object,&specs[0]));}
    HSD_GObj* extra=create(NATIVE_SCRIPT_ITEM_ACTORS,0);CHECK(!nativeScriptItem_Attach(extra,&specs[0]));
    CHECK(nativeScriptItem_Save(packed,sizeof(packed),&bytes));CHECK(((NativeScriptItemSnapshotHeader*)packed)->count==NATIVE_SCRIPT_ITEM_ACTORS);
    for(unsigned i=0;i<=NATIVE_SCRIPT_ITEM_ACTORS;++i)HSD_GObjFree(objects[i]);CHECK(!nativeScriptItem_ReferencesProgram(&bank));++cases;
    size_t empty;CHECK(nativeScriptItem_Save(bad,sizeof(bad),&empty)&&empty==sizeof(NativeScriptItemSnapshotHeader));
    CHECK(nativeScriptItem_Load(bad,empty));
    CHECK(!nativeScriptItem_Save(bad,sizeof(bad),(size_t*)bad));
    CHECK(!nativeScriptItem_Save(nativeScriptItemActors,sizeof(nativeScriptItemActors),&empty));++cases;
    CHECK(!nativeScriptItem_Save(nativeScriptItemActorByObject,sizeof(nativeScriptItemActorByObject),&empty));
    CHECK(nativeScriptItem_RetireScene()&&nativeGObjPool_RetireScene());CHECK(!nativeScriptItem_Load(packed,bytes));++cases;
    NativeRollbackStateRegion region;CHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1);
    CHECK((uintptr_t)nativeScriptItemActors>=(uintptr_t)region.memory&&(uintptr_t)nativeScriptItemActors+sizeof(nativeScriptItemActors)<=(uintptr_t)region.memory+region.size);
    CHECK((uintptr_t)nativeScriptItemActorByObject>=(uintptr_t)region.memory&&(uintptr_t)nativeScriptItemActorByObject+sizeof(nativeScriptItemActorByObject)<=(uintptr_t)region.memory+region.size);
    CHECK(!((uintptr_t)nativeScriptRuntime_VM()>=(uintptr_t)region.memory&&(uintptr_t)nativeScriptRuntime_VM()<(uintptr_t)region.memory+region.size));
    printf("{\"cases\":%u,\"checks\":%u,\"phase_ticks\":%u,\"full_actor_records_bytes\":%zu,\"actor_reservation\":%zu,\"native_frees\":%u,\"status\":\"PASS\"}\n",cases,checks,phase_ticks,bytes,sizeof(nativeScriptItemActors),fixture_native_frees);return 0;
}
