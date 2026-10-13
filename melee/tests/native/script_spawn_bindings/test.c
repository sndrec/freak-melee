#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_source_package.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/item.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include "boundary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""

unsigned spawn_stage,spawn_retire_stage,spawn_fail_stage,spawn_order[128],spawn_order_count;
HSD_GObj* spawn_caller;
const NativeScriptItemProgram* spawn_food_asset;
unsigned spawn_collision_flag;
SpawnItem positioned_spawn;
Vec3 projectile_offset;HSD_JObj* projectile_joint;int projectile_appearance;float projectile_variant;
static unsigned checks,cases;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"binding %u: %s (%s / %s)\n",__LINE__,#x,diagnostic.message,nativeScriptRuntime_VM()->diagnostic.message);exit(1);}}while(0)
enum { N=4096,F=64,V=128 };
static char source[65536];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[N*2];static uint32_t args[N*2];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[128];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction executions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[16];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptFighterProgram root;static NativeScriptItemState states[4][4];
static NativeScriptItemProjectile projectile;static NativeItemDefinition recipe;
static Article assets;static ItemAttr attributes;static HSD_Joint model_joint;static ItemModelDesc model={.x0_joint=&model_joint};
static ItemStateDesc descriptors[4];static ItemStateArray animations={descriptors,4};
static Fighter actor;static FighterBone bones[140];static HSD_JObj joint;static ftData data;
static uint8_t remap[54];static FighterPartsTable parts={.parts_num=140,.part_to_joint=remap};
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{(void)kind;return NULL;}
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){return kind==Ft_Kind_Mario?&root:NULL;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind kind,uint32_t owner)
{for(unsigned i=0;i<root.items.count;++i)if(kind==Ft_Kind_Mario&&root.items.programs[i].owner==owner)return &root.items.programs[i];return NULL;}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t i,NativeScriptDiagnostic* d)
{
    (void)unused;const NativeScriptSourceNode* n=&tree->nodes[i];
    if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 2");
    if(n->kind==NativeScriptSource_Item)return nativeScriptSource_SpanEquals(tree,n->value,"damage 4 angle 45 lifetime 90");
    if(n->kind==NativeScriptSource_ArticleState || n->kind==NativeScriptSource_State)return true;
    if(n->kind==NativeScriptSource_Callback){NativeScriptEventResolution e;return nativeScriptEvents_Resolve(tree,n,&e,d);}
    return nativeScriptCompile_MinimalMetadata(NULL,tree,i,d);
}
static float creation_value(Item_GObj* object){float value;CHECK(nativeScriptItem_GetCreationValue(object,&value));return value;}
static float item_value(Item_GObj* object,const char* name)
{
    NativeScriptReceiverView view;NativeScriptValue result;
    CHECK(nativeScriptItem_View(object,((Item*)object->user_data)->x1C,&view));
    for(unsigned i=0;i<bank.declarations.value_count;++i)
        if(values[i].owner==nativeScriptItem_Spec(object)->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name)){
            CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&result,&diagnostic));
            CHECK(result.type==NativeScriptType_F32);return result.as.f32;
        }
    CHECK(false);return 0;
}
// These stage map kernels are unselected in the ownership-property fixture;
// actual collision contracts are covered by the separate motion suite.
bool it_8026D9A0(Item_GObj* object){(void)object;abort();}
bool it_8026DA08(Item_GObj* object){(void)object;abort();}
static void prepare(const char* path)
{
    FILE* file=fopen(path,"rb");CHECK(file);char input[65536];size_t bytes=fread(input,1,sizeof(input)-1,file);fclose(file);input[bytes]=0;
    bank=(NativeScriptCompileBank){.source_storage=source,.source_capacity=sizeof(source),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N*2,.arguments=args,.argument_capacity=N*2},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=128,
        .instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=executions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=16},.parameter_slots=slots,.parameter_slot_capacity=V};
    NativeScriptCompileRequest request={.source=input,.source_bytes=(uint32_t)bytes,.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(accepted==&bank);
    assets=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animations};
    root=(NativeScriptFighterProgram){.bank=&bank,.identity=44};
    for(unsigned i=0;i<NativeScriptFighter_HookCount;++i)root.hooks[i]=UINT32_MAX;
    for(unsigned i=0;i<NativeScriptFighterCallback_Count;++i)root.callbacks[i]=UINT32_MAX;
    unsigned entity=0;
    for(unsigned i=0;i<bank.tree.count;++i)if(nodes[i].kind==NativeScriptSource_Article||nodes[i].kind==NativeScriptSource_Item) {
        CHECK(entity<4);NativeScriptItemProgram* p=&root.items.programs[entity];
        *p=(NativeScriptItemProgram){.bank=&bank,.identity=100+entity,.author_kind=Ft_Kind_Mario,.kind=entity==0?It_Kind_Mario_Cape:entity==2?It_Kind_NativeHoldable:It_Kind_Mario_Fire,
            .recipe_index=entity==2?0:UINT32_MAX,.owner=i,.article=&assets,.states=states[entity],.state_count=entity==2?4:2,.recipe=entity==2?&recipe:NULL};
        p->drawing=nodes[i].kind==NativeScriptSource_Item?NativeScriptItemDrawing_Billboard:NativeScriptItemDrawing_Standard;
        for(unsigned h=0;h<NativeScriptItemHook_Count;++h)p->hooks[h]=UINT32_MAX;
        for(unsigned s=0;s<4;++s){states[entity][s].animation_id=-1;for(unsigned phase=0;phase<3;++phase)states[entity][s].functions[phase]=UINT32_MAX;}++entity;
    }
    CHECK(entity==4);root.items.count=4;root.items.recipe_count=1;
    for(unsigned i=0;i<bank.declarations.function_count;++i)if(functions[i].callback) {
        NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&bank.tree,&nodes[functions[i].source_node],&e,&diagnostic));
        if(!functions[i].owner){root.hooks[!strcmp(e.descriptor->name,"load")?NativeScriptFighter_Load:NativeScriptFighter_Tick]=i;continue;}
        NativeScriptItemProgram* p=NULL;unsigned entity_at=0;
        for(unsigned j=0;j<4;++j)if(root.items.programs[j].owner==functions[i].owner){p=&root.items.programs[j];entity_at=j;}
        CHECK(p);
        if(e.descriptor->kind==NativeScriptEvent_ArticleState){CHECK(e.motion<4);states[entity_at][e.motion].functions[NativeScriptItemPhase_Animation]=i;}
        else {
            unsigned h=!strcmp(e.descriptor->name,"created")?NativeScriptItemHook_Created:!strcmp(e.descriptor->name,"picked_up")?NativeScriptItemHook_PickedUp:
                !strcmp(e.descriptor->name,"interaction")?NativeScriptItemHook_Interaction:!strcmp(e.descriptor->name,"destroyed")?NativeScriptItemHook_Destroyed:
                !strcmp(e.descriptor->name,"spawned")?NativeScriptItemHook_Spawned:
                !strcmp(e.descriptor->name,"launched")?NativeScriptItemHook_Launched:NativeScriptItemHook_WorkshopHit;
            p->hooks[h]=i;
        }
    }
    projectile=(NativeScriptItemProjectile){.appearance_state=0,.item_state=0,.speed={2,UINT32_MAX},.angle={0,UINT32_MAX},.lifetime={60,UINT32_MAX},.initial_velocity=true,.initial_update=true};
    root.items.programs[1].projectile=&projectile;root.items.programs[3].projectile=&projectile;scheduler_spec=&root.items.programs[0];spawn_food_asset=&root.items.programs[1];
}
static uint32_t field(const char* name)
{for(unsigned i=0;i<bank.declarations.value_count;++i)if(!values[i].owner && nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name))return NATIVE_SCRIPT_STATE_SEMANTIC|i;CHECK(false);return 0;}
static NativeScriptValue read(const char* name)
{NativeScriptReceiverView view;NativeScriptValue result;CHECK(nativeScriptFighter_View(spawn_caller,777,&view));CHECK(nativeScriptStorage_Read(view.storage,field(name),NULL,0,&result,&diagnostic));return result;}
static void set_action(int value)
{NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(spawn_caller,777,&view));NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=value};CHECK(nativeScriptStorage_Write(view.storage,field("action"),NULL,0,&v,&diagnostic));spawn_order_count=0;}
static bool run(void)
{return nativeScriptFighter_Run(spawn_caller,NativeScriptFighter_Tick,NULL,0, NULL,NULL)==NativeScriptVM_Ok;}
static HSD_GObj* find(int32_t id)
{for(HSD_GObj* o=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];o;o=o->next)if((int32_t)((Item*)o->user_data)->x1C==id)return o;return NULL;}
static HSD_GObj* by_kind(ItemKind kind)
{for(HSD_GObj* o=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];o;o=o->next)if(((Item*)o->user_data)->kind==kind)return o;return NULL;}
static void fighter_remove(void* ptr){Fighter* fp=ptr;nativeScriptFighter_Detach(fp->gobj);}
static void caller(void)
{
    if(spawn_caller){HSD_GObjFree(spawn_caller);spawn_caller=NULL;}
    actor=(Fighter){0};spawn_caller=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(spawn_caller);
    actor.gobj=spawn_caller;actor.kind=Ft_Kind_Mario;actor.x8_spawnNum=777;actor.facing_dir=-1;actor.cur_pos=(Vec3){4,5,6};actor.parts=bones;actor.ft_data=&data;
    joint.translate=(Vec3){10,11,12};bones[0].joint=&joint;tables[Ft_Kind_Mario]=&parts;for(unsigned i=0;i<54;++i)remap[i]=(uint8_t)i;
    static struct ftData_x8 info;data.x8=&info;data.x8->x10=0;
    GObj_InitUserData(spawn_caller,4,fighter_remove,&actor);CHECK(nativeScriptFighter_Attach(spawn_caller));spawn_order_count=spawn_retire_stage=0;
}
static bool deferred_seen;
static bool resolve(void* context,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,NativeScriptIRResolution* result,NativeScriptDiagnostic* d)
{int found=nativeScriptBindingSpawn_TryResolve(context,arena,node,result,d);return found?found>0:nativeScriptBindings_Resolve(&((NativeScriptBindingSpawnResolverContext*)context)->bindings,arena,node,result,d);}
static void reject(const char* input,const char* expected,bool default_value)
{
    NativeScriptSourceNode syntax[64];NativeScriptIRNode expressions[256];uint32_t arguments[256];
    NativeScriptSourceTree tree={input,(uint32_t)strlen(input),syntax,0,64};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={input,tree.source_bytes,expressions,0,256,arguments,0,256};
    NativeScriptFunctionDeclaration fn[4];NativeScriptParameterDeclaration param[4];NativeScriptValueDeclaration declarations[4];
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=fn,.function_capacity=4,.parameters=param,.parameter_capacity=4,.values=declarations,.value_capacity=4};
    NativeScriptBindingSpawnResolverContext scope={.program=&index,.bindings={NativeScriptBindings_Fighter}};
    if(default_value){index.constant_resolver=resolve;index.constant_context=&scope;CHECK(!nativeScriptProgram_Index(&index,&diagnostic));}
    else {
        CHECK(nativeScriptProgram_Index(&index,&diagnostic));NativeScriptLocal local[8];
        NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=resolve,.native_binding_context=&scope,.locals=local,.local_capacity=8};
        NativeScriptStatementCompiler body;CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    }
    CHECK(strstr(diagnostic.message,expected)!=NULL);++cases;
}
static void deferred(HSD_GObj* object)
{
    Item_8026A8EC(object);uint32_t generation;
    CHECK(nativeCallbackSource_Snapshot(object,NativeCallbackSource_Item,&generation));
    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    NativeScriptRuntimeContext c={.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,.generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
    CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemsHeld,NULL,0,&result,&diagnostic));CHECK(result.type==NativeScriptType_Bool&&!result.as.u64);deferred_seen=true;
}
// Explicit projectile tests use the same real compiler, factories and actor pools.
#include "projectile_cases.h"
int main(int argc,char** argv)
{
    CHECK(argc==2);prepare(argv[1]);HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();caller();
    reject("func Start(int kind):\n    articles.spawn_bone_attached(kind, 0)\narticle CustomBolt states 2:\n    state int unused\n","identifier constant",false);
    reject("func Start():\n    articles.spawn_bone_attached(130, 0)\narticle CustomBolt states 2:\n    state int unused\n","identifier constant",false);
    reject("func Start():\n    articles.spawn_bone_attached(position, 0)\narticle position states 2:\n    state int unused\n","conflicts with a native",false);
    reject("func Start():\n    articles.spawn_bone_attached(CustomBolt, 0)\nfunc CustomBolt():\n    pass\narticle CustomBolt states 2:\n    state int unused\n","conflicts with a root helper",false);
    reject("parameter int choice = CustomBolt\nfunc Start():\n    pass\narticle CustomBolt states 2:\n    state int unused\n","indexed body",true);
    reject("func Start():\n    item.create_child(CustomBolt, 0, 0, 0, 1, 0, 0, 0, 0)\narticle CustomBolt states 2:\n    state int unused\n","context is unsupported",false);
    reject("func Start():\n    item.create_at_position(CustomBolt, 0, 0, 0, 1)\narticle CustomBolt states 2:\n    state int unused\n","context is unsupported",false);
    reject("func Start():\n    articles.create_with_velocity(CustomBolt, 0, 0, 0, 0, 0, 0, 1, false, 0, 0)\narticle CustomBolt states 2:\n    state int unused\n","argument count",false);
    reject("func Start():\n    articles.create_at_position(CustomBolt, 0, 0, 0, 1, false, 1.5, 0)\narticle CustomBolt states 2:\n    state int unused\n","collision group requires an integral",false);
    reject("func Start():\n    articles.create_at_position(CustomBolt, 0, 0, 0, 1, false, 0)\narticle CustomBolt states 2:\n    state int unused\n","argument count",false);
    reject("func Start():\n    articles.create_at_position(CustomBolt, 0, 0, 0, 1, false, 0, 0, true)\narticle CustomBolt states 2:\n    state int unused\n","argument count",false);
    set_action(1);CHECK(run());int32_t id=(int32_t)read("cape").as.f32;HSD_GObj* cape=find(id);
    CHECK(cape && read("present").as.u64 && read("created").as.i64==1);CHECK(((Item*)cape->user_data)->msid==0);++cases;
    NativeScriptValue attachment_result;
    CHECK(nativeScriptItem_RunPhase(cape,NativeScriptItemPhase_Animation,&attachment_result)==NativeScriptVM_Ok&&attachment_result.type==NativeScriptType_Bool&&!attachment_result.as.u64);
    CHECK(nativeScriptItem_ClearAttachmentOwner(cape)&&nativeScriptItem_Creator(cape)==spawn_caller);
    CHECK(nativeScriptItem_RunPhase(cape,NativeScriptItemPhase_Animation,&attachment_result)==NativeScriptVM_Ok&&attachment_result.as.u64);
    CHECK(nativeScriptItem_SetAttachmentOwner(cape,spawn_caller));++cases;
    set_action(2);CHECK(run());HSD_GObj* fire=by_kind(It_Kind_Mario_Fire);CHECK(fire!=cape && ((Item*)fire->user_data)->kind==It_Kind_Mario_Fire);
    CHECK(((Item*)fire->user_data)->x40_vel.x==-2 && ((Item*)fire->user_data)->xD44_lifeTimer==60);Item_8026A8EC(fire);++cases;
    set_action(8);CHECK(run());HSD_GObj* custom=by_kind(It_Kind_Mario_Fire);CHECK(nativeScriptItem_Spec(custom)==&root.items.programs[3]);Item_8026A8EC(custom);++cases;
    set_action(9);CHECK(run());custom=find((int32_t)read("action").as.i64);
    CHECK(custom&&nativeScriptItem_Spec(custom)==&root.items.programs[3]);
    Item* placed=custom->user_data;
    CHECK(placed->pos.x==21&&placed->pos.y==22&&placed->pos.z==23&&placed->facing_dir==-1);
    CHECK(placed->x40_vel.x==0&&placed->x40_vel.y==0&&placed->x40_vel.z==0);
    CHECK(spawn_collision_flag==1&&placed->xAC4_ignoreItemID==0&&creation_value(custom)==0);
    CHECK(item_value(custom,"born")==0&&item_value(custom,"created_value")==0);
    Item_8026A8EC(custom);++cases;
    set_action(10);CHECK(run());custom=find((int32_t)read("action").as.i64);
    CHECK(custom&&nativeScriptItem_Spec(custom)==&root.items.programs[3]);placed=custom->user_data;
    CHECK(placed->x40_vel.x==4&&placed->x40_vel.y==5&&placed->x40_vel.z==6);
    CHECK((uint32_t)placed->xAC4_ignoreItemID==UINT32_MAX);
    CHECK(creation_value(custom)==7);
    CHECK(nativeItem_Creator(custom)==spawn_caller&&spawn_collision_flag==1);
    Item_8026A8EC(custom);++cases;
    for(unsigned action=12;action<=14;++action){
        set_action((int)action);CHECK(run());custom=find((int32_t)read("action").as.i64);
        CHECK(custom&&nativeScriptItem_Spec(custom)==&root.items.programs[3]);placed=custom->user_data;
        CHECK(positioned_spawn.pos.x==(action==14?actor.cur_pos.x:41)&&positioned_spawn.pos.y==(action==14?actor.cur_pos.y:42)&&positioned_spawn.pos.z==(action==14?actor.cur_pos.z:43));
        CHECK(positioned_spawn.prev_pos.x==41&&positioned_spawn.prev_pos.y==42&&positioned_spawn.prev_pos.z==43);
        CHECK(placed->pos.x==41&&placed->pos.y==42&&placed->pos.z==43);
        CHECK(placed->x40_vel.x==0&&placed->x40_vel.y==0&&placed->x40_vel.z==0&&spawn_collision_flag==1);
        CHECK((uint32_t)placed->xAC4_ignoreItemID==(action==12?UINT32_MAX:action==13?UINT32_C(0x80000000):0));
        float expected=action==12?7.25f:action==13?-3.5f:-0.0f;
        float observed=creation_value(custom),from_created=item_value(custom,"created_value");
        CHECK(!memcmp(&observed,&expected,sizeof(expected))&&!memcmp(&from_created,&expected,sizeof(expected)));
        CHECK(item_value(custom,"born")==0&&nativeScriptItem_Creator(custom)==spawn_caller);
        Item_8026A8EC(custom);++cases;
    }
    set_action(11);CHECK(run());custom=find((int32_t)read("action").as.i64);
    CHECK(custom&&nativeScriptItem_Spec(custom)==&root.items.programs[3]&&spawn_collision_flag==0);
    placed=custom->user_data;CHECK(placed->x40_vel.x==4&&creation_value(custom)==7);
    Item_8026A8EC(custom);++cases;
    set_action(4);CHECK(run());CHECK(!find(id) && read("cape").as.f32==-1 && !read("present").as.u64);actor.x1984_heldItemSpec=NULL;++cases;
    set_action(3);CHECK(run());HSD_GObj* snack=actor.item_gobj;CHECK(snack && read("pulled").as.u64 && read("held").as.u64);++cases;
    NativeScriptValue result;CHECK(nativeScriptItem_RunHook(snack,NativeScriptItemHook_WorkshopHit,NULL,0,&result)==NativeScriptVM_Ok && result.as.u64);
    HSD_GObj* food=by_kind(It_Kind_Foods);CHECK(food!=snack && ((Item*)food->user_data)->kind==It_Kind_Foods);Item_8026A8EC(food);++cases;
    HSD_GObj_SetupProc(snack,deferred,0);HSD_GObj_80390CFC();CHECK(deferred_seen && !nativeScriptItem_IsAttached(snack));actor.item_gobj=NULL;++cases;
    set_action(1);CHECK(run());id=(int32_t)read("cape").as.f32;cape=find(id);CHECK(cape);
    ((Item*)cape->user_data)->owner=NULL;set_action(5);CHECK(run());CHECK(!read("present").as.u64 && find(id)==cape);++cases;
    NativeScriptRuntimeContext native={.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,.generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
    uint32_t factory=NATIVE_SCRIPT_SPAWN_DOMAIN|(NativeScriptSpawn_WithVelocity<<20)|root.items.programs[3].owner;
    NativeScriptValue factory_args[12]={{.type=NativeScriptType_I32,.as.i64=root.items.programs[3].owner}};
    for(unsigned a=1;a<8;++a)factory_args[a]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};
    factory_args[8]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
    factory_args[9]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=0};
    factory_args[10]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};
    factory_args[11]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=2};
    NativeScriptValue bad_flag_result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    unsigned old_models=scheduler_models;
    CHECK(!nativeScriptBindingSpawn_Invoke(&native,factory,factory_args,12,&bad_flag_result,&diagnostic));
    CHECK(bad_flag_result.type==NativeScriptType_U64&&bad_flag_result.as.u64==UINT64_MAX&&scheduler_models==old_models);++cases;
    uint32_t extended=NATIVE_SCRIPT_SPAWN_DOMAIN|(NativeScriptSpawn_AtPositionData<<20)|root.items.programs[3].owner;
    const NativeScriptBindingDescriptor* extended_descriptor=nativeScriptBindings_Descriptor(extended);
    CHECK(extended_descriptor&&extended_descriptor->argument_count==8);
    NativeScriptValue extended_args[8];
    memcpy(extended_args,factory_args,sizeof(extended_args));
    extended_args[5]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=0};
    extended_args[6]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)};
    extended_args[7]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};
    CHECK(!nativeScriptBindingSpawn_Invoke(&native,extended,extended_args,8,&bad_flag_result,&diagnostic));
    CHECK(bad_flag_result.type==NativeScriptType_U64&&bad_flag_result.as.u64==UINT64_MAX&&scheduler_models==old_models);++cases;
    extended_args[6]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1.5f};
    CHECK(!nativeScriptBindingSpawn_Invoke(&native,extended,extended_args,8,&bad_flag_result,&diagnostic));
    CHECK(bad_flag_result.type==NativeScriptType_U64&&bad_flag_result.as.u64==UINT64_MAX&&scheduler_models==old_models);++cases;
    NativeScriptValue fractional={.type=NativeScriptType_F32,.as.f32=(float)id+0.5f},unchanged={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(!nativeScriptBindings_Invoke(&native,NativeScriptBinding_ArticlesOwned,NULL,0,&fractional,1,&unchanged,&diagnostic));CHECK(unchanged.type==NativeScriptType_U64&&unchanged.as.u64==UINT64_MAX);++cases;
    ((Item*)cape->user_data)->owner=spawn_caller;
    NativeScriptValue other;CHECK(nativeScriptReceiver_CaptureExternal(spawn_caller,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&other,&diagnostic));
    CHECK(nativeScriptItem_RunHook(cape,NativeScriptItemHook_Interaction,&other,1,&result)==NativeScriptVM_Ok);
    CHECK(((Item*)cape->user_data)->owner==NULL);scheduler_animation(cape);CHECK(!find(id));actor.x1984_heldItemSpec=NULL;++cases;
    set_action(7);CHECK(run());id=(int32_t)read("cape").as.f32;cape=find(id);CHECK(cape && ((Item*)cape->user_data)->msid==1);set_action(4);CHECK(run());actor.x1984_heldItemSpec=NULL;++cases;
    projectile_cases();
    set_action(1);spawn_retire_stage=6;CHECK(!run());CHECK(nativeScriptRuntime_Idle());++cases;
    printf("{\"status\":\"PASS source spawn bindings\",\"cases\":%u,\"checks\":%u,\"source_owner_calls\":true,\"native_pointer_values\":false}\n",cases,checks);return 0;
}
