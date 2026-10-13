#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include "native_leaves.h"
#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_binding_targets.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#pragma clang section data = ""

NativeSourcePackage packages[2];
static ftCommonData common_data;ftCommonData* p_ftCommonData=&common_data;
static struct ftCo_DatAttrs native_attributes[2];static struct ftData_x8 native_parts[2];
static Article article;static ItemAttr attr;static ItemModelDesc model;
static Fighter fighters[128],substitute;static Item items[64];
static FighterBone bones[128][140];static HSD_JObj joints[128][4],replacement_joint;
static Vec3 scales[128][4];static HSD_RObj observer;
static FighterPartsTable stock_parts,*tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=tables;
struct UnkCostumeList CostumeListsForeachCharacter[Ft_Kind_RuntimeCapacity];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
HSD_JObjInfo hsdJObj;
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*query_target,*replacement;HSD_GObj*(*target_reuse)(void);
static unsigned checks,cases,comparisons,vm_calls,negative_count,native_visits,fighter_count,item_count;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"target bone %u: %s (%u:%u %s)\n",__LINE__,#x,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}while(0)
static NativeScriptValue integer(int64_t x){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=x};}
static NativeScriptValue floating(float x){return(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=x};}
static uint32_t bits(float x){uint32_t result;memcpy(&result,&x,4);return result;}
static uint32_t hash(const char* s){uint32_t h=2166136261u;while(*s){h^=(unsigned char)*s++;h*=16777619u;}return h;}
static void prepare(void)
{
    const char* source="hitbox_definition OwnedCapsule:\n    bone 2\n    damage 12\n    radius 1200 * 0.003906\n    item_interaction true\n    element 8\n    sound_severity 1\n    sound_kind 2\n    aerial false\nhitbox_definition EndpointCapsule:\n    bone 139\n    damage 0\n    radius 1200 * 0.003906\n    item_interaction true\n    element 8\n    sound_severity 1\n    sound_kind 2\n    aerial false\nfighter:\n    behavior independent\nstate int handle\nstate int part\nstate int zero\nstate int nearest\nstate bool found\nstate float x\nstate float y\nstate float z\nstate float radius\nstate float scale\nstate bool guard\nfunc QueryShield():\n    state.found = targets.shield(state.handle, state.x, state.y, state.z, state.radius)\nfunc QueryCamera():\n    state.found = targets.read_camera_target_position(state.handle, state.x, state.y, state.z)\nfunc CreateOwned() -> bool:\n    return hitbox.create(state.part, OwnedCapsule)\nfunc CreateEndpoint() -> bool:\n    return hitbox.create(state.part, EndpointCapsule)\nfunc QueryNearest():\n    state.nearest = targets.nearest_available_fighter()\nfunc QueryScalar():\n    state.scale = targets.model_scale(state.handle)\n    state.guard = targets.guard_active(state.handle)\naction Query:\n    on tick:\n        state.found = targets.read_bone_position(state.handle, state.part, state.x, state.y, state.z)\nfunc Fault():\n    state.part = 5 / state.zero\narticle Probe states 1:\n    resources capacity 2 reserve_part 139\n    state int handle\n    state int part\n    state int zero\n    state bool found\n    state float x\n    state float y\n    state float z\n    state float radius\n    state float scale\n    state bool guard\n    on signal QueryOverlap:\n        item.state.found = targets.overlaps_hitbox(item.state.handle, item.state.part)\n    on signal QueryControlID:\n        if attachment_owner.exists:\n            item.state.handle = attachment_owner.fighter.creation_id\n    on signal QueryOpponentAhead:\n        item.state.found = targets.read_opponent_ahead(item.state.x, item.state.y, item.state.z)\n    on signal QueryItemAhead:\n        item.state.found = targets.read_lockable_item_ahead(item.state.x, item.state.y, item.state.z)\n    on signal QueryShield:\n        item.state.found = targets.shield(item.state.handle, item.state.x, item.state.y, item.state.z, item.state.radius)\n    on signal QueryCamera:\n        item.state.found = targets.read_camera_target_position(item.state.handle, item.state.x, item.state.y, item.state.z)\n    on signal QueryScalar:\n        item.state.scale = targets.model_scale(item.state.handle)\n        item.state.guard = targets.guard_active(item.state.handle)\n    on signal Query:\n        item.state.found = targets.read_bone_position(item.state.handle, item.state.part, item.state.x, item.state.y, item.state.z)\n    on signal Fault:\n        item.state.part = 5 / item.state.zero\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attr,.x10_modelDesc=&model};
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
    for(unsigned i=0;i<2;++i) {
        NativeSourcePackage* p=&packages[i];p->address=p;p->target=(FighterKind)(Ft_Kind_CustomFirst+i);
        CHECK(nativeSourceBank_Allocate(&p->compiler,&diagnostic));
        p->assets.facts.joints=4;metadata.source_facts=&p->assets.facts;
        p->assets.facts.moves=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&p->assets.facts.moves,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
        const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&p->compiler.bank,&request,&accepted,&diagnostic));
        CHECK(nativeScriptFighter_PrepareProgram(p->target,&p->compiler.bank,&p->assets.facts.moves,&metadata,&p->program,&diagnostic));
        p->assets.facts.joints=4;p->assets.parts=(FighterPartsTable){.parts_num=4,.part_to_joint=p->assets.part_to_joint,.joint_to_part=p->assets.joint_to_part};
        native_attributes[i].camera_zoom_target_bone=2;native_parts[i].x11=2;
        p->assets.character.data.x0=&native_attributes[i];p->assets.character.data.x8=&native_parts[i];
        p->ready=p->assets.ready=true;tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;
        ftData_CharacterStateTables[p->target]=p->assets.motions;nativeScriptFighter_PublishProgram(p->target,&p->program);
    }
    stock_parts.parts_num=4;tables[Ft_Kind_Popo]=&stock_parts;
    memset(&hsdJObj,0,sizeof(hsdJObj));HSD_CLASS_INFO(&hsdJObj)->head.flags=1;
    HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);hsdJObj.make_mtx=HSD_JObjMakeMatrix;
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(int package,uint32_t id)
{
    CHECK(fighter_count<128);unsigned n=fighter_count++;HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
    Fighter* f=&fighters[n];FighterKind kind=package<0?Ft_Kind_Popo:packages[package].target;
    *f=(Fighter){.gobj=o,.kind=kind,.behavior_kind=kind,.x8_spawnNum=id,.ft_data=&packages[package<0?0:package].assets.character.data,.parts=bones[n]};
    GObj_InitUserData(o,4,fighter_remove,f);
    for(unsigned j=0;j<4;++j) {
        HSD_JObj* obj=&joints[n][j];*obj=(HSD_JObj){.flags=JOBJ_MTX_DIRTY,.scale={1.5f,.75f,-2},.translate={1.25f+j,-2.5f+j,3.75f-j},.rotate={.x=.2f,.y=-.35f,.z=.6f},.scl=&scales[n][j]};
        HSD_CLASS_METHOD(obj)=HSD_CLASS_INFO(&hsdJObj);bones[n][j].joint=obj;
        if(j){obj->parent=&joints[n][j-1];joints[n][j-1].child=obj;}
    }
    o->hsd_obj=&joints[n][0];o->obj_kind=HSD_GObj_JObjKind;
    if(package>=0)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(void)
{
    CHECK(item_count<64);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(o);
    const NativeScriptItemProgram* p=&nativeScriptFighter_Program(packages[0].target)->items.programs[0];
    Item* i=&items[item_count++];*i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100+item_count,.xC4_article_data=&article};
    GObj_InitUserData(o,6,item_remove,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    NativeScriptRuntimeContext c=context(o);NativeScriptReceiverView view;
    CHECK(c.source_class==NativeCallbackSource_Fighter?nativeScriptFighter_View(o,c.generation,&view):nativeScriptItem_View(o,c.generation,&view));
    const NativeScriptProgramIndex* d=view.program;const NativeScriptCompileBank* bank=c.source_class==NativeCallbackSource_Fighter?nativeScriptFighter_Program(((Fighter*)o->user_data)->kind)->bank:nativeScriptItem_Spec(o)->bank;
    for(unsigned n=0;n<d->value_count;++n)if(d->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank->tree,d->values[n].name,name)) {
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));
        CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static void arguments(NativeScriptValue* a,uint32_t handle,unsigned part)
{a[0]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=handle};a[1]=integer(part);for(unsigned i=2;i<5;++i)a[i]=floating(90+i);}
static bool invoke(HSD_GObj* caller,NativeScriptValue* a,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(caller);return nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsReadBonePosition,a,5,out,&diagnostic);}
void target_guard_probe(NativeScriptRuntimeContext* c){(void)c;++guard_calls;}
void bone_native_observe(void)
{
    ++native_visits;if(!mutation)return;unsigned m=mutation;mutation=0;
    Fighter* f=query_target->user_data;
    if(m==1)++f->x8_spawnNum;
    if(m==2){substitute=*f;query_target->user_data=&substitute;}
    if(m==3)f->parts[2].joint=&replacement_joint;
    if(m==4)nativeScriptFighter_Detach(query_target);
    if(m==5){HSD_GObjFree(query_target);replacement=fighter(1,999);}
    if(m==6){HSD_GObjFree(query_caller);replacement=fighter(0,998);}
    if(m==7){NativeScriptValue r;CHECK(nativeScriptItem_RunSignal(query_caller,hash("Fault"),0,&r)==NativeScriptVM_Error);}
    if(m==8){const NativeScriptCompileBank* b=nativeScriptFighter_Program(((Fighter*)query_caller->user_data)->kind)->bank;uint32_t id=0;for(;id<b->declarations.function_count;++id)if(nativeScriptSource_SpanEquals(&b->tree,b->declarations.functions[id].name,"Fault"))break;NativeScriptValue r;CHECK(nativeScriptFighter_Helper(query_caller,((Fighter*)query_caller->user_data)->x8_spawnNum,id,NULL,0,&r,&diagnostic)==NativeScriptVM_Error);}
    if(m==9)query_target->classifier=HSD_GOBJ_CLASS_ITEM;
    if(m==10)query_target->hsd_obj=&replacement_joint;
    if(m==11)f->parts[2].joint->parent=NULL;
}
void HSD_RObjUpdateAll(HSD_RObj* r,void* o,HSD_ObjUpdateFunc f){(void)r;(void)o;(void)f;bone_native_observe();}
void* HSD_VecAlloc(void){abort();}
void HSD_VecFree(void* p){(void)p;abort();}
HSD_Rvalue* HSD_RvalueAlloc(void){abort();}
HSD_RObj* HSD_RObjGetByType(HSD_RObj* r,u32 t,u32 s){(void)r;(void)t;(void)s;abort();}
int HSD_RObjGetGlobalPosition(HSD_RObj* r,int m,Vec3* p){(void)r;(void)m;(void)p;abort();}
void JObjUpdateFunc(void* o,enum_t t,HSD_ObjData* v){(void)o;(void)t;(void)v;abort();}
bool ftLib_800870BC(Fighter_GObj* o,GXColor* c){(void)o;(void)c;abort();}
static void compare(HSD_GObj* caller,HSD_GObj* target,unsigned part)
{
    Fighter* f=target->user_data;Vec3 native;CHECK(nativeFighterTarget_BonePosition(f->x8_spawnNum,part,&native));
    // Re-dirty the same static graph to compare actual recomputation rather than
    // solely a cached result. Scale vectors are prepared fixture storage.
    for(unsigned j=0;j<4;++j)f->parts[j].joint->flags|=JOBJ_MTX_DIRTY;
    NativeScriptValue a[5],out;arguments(a,f->x8_spawnNum,part);CHECK(invoke(caller,a,&out)&&out.as.u64);
    CHECK(bits(a[2].as.f32)==bits(native.x)&&bits(a[3].as.f32)==bits(native.y)&&bits(a[4].as.f32)==bits(native.z));++comparisons;
}
#include "geometry_cases.h"
#include "nearest_cases.h"
#include "ahead_cases.h"
#include "overlap_cases.h"
#include "hitbox_create_cases.h"

static void compiler_negatives(void)
{
    const char* bad[]={"targets.model_scale(1.5)","targets.guard_active(1.5)","targets.shield(1.5, state.x, state.y, state.z, state.radius)","targets.shield(1, state.x, state.x, state.z, state.radius)","targets.shield(1, state.x, state.y, state.z, state.handle)","targets.shield(1, state.x, state.y, state.z)","targets.read_camera_target_position(1.5, state.x, state.y, state.z)","targets.read_camera_target_position(1, state.x, state.x, state.z)","targets.read_camera_target_position(1, state.x, state.y, state.handle)","targets.read_camera_target_position(1, state.x, state.y, state.z, state.radius)","targets.read_bone_position(1.5, 1, state.x, state.y, state.z)","targets.read_bone_position(1, 1.5, state.x, state.y, state.z)","targets.read_bone_position(1, 1, state.x, state.x, state.z)","targets.read_bone_position(1, 1, state.x, state.y)","targets.read_bone_position(1, 1, state.x, state.y, state.handle)","targets.read_bone_position(1, 1, state.x, state.y, state.z, 0)"};
    NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){char s[1024];snprintf(s,sizeof(s),"fighter:\n    behavior independent\nstate float x\nstate float y\nstate float z\nstate float radius\nstate int handle\nfunc Bad():\n    %s\n",bad[i]);NativeScriptCompileRequest r={.source=s,.source_bytes=(uint32_t)strlen(s)};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&scratch.bank,&r,&accepted,&diagnostic));++negative_count;}
    nativeSourceBank_Release(&scratch);
}
int main(void)
{
    prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);compiler_negatives();
    HSD_GObj* caller=fighter(0,21),*article_caller=item(),*owned=fighter(1,UINT32_C(0x80000005)),*stock=fighter(-1,22);
    hitbox_create_cases(caller,article_caller);
    nearest_cases(caller,article_caller,owned,stock);
    ahead_cases(caller,article_caller,owned,stock);
    overlap_cases(caller,article_caller,owned,stock);
    geometry_cases(caller,article_caller,owned,stock);
    for(unsigned k=0;k<2;++k)for(unsigned p=0;p<4;++p){compare(k?article_caller:caller,owned,p);compare(k?article_caller:caller,stock,p);++cases;}
    for(unsigned k=0;k<2;++k){HSD_GObj* c=k?article_caller:caller;NativeScriptValue v=integer(INT32_MIN+5);field(c,"handle",&v);v=integer(2);field(c,"part",&v);NativeScriptValue out;
        CHECK(k?nativeScriptItem_RunSignal(c,hash("Query"),0,&out)==NativeScriptVM_Ok:nativeScriptFighter_Run(c,NativeScriptFighter_Tick,NULL,0,NULL,&out)==NativeScriptVM_Ok);++vm_calls;
        CHECK(field(c,"found",NULL).as.u64);Vec3 native;CHECK(nativeFighterTarget_BonePosition(UINT32_C(0x80000005),2,&native));CHECK(bits(field(c,"x",NULL).as.f32)==bits(native.x));
    }
    for(unsigned mode=0;mode<7;++mode){NativeScriptValue a[5],out;arguments(a,22,2);Fighter* f=stock->user_data;HSD_JObj* save=f->parts[2].joint;
        if(mode==0)a[0].as.u64=0;if(mode==1)a[0].as.u64=100000;if(mode==2)a[1]=integer(-1);if(mode==3)a[1].as.i64=4;
        if(mode==4)a[1]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000002)};if(mode==5)f->parts[2].joint=NULL;
        if(mode==6){a[0].type=NativeScriptType_I64;a[0].as.i64=INT64_C(-2147483643);a[1].as.i64=4;}
        CHECK(invoke(caller,a,&out)&&!out.as.u64&&a[2].as.f32==92&&a[3].as.f32==93&&a[4].as.f32==94);f->parts[2].joint=save;++cases;
    }
    const NativeScriptValue invalid[]={floating(22),{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000016)},{.type=NativeScriptType_I32,.as.i64=INT64_C(0x100000016)},{.type=NativeScriptType_Bool,.as.u64=2}};
    for(unsigned arg=0;arg<2;++arg)for(unsigned n=0;n<4;++n){if(arg&&n==1)continue;NativeScriptValue a[5],old[5],out={.type=NativeScriptType_U64,.as.u64=99};arguments(a,22,2);a[arg]=invalid[n];memcpy(old,a,sizeof(a));CHECK(!invoke(caller,a,&out));CHECK(out.as.u64==99&&out.type==NativeScriptType_U64&&!memcmp(old,a,sizeof(a)));++negative_count;}
    // A known native dummy does not make callback or vector destinations safe.
    {static const unsigned channels[]={4,20,29,30,39,40,41,42,50,51,52,53};
     Fighter* f=stock->user_data;HSD_JObj* selected=f->parts[2].joint;unsigned before=native_visits;
     bone_install_dummy(&observer);selected->robj=&observer;
     for(unsigned n=0;n<sizeof(channels)/sizeof(*channels);++n){selected->flags|=JOBJ_MTX_DIRTY;observer.flags=0x80000000|channels[n];
       NativeScriptValue a[5],old[5],out={.type=NativeScriptType_U64,.as.u64=99};arguments(a,22,2);memcpy(old,a,sizeof(a));
       CHECK(!invoke(caller,a,&out));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(a,old,sizeof(a)));CHECK(native_visits==before);++negative_count;}
     selected->robj=NULL;}
    // An initially faulted target remains readable while its owned data is live.
    {const NativeScriptCompileBank* b=nativeScriptFighter_Program(packages[1].target)->bank;uint32_t id=0;for(;id<b->declarations.function_count;++id)if(nativeScriptSource_SpanEquals(&b->tree,b->declarations.functions[id].name,"Fault"))break;NativeScriptValue out;CHECK(nativeScriptFighter_Helper(owned,UINT32_C(0x80000005),id,NULL,0,&out,&diagnostic)==NativeScriptVM_Error);compare(caller,owned,2);}
    for(unsigned m=1;m<=11;++m){query_caller=m==7?item():fighter(0,100+fighter_count);query_target=fighter(1,200+fighter_count);replacement=NULL;mutation=m;
        bone_install_dummy(&observer);((Fighter*)query_target->user_data)->parts[2].joint->robj=&observer;
        NativeScriptValue a[5],old[5],out={.type=NativeScriptType_U64,.as.u64=99};arguments(a,((Fighter*)query_target->user_data)->x8_spawnNum,2);memcpy(old,a,sizeof(a));
        CHECK(!invoke(query_caller,a,&out));CHECK(!mutation&&out.as.u64==99&&out.type==NativeScriptType_U64&&!memcmp(old,a,sizeof(a)));++cases;
        if(m==5)CHECK(replacement==query_target);if(m==6)CHECK(replacement==query_caller);
        if(m!=5)HSD_GObjFree(query_target);if(m!=6)HSD_GObjFree(query_caller);if(replacement)HSD_GObjFree(replacement);
    }
    HSD_GObjFree(stock);HSD_GObjFree(owned);HSD_GObjFree(article_caller);HSD_GObjFree(caller);CHECK(nativeScriptRuntime_Idle());
    printf("{\"status\":\"PASS target bone queries\",\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u,\"vm_calls\":%u,\"negative_cases\":%u,\"controlled_native_observations\":%u,\"nearest_comparisons\":%u,\"hitbox_create_comparisons\":%u}\n",checks,cases,comparisons,vm_calls,negative_count,native_visits,nearest_comparisons,hitbox_create_comparisons);return 0;
}
