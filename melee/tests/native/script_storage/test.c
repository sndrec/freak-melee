#include <melee/ft/native_script_storage.h>
#include <melee/ft/native_script_events.h>
#include <melee/ft/native_script_expression.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char source[8192];
static NativeScriptSourceNode syntax[512];
static NativeScriptIRNode nodes[2048];
static uint32_t arguments[1024];
static NativeScriptFunctionDeclaration functions[32];
static NativeScriptParameterDeclaration function_parameters[64];
static NativeScriptValueDeclaration declarations[64];
static NativeScriptStorageField fields[64];
static NativeScriptStorageOwner owners[16];
static NativeScriptParameterSlot parameter_slots[64],provided_slots[64],other_slots[64];
static NativeScriptStorageParameters parameters,provided,other_parameters;
static NativeScriptSourceTree tree;
static NativeScriptIRArena arena;
static NativeScriptProgramIndex index;
static NativeScriptStorageLayout layout;
static NativeScriptDiagnostic diagnostic;
static unsigned checks,ticks;
static uint32_t article_owner;
#define CHECK(value) do { ++checks; if (!(value)) { fprintf(stderr,"Storage check %d: %s (%s)\n",__LINE__,#value,diagnostic.message);exit(1); } } while (0)

static uint32_t value_id(uint32_t owner,const char* name)
{
    for(uint32_t i=0;i<index.value_count;++i)if(index.values[i].owner==owner&&nativeScriptSource_SpanEquals(&tree,index.values[i].name,name))
        return (index.values[i].parameter?NATIVE_SCRIPT_PARAMETER_SEMANTIC:NATIVE_SCRIPT_STATE_SEMANTIC)|i;
    fprintf(stderr,"Missing declaration %u.%s\n",owner,name);exit(1);
}

static uint32_t function_id(uint32_t owner,const char* name)
{
    for(uint32_t i=0;i<index.function_count;++i)if(index.functions[i].owner==owner&&!index.functions[i].callback&&nativeScriptSource_SpanEquals(&tree,index.functions[i].name,name))return i;
    fprintf(stderr,"Missing helper %u.%s\n",owner,name);exit(1);
}

static NativeScriptValue i32(int64_t value){return (NativeScriptValue){.type=NativeScriptType_I32,.as.i64=value};}
static NativeScriptValue f32(float value){return (NativeScriptValue){.type=NativeScriptType_F32,.as.f32=value};}
static NativeScriptValue read(NativeScriptStorageInstance* instance,uint32_t id,const NativeScriptValue* selector)
{
    NativeScriptValue result={0};CHECK(nativeScriptStorage_Read(instance,id,selector,selector?1:0,&result,&diagnostic));return result;
}

static void initialize(const char* path)
{
    FILE* input=fopen(path,"rb");CHECK(input!=NULL);size_t count=fread(source,1,sizeof(source)-1,input);CHECK(!ferror(input)&&feof(input));fclose(input);source[count]=0;
    tree=(NativeScriptSourceTree){source,(uint32_t)count,syntax,0,512};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    arena=(NativeScriptIRArena){source,(uint32_t)count,nodes,0,2048,arguments,0,1024};
    index=(NativeScriptProgramIndex){.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=32,
        .parameters=function_parameters,.parameter_capacity=64,.values=declarations,.value_capacity=64,
        .event_signature=nativeScriptEvents_Signature};
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    article_owner=UINT32_MAX;for(uint32_t i=0;i<tree.count;++i)if(tree.nodes[i].kind==NativeScriptSource_Article)article_owner=i;
    CHECK(article_owner!=UINT32_MAX);
    layout=(NativeScriptStorageLayout){.fields=fields,.field_capacity=64,.owners=owners,.owner_capacity=16};
    CHECK(nativeScriptStorage_BuildLayout(&layout,&index,&diagnostic));
    CHECK(layout.field_count==index.value_count&&layout.owner_count==2&&layout.identity);
    CHECK(nativeScriptStorage_Owner(&layout,0)->state_bytes==84);
    CHECK(nativeScriptStorage_Owner(&layout,article_owner)->state_bytes==24);
    uint32_t single=value_id(0,"single")&UINT32_C(0x0fffffff);
    CHECK(fields[single].array&&fields[single].count==1);
    for(uint32_t i=0;i<layout.field_count;++i)if(fields[i].parameter)CHECK(fields[i].offset==0);
}

static void parameter_tests(void)
{
    CHECK(nativeScriptStorage_PrepareParameters(&layout,parameter_slots,64,NULL,0,&parameters,&diagnostic));
    CHECK(!nativeScriptStorage_ParametersReady(&layout,&parameters,0,&diagnostic));
    CHECK(!nativeScriptStorage_ParametersReady(&layout,&parameters,article_owner,&diagnostic));
    uint32_t imported=value_id(0,"imported_gain")&UINT32_C(0x0fffffff);
    CHECK(!parameter_slots[imported].resolved&&parameter_slots[imported].value.type==NativeScriptType_Invalid);
    NativeScriptParameterSupply supplies[]={
        {value_id(0,"imported_gain"),0,f32(0.75f)},
        {value_id(article_owner,"imported_gain"),article_owner,f32(-1.25f)},
        {value_id(0,"gain"),0,f32(1.5f)},
        {value_id(0,"imported_flags"),0,{.type=NativeScriptType_U32,.as.u64=UINT32_C(0x80000001)}},
    };
    CHECK(nativeScriptStorage_PrepareParameters(&layout,provided_slots,64,supplies,4,&provided,&diagnostic));
    CHECK(nativeScriptStorage_ParametersReady(&layout,&provided,0,&diagnostic));
    CHECK(nativeScriptStorage_ParametersReady(&layout,&provided,article_owner,&diagnostic));
    CHECK(provided.identity!=parameters.identity&&provided.layout_identity==layout.identity);
    NativeScriptParameterSlot before[64];memcpy(before,provided_slots,sizeof(before));NativeScriptStorageParameters table_before=provided;
    NativeScriptParameterSupply wrong=supplies[0];wrong.owner=article_owner;
    CHECK(!nativeScriptStorage_PrepareParameters(&layout,provided_slots,64,&wrong,1,&provided,&diagnostic));
    wrong=(NativeScriptParameterSupply){value_id(0,"imported_flags"),0,{.type=NativeScriptType_U32,.as.u64=UINT64_C(0x100000000)}};
    CHECK(!nativeScriptStorage_PrepareParameters(&layout,provided_slots,64,&wrong,1,&provided,&diagnostic));
    CHECK(!memcmp(before,provided_slots,sizeof(before))&&!memcmp(&table_before,&provided,sizeof(provided)));
    wrong=supplies[0];wrong.value=(NativeScriptValue){.type=NativeScriptType_F64,.as.f64=0.75};
    CHECK(!nativeScriptStorage_PrepareParameters(&layout,provided_slots,64,&wrong,1,&provided,&diagnostic));
    wrong=supplies[0];wrong.semantic=value_id(0,"handle");wrong.value=i32(7);
    CHECK(!nativeScriptStorage_PrepareParameters(&layout,provided_slots,64,&wrong,1,&provided,&diagnostic));
    wrong=(NativeScriptParameterSupply){value_id(0,"seed"),0,i32(INT64_MAX)};
    CHECK(!nativeScriptStorage_PrepareParameters(&layout,provided_slots,64,&wrong,1,&provided,&diagnostic));
    NativeScriptParameterSupply duplicates[]={supplies[0],supplies[0]};
    CHECK(!nativeScriptStorage_PrepareParameters(&layout,provided_slots,64,duplicates,2,&provided,&diagnostic));
    CHECK(!nativeScriptStorage_PrepareParameters(&layout,provided_slots,layout.field_count-1,NULL,0,&provided,&diagnostic));
    supplies[0].value=f32(9.0f);CHECK(nativeScriptStorage_PrepareParameters(&layout,other_slots,64,supplies,4,&other_parameters,&diagnostic));
    CHECK(other_parameters.identity!=provided.identity);
}

static void field_tests(void)
{
    uint8_t bytes[128],article_bytes[128];memset(bytes,0xCC,sizeof(bytes));memset(article_bytes,0xDD,sizeof(article_bytes));
    NativeScriptStorageInstance instance,other;
    CHECK(nativeScriptStorage_Create(&layout,&provided,0,17,UINT32_C(0x80000001),bytes,sizeof(bytes),&instance,&diagnostic));
    CHECK(nativeScriptStorage_Create(&layout,&provided,article_owner,18,UINT32_C(16777217),article_bytes,sizeof(article_bytes),&other,&diagnostic));
    for(unsigned i=0;i<84;++i)CHECK(bytes[i]==0);
    for(unsigned i=84;i<sizeof(bytes);++i)CHECK(bytes[i]==0xCC);
    CHECK(read(&instance,value_id(0,"seed"),NULL).as.i64==INT32_MIN+1);
    CHECK(read(&instance,value_id(0,"gain"),NULL).as.f32==1.5f);
    CHECK(read(&instance,value_id(0,"enabled"),NULL).as.u64==1);
    CHECK(read(&instance,value_id(0,"imported_flags"),NULL).type==NativeScriptType_U32);
    CHECK(read(&instance,value_id(0,"imported_flags"),NULL).as.u64==UINT32_C(0x80000001));
    CHECK(read(&other,value_id(article_owner,"gain"),NULL).as.f32==2.0f);
    uint32_t handle=value_id(0,"handle");
    const uint32_t ids[]={0,16777217,UINT32_C(0x80000001),UINT32_MAX,UINT32_C(0xfffffffe)};
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i) {
        NativeScriptValue value={.type=NativeScriptType_U32,.as.u64=ids[i]};
        CHECK(nativeScriptStorage_Write(&instance,handle,NULL,0,&value,&diagnostic));
        CHECK((uint32_t)read(&instance,handle,NULL).as.i64==ids[i]);
    }
    for(unsigned i=0;i<12;++i) {
        uint32_t bits=UINT32_C(0x80000000)+i;float floating;memcpy(&floating,&bits,4);
        NativeScriptValue value=f32(floating),selector=i32(i);
        CHECK(nativeScriptStorage_Write(&instance,value_id(0,"matrix"),&selector,1,&value,&diagnostic));
        NativeScriptValue result=read(&instance,value_id(0,"matrix"),&selector);uint32_t observed;memcpy(&observed,&result.as.f32,4);CHECK(observed==bits);
    }
    NativeScriptValue yes={.type=NativeScriptType_Bool,.as.u64=1};NativeScriptValue truth={.type=NativeScriptType_Bool,.as.u64=7};
    CHECK(nativeScriptStorage_Write(&instance,value_id(0,"flags"),&yes,1,&truth,&diagnostic));
    CHECK(read(&instance,value_id(0,"flags"),&yes).as.u64==1);
    NativeScriptValue single_index=i32(0),floating=f32(-0.0f);
    CHECK(nativeScriptStorage_Write(&instance,value_id(0,"single"),&single_index,1,&floating,&diagnostic));
    CHECK(signbit(read(&instance,value_id(0,"single"),&single_index).as.f32));
    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},sentinel=result;
    CHECK(!nativeScriptStorage_Read(&instance,value_id(article_owner,"charge"),NULL,0,&result,&diagnostic)&&!memcmp(&result,&sentinel,sizeof(result)));
    CHECK(!nativeScriptStorage_Read(&other,value_id(0,"gain"),NULL,0,&result,&diagnostic));
    CHECK(!nativeScriptStorage_Read(&instance,value_id(0,"single"),NULL,0,&result,&diagnostic));
    CHECK(!nativeScriptStorage_Read(&instance,value_id(0,"charge"),&single_index,1,&result,&diagnostic));
    const NativeScriptValue bad_indices[]={
        {.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I64,.as.i64=-9223372036854775807LL},
        {.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)}, {.type=NativeScriptType_U32,.as.u64=12},
        {.type=NativeScriptType_F32,.as.f32=0},{.type=NativeScriptType_F64,.as.f64=0}};
    uint8_t unchanged[128];memcpy(unchanged,bytes,sizeof(bytes));
    for(unsigned i=0;i<sizeof(bad_indices)/sizeof(*bad_indices);++i) {
        result=sentinel;CHECK(!nativeScriptStorage_Read(&instance,value_id(0,"matrix"),&bad_indices[i],1,&result,&diagnostic));
        CHECK(!memcmp(&result,&sentinel,sizeof(result)));
        CHECK(!nativeScriptStorage_Write(&instance,value_id(0,"matrix"),&bad_indices[i],1,&floating,&diagnostic));
        CHECK(!memcmp(unchanged,bytes,sizeof(bytes)));
    }
    CHECK(!nativeScriptStorage_Write(&instance,value_id(0,"gain"),NULL,0,&floating,&diagnostic));
    CHECK(!nativeScriptStorage_Write(&instance,handle,NULL,0,&floating,&diagnostic));
    NativeScriptStorageInstance unresolved;CHECK(nativeScriptStorage_Create(&layout,&parameters,0,19,1,article_bytes,sizeof(article_bytes),&unresolved,&diagnostic));
    result=sentinel;CHECK(!nativeScriptStorage_Read(&unresolved,value_id(0,"imported_gain"),NULL,0,&result,&diagnostic));CHECK(!memcmp(&result,&sentinel,sizeof(result)));
    nativeScriptStorage_Destroy(&instance);result=sentinel;CHECK(!nativeScriptStorage_Read(&instance,handle,NULL,0,&result,&diagnostic)&&!memcmp(&result,&sentinel,sizeof(result)));
    CHECK(!nativeScriptStorage_Create(&layout,&provided,0,1,1,bytes,83,&instance,&diagnostic));
}

static void snapshot_tests(void)
{
    uint8_t bytes[128],saved[128];NativeScriptStorageInstance instance;
    CHECK(nativeScriptStorage_Create(&layout,&provided,0,99,7,bytes,sizeof(bytes),&instance,&diagnostic));
    NativeScriptValue value=i32(INT32_MIN+1);CHECK(nativeScriptStorage_Write(&instance,value_id(0,"handle"),NULL,0,&value,&diagnostic));
    NativeScriptStorageSnapshot snapshot={.bytes=saved,.capacity=sizeof(saved)};
    CHECK(nativeScriptStorage_Save(&instance,&snapshot,&diagnostic));CHECK(snapshot.state_bytes==84&&snapshot.parameter_identity==provided.identity);
    value=i32(123);CHECK(nativeScriptStorage_Write(&instance,value_id(0,"handle"),NULL,0,&value,&diagnostic));
    CHECK(nativeScriptStorage_Restore(&instance,&snapshot,&diagnostic));CHECK(read(&instance,value_id(0,"handle"),NULL).as.i64==INT32_MIN+1);
    for(unsigned change=0;change<6;++change) {
        NativeScriptStorageSnapshot wrong=snapshot;
        if(change==0)++wrong.owner;if(change==1)++wrong.identity;if(change==2)++wrong.generation;
        if(change==3)++wrong.layout_identity;if(change==4)++wrong.parameter_identity;if(change==5)--wrong.state_bytes;
        uint8_t previous[128];memcpy(previous,bytes,sizeof(previous));
        CHECK(!nativeScriptStorage_Restore(&instance,&wrong,&diagnostic));CHECK(!memcmp(previous,bytes,sizeof(bytes)));
    }
    NativeScriptStorageSnapshot tiny={.bytes=saved,.capacity=83};CHECK(!nativeScriptStorage_Save(&instance,&tiny,&diagnostic));
    nativeScriptStorage_Destroy(&instance);CHECK(!nativeScriptStorage_Restore(&instance,&snapshot,&diagnostic));
    CHECK(nativeScriptStorage_Create(&layout,&provided,0,99,8,bytes,sizeof(bytes),&instance,&diagnostic));
    CHECK(read(&instance,value_id(0,"handle"),NULL).as.i64==0);CHECK(!nativeScriptStorage_Restore(&instance,&snapshot,&diagnostic));
    CHECK(nativeScriptStorage_Create(&layout,&other_parameters,0,99,7,bytes,sizeof(bytes),&instance,&diagnostic));
    CHECK(!nativeScriptStorage_Restore(&instance,&snapshot,&diagnostic));
}

static NativeScriptStorageInstance* active(NativeScriptRuntimeContext* context)
{ return context->user; }
static bool alive(NativeScriptRuntimeContext* context)
{ NativeScriptStorageInstance* storage=active(context);return storage&&storage->live&&storage->identity==context->actor_id&&storage->generation==context->generation; }
static bool host_read(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* error)
{ return nativeScriptStorage_Read(active(context),semantic,selectors,count,result,error); }
static bool host_write(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* error)
{ return nativeScriptStorage_Write(active(context),semantic,selectors,count,value,error); }

static void vm_tests(void)
{
    NativeScriptInstruction instructions[2048];NativeScriptFunctionCode code[32];NativeScriptLocal locals[64];
    memset(code,0,sizeof(code));
    NativeScriptProgramCompiler compiler={.index=&index,.locals=locals,.local_capacity=64};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=2048,.functions=code,.function_count=index.function_count};
    const uint32_t helpers[]={function_id(0,"ReadImported"),function_id(0,"Tick"),function_id(article_owner,"Tick")};
    for(unsigned i=0;i<index.function_count;++i) {
        NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic));
        CHECK(nativeScriptBytecode_Compile(&builder,&body,index.functions[i].source_node,i,(uint16_t)index.functions[i].parameter_count,&diagnostic));
    }
    NativeScriptBytecodeProgram program={source,tree.source_bytes,instructions,builder.instruction_count,code,index.function_count,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptValue registers[1024];NativeScriptVMFrame frames[16];NativeScriptVM vm={.registers=registers,.register_capacity=1024,.frames=frames,.frame_capacity=16};
    NativeScriptVMHost host={.source_alive=alive,.read=host_read,.write=host_write};
    uint8_t bytes[128],snapshot_bytes[128];NativeScriptStorageInstance instance;
    CHECK(nativeScriptStorage_Create(&layout,&parameters,0,77,3,bytes,sizeof(bytes),&instance,&diagnostic));
    NativeScriptRuntimeContext context={.user=&instance,.generation=3,.actor_id=77,.frame=123};NativeScriptValue result=i32(-7);
    CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,helpers[0],NULL,0,&result)==NativeScriptVM_Error);
    CHECK(result.type==NativeScriptType_I32&&result.as.i64==-7&&strstr(vm.diagnostic.message,"unresolved"));
    CHECK(nativeScriptStorage_Create(&layout,&provided,0,77,3,bytes,sizeof(bytes),&instance,&diagnostic));
    NativeScriptValue initial=i32(INT32_MIN+1);CHECK(nativeScriptStorage_Write(&instance,value_id(0,"handle"),NULL,0,&initial,&diagnostic));
    NativeScriptStorageSnapshot snapshot={.bytes=snapshot_bytes,.capacity=sizeof(snapshot_bytes)};
    CHECK(nativeScriptStorage_Save(&instance,&snapshot,&diagnostic));
    uint8_t endpoint[128];float reference=0;
    for(unsigned run=0;run<2;++run) {
        if(run)CHECK(nativeScriptStorage_Restore(&instance,&snapshot,&diagnostic));
        reference=0;
        for(unsigned tick=0;tick<64;++tick) {
            NativeScriptValue step=i32(1);context.frame=tick;
            CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,helpers[1],&step,1,&result)==NativeScriptVM_Ok);
            float sum=reference+1.5f;
            reference=(float)((double)sum*0.95+0.75f*0.05);
            CHECK(result.type==NativeScriptType_F32&&!memcmp(&result.as.f32,&reference,4));
            CHECK(vm.depth==0&&vm.register_top==0);++ticks;
        }
        if(!run)memcpy(endpoint,bytes,84);else CHECK(!memcmp(endpoint,bytes,84));
    }
    CHECK((uint32_t)read(&instance,value_id(0,"handle"),NULL).as.i64==UINT32_C(0x80000041));
    NativeScriptValue at=i32(2);CHECK((uint32_t)read(&instance,value_id(0,"histories"),&at).as.i64==UINT32_C(0x80000043));
    CHECK(read(&instance,value_id(0,"ready"),NULL).as.u64==1);
    NativeScriptValue false_index={.type=NativeScriptType_Bool,.as.u64=0};CHECK(read(&instance,value_id(0,"flags"),&false_index).as.u64==1);
    nativeScriptStorage_Destroy(&instance);CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,helpers[1],&initial,1,&result)==NativeScriptVM_SourceRetired);
    CHECK(nativeScriptStorage_Create(&layout,&provided,article_owner,88,4,bytes,sizeof(bytes),&instance,&diagnostic));context.actor_id=88;context.generation=4;
    NativeScriptValue step=f32(0.5f);CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,helpers[2],&step,1,&result)==NativeScriptVM_Ok&&result.as.f32==1.25f);
    CHECK(read(&instance,value_id(article_owner,"charge"),NULL).as.f32==1.25f);
}

static void rejected_layout_tests(void)
{
    NativeScriptValueDeclaration modified[64];memcpy(modified,declarations,sizeof(modified));NativeScriptProgramIndex invalid=index;invalid.values=modified;
    NativeScriptStorageField rejected_fields[64];NativeScriptStorageOwner rejected_owners[16];
    NativeScriptStorageLayout rejected={.fields=rejected_fields,.field_capacity=64,.owners=rejected_owners,.owner_capacity=16};
    modified[1].byte_offset=0;CHECK(!nativeScriptStorage_BuildLayout(&rejected,&invalid,&diagnostic)&&rejected.field_count==0);
    memcpy(modified,declarations,sizeof(modified));modified[1].byte_offset=UINT32_MAX-3;modified[1].count=2;modified[1].array=true;
    CHECK(!nativeScriptStorage_BuildLayout(&rejected,&invalid,&diagnostic));
    memcpy(modified,declarations,sizeof(modified));modified[0].type=NativeScriptType_F64;CHECK(!nativeScriptStorage_BuildLayout(&rejected,&invalid,&diagnostic));
    memcpy(modified,declarations,sizeof(modified));modified[0].owner=UINT32_MAX;CHECK(!nativeScriptStorage_BuildLayout(&rejected,&invalid,&diagnostic));
}

static void empty_owner_tests(void)
{
    const char* text="article Empty states 1:\n    state 0 anim -1:\nitem Plain damage 1 angle 45 lifetime 0:\n    hitbox_size 1\nmodel_effect Glow:\n    local_id 0\n";
    NativeScriptSourceNode syntax_nodes[64];NativeScriptIRNode expression_nodes[64];uint32_t argument_nodes[64];
    NativeScriptFunctionDeclaration fs[4];NativeScriptParameterDeclaration ps[4];NativeScriptValueDeclaration vs[4];
    NativeScriptSourceTree source_tree={text,(uint32_t)strlen(text),syntax_nodes,0,64};
    NativeScriptIRArena expressions={text,(uint32_t)strlen(text),expression_nodes,0,64,argument_nodes,0,64};
    NativeScriptProgramIndex program={.tree=&source_tree,.expressions=&expressions,.functions=fs,.function_capacity=4,.parameters=ps,.parameter_capacity=4,.values=vs,.value_capacity=4};
    CHECK(nativeScriptSource_Parse(&source_tree,&diagnostic));CHECK(nativeScriptProgram_Index(&program,&diagnostic));
    CHECK(program.function_count==0&&program.value_count==0);
    NativeScriptStorageOwner owner_rows[4];NativeScriptStorageLayout empty={.owners=owner_rows,.owner_capacity=4};
    CHECK(nativeScriptStorage_BuildLayout(&empty,&program,&diagnostic));CHECK(empty.owner_count==4&&empty.field_count==0&&empty.identity);
    NativeScriptStorageParameters immutable;CHECK(nativeScriptStorage_PrepareParameters(&empty,NULL,0,NULL,0,&immutable,&diagnostic));CHECK(immutable.count==0&&immutable.identity);
    NativeScriptStorageInstance instance,other;NativeScriptStorageSnapshot snapshot={0};
    for(uint32_t i=0;i<empty.owner_count;++i){
        const NativeScriptStorageOwner* owner=&owner_rows[i];CHECK(owner->state_bytes==0&&owner->parameter_count==0);
        CHECK(nativeScriptStorage_ParametersReady(&empty,&immutable,owner->owner,&diagnostic));
        CHECK(nativeScriptStorage_Create(&empty,&immutable,owner->owner,7,9,NULL,0,&instance,&diagnostic));
        CHECK(nativeScriptStorage_Save(&instance,&snapshot,&diagnostic)&&snapshot.state_bytes==0&&snapshot.owner==owner->owner);
        CHECK(nativeScriptStorage_Restore(&instance,&snapshot,&diagnostic));
        uint32_t other_owner=owner_rows[(i+1)%empty.owner_count].owner;CHECK(nativeScriptStorage_Create(&empty,&immutable,other_owner,7,9,NULL,0,&other,&diagnostic));
        CHECK(!nativeScriptStorage_Restore(&other,&snapshot,&diagnostic));nativeScriptStorage_Destroy(&instance);
        CHECK(!nativeScriptStorage_Save(&instance,&snapshot,&diagnostic));
    }
    NativeScriptStorageLayout exhausted={.owners=owner_rows,.owner_capacity=3};
    CHECK(!nativeScriptStorage_BuildLayout(&exhausted,&program,&diagnostic)&&!exhausted.owner_count&&!exhausted.field_count&&!exhausted.identity);
    CHECK(!nativeScriptStorage_Owner(&empty,UINT32_MAX));
}

int main(int argc,char** argv)
{
    CHECK(argc==2);initialize(argv[1]);parameter_tests();field_tests();snapshot_tests();vm_tests();rejected_layout_tests();empty_owner_tests();
    printf("{\"checks\":%u,\"vm_ticks\":%u,\"declarations\":%u,\"owners\":%u,\"status\":\"PASS owned typed storage and VM snapshot replay\"}\n",checks,ticks,index.value_count,layout.owner_count);
    return 0;
}
