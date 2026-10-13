#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ft_0819.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_07C1.h>
#include <melee/ft/ft_07C6.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ft_0C8C.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftCommon/ftCo_09F4.h>
#include <melee/ft/ftmetal.h>
#include <melee/ft/ft_0DF0.h>
#include <melee/ft/ft_0C31.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/efsync.h>
#include <melee/ef/eflib.h>
#include <melee/ef/inlines.h>
#include <melee/ef/types.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/generator.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/it/native_item_owner.h>
#include <melee/pl/player.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdemo.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ft_0C88.h>
#include <melee/pl/types.h>
#include <melee/lb/lbanim.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_rollback_state.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma clang section data = ""
static unsigned checks,cases,normal_loads,demo_loads,callbacks,reset_calls,tail_calls,camera_calls,effect_calls,rig_calls;
static unsigned trace[128],trace_count,mutation,mutation_phase;
static HSD_GObj* replacement;
static EF_Effect spawned[2];
static unsigned spawn_phase,effect_retired;
EF_ParamEntry efLib_ParamTable[8];
static NativeScriptDiagnostic diagnostic;
static void step(unsigned n);
static FigaTree tree={.frames=20};
static HSD_JObj root,alternate,endpoint;
NativeSourcePackage constructor_package;
#define bank constructor_package.compiler.bank
#define owned_data constructor_package.assets.character.data
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_OnDeath[Ft_Kind_RuntimeCapacity];
static ftCommonData common;
ftCommonData* p_ftCommonData=&common;
static struct Fighter_WaitAnimData animations[32];
static FighterPartsTable part_table={.parts_num=60};
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=tables;
HSD_ObjAllocData fighter_x2040_alloc_data;
#pragma clang section data = ".mstate$m"
u32 Fighter_804D64F8;
#pragma clang section data = ""
#define g_spawnNumCounter Fighter_804D64F8
Event ftData_Table_Unk1[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_OnLoad[Ft_Kind_RuntimeCapacity],ftData_OnUserDataRemove[Ft_Kind_RuntimeCapacity];
u8 ftData_UnkBytePerCharacter[Ft_Kind_RuntimeCapacity];
MotionState ftData_803C52A0[14];MotionState* ftData_UnkMotionStates0[Ft_Kind_RuntimeCapacity];
static HSD_GObjEvent on_create_fighter[16];
void (*ftData_UnkDemoCallbacks0[Ft_Kind_RuntimeCapacity])(int,int*,int*);
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u: %s (%s)\n",(unsigned)__LINE__,#x,diagnostic.message);assert(x);}}while(0)
void OSReport(char* format,...){(void)format;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason){fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
__declspec(noreturn) void HSD_Panic(char* file,u32 line,char* reason){__assert(file,line,reason);}
static void step(unsigned n){CHECK(trace_count<128);trace[trace_count++]=n;}
void HSD_ObjAllocInit(HSD_ObjAllocData* pool,size_t size,u32 align){CHECK(pool==&fighter_x2040_alloc_data&&size==32*sizeof(void*)&&align==4);}
void* HSD_ObjAlloc(HSD_ObjAllocData* pool){(void)pool;CHECK(false);return NULL;}
void HSD_ObjFree(HSD_ObjAllocData* pool,void* pointer){(void)pool;(void)pointer;CHECK(false);}
void ft_800852B0(void){}
void Fighter_LoadCommonData(void){}
void ft_8008549C(void){}
void ftCo_8009F4A4(void){}
void ftCo_800C8064(void){}
void ftCo_800C8F6C(void){}
void ftData_8008572C(FighterKind kind){CHECK(kind==Ft_Kind_CustomFirst||kind==Ft_Kind_Samus);}
void ftData_80085820(FighterKind kind,int color){(void)color;CHECK(kind==Ft_Kind_CustomFirst||kind==Ft_Kind_Samus);}
void Fighter_UnkInitLoad_80068914(Fighter_GObj* object,struct plAllocInfo* input)
{
    Fighter* fighter=object->user_data;NativeFighterPoolSlot* own=nativeFighterPool_Slot(fighter);CHECK(own);
    CHECK(fighter->dat_attrs_backup==own->attributes&&fighter->parts==NULL&&fighter->x203C.data==NULL);
    fighter->gobj=object;fighter->kind=input->internal_id;fighter->player_id=input->slot;
    fighter->x8_spawnNum=0;fighter->behavior_kind=fighter->kind;fighter->x221F_b4=input->b0;fighter->ft_data=&owned_data;fighter->x24=animations;fighter->x34_scale=(Vec3){1,1,1};fighter->smash_attrs.x2135=-1;fighter->x61C=-1;
}
void Fighter_UnkUpdateCostumeJoint_800686E4(Fighter_GObj* object){HSD_GObjObject_80390A70(object,HSD_GObj_JObjKind,&root);}
void ftData_80085B10(Fighter* fighter){CHECK(fighter->parts==NULL);fighter->x59C=fighter->x5A0=NULL;++normal_loads;}
void ftData_80085B98(Fighter* fighter,int first,int last){CHECK(fighter->parts==NULL&&first==0&&last==9);fighter->x59C=fighter->x5A0=NULL;++demo_loads;}
FigaTree* ftData_80085E50(Fighter* fighter,int motion){(void)fighter;(void)motion;return &tree;}
float lbAnim_8001E8F8(FigaTree* selected){CHECK(selected==&tree);return selected->frames;}
Fighter_Part ftParts_GetBoneIndex(Fighter* fighter,Fighter_Part part){(void)fighter;return part;}
void ftParts_SetupParts(Fighter_GObj* object){Fighter* f=object->user_data;CHECK(f->parts==nativeFighterPool_Slot(f)->parts);f->parts[0].joint=&root;f->parts[139].joint=&endpoint;}
void ftAnim_80070308(Fighter_GObj* object){CHECK(((Fighter*)object->user_data)->x203C.data==NULL);}
void ftPartsPObjSetDefaultClass(void){}
void ftPartsPObjClearDefaultClass(void){}
void ft_800C85B8(Fighter_GObj* object){Fighter* f=object->user_data;CHECK(f->x203C.data==nativeFighterPool_Slot(f)->secondary_dobjs);}
void Fighter_80068E64(Fighter_GObj* object){(void)object;}
void ftParts_800749CC(Fighter_GObj* object){(void)object;}
void ftAnim_8007077C(Fighter_GObj* object){(void)object;}
void ftCo_8009CF84(Fighter* fighter){(void)fighter;}
void ftAnim_8006FE48(Fighter_GObj* object){((Fighter*)object->user_data)->x8AC_animSkeleton=&alternate;}
void Fighter_UnkUpdateVecFromBones_8006876C(Fighter* fighter){(void)fighter;}
void ftCo_8009F578(Fighter* fighter){(void)fighter;}
void efAsync_LoadSync(int resource_bank){(void)resource_bank;CHECK(false);}
void ftColl_8007B320(Fighter_GObj* object){(void)object;}
static CmSubject camera;
CmSubject* Camera_80029020(void){++camera_calls;return &camera;}
void lbShadow_8000ED54(LbShadow* shadow,HSD_JObj* joint){(void)shadow;CHECK(joint==&root);}
s32 Player_GetFlagsBit3(s32 player){(void)player;return 0;}
#define EVENT(name) void name(Fighter_GObj* object){(void)object;}
EVENT(Fighter_8006A1BC) EVENT(Fighter_8006A360) EVENT(Fighter_8006ABA0)
EVENT(Fighter_Spaghetti_8006AD10) EVENT(Fighter_procUpdate) EVENT(Fighter_procMap)
EVENT(Fighter_8006C5F4) EVENT(Fighter_CallAcessoryCallbacks_8006C624) EVENT(Fighter_8006C80C)
EVENT(Fighter_UnkProcessGrab_8006CA5C) EVENT(Fighter_8006CB94) EVENT(Fighter_ProcessHit_8006D1EC)
EVENT(Fighter_8006D9AC) EVENT(Fighter_UnkCallCameraCallback_8006D9EC) EVENT(Fighter_8006DA4C)
EVENT(ftMh_MS_341_8014FE10) EVENT(ftCh_Init_80155FCC) EVENT(ftCo_800BFD04) EVENT(ftCo_800C61B0)
void ftCommon_8007D92C(Fighter_GObj* object){(void)nativeScriptFighter_Run(object,NativeScriptFighter_NeutralEnter,&(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0},1,NULL,NULL);}
void ftLib_800867E8(Fighter_GObj* object){CHECK(object->user_data);++tail_calls;step(6);}
void ftDrawCommon_80080E18(Fighter_GObj* object,int pass){(void)object;(void)pass;}
void nativeArticleResource_CancelCreator(Fighter_GObj* object,uint32_t generation){(void)object;(void)generation;}
void nativeArticleResource_CancelConstructingAttachment(Fighter_GObj* object,NativeMatchPoolReference gobj,NativeMatchPoolReference owner)
{CHECK(nativeMatchPool_Resolve(gobj)==object&&nativeMatchPool_Resolve(owner)==object->user_data);}
EVENT(ftColl_8007B8E8) EVENT(it_8026B7F8)
void nativeFighterAttachment_Clear(Fighter_GObj* object){CHECK(object->user_data);step(10);}
EVENT(ftColl_800765AC) EVENT(ft_80088C5C)
void nativeItem_RemoveConstructingOwnerChildren(Fighter_GObj* object,NativeMatchPoolReference gobj,NativeMatchPoolReference owner)
{CHECK(nativeMatchPool_Resolve(gobj)==object&&nativeMatchPool_Resolve(owner)==object->user_data);CHECK(!HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);}
void efAsync_QueueClear(void* pointer){(void)pointer;}
void Camera_800290D4(CmSubject* subject){CHECK(subject==&camera);step(9);}
void ftCo_UnloadDynamicBones(Fighter* fighter){(void)fighter;}
void lbShadow_8000EE8C(LbShadow* shadow){(void)shadow;}
void HSD_JObjRemoveAll(HSD_JObj* joint){CHECK(joint==&alternate||joint==&root||joint==NULL);++rig_calls;}
void HSD_JObjUnref(HSD_JObj* joint){(void)joint;}
void ftData_800859A8(Fighter* fighter){(void)fighter;}
void HSD_LObjRemoveAll(HSD_LObj* light){(void)light;}
void Player_80031FB0(int player,s32 member,HSD_GObj* expected){(void)player;(void)member;CHECK(expected);}
void HSD_LObj_803668EC(HSD_LObj* light){(void)light;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* object){(void)object;abort();}
void HSD_JObjDispAll(HSD_JObj* object,Mtx matrix,u32 flags,u32 mode){(void)object;(void)matrix;(void)flags;(void)mode;abort();}
void HSD_FogSet(HSD_Fog* object){(void)object;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* object){(void)object;abort();}
void HSD_CObjEndCurrent(void){abort();}

// Actual reset/ProcessDeath retain their writes and call ordering. These leaves
// represent engine data/geometry/status work, not imported gameplay acceptance.
void Player_LoadPlayerCoords(s32 p,Vec3* v){(void)p;*v=(Vec3){2,3,4};}
float Player_GetFacingDirection(s32 p){(void)p;return 1;}
s32 Player_GetDamage(s32 p){(void)p;return 10;}
float ftCommon_800804EC(Fighter* f){(void)f;return 0;}
uint32_t nativeFighterTarget_Id(HSD_GObj* o){CHECK(!o);return 0;}
void ftCo_800C88A0(Fighter* f){CHECK(f);}
EVENT(ftCo_800DEEA8)
void ftCo_800D105C(Fighter_GObj* o){++reset_calls;step(2);(void)nativeScriptFighter_Run(o,NativeScriptFighter_LoadSpecialAttrs,NULL,0,NULL,NULL);}
EVENT(ftCo_800C09B4) EVENT(ftCommon_8007E2FC)
void ft_80088A50(Fighter* f){CHECK(f);}
void ft_800890BC(Fighter* f){CHECK(f);}
void ft_800892D4(Fighter* f){CHECK(f);}
EVENT(ft_80081B38) EVENT(ft_80081938)
bool ft_80082A68(Fighter_GObj* o){CHECK(o);return false;}
void ftCommon_8007D6A4(Fighter* f){CHECK(f);abort();}
void ftCommon_8007D5D4(Fighter* f){CHECK(f);}
void ftCamera_80076064(Fighter* f){CHECK(f);}
EVENT(Fighter_UnkApplyTransformation_8006C0F0) EVENT(Fighter_UpdateModelScale)
void ftCo_800BFFAC(Fighter* f){CHECK(f);}
void ftCo_800C0074(Fighter* f){CHECK(f);}
EVENT(ftCo_800C8438) EVENT(ftCo_800C89A0) EVENT(ftCo_800C8FC4) EVENT(ftColl_8007AFF8)
void ftColl_8007B0C0(Fighter_GObj* o,HurtCapsuleState s){CHECK(o);(void)s;}
int Player_GetCpuType(int p){(void)p;return 3;}
int Player_GetCpuLevel(int p){(void)p;return 4;}
void ftCo_800A101C(Fighter* f,int t,int l,int unused){CHECK(t==3&&l==4&&!unused);(void)nativeScriptFighter_Run(f->gobj,NativeScriptFighter_CpuInitialize,NULL,0,NULL,NULL);}
EVENT(ft_8007C17C) EVENT(ft_8007C630)
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g){(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g){(void)o;(void)g;return NULL;}
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* o){CHECK(o==&root);}
void hsd_8039D688(HSD_JObj* o,f32** a,s32 n){(void)o;(void)a;(void)n;abort();}
void HSD_JObjWalkTree(HSD_JObj* o,HSD_JObjWalkTreeCallback cb,f32** a)
{CHECK(o==&root&&cb==hsd_8039D688&&!a);++effect_calls;step(8);}
void nativeModelEffect_Cancel(EF_Effect* e){(void)e;abort();}
static void effect_remove(void* data)
{EF_Effect* e=data;CHECK(e>=spawned&&e<spawned+2&&e->attach_jobj==&root);++effect_retired;e->gobj=NULL;}
static void spawn_effects(HSD_GObj* parent)
{
 for(unsigned i=0;i<2;++i){CHECK(!spawned[i].gobj);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_EFFECT,i?HSD_GOBJ_PLINK_EFFECT_UNK1:HSD_GOBJ_PLINK_EFFECT_UNK0,0);CHECK(o);spawned[i]=(EF_Effect){.gobj=o,.parent_gobj=parent,.attach_jobj=&root};GObj_InitUserData(o,4,effect_remove,&spawned[i]);}
 efLib_ParamTable[0].gobj=parent;
}

#include "parts.inc"
#include "effects.inc"
#include "connected_bodies.inc"

enum { N=1024,I=4096,F=64,V=64 };
static char bytes[8192],source[8192];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static MotionState motions[342];
static void prepare(unsigned fault)
{
 CHECK(!nativeScriptFighter_HasActors(Ft_Kind_CustomFirst));
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N,.arguments=arguments,.argument_capacity=I},.declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 size_t used=(size_t)snprintf(source,sizeof(source),"fighter:\n    behavior independent\nstate int zero\nstate int completed\naction Lifecycle:\n");
 const char* events[]={"load","load_special_attrs","death","cpu_initialize","neutral_enter","user_data_remove"};
 const unsigned phases[]={1,2,3,4,5,7};
 for(unsigned i=0;i<6;++i){used+=(size_t)snprintf(source+used,sizeof(source)-used,"    on %s:\n        command(0) = %u\n",events[i],phases[i]);if(fault==phases[i])used+=(size_t)snprintf(source+used,sizeof(source)-used,"        let int invalid = 1 / state.zero\n");used+=(size_t)snprintf(source+used,sizeof(source)-used,"        state.completed = state.completed + 1\n");}
 CHECK(used<sizeof(source));NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)used,.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&constructor_package.program,&diagnostic));
 constructor_package.address=&constructor_package;constructor_package.ready=constructor_package.assets.ready=true;constructor_package.target=Ft_Kind_CustomFirst;constructor_package.assets.motions=motions;constructor_package.assets.facts.moves=layout;
 gFtDataList[Ft_Kind_CustomFirst]=&owned_data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&constructor_package.program);
}
static Fighter_GObj* create(void)
{struct plAllocInfo input={.internal_id=Ft_Kind_CustomFirst,.slot=0,.x5=-1};return Fighter_Create(&input);}
static Fighter_GObj* create_stock(void)
{struct plAllocInfo input={.internal_id=Ft_Kind_Samus,.slot=0,.x5=-1};return Fighter_Create(&input);}
void constructor_probe(HSD_GObj* object);
static void stock_load(Fighter_GObj* object)
{((Fighter*)object->user_data)->cmd_vars[0]=1;constructor_probe(object);}
static void stock_remove(Fighter_GObj* object)
{((Fighter*)object->user_data)->cmd_vars[0]=7;constructor_probe(object);}
void constructor_probe(HSD_GObj* object)
{
 ++callbacks;unsigned phase=((Fighter*)object->user_data)->cmd_vars[0];step(phase);
 if(phase==spawn_phase){spawn_phase=0;spawn_effects(object);}
 if(phase!=mutation_phase||!mutation)return;
 unsigned mode=mutation;mutation=0;
 if(mode==1)HSD_GObjFree(object);
 else if(mode==2){HSD_GObjFree(object);replacement=create();CHECK(replacement==object);}
 else if(mode==3){HSD_GObjFree(object);GObj_RemoveUserData(object);}
 else CHECK(false);
}
static void clear_counters(void)
{trace_count=mutation=mutation_phase=spawn_phase=effect_retired=0;replacement=NULL;tail_calls=camera_calls=effect_calls=rig_calls=reset_calls=0;}
static void empty(void)
{CHECK(!nativeFighterPool.used&&!nativeGObjPool.used&&!nativeGObjProcPool.used);CHECK(!nativeScriptFighter_HasActors(Ft_Kind_CustomFirst));CHECK(!nativeScriptFighter_ReferencesProgram(&bank));CHECK(nativeScriptRuntime_Idle());}
int main(void)
{
 memset(ftData_UnkBytePerCharacter,0xFF,sizeof(ftData_UnkBytePerCharacter));tables[Ft_Kind_CustomFirst]=tables[Ft_Kind_Samus]=&part_table;owned_data.x14=animations;
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);Fighter_804D64F8=1;
 prepare(0);clear_counters();Fighter_GObj* healthy=create();CHECK(healthy&&tail_calls==1&&camera_calls==1&&reset_calls==1);
 const unsigned expected[]={1,2,2,3,4,5,6};CHECK(trace_count==7&&!memcmp(trace,expected,sizeof(expected)));CHECK(((Fighter*)healthy->user_data)->x8_spawnNum>0&&nativeScriptFighter_Status(healthy)==NativeScriptVM_Ok);
 HSD_GObjFree(healthy);CHECK(effect_calls==1&&rig_calls==2);empty();++cases;
 for(unsigned phase=1;phase<=5;++phase){prepare(phase);clear_counters();CHECK(create()==NULL&&!tail_calls&&effect_calls==1);CHECK(camera_calls==(phase==1?0u:1u));empty();++cases;}
 prepare(0);
 for(unsigned phase=1;phase<=5;++phase)for(unsigned mode=1;mode<=2;++mode){clear_counters();mutation=mode;mutation_phase=phase;CHECK(create()==NULL&&!mutation);if(mode==2){CHECK(replacement&&nativeScriptFighter_Status(replacement)==NativeScriptVM_Ok);HSD_GObjFree(replacement);}empty();++cases;}
 clear_counters();healthy=create();mutation=3;mutation_phase=7;unsigned previous=callbacks;HSD_GObjFree(healthy);CHECK(callbacks==previous+1&&!mutation&&effect_calls==1);empty();++cases;
 clear_counters();healthy=create();mutation=3;mutation_phase=7;previous=callbacks;GObj_RemoveUserData(healthy);CHECK(callbacks==previous+1&&!mutation);empty();++cases;
 for(unsigned phase=1;phase<=3;phase+=2){prepare(phase);clear_counters();spawn_phase=phase;CHECK(create()==NULL);CHECK(effect_retired==2&&!efLib_ParamTable[0].gobj&&!spawned[0].gobj&&!spawned[1].gobj);empty();++cases;}
 prepare(0);clear_counters();healthy=create();spawn_phase=7;HSD_GObjFree(healthy);CHECK(effect_retired==2&&!efLib_ParamTable[0].gobj);empty();++cases;
 ftData_OnLoad[Ft_Kind_Samus]=stock_load;ftData_OnUserDataRemove[Ft_Kind_Samus]=stock_remove;
 clear_counters();healthy=create_stock();CHECK(healthy&&!nativeScriptFighter_IsAttached(healthy));HSD_GObjFree(healthy);empty();++cases;
 for(unsigned mode=1;mode<=2;++mode){clear_counters();mutation=mode;mutation_phase=1;CHECK(!create_stock());if(replacement)HSD_GObjFree(replacement);empty();++cases;}
 clear_counters();healthy=create_stock();mutation=3;mutation_phase=7;previous=callbacks;HSD_GObjFree(healthy);CHECK(callbacks==previous+1);empty();++cases;
 clear_counters();for(unsigned i=0;i<NATIVE_CALLBACK_DISPATCH_LIMIT;++i)CHECK(nativeCallbackDispatch_Enter());CHECK(!nativeCallbackDispatch_Enter());CHECK(!create_stock()&&!camera_calls&&!tail_calls);for(unsigned i=0;i<NATIVE_CALLBACK_DISPATCH_LIMIT;++i)nativeCallbackDispatch_Leave();empty();++cases;
 // Quiescent broad-oracle replay of the real GObj/Fighter/actor owners. The
 // selected compiler/package and controlled asset leaves remain immutable.
 prepare(3);NativeRollbackStateRegion region;CHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1);
 uint8_t* prior=malloc(region.size);uint8_t* after=malloc(region.size);CHECK(prior&&after);
 memcpy(prior,region.memory,region.size);clear_counters();CHECK(!create());empty();memcpy(after,region.memory,region.size);
 unsigned replay_trace[128],replay_count=trace_count;memcpy(replay_trace,trace,trace_count*sizeof(*trace));
 memcpy(region.memory,prior,region.size);clear_counters();CHECK(!create());empty();CHECK(region.size&&!memcmp(after,region.memory,region.size));CHECK(trace_count==replay_count&&!memcmp(trace,replay_trace,trace_count*sizeof(*trace)));
 free(prior);free(after);++cases;
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"authored_callbacks\":%u,\"actual_reset\":true,\"actual_constructor_unload\":true}\n",checks,cases,callbacks);
 return 0;
}

