#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcoll.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/types.h>
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/lb/types.h>
#include <melee/lb/lb_00F9.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Nodes=256, Expressions=1024, Functions=16, Fields=32 };
static char text[4096];static NativeScriptSourceNode syntax[Nodes];
static NativeScriptIRNode expressions[Expressions];static uint32_t arguments[Expressions];
static NativeScriptFunctionDeclaration declarations[Functions];static NativeScriptParameterDeclaration parameters[32];
static NativeScriptValueDeclaration values[Fields];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[2048];static NativeScriptFunctionCode functions[Functions];
static NativeScriptExecutionFunction executions[Functions];static NativeScriptCompileFunctionScratch graph[Functions];
static NativeScriptStorageField fields[Fields];static NativeScriptStorageOwner owners[4];static NativeScriptParameterSlot slots[Fields];
static NativeScriptCompileBank bank;static NativeScriptItemProgram program;static NativeScriptItemState state;
static Article asset;static Item item;static Item_GObj* object;
static NativeScriptDiagnostic diagnostic;static unsigned checks,cases,pairs,vm_calls,wind_calls,mutation;
static struct lb_80011A50_t pool[8],expected[8];
static struct lb_80011A50_t* lb_804D63AC;static struct lb_80011A50_t* lb_804D63B0;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"%u: %s (%s)\n",(unsigned)__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static NativeScriptItemActor* actor(void)
{for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==object)return &nativeScriptItemActors[i];CHECK(false);return NULL;}
static void fixture_after_wind(void)
{
    ++wind_calls;if(!mutation)return;
    NativeScriptItemActor* a=actor();
    switch(mutation){
    case 1:++item.x1C;break;case 2:a->storage.live=false;break;case 3:a->faulted=true;break;
    case 4:++program.identity;break;case 5:++bank.storage_layout.identity;break;
    case 6:++bank.storage_parameters.identity;break;case 7:item.native_removal_in_progress=true;break;
    case 8:object->classifier=HSD_GOBJ_CLASS_FIGHTER;break;case 9:object->hsd_obj=&asset;break;
    case 10:item.xC4_article_data=NULL;break;case 11:bank.valid=false;break;
    case 12:item.owner=object;item.msid=0;break; // ordinary state/owner changes remain legal
    }
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "wind_native.inc"
#pragma clang diagnostic pop

// No Fighter actor/rig in the Item executable: the actual source guard must
// reject instead of inventing a bound Fighter layout. Fighter proof is separate.
FighterPartsTable** ftPartsTable;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){(void)kind;return NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){(void)kind;return NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* o){(void)o;return NativeScriptVM_SourceRetired;}
#include "fighter_guard.inc"
Fighter_Part ftParts_GetBoneIndex(Fighter* fp,Fighter_Part part){(void)fp;(void)part;abort();}
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result){(void)joint;(void)offset;(void)result;abort();}
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj* source,...){(void)effect;(void)source;abort();}
void efAsync_Spawn(HSD_GObj* o,void* q,u32 m,u32 e,HSD_JObj* j,...){(void)o;(void)q;(void)m;(void)e;(void)j;abort();}
void ftColl_CreateReflectHit(Fighter_GObj* o,ReflectDesc* d,HSD_GObjEvent cb){(void)o;(void)d;(void)cb;abort();}
void ftColl_8007AEF8(Fighter_GObj* o){(void)o;abort();}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{return author==program.author_kind&&owner==program.owner?&program:NULL;}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t node,NativeScriptDiagnostic* d)
{
    (void)unused;const NativeScriptSourceNode* n=&tree->nodes[node];
    if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 1");
    if(n->kind==NativeScriptSource_ArticleState)return nativeScriptSource_SpanEquals(tree,n->value,"0 anim -1");
    if((n->kind==NativeScriptSource_Function||n->kind==NativeScriptSource_State)&&tree->nodes[n->parent].kind==NativeScriptSource_Article)return true;
    return nativeScriptCompile_MinimalMetadata(NULL,tree,node,d);
}
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=Nodes},
        .expressions={.nodes=expressions,.node_capacity=Expressions,.arguments=arguments,.argument_capacity=Expressions},
        .declarations={.functions=declarations,.function_capacity=Functions,.parameters=parameters,.parameter_capacity=32,.values=values,.value_capacity=Fields},
        .locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=2048,.function_code=functions,.function_code_capacity=Functions,
        .execution_functions=executions,.execution_function_capacity=Functions,.graph=graph,.graph_capacity=Functions,
        .storage_layout={.fields=fields,.field_capacity=Fields,.owners=owners,.owner_capacity=4},.parameter_slots=slots,.parameter_slot_capacity=Fields};
    const char* source="fighter:\n    behavior independent\narticle Probe states 1:\n    state int tail\n"
        "    func Wind(float x, float y, float z, int frames, float strength, float decay, float frequency):\n"
        "        effects.wind_at_position(x, y, z, frames, strength, decay, frequency)\n        item.state.tail = item.state.tail + 1\n"
        "    state 0 anim -1:\n";
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(accepted==&bank);
    unsigned owner=0;for(unsigned i=0;i<bank.tree.count;++i)if(bank.tree.nodes[i].kind==NativeScriptSource_Article)owner=i;CHECK(owner);
    program=(NativeScriptItemProgram){.bank=&bank,.identity=17,.author_kind=Ft_Kind_Mario,.owner=owner,.kind=It_Kind_Mario_Fire,
        .recipe_index=UINT32_MAX,.article=&asset,.states=&state,.state_count=1,.drawing=NativeScriptItemDrawing_Standard};
    for(unsigned h=0;h<NativeScriptItemHook_Count;++h)program.hooks[h]=UINT32_MAX;
    state.animation_id=-1;for(unsigned p=0;p<NativeScriptItemPhase_Count;++p)state.functions[p]=UINT32_MAX;CHECK(nativeScriptItem_ProgramValid(&program));
}
static void data_remove(void* p){(void)p;}
static void reset_pool(void)
{memset(pool,0,sizeof(pool));for(unsigned i=0;i<8;++i)pool[i].next=i<7?&pool[i+1]:NULL;lb_804D63AC=pool;lb_804D63B0=NULL;wind_calls=0;}
static NativeScriptValue float_value(float x){return (NativeScriptValue){.type=NativeScriptType_F32,.as.f32=x};}
static NativeScriptValue i32(int x){return (NativeScriptValue){.type=NativeScriptType_I32,.as.i64=x};}
static NativeScriptValue sentinel(void){return (NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};}
static NativeScriptRuntimeContext context(void)
{return (NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Item,.generation=7,.actor_id=7,.scene_epoch=nativeScriptItemSceneEpoch};}
static bool invoke(NativeScriptValue* args,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context();return nativeScriptBindingFeedback_Invoke(&c,NativeScriptBinding_EffectsWindAtPosition,args,7,out,&diagnostic);}
static void restore(const NativeScriptItemActor* a,const Item* native,uint64_t program_id,uint64_t layout_id,uint64_t parameter_id)
{*actor()=*a;item=*native;program.identity=program_id;bank.storage_layout.identity=layout_id;bank.storage_parameters.identity=parameter_id;bank.valid=true;object->classifier=HSD_GOBJ_CLASS_ITEM;object->hsd_obj=NULL;mutation=0;}
int main(void)
{
    prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=2,.gproc_pri_max=8};HSD_GObj_80391304(&config);
    object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);item.entity=object;item.kind=program.kind;item.x1C=7;item.msid=-1;
    item.native_article_author_kind=program.author_kind;item.native_script_owner=program.owner;item.xC4_article_data=&asset;
    GObj_InitUserData(object,HSD_GOBJ_CLASS_ITEM,data_remove,&item);CHECK(nativeScriptItem_Attach(object,&program));
    const NativeScriptItemActor saved=*actor();const Item native=item;uint64_t program_id=program.identity,layout_id=bank.storage_layout.identity,parameter_id=bank.storage_parameters.identity;
    NativeScriptValue args[]={float_value(-0.0f),float_value(35.25f),float_value(-9.5f),i32(120),float_value(.4f),float_value(.003f),float_value(1.0471976f)};
    for(unsigned pair=0;pair<12;++pair){reset_pool();args[3]=i32(pair&1?-7:120);args[4]=float_value(pair&1?-.4f:.4f);args[5]=float_value(pair&1?-0.0f:.003f);args[6]=float_value(pair&1?-1.0471976f:1.0471976f);
        for(unsigned fill=0;fill<pair;++fill){Vec3 position={(float)fill,2,3};wind_reference(&position,4,.2f,.3f,.4f);}
        struct lb_80011A50_t before[8];memcpy(before,pool,sizeof(before));struct lb_80011A50_t* free_head=lb_804D63AC;struct lb_80011A50_t* active_head=lb_804D63B0;
        Vec3 position={args[0].as.f32,args[1].as.f32,args[2].as.f32};wind_reference(&position,(int)args[3].as.i64,args[4].as.f32,args[5].as.f32,args[6].as.f32);memcpy(expected,pool,sizeof(expected));
        struct lb_80011A50_t* expected_free=lb_804D63AC;struct lb_80011A50_t* expected_active=lb_804D63B0;
        memcpy(pool,before,sizeof(pool));lb_804D63AC=free_head;lb_804D63B0=active_head;NativeScriptValue out=sentinel();CHECK(invoke(args,&out));
        CHECK(out.type==NativeScriptType_Void&&wind_calls==1&&!memcmp(expected,pool,sizeof(pool))&&lb_804D63AC==expected_free&&lb_804D63B0==expected_active);++pairs;++cases;}
    args[3]=i32(120);args[4]=float_value(.4f);args[5]=float_value(.003f);args[6]=float_value(1.0471976f);
    // Actual compiled Article helper reaches the catalog and native backend.
    reset_pool();NativeScriptValue out=sentinel();CHECK(nativeScriptExecution_Run(&actor()->execution,object,NativeCallbackSource_Item,7,1,nativeScriptItemSceneEpoch,0,args,7, NULL,&out)==NativeScriptVM_Ok);++vm_calls;
    CHECK(out.type==NativeScriptType_Void&&wind_calls==1&&actor()->bytes[0]==1&&nativeScriptRuntime_Idle());++cases;
    for(unsigned how=1;how<=12;++how){restore(&saved,&native,program_id,layout_id,parameter_id);reset_pool();mutation=how;out=sentinel();NativeScriptValue before=out;
        bool ok=invoke(args,&out);CHECK(ok==(how==12));CHECK(wind_calls==1&&lb_804D63B0&&lb_804D63B0->unk_count0==120);
        CHECK(how==12?out.type==NativeScriptType_Void:!memcmp(&out,&before,sizeof(out)));++cases;}
    for(unsigned how=1;how<=3;++how){restore(&saved,&native,program_id,layout_id,parameter_id);reset_pool();mutation=how;out=sentinel();NativeScriptValue before=out;
        NativeScriptVMStatus status=nativeScriptExecution_Run(&actor()->execution,object,NativeCallbackSource_Item,7,2,nativeScriptItemSceneEpoch,0,args,7, NULL,&out);++vm_calls;
        CHECK(status==(how<3?NativeScriptVM_SourceRetired:NativeScriptVM_Error));CHECK(!memcmp(&out,&before,sizeof(out))&&actor()->bytes[0]==0&&wind_calls==1&&nativeScriptRuntime_Idle());++cases;}
    restore(&saved,&native,program_id,layout_id,parameter_id);reset_pool();out=sentinel();NativeScriptValue before=out;
    for(unsigned i=0;i<7;++i)if(i!=3){NativeScriptValue original=args[i];args[i]=float_value(NAN);CHECK(!invoke(args,&out)&&!memcmp(&out,&before,sizeof(out))&&!wind_calls);args[i]=original;++cases;}
    NativeScriptRuntimeContext c=context();++c.scene_epoch;CHECK(!nativeScriptBindingFeedback_Invoke(&c,NativeScriptBinding_EffectsWindAtPosition,args,7,&out,&diagnostic)&&!wind_calls);++cases;
    actor()->faulted=true;CHECK(!invoke(args,&out)&&!wind_calls);actor()->faulted=false;++cases;
    nativeScriptItem_Detach(object);CHECK(!invoke(args,&out)&&!wind_calls);CHECK(nativeScriptItem_Attach(object,&program));++cases;
    c=context();NativeScriptValue joint_wind[]={{.type=NativeScriptType_U32,.as.u64=0},float_value(0),i32(1),float_value(1),float_value(1),float_value(1)};
    CHECK(!nativeScriptBindingFeedback_Invoke(&c,NativeScriptBinding_EffectsWind,joint_wind,6,&out,&diagnostic)&&!wind_calls);++cases;
    CHECK(!nativeScriptBindingFeedback_Invoke(&c,NativeScriptBinding_EffectsWindAtPosition,args,6,&out,&diagnostic)&&!wind_calls);++cases;
    NativeScriptValue original=args[0];args[0]=i32(1);CHECK(!invoke(args,&out)&&!wind_calls);args[0]=original;++cases;
    item.native_removal_in_progress=true;CHECK(invoke(args,&out)&&out.type==NativeScriptType_Void&&wind_calls==1);item.native_removal_in_progress=false;++cases;
    CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pool_pairs\":%u,\"vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
