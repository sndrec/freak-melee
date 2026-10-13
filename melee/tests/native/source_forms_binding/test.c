#include <melee/ft/native_script_binding_forms.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcommon.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static NativeSourcePackage package;static NativeScriptFighterProgram selected;
static NativeScriptStorageInstance storage;static MotionState motions[2];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static Fighter actor,partner;static HSD_GObj object,destination;static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static unsigned checks,calls,mode;static bool framework_ok,can_destination,pair,dormant;
static NativeScriptVMStatus status;static NativeScriptRuntimeContext context;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"forms %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* n){(void)g;(void)n;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t n){(void)g;(void)n;return NULL;}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==package.target?&package:NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{return kind==package.target?&selected:NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* g){return g==&object?status:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* g,uint32_t generation,NativeScriptReceiverView* view)
{if(g!=&object||generation!=storage.generation)return false;*view=(NativeScriptReceiverView){&package.compiler.bank.declarations,&storage};return true;}
bool nativeFighterRuntime_AlternatePair(FighterKind a,FighterKind b)
{CHECK(a==Ft_Kind_CustomFirst&&b==Ft_Kind_CustomFirst+1);return pair;}
bool nativeScriptFighter_CanActivateForm(Fighter_GObj* g)
{CHECK(g==&destination);return can_destination;} // Never gate the running source.
bool ftCommon_ActivatePartner(Fighter_GObj* g,HSD_GObjEvent finish)
{
    CHECK(g==&object&&finish==NULL);++calls;
    if(!nativeFighterRuntime_AlternatePair(actor.kind,partner.kind)||!nativeScriptFighter_CanActivateForm(&destination)||!framework_ok)return false;
    dormant=true;
    if(mode==1)storage.live=false;
    if(mode==2){heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;object.user_data=(void*)1;}
    if(mode==3)++actor.x8_spawnNum;
    if(mode==4)status=NativeScriptVM_Error;
    return true;
}
#include "actual_source_guard.h"
static void setup(void)
{
    memset(&package,0,sizeof(package));memset(&actor,0,sizeof(actor));memset(&partner,0,sizeof(partner));memset(heads,0,sizeof(heads));
    object=(HSD_GObj){.user_data=&actor};destination=(HSD_GObj){.user_data=&partner};heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    actor.gobj=&object;actor.x8_spawnNum=17;actor.kind=actor.behavior_kind=Ft_Kind_CustomFirst;partner.kind=Ft_Kind_CustomFirst+1;
    package.address=&package;package.ready=package.assets.ready=true;package.target=actor.kind;
    package.compiler.bank.valid=package.compiler.bank.has_motion_layout=true;
    package.compiler.bank.motion_layout=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=2};package.assets.facts.moves=package.compiler.bank.motion_layout;
    package.assets.motions=motions;actor.ft_data=&package.assets.character.data;
    gFtDataList[actor.kind]=actor.ft_data;ftData_CharacterStateTables[actor.kind]=motions;
    package.program.identity=29;selected=(NativeScriptFighterProgram){.bank=&package.compiler.bank,.identity=29};
    storage=(NativeScriptStorageInstance){.layout=&package.compiler.bank.storage_layout,.owner=0,.generation=17,.live=true};
    context=(NativeScriptRuntimeContext){.source=&object,.source_class=NativeCallbackSource_Fighter,.generation=17};
    calls=mode=0;framework_ok=can_destination=pair=true;dormant=false;status=NativeScriptVM_Ok;
}
static void semantic(void)
{
    const char* text="func Change():\n    forms.activate_partner()\n";
    NativeScriptSourceNode syntax[32];NativeScriptIRNode nodes[64];uint32_t args[64];NativeScriptLocal locals[4];
    NativeScriptSourceTree tree={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=syntax,.capacity=32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={.source=text,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=64,.arguments=args,.argument_capacity=64};
    NativeScriptBindingResolverContext resolver={NativeScriptBindings_Fighter};
    NativeScriptStatementCompiler compiler={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&resolver,
        .locals=locals,.local_capacity=4,.return_type=NativeScriptType_Void};
    CHECK(nativeScriptStatement_Compile(&compiler,1,&diagnostic));
    NativeScriptInstruction instructions[64];NativeScriptFunctionCode function[1];NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=64,
        .functions=function,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&resolver};
    CHECK(nativeScriptBytecode_Compile(&builder,&compiler,1,0,0,&diagnostic));bool found=false;
    for(uint32_t i=0;i<builder.instruction_count;++i)if(instructions[i].semantic==NativeScriptBinding_FormsActivatePartner)found=true;
    CHECK(found&&nativeScriptBindings_Descriptor(NativeScriptBinding_FormsActivatePartner)->result_type==NativeScriptType_Void);
}
int main(void)
{
    semantic();setup();NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    CHECK(nativeScriptBindings_Invoke(&context,NativeScriptBinding_FormsActivatePartner,NULL,0,NULL,0,&result,&diagnostic));
    CHECK(result.type==NativeScriptType_Void&&calls==1&&dormant&&nativeScriptBindings_SourceAlive(&context));
    for(unsigned failure=0;failure<8;++failure){setup();result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
        if(failure==0)framework_ok=false;if(failure==1)pair=false;if(failure==2)can_destination=false;
        if(failure>=3&&failure<=6)mode=failure-2;if(failure==7)package.compiler.bank.valid=false;
        CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_FormsActivatePartner,NULL,0,NULL,0,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);CHECK(calls==(failure==7?0:1));
    }
    setup();context.source_class=NativeCallbackSource_Item;CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_FormsActivatePartner,NULL,0,NULL,0,&result,&diagnostic));CHECK(calls==0);
    setup();NativeScriptValue arg={.type=NativeScriptType_I32};CHECK(!nativeScriptBindings_Invoke(&context,NativeScriptBinding_FormsActivatePartner,NULL,0,&arg,1,&result,&diagnostic));CHECK(calls==0);
    printf("{\"status\":\"PASS forms catalog/source guard/transfer boundary\",\"checks\":%u,\"transfer_leaf_controlled\":true}\n",checks);return 0;
}
