#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/it/native_held_article.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjplink.h>
#include "boundary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""

enum { N=1024,F=32,V=32 };
static char source[16384];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction execution[F];
static NativeScriptCompileFunctionScratch graph[F];static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[4];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptItemProgram specs[2];static NativeScriptItemState states[2][2];static Article asset;
static HSD_GObj* holder;static Fighter fighter;static Item items[8];static unsigned next_item,checks,cases;
static NativeScriptDiagnostic diagnostic;
void (*held_after_write)(HSD_GObj*);
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"held source line %u: %s (%s / %s)\n",__LINE__,#x,diagnostic.message,nativeScriptRuntime_VM()->diagnostic.message);exit(1);}}while(0)
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{for(unsigned i=0;i<2;++i)if(author==specs[i].author_kind&&owner==specs[i].owner)return &specs[i];return NULL;}
static bool metadata(void* user,const NativeScriptSourceTree* tree,uint32_t i,NativeScriptDiagnostic* d)
{
    (void)user;const NativeScriptSourceNode* n=&tree->nodes[i];
    if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 2");
    if(n->kind==NativeScriptSource_ArticleState)return true;
    if(n->kind==NativeScriptSource_State)return n->parent<tree->count&&tree->nodes[n->parent].kind==NativeScriptSource_Article;
    if(n->kind==NativeScriptSource_Callback){NativeScriptEventResolution e;return nativeScriptEvents_Resolve(tree,n,&e,d);}
    return nativeScriptCompile_MinimalMetadata(NULL,tree,i,d);
}
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source,.source_capacity=sizeof(source),.tree={.nodes=nodes,.capacity=N},
        .expressions={.nodes=ir,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},
        .locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,
        .execution_functions=execution,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=4},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* input="fighter:\n    behavior independent\n"
        "article HeldTool states 2:\n    state int answer\n    state int requests\n    state int neutral\n    state float rate\n    state float duration\n"
        "    on held_status:\n            return item.state.answer\n"
        "    on held_request:\n            item.state.requests = item.state.requests + 1\n            item.state.rate = event.frame_rate\n            item.state.duration = event.duration\n            position.x = 12\n            if event.action == 9: return 1 / (event.action - 9) > 0\n            return event.action == 7\n"
        "    on holder_neutral:\n            item.state.neutral = item.state.neutral + 1\n            position.x = 12\n"
        "    state 0 anim -1:\n    state 1 anim -1:\n"
        "article PlainTool states 2:\n    state 0 anim -1:\n    state 1 anim -1:\n";
    NativeScriptCompileRequest request={.source=input,.source_bytes=(uint32_t)strlen(input),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)&&accepted==&bank);
    unsigned entity=0;
    for(unsigned i=0;i<bank.tree.count;++i)if(nodes[i].kind==NativeScriptSource_Article){
        CHECK(entity<2);specs[entity]=(NativeScriptItemProgram){.bank=&bank,.identity=100+entity,.author_kind=Ft_Kind_CustomFirst,
            .kind=It_Kind_Mario_Cape,.recipe_index=UINT32_MAX,.owner=i,.article=&asset,.states=states[entity],.state_count=2};
        for(unsigned h=0;h<NativeScriptItemHook_Count;++h)specs[entity].hooks[h]=UINT32_MAX;
        for(unsigned s=0;s<2;++s){states[entity][s].animation_id=-1;for(unsigned p=0;p<3;++p)states[entity][s].functions[p]=UINT32_MAX;}++entity;
    }
    CHECK(entity==2);
    for(unsigned i=0;i<bank.declarations.function_count;++i)if(functions[i].callback){
        NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&bank.tree,&nodes[functions[i].source_node],&e,&diagnostic));
        unsigned h=!strcmp(e.descriptor->name,"held_status")?NativeScriptItemHook_HeldStatus:
            !strcmp(e.descriptor->name,"held_request")?NativeScriptItemHook_HeldRequest:NativeScriptItemHook_HolderNeutral;
        CHECK(functions[i].owner==specs[0].owner);specs[0].hooks[h]=i;
    }
    CHECK(nativeScriptItem_ProgramValid(&specs[0])&&nativeScriptItem_ProgramValid(&specs[1]));
}
static HSD_GObj* create(unsigned which)
{
    CHECK(next_item<8);Item* item=&items[next_item];memset(item,0,sizeof(*item));
    HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
    *item=(Item){.entity=object,.kind=specs[which].kind,.x1C=next_item++,.msid=-1,.owner=holder,
        .native_article_author_kind=specs[which].author_kind,.native_script_owner=specs[which].owner,.xC4_article_data=&asset};
    GObj_InitUserData(object,6,fixture_userdata_remove,item);CHECK(nativeScriptItem_Attach(object,&specs[which]));
    if(which==0){NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,item->x1C,&view));
        for(unsigned i=0;i<bank.declarations.value_count;++i)if(values[i].owner==specs[0].owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,"answer")){
            NativeScriptValue answer={.type=NativeScriptType_I32,.as.i64=5};CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&answer,&diagnostic));}}
    fighter.item_gobj=object;return object;
}
static NativeScriptValue state(HSD_GObj* object,const char* name)
{
    NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,((Item*)object->user_data)->x1C,&view));
    for(unsigned i=0;i<bank.declarations.value_count;++i)if(values[i].owner==specs[0].owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name)){
        NativeScriptValue out;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&out,&diagnostic));return out;}
    CHECK(false);return(NativeScriptValue){0};
}
static void replace_binding(HSD_GObj* object){(void)object;fighter.item_gobj=NULL;}
static void change_generation(HSD_GObj* object){(void)object;++fighter.x8_spawnNum;}
static void fighter_remove(void* data){(void)data;}
static void reuse_holder(HSD_GObj* object)
{
    HSD_GObj* previous=holder;NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,holder,&reference));
    HSD_GObjFree(holder);holder=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(holder==previous);
    fighter.gobj=holder;fighter.item_gobj=object;GObj_InitUserData(holder,4,fighter_remove,&fighter);
    CHECK(nativeCallbackSource_Alive(holder,fighter.x8_spawnNum,NativeCallbackSource_Fighter));CHECK(nativeMatchPool_Resolve(reference)==NULL);
}
static void fixture(void)
{
    HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=2,.gproc_pri_max=8};HSD_GObj_80391304(&config);
    holder=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(holder);
    fighter=(Fighter){.gobj=holder,.kind=Ft_Kind_CustomFirst,.x8_spawnNum=0};GObj_InitUserData(holder,4,fighter_remove,&fighter);
    CHECK(nativeFighterRuntime_Definition(fighter.kind)==NULL);prepare();
    int32_t status=99;bool claimed=false;
    CHECK(!nativeHeldArticle_Status(holder,&status)&&status==-1);CHECK(!nativeHeldArticle_Request(holder,7,1,0));
    CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&!claimed);++cases;
    HSD_GObj* object=create(0);CHECK(((Item*)object->user_data)->x1C==0);
    CHECK(nativeHeldArticle_Status(holder,&status)&&status==5);++cases;
    CHECK(nativeHeldArticle_Request(holder,7,1.25f,15.5f));CHECK(state(object,"requests").as.i64==1);
    CHECK(state(object,"rate").as.f32==1.25f&&state(object,"duration").as.f32==15.5f);++cases;
    CHECK(nativeHeldArticle_Request(holder,4,-2.5f,0));CHECK(state(object,"requests").as.i64==2);
    CHECK(state(object,"rate").as.f32==-2.5f&&state(object,"duration").as.f32==0);++cases;
    CHECK(nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed&&state(object,"neutral").as.i64==1);++cases;
    fixture_transition=true;CHECK(nativeHeldArticle_Request(holder,7,2,3));CHECK(((Item*)object->user_data)->msid==1);
    CHECK(nativeHeldArticle_Status(holder,&status)&&status==5);++cases;
    held_after_write=replace_binding;CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed);fighter.item_gobj=object;++cases;
    held_after_write=change_generation;CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed);++cases;
    held_after_write=reuse_holder;CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed);++cases;
    CHECK(nativeHeldArticle_Request(holder,9,1,0));CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Error);
    CHECK(nativeHeldArticle_Status(holder,&status)&&status==-1);CHECK(nativeHeldArticle_Request(holder,7,1,0));
    CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed);HSD_GObjFree(object);++cases;
    object=create(1);CHECK(nativeHeldArticle_Status(holder,&status)&&status==-1);
    CHECK(nativeHeldArticle_Request(holder,7,1,0));CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed);HSD_GObjFree(object);++cases;
    object=create(0);nativeScriptItem_Detach(object);CHECK(nativeHeldArticle_Status(holder,&status)&&status==-1);
    CHECK(nativeHeldArticle_Request(holder,7,1,0));CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed);HSD_GObjFree(object);++cases;
    object=create(0);((Item*)object->user_data)->native_removal_in_progress=true;
    CHECK(nativeHeldArticle_Status(holder,&status)&&status==-1);CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed);HSD_GObjFree(object);++cases;
    object=create(0);fixture_remove=true;CHECK(!nativeHeldArticle_NotifyNeutral(holder,&claimed)&&claimed);
    CHECK(!nativeScriptItem_IsAttached(object)&&nativeScriptRuntime_Idle());fighter.item_gobj=NULL;++cases;
    CHECK(!nativeHeldArticle_Status(holder,&status)&&status==-1);
}
int main(void){fixture();printf("{\"status\":\"PASS source held article\",\"checks\":%u,\"cases\":%u,\"definitions\":0}\n",checks,cases);return 0;}
