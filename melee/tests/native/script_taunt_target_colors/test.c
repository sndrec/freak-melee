#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_targets.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage target_package;
#define selected_bank target_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static Fighter fighters[128],alternate;static Item items[64];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*query_target,*replacement;HSD_GObj*(*target_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons;
static MotionState common[341], extended[2];
static struct Fighter_WaitAnimData animations[2];
static ftCommonData common_data;ftCommonData* p_ftCommonData=&common_data;
static unsigned entries_seen,entry_retire;static FtMotionId entered;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"target query %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
    selected_bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="fighter:\n    behavior independent\nstate int color\nstate bool color_present\nstate int handle\nstate bool found\nstate bool air\nstate bool fighter\nstate float x\nstate float y\nstate float z\naction Queries:\n    on tick:\n        state.color_present = targets.controller_color_present(state.handle)\n        state.color = targets.controller_color(state.handle)\n        state.found = targets.read_position(state.handle, state.x, state.y, state.z)\n        state.air = targets.airborne(state.handle)\n        state.fighter = targets.is_fighter(state.handle)\nfunc EnterTaunt():\n    state.taunt(341, 342)\naction Taunt:\n    on taunt:\n        EnterTaunt()\narticle Probe states 1:\n    state int color\n    state bool color_present\n    state int handle\n    state bool found\n    state bool air\n    state bool fighter\n    state float x\n    state float y\n    state float z\n    on signal ObjectColor:\n        item.state.color_present = targets.controller_color_present(current_owner)\n        item.state.color = targets.controller_color(current_owner)\n    on signal Color:\n        item.state.color_present = targets.controller_color_present(item.current_owner)\n        item.state.color = targets.controller_color(item.current_owner)\n    on signal Query:\n        item.state.found = targets.read_position(item.state.handle, item.state.x, item.state.y, item.state.z)\n        item.state.air = targets.airborne(item.state.handle)\n        item.state.fighter = targets.is_fighter(item.state.handle)\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=2};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&selected_bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&selected_bank,&layout,&metadata,&target_package.program,&diagnostic));
    target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;target_package.assets.facts.moves=layout;
    target_package.assets.common_motions=common;target_package.assets.motions=extended;target_package.assets.facts.actions=2;target_package.assets.character.data.xC=animations;
    extended[0].anim_id=0;extended[1].anim_id=1;common[264].anim_id=0;common[265].anim_id=1;
    target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;tables[Ft_Kind_CustomFirst]=&target_package.assets.parts;
    gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=target_package.assets.motions;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned,uint32_t id)
{
    CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,
        .x8_spawnNum=id,.ft_data=&target_package.assets.character.data,.cur_pos={1.25f,-2.5f,3.75f}};
    f->x18=341;f->x1C_actionStateList=common;f->x20_actionStateList=extended;f->x24=animations;
    GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(void)
{
    CHECK(item_count<64);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100+item_count,.xC4_article_data=&article};
    GObj_InitUserData(o,6,item_remove,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    for(unsigned n=0;n<selected_bank.declarations.value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&selected_bank.tree,values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};}
static uint32_t bits(float value){uint32_t result;memcpy(&result,&value,4);return result;}
static float floating(uint32_t bits){float result;memcpy(&result,&bits,4);return result;}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static void args(NativeScriptValue* a,uint32_t id)
{a[0]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=id};for(unsigned i=1;i<4;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=111.0f+(float)i};}
static HSD_GObj* reuse(void){replacement=fighter(false,UINT32_C(0x80000005));return replacement;}
void target_guard_probe(NativeScriptRuntimeContext* c)
{
    if(c->source!=query_caller)return;++guard_calls;if(!mutation||guard_calls!=mutate_at)return;
    unsigned mode=mutation;mutation=0;
    if(mode==1)((Fighter*)query_target->user_data)->x8_spawnNum++;
    if(mode==2)((Fighter*)query_target->user_data)->kind=Ft_Kind_Zelda;
    if(mode==3){alternate=*(Fighter*)query_target->user_data;query_target->user_data=&alternate;}
    if(mode==4){HSD_GObjFree(query_target);if(target_reuse)target_reuse();}
    if(mode==5)HSD_GObjFree(query_caller);
    if(mode==6){NativeScriptValue result;CHECK(nativeScriptItem_RunHook(query_caller,NativeScriptItemHook_Destroyed,NULL,0,&result)==NativeScriptVM_Ok);}
    if(mode==7){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==query_caller)nativeScriptItemActors[i].faulted=true;}
    if(mode==8)query_target->classifier=HSD_GOBJ_CLASS_ITEM;
}
void taunt_change(Fighter_GObj* o,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* other)
{
    Fighter* f=o->user_data;CHECK(!f->allow_interrupt);CHECK(flags==0&&frame==0&&rate==1&&blend==0&&other==NULL);
    entered=motion;++entries_seen;f->motion_id=motion;
    if(entry_retire==1)HSD_GObjFree(o);
    if(entry_retire==2)++target_package.program.identity;
}
static uint32_t packed(GXColor c){return((uint32_t)c.r<<24)|((uint32_t)c.g<<16)|((uint32_t)c.b<<8)|c.a;}
static void color_taunt_cases(HSD_GObj* f,HSD_GObj* a,HSD_GObj* foreign)
{
    NativeScriptRuntimeContext c=context(f);NativeScriptValue arg={.type=NativeScriptType_U32,.as.u64=UINT32_C(0x80000005)},out;
    common_data.x6DC_colorsByPlayer[0]=(GXColor){255,2,3,4};common_data.x6DC_colorsByPlayer[1]=(GXColor){0,255,128,0};
    common_data.x6DC_colorsByPlayer[2]=(GXColor){1,2,3,255};common_data.x6DC_colorsByPlayer[3]=(GXColor){128,0,255,17};
    ((Item*)a->user_data)->owner=foreign;
    for(unsigned selector=0;selector<256;++selector){
        ((Fighter*)foreign->user_data)->x61A_controller_index=(u8)selector;GXColor native={0};bool present=ftLib_800870BC(foreign,&native);
        CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_TargetsControllerColorPresent,NULL,0,&arg,1,&out,&diagnostic)&&out.as.u64==present);
        CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_TargetsControllerColor,NULL,0,&arg,1,&out,&diagnostic)&&(uint32_t)out.as.i64==packed(native));
        NativeScriptValue id={.type=NativeScriptType_I32,.as.i64=-2147483643};field(f,"handle",&id);
        CHECK(nativeScriptFighter_Run(f,NativeScriptFighter_Tick,NULL,0, NULL,&out)==NativeScriptVM_Ok);
        CHECK(field(f,"color_present",NULL).as.u64==present&&(uint32_t)field(f,"color",NULL).as.i64==packed(native));
        CHECK(nativeScriptItem_RunSignal(a,hash("Color"),0,&out)==NativeScriptVM_Ok);
        CHECK(field(a,"color_present",NULL).as.u64==present&&(uint32_t)field(a,"color",NULL).as.i64==packed(native));
        CHECK(nativeScriptItem_RunSignal(a,hash("ObjectColor"),0,&out)==NativeScriptVM_Ok);CHECK(field(a,"color_present",NULL).as.u64==present&&(uint32_t)field(a,"color",NULL).as.i64==packed(native));++comparisons;
    }
    NativeScriptValue external;CHECK(nativeScriptReceiver_CaptureExternal(foreign,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&external,&diagnostic));
    CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsControllerColor,&external,1,&out,&diagnostic));
    ((Item*)a->user_data)->owner=NULL;CHECK(nativeScriptItem_RunSignal(a,hash("Color"),0,&out)==NativeScriptVM_Ok);CHECK(!field(a,"color_present",NULL).as.u64&&field(a,"color",NULL).as.i64==0);
    ftCommonData* saved=p_ftCommonData;p_ftCommonData=NULL;CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsControllerColor,&arg,1,&out,&diagnostic)&&out.as.i64==0);p_ftCommonData=saved;
    external.as.object.generation++;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=77};CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsControllerColor,&external,1,&out,&diagnostic)&&out.as.u64==77);
    float facings[]={-1,1,0,2,floating(0x7fc01234)};
    for(unsigned n=0;n<5;++n)for(unsigned flag=0;flag<2;++flag){Fighter* data=f->user_data;data->facing_dir=facings[n];animations[1].x8=(s32)flag;data->allow_interrupt=true;
        ftCo_800DEAE8(f,341,342);FtMotionId expected=entered;data->allow_interrupt=true;
        CHECK(nativeScriptFighter_Run(f,NativeScriptFighter_Taunt,NULL,0, NULL,&out)==NativeScriptVM_Ok);CHECK(entered==expected&&!data->allow_interrupt);++comparisons;
    }
    NativeScriptValue motions[2]={{.type=NativeScriptType_U64,.as.u64=264},{.type=NativeScriptType_I64,.as.i64=265}};
    CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_StateTaunt,NULL,0,motions,2,&out,&diagnostic)&&entered==264);
    unsigned old=entries_seen;motions[0].as.u64=UINT64_C(0x100000155);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=77};
    CHECK(!nativeScriptBindingState_Invoke(&c,NativeScriptBinding_StateTaunt,motions,2,&out,&diagnostic)&&entries_seen==old&&out.as.u64==77);
    motions[0]=(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=341};motions[1].as.i64=342;
    struct Fighter_WaitAnimData* original=((Fighter*)f->user_data)->x24;((Fighter*)f->user_data)->x24=NULL;
    CHECK(!nativeScriptBindingState_Invoke(&c,NativeScriptBinding_StateTaunt,motions,2,&out,&diagnostic)&&entries_seen==old);((Fighter*)f->user_data)->x24=original;
    extended[1].anim_id=2;CHECK(!nativeScriptBindingState_Invoke(&c,NativeScriptBinding_StateTaunt,motions,2,&out,&diagnostic)&&entries_seen==old);extended[1].anim_id=1;
    for(unsigned retire=1;retire<=2;++retire){HSD_GObj* doomed=fighter(true,700+retire);NativeScriptRuntimeContext dc=context(doomed);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=77};uint64_t identity=target_package.program.identity;entry_retire=retire;
        CHECK(!nativeScriptBindingState_Invoke(&dc,NativeScriptBinding_StateTaunt,motions,2,&out,&diagnostic)&&out.type==NativeScriptType_U64&&out.as.u64==77);
        entry_retire=0;target_package.program.identity=identity;if(retire!=1)HSD_GObjFree(doomed);
    }
}
static void type_cases(void)
{
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
    const char* invalid[]={
        "fighter:\n    behavior independent\nfunc Bad():\n    state.taunt(341.5,342)\n",
        "fighter:\n    behavior independent\nfunc Bad():\n    state.taunt(341)\n",
        "fighter:\n    behavior independent\nfunc Bad() -> int:\n    return targets.controller_color(1.5)\n",
        "fighter:\n    behavior independent\nfunc Bad() -> bool:\n    return targets.controller_color_present(1.5)\n",
        "fighter:\n    behavior independent\narticle Probe states 1:\n    on created:\n        state.taunt(341,342)\n    state 0 anim -1:\n"};
    for(unsigned i=0;i<5;++i) {
        NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
        const NativeScriptCompileBank* accepted=&selected_bank;
        NativeScriptCompileRequest request={.source=invalid[i],.source_bytes=(uint32_t)strlen(invalid[i]),.motion_layout=&selected_bank.motion_layout,
            .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));CHECK(accepted==&selected_bank&&selected_bank.valid);
        nativeSourceBank_Release(&scratch);
    }
}
int main(void)
{
    prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
    HSD_GObj* f=fighter(true,21),*a=item(),*foreign=fighter(false,UINT32_C(0x80000005));
    color_taunt_cases(f,a,foreign);
    type_cases();
    NativeScriptValue values4[4],output;args(values4,UINT32_C(0x80000005));NativeScriptRuntimeContext c=context(f);
    CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&output.as.u64);
    CHECK(values4[1].as.f32==1.25f&&values4[2].as.f32==-2.5f&&values4[3].as.f32==3.75f);
    // Both real owned caller classes publish SuccessOnly outputs through VM storage.
    for(unsigned k=0;k<2;++k){HSD_GObj* caller=k?a:f;NativeScriptValue id={.type=NativeScriptType_I32,.as.i64=-2147483643};field(caller,"handle",&id);
        if(k)CHECK(nativeScriptItem_RunSignal(caller,hash("Query"),0,&output)==NativeScriptVM_Ok);else CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_Tick,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(field(caller,"found",NULL).as.u64&&field(caller,"fighter",NULL).as.u64&&!field(caller,"air",NULL).as.u64);
        CHECK(field(caller,"x",NULL).as.f32==1.25f&&field(caller,"y",NULL).as.f32==-2.5f&&field(caller,"z",NULL).as.f32==3.75f);
        id.as.i64=0;field(caller,"handle",&id);if(k)CHECK(nativeScriptItem_RunSignal(caller,hash("Query"),0,&output)==NativeScriptVM_Ok);else CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_Tick,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(!field(caller,"found",NULL).as.u64&&!field(caller,"fighter",NULL).as.u64&&field(caller,"x",NULL).as.f32==1.25f);
    }
    static const uint32_t patterns[]={0,UINT32_C(0x80000000),UINT32_C(0x3f800000),UINT32_C(0xbf800000),UINT32_C(0x7f800000),UINT32_C(0xff800000),UINT32_C(0x7fc01234),UINT32_C(0x7fa00001)};
    for(unsigned n=0;n<64;++n){Fighter* target=foreign->user_data;target->cur_pos=(Vec3){floating(patterns[n%8]),floating(patterns[(n/8)%8]),floating(patterns[(n+3)%8])};target->ground_or_air=n%3;
        args(values4,UINT32_C(0x80000005));Vec3 native={0};CHECK(nativeFighterTarget_Position(UINT32_C(0x80000005),&native));
        CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&output.as.u64);
        CHECK(bits(values4[1].as.f32)==bits(native.x)&&bits(values4[2].as.f32)==bits(native.y)&&bits(values4[3].as.f32)==bits(native.z));
        CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsAirborne,values4,1,&output,&diagnostic)&&output.as.u64==nativeFighterTarget_Airborne(UINT32_C(0x80000005)));
        CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsIsFighter,values4,1,&output,&diagnostic)&&output.as.u64==nativeFighterTarget_IsFighter(UINT32_C(0x80000005)));++comparisons;
    }
    for(unsigned n=0;n<3;++n){args(values4,n==0?0:n==1?123456:UINT32_MAX);NativeScriptValue old[4];memcpy(old,values4,sizeof(old));CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&!output.as.u64);CHECK(memcmp(old,values4,sizeof(old))==0);}
    HSD_GObj* maximum=fighter(false,UINT32_MAX);args(values4,UINT32_MAX);values4[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1};
    CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&output.as.u64&&values4[1].as.f32==1.25f);HSD_GObjFree(maximum);
    HSD_GObj* wrong=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_FIGHTER,0);static Fighter wrongdata;wrongdata=(Fighter){.gobj=wrong,.x8_spawnNum=345};wrong->user_data=&wrongdata;args(values4,345);CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&!output.as.u64&&values4[1].as.f32==112);HSD_GObjFree(wrong);
    for(unsigned bad=0;bad<3;++bad){args(values4,5);if(bad==0)values4[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000005)};if(bad==1)values4[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=5};if(bad==2)values4[2].type=NativeScriptType_I32;
        NativeScriptValue old[4];memcpy(old,values4,sizeof(old));output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic)&&output.type==NativeScriptType_U64&&output.as.u64==99&&memcmp(old,values4,sizeof(old))==0);}
    HSD_GObjFree(foreign);
    for(unsigned caller_kind=0;caller_kind<2;++caller_kind)for(unsigned mode=1;mode<=8;++mode){if(!caller_kind&&(mode==6||mode==7))continue;
        query_caller=caller_kind?item():fighter(true,30+fighter_count);query_target=fighter(false,UINT32_C(0x80000005));c=context(query_caller);args(values4,UINT32_C(0x80000005));NativeScriptValue old[4];memcpy(old,values4,sizeof(old));output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        guard_calls=0;mutate_at=caller_kind?3:4;mutation=mode;target_reuse=mode==4?reuse:NULL;
        CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadPosition,values4,4,&output,&diagnostic));CHECK(mutation==0&&output.type==NativeScriptType_U64&&output.as.u64==99&&memcmp(old,values4,sizeof(old))==0);
        if(mode==4){CHECK(replacement==query_target);HSD_GObjFree(replacement);}else HSD_GObjFree(query_target);if(mode!=5)HSD_GObjFree(query_caller);query_caller=NULL;target_reuse=NULL;
    }
    HSD_GObjFree(a);HSD_GObjFree(f);CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS target queries\",\"checks\":%u,\"native_comparisons\":%u}\n",checks,comparisons);return 0;
}
