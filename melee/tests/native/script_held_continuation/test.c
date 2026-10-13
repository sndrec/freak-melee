#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/it/native_held_article.h>

#include <melee/ft/native_script_fighter_snapshot.h>

#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage lifecycle_package;
#define bank lifecycle_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;
static Fighter fighters[64];static Item items[128];static FighterBone parts[4];static HSD_JObj joints[4],roots[128];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned held_mode,native_tail,status_reads,read_trigger;
static unsigned checks,fighter_count,item_count,cases;
static NativeScriptDiagnostic diagnostic;
static Fighter swapped_fighter;static Item swapped_item;static HSD_GObj* holder;static HSD_GObj* held;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"lifecycle %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="fighter:\n    behavior independent\nstate int calls\nstate int zero\nstate bool request_fault\nfunc RequestProbe():\n    state.calls = state.calls + 1\n    if state.request_fault:\n        let int fail = 1 / state.zero\n    position.x = 77\narticle Probe states 1:\n    on held_status:\n        let float observed = creator.position.x\n        return 5\n    on held_request:\n        creator.helpers.RequestProbe()\n        return true\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Seak_Chain,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
    NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&lifecycle_package.program,&diagnostic));
    lifecycle_package.address=&lifecycle_package;lifecycle_package.target=Ft_Kind_CustomFirst;lifecycle_package.ready=lifecycle_package.assets.ready=true;
    lifecycle_package.assets.facts.moves=layout;lifecycle_package.assets.facts.joints=4;lifecycle_package.assets.parts.parts_num=4;
    tables[Ft_Kind_CustomFirst]=&lifecycle_package.assets.parts;gFtDataList[Ft_Kind_CustomFirst]=&lifecycle_package.assets.character.data;
    ftData_CharacterStateTables[Ft_Kind_CustomFirst]=lifecycle_package.assets.motions;
    for(unsigned i=0;i<4;++i)parts[i].joint=&joints[i];
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&lifecycle_package.program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned)
{
    CHECK(fighter_count<64);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,
        .x8_spawnNum=19+fighter_count,.ft_data=&lifecycle_package.assets.character.data,.parts=parts,.player_id=2,.team=3,.x34_scale={1,2.5f,1}};
    GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(HSD_GObj* creator)
{
    CHECK(item_count<128);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count];HSD_JObj* r=&roots[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,
        .x1C=100+item_count,.owner=creator,.xC4_article_data=&article,.sfx_unk2=12};
    o->hsd_obj=r;GObj_InitUserData(o,6,item_remove,i);nativeItem_CaptureCreator(o,creator);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    uint32_t kind=o->classifier==HSD_GOBJ_CLASS_FIGHTER?NativeCallbackSource_Fighter:NativeCallbackSource_Item;
    uint32_t gen=kind==NativeCallbackSource_Fighter?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(kind==NativeCallbackSource_Fighter?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    for(unsigned n=0;n<view.program->value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));
        CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}

void held_mutation(HSD_GObj* object)
{
 unsigned mode=held_mode;held_mode=0;
 if(mode==1){swapped_fighter=*(Fighter*)object->user_data;object->user_data=&swapped_fighter;}
 if(mode==2)object->classifier=HSD_GOBJ_CLASS_ITEM;
 if(mode==3){swapped_item=*(Item*)held->user_data;held->user_data=&swapped_item;}
 if(mode==4)held->classifier=HSD_GOBJ_CLASS_FIGHTER;
}
static void set(HSD_GObj* o,const char* name,int value)
{NativeScriptValue v={.type=NativeScriptType_Bool,.as.i64=value};field(o,name,&v);}
static void cleanup(void){HSD_GObjFree(held);HSD_GObjFree(holder);}
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=2,.gproc_pri_max=8};HSD_GObj_80391304(&config);
 holder=fighter(true);held=item(holder);Fighter* data=holder->user_data;data->item_gobj=held;data->x213C=-1;native_tail=status_reads=read_trigger=0;
 held_motion_block(holder);CHECK(native_tail==1&&data->x213C==99&&status_reads==3&&field(holder,"calls",NULL).as.i64==1);cleanup();++cases;
 holder=fighter(true);held=item(holder);data=holder->user_data;data->item_gobj=held;data->x213C=-1;native_tail=status_reads=read_trigger=0;set(holder,"request_fault",1);
 held_motion_block(holder);CHECK(native_tail==0&&data->x213C==-1&&nativeScriptFighter_Status(holder)==NativeScriptVM_Error);CHECK(status_reads==3);cleanup();++cases;
 for(unsigned step=1;step<=3;++step)for(unsigned mode=1;mode<=2;++mode){holder=fighter(true);held=item(holder);data=holder->user_data;data->item_gobj=held;data->x213C=-1;native_tail=status_reads=0;read_trigger=step;held_mode=mode;
  held_motion_block(holder);CHECK(native_tail==0&&data->x213C==-1&&status_reads==step);cleanup();++cases;
 }
 for(unsigned mode=1;mode<=2;++mode){holder=fighter(true);held=item(holder);data=holder->user_data;data->item_gobj=held;data->x213C=-1;native_tail=status_reads=read_trigger=0;held_mode=mode;
  held_motion_block(holder);CHECK(native_tail==0&&data->x213C==-1&&status_reads==3);cleanup();++cases;
 }
 for(unsigned mode=3;mode<=4;++mode){holder=fighter(true);held=item(holder);data=holder->user_data;data->item_gobj=held;status_reads=0;read_trigger=1;held_mode=mode;int32_t status=77;
  CHECK(nativeHeldArticle_Status(holder,&status)&&status==-1&&status_reads==1);CHECK(nativeScriptFighter_Status(holder)==NativeScriptVM_Ok);cleanup();++cases;
 }
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS held continuation\",\"checks\":%u,\"cases\":%u,\"real_helper_fault_cases\":1}\n",checks,cases);return 0;
}
