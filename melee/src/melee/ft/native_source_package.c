#include "native_source_package.h"
#include "native_script_runtime.h"
#include "native_source_declarations.h"
#include "native_source_motion_policy.h"
#include "ftdata.h"
#include <melee/ef/native_script_effect_actor.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y||(x<y+bn&&y<x+an);}
static bool quiescent(void)
{return nativeScriptRuntime_Idle()&&!HSD_GObj_CurrentInvokedProc&&!HSD_GObj_CurrentInvokedProcGObj&&!HSD_GObj_804D7818&&!HSD_GObj_DelayedProcInfo.flags;}
static bool vacant(const NativeSourcePackage* package)
{
    if(!package)return false;const uint8_t* bytes=(const uint8_t*)package;
    for(size_t i=0;i<sizeof(*package);++i)if(bytes[i])return false;return true;
}
static bool stable(const NativeSourcePackage* package)
{
    return package&&package->address==package&&package->ready&&package->assets.ready&&package->assets.common_motions==ftData_MotionStateList&&package->compiler.arena&&
        package->visuals.ready&&package->visuals.address==&package->visuals&&
        package->resources.bytes==package->owned_input.resources&&package->resources.size==package->owned_input.resource_bytes&&
        package->compiler.bank.valid&&package->program.bank==&package->compiler.bank&&package->program.identity&&
        package->metadata.assets==package->entities.rows&&package->metadata.asset_count==package->entities.count&&
        package->metadata.motions==&package->assets.facts.moves&&package->metadata.source_facts==&package->assets.facts&&package->metadata.source_assets==&package->assets&&package->parameters.items==&package->metadata&&
        package->parameters.fighter.bytes==package->owned_input.assets.fighter&&
        package->parameters.fighter.fighter_root==package->assets.facts.fighter_root&&package->input_memory&&
        package->owned_input.family==NativeSourcePackage_Single;
}
static bool discard(NativeSourcePackage* package)
{
    if(!nativeSourceItemVisuals_Release(&package->visuals))return false;
    nativeSourceEntityAssets_Release(&package->entities);
    nativeSourceAssets_Release(&package->assets);nativeSourceBank_Release(&package->compiler);free(package->input_memory);
    memset(package,0,sizeof(*package));
    return true;
}
static bool copy_inputs(NativeSourcePackage* package,const NativeSourcePackageInput* input,NativeScriptDiagnostic* d)
{
    const void* pointers[]={input->facts,input->assets.fighter,input->assets.animation,input->assets.costume,input->source,input->parameters,input->resources};
    const size_t sizes[]={input->facts_bytes,input->assets.fighter_bytes,input->assets.animation_bytes,input->assets.costume_bytes,input->source_bytes,input->parameter_bytes,input->resource_bytes};
    const size_t limits[]={NATIVE_SOURCE_FACT_BYTES,NATIVE_SOURCE_DAT_BYTES,NATIVE_SOURCE_DAT_BYTES,NATIVE_SOURCE_DAT_BYTES,NATIVE_SOURCE_SOURCE_BYTES-1,NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY,NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES};
    size_t offsets[7],total=0;
    for(unsigned i=0;i<7;++i) {
        if(sizes[i]>limits[i]||(!pointers[i]&&sizes[i])||(i<5&&!sizes[i])||overlap(pointers[i],sizes[i],package,sizeof(*package)))
            return error(d,"Missing/oversized source package input or candidate alias");
        offsets[i]=total=(total+63)&~(size_t)63;
        if(sizes[i]>SIZE_MAX-total-1)return error(d,"Source package input size overflow");total+=sizes[i]+1;
    }
    void* memory=calloc(1,total);if(!memory)return error(d,"Could not reserve owned source package input bytes");
    package->input_memory=memory;package->input_bytes=total;package->owned_input=*input;
    package->owned_input.diagnostics=NULL;
    const void* copies[7];for(unsigned i=0;i<7;++i){copies[i]=(uint8_t*)memory+offsets[i];if(sizes[i])memcpy((void*)copies[i],pointers[i],sizes[i]);}
    package->owned_input.facts=copies[0];package->owned_input.assets.fighter=copies[1];package->owned_input.assets.animation=copies[2];package->owned_input.assets.costume=copies[3];
    package->owned_input.source=copies[4];package->owned_input.parameters=copies[5];
    package->owned_input.resources=sizes[6]?copies[6]:NULL;return true;
}
static bool metadata(void* opaque,const NativeScriptSourceTree* tree,uint32_t node,NativeScriptDiagnostic* d)
{
    NativeSourcePackage* package=opaque;
    if(nativeScriptPoseMetadata_Owns(tree,node))return nativeScriptPoseMetadata_Validate(&package->compiler.bank,tree,node,d);
    if(nativeScriptHitboxMetadata_Owns(tree,node))return nativeScriptHitboxMetadata_Validate(&package->compiler.bank,tree,node,d);
    if(nativeScriptEffectMetadata_Owns(tree,node))return nativeScriptEffectMetadata_Validate(tree,node,d);
    return nativeScriptItemMetadata_Validate(&package->metadata,tree,node,d);
}
bool nativeSourcePackage_Prepare(NativeSourcePackage* package,const NativeSourcePackageInput* input,FighterKind target,
    const NativeSourcePackage* accepted,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptDiagnostic scratch={0};NativeScriptDiagnostic* d=diagnostic?diagnostic:&scratch;
    if(!package||!input||!vacant(package)||!quiescent()||input->family!=NativeSourcePackage_Single||
       (unsigned)target>=Ft_Kind_RuntimeCapacity||target==Ft_Kind_None||
       overlap(input,sizeof(*input),package,sizeof(*package))||
       (accepted&&(!stable(accepted)||overlap(package,sizeof(*package),accepted,sizeof(*accepted)))))
        return error(d,"Source preparation needs separate stable/vacant owners, Single family and a quiescent load transition");
    package->address=package;package->target=target;
    if(!copy_inputs(package,input,d))goto failure;
    if(!nativeSourceResources_Read(package->owned_input.resources,package->owned_input.resource_bytes,
                                   &package->resources,d->message,sizeof(d->message)))goto failure;
    if(!nativeSourceBank_Allocate(&package->compiler,d))goto failure;
    package->compiler.bank.tree.source=package->owned_input.source;
    package->compiler.bank.tree.source_bytes=(uint32_t)package->owned_input.source_bytes;
    NativeSourceAssetFacts facts;
    if(!nativeScriptSource_Parse(&package->compiler.bank.tree,d)||
       !nativeSourceAssets_Read(package->owned_input.facts,package->owned_input.facts_bytes,&facts,d)||
       !nativeSourceDeclarations_Prepare(&package->compiler.bank.tree,&facts,&facts,d)||
       !nativeSourceAssets_Import(&facts,&package->owned_input.assets,target,&package->assets,d)||
       !nativeSourceMotionPolicy_Prepare(&package->compiler.bank.tree,ftData_MotionStateList,&package->assets,d)||
       !nativeSourceEntityAssets_Prepare(&package->entities,&package->assets,&package->owned_input.assets,
                                        &package->resources,&package->compiler.bank.tree,d))goto failure;
    package->metadata=(NativeScriptItemMetadataContext){.motions=&package->assets.facts.moves,.assets=package->entities.rows,.asset_count=package->entities.count,.source_facts=&package->assets.facts,.source_assets=&package->assets};
    package->parameters.fighter=(NativeScriptMetadataDat){package->owned_input.assets.fighter,package->owned_input.assets.fighter_bytes,package->assets.facts.fighter_root};
    package->parameters.items=&package->metadata;package->parameters.text=package->owned_input.parameters;package->parameters.text_bytes=package->owned_input.parameter_bytes;
    if(!nativeSourceParameters_Parse(&package->parameters,d))goto failure;
    NativeScriptCompileRequest request={.source=package->owned_input.source,.source_bytes=(uint32_t)package->owned_input.source_bytes,
        .metadata=metadata,.metadata_context=package,.parameter_provider=nativeSourceParameters_Provide,.parameter_context=&package->parameters,.motion_layout=&package->assets.facts.moves,
        .pose_graphs=package->assets.facts.poses,.pose_graph_count=package->assets.facts.pose_count,
        .pose_graph_mask=package->assets.facts.layout.joint_graph_mask,.diagnostics=input->diagnostics};
    const NativeScriptCompileBank* previous=accepted?&accepted->compiler.bank:NULL;
    if(!nativeScriptCompile_Build(&package->compiler.bank,&request,&previous,d))goto failure;
    unsigned fighters=0,independent=0;const NativeScriptSourceTree* tree=&package->compiler.bank.tree;
    for(unsigned i=0;i<tree->count;++i) {
        if(tree->nodes[i].kind==NativeScriptSource_Fighter)++fighters;
        if(tree->nodes[i].kind==NativeScriptSource_Metadata&&tree->nodes[i].parent<tree->count&&tree->nodes[tree->nodes[i].parent].kind==NativeScriptSource_Fighter&&
           nativeScriptSource_SpanEquals(tree,tree->nodes[i].name,"behavior")&&nativeScriptSource_SpanEquals(tree,tree->nodes[i].value,"independent"))++independent;
    }
    if(fighters!=1||independent!=1){error(d,"Source package requires exactly one independent Fighter declaration");goto failure;}
    if(!nativeScriptFighter_PrepareProgram(target,&package->compiler.bank,&package->assets.facts.moves,&package->metadata,&package->program,d)||
       !nativeSourceBank_Stats(&package->compiler,&package->stats)||
       !nativeSourceItemVisuals_Prepare(&package->visuals,&package->program.items,d))goto failure;
    package->ready=true;return true;
failure:
    discard(package);return false;
}
bool nativeSourcePackage_CanActivate(const NativeSourcePackage* package)
{return stable(package)&&quiescent()&&nativeScriptFighter_CanPublish(package->target);}
bool nativeSourcePackage_Pin(NativeSourcePackage* package)
{if(!stable(package)||!quiescent()||package->leases==UINT32_MAX)return false;++package->leases;return true;}
bool nativeSourcePackage_Unpin(NativeSourcePackage* package)
{if(!stable(package)||!quiescent()||!package->leases)return false;--package->leases;return true;}
bool nativeSourcePackage_CanRelease(const NativeSourcePackage* package)
{
    if(!package||!quiescent())return false;if(vacant(package))return true;
    if(!stable(package)||package->leases||nativeScriptFighter_ReferencesProgram(&package->compiler.bank)||
       nativeScriptItem_ReferencesProgram(&package->compiler.bank)||nativeScriptEffect_ReferencesProgram(&package->compiler.bank))return false;
    for(unsigned kind=0;kind<Ft_Kind_RuntimeCapacity;++kind){const NativeScriptFighterProgram* selected=nativeScriptFighter_Program((FighterKind)kind);if(selected&&selected->bank==&package->compiler.bank)return false;}
    return true;
}
bool nativeSourcePackage_Release(NativeSourcePackage* package)
{if(!nativeSourcePackage_CanRelease(package))return false;return discard(package);}
