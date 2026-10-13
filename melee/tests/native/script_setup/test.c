#include "engine_fixture.h"
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_program.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/inlines.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;static HSD_GObj* heads[32];static HSD_GObj object,held;static Fighter actor;
static ftData data;static uint8_t immutable[FT_CHARACTER_EXT_ATTR_CAPACITY],scratch[FT_CHARACTER_EXT_ATTR_CAPACITY];
static NativeScriptRuntimeContext context;static NativeScriptDiagnostic diagnostic;static unsigned checks,body_pairs;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"Setup check %d: %s (%s)\n",__LINE__,#v,diagnostic.message);exit(1);}}while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
static void setup(void)
{
    memset(&object,0,sizeof(object));memset(&actor,0,sizeof(actor));memset(heads,0,sizeof(heads));memset(&data,0,sizeof(data));
    HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;object.user_data=&actor;actor.gobj=&object;actor.x8_spawnNum=17;actor.item_gobj=&held;
    actor.ft_data=&data;data.ext_attr=immutable;actor.dat_attrs_backup=scratch;
    for(unsigned i=0;i<sizeof(immutable);++i)immutable[i]=(uint8_t)(i*7);memset(scratch,0xCC,sizeof(scratch));
    scriptSetupFixture_Reset();CHECK(nativeScriptBindings_Context(&object,NativeCallbackSource_Fighter,17,&context,&diagnostic));
}
static NativeScriptValue integer(int64_t n){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=n};}
static void invoke(uint32_t id,NativeScriptValue* args,unsigned count)
{NativeScriptValue result;CHECK(nativeScriptBindings_Invoke(&context,id,NULL,0,args,count,&result,&diagnostic));CHECK(result.type==nativeScriptBindings_Descriptor(id)->result_type);}
static void direct(void)
{
    setup();invoke(NativeScriptBinding_FighterEnableWalljump,NULL,0);CHECK(actor.can_walljump);
    NativeScriptValue result={0};CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_FighterReloadExtAttributes,NULL,0,NULL,0,&result,&diagnostic));
    invoke(NativeScriptBinding_FighterCopyExtAttributes,NULL,0);CHECK(actor.dat_attrs==scratch&&!memcmp(immutable,scratch,sizeof(scratch)));
    scratch[4]=0;invoke(NativeScriptBinding_FighterReloadExtAttributes,NULL,0);CHECK(scratch[4]==immutable[4]);
    invoke(NativeScriptBinding_FighterEnableMultiJump,NULL,0);CHECK(actor.can_multijump&&(void*)actor.x2D0==scratch);
    actor.dat_attrs_backup=immutable;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_FighterCopyExtAttributes,NULL,0,NULL,0,&result,&diagnostic));
    setup();const uint32_t ids[]={0,16777217,UINT32_C(0x80000001),UINT32_MAX};
    for(unsigned i=0;i<4;++i){NativeScriptValue select=integer(i),value={.type=NativeScriptType_U32,.as.u64=ids[i]};
        CHECK(nativeScriptBindings_Write(&context,NativeScriptBinding_Command,&select,1,&value,&diagnostic));
        CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_Command,&select,1,&result,&diagnostic));CHECK((uint32_t)result.as.i64==ids[i]);CHECK(result.as.i64==(ids[i]<=INT32_MAX?(int64_t)ids[i]:(int64_t)ids[i]-INT64_C(0x100000000)));}
    NativeScriptValue invalid={.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)};u32 before=actor.cmd_vars[0];CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_Command,NULL,0,&invalid,1,&result,&diagnostic)&&actor.cmd_vars[0]==before);
    actor.throw_flags=15;invoke(NativeScriptBinding_CueReverseConsume,NULL,0);CHECK(actor.throw_flags==7);invoke(NativeScriptBinding_CueArticleConsume,NULL,0);CHECK(actor.throw_flags==6);invoke(NativeScriptBinding_CueThrowConsume,NULL,0);CHECK(actor.throw_flags==4);invoke(NativeScriptBinding_CueThrowReset,NULL,0);CHECK(actor.throw_flags==0);
    invoke(NativeScriptBinding_CallbackTrackDamage,NULL,0);CHECK(script_setup_callbacks[NativeScriptFighterCallback_Damage]==64);
    invoke(NativeScriptBinding_CallbackTrackDeath3,NULL,0);CHECK(script_setup_callbacks[NativeScriptFighterCallback_Death3]==66);
    invoke(NativeScriptBinding_CallbackTrackHitlag,NULL,0);CHECK(script_setup_callbacks[4]==68&&script_setup_callbacks[5]==69);
    NativeScriptValue code={.type=NativeScriptType_Callable,.as.u64=7};invoke(NativeScriptBinding_CallbackDeath1,&code,1);CHECK(script_setup_callbacks[1]==7);
    NativeScriptValue none={.type=NativeScriptType_Null};invoke(NativeScriptBinding_CallbackDeath1,&none,1);CHECK(script_setup_callbacks[1]==UINT32_MAX);
    invoke(NativeScriptBinding_CallbackClearDamageDeath,NULL,0);CHECK(script_setup_callbacks[0]==UINT32_MAX&&script_setup_callbacks[2]==UINT32_MAX);
    code.as.u64=UINT64_C(0x100000000);CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_CallbackDamage,NULL,0,&code,1,&result,&diagnostic));
    actor.co_attrs.gravity=0.1234567f;actor.co_attrs.aerial_friction=0.7654321f;
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_AirGravity,NULL,0,&result,&diagnostic)&&result.type==NativeScriptType_F32&&result.as.f32==actor.co_attrs.gravity);
    CHECK(nativeScriptBindings_Read(&context,NativeScriptBinding_AirFriction,NULL,0,&result,&diagnostic)&&result.type==NativeScriptType_F32&&result.as.f32==actor.co_attrs.aerial_friction);
}
static void presentation(void)
{
    for(unsigned heavy=0;heavy<2;++heavy)for(int kind=0;kind<6;++kind)for(unsigned catch=0;catch<2;++catch){
        setup();scriptSetupFixture_Held(heavy!=0,kind);Fighter_OnItemPickup(&object,catch!=0,true,true);unsigned count=script_setup_count,expected[16][3];memcpy(expected,script_setup_log,sizeof(expected));
        scriptSetupFixture_Held(heavy!=0,kind);NativeScriptValue values[]={{.type=NativeScriptType_Bool,.as.u64=catch},integer(1),integer(1)};invoke(NativeScriptBinding_FighterItemPickup,values,3);CHECK(script_setup_count==count&&!memcmp(expected,script_setup_log,count*sizeof(expected[0])));++body_pairs;}
    for(unsigned stage=1;stage<=2;++stage){setup();scriptSetupFixture_RetireAt(stage);NativeScriptValue args[]={{.type=NativeScriptType_Bool,.as.u64=1},integer(1),integer(1)},result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_FighterItemDrop,NULL,0,args,3,&result,&diagnostic));CHECK(script_setup_count==stage&&result.type==NativeScriptType_U64);}
}
static void frontend(void)
{
    const char* text="func Cleanup():\n    command(0) = 0\nfunc Initialize():\n    fighter.enable_walljump()\n    fighter.copy_ext_attributes()\n    command(2) = -1\n    callbacks.damage(Cleanup)\n    callbacks.death2(None)\n";
    NativeScriptSourceNode syntax[64];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,64};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRNode nodes[512];uint32_t args[256];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,512,args,0,256};
    NativeScriptFunctionDeclaration funcs[8];NativeScriptParameterDeclaration parameters[8];NativeScriptValueDeclaration fields[8];NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=funcs,.function_capacity=8,.parameters=parameters,.parameter_capacity=8,.values=fields,.value_capacity=8};CHECK(nativeScriptProgram_Index(&index,&diagnostic));
    NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};NativeScriptLocal locals[32];NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=nativeScriptBindings_Resolve,.native_binding_context=&binding,.locals=locals,.local_capacity=32};
    NativeScriptInstruction code[512];NativeScriptFunctionCode functions[2];NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=512,.functions=functions,.function_count=2,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};
    for(unsigned i=0;i<2;++i){NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic));CHECK(nativeScriptBytecode_Compile(&builder,&body,funcs[i].source_node,i,0,&diagnostic));}
    NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,functions,2,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[256];NativeScriptVMFrame frames[8];NativeScriptVM vm={.registers=registers,.register_capacity=256,.frames=frames,.frame_capacity=8};
    setup();NativeScriptValue result;CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,1,NULL,0,&result)==NativeScriptVM_Ok);CHECK(actor.can_walljump&&actor.cmd_vars[2]==UINT32_MAX&&actor.dat_attrs==scratch&&script_setup_callbacks[0]==0&&script_setup_callbacks[2]==UINT32_MAX);
    const char* bad[]={"command(4)","command(1.0)","callbacks.damage(1)","callbacks.reset_root_x_on_damage()"};
    for(unsigned i=0;i<4;++i){NativeScriptIRArena one={bad[i],(uint32_t)strlen(bad[i]),nodes,0,512,args,0,256};uint32_t out;CHECK(!nativeScriptExpression_Parse(&one,(NativeScriptSourceSpan){0,one.source_bytes,1,1},nativeScriptBindings_Resolve,&binding,&out,&diagnostic));}
}
static void readonly_and_retirement(void)
{
    const char* sources[]={"func Inspect() -> int:\n    return command(3)\n",
        "func Inspect() -> int:\n    command(3) = 9\n    return command(3)\n",
        "func DropAndContinue():\n    fighter.item_drop(true, 1, 1)\n    command(3) = 9\n"};
    for(unsigned i=0;i<3;++i){
        const char* text=sources[i];NativeScriptSourceNode syntax[32];NativeScriptSourceTree tree={text,(uint32_t)strlen(text),syntax,0,32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
        NativeScriptIRNode nodes[128];uint32_t args[64];NativeScriptIRArena arena={text,tree.source_bytes,nodes,0,128,args,0,64};NativeScriptLocal locals[8];NativeScriptBindingResolverContext binding={NativeScriptBindings_Fighter};
        NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&binding,.locals=locals,.local_capacity=8,.return_type=i<2?NativeScriptType_I32:NativeScriptType_Void,.body_policy=i<2?NativeScriptBodyPolicy_ReadOnly:0};
        if(i==1){CHECK(!nativeScriptStatement_Compile(&body,1,&diagnostic));CHECK(strstr(diagnostic.message,"cannot write")!=NULL);continue;}
        CHECK(nativeScriptStatement_Compile(&body,1,&diagnostic));
        if(i==0){const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(NativeScriptBinding_Command);CHECK((descriptor->flags&(NativeScriptFlag_Pure|NativeScriptFlag_Writable|NativeScriptFlag_Addressable))==(NativeScriptFlag_Pure|NativeScriptFlag_Writable|NativeScriptFlag_Addressable));CHECK(!(descriptor->flags&NativeScriptFlag_MayInvalidateSource));}
        NativeScriptInstruction code[128];NativeScriptFunctionCode functions[1];NativeScriptBytecodeBuilder builder={.instructions=code,.instruction_capacity=128,.functions=functions,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};CHECK(nativeScriptBytecode_Compile(&builder,&body,1,0,0,&diagnostic));
        NativeScriptBytecodeProgram program={text,tree.source_bytes,code,builder.instruction_count,functions,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[64];NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=4};
        setup();actor.cmd_vars[3]=UINT32_C(0x80000001);NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        if(i==0){CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_Ok);CHECK(result.type==NativeScriptType_I32&&result.as.i64==-INT64_C(2147483647));}
        else{scriptSetupFixture_RetireAt(1);CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&context,0,NULL,0,&result)==NativeScriptVM_SourceRetired);CHECK(actor.cmd_vars[3]==UINT32_C(0x80000001)&&script_setup_count==1&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);CHECK(vm.depth==0&&vm.register_top==0);}
    }
}
int main(void){direct();presentation();frontend();readonly_and_retirement();printf("{\"checks\":%u,\"actual_inline_pairs\":%u,\"status\":\"PASS fighter setup/command/code-ref binding contracts\"}\n",checks,body_pairs);return 0;}
