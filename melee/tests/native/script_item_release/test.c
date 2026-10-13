#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_article_motion.h>
#include "kernels.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage target_package;
#define bank target_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static Fighter fighters[1024];static Item items[1024];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*query_target,*replacement;HSD_GObj*(*target_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons,compile_negatives;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"Item release %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="fighter:\n    behavior independent\nstate int observed\nstate int mode\naction Drop:\n    on item_drop_ext:\n        state.observed = state.observed + 1\n        if state.mode == 6:\n            let int zero = 0\n            state.observed = 1 / zero\n        velocity.x = state.mode\narticle Probe states 1:\n    state int completed\n    on spawned:\n        item.release_attachment(2, -3, 4)\n        item.state.completed = item.state.completed + 1\n    on created:\n        item.release_attachment(2, -3, 4, true)\n        item.state.completed = item.state.completed + 1\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&target_package.program,&diagnostic));
    target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;target_package.assets.facts.moves=layout;
    target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;tables[Ft_Kind_CustomFirst]=&target_package.assets.parts;
    gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=target_package.assets.motions;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned,uint32_t id)
{
    CHECK(fighter_count<1024);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,
        .x8_spawnNum=id,.ft_data=&target_package.assets.character.data,.cur_pos={1.25f,-2.5f,3.75f},.pos_delta={4.25f,-5.5f,6.75f}};
    GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* creator;
static HSD_GObj* item(void)
{
    CHECK(item_count<1024);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100+item_count,.xC4_article_data=&article};
    GObj_InitUserData(o,6,item_remove,i);nativeItem_CaptureCreator(o,creator);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    for(unsigned n=0;n<bank.declarations.value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};}
static uint32_t bits(float value){uint32_t result;memcpy(&result,&value,4);return result;}
static float floating(uint32_t bits){float result;memcpy(&result,&bits,4);return result;}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}

HSD_JObjInfo hsdJObj;static HSD_ClassInfo joint_class;static HSD_JObj joints[2048];static FighterBone bones[4];static HSD_Joint joint_descriptor;
static HSD_GObj* released,*holder,*other,*held,*dead;static unsigned destructive,trace_count,vm_calls,retirements,admissions;typedef struct{int event,arg;float a,b,c;}Trace;static Trace trace[128];
static void record(int event,int arg,float a,float b,float c){CHECK(trace_count<128);trace[trace_count++]=(Trace){event,arg,a,b,c};}
void target_guard_probe(NativeScriptRuntimeContext* c){(void)c;}
static void close(HSD_GObj* o){o->obj_kind=HSD_GOBJ_OBJ_NONE;o->hsd_obj=NULL;HSD_GObjFree(o);}
static void set(HSD_GObj* o,const char* name,int n){NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=n};field(o,name,&v);}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* a,uint32_t n,const NativeScriptValue* v,NativeScriptDiagnostic* d){(void)a;(void)d;if(n||s!=NativeScriptBinding_VelocityX||v->type!=NativeScriptType_F32||c->source_class!=NativeCallbackSource_Fighter)return false;record(9,0,v->as.f32,0,0);((Fighter*)((HSD_GObj*)c->source)->user_data)->self_vel.x=v->as.f32;
unsigned mode=destructive;destructive=0;if(mode==1||mode==2){dead=mode==1?released:holder;close(dead);}if(mode==3){target_package.program.identity++;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);}if(mode==4)bank.storage_layout.identity++;if(mode==5)bank.storage_parameters.identity++;return true;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* j){CHECK(j);j->flags|=JOBJ_MTX_DIRTY;}
void(HSD_JObjSetMtxDirty)(HSD_JObj* j){HSD_JObjSetMtxDirtySub(j);}
void it_80275070(Item_GObj* o,s32 n){CHECK(o==released);record(1,n,0,0,0);}
bool it_8026B6C8(Item_GObj* o){CHECK(o==released);return false;}
void it_8026B390(Item_GObj* o){CHECK(o==released);record(2,0,0,0,0);}
void it_802756E0(Item_GObj* o){CHECK(o==released);record(3,0,0,0,0);((Item*)o->user_data)->xDC8_word.flags.x1F=1;}
f32 it_80274990(Item_GObj* o){CHECK(o==released);record(4,0,0,0,0);return .25f;}
void lb_8000B804(HSD_JObj* j,HSD_Joint* d){if(!j||!d)return;record(5,0,0,0,0);j->translate=d->position;}
void Item_8026849C(Item_GObj* o){CHECK(o==released);record(6,0,0,0,0);}
int it_80274658(Item_GObj* o,f32 n){CHECK(o==released);record(7,0,n,0,0);return 1;}
void mpCollSetFacingDir(CollData* c,int n){CHECK(c);record(8,n,0,0,0);}
bool itIsHeavy(Item_GObj* o){CHECK(o==released);return ((Item*)o->user_data)->hold_kind==4;}
HSD_JObj* it_80272C90(Item_GObj* o){CHECK(o==released);return o->hsd_obj;}
void lb_8000B1CC(HSD_JObj* j,Vec3* offset,Vec3* p){CHECK(j&&offset&&p);record(10,0,offset->x,offset->y,offset->z);*p=(Vec3){j->translate.x+offset->x,j->translate.y+offset->y,j->translate.z+offset->z};}
void it_80275BC8(Item_GObj* o,HSD_GObj* owner){CHECK(o==released&&owner);record(11,owner==creator?1:2,0,0,0);}
void it_80274DAC(Item_GObj* o){CHECK(o==released);record(12,0,0,0,0);}
void it_8027B070(Item_GObj* o,Fighter_GObj* owner){CHECK(o==released&&owner);record(13,0,0,0,0);((Item*)o->user_data)->xD88_attackID=((Fighter*)owner->user_data)->x2068_attackID;}
void Item_8026B074(Item* i){CHECK(i);record(14,0,0,0,0);}
void it_802762BC(Item* i){CHECK(i);record(15,0,0,0,0);}
void it_8027B4A4(Fighter_GObj* o,Item_GObj* i){CHECK(o&&i==released);record(16,0,0,0,0);}
f32 it_802758D4(Item_GObj* o){CHECK(o==released);record(17,0,0,0,0);return 2;}
void it_8027B378(Fighter_GObj* o,Item_GObj* i,f32 v){CHECK(o&&i==released);record(18,0,v,0,0);}
bool ftCo_800C5240(Fighter_GObj* o){CHECK(o);return false;}
void ftCo_800C5500(Fighter_GObj* o){(void)o;abort();}
void pl_8003EA08(int id,int b){record(19,id,b,0,0);}
Fighter_ItemEvent ftData_OnItemDropExt[Ft_Kind_RuntimeCapacity];
static void begin(unsigned variant){dead=NULL;destructive=0;trace_count=0;creator=fighter(true,100+fighter_count);other=fighter(true,100+fighter_count);holder=variant&1?other:creator;Fighter* fp=holder->user_data;Fighter* cp=creator->user_data;cp->parts=fp->parts=bones;for(unsigned n=0;n<4;++n){bones[n].joint=&joints[2000+n];HSD_CLASS_METHOD(bones[n].joint)=&joint_class;bones[n].joint->translate=(Vec3){10+n,20+n,30+n};}
released=item();held=item();Item* ip=released->user_data;released->obj_kind=HSD_GObj_JObjKind;released->hsd_obj=&joints[item_count];HSD_JObj* j=released->hsd_obj;*j=(HSD_JObj){.scale={1,1,1},.translate={1,2,3}};HSD_CLASS_METHOD(j)=&joint_class;
ip->xCC_item_attr=&attributes;ip->xC8_joint=&joint_descriptor;ip->scl=1;ip->owner=variant&2?creator:holder;ip->x51C=variant&2?holder:NULL;ip->xDC8_word.flags.x0=(variant&2)!=0;ip->hold_kind=variant&4?8:0;ip->xDC4=2;ip->facing_dir=variant&8?-1:1;ip->xDC8_word.flags.x19=(variant&8)!=0;fp->item_gobj=variant&16?held:released;fp->x1984_heldItemSpec=released;fp->x2068_attackID=99;attributes.x4_throw_speed_mul=variant&32?2:.5f;set(holder,"mode",0);set(holder,"observed",0);}
static void end(void){if(released!=dead)close(released);if(held!=dead)close(held);if(creator!=dead)close(creator);if(other!=dead)close(other);CHECK(nativeScriptRuntime_Idle());}
typedef struct{Item item;Vec3 translate;Trace calls[128];unsigned count;bool held_source,held_other,held_null;int observed;}Result;
static Result take(void){Result r={.item=*(Item*)released->user_data,.translate=((HSD_JObj*)released->hsd_obj)->translate,.count=trace_count,.held_source=((Fighter*)creator->user_data)->item_gobj==released,.held_other=((Fighter*)creator->user_data)->item_gobj==held,.held_null=((Fighter*)creator->user_data)->item_gobj==NULL,.observed=(int)field(holder,"observed",NULL).as.i64};memcpy(r.calls,trace,sizeof(trace));return r;}
static void healthy(void){for(unsigned v=0;v<65;++v)for(unsigned preserve=0;preserve<2;++preserve){prepare();model.x0_joint=v==64?NULL:&joint_descriptor;begin(v);if(v==64)((Item*)released->user_data)->xC8_joint=NULL;uint32_t hook=preserve?NativeScriptItemHook_Created:NativeScriptItemHook_Spawned;Item initial=*(Item*)released->user_data;HSD_JObj initial_joint=*(HSD_JObj*)released->hsd_obj;HSD_GObj* saved=((Fighter*)creator->user_data)->item_gobj;HSD_GObj* held_initial=((Fighter*)holder->user_data)->item_gobj;HSD_GObj* aux=((Fighter*)holder->user_data)->x1984_heldItemSpec;
Vec3 velocity={2,-3,4};bool restore=preserve&&initial.owner==creator;reference_it_8027429C(released,&velocity);if(restore)((Fighter*)creator->user_data)->item_gobj=saved;Result expected=take();
*(Item*)released->user_data=initial;*(HSD_JObj*)released->hsd_obj=initial_joint;((Fighter*)holder->user_data)->item_gobj=held_initial;((Fighter*)holder->user_data)->x1984_heldItemSpec=aux;set(holder,"observed",0);trace_count=0;NativeScriptValue result;CHECK(nativeScriptItem_RunHook(released,hook,NULL,0,&result)==NativeScriptVM_Ok);++vm_calls;CHECK(field(released,"completed",NULL).as.i64==1);Result actual=take();CHECK(actual.count==expected.count&&!memcmp(actual.calls,expected.calls,actual.count*sizeof(Trace)));CHECK(!memcmp(&actual.item,&expected.item,sizeof(Item)));CHECK(!memcmp(&actual.translate,&expected.translate,sizeof(Vec3)));CHECK(actual.held_source==expected.held_source&&actual.held_other==expected.held_other&&actual.held_null==expected.held_null&&actual.observed==expected.observed);++comparisons;end();}}
static void retirement(void){for(unsigned mode=1;mode<=6;++mode){prepare();model.x0_joint=&joint_descriptor;begin(0);set(holder,"mode",mode);destructive=mode==6?0:mode;NativeScriptRuntimeContext c=context(released);NativeScriptValue r={.type=NativeScriptType_I32,.as.i64=991},args[4]={{.type=NativeScriptType_F32,.as.f32=2},{.type=NativeScriptType_F32,.as.f32=-3},{.type=NativeScriptType_F32,.as.f32=4},{.type=NativeScriptType_Bool,.as.u64=1}};
CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemReleaseAttachmentPreserveHeld,NULL,0,args,4,&r,&diagnostic));CHECK(r.type==NativeScriptType_I32&&r.as.i64==991);for(unsigned n=0;n<trace_count;++n)CHECK(trace[n].event!=13&&trace[n].event!=14&&trace[n].event!=15&&trace[n].event!=16&&trace[n].event!=17&&trace[n].event!=18);CHECK(mode==6||destructive==0);end();++retirements;}}
static void negative(void){prepare();model.x0_joint=&joint_descriptor;begin(0);NativeScriptRuntimeContext c=context(released);NativeScriptValue r={.type=NativeScriptType_I32,.as.i64=991},args[4]={{.type=NativeScriptType_F32,.as.f32=2},{.type=NativeScriptType_F32,.as.f32=-3},{.type=NativeScriptType_F32,.as.f32=4},{.type=NativeScriptType_I32,.as.i64=1}};
CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemReleaseAttachmentPreserveHeld,NULL,0,args,4,&r,&diagnostic));++admissions;args[3]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};args[1].as.f32=NAN;CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemReleaseAttachmentPreserveHeld,NULL,0,args,4,&r,&diagnostic));++admissions;args[1].as.f32=-3;c.generation++;CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemReleaseAttachmentPreserveHeld,NULL,0,args,4,&r,&diagnostic));++admissions;CHECK(trace_count==0&&r.as.i64==991);c=context(released);Item* ip=released->user_data;ip->xC8_joint=NULL;
CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemReleaseAttachmentPreserveHeld,NULL,0,args,4,&r,&diagnostic));++admissions;CHECK(trace_count==0&&r.as.i64==991);end();
const char* bad[]={"item.release_attachment(0, 0, 0, 1)","item.release_attachment(0, 0, 0, 1.0)","item.release_attachment(0, 0)","item.release_attachment(0, 0, 0, true, false)"};
for(unsigned n=0;n<4;++n){char text[500];snprintf(text,sizeof(text),"fighter:\n    behavior independent\narticle Probe states 1:\n    on spawned:\n        %s\n    state 0 anim -1:\n",bad[n]);NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));++compile_negatives;}}
int main(void){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);HSD_CLASS_INFO(&hsdJObj)->head.flags=1;joint_class.head.obj_size=sizeof(HSD_JObj);joint_class.head.parent=HSD_CLASS_INFO(&hsdJObj);joint_descriptor.scale=(Vec3){1,1,1};model.x0_joint=&joint_descriptor;healthy();retirement();negative();printf("{\"status\":\"PASS Item release\",\"checks\":%u,\"native_pairs\":%u,\"vm_calls\":%u,\"retirement_cases\":%u,\"admission_cases\":%u,\"compiler_negatives\":%u}\n",checks,comparisons,vm_calls,retirements,admissions,compile_negatives);return 0;}
