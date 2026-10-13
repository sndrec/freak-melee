#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_binding_item_resources_resolve.h>
#include <melee/ft/native_source_asset_facts.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SourceBytes=32768,Nodes=2048,Exprs=8192,Functions=64,Fields=128,Instructions=16384 };
typedef struct Buffers {
    char source[SourceBytes];NativeScriptSourceNode nodes[Nodes];NativeScriptIRNode expressions[Exprs];uint32_t arguments[Exprs];
    NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[Fields];NativeScriptValueDeclaration values[Fields];
    NativeScriptLocal locals[128];NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];NativeScriptExecutionFunction execution[Functions];NativeScriptCompileFunctionScratch graph[Functions];
    NativeScriptStorageField fields[Fields];NativeScriptStorageOwner owners[32];NativeScriptParameterSlot slots[Fields];
} Buffers;
static Buffers buffers[2];static NativeScriptCompileBank banks[2];
static NativeScriptItemPrograms prepared,before,copied;
static NativeScriptDiagnostic diagnostic;
static NativeSourceAssetFacts facts;
static HSD_Joint primary,model_root;
static ItemAttr attributes;
static ItemModelDesc model={.x0_joint=&primary};
static ItemStateDesc state_desc[1];static ItemStateArray state_array={state_desc,1};
static Article articles[2];
static NativeScriptItemModelRootAsset roots[2];
static NativeScriptItemAsset assets[2];static NativeScriptItemMetadataContext metadata;
static unsigned checks,cases,vm_calls,index_checks,matrix_constants,decoded_paths;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %u: %s; %s (%u:%u)\n",(unsigned)__LINE__,#v,diagnostic.message,diagnostic.source.line,diagnostic.source.column);exit(1);}}while(0)
static const char* prefix=
"fighter:\n    behavior independent\narticle Widget states 1:\n"
"    resources capacity 6 reserve_part 6\n    model_root RootA 84\n"
"    state int index\n    state int evaluations\n    state int rhs_calls\n"
"    state float matrix[12]\n    state float x\n    state float y\n    state float z\n"
"    state 0 anim 0:\n"
"    func NextIndex() -> int:\n        item.state.evaluations = item.state.evaluations + 1\n        return item.state.index\n"
"    func RHS() -> float:\n        item.state.rhs_calls = item.state.rhs_calls + 1\n        item.state.index = 7\n        return 1.25\n";
static const char* suffix=
"article Other states 1:\n    model_root OtherRoot 88\n    state float foreign[12]\n    state 0 anim 0:\n";
static void setup(unsigned i)
{
    Buffers* b=&buffers[i];banks[i]=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=SourceBytes,.tree={.nodes=b->nodes,.capacity=Nodes},
        .expressions={.nodes=b->expressions,.node_capacity=Exprs,.arguments=b->arguments,.argument_capacity=Exprs},
        .declarations={.functions=b->functions,.function_capacity=Functions,.parameters=b->parameters,.parameter_capacity=Fields,.values=b->values,.value_capacity=Fields},
        .locals=b->locals,.local_capacity=128,.instructions=b->instructions,.instruction_capacity=Instructions,.function_code=b->code,.function_code_capacity=Functions,
        .execution_functions=b->execution,.execution_function_capacity=Functions,.graph=b->graph,.graph_capacity=Functions,
        .storage_layout={.fields=b->fields,.field_capacity=Fields,.owners=b->owners,.owner_capacity=32},.parameter_slots=b->slots,.parameter_slot_capacity=Fields};
}
static void source(char* out,const char* body){snprintf(out,SourceBytes,"%s%s%s",prefix,body,suffix);}
static void bind_assets(const char* text,unsigned i)
{
    setup(i);NativeScriptSourceTree* tree=&banks[i].tree;tree->source=text;tree->source_bytes=(uint32_t)strlen(text);
    if(!nativeScriptSource_Parse(tree,&diagnostic))return;
    for(unsigned n=1;n<tree->count;++n)if(tree->nodes[n].kind==NativeScriptSource_Article){
        unsigned slot=nativeScriptSource_SpanEquals(tree,tree->nodes[n].name,"Widget")?0:1;assets[slot].owner_node=n;
    }
}
static bool compile(const char* text,unsigned i,const NativeScriptCompileBank** accepted)
{
    diagnostic=(NativeScriptDiagnostic){0};bind_assets(text,i);
    NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    return nativeScriptCompile_Build(&banks[i],&request,accepted,&diagnostic);
}
static uint32_t function(const NativeScriptCompileBank* b,uint32_t owner,const char* name)
{for(unsigned f=0;f<b->declarations.function_count;++f)if(b->declarations.functions[f].owner==owner&&nativeScriptSource_SpanEquals(&b->tree,b->declarations.functions[f].name,name))return f;CHECK(false);return UINT32_MAX;}
static uint32_t field(const NativeScriptCompileBank* b,uint32_t owner,const char* name)
{for(unsigned v=0;v<b->declarations.value_count;++v)if(b->declarations.values[v].owner==owner&&nativeScriptSource_SpanEquals(&b->tree,b->declarations.values[v].name,name))return v;CHECK(false);return UINT32_MAX;}
static void replace(char* out,const char* old,const char* replacement)
{char* at=strstr(out,old);CHECK(at);size_t rest=strlen(at+strlen(old));memmove(at+strlen(replacement),at+strlen(old),rest+1);memcpy(at,replacement,strlen(replacement));}
static void reject(const char* text,const NativeScriptCompileBank* accepted)
{
    before=prepared;const NativeScriptCompileBank* kept=accepted;
    bool ok=compile(text,1,&kept);if(ok)ok=nativeScriptItemPrograms_Prepare(&banks[1],Ft_Kind_Mario,99,&metadata,&prepared,&diagnostic);
    CHECK(!ok);CHECK(!memcmp(&before,&prepared,sizeof(prepared)));if(!banks[1].valid)CHECK(kept==accepted);++cases;
}
typedef struct ResourceVM {
    NativeScriptStorageInstance storage;
    uint32_t matrix,index,evaluations,rhs,x,y,z;
    unsigned expected_index,output_writes;
    bool live,false_result,retire;
} ResourceVM;
static bool alive(NativeScriptRuntimeContext* c){ResourceVM* r=c->user;return r->live&&r->storage.live;}
static bool read_value(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    ResourceVM* r=c->user;
    if((semantic&NATIVE_SCRIPT_STORAGE_DOMAIN_MASK)==NATIVE_SCRIPT_STATE_SEMANTIC)return nativeScriptStorage_Read(&r->storage,semantic,selectors,count,out,d);
    if(semantic==NativeScriptItemResource_Exists){CHECK(count==1&&selectors[0].type==NativeScriptType_I64&&selectors[0].as.u64==UINT64_C(4294967296));++index_checks;*out=(NativeScriptValue){.type=NativeScriptType_Bool};return true;}
    return false;
}
static bool write_value(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    ResourceVM* r=c->user;
    if((semantic&NATIVE_SCRIPT_STORAGE_DOMAIN_MASK)==NATIVE_SCRIPT_STATE_SEMANTIC){uint32_t id=semantic&NATIVE_SCRIPT_STORAGE_INDEX_MASK;if(id==r->x||id==r->y||id==r->z)++r->output_writes;return nativeScriptStorage_Write(&r->storage,semantic,selectors,count,value,d);}
    if(semantic==NativeScriptItemResource_FloatFirst){CHECK(count==1&&selectors[0].type==NativeScriptType_I32&&selectors[0].as.i64==r->expected_index);CHECK(value->type==NativeScriptType_F32&&value->as.f32==1.25f);++index_checks;return true;}
    return false;
}
static bool invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,uint32_t selector_count,NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)d;ResourceVM* r=c->user;++vm_calls;CHECK(selector_count==1&&selectors[0].type==NativeScriptType_I32&&selectors[0].as.i64==2);
    if((semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_MATRIX){CHECK(count==1&&args[0].type==NativeScriptType_I32&&args[0].as.i64==r->matrix);++matrix_constants;}
    else if(semantic==NativeScriptItemResource_AttachmentPoint){CHECK(count==6&&args[0].type==NativeScriptType_F32&&args[1].type==NativeScriptType_F32&&args[2].type==NativeScriptType_F32);for(unsigned i=3;i<6;++i){CHECK(args[i].type==NativeScriptType_F32);args[i].as.f32=r->false_result?0:(float)(i+1);}}
    else if(semantic==NativeScriptItemResource_LineSpeed){CHECK(count==4&&args[0].type==NativeScriptType_I64&&args[0].as.u64==UINT64_C(4294967296));for(unsigned i=1;i<4;++i)args[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=(float)(i+10)};}
    else CHECK(false);
    if(r->retire)r->live=false;*out=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=!r->false_result};return true;
}
static NativeScriptVMStatus run_resource(const NativeScriptCompileBank* b,const NativeScriptItemProgram* p,const char* name,ResourceVM* r)
{
    static NativeScriptValue registers[4096];static NativeScriptVMFrame frames[32];NativeScriptVM vm={.registers=registers,.register_capacity=4096,.frames=frames,.frame_capacity=32};
    NativeScriptRuntimeContext context={.user=r};NativeScriptVMHost host={.source_alive=alive,.read=read_value,.write=write_value,.invoke=invoke};NativeScriptValue out;
    NativeScriptVMStatus status=nativeScriptVM_Run(&vm,&b->code,&host,&context,function(b,p->owner,name),NULL,0,&out);if(status==NativeScriptVM_Error)diagnostic=vm.diagnostic;return status;
}
static float scalar(ResourceVM* r,unsigned id)
{NativeScriptValue out;CHECK(nativeScriptStorage_Read(&r->storage,NATIVE_SCRIPT_STATE_SEMANTIC|id,NULL,0,&out,&diagnostic));return out.type==NativeScriptType_F32?out.as.f32:(float)out.as.i64;}
static void initial_values(ResourceVM* r)
{
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=2};CHECK(nativeScriptStorage_Write(&r->storage,NATIVE_SCRIPT_STATE_SEMANTIC|r->index,NULL,0,&value,&diagnostic));
    const uint32_t ids[]={r->x,r->y,r->z};for(unsigned i=0;i<3;++i){value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=(float)(9-i)};CHECK(nativeScriptStorage_Write(&r->storage,NATIVE_SCRIPT_STATE_SEMANTIC|ids[i],NULL,0,&value,&diagnostic));}
}
int main(void)
{
    facts.joints=4;
    for(unsigned i=0;i<2;++i){articles[i]=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&state_array};roots[i]=(NativeScriptItemModelRootAsset){i?"OtherRoot":"RootA",i?88:84,&model_root};assets[i]=(NativeScriptItemAsset){.declaration=i?"Other":"Widget",.kind=It_Kind_Link_HShot,.article=&articles[i],.model_roots=&roots[i],.model_root_count=1};}
    metadata=(NativeScriptItemMetadataContext){.assets=assets,.asset_count=2,.source_facts=&facts};
    static char text[SourceBytes];source(text,
"    func CaptureOnce():\n        item.resource(NextIndex()).position.x = RHS()\n"
"    func Matrix():\n        item.resource(NextIndex()).read_attachment_matrix(item.state.matrix)\n        item.resource(NextIndex()).read_world_matrix(item.state.matrix)\n        item.resource(NextIndex()).set_world_matrix(item.state.matrix)\n"
"    func Outputs():\n        item.resource(NextIndex()).read_attachment_point(0, 0, 6, item.state.x, item.state.y, item.state.z)\n"
"    func LineSpeed():\n        item.resource(NextIndex()).read_line_speed(4294967296, item.state.x, item.state.y, item.state.z)\n"
"    func Wide() -> bool:\n        return item.resource(4294967296).exists\n"
"    func APIs():\n        item.resources.begin(6)\n        let int resource_count = item.resources.count\n        item.resource(0).create(0, 1.5, 5)\n        item.resource(1).create(0, 1.5, 5, 1, 2, 3)\n        item.resource(0).load_model(RootA)\n        item.resource(0).set_draw(true)\n        item.resource(0).bind_part(6)\n        item.resource(0).bind_capture_anchor()\n        item.resource(0).sweep_stage()\n        item.resource(0).sweep_point()\n        item.resource(0).follow_floor()\n        item.resource(0).spawn_effect_at_position(1011, 0, 1, 2)\n        item.resource(0).spawn_effect_facing_at_position(1007, 0, 1, 2, -1)\n        let bool present = item.resource(0).exists\n        item.resource(0).surface_line = -1\n        let int environment = item.resource(0).collision.environment\n        let int floor_line = item.resource(0).collision.floor_line\n        let int left_wall_line = item.resource(0).collision.left_wall_line\n        let int right_wall_line = item.resource(0).collision.right_wall_line\n        let int ceiling_line = item.resource(0).collision.ceiling_line\n        item.resource(0).velocity.z = 3\n        item.resource(0).collision.position.x = 2\n        item.resource(0).collision.previous.y = 4\n        let float floor_normal_x = item.resource(0).collision.floor_normal.x\n        let float left_wall_normal_y = item.resource(0).collision.left_wall_normal.y\n        let float right_wall_normal_z = item.resource(0).collision.right_wall_normal.z\n        let float ceiling_normal_y = item.resource(0).collision.ceiling_normal.y\n        item.resource(0).joint(\"0/2/255\").translation.x = 2\n        item.resource(0).joint(\"0/2/255\").rotation.y = 3\n        item.resource(0).joint(\"0/2/255\").scale.z = 4\n        item.resource(0).joint(\"\").quaternion.w = 1\n        item.resource(0).joint(\"\").visible = true\n        item.resource(0).joint(\"\").self_visible = false\n        let bool joint_present = item.resource(0).joint(\"\").exists\n        item.resources.release()\n");
    const NativeScriptCompileBank* accepted=NULL;CHECK(compile(text,0,&accepted));CHECK(accepted==&banks[0]);CHECK(nativeScriptItemPrograms_Prepare(accepted,Ft_Kind_Mario,123,&metadata,&prepared,&diagnostic));++cases;
    const NativeScriptItemProgram* p=&prepared.programs[0];CHECK(prepared.count==2&&p->resource_capacity==6&&p->resource_reserved_part==6);CHECK(p->resource_selector_count==10&&prepared.programs[1].resource_selector_count==0);
    for(unsigned i=0;i<p->resource_selector_count;++i){const NativeScriptItemResourceSelector* s=&p->resource_selectors[i];CHECK(s>=prepared.resource_selectors&&s<prepared.resource_selectors+prepared.resource_selector_count);
        if(i)CHECK(p->resource_selectors[i-1].semantic<s->semantic);
        if((s->semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_PROPERTY){CHECK(s->depth==0||(s->depth==3&&s->path[0]==0&&s->path[1]==2&&s->path[2]==255));++decoded_paths;}
        else CHECK(s->declaration==field(accepted,p->owner,"matrix"));
    }
    copied=prepared;nativeScriptItemPrograms_Rebind(&copied,&prepared);CHECK(copied.programs[0].resource_selectors==copied.resource_selectors);++cases;
    NativeScriptItemProgram narrow=*p;NativeScriptItemResourceSelector destination[16],old[16];memset(destination,0xA5,sizeof(destination));memcpy(old,destination,sizeof(old));CHECK(!nativeScriptBindingItemResources_PrepareSelectors(accepted,&narrow,destination,1,&diagnostic));CHECK(!memcmp(destination,old,sizeof(old))&&narrow.resource_selectors==p->resource_selectors);++cases;
    uint32_t matrix=field(accepted,p->owner,"matrix");for(unsigned i=0;i<accepted->code.instruction_count;++i){const NativeScriptInstruction* n=&accepted->code.instructions[i];CHECK(!(n->opcode==NativeScriptBC_Read&&n->semantic==(NATIVE_SCRIPT_STATE_SEMANTIC|matrix)));}
    static uint8_t state_bytes[1024];ResourceVM r={.live=true,.matrix=matrix,.index=field(accepted,p->owner,"index"),.evaluations=field(accepted,p->owner,"evaluations"),.rhs=field(accepted,p->owner,"rhs_calls"),.x=field(accepted,p->owner,"x"),.y=field(accepted,p->owner,"y"),.z=field(accepted,p->owner,"z"),.expected_index=2};
#define RESET() do{r.live=true;r.false_result=false;r.retire=false;r.output_writes=0;CHECK(nativeScriptStorage_Create(&accepted->storage_layout,&accepted->storage_parameters,p->owner,1,1,state_bytes,sizeof(state_bytes),&r.storage,&diagnostic));initial_values(&r);}while(0)
    RESET();CHECK(run_resource(accepted,p,"CaptureOnce",&r)==NativeScriptVM_Ok);CHECK(scalar(&r,r.evaluations)==1&&scalar(&r,r.rhs)==1&&scalar(&r,r.index)==7);++cases;
    RESET();CHECK(run_resource(accepted,p,"Matrix",&r)==NativeScriptVM_Ok);CHECK(scalar(&r,r.evaluations)==3&&matrix_constants==3);++cases;
    RESET();CHECK(run_resource(accepted,p,"Outputs",&r)==NativeScriptVM_Ok);CHECK(scalar(&r,r.evaluations)==1&&r.output_writes==3&&scalar(&r,r.x)==4&&scalar(&r,r.y)==5&&scalar(&r,r.z)==6);++cases;
    RESET();r.false_result=true;CHECK(run_resource(accepted,p,"Outputs",&r)==NativeScriptVM_Ok);CHECK(r.output_writes==3&&scalar(&r,r.x)==0&&scalar(&r,r.y)==0&&scalar(&r,r.z)==0);++cases;
    RESET();r.retire=true;CHECK(run_resource(accepted,p,"Outputs",&r)==NativeScriptVM_SourceRetired);CHECK(r.output_writes==0&&scalar(&r,r.x)==9&&scalar(&r,r.y)==8&&scalar(&r,r.z)==7);++cases;
    RESET();CHECK(run_resource(accepted,p,"LineSpeed",&r)==NativeScriptVM_Ok);CHECK(r.output_writes==3&&scalar(&r,r.x)==11&&scalar(&r,r.y)==12&&scalar(&r,r.z)==13);++cases;
    RESET();CHECK(run_resource(accepted,p,"Wide",&r)==NativeScriptVM_Ok);++cases;
    NativeScriptValue sentinel={.type=NativeScriptType_I32,.as.i64=456},args[1]={{.type=NativeScriptType_I32,.as.i64=matrix}},selection={.type=NativeScriptType_I32};NativeScriptRuntimeContext empty={0};
    CHECK(!nativeScriptBindingItemResources_Invoke(&empty,NativeScriptItemResource_Begin,NULL,0,args,1,&sentinel,&diagnostic));CHECK(sentinel.type==NativeScriptType_I32&&sentinel.as.i64==456&&args[0].as.i64==matrix);
    CHECK(!nativeScriptBindingItemResources_Read(&empty,NativeScriptItemResource_Exists,&selection,1,&sentinel,&diagnostic));CHECK(sentinel.as.i64==456);++cases;
    const char* bad[]={"item.resource(1.5).exists","item.resource(0).create(0, 1, 2, 3)","item.resource(0).create(0.5, 1, 2)","item.resource(0).create(0, 1, 2.5)","item.resource(0).set_draw(1)","item.resource(0).load_model(0)","item.resource(0).load_model(OtherRoot)","item.resource(0).joint(\"256\").visible","item.resource(0).joint(\"0/\").visible","item.resource(0).joint(\"0//1\").visible","item.resource(0).joint(\"0/0/0/0/0/0/0/0/0/0/0/0/0\").visible","item.resource(0).joint(item.state.index).visible","item.resource(0).collision.floor_normal.x = 1","item.resource(0).collision.environment = 1","item.resource(0).joint(\"\").exists = false","item.resource(0).read_attachment_matrix(item.state.x)","item.resource(0).read_attachment_matrix(item.state.matrix[0])","item.resource(0).read_attachment_point(0, 0, 0, item.state.x, item.state.x, item.state.z)","item.resource(0).read_line_speed(0.5, item.state.x, item.state.y, item.state.z)","item.resource(0).add_animation(RootA)"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){static char candidate[SourceBytes],body[4096];snprintf(body,sizeof(body),"    func Bad():\n        %s\n",bad[i]);source(candidate,body);reject(candidate,accepted);}
    const char* metadata_bad[]={"capacity 1 reserve_part 6","capacity 91 reserve_part 6","capacity 4294967296 reserve_part 6","capacity 6 reserve_part 2","capacity 6 reserve_part 999","capacity 6 reserve_part 6 trailing","capacity count reserve_part 6"};
    for(unsigned i=0;i<sizeof(metadata_bad)/sizeof(*metadata_bad);++i){static char candidate[SourceBytes];source(candidate,"    func Bad():\n        pass\n");replace(candidate,"capacity 6 reserve_part 6",metadata_bad[i]);reject(candidate,accepted);}
    const char* matrix_bad[]={"state float matrix[11]","state float matrix[13]","state int matrix[12]","state float matrix"};
    for(unsigned i=0;i<sizeof(matrix_bad)/sizeof(*matrix_bad);++i){static char candidate[SourceBytes];source(candidate,"    func Bad():\n        item.resource(0).read_attachment_matrix(item.state.matrix)\n");replace(candidate,"state float matrix[12]",matrix_bad[i]);reject(candidate,accepted);}
    static char candidate[SourceBytes];source(candidate,"    resources capacity 6\n    func Bad():\n        pass\n");reject(candidate,accepted);
    source(candidate,"    func Bad():\n        pass\n");replace(candidate,"    resources capacity 6 reserve_part 6\n","");replace(candidate,"    func Bad():\n        pass\n","    func Bad():\n        resources capacity 6\n");reject(candidate,accepted);
    source(candidate,"    func Bad():\n        pass\n");replace(candidate,"fighter:\n    behavior independent\n","fighter:\n    behavior independent\nfunc RootBad():\n    item.resources.begin(6)\n");reject(candidate,accepted);
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"vm_calls\":%u,\"captured_index_checks\":%u,\"matrix_constants\":%u,\"decoded_paths\":%u}\n",checks,cases,vm_calls,index_checks,matrix_constants,decoded_paths);return 0;
}
