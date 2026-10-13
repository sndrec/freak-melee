#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_world_feedback.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcommon.h>
#include <melee/cm/camera.h>
#include <melee/gr/grlib.h>
#include <melee/gr/stage.h>
#include <melee/sc/types.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_capture_item.h>
#include <melee/it/it_2725.h>
#include <melee/lb/lb_013B.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage feedback_package;
#define bank feedback_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc item_model;static Fighter fighters[128],alternate_fighter;static Item items[256],alternate_item;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned model_loads,model_frees,animation_adds,animation_requests,animation_ticks,loop_flags,hardware_calls;
int last_controller,last_id,last_duration,last_priority;const void* last_commands;
unsigned feedback_mutation;HSD_GObj* feedback_source,*feedback_owner,*feedback_replacement;HSD_GObj*(*feedback_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons;static NativeScriptDiagnostic diagnostic;
static HSD_Joint model_joint;static HSD_AnimJoint model_animations[4];static HSD_AnimJoint* anims[]={&model_animations[0],&model_animations[1],&model_animations[2],&model_animations[3]};static DynamicModelDesc quake_model={.joint=&model_joint,.anims=anims};static uint16_t commands[3][2]={{0x101,0},{0x202,0},{0x303,0}};
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"world feedback %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="fighter:\n    behavior independent\nstate int completed\nfunc Rumble():\n    controller.rumble(1, 0)\nfunc CaptureAttach():\n    if capture.has_item:\n        capture.attach_item(0)\nfunc CaptureRelease():\n    capture.release_item(0, true)\naction Feedback:\n    on tick:\n        camera.quake(2, 1.25, -2.5, 3.75)\n        state.completed = state.completed + 1\narticle Probe states 1:\n    state int completed\n    on signal Feedback:\n        item.rumble(1, 0)\n        camera.quake(3, 4.5, 5.5, 6.5)\n        item.state.completed = item.state.completed + 1\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&item_model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Zelda_DinFire_Explode,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&feedback_package.program,&diagnostic));
    feedback_package.address=&feedback_package;feedback_package.target=Ft_Kind_CustomFirst;feedback_package.ready=feedback_package.assets.ready=true;feedback_package.assets.facts.moves=layout;
    feedback_package.assets.parts.parts_num=4;feedback_package.assets.facts.joints=4;tables[Ft_Kind_CustomFirst]=&feedback_package.assets.parts;gFtDataList[Ft_Kind_CustomFirst]=&feedback_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=feedback_package.assets.motions;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&feedback_package.program);
    lbRumblePatternCount=3;for(unsigned i=0;i<3;++i)rumble_patterns[i]=(HSD_RumblePattern){.commands=commands[i],.priority=5+i};
    player_slots[2].player_state=2;player_slots[2].flags.b0=1;stage_info.quake_model_set=&quake_model;
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned)
{
    CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];*f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.x8_spawnNum=20+fighter_count,.player_id=2,.x618_player_id=1,.ft_data=&feedback_package.assets.character.data};GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(void)
{
    CHECK(item_count<256);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count++];*i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100+item_count,.xC4_article_data=&article};GObj_InitUserData(o,6,item_remove,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static int completed(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;NativeScriptReceiverView view;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));for(unsigned n=0;n<bank.declarations.value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,"completed")){NativeScriptValue out;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&out,&diagnostic));return(int)out.as.i64;}CHECK(false);return 0;}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};}
static bool invoke(HSD_GObj* o,uint32_t id,NativeScriptValue* a,uint32_t count,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(o);return nativeScriptBindingWorldFeedback_Invoke(&c,id,a,count,out,&diagnostic);}
static void clear_quakes(void)
{for(HSD_GObj* o=HSD_GObjPLinkHead[18];o;){HSD_GObj* next=o->next;HSD_GObjFree(o);o=next;}memset(&game_camera,0,sizeof(game_camera));}
static HSD_GObj* reuse_item(void){feedback_replacement=item();return feedback_replacement;}
static HSD_GObj* reuse_owner(void){feedback_replacement=fighter(false);return feedback_replacement;}
void feedback_completed(void)
{
    unsigned mode=feedback_mutation;if(!mode)return;feedback_mutation=0;
    if(mode==1){NativeScriptValue out;CHECK(nativeScriptItem_RunHook(feedback_source,NativeScriptItemHook_Destroyed,NULL,0,&out)==NativeScriptVM_Ok);}
    if(mode==2)HSD_GObjFree(feedback_source);
    if(mode==3){HSD_GObjFree(feedback_source);if(feedback_reuse)feedback_reuse();}
    if(mode==4){alternate_item=*(Item*)feedback_source->user_data;feedback_source->user_data=&alternate_item;}
    if(mode==5){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==feedback_source)nativeScriptItemActors[i].faulted=true;}
    if(mode==6)HSD_GObjFree(feedback_owner);
    if(mode==7)((Fighter*)feedback_owner->user_data)->kind=Ft_Kind_Zelda;
    if(mode==8){alternate_fighter=*(Fighter*)feedback_owner->user_data;feedback_owner->user_data=&alternate_fighter;}
    if(mode==9){HSD_GObjFree(feedback_owner);if(feedback_reuse)feedback_reuse();}
    if(mode==10)feedback_source->classifier=HSD_GOBJ_CLASS_STAGE;
    if(mode==11)game_camera.quake_gobj->classifier=HSD_GOBJ_CLASS_FIGHTER;
    if(mode==12)HSD_GObjFree(game_camera.quake_gobj);
    if(mode==13)++((Fighter*)feedback_source->user_data)->x8_spawnNum;
    if(mode==14)++feedback_package.program.identity;
    if(mode==15)++bank.storage_layout.identity;
    if(mode==16)++bank.storage_parameters.identity;
    if(mode==18){alternate_fighter=*(Fighter*)feedback_source->user_data;feedback_source->user_data=&alternate_fighter;}
}
#include "fighter_rumble_cases.h"
#include "capture_item_cases.h"
#include "capture_item_bindings_cases.h"
int main(void)
{
    prepare();HSD_GObjLibInitDataType configuration={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&configuration);HSD_GObj* f=fighter(true),*a=item(),*owner=fighter(false),*other=item();((Item*)a->user_data)->owner=owner;
    NativeScriptValue out;CHECK(nativeScriptFighter_Run(f,NativeScriptFighter_Tick,NULL,0, NULL,&out)==NativeScriptVM_Ok&&completed(f)==1);CHECK(game_camera.quake_frames_left[QuakeKind_Small]==22&&model_loads==1&&animation_adds==1&&animation_requests==1);
    for(unsigned i=0;i<16;++i)CHECK(game_camera.quakes[0][i].kind==QuakeKind_Small&&game_camera.quakes[0][i].epicenter.x==1.25f);
    CHECK(nativeScriptItem_RunSignal(a,hash("Feedback"),0,&out)==NativeScriptVM_Ok&&completed(a)==1&&hardware_calls==1&&last_controller==1&&last_id==1&&last_duration==-2&&last_priority==6&&last_commands==commands[1]);clear_quakes();
    NativeScriptValue q[]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_F32,.as.f32=-0.0f},{.type=NativeScriptType_F32,.as.f32=NAN},{.type=NativeScriptType_F32,.as.f32=INFINITY}};
    for(unsigned kind=1;kind<=4;++kind){clear_quakes();q[0].as.i64=kind;unsigned loaded=model_loads;CHECK(invoke(a,NativeScriptBinding_CameraQuake,q,4,&out)&&out.type==NativeScriptType_Void&&model_loads==loaded+1&&game_camera.quake_frames_left[kind]==(kind==1?10:22));
        HSD_GObj* quake=HSD_GObjPLinkHead[18];CHECK(quake&&quake->classifier==HSD_GOBJ_CLASS_STAGE&&quake->proc&&quake->proc->s_link==1&&quake->hsd_obj);
        for(unsigned i=0;i<16;++i)CHECK(game_camera.quakes[0][i].kind==(CmQuakeKind)kind&&signbit(game_camera.quakes[0][i].epicenter.x)&&isnan(game_camera.quakes[0][i].epicenter.y)&&isinf(game_camera.quakes[0][i].epicenter.z));
        if(kind==1){CHECK(game_camera.quake_gobj==quake&&(((HSD_JObj*)quake->hsd_obj)->aobj->flags&AOBJ_LOOP));unsigned count=model_loads;CHECK(invoke(f,NativeScriptBinding_CameraQuake,q,4,&out)&&model_loads==count&&game_camera.quake_gobj==quake);quake->proc->on_invoke(quake);CHECK(game_camera.quake_offset.x==12&&game_camera.quake_offset.y==34);}
        else{((HSD_JObj*)quake->hsd_obj)->aobj->flags|=0x40000000;HSD_GObj_80390CFC();CHECK(HSD_GObjPLinkHead[18]==NULL&&game_camera.quake_offset.x==12&&game_camera.quake_offset.y==34);}++comparisons;
    }
    // A model-less native request still sets timers and every empty slot.
    clear_quakes();stage_info.quake_model_set=NULL;unsigned before=model_loads;q[0].as.i64=1;CHECK(invoke(a,NativeScriptBinding_CameraQuake,q,4,&out)&&game_camera.quake_gobj==NULL&&game_camera.quake_frames_left[1]==10&&model_loads==before);stage_info.quake_model_set=&quake_model;
    for(unsigned n=0;n<16;++n){clear_quakes();for(unsigned i=0;i<16;++i)if(i&1)game_camera.quakes[0][i].kind=QuakeKind_Large;Camera original=game_camera;q[0].as.i64=2+(n%3);CHECK(invoke(a,NativeScriptBinding_CameraQuake,q,4,&out));Camera actual=game_camera;clear_quakes();game_camera=original;Vec3 position={q[1].as.f32,q[2].as.f32,q[3].as.f32};Camera_RequestQuake((CmQuakeKind)q[0].as.i64,&position);CHECK(memcmp(actual.quakes,game_camera.quakes,sizeof(actual.quakes))==0&&memcmp(actual.quake_frames_left,game_camera.quake_frames_left,sizeof(actual.quake_frames_left))==0);++comparisons;}
    clear_quakes();for(unsigned k=0;k<2;++k){q[0].as.i64=k?5:0;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};before=model_loads;CHECK(!invoke(a,NativeScriptBinding_CameraQuake,q,4,&out)&&out.as.u64==99&&model_loads==before);}q[0].as.i64=2;
    NativeScriptValue rumble[]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_I32,.as.i64=0}};Fighter* fp=owner->user_data;
    for(unsigned n=0;n<12;++n){rumble[0].as.i64=n%3;rumble[1].as.i64=n==0?0:n==1?-7:(int)n;unsigned count=hardware_calls;CHECK(invoke(a,NativeScriptBinding_ItemRumble,rumble,2,&out)&&hardware_calls==count+1);int c=last_controller,id=last_id,duration=last_duration,priority=last_priority;const void* list=last_commands;it_80273598(a,n%3,(s32)rumble[1].as.i64);CHECK(last_controller==c&&last_id==id&&last_duration==duration&&last_priority==priority&&last_commands==list);++comparisons;}
    rumble[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000001)};rumble[1]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(invoke(a,NativeScriptBinding_ItemRumble,rumble,2,&out)&&last_duration==-1&&last_priority==6);
    for(unsigned n=0;n<3;++n){unsigned count=hardware_calls;rumble[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=n==0?-1:n==1?3:1};fp->x618_player_id=n==2?4:1;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(a,NativeScriptBinding_ItemRumble,rumble,2,&out)&&out.as.u64==99&&hardware_calls==count);}fp->x618_player_id=1;
    // Ineligible/null/non-Fighter owners preserve the native no-op, including
    // otherwise invalid pattern/controller values not reached by native code.
    for(unsigned n=0;n<6;++n){((Item*)a->user_data)->owner=n==0?NULL:n==1?other:owner;fp->x221F_b4=n==2;fp->x221F_b3=n==3;fp->x2224_b2=n==4;player_slots[2].flags.b0=n!=5;fp->x618_player_id=255;rumble[0].as.i64=-1;unsigned count=hardware_calls;CHECK(invoke(a,NativeScriptBinding_ItemRumble,rumble,2,&out)&&hardware_calls==count);}
    fp->x221F_b4=fp->x221F_b3=fp->x2224_b2=0;fp->x618_player_id=1;player_slots[2].flags.b0=1;((Item*)a->user_data)->owner=owner;
    fp->player_id=255;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(a,NativeScriptBinding_ItemRumble,rumble,2,&out)&&out.as.u64==99);fp->x221F_b4=1;CHECK(invoke(a,NativeScriptBinding_ItemRumble,rumble,2,&out));fp->player_id=2;fp->x221F_b4=0;
    for(unsigned mode=1;mode<=10;++mode){feedback_source=item();feedback_owner=fighter(false);((Item*)feedback_source->user_data)->owner=feedback_owner;rumble[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=1};rumble[1]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=2};feedback_mutation=mode;feedback_reuse=mode==3?reuse_item:mode==9?reuse_owner:NULL;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(feedback_source,NativeScriptBinding_ItemRumble,rumble,2,&out)&&feedback_mutation==0&&out.type==NativeScriptType_U64&&out.as.u64==99);
        if(mode==3){CHECK(feedback_replacement==feedback_source);HSD_GObjFree(feedback_replacement);}else if(mode!=2)HSD_GObjFree(feedback_source);if(mode==9){CHECK(feedback_replacement==feedback_owner);HSD_GObjFree(feedback_replacement);}else if(mode!=6)HSD_GObjFree(feedback_owner);feedback_source=NULL;feedback_reuse=NULL;
    }
    for(unsigned mode=10;mode<=12;++mode){clear_quakes();q[0].as.i64=1;CHECK(invoke(a,NativeScriptBinding_CameraQuake,q,4,&out));feedback_source=item();feedback_mutation=mode;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(feedback_source,NativeScriptBinding_CameraQuake,q,4,&out)&&feedback_mutation==0&&out.type==NativeScriptType_U64&&out.as.u64==99);HSD_GObjFree(feedback_source);feedback_source=NULL;if(mode==12)game_camera.quake_gobj=NULL;clear_quakes();}
    for(unsigned source_kind=0;source_kind<2;++source_kind){clear_quakes();feedback_source=source_kind?item():fighter(true);feedback_mutation=source_kind?1:2;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};q[0].as.i64=2;
        CHECK(!invoke(feedback_source,NativeScriptBinding_CameraQuake,q,4,&out)&&feedback_mutation==0&&out.type==NativeScriptType_U64&&out.as.u64==99);if(source_kind)HSD_GObjFree(feedback_source);feedback_source=NULL;
    }
    clear_quakes();feedback_source=fighter(true);feedback_mutation=2;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(nativeScriptFighter_Run(feedback_source,NativeScriptFighter_Tick,NULL,0, NULL,&out)==NativeScriptVM_SourceRetired&&out.type==NativeScriptType_U64&&out.as.u64==99);feedback_source=NULL;clear_quakes();
    for(unsigned bad=0;bad<3;++bad){q[0]=(NativeScriptValue){.type=bad==0?NativeScriptType_F32:NativeScriptType_I32,.as.i64=2};q[1].type=bad==1?NativeScriptType_F64:NativeScriptType_F32;unsigned loaded=model_loads;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!invoke(a,NativeScriptBinding_CameraQuake,q,bad==2?3:4,&out)&&out.type==NativeScriptType_U64&&out.as.u64==99&&model_loads==loaded);
    }q[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=2};q[1].type=NativeScriptType_F32;
    HSD_GObj* faulted=item();for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==faulted)nativeScriptItemActors[i].faulted=true;
    before=model_loads;CHECK(!invoke(faulted,NativeScriptBinding_CameraQuake,q,4,&out)&&out.as.u64==99&&model_loads==before);HSD_GObjFree(faulted);
    NativeScriptRuntimeContext wrong=context(a);wrong.scene_epoch=0;CHECK(!nativeScriptBindingWorldFeedback_Invoke(&wrong,NativeScriptBinding_CameraQuake,q,4,&out,&diagnostic)&&out.as.u64==99);
    fighter_rumble_cases(f,a);
    capture_item_cases();
    capture_item_bindings_cases();
    HSD_GObjFree(f);HSD_GObjFree(a);HSD_GObjFree(owner);HSD_GObjFree(other);CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS world feedback\",\"checks\":%u,\"native_comparisons\":%u,\"quake_models\":%u,\"hardware_requests\":%u}\n",checks,comparisons,model_loads,hardware_calls);return 0;
}
