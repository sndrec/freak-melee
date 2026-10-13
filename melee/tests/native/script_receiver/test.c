#include "engine_fixture.h"
#include "../script_kinetics/engine_fixture.h"
#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/ef/types.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {N=2048,F=64,V=128,I=8192};
static char source[32768],bank_source[32768];static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode ir[I];static uint32_t args[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[V];
static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[16];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank* bank;static const NativeScriptCompileBank* accepted;
static NativeScriptDiagnostic diagnostic;static unsigned checks,runs,effects,mode;
static HSD_GObj* objects[3];static Fighter fighters[2];static Item item;
static NativeScriptStorageInstance storage[3];static uint8_t state[3][128];static NativeScriptExecution execution[3];
static NativeScriptParameterSlot article_parameter_slots[V];
static NativeScriptStorageParameters article_parameters;

NativeMatchPool nativeGObjPool,nativeGObjProcPool;
static _Alignas(16) uint8_t gobj_bytes[4096];static NativeMatchPoolSlot gobj_slots[16];static uint32_t free_order[16];
HSD_GObj** HSD_GObjPLinkHead;static HSD_GObj* heads[32];
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"line %u: %s (%s; %s)\n",__LINE__,#x,diagnostic.message,nativeScriptRuntime_VM()->diagnostic.message);exit(1);}}while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
// This core fixture supplies controlled storage views rather than Item actors.
Fighter_GObj* nativeScriptItem_Creator(Item_GObj* object){return nativeItem_Creator(object);}
Fighter_GObj* nativeScriptItem_AttachmentOwner(Item_GObj* object){return nativeItem_Creator(object);}
Fighter_GObj* nativeModelEffect_Creator(EF_Effect* e){(void)e;return NULL;}

static bool metadata(void* user,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
    (void)user;const NativeScriptSourceNode* node=&tree->nodes[at];
    unsigned parent=node->parent<tree->count?tree->nodes[node->parent].kind:0;
    if(node->kind==NativeScriptSource_Article)return node->parent==0 && nativeScriptSource_SpanEquals(tree,node->name,"It_Kind_Mario_Cape") &&
        nativeScriptSource_SpanEquals(tree,node->value,"states 1");
    if(parent==NativeScriptSource_Article && (node->kind==NativeScriptSource_State||node->kind==NativeScriptSource_Function))return true;
    if(node->kind==NativeScriptSource_ArticleState)return parent==NativeScriptSource_Article && nativeScriptSource_SpanEquals(tree,node->value,"0 anim 0");
    if(node->kind==NativeScriptSource_Callback && parent==NativeScriptSource_ArticleState) {
        NativeScriptEventResolution event;return nativeScriptEvents_Resolve(tree,node,&event,d);
    }
    return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);
}
static void build(const char* path)
{
    FILE* file=fopen(path,"rb");CHECK(file!=NULL);size_t count=fread(source,1,sizeof(source)-1,file);CHECK(feof(file));fclose(file);
    *bank=(NativeScriptCompileBank){.source_storage=bank_source,.source_capacity=sizeof(bank_source),
        .tree={.nodes=syntax,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=args,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},
        .locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=code,.function_code_capacity=F,
        .execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=16},.parameter_slots=slots,.parameter_slot_capacity=V};
    NativeScriptMoveLayout motions={.common_count=341,.first=341,.count=10};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)count,.metadata=metadata,.motion_layout=&motions};
    if(!nativeScriptCompile_Build(bank,&request,&accepted,&diagnostic)){fprintf(stderr,"build %u: %s\n",diagnostic.source.line,diagnostic.message);exit(1);}
    CHECK(accepted==bank && bank->valid);CHECK(bank->maximum_call_depth>=2);
}
static uint32_t function(const char* name)
{
    for(uint32_t i=0;i<bank->declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank->tree,functions[i].name,name))return i;
    CHECK(false);return UINT32_MAX;
}
static bool lookup(void* user,void* actor,uint32_t source_class,uint32_t generation,NativeScriptReceiverView* output)
{
    (void)user;
    for(unsigned i=0;i<3;++i)if(objects[i]==actor && source_class==(i==2?NativeCallbackSource_Item:NativeCallbackSource_Fighter) &&
        storage[i].live && storage[i].generation==generation) {
        *output=(NativeScriptReceiverView){storage[i].layout->program,&storage[i]};return true;
    }
    return false;
}
static bool helper(void* user,void* actor,uint32_t generation,const NativeScriptProgramIndex* expected,uint32_t id,
    const NativeScriptValue* inputs,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    (void)user;CHECK(expected==&bank->declarations);
    for(unsigned i=0;i<2;++i)if(actor==objects[i]) {
        NativeScriptVMStatus status=nativeScriptExecution_Run(&execution[i],actor,NativeCallbackSource_Fighter,generation,7,19,id,inputs,count, NULL,output);
        if(status!=NativeScriptVM_Ok)*d=nativeScriptRuntime_VM()->diagnostic;return status==NativeScriptVM_Ok;
    }
    return false;
}
static void unlink_actor(HSD_GObj* object)
{
    HSD_GObj** cursor=&heads[HSD_GOBJ_PLINK_FIGHTER];while(*cursor && *cursor!=object)cursor=&(*cursor)->next;
    if(*cursor)*cursor=object->next;object->user_data=(void*)1;
}
static void changed(HSD_GObj* actor)
{
    ++effects;
    if(mode==1)item.owner=objects[1];
    if(mode==2)unlink_actor(actor);
    if(mode==3)++((Fighter*)actor->user_data)->x8_spawnNum;
}
static void world(void)
{
    CHECK(nativeScriptRuntime_Idle());scriptKineticsFixture_Reset();
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,gobj_bytes,sizeof(gobj_bytes),gobj_slots,free_order,16,sizeof(HSD_GObj),16,19));
    memset(fighters,0,sizeof(fighters));memset(&item,0,sizeof(item));memset(heads,0,sizeof(heads));HSD_GObjPLinkHead=heads;
    for(unsigned i=0;i<3;++i){NativeMatchPoolReference reference;objects[i]=nativeMatchPool_Allocate(&nativeGObjPool,&reference);CHECK(objects[i]!=NULL);}
    for(unsigned i=0;i<2;++i) {
        objects[i]->user_data=&fighters[i];fighters[i].gobj=objects[i];fighters[i].kind=Ft_Kind_CustomFirst;fighters[i].behavior_kind=Ft_Kind_CustomFirst;
        fighters[i].x8_spawnNum=101+i;fighters[i].cur_pos.x=1+i;fighters[i].cur_pos.y=0;fighters[i].motion_id=343+i;
        fighters[i].cmd_vars[3]=4;fighters[i].coll_data.floor.normal.x=0.25f;
    }
    objects[0]->next=objects[1];heads[HSD_GOBJ_PLINK_FIGHTER]=objects[0];heads[HSD_GOBJ_PLINK_ITEM]=objects[2];
    scriptKineticsFixture_OwnSource(&fighters[0],bank,&storage[0]);
    scriptKineticsFixture_AddSource(&fighters[1],&storage[1]);
    objects[2]->user_data=&item;item.entity=objects[2];item.x1C=201;item.kind=It_Kind_Mario_Cape;item.native_article_author_kind=Ft_Kind_CustomFirst;
    item.owner=objects[0];item.xDD4_itemVar.native_article.attachment_owner=objects[0];item.xDD4_itemVar.native_article.creator_spawn_id=101;
    fighters[0].item_gobj=objects[2];nativeItem_CaptureCreator(objects[2],objects[0]);
    // Capture creator after selecting the controlled immutable source owner.
    nativeItem_CaptureCreator(objects[2],objects[0]);
    uint32_t article_owner=functions[function("Assign")].owner;
    item.native_script_owner=article_owner;
    item.xDD4_itemVar.native_article.attachment_owner=NULL;item.xDD4_itemVar.native_article.creator_spawn_id=0;
    NativeScriptParameterSupply gain={.semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC|2,.owner=0,
        .value={.type=NativeScriptType_F32,.as.f32=11}};
    CHECK(nativeScriptStorage_PrepareParameters(&bank->storage_layout,article_parameter_slots,V,
        &gain,1,&article_parameters,&diagnostic));
    for(unsigned i=0;i<3;++i) {
        // The two Fighters are instances of one package: authored attributes
        // belong to that package's parameter prototype. The article has its own
        // parameter supply (11), so receiver and program reads remain distinct.
        const NativeScriptStorageParameters* parameters=i==2?&article_parameters:&bank->storage_parameters;
        CHECK(nativeScriptStorage_Create(&bank->storage_layout,parameters,i==2?article_owner:0,i==2?201:101+i,i==2?201:101+i,
            state[i],128,&storage[i],&diagnostic));
        execution[i]=(NativeScriptExecution){.declarations=&bank->declarations,.code=&bank->code,.storage=&storage[i],
            .vm=nativeScriptRuntime_VM(),.payloads=nativeScriptRuntime_Payloads(),.functions=entries,.function_count=bank->declarations.function_count,
            .receiver_lookup=lookup,.receiver_helper=helper,.fighter_kind=Ft_Kind_CustomFirst};
        CHECK(nativeScriptExecution_Bind(&execution[i],&diagnostic));
    }
    effects=mode=0;script_receiver_change_hook=changed;
}
static NativeScriptVMStatus run(const char* name,NativeScriptValue* result)
{++runs;return nativeScriptExecution_Run(&execution[2],objects[2],NativeCallbackSource_Item,201,7,19,function(name),NULL,0, NULL,result);}
static float read_state(unsigned actor,uint32_t index,uint32_t element,bool array)
{
    NativeScriptValue selector={.type=NativeScriptType_I32,.as.i64=element},value;
    CHECK(nativeScriptStorage_Read(&storage[actor],NATIVE_SCRIPT_STATE_SEMANTIC|index,array?&selector:NULL,array?1:0,&value,&diagnostic));return value.as.f32;
}
static void source_cases(void)
{
    NativeScriptValue result;
    world();mode=1;CHECK(run("Assign",&result)==NativeScriptVM_Ok);CHECK(result.type==NativeScriptType_F32 && result.as.f32==12);
    CHECK(fighters[0].cur_pos.x==12 && fighters[1].cur_pos.x==2 && item.owner==objects[1] && effects==1);CHECK(nativeScriptRuntime_Idle());
    world();CHECK(run("Owned",&result)==NativeScriptVM_Ok);CHECK(result.as.f32==5 && read_state(0,0,0,false)==5 && read_state(1,0,0,false)==0);
    world();CHECK(run("Inputs",&result)==NativeScriptVM_Ok);CHECK(result.as.f32==349.25f);
    world();CHECK(run("Missing",&result)==NativeScriptVM_Ok && result.as.u64==1);fighters[0].item_gobj=NULL;
    CHECK(run("Missing",&result)==NativeScriptVM_Ok && result.as.u64==0);
    // A readable parameter view is not permission to replace a package's
    // Fighter prototype for native operations. Reject before installing a
    // motion or publishing the helper/assignment result.
    world();storage[0].parameters=&article_parameters;
    CHECK(nativeScriptExecution_Bind(&execution[0],&diagnostic));
    result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(run("Assign",&result)==NativeScriptVM_Error);
    CHECK(effects==0 && fighters[0].cur_pos.x==1 && fighters[0].motion_id==343);
    CHECK(result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX && nativeScriptRuntime_Idle());
    world();mode=1;CHECK(run("Indexed",&result)==NativeScriptVM_Ok);CHECK(result.as.f32==12 && effects==2 && read_state(0,1,0,true)==12);
    for(unsigned failure=2;failure<=3;++failure) {
        world();mode=failure;result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(run("Indexed",&result)==NativeScriptVM_Error);CHECK(effects==1);CHECK(read_state(0,1,0,true)==0);
        CHECK(result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX);CHECK(nativeScriptRuntime_Idle());
    }
    world();item.owner=objects[1];result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(run("Owned",&result)==NativeScriptVM_Error);CHECK(effects==0 && read_state(0,0,0,false)==0);
    CHECK(result.type==NativeScriptType_U64 && nativeScriptRuntime_Idle());
}
static void parameter_cases(void)
{
    NativeScriptValue result;
    for(unsigned reflected=0;reflected<2;++reflected) {
        world();if(reflected)item.owner=objects[1];
        CHECK(run("CreatorParameter",&result)==NativeScriptVM_Ok);
        CHECK(result.type==NativeScriptType_F32 && result.as.f32==2);
        CHECK(run("DamageOwnerParameter",&result)==NativeScriptVM_Ok);
        CHECK(result.type==NativeScriptType_F32 && result.as.f32==2);
        CHECK(run("ProgramParameter",&result)==NativeScriptVM_Ok);
        CHECK(result.type==NativeScriptType_F32 && result.as.f32==11);
        result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        CHECK(run("AttachedParameter",&result)==(reflected?NativeScriptVM_Error:NativeScriptVM_Ok));
        if(reflected)CHECK(result.type==NativeScriptType_U64 && result.as.u64==UINT64_MAX);
        else CHECK(result.type==NativeScriptType_F32 && result.as.f32==2);
        CHECK(effects==0 && nativeScriptRuntime_Idle());
    }
}
static NativeScriptReceiverScope scope(NativeScriptRuntimeContext* context)
{
    NativeScriptReceiverScope selected={.pool=nativeScriptRuntime_Receivers()};
    CHECK(nativeScriptBindings_Context(objects[2],NativeCallbackSource_Item,201,context,&diagnostic));context->frame=7;context->scene_epoch=19;
    CHECK(nativeScriptReceiver_Push(selected.pool,context,19,&bank->declarations,Ft_Kind_CustomFirst,lookup,helper,NULL,&selected.frame,&diagnostic));return selected;
}
static void retained_cases(void)
{
    world();NativeScriptRuntimeContext caller;NativeScriptReceiverScope outer=scope(&caller);NativeScriptValue creator,owner,current,holder;
    CHECK(nativeScriptReceiver_Capture(&outer,&caller,NativeScriptReceiver_Creator,true,true,&creator,&diagnostic));
    CHECK(nativeScriptReceiver_Capture(&outer,&caller,NativeScriptReceiver_Owner,true,true,&owner,&diagnostic));
    CHECK(nativeScriptReceiver_Capture(&outer,&caller,NativeScriptReceiver_CurrentOwner,true,false,&current,&diagnostic));
    CHECK(nativeScriptReceiver_Capture(&outer,&caller,NativeScriptReceiver_Holder,true,false,&holder,&diagnostic));
    CHECK(outer.pool->top==1);item.owner=objects[1];
    const NativeScriptReceiverRecord* record;CHECK(nativeScriptReceiver_Validate(&outer,&caller,&owner,true,&record,&diagnostic)&&record->actor==objects[0]);
    uint64_t parameter_identity=bank->storage_parameters.identity;bank->storage_parameters.identity^=1;
    NativeScriptValue preserved={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(!nativeScriptBindingReceiver_Read(&outer,&caller,NATIVE_SCRIPT_RECEIVER_PARAMETER|2,&owner,1,&preserved,&diagnostic));
    CHECK(preserved.type==NativeScriptType_U64 && preserved.as.u64==UINT64_MAX);
    bank->storage_parameters.identity=parameter_identity;
    CHECK(nativeScriptBindingReceiver_Read(&outer,&caller,NATIVE_SCRIPT_RECEIVER_PARAMETER|2,&owner,1,&preserved,&diagnostic));
    CHECK(preserved.type==NativeScriptType_F32 && preserved.as.f32==2);
    NativeScriptValue next;CHECK(nativeScriptReceiver_Capture(&outer,&caller,NativeScriptReceiver_Owner,false,false,&next,&diagnostic));
    bool exists;CHECK(nativeScriptReceiver_Exists(&outer,&caller,&next,&exists,&diagnostic)&&!exists);
    CHECK(nativeScriptReceiver_Capture(&outer,&caller,NativeScriptReceiver_CurrentOwner,true,false,&next,&diagnostic));
    CHECK(nativeScriptReceiver_Validate(&outer,&caller,&next,false,&record,&diagnostic)&&record->actor==objects[1]);
    NativeScriptProgramIndex alternative_program=bank->declarations;NativeScriptStorageLayout alternative_layout=bank->storage_layout;
    alternative_layout.program=&alternative_program;const NativeScriptStorageLayout* previous=storage[1].layout;storage[1].layout=&alternative_layout;
    CHECK(!nativeScriptReceiver_Validate(&outer,&caller,&next,false,&record,&diagnostic));storage[1].layout=previous;
    CHECK(nativeScriptReceiver_Validate(&outer,&caller,&next,false,&record,&diagnostic));
    fighters[1].kind=Ft_Kind_DrMario;
    CHECK(!nativeScriptReceiver_Validate(&outer,&caller,&next,true,&record,&diagnostic));
    fighters[1].kind=Ft_Kind_CustomFirst;
    NativeScriptReceiverScope inner=scope(&caller);CHECK(!nativeScriptReceiver_Validate(&outer,&caller,&creator,false,&record,&diagnostic));
    CHECK(nativeScriptReceiver_Pop(inner.pool,inner.frame,&diagnostic));CHECK(nativeScriptReceiver_Validate(&outer,&caller,&creator,true,&record,&diagnostic));
    NativeScriptValue external,imported,exported;CHECK(nativeScriptReceiver_CaptureExternal(objects[0],NativeCallbackSource_Fighter,19,&external,&diagnostic));
    CHECK(nativeScriptReceiver_Import(&outer,&caller,&external,19,&imported,&diagnostic));CHECK(nativeScriptReceiver_Export(&outer,&caller,&imported,&exported,&diagnostic));
    CHECK(!memcmp(&external.as.object,&exported.as.object,sizeof(external.as.object)));
    void* native=NULL;uint32_t generation=0;CHECK(nativeScriptReceiver_ResolveExternal(&caller,&exported,19,NativeCallbackSource_Fighter,&native,&generation,&diagnostic));
    CHECK(native==objects[0] && generation==101);
    CHECK(!nativeScriptReceiver_Import(&outer,&caller,&external,18,&imported,&diagnostic));
    CHECK(!nativeScriptReceiver_ResolveExternal(&caller,&exported,19,NativeCallbackSource_Item,&native,&generation,&diagnostic));
    NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,objects[0],&reference));
    CHECK(nativeMatchPool_Free(&nativeGObjPool,reference));CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&reference)==objects[0]);
    // Same GObj address and game generation, different actual pool slot generation.
    CHECK(!nativeScriptReceiver_Validate(&outer,&caller,&creator,false,&record,&diagnostic));
    CHECK(!nativeScriptReceiver_ResolveExternal(&caller,&exported,19,NativeCallbackSource_Fighter,&native,&generation,&diagnostic));
    CHECK(nativeScriptReceiver_Pop(outer.pool,outer.frame,&diagnostic));CHECK(nativeScriptRuntime_Idle());
}
static void bounds_and_schema(void)
{
    world();NativeScriptRuntimeContext caller;NativeScriptReceiverScope selected=scope(&caller);NativeScriptValue receiver;
    CHECK(nativeScriptReceiver_Capture(&selected,&caller,NativeScriptReceiver_Creator,true,true,&receiver,&diagnostic));
    NativeScriptValue selectors[]={receiver,{.type=NativeScriptType_I32,.as.i64=0}};
    CHECK(nativeScriptBindingReceiver_ValidateTarget(&selected,&caller,NATIVE_SCRIPT_RECEIVER_STATE|1,selectors,2,&diagnostic));
    selectors[1].as.i64=-1;CHECK(!nativeScriptBindingReceiver_ValidateTarget(&selected,&caller,NATIVE_SCRIPT_RECEIVER_STATE|1,selectors,2,&diagnostic));
    selectors[1].as.i64=4;CHECK(!nativeScriptBindingReceiver_ValidateTarget(&selected,&caller,NATIVE_SCRIPT_RECEIVER_STATE|1,selectors,2,&diagnostic));
    CHECK(!nativeScriptBindingReceiver_ValidateTarget(&selected,&caller,NATIVE_SCRIPT_RECEIVER_PARAMETER|2,selectors,1,&diagnostic));
    // Current motion identity is writable; bounded native admission happens in
    // its backend. Animation IDs remain observations, including recipients.
    CHECK(nativeScriptBindingReceiver_ValidateTarget(&selected,&caller,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_MotionID,selectors,1,&diagnostic));
    CHECK(!nativeScriptBindingReceiver_ValidateTarget(&selected,&caller,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_AnimationID,selectors,1,&diagnostic));
    NativeScriptValue value={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},before=value;
    CHECK(!nativeScriptBindingReceiver_Read(&selected,&caller,NATIVE_SCRIPT_RECEIVER_STATE|127,selectors,1,&value,&diagnostic));
    CHECK(!memcmp(&before,&value,sizeof(value)));
    uint64_t old_identity=bank->storage_layout.identity;bank->storage_layout.identity^=1;
    const NativeScriptReceiverRecord* record;CHECK(!nativeScriptReceiver_Validate(&selected,&caller,&receiver,true,&record,&diagnostic));bank->storage_layout.identity=old_identity;
    CHECK(nativeScriptReceiver_Validate(&selected,&caller,&receiver,true,&record,&diagnostic));
    CHECK(nativeScriptBindingReceiver_Read(&selected,&caller,NATIVE_SCRIPT_RECEIVER_HELPER|function("Add"),selectors,1,&value,&diagnostic));
    CHECK(value.type==NativeScriptType_Callable && value.as.u64==function("Add"));
    NativeScriptReceiverPool* pool=selected.pool;uint32_t top=pool->top;
    for(unsigned i=0;i<4096;++i)CHECK(nativeScriptReceiver_Capture(&selected,&caller,NativeScriptReceiver_Creator,true,true,&value,&diagnostic));
    CHECK(pool->top==top);
    CHECK(nativeScriptReceiver_Pop(pool,selected.frame,&diagnostic));
    CHECK(!nativeScriptReceiver_Validate(&selected,&caller,&receiver,false,&record,&diagnostic));CHECK(nativeScriptRuntime_Idle());
}
static bool receiver_only(void* user,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* result,NativeScriptDiagnostic* d)
{return nativeScriptBindingReceiver_TryResolve(user,arena,node,result,d)>0;}
static void source_negatives(void)
{
    const char* bodies[]={"creator.attrs.gain = 3","creator.state.no_such = 3","creator.helpers.Add(\"bad\")","holder.helpers.no_such()",
        "owner.params.gain = 3","current_owner.params.no_such = 3"};
    for(unsigned i=0;i<sizeof(bodies)/sizeof(*bodies);++i) {
        char text[512];snprintf(text,sizeof(text),"state float total\nparameter float gain = 2\nfunc Add(float value) -> float:\n    return value\narticle It_Kind_Mario_Cape states 1:\n    func Bad():\n        %s\n",bodies[i]);
        NativeScriptSourceNode source_nodes[64];NativeScriptIRNode expression_nodes[128];uint32_t expression_args[128];
        NativeScriptFunctionDeclaration declarations[8];NativeScriptParameterDeclaration params[8];NativeScriptValueDeclaration declarations_values[8];
        NativeScriptSourceTree tree={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=source_nodes,.capacity=64};
        NativeScriptIRArena arena={.source=text,.source_bytes=tree.source_bytes,.nodes=expression_nodes,.node_capacity=128,
            .arguments=expression_args,.argument_capacity=128};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
        NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=declarations,.function_capacity=8,
            .parameters=params,.parameter_capacity=8,.values=declarations_values,.value_capacity=8};CHECK(nativeScriptProgram_Index(&index,&diagnostic));
        NativeScriptBindingReceiverResolver resolver={.program=&index,.owner=declarations[1].owner,.source_class=NativeCallbackSource_Item};
        NativeScriptProgramCompiler compiler={.index=&index,.locals=locals,.local_capacity=V};
        // Use the production tri-state resolver via a typed bool composition,
        // just like the controller's source resolver.
        NativeScriptStatementCompiler body={0};compiler.native_bindings=receiver_only;compiler.native_binding_context=&resolver;
        CHECK(!nativeScriptProgram_CompileBody(&compiler,1,&body,&diagnostic));CHECK(diagnostic.message[0]);
    }
}
static void article_retention(void)
{
    world();NativeScriptRuntimeContext caller={.source=objects[0],.source_class=NativeCallbackSource_Fighter,
        .generation=101,.actor_id=101,.frame=7,.scene_epoch=19};
    NativeScriptReceiverScope scope={.pool=nativeScriptRuntime_Receivers()};
    CHECK(nativeScriptReceiver_Push(scope.pool,&caller,19,&bank->declarations,Ft_Kind_CustomFirst,
        lookup,helper,NULL,&scope.frame,&diagnostic));
    uint32_t owner=item.native_script_owner;NativeScriptValue token,absent;bool exists;
    const NativeScriptReceiverRecord* record;
    CHECK(nativeScriptReceiver_CaptureArticle(&scope,&caller,201,owner,&token,&diagnostic));
    CHECK(nativeScriptReceiver_ValidateArticle(&scope,&caller,&token,owner,&record,&diagnostic));
    CHECK(record->actor==objects[2]&&record->storage_owner==owner);
    for(unsigned i=0;i<2;++i){
        CHECK(nativeScriptReceiver_CaptureArticle(&scope,&caller,i?UINT32_MAX:999,owner,&absent,&diagnostic));
        CHECK(absent.type==NativeScriptType_Object&&!absent.as.object.identity&&!absent.as.object.generation);
        CHECK(nativeScriptReceiver_Exists(&scope,&caller,&absent,&exists,&diagnostic)&&!exists);
    }
    item.owner=objects[1];item.native_removal_in_progress=true;
    CHECK(nativeScriptReceiver_ValidateArticle(&scope,&caller,&token,owner,&record,&diagnostic));
    CHECK(nativeScriptReceiver_Exists(&scope,&caller,&token,&exists,&diagnostic)&&exists);
    item.native_removal_in_progress=false;
    item.native_article_author_kind=Ft_Kind_Link;
    CHECK(!nativeScriptReceiver_ValidateArticle(&scope,&caller,&token,owner,&record,&diagnostic));
    CHECK(nativeScriptReceiver_CaptureArticle(&scope,&caller,201,owner,&absent,&diagnostic)&&!absent.as.object.identity);
    item.native_article_author_kind=Ft_Kind_CustomFirst;
    storage[2].owner=0;
    CHECK(!nativeScriptReceiver_ValidateArticle(&scope,&caller,&token,owner,&record,&diagnostic));storage[2].owner=owner;
    Item replacement=item;objects[2]->user_data=&replacement;
    CHECK(!nativeScriptReceiver_ValidateArticle(&scope,&caller,&token,owner,&record,&diagnostic));objects[2]->user_data=&item;
    Fighter changed_fighter=fighters[0];objects[0]->user_data=&changed_fighter;
    CHECK(!nativeScriptReceiver_ValidateArticle(&scope,&caller,&token,owner,&record,&diagnostic));objects[0]->user_data=&fighters[0];
    CHECK(nativeScriptReceiver_ValidateArticle(&scope,&caller,&token,owner,&record,&diagnostic));
    NativeMatchPoolReference physical;CHECK(nativeMatchPool_Reference(&nativeGObjPool,objects[2],&physical));
    CHECK(nativeMatchPool_Free(&nativeGObjPool,physical));CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&physical)==objects[2]);
    objects[2]->user_data=&item;item.entity=objects[2];
    CHECK(!nativeScriptReceiver_ValidateArticle(&scope,&caller,&token,owner,&record,&diagnostic));
    CHECK(nativeScriptReceiver_Exists(&scope,&caller,&token,&exists,&diagnostic)&&!exists);
    CHECK(nativeScriptReceiver_Pop(scope.pool,scope.frame,&diagnostic));CHECK(nativeScriptRuntime_Idle());
}
int main(int argc,char** argv)
{
    if(argc!=2)return 2;bank=&scriptKineticsFixture_Source()->compiler.bank;build(argv[1]);source_cases();parameter_cases();retained_cases();bounds_and_schema();source_negatives();article_retention();
    printf("{\"result\":\"pass\",\"checks\":%u,\"execution_runs\":%u,\"native_pointer_values\":false}\n",checks,runs);return 0;
}
