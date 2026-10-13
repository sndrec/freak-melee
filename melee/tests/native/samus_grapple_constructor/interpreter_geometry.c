#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_receiver_semantics.h>
#include <melee/ft/native_script_storage.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/it/types.h>
#include <melee/it/itarticle_data.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// This is the exact arithmetic slice preceding native link allocation, not a
// substitute constructor. The runner extracts and hashes it from live source.
#include "native_geometry.inc"

static NativeSourceBank source_bank;
static NativeScriptStorageInstance storage;
static NativeScriptStorageParameters parameters;
static NativeScriptDiagnostic diagnostic;
static NativeScriptValue registers[4096];
static NativeScriptVMFrame frames[32];
static unsigned checks, comparisons, safety_cases;
static uint8_t state[512];
static float scale;
static int motion;
static itSamusGrappleAttributes attributes;
static NativeScriptParameterSupply supplied[128];
static uint32_t supply_count, owner, geometry_function;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"geometry line %u: %s (%s)\n",__LINE__,#x,diagnostic.message); exit(1); } } while (0)

static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
    (void)unused;
    const NativeScriptSourceNode* node=&tree->nodes[at];
    if(node->kind==NativeScriptSource_Article)
        return nativeScriptSource_SpanEquals(tree,node->value,"states 9");
    if(node->parent<tree->count && tree->nodes[node->parent].kind==NativeScriptSource_Article &&
       (node->kind==NativeScriptSource_Function || node->kind==NativeScriptSource_State ||
        node->kind==NativeScriptSource_Attribute))return true;
    return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);
}
static bool import_parameters(void* unused,const NativeScriptProgramIndex* program,
    const NativeScriptParameterSupply** values,uint32_t* count,NativeScriptDiagnostic* d)
{
    (void)unused;(void)d;supply_count=0;
    for(uint32_t i=0;i<program->value_count;++i) {
        const NativeScriptValueDeclaration* v=&program->values[i];
        if(!v->imported)continue;
        CHECK(supply_count<128 && v->import_offset+4<=sizeof(attributes));
        NativeScriptValue value={.type=v->type};
        if(v->type==NativeScriptType_I32) {
            int32_t number;memcpy(&number,(uint8_t*)&attributes+v->import_offset,4);value.as.i64=number;
        } else { CHECK(v->type==NativeScriptType_F32);memcpy(&value.as.f32,(uint8_t*)&attributes+v->import_offset,4); }
        supplied[supply_count++]=(NativeScriptParameterSupply){.semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,.owner=v->owner,.value=value};
    }
    *values=supplied;*count=supply_count;return true;
}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c->user==&storage && storage.live;}
static bool read_value(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)c;
    uint32_t domain=semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK;
    if(domain==NATIVE_SCRIPT_RECEIVER_ROOT) {
        CHECK(count==0);
        *out=(NativeScriptValue){.type=NativeScriptType_Object,.as.object={42,1,1}};return true;
    }
    if(domain==NATIVE_SCRIPT_RECEIVER_NATIVE) {
        CHECK(count==1 && selectors[0].type==NativeScriptType_Object && selectors[0].as.object.identity==42);
        uint32_t id=semantic&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK;
        if(id==NativeScriptBinding_ModelInstanceScaleY){*out=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=scale};return true;}
        if(id==NativeScriptBinding_MotionID){*out=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=motion};return true;}
        CHECK(false);return false;
    }
    return nativeScriptStorage_Read(&storage,semantic,selectors,count,out,d);
}
static bool write_value(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,const NativeScriptValue* value,NativeScriptDiagnostic* d)
{(void)c;return nativeScriptStorage_Write(&storage,semantic,selectors,count,value,d);}
static bool validate(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* selectors,
    uint32_t count,NativeScriptDiagnostic* d)
{NativeScriptValue value;return read_value(c,semantic,selectors,count,&value,d);}
static uint32_t field(const char* name)
{
    const NativeScriptCompileBank* b=&source_bank.bank;
    for(uint32_t i=0;i<b->declarations.value_count;++i)
        if(!b->declarations.values[i].parameter && nativeScriptSource_SpanEquals(&b->tree,b->declarations.values[i].name,name))return i;
    CHECK(false);return 0;
}
static void set_mode(int mode)
{NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=mode};CHECK(nativeScriptStorage_Write(&storage,NATIVE_SCRIPT_STATE_SEMANTIC|field("modified_mode"),NULL,0,&v,&diagnostic));}
static NativeScriptValue get(const char* name)
{NativeScriptValue v;CHECK(nativeScriptStorage_Read(&storage,NATIVE_SCRIPT_STATE_SEMANTIC|field(name),NULL,0,&v,&diagnostic));return v;}
static bool run_geometry(int mode)
{
    const NativeScriptCompileBank* b=&source_bank.bank;
    const NativeScriptParameterSupply* values;uint32_t count;
    CHECK(import_parameters(NULL,&b->declarations,&values,&count,&diagnostic));
    CHECK(nativeScriptStorage_PrepareParameters(&b->storage_layout,b->parameter_slots,b->parameter_slot_capacity,values,count,&parameters,&diagnostic));
    CHECK(nativeScriptStorage_Create(&b->storage_layout,&parameters,owner,7,1,state,sizeof(state),&storage,&diagnostic));
    set_mode(mode);
    NativeScriptVM vm={.registers=registers,.register_capacity=4096,.frames=frames,.frame_capacity=32};
    const NativeScriptVMHost host={.source_alive=nativeScriptBindings_SourceAlive,.read=read_value,.write=write_value,.validate=validate,.invoke=nativeScriptBindings_Invoke};
    NativeScriptRuntimeContext context={.user=&storage,.source_class=NativeCallbackSource_Item};NativeScriptValue result;
    NativeScriptVMStatus status=nativeScriptVM_Run(&vm,&b->code,&host,&context,geometry_function,NULL,0,&result);
    if(status!=NativeScriptVM_Ok)diagnostic=vm.diagnostic;
    CHECK(status==NativeScriptVM_Ok && result.type==NativeScriptType_Bool);
    return result.as.u64!=0;
}
static void compare(const char* name,float expected)
{NativeScriptValue v=get(name);CHECK(v.type==NativeScriptType_F32);CHECK(fabsf(v.as.f32-expected)<=0.00001f*fmaxf(1,fabsf(expected)));}
int main(int argc,char** argv)
{
    CHECK(argc==2);FILE* file=fopen(argv[1],"rb");CHECK(file);CHECK(fseek(file,0,SEEK_END)==0);long size=ftell(file);CHECK(size>0);
    char* source=malloc((size_t)size+1);CHECK(source);rewind(file);CHECK(fread(source,1,(size_t)size,file)==(size_t)size);fclose(file);source[size]=0;
    attributes=(itSamusGrappleAttributes){.x8=.5f,.xC=30,.x10=1,.x14=.2f,.x18=1.25f,.x1C=.04f,.x20=.7f,.x24=.6f,.x28=.3f,.x2C=.8f,.x30=.09f,.x5C=1,.x60=1.5f};
    CHECK(nativeSourceBank_Allocate(&source_bank,&diagnostic));
    const NativeScriptCompileBank* accepted=NULL;
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)size,.metadata=metadata,.parameter_provider=import_parameters};
    CHECK(nativeScriptCompile_Build(&source_bank.bank,&request,&accepted,&diagnostic));
    for(uint32_t i=0;i<accepted->declarations.function_count;++i)
        if(nativeScriptSource_SpanEquals(&accepted->tree,accepted->declarations.functions[i].name,"InitializeGeometry")) {
            geometry_function=i;owner=accepted->declarations.functions[i].owner;
        }
    CHECK(owner!=0);
    const int motions[]={212,214,357,358};const float scales[]={.5f,1,1.25f,1.75f,2.5f};
    for(unsigned m=0;m<4;++m)for(unsigned s=0;s<5;++s)for(int mode=0;mode<3;++mode)
        for(unsigned variant=0;variant<8;++variant) {
            motion=motions[m];scale=scales[s];attributes.xC=12+(int)variant*2;attributes.x8=.125f*variant;
            attributes.x10=.75f+.125f*variant;attributes.x5C=.5f+.0625f*variant;attributes.x60=1+.0625f*variant;
            itSamusGrappleAttributes original=attributes;Article article={.x4_specialAttributes=&original};Item item={.xC4_article_data=&article};item.xDD4_itemVar.samusgrapple.x16=mode;
            native_geometry(&item,motion==357,scale);
            CHECK(original.x34>=2 && original.x34<=90);CHECK(run_geometry(mode));
            CHECK(get("count").as.i64==original.x34);CHECK(get("endpoint").as.i64==original.x34-1);
            compare("length",original.x38);compare("min_length",original.x3C);compare("launch_speed",original.x40);compare("gravity",original.x44);
            compare("launch_auxiliary",original.x48);compare("retract_speed",original.x4C);compare("retract_auxiliary",original.x50);compare("pull_speed",original.x54);compare("friction",original.x58);
            ++comparisons;
        }
    // The native s32/byte conversion has no defined guarantee for malformed
    // geometry. Only the authored rejection policy is tested on these cases.
    attributes.xC=30;attributes.x10=1;attributes.x5C=1;attributes.x60=1.5f;attributes.x8=.5f;motion=357;
    const float bad_scales[]={0,NAN,INFINITY};
    for(unsigned i=0;i<3;++i){scale=bad_scales[i];CHECK(!run_geometry(0));++safety_cases;}
    scale=1;attributes.xC=1;CHECK(!run_geometry(0));++safety_cases;
    attributes.xC=100;CHECK(!run_geometry(0));++safety_cases;
    attributes.xC=31;CHECK(!run_geometry(1));++safety_cases;
    free(source);nativeSourceBank_Release(&source_bank);
    printf("{\"checks\":%u,\"geometry_comparisons\":%u,\"safety_cases\":%u}\n",checks,comparisons,safety_cases);return 0;
}
