#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/it/native_script_item_spawn.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_source_package.h>
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_item_owner.h>
#include <math.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include "boundary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum { StageConstruct=1,StageSpawned,StageState,StageAppearance,StageCreated,StageDebug,StageUpdate,StageLaunched,StagePickup,StageInventory };
unsigned spawn_stage,spawn_retire_stage,spawn_fail_stage,spawn_order[128],spawn_order_count;
HSD_GObj* spawn_caller;
const NativeScriptItemProgram* spawn_food_asset;
static unsigned checks,cases;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"spawn %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
enum { N=4096,F=64,V=128 };
static char source_bytes[65536];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[N*2];static uint32_t args[N*2];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[128];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction executions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[16];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptFighterProgram root;static NativeScriptItemProgram programs[2];static NativeScriptItemSignal signals[4];
static NativeScriptItemState states[2][4];
static Article assets;static ItemAttr attributes;static HSD_Joint model_joint;static ItemModelDesc model={.x0_joint=&model_joint};static ItemStateDesc descriptors[4];static ItemStateArray animation_array={descriptors,4};
static Fighter actor;static FighterBone bones[140];static HSD_JObj bone_joint;static ftData data;static FighterPartsTable part_table={.parts_num=140};static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=part_tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
// Stock-shaped rig and controlled article assets; no archive-backed source
// package is supplied by this constructor fixture.
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){(void)kind;return NULL;}
static NativeScriptRuntimeContext context;
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){return kind==Ft_Kind_Mario?&root:NULL;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner){for(unsigned i=0;i<2;++i)if(author==Ft_Kind_Mario&&programs[i].owner==owner)return &programs[i];return NULL;}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t n,NativeScriptDiagnostic* d)
{
 (void)unused;const NativeScriptSourceNode* node=&tree->nodes[n];
 if(node->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,node->value,"states 2");
 if(node->kind==NativeScriptSource_Item)return nativeScriptSource_SpanEquals(tree,node->value,"damage 4 angle 45 lifetime 90");
 if(node->kind==NativeScriptSource_ArticleState)return true;
 if(node->kind==NativeScriptSource_State||node->kind==NativeScriptSource_Function)return true;
 if(node->kind==NativeScriptSource_Callback){NativeScriptEventResolution e;return nativeScriptEvents_Resolve(tree,node,&e,d);}
 return nativeScriptCompile_MinimalMetadata(NULL,tree,n,d);
}
static uint32_t nativeHash(const char* text,unsigned count){uint32_t h=2166136261u;for(unsigned i=0;i<count;++i){h^=(unsigned char)text[i];h*=16777619u;}return h;}
static float creation_value(Item_GObj* object){float value;CHECK(nativeScriptItem_GetCreationValue(object,&value));return value;}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N*2,.arguments=args,.argument_capacity=N*2},
  .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=128,
  .instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=executions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
  .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=16},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* text="fighter:\n    behavior independent\nstate int caller_count\naction Caller:\n    on load:\n        state.caller_count = 1\n"
  "article Shot states 2:\n    state int public_linked\n    state int public_child\n    state int public_at\n    state int count\n    on spawned:\n            item.state.count = 1\n            position.x = 101\n"
  "    on created:\n            item.state.count = item.state.count + 3\n            position.x = 103\n"
  "    on launched:\n            item.state.count = item.state.count + 7\n            position.x = 107\n"
  "    on destroyed:\n            item.state.count = item.state.count + 100\n    on picked_up:\n            item.state.count = item.state.count + 5\n            position.x = 109\n"
  "    on signal public_link:\n            item.state.public_linked = item.create_linked(Child, 0, 1, 1.25, -0.0, 3.75, -1)\n    on signal public_spawn:\n            item.state.public_child = item.create_child(Child, 1.25, -0.0, 3.75, -1, uint(4294967280), uint(4294967295), 7.125, uint(3))\n            item.state.public_at = item.create_at_position(Child, 2.25, 3.5, -0.0, 1)\n    state 0 anim -1:\n    state 1 anim -1:\n"
  "article Child states 2:\n    state float spawned_value\n    state float created_value\n    state int count\n    on spawned:\n            item.state.spawned_value = item.creation_value\n            position.x = 101\n    on created:\n            item.state.created_value = item.creation_value\n            item.state.count = item.state.count + 5\n            position.x = 103\n    on destroyed:\n            item.state.count = item.state.count + 100\n    state 0 anim -1:\n    state 1 anim -1:\n";
 NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));assets=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animation_array};
 root=(NativeScriptFighterProgram){.bank=&bank,.identity=44};for(unsigned h=0;h<NativeScriptFighter_HookCount;++h)root.hooks[h]=UINT32_MAX;for(unsigned h=0;h<NativeScriptFighterCallback_Count;++h)root.callbacks[h]=UINT32_MAX;
 unsigned entity=0;for(unsigned n=0;n<bank.tree.count;++n)if(nodes[n].kind==NativeScriptSource_Article||nodes[n].kind==NativeScriptSource_Item){
  programs[entity]=(NativeScriptItemProgram){.bank=&bank,.identity=100+entity,.author_kind=Ft_Kind_Mario,.kind=It_Kind_Mario_Fire,.recipe_index=UINT32_MAX,.owner=n,.article=&assets,.states=states[entity],.state_count=2,.recipe=NULL};
  for(unsigned h=0;h<NativeScriptItemHook_Count;++h)programs[entity].hooks[h]=UINT32_MAX;
  for(unsigned s=0;s<4;++s){states[entity][s].animation_id=-1;for(unsigned p=0;p<3;++p)states[entity][s].functions[p]=UINT32_MAX;}++entity;
 }
 for(unsigned f=0;f<bank.declarations.function_count;++f)if(functions[f].callback){NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&bank.tree,&nodes[functions[f].source_node],&e,&diagnostic));
  if(!functions[f].owner){root.hooks[NativeScriptFighter_Load]=f;continue;}
  if(e.descriptor->kind==NativeScriptEvent_ArticleSignal){unsigned signal=programs[0].signal_count++;CHECK(signal<4);signals[signal]=(NativeScriptItemSignal){nativeHash(bank.tree.source+e.signal_name.offset,e.signal_name.length),f};programs[0].signals=signals;continue;}
  unsigned h=!strcmp(e.descriptor->name,"spawned")?NativeScriptItemHook_Spawned:!strcmp(e.descriptor->name,"created")?NativeScriptItemHook_Created:
    !strcmp(e.descriptor->name,"launched")?NativeScriptItemHook_Launched:!strcmp(e.descriptor->name,"destroyed")?NativeScriptItemHook_Destroyed:
    !strcmp(e.descriptor->name,"picked_up")?NativeScriptItemHook_PickedUp:NativeScriptItemHook_WorkshopTick;
  for(unsigned p=0;p<2;++p)if(programs[p].owner==functions[f].owner)programs[p].hooks[h]=f;
 }
 // Actor admission requires strictly increasing prepared signal IDs.
 for(unsigned i=1;i<programs[0].signal_count;++i) {
     NativeScriptItemSignal signal=signals[i];unsigned at=i;
     while(at&&signals[at-1].signal_id>signal.signal_id){signals[at]=signals[at-1];--at;}
     signals[at]=signal;
 }
 scheduler_spec=&programs[0];spawn_food_asset=&programs[0];
}
static void fighter_remove(void* data_ptr){Fighter* fp=data_ptr;nativeScriptFighter_Detach(fp->gobj);}
static void caller(void)
{
 if(spawn_caller){HSD_GObjFree(spawn_caller);spawn_caller=NULL;}
 memset(&actor,0,sizeof(actor));spawn_caller=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(spawn_caller);
 actor.gobj=spawn_caller;actor.kind=Ft_Kind_Mario;actor.x8_spawnNum=777;actor.facing_dir=-1;actor.cur_pos=(Vec3){4,5,6};actor.parts=bones;actor.ft_data=&data;
 bone_joint.translate=(Vec3){10,11,12};bones[0].joint=&bone_joint;part_tables[Ft_Kind_Mario]=&part_table;
 static struct ftData_x8 part_info;data.x8=&part_info;data.x8->x10=0;
 GObj_InitUserData(spawn_caller,4,fighter_remove,&actor);CHECK(nativeScriptFighter_Attach(spawn_caller));
 context=(NativeScriptRuntimeContext){.source=spawn_caller,.source_class=NativeCallbackSource_Fighter,.generation=777,.actor_id=777,.scene_epoch=nativeGObjPool.lifetime};
 spawn_order_count=spawn_retire_stage=0;
}
static void remove_item(Item_GObj* item){if(item&&nativeScriptItem_IsAttached(item))Item_8026A8EC(item);else if(item)HSD_GObjFree(item);}


bool positional_oom;SpawnItem positional_spawn,positional_initialized;
static HSD_GObj* parent_item,*original,*damage_owner,*attachment;
static Fighter foreign_data;
static unsigned mode,trigger,spawned_checks,created_checks,backlink_index;
unsigned child_lookup_count;
static bool child_call,attachment_cleared;static uint32_t linked_slot=UINT32_MAX;static float expected_creation=7.125f;
static uint32_t parent_id;
enum {DropParent=1,DropOriginal,ChangeDamageKind,FaultParent,RemoveChild,FaultChild,ClearParentAttachment};
static NativeScriptItemActor* item_actor(HSD_GObj* object)
{for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==object)return &nativeScriptItemActors[i];return NULL;}
void positional_stage(unsigned stage,HSD_GObj* object)
{
    spawn_stage=stage;CHECK(spawn_order_count<128);spawn_order[spawn_order_count++]=stage;
    if(child_call && (stage==StageSpawned||stage==StageCreated)){
        Item* item=object->user_data;CHECK(nativeScriptItem_Creator(object)==original);CHECK(item->owner==damage_owner);
        CHECK(nativeScriptItem_AttachmentOwner(object)==attachment);
        if(stage==StageSpawned){++spawned_checks;CHECK(creation_value(object)==0);CHECK(item->native_article_links[backlink_index]==UINT32_MAX);}
        else{++created_checks;CHECK(creation_value(object)==expected_creation);CHECK(item->native_article_links[backlink_index]==parent_id);if(linked_slot!=UINT32_MAX)CHECK(((Item*)parent_item->user_data)->native_article_links[linked_slot]==item->x1C);}
    }
    if(mode&&stage==trigger){unsigned operation=mode;mode=0;
        if(operation==DropParent){Item_8026A8EC(parent_item);parent_item=NULL;}
        else if(operation==DropOriginal)nativeScriptFighter_Detach(original);
        else if(operation==ChangeDamageKind)++foreign_data.kind;
        else if(operation==FaultParent){NativeScriptItemActor* a=item_actor(parent_item);CHECK(a);a->faulted=true;}
        else if(operation==RemoveChild)Item_8026A8EC(object);
        else if(operation==FaultChild){NativeScriptItemActor* a=item_actor(object);CHECK(a);a->faulted=true;}
        else if(operation==ClearParentAttachment){CHECK(nativeScriptItem_ClearAttachmentOwner(parent_item));attachment_cleared=true;}
        else abort();
    }
}
static void reset_records(void){spawn_order_count=spawned_checks=created_checks=child_lookup_count=0;mode=trigger=0;positional_oom=false;child_call=false;attachment_cleared=false;linked_slot=UINT32_MAX;expected_creation=7.125f;}
static HSD_GObj* find_item(uint32_t owner)
{for(HSD_GObj* o=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];o;o=o->next)if(((Item*)o->user_data)->native_script_owner==owner)return o;return NULL;}
static void foreign_remove(void* data){((Fighter*)data)->gobj=NULL;}
static HSD_GObj* foreign(uint32_t generation)
{
    HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
    memset(&foreign_data,0,sizeof(foreign_data));foreign_data.kind=Ft_Kind_Ness;foreign_data.gobj=object;foreign_data.x8_spawnNum=generation;
    GObj_InitUserData(object,4,foreign_remove,&foreign_data);return object;
}
static void parent(NativeScriptRuntimeContext* c)
{
    caller();reset_records();Vec3 point={4,5,6},velocity={0};parent_item=NULL;
    NativeScriptVMStatus status=nativeScriptItemSpawn_At(&context,programs[0].owner,&point,&velocity,-1,false,0,0,true,true,&parent_item,&diagnostic);
    if(status!=NativeScriptVM_Ok||!parent_item)fprintf(stderr,"parent status %u case %u stages %u caller status %u item %p\n",status,cases,spawn_order_count,nativeScriptFighter_Status(spawn_caller),(void*)parent_item);
    CHECK(status==NativeScriptVM_Ok&&parent_item);
    Item* data=parent_item->user_data;parent_id=(uint32_t)data->x1C;
    *c=(NativeScriptRuntimeContext){.source=parent_item,.source_class=NativeCallbackSource_Item,.generation=parent_id,.actor_id=parent_id,.scene_epoch=nativeGObjPool.lifetime};
    original=attachment=spawn_caller;damage_owner=spawn_caller;reset_records();
}
static NativeScriptVMStatus spawn(NativeScriptRuntimeContext* c,uint32_t damage,unsigned backlink,Item_GObj** out)
{
    Vec3 position={1.25f,-0.0f,3.75f};backlink_index=backlink;child_call=true;
    return nativeScriptItemSpawn_Child(c,programs[1].owner,&position,-1,damage,UINT32_MAX,7.125f,backlink,UINT32_MAX,out,&diagnostic);
}
static float observed(HSD_GObj* object,const char* name)
{
    NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,((Item*)object->user_data)->x1C,&view));
    for(unsigned i=0;i<bank.declarations.value_count;++i){const NativeScriptValueDeclaration* f=&bank.declarations.values[i];
        if(f->owner==programs[1].owner&&nativeScriptSource_SpanEquals(&bank.tree,f->name,name)){NativeScriptValue value;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic));return value.as.f32;}}
    abort();
}
static void inspect(HSD_GObj* child)
{
    Item* data=child->user_data;const Vec3 point={1.25f,-0.0f,3.75f},zero={0};
    CHECK(positional_spawn.x0_parent_gobj==damage_owner&&positional_spawn.x4_parent_gobj2==parent_item);
    CHECK(!memcmp(&positional_spawn.pos,&point,sizeof(point))&&!memcmp(&positional_spawn.prev_pos,&point,sizeof(point)));
    CHECK(!memcmp(&positional_spawn.vel,&zero,sizeof(zero))&&positional_spawn.x44_flag.b0);
    CHECK(positional_spawn.x48_ground_or_air==0&&positional_initialized.x48_ground_or_air==GA_Air);
    uint32_t group;memcpy(&group,&positional_spawn.x40,4);CHECK(group==UINT32_MAX);
    CHECK(data->msid==-1&&data->xDD4_itemVar.native_article.attachment_facing==-1);
    CHECK(!memcmp(&data->xDD4_itemVar.native_article.launch_origin,&point,sizeof(point)));
    CHECK(spawned_checks==1&&created_checks==1);CHECK(observed(child,"spawned_value")==0&&observed(child,"created_value")==7.125f);
}
static void snapshots(HSD_GObj* child)
{
    void* saved=malloc(8*1024*1024);CHECK(saved);size_t bytes=0;
    CHECK(nativeScriptItem_Save(saved,8*1024*1024,&bytes));CHECK(nativeScriptItem_ValidateSnapshot(saved,bytes));
    CHECK(nativeScriptItem_ClearAttachmentOwner(child));CHECK(!nativeScriptItem_AttachmentOwner(child)&&nativeScriptItem_Creator(child)==original);
    CHECK(nativeScriptItem_Load(saved,bytes));CHECK(nativeScriptItem_AttachmentOwner(child)==attachment&&nativeScriptItem_Creator(child)==original);
    CHECK(((Item*)child->user_data)->native_article_links[backlink_index]==parent_id);free(saved);++cases;
}
static void normal(void)
{
    for(unsigned b=0;b<4;++b){NativeScriptRuntimeContext c;parent(&c);damage_owner=NULL;Item_GObj* child=(void*)1;
        CHECK(spawn(&c,0,b,&child)==NativeScriptVM_Ok&&child);inspect(child);CHECK(!child_lookup_count);if(b==0)snapshots(child);remove_item(child);remove_item(parent_item);parent_item=NULL;++cases;}
    NativeScriptRuntimeContext c;parent(&c);HSD_GObj* other=foreign(UINT32_C(0xFFFFFFF0));damage_owner=other;Item_GObj* child=(void*)1;
    CHECK(spawn(&c,UINT32_C(0xFFFFFFF0),3,&child)==NativeScriptVM_Ok&&child);inspect(child);CHECK(child_lookup_count==1);snapshots(child);remove_item(child);
    CHECK(nativeScriptItem_ClearAttachmentOwner(parent_item));attachment=NULL;reset_records();child=(void*)1;
    CHECK(spawn(&c,UINT32_C(0xFFFFFFF0),2,&child)==NativeScriptVM_Ok&&child);inspect(child);CHECK(nativeScriptItem_Creator(child)==original&&!nativeScriptItem_AttachmentOwner(child));snapshots(child);remove_item(child);++cases;
    CHECK(nativeScriptItem_SetAttachmentOwner(parent_item,other));HSD_GObj* old=other;HSD_GObjFree(other);other=foreign(UINT32_C(0xFFFFFFF0));CHECK(old==other);
    // Old attachment slot/generation must stay unavailable despite replacement
    // sharing its address, userdata and native generation.
    CHECK(!nativeScriptItem_AttachmentOwner(parent_item));damage_owner=other;attachment=NULL;reset_records();child=(void*)1;
    CHECK(spawn(&c,UINT32_C(0xFFFFFFF0),1,&child)==NativeScriptVM_Ok&&child);CHECK(!nativeScriptItem_AttachmentOwner(child));remove_item(child);++cases;
    CHECK(nativeScriptItem_SetAttachmentOwner(parent_item,other));attachment=other;reset_records();child=(void*)1;
    CHECK(spawn(&c,UINT32_C(0xFFFFFFF0),0,&child)==NativeScriptVM_Ok&&child);CHECK(nativeScriptItem_AttachmentOwner(child)==other);remove_item(child);++cases;
    CHECK(nativeScriptItem_SetAttachmentOwner(parent_item,original));attachment=original;reset_records();mode=ClearParentAttachment;trigger=StageSpawned;child=(void*)1;
    CHECK(spawn(&c,UINT32_C(0xFFFFFFF0),0,&child)==NativeScriptVM_Ok&&child&&!nativeScriptItem_AttachmentOwner(parent_item)&&nativeScriptItem_AttachmentOwner(child)==original);remove_item(child);++cases;
    remove_item(parent_item);parent_item=NULL;HSD_GObjFree(other);
}
static void failures(void)
{
    NativeScriptRuntimeContext c;parent(&c);damage_owner=NULL;Item_GObj* child=(void*)1;positional_oom=true;
    CHECK(spawn(&c,0,0,&child)==NativeScriptVM_Ok&&!child);positional_oom=false;++cases;
    reset_records();child=(void*)1;CHECK(spawn(&c,UINT32_MAX,0,&child)==NativeScriptVM_Error&&child==(void*)1&&!spawn_order_count);++cases;
    CHECK(spawn(&c,0,4,&child)==NativeScriptVM_Error&&child==(void*)1&&!spawn_order_count);++cases;
    CHECK(spawn(&context,0,0,&child)==NativeScriptVM_Error&&child==(void*)1&&!spawn_order_count);++cases;
    // The actual native constructor cleans Spawned retirement/fault and only
    // returns NULL; a healthy caller receives normal absence without guessing
    // a hidden constructor outcome from the pointer.
    for(unsigned operation=RemoveChild;operation<=FaultChild;++operation){reset_records();child=(void*)1;mode=operation;trigger=StageSpawned;
        if(operation==FaultChild){mode=0;scheduler_fail_write=true;}
        unsigned freed=scheduler_frees;CHECK(spawn(&c,0,0,&child)==NativeScriptVM_Ok&&!child&&scheduler_frees==freed+1&&!created_checks);scheduler_fail_write=false;++cases;}
    reset_records();child=(void*)1;static NativeScriptCompileBank other_bank;const NativeScriptCompileBank* old=programs[1].bank;programs[1].bank=&other_bank;
    CHECK(spawn(&c,0,0,&child)==NativeScriptVM_Error&&!spawn_order_count);programs[1].bank=old;++cases;
    for(unsigned operation=DropParent;operation<=FaultChild;++operation){if(parent_item)remove_item(parent_item);parent_item=NULL;parent(&c);
        HSD_GObj* other=foreign(999);damage_owner=other;reset_records();child=(void*)1;mode=operation;trigger=StageCreated;
        NativeScriptVMStatus result=spawn(&c,999,0,&child);CHECK(result!=NativeScriptVM_Ok&&child==(void*)1);
        HSD_GObj* survivor=find_item(programs[1].owner);if(operation==RemoveChild)CHECK(!survivor);else CHECK(survivor);
        if(survivor)remove_item(survivor);if(parent_item)remove_item(parent_item);parent_item=NULL;HSD_GObjFree(other);++cases;}
    parent(&c);NativeScriptItemActor* a=item_actor(parent_item);CHECK(a);a->faulted=true;reset_records();child=(void*)1;
    CHECK(spawn(&c,0,0,&child)!=NativeScriptVM_Ok&&!spawn_order_count&&child==(void*)1);remove_item(parent_item);parent_item=NULL;++cases;
}
static uint32_t public_signal(const char* name)
{
    const NativeScriptItemProgram* p=nativeScriptItem_Spec(parent_item);
    for(unsigned i=0;i<p->signal_count;++i) {
        const NativeScriptFunctionDeclaration* f=&bank.declarations.functions[p->signals[i].function];
        NativeScriptEventResolution event;CHECK(nativeScriptEvents_Resolve(&bank.tree,&bank.tree.nodes[f->source_node],&event,&diagnostic));
        if(nativeScriptSource_SpanEquals(&bank.tree,event.signal_name,name))return p->signals[i].signal_id;
    }
    CHECK(false);return UINT32_MAX;
}
static void public_source(void)
{
    NativeScriptRuntimeContext c;parent(&c);HSD_GObj* other=foreign(UINT32_C(0xFFFFFFF0));damage_owner=other;reset_records();
    child_call=false;NativeScriptValue result;CHECK(nativeScriptItem_RunSignal(parent_item,public_signal("public_spawn"),0,&result)==NativeScriptVM_Ok);
    unsigned count=0;for(HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];object;object=object->next){Item* item=object->user_data;
        if(item->native_script_owner!=programs[1].owner)continue;++count;
        if(item->native_article_links[3]==parent_id){CHECK(item->owner==other&&creation_value(object)==7.125f);}
        else CHECK(item->owner==spawn_caller&&creation_value(object)==0);
    }
    CHECK(count==2&&child_lookup_count==1);HSD_GObj* object;while((object=find_item(programs[1].owner)))remove_item(object);
    remove_item(parent_item);parent_item=NULL;HSD_GObjFree(other);++cases;
}
#include "linked_cases.h"
int main(void)
{
    prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();
    normal();failures();public_source();linked_cases();NativeScriptRuntimeContext c;parent(&c);damage_owner=NULL;
    for(unsigned i=0;i<1024;++i){reset_records();Item_GObj* out=(void*)1;CHECK(spawn(&c,0,0,&out)==NativeScriptVM_Ok&&out);NativeMatchPoolReference r;CHECK(nativeMatchPool_Reference(&nativeGObjPool,out,&r));remove_item(out);CHECK(!nativeMatchPool_Resolve(r));}
    ++cases;remove_item(parent_item);parent_item=NULL;printf("{\"status\":\"PASS source child Article\",\"checks\":%u,\"cases\":%u,\"native_frees\":%u,\"reuse_cycles\":1024,\"public_source_factories\":3}\n",checks,cases,scheduler_frees);return 0;
}
