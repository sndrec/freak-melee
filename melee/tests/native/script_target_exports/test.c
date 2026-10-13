#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_targets.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum { N=4096,I=16384,F=128,V=256 };
typedef struct Buffers {
    char source[32768];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
    NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
    NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];
    NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
    NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} Buffers;
NativeSourcePackage target_packages[3];static Buffers buffers[4];static NativeScriptCompileBank scratch;
static Article article;static ItemAttr attributes;static ItemModelDesc model;
static Fighter fighters[128],alternate;static Item items[64];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static unsigned checks,cases,vm_calls,fighter_count,item_count,probe_mode,probe_calls;
static HSD_GObj *caller,*recipient,*replacement;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"exports %u: %s (%s)\n",__LINE__,#x,diagnostic.message);fprintf(stderr,"diagnostic at %u:%u offset%u\n",diagnostic.source.line,diagnostic.source.column,diagnostic.source.offset);exit(1);}}while(0)
static void configure(NativeScriptCompileBank* bank,Buffers* b)
{
    *bank=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=sizeof(b->source),.tree={.nodes=b->nodes,.capacity=N},
        .expressions={.nodes=b->ir,.node_capacity=I,.arguments=b->arguments,.argument_capacity=I},
        .declarations={.functions=b->functions,.function_capacity=F,.parameters=b->parameters,.parameter_capacity=V,.values=b->values,.value_capacity=V},
        .locals=b->locals,.local_capacity=V,.instructions=b->instructions,.instruction_capacity=I,.function_code=b->codes,.function_code_capacity=F,
        .execution_functions=b->entries,.execution_function_capacity=F,.graph=b->graph,.graph_capacity=F,
        .storage_layout={.fields=b->fields,.field_capacity=V,.owners=b->owners,.owner_capacity=V},.parameter_slots=b->slots,.parameter_slot_capacity=V};
}
static void prepare(unsigned n)
{
    static char source[32768];NativeSourcePackage* p=&target_packages[n];NativeScriptCompileBank* bank=&p->compiler.bank;
    configure(bank,&buffers[n]);
    int length=snprintf(source,sizeof(source),
        "fighter:\n    behavior independent\nstate int handle\nstate int result\nstate int tail\nstate int visits\nstate int zero\nstate bool boolean\nstate float scalar\n"
        "%s"
        "export func Mixed(int count, float scale, bool enabled, uint mask) -> float:\n    state.visits = state.visits + 1\n    if enabled:\n        return float(count) * scale + float(mask)\n    return -scale\nexport func Identity(int number) -> int:\n    return number\nexport func Flag(bool enabled) -> bool:\n    return enabled\nexport func SetValue(float value):\n    state.scalar = value\nfunc InvokeMixed():\n    state.scalar = targets.call_float(state.handle, \"Mixed\", -7, 1.25, true, uint(15))\nfunc InvokeIdentity():\n    state.result = targets.call_int(state.handle, \"Identity\", uint(4294967295))\nfunc InvokeFlag():\n    state.boolean = targets.call_bool(state.handle, \"Flag\", true)\nfunc InvokeSetValue():\n    state.boolean = targets.call(state.handle, \"SetValue\", 17)\n"
        "export func Many(int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9, int x10, int x11, int x12, int x13, int x14, int x15, int x16, int x17, int x18, int x19, int x20, int x21, int x22, int x23, int x24, int x25, int x26, int x27, int x28, int x29) -> int:\n    return x0 + x29\nfunc InvokeMany():\n    state.result = targets.call_int(state.handle, \"Many\", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29)\n"
        "export func Shared() -> int:\n    state.visits = state.visits + 1\n    return %u\n"
        "export func Boolean() -> bool:\n    return %s\n"
        "export func Scalar() -> float:\n    return %s\n"
        "export func costarring() -> int:\n    return 401\n"
        "export func liquid() -> int:\n    return 402\n"
        "export func Empty():\n    state.visits = state.visits + 1\n"
        "func Private() -> int:\n    return 999\n"
        "export func Notify(float value) -> int:\n    position.x = value\n    return 77\n"
        "export func Fault() -> int:\n    return 1 / state.zero\n"
        "export func Nested() -> int:\n    return targets.call_int(state.handle, \"Shared\")\n"
        "export func Cycle() -> int:\n    return targets.call_int(state.handle, \"Cycle\")\n"
        "func InvokeInt():\n    state.result = targets.call_int(state.handle, \"Shared\")\n    state.tail = state.tail + 1\n"
        "func InvokeBool():\n    state.boolean = targets.call_bool(state.handle, \"Boolean\")\n    state.tail = state.tail + 1\n"
        "func InvokeFloat():\n    state.scalar = targets.call_float(state.handle, \"Scalar\")\n    state.tail = state.tail + 1\n"
        "func InvokeCall():\n    state.boolean = targets.call(state.handle, \"Empty\")\n    state.tail = state.tail + 1\n"
        "func InvokeDiscard():\n    state.boolean = targets.call(state.handle, \"Shared\")\n"
        "func InvokeCollisionA():\n    state.result = targets.call_int(state.handle, \"costarring\")\n"
        "func InvokeCollisionB():\n    state.result = targets.call_int(state.handle, \"liquid\")\n"
        "func InvokeRepeated():\n    state.result = targets.call_int(state.handle, \"Shared\") + targets.call_int(state.handle, \"Shared\")\n"
        "func InvokeMissing():\n    state.result = targets.call_int(state.handle, \"Absent\")\n"
        "func InvokePrivate():\n    state.result = targets.call_int(state.handle, \"Private\")\n"
        "func InvokeWrong():\n    state.result = targets.call_int(state.handle, \"Boolean\")\n"
        "func InvokeWrongBool():\n    state.boolean = targets.call_bool(state.handle, \"Shared\")\n"
        "func InvokeWrongFloat():\n    state.scalar = targets.call_float(state.handle, \"Shared\")\n"
        "func InvokeNested():\n    state.result = targets.call_int(state.handle, \"Nested\")\n"
        "func InvokeCycle():\n    state.result = targets.call_int(state.handle, \"Cycle\")\n    state.tail = state.tail + 1\n"
        "func InvokeNotify():\n    state.result = targets.call_int(state.handle, \"Notify\", 1.0)\n    state.tail = state.tail + 1\n"
        "func InvokeNotifyCall():\n    state.boolean = targets.call(state.handle, \"Notify\", 1.0)\n    state.tail = state.tail + 1\n"
        "func InvokeFault():\n    state.result = targets.call_int(state.handle, \"Fault\")\n    state.tail = state.tail + 1\n"
        "action Placeholder:\n    on tick:\n        state.tail = state.tail + 1\n"
        "article Probe states 1:\n    state int handle\n    state int result\n    state int tail\n    state bool boolean\n    state float scalar\n"
        "    on signal Mixed:\n        item.state.scalar = targets.call_float(item.state.handle, \"Mixed\", -7, 1.25, true, uint(15))\n    on signal Identity:\n        item.state.result = targets.call_int(item.state.handle, \"Identity\", uint(4294967295))\n    on signal Flag:\n        item.state.boolean = targets.call_bool(item.state.handle, \"Flag\", true)\n    on signal SetValue:\n        item.state.boolean = targets.call(item.state.handle, \"SetValue\", 17)\n"
        "    on signal Many:\n        item.state.result = targets.call_int(item.state.handle, \"Many\", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29)\n"
        "    on signal Int:\n        item.state.result = targets.call_int(item.state.handle, \"Shared\")\n        item.state.tail = item.state.tail + 1\n"
        "    on signal Bool:\n        item.state.boolean = targets.call_bool(item.state.handle, \"Boolean\")\n"
        "    on signal Float:\n        item.state.scalar = targets.call_float(item.state.handle, \"Scalar\")\n"
        "    on signal Call:\n        item.state.boolean = targets.call(item.state.handle, \"Empty\")\n"
        "    on signal Notify:\n        item.state.result = targets.call_int(item.state.handle, \"Notify\", 1.0)\n        item.state.tail = item.state.tail + 1\n"
        "    on signal NotifyCall:\n        item.state.boolean = targets.call(item.state.handle, \"Notify\", 1.0)\n        item.state.tail = item.state.tail + 1\n"
        "    on signal LocalFault:\n        item.state.result = 1 / item.state.tail\n"
        "    on signal Fault:\n        item.state.result = targets.call_int(item.state.handle, \"Fault\")\n        item.state.tail = item.state.tail + 1\n"
        "    state 0 anim -1:\n",
        n==0?"":n==1?"func PaddingA():\n    state.tail = state.tail + 0\n":"func PaddingA():\n    state.tail = state.tail + 0\nfunc PaddingB():\n    state.tail = state.tail + 0\n",(n+1)*111,n==1?"false":"true",n==0?"-0.0":n==1?"2.5":"-3.75");
    CHECK(length>0&&(size_t)length<sizeof(source));
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)length,.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(bank,&request,&accepted,&diagnostic));
    CHECK(accepted==bank);CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst+n,bank,&layout,&metadata,&p->program,&diagnostic));
    p->address=p;p->target=Ft_Kind_CustomFirst+n;p->ready=p->assets.ready=true;p->assets.facts.moves=layout;p->assets.parts.parts_num=4;p->assets.facts.joints=4;
    tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=p->assets.motions;
    nativeScriptFighter_PublishProgram(p->target,&p->program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(unsigned bank,uint32_t id)
{
    CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
    Fighter* f=&fighters[fighter_count++];NativeSourcePackage* p=&target_packages[bank];
    *f=(Fighter){.gobj=o,.kind=p->target,.behavior_kind=p->target,.x8_spawnNum=id,.ft_data=&p->assets.character.data};
    GObj_InitUserData(o,4,fighter_remove,f);CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(unsigned bank)
{
    CHECK(item_count<64);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(target_packages[bank].target)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(o);Item* i=&items[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=1000+item_count,.xC4_article_data=&article};
    GObj_InitUserData(o,6,item_remove,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?(uint32_t)((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    const NativeScriptProgramIndex* index=view.program;
    for(unsigned n=0;n<index->value_count;++n)if(index->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(index->tree,index->values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));
        CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static void set(HSD_GObj* o,const char* name,int32_t value){NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=value};field(o,name,&v);}
static uint32_t function(unsigned bank,const char* name)
{
    NativeScriptProgramIndex* p=&target_packages[bank].compiler.bank.declarations;
    for(unsigned n=0;n<p->function_count;++n)if(p->functions[n].owner==0&&nativeScriptSource_SpanEquals(p->tree,p->functions[n].name,name))return n;
    CHECK(false);return UINT32_MAX;
}
static uint8_t* state_address(HSD_GObj* o,const char* name)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?(uint32_t)((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    const NativeScriptProgramIndex* p=view.program;
    for(unsigned n=0;n<p->value_count;++n)if(p->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(p->tree,p->values[n].name,name)){
        CHECK(view.storage->layout->fields[n].type==NativeScriptType_I32);return view.storage->state+view.storage->layout->fields[n].offset;
    }CHECK(false);return NULL;
}
static int32_t observed_i32(const uint8_t* bytes){int32_t value;memcpy(&value,bytes,sizeof(value));return value;}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static NativeScriptVMStatus invoke(HSD_GObj* o,const char* name,NativeScriptValue* out)
{
    ++vm_calls;NativeScriptVMStatus status;
    if(o->classifier==HSD_GOBJ_CLASS_ITEM)status=nativeScriptItem_RunSignal(o,hash(name),0,out);
    else {Fighter* f=o->user_data;status=nativeScriptFighter_Helper(o,(uint32_t)f->x8_spawnNum,function(f->kind-Ft_Kind_CustomFirst,name),NULL,0,out,&diagnostic);}
    CHECK(nativeScriptRuntime_Idle());return status;
}
void target_guard_probe(NativeScriptRuntimeContext* c){(void)c;}
void target_write_probe(NativeScriptRuntimeContext* c)
{
    if(c->source!=recipient||!probe_mode)return;unsigned mode=probe_mode;probe_mode=0;++probe_calls;
    if(mode==1)HSD_GObjFree(recipient);
    if(mode==2){HSD_GObjFree(recipient);replacement=fighter(2,UINT32_C(0x80000005));}
    if(mode==3)HSD_GObjFree(caller);
    if(mode==4){NativeScriptValue output;NativeScriptVMStatus status;if(caller->classifier==HSD_GOBJ_CLASS_ITEM)status=nativeScriptItem_RunSignal(caller,hash("LocalFault"),0,&output);else{Fighter* f=caller->user_data;status=nativeScriptFighter_Helper(caller,(uint32_t)f->x8_spawnNum,function(0,"Fault"),NULL,0,&output,&diagnostic);}CHECK(status==NativeScriptVM_Error);}
    if(mode==5)((Fighter*)recipient->user_data)->x8_spawnNum++;
    if(mode==6){alternate=*(Fighter*)recipient->user_data;recipient->user_data=&alternate;}
    if(mode==7)recipient->classifier=HSD_GOBJ_CLASS_ITEM;
    if(mode==8)nativeScriptFighter_Detach(recipient);
}
static uint32_t checksum(const void* data,size_t size)
{const unsigned char* p=data;uint32_t h=2166136261u;while(size--){h^=*p++;h*=16777619u;}return h;}
static void compile_rejections(void)
{
    const char* bad[]={
        "export func Bad(int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8, int x9, int x10, int x11, int x12, int x13, int x14, int x15, int x16, int x17, int x18, int x19, int x20, int x21, int x22, int x23, int x24, int x25, int x26, int x27, int x28, int x29, int x30) -> int:\n    return x0\n",
        "export func Bad() -> uint:\n    return 1\n",
        "export func Bad() -> double:\n    return 1.0\n",
        "export func Bad() -> bool:\n    return true\nexport func Bad() -> bool:\n    return false\n",
        "article Probe states 1:\n    export func Bad() -> int:\n        return 1\n    state 0 anim -1:\n",
        "func Bad():\n    let bool delivered = targets.call(1, \"\")\n",
        "func Bad():\n    let bool delivered = targets.call(1, \"not a name\")\n",
        "func Bad():\n    let bool delivered = targets.call(1.25, \"Shared\")\n",
        "func Bad():\n    let bool delivered = targets.call(1, 2)\n",
        "export func AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA() -> int:\n    return 1\n"};
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
    uint32_t original=checksum(&buffers[0],sizeof(buffers[0]));
    for(unsigned n=0;n<sizeof(bad)/sizeof(*bad);++n){configure(&scratch,&buffers[3]);static char source[1024];snprintf(source,sizeof(source),"fighter:\n    behavior independent\n%s",bad[n]);
        NativeScriptCompileRequest r={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=&target_packages[0].compiler.bank;
        CHECK(!nativeScriptCompile_Build(&scratch,&r,&accepted,&diagnostic));CHECK(accepted==&target_packages[0].compiler.bank);CHECK(checksum(&buffers[0],sizeof(buffers[0]))==original);
        if(n==0||n==1||n==4)CHECK(strstr(diagnostic.message,"Export must be a root helper")!=NULL);
        if(n==2)CHECK(strstr(diagnostic.message,"Unknown helper return type")!=NULL);
        if(n==3)CHECK(strstr(diagnostic.message,"Duplicate")!=NULL);++cases;}
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?(uint32_t)((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
 return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};}
static uint32_t call_arguments(HSD_GObj* object,const char* name,unsigned operation,NativeScriptValue args[2])
{
    unsigned kind=object->classifier==HSD_GOBJ_CLASS_FIGHTER?((Fighter*)object->user_data)->kind:((Item*)object->user_data)->native_article_author_kind;
    NativeScriptProgramIndex* p=&target_packages[kind-Ft_Kind_CustomFirst].compiler.bank.declarations;
    for(unsigned n=0;n<p->export_name_count;++n)if(nativeScriptSource_SpanEquals(p->tree,p->export_names[n].name,name)){
        args[0]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=UINT32_C(0x80000005)};
        args[1]=(NativeScriptValue){.type=NativeScriptType_Text,.as.text={p->export_names[n].name.offset,p->export_names[n].name.length}};
        return NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN|(operation<<20)|(2u<<NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT)|n;
    }CHECK(false);return 0;
}
static void direct_negatives(HSD_GObj* from,HSD_GObj* target)
{
    NativeScriptRuntimeContext c=context(from);NativeScriptValue args[2],out;uint32_t semantic=call_arguments(from,"Shared",3,args);
    CHECK(nativeScriptBindingTargets_Invoke(&c,semantic,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_I32&&out.as.i64==222);++cases;
    const NativeScriptValue handles[]={{.type=NativeScriptType_I32,.as.i64=-2147483643},{.type=NativeScriptType_U64,.as.u64=UINT32_C(0x80000005)}};
    for(unsigned n=0;n<2;++n){args[0]=handles[n];CHECK(nativeScriptBindingTargets_Invoke(&c,semantic,args,2,&out,&diagnostic));CHECK(out.as.i64==222);++cases;}
    for(unsigned mode=0;mode<8;++mode){semantic=call_arguments(from,"Shared",3,args);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        if(mode==0)args[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000005)};
        if(mode==1)args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=5};
        if(mode==2)args[1].type=NativeScriptType_I32;
        if(mode==3)args[1].as.text.offset=UINT32_MAX;
        if(mode==4)args[1].as.text.length--;
        if(mode==5)semantic=NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN|(3u<<20)|(2u<<NATIVE_SCRIPT_TARGET_EXPORT_ARGUMENT_SHIFT)|NATIVE_SCRIPT_EXPORT_NAMES;
        if(mode==6)semantic=NATIVE_SCRIPT_TARGET_EXPORT_DOMAIN;
        CHECK(!nativeScriptBindingTargets_Invoke(&c,semantic,args,mode==7?1:2,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);CHECK(nativeScriptRuntime_Idle());++cases;
    }
    NativeScriptProgramIndex* p=&target_packages[((Fighter*)target->user_data)->kind-Ft_Kind_CustomFirst].compiler.bank.declarations;
    unsigned entry=0;while(entry<p->export_count&&!nativeScriptSource_SpanEquals(p->tree,p->exports[entry].name,"Shared"))++entry;CHECK(entry<p->export_count);
    for(unsigned mode=0;mode<3;++mode){NativeScriptExport old=p->exports[entry];uint32_t count=p->export_count;
        if(mode==0)p->export_count=NATIVE_SCRIPT_EXPORTS+1;if(mode==1)p->exports[entry].function=p->function_count;
        if(mode==2)p->exports[entry].result_type=NativeScriptType_F64;
        semantic=call_arguments(from,"Shared",3,args);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingTargets_Invoke(&c,semantic,args,2,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
        p->export_count=count;p->exports[entry]=old;CHECK(nativeScriptRuntime_Idle());++cases;
    }
}
#include "argument_cases.h"
int main(void)
{
    for(unsigned n=0;n<3;++n)prepare(n);CHECK(function(0,"Shared")!=function(1,"Shared")&&function(1,"Shared")!=function(2,"Shared"));compile_rejections();
    HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
    HSD_GObj* a=fighter(0,21),*b=fighter(1,UINT32_C(0x80000005)),*third=fighter(2,UINT32_MAX),*article_caller=item(0);
    NativeScriptValue output;direct_negatives(a,b);direct_negatives(article_caller,b);
    argument_cases(a,article_caller,b);
    for(unsigned type=0;type<2;++type){HSD_GObj* from=type?article_caller:a;set(from,"handle",(int32_t)UINT32_C(0x80000005));
        CHECK(invoke(from,type?"Int":"InvokeInt",&output)==NativeScriptVM_Ok);CHECK(field(from,"result",NULL).as.i64==222);++cases;
        CHECK(invoke(from,type?"Bool":"InvokeBool",&output)==NativeScriptVM_Ok);CHECK(!field(from,"boolean",NULL).as.u64);++cases;
        CHECK(invoke(from,type?"Float":"InvokeFloat",&output)==NativeScriptVM_Ok);CHECK(field(from,"scalar",NULL).as.f32==2.5f);++cases;
        CHECK(invoke(from,type?"Call":"InvokeCall",&output)==NativeScriptVM_Ok);CHECK(field(from,"boolean",NULL).as.u64==1);++cases;
        set(from,"handle",12345);CHECK(invoke(from,type?"Int":"InvokeInt",&output)==NativeScriptVM_Ok);CHECK(field(from,"result",NULL).as.i64==0);++cases;
        set(from,"handle",-1);CHECK(invoke(from,type?"Int":"InvokeInt",&output)==NativeScriptVM_Ok);CHECK(field(from,"result",NULL).as.i64==333);++cases;
    }
    set(a,"handle",21);CHECK(invoke(a,"InvokeInt",&output)==NativeScriptVM_Ok);CHECK(field(a,"result",NULL).as.i64==111);++cases;
    CHECK(invoke(a,"InvokeFloat",&output)==NativeScriptVM_Ok);float zero=field(a,"scalar",NULL).as.f32;uint32_t bits;memcpy(&bits,&zero,4);CHECK(bits==UINT32_C(0x80000000));++cases;
    set(a,"handle",(int32_t)UINT32_C(0x80000005));
    CHECK(invoke(a,"InvokeCollisionA",&output)==NativeScriptVM_Ok);CHECK(field(a,"result",NULL).as.i64==401);++cases;
    CHECK(invoke(a,"InvokeCollisionB",&output)==NativeScriptVM_Ok);CHECK(field(a,"result",NULL).as.i64==402);++cases;
    CHECK(invoke(a,"InvokeRepeated",&output)==NativeScriptVM_Ok);CHECK(field(a,"result",NULL).as.i64==444);++cases;
    const char* defaults[]={"InvokeMissing","InvokePrivate","InvokeWrong"};for(unsigned n=0;n<3;++n){set(a,"result",99);CHECK(invoke(a,defaults[n],&output)==NativeScriptVM_Ok);CHECK(!field(a,"result",NULL).as.i64);++cases;}
    CHECK(invoke(a,"InvokeWrongBool",&output)==NativeScriptVM_Ok);CHECK(!field(a,"boolean",NULL).as.u64);++cases;
    CHECK(invoke(a,"InvokeWrongFloat",&output)==NativeScriptVM_Ok);CHECK(field(a,"scalar",NULL).as.f32==0.0f);++cases;
    CHECK(invoke(a,"InvokeDiscard",&output)==NativeScriptVM_Ok);CHECK(field(a,"boolean",NULL).as.u64);++cases;
    set(b,"handle",-1);CHECK(invoke(a,"InvokeNested",&output)==NativeScriptVM_Ok);CHECK(field(a,"result",NULL).as.i64==333);++cases;
    CHECK(invoke(a,"InvokeNotify",&output)==NativeScriptVM_Ok);CHECK(field(a,"result",NULL).as.i64==77&&((Fighter*)b->user_data)->cur_pos.x==1.0f);++cases;
    HSD_GObj* faulty=fighter(1,678);output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(invoke(faulty,"Fault",&output)==NativeScriptVM_Error);
    NativeScriptRuntimeContext c=context(a);NativeScriptValue args[2];uint32_t semantic=call_arguments(a,"Shared",3,args);args[0].as.u64=678;
    CHECK(!nativeScriptBindingTargets_Invoke(&c,semantic,args,2,&output,&diagnostic));CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++cases;
    c=context(faulty);semantic=call_arguments(faulty,"Shared",3,args);CHECK(!nativeScriptBindingTargets_Invoke(&c,semantic,args,2,&output,&diagnostic));CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++cases;
    HSD_GObjFree(faulty);HSD_GObjFree(a);HSD_GObjFree(b);HSD_GObjFree(third);HSD_GObjFree(article_caller);
    for(unsigned type=0;type<2;++type)for(unsigned mode=1;mode<=8;++mode){
        caller=type?item(0):fighter(0,40+fighter_count);recipient=fighter(1,UINT32_C(0x80000005));replacement=NULL;
        set(caller,"handle",(int32_t)UINT32_C(0x80000005));set(caller,"result",999);uint8_t* retained_tail=state_address(caller,"tail");uint8_t* retained_result=state_address(caller,"result");probe_mode=mode;
        output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};NativeScriptVMStatus status=invoke(caller,type?"Notify":"InvokeNotify",&output);
        CHECK(!probe_mode);
        if(mode<=2||mode==5||mode==8){CHECK(status==NativeScriptVM_Ok);CHECK(!field(caller,"result",NULL).as.i64&&field(caller,"tail",NULL).as.i64==1);
            if(mode==2){CHECK(replacement==recipient);CHECK(!field(replacement,"visits",NULL).as.i64);HSD_GObjFree(replacement);}if(mode==5||mode==8)HSD_GObjFree(recipient);HSD_GObjFree(caller);
        } else {CHECK(status==NativeScriptVM_Error||status==NativeScriptVM_SourceRetired);CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);
            CHECK(observed_i32(retained_tail)==0&&observed_i32(retained_result)==999);if(mode!=3)HSD_GObjFree(caller);HSD_GObjFree(recipient);}
        ++cases;
    }
    for(unsigned type=0;type<2;++type){caller=type?item(0):fighter(0,80+fighter_count);recipient=fighter(1,UINT32_C(0x80000005));set(caller,"handle",(int32_t)UINT32_C(0x80000005));probe_mode=1;
        CHECK(invoke(caller,type?"NotifyCall":"InvokeNotifyCall",&output)==NativeScriptVM_Ok);CHECK(field(caller,"boolean",NULL).as.u64==1&&field(caller,"tail",NULL).as.i64==1);HSD_GObjFree(caller);++cases;}
    for(unsigned type=0;type<2;++type){caller=type?item(0):fighter(0,100+fighter_count);recipient=fighter(1,UINT32_C(0x80000005));set(caller,"handle",(int32_t)UINT32_C(0x80000005));
        output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(invoke(caller,type?"Fault":"InvokeFault",&output)==NativeScriptVM_Error);CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);
        CHECK(nativeScriptFighter_Status(recipient)==NativeScriptVM_Error);HSD_GObjFree(caller);HSD_GObjFree(recipient);++cases;}
    caller=fighter(0,123);set(caller,"handle",123);uint8_t* cycle_tail=state_address(caller,"tail");output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(invoke(caller,"InvokeCycle",&output)==NativeScriptVM_Error);CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);CHECK(observed_i32(cycle_tail)==0);HSD_GObjFree(caller);++cases;
    CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS recipient exports\",\"checks\":%u,\"cases\":%u,\"vm_calls\":%u,\"program_banks\":3,\"callback_mutations\":%u}\n",checks,cases,vm_calls,probe_calls);return 0;
}
