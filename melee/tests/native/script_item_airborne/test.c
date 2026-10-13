#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_compile.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/ef/types.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char text[]="article Probe states 1:\n    func Set(bool next) -> bool:\n        item.airborne = next\n        return item.airborne\n";
static NativeScriptSourceNode syntax[64];static NativeScriptIRNode expressions[128];static uint32_t arguments[64];
static NativeScriptFunctionDeclaration functions[4];static NativeScriptParameterDeclaration parameters[4];
static NativeScriptValueDeclaration values[4];static NativeScriptLocal locals[8];
static NativeScriptInstruction instructions[128];static NativeScriptFunctionCode code[1];
static NativeScriptStorageField fields[4];static NativeScriptStorageOwner owners[4];static NativeScriptParameterSlot slots[4];
static NativeScriptCompileBank bank;static NativeScriptItemProgram program;static NativeScriptStorageInstance storage;
static NativeScriptDiagnostic diagnostic;static NativeScriptBindingResolverContext resolver={NativeScriptBindings_Article};
NativeMatchPool nativeGObjPool;static HSD_GObj objects[2];static NativeMatchPoolSlot pool_slots[2];static uint32_t order[2];
static HSD_GObj* object;static HSD_GObj* lists[32];HSD_GObj** HSD_GObjPLinkHead=lists;
static Item item;static Article article;static ItemAttr attributes;
static bool attached,has_spec,has_view;static NativeScriptVMStatus actor_status;
static NativeScriptStorageLayout foreign_layout;static NativeScriptStorageParameters foreign_parameters;
static const NativeScriptProgramIndex* published_program;
static unsigned checks,cases,pairs,vm_calls;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"airborne %d: %s (%u:%u %s)\n",__LINE__,#x,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}while(0)

// Actor publication is controlled; pool membership, registry, frontend, storage,
// dispatcher, Item backend and VM are production implementations.
bool nativeScriptItem_IsAttached(Item_GObj* g){return g==object&&attached;}
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* g){return g==object&&has_spec?&program:NULL;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* g){return g==object?actor_status:NativeScriptVM_SourceRetired;}
bool nativeScriptItem_View(Item_GObj* g,uint32_t generation,NativeScriptReceiverView* out){if(g!=object||generation!=17||!has_view)return false;*out=(NativeScriptReceiverView){.program=published_program,.storage=&storage};return true;}
uint32_t nativeScriptItem_CallbackFunction(Item_GObj* g,uint32_t slot){(void)g;(void)slot;return UINT32_MAX;}
bool nativeScriptItem_BindCallback(Item_GObj* g,uint32_t slot,uint32_t function){(void)g;(void)slot;(void)function;return false;}
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* generation){(void)g;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t generation){(void)g;(void)generation;return NULL;}

static void compile(void)
{
 bank.tree=(NativeScriptSourceTree){text,sizeof(text)-1,syntax,0,64};CHECK(nativeScriptSource_Parse(&bank.tree,&diagnostic));
 bank.expressions=(NativeScriptIRArena){text,sizeof(text)-1,expressions,0,128,arguments,0,64};
 bank.declarations=(NativeScriptProgramIndex){.tree=&bank.tree,.expressions=&bank.expressions,.functions=functions,.function_capacity=4,.parameters=parameters,.parameter_capacity=4,.values=values,.value_capacity=4};
 CHECK(nativeScriptProgram_Index(&bank.declarations,&diagnostic));
 NativeScriptProgramCompiler compiler={.index=&bank.declarations,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&resolver,.locals=locals,.local_capacity=8};NativeScriptStatementCompiler body;
 CHECK(nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
 NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=128,.functions=code,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&resolver};
 CHECK(nativeScriptBytecode_Compile(&builder,&body,functions[0].source_node,0,1,&diagnostic));
 bank.code=(NativeScriptBytecodeProgram){text,sizeof(text)-1,instructions,builder.instruction_count,code,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&bank.code,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
 bank.storage_layout=(NativeScriptStorageLayout){.fields=fields,.field_capacity=4,.owners=owners,.owner_capacity=4};
 CHECK(nativeScriptStorage_BuildLayout(&bank.storage_layout,&bank.declarations,&diagnostic));
 CHECK(nativeScriptStorage_PrepareParameters(&bank.storage_layout,slots,4,NULL,0,&bank.storage_parameters,&diagnostic));bank.valid=true;
 program=(NativeScriptItemProgram){.bank=&bank,.identity=71,.owner=functions[0].owner,.author_kind=Ft_Kind_CustomFirst,.kind=It_Kind_GameWatch_Panic,.article=&article};CHECK(program.owner!=0);
}
static NativeScriptRuntimeContext context(void){return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Item,.generation=17,.scene_epoch=64};}
static void seed(ItemKind kind)
{
 NativeMatchPoolReference ref;nativeGObjPool=(NativeMatchPool){0};CHECK(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),pool_slots,order,2,sizeof(HSD_GObj),_Alignof(HSD_GObj),64));object=nativeMatchPool_Allocate(&nativeGObjPool,&ref);CHECK(object);memset(object,0,sizeof(*object));
 object->classifier=HSD_GOBJ_CLASS_ITEM;object->user_data=&item;memset(lists,0,sizeof(lists));lists[HSD_GOBJ_PLINK_ITEM]=object;
 program.kind=kind;program.article=&article;program.author_kind=Ft_Kind_CustomFirst;program.bank=&bank;bank.valid=true;
 item=(Item){.entity=object,.x1C=17,.kind=kind,.native_script_owner=program.owner,.native_article_author_kind=program.author_kind,.xC4_article_data=&article,.xCC_item_attr=&attributes,.ground_or_air=GA_Ground,.msid=23,.pos={1,-0.0f,3},.x40_vel={4,5,6}};
 CHECK(nativeScriptStorage_Create(&bank.storage_layout,&bank.storage_parameters,program.owner,29,17,NULL,0,&storage,&diagnostic));
 attached=has_spec=has_view=true;actor_status=NativeScriptVM_Ok;published_program=&bank.declarations;
}
static bool write(NativeScriptValue value){NativeScriptRuntimeContext c=context();return nativeScriptBindings_Write(&c,NativeScriptBinding_ItemAirborne,NULL,0,&value,&diagnostic);}
static NativeScriptValue boolean(unsigned value){return(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value};}
static NativeScriptVMStatus run(unsigned value,NativeScriptValue* out)
{
 NativeScriptValue registers[32];NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=registers,.register_capacity=32,.frames=frames,.frame_capacity=4};NativeScriptRuntimeContext c=context();NativeScriptValue input=boolean(value);++vm_calls;
 NativeScriptVMStatus result=nativeScriptVM_Run(&vm,&bank.code,&nativeScriptBindingHost,&c,0,&input,1,out);diagnostic=vm.diagnostic;return result;
}
static void rejects(void){Item before=item;CHECK(!write(boolean(1)));CHECK(!memcmp(&before,&item,sizeof(item)));++cases;}
static void type_negative(void)
{
 const char invalid[]="article Probe states 1:\n    func Set():\n        item.airborne = 1\n";
 NativeScriptSourceNode n[32];NativeScriptSourceTree tree={invalid,sizeof(invalid)-1,n,0,32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptIRNode e[64];uint32_t a[32];NativeScriptIRArena arena={invalid,sizeof(invalid)-1,e,0,64,a,0,32};NativeScriptFunctionDeclaration f[2];NativeScriptParameterDeclaration p[2];NativeScriptValueDeclaration v[2];NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=f,.function_capacity=2,.parameters=p,.parameter_capacity=2,.values=v,.value_capacity=2};CHECK(nativeScriptProgram_Index(&index,&diagnostic));NativeScriptLocal l[4];NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&resolver,.locals=l,.local_capacity=4};NativeScriptStatementCompiler body;CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));++cases;
}
int main(void)
{
 compile();CHECK(nativeScriptBindings_Descriptor(NativeScriptBinding_ItemAirborne)->flags&NativeScriptFlag_Writable);
 const ItemKind kinds[]={It_Kind_GameWatch_Panic,It_Kind_Mario_Fire};
 for(unsigned k=0;k<2;++k)for(unsigned initial=0;initial<3;++initial)for(unsigned v=0;v<2;++v){seed(kinds[k]);item.ground_or_air=(GroundOrAir)initial;Item expected=item;expected.ground_or_air=v?GA_Air:GA_Ground;CHECK(write(boolean(v)));CHECK(!memcmp(&expected,&item,sizeof(item)));NativeScriptRuntimeContext c=context();NativeScriptValue out;CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemAirborne,NULL,0,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==v);++pairs;++cases;}
 for(unsigned state=0;state<5;++state)for(unsigned v=0;v<2;++v){seed(It_Kind_NativeHoldable);item.msid=state;Item before=item;CHECK(!write(boolean(v)));CHECK(!memcmp(&before,&item,sizeof(item)));NativeScriptRuntimeContext c=context();NativeScriptValue out;CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemAirborne,NULL,0,&out,&diagnostic)&&out.as.u64==(state==2||state==3));++cases;}
 for(unsigned bad=0;bad<17;++bad){seed(kinds[0]);switch(bad){case 0:attached=false;item.native_script_owner=0;break;case 1:has_spec=false;break;case 2:has_view=false;break;case 3:actor_status=NativeScriptVM_Error;break;case 4:storage.live=false;break;case 5:++storage.generation;break;case 6:storage.layout=&foreign_layout;break;case 7:storage.parameters=&foreign_parameters;break;case 8:published_program=NULL;break;case 9:bank.valid=false;break;case 10:item.native_removal_in_progress=true;break;case 11:++item.x1C;break;case 12:object->classifier=HSD_GOBJ_CLASS_FIGHTER;break;case 13:program.article=NULL;break;case 14:++item.native_article_author_kind;break;case 15:++item.native_script_owner;break;case 16:program.kind=It_Kind_Mario_Fire;break;}rejects();}
 seed(kinds[0]);NativeScriptValue invalid={.type=NativeScriptType_I32,.as.i64=1};Item before=item;CHECK(!write(invalid)&&!memcmp(&before,&item,sizeof(item)));invalid=boolean(2);CHECK(!write(invalid)&&!memcmp(&before,&item,sizeof(item)));cases+=2;
 seed(kinds[0]);program.article=NULL;item.xC4_article_data=NULL;rejects();
 seed(kinds[0]);item.xCC_item_attr=NULL;rejects();
 seed(kinds[0]);NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));CHECK(nativeMatchPool_Free(&nativeGObjPool,ref));rejects();
 seed(kinds[0]);before=item;
 seed(kinds[0]);NativeScriptRuntimeContext c=context();c.scene_epoch++;CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_ItemAirborne,NULL,0,&(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1},&diagnostic));CHECK(!memcmp(&before,&item,sizeof(item)));++cases;
 for(unsigned v=0;v<2;++v){seed(kinds[0]);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(run(v,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Bool&&out.as.u64==v);CHECK(item.ground_or_air==(v?GA_Air:GA_Ground));++cases;}
 seed(It_Kind_NativeHoldable);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};before=item;CHECK(run(1,&out)==NativeScriptVM_Error);CHECK(!memcmp(&before,&item,sizeof(item))&&out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 seed(kinds[0]);storage.live=false;before=item;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(run(1,&out)==NativeScriptVM_Error);CHECK(!memcmp(&before,&item,sizeof(item))&&out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 type_negative();printf("{\"checks\":%u,\"cases\":%u,\"native_field_pairs\":%u,\"vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
