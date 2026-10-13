#include "boundary.h"
#include <melee/ft/native_script_binding_stage.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/lb/types.h>
#include <melee/lb/lbvector.h>
#include <melee/ft/ftlib.h>
#include <melee/gr/grdynamicattr.h>
#include <sysdolphin/baselib/debug.h>
#include <placeholder.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/fighter.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
ftCommonData* p_ftCommonData;
u32 it_804D6D14;
void ftCo_8008DCE0(Fighter_GObj* object,int motion,float facing)
{(void)object;(void)motion;(void)facing;abort();}
void ftCo_8008E908(Fighter_GObj* object,float facing)
{(void)object;(void)facing;abort();}
#include <melee/lb/lb_00B0.h>
#pragma clang section data = ""
enum{N=2048,F=64,V=64};
static char bytes[32768];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeSourcePackage package;
#define bank package.compiler.bank
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=10,.ground={343,345,347,349},.air={344,346,348,350}};
static Fighter fighters[64];static MotionState motions[351];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];

DbLKind DbLevel;
static unsigned allocated,checks,cases,pairs,vm_calls,negative_cases;

#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"stage line%u %s\n",__LINE__,#x);exit(1);}}while(0)
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&package:NULL;}
static void userdata_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}


static HSD_GObj* callback_source;
static HSD_GObj* replacement;
static HSD_GObj* create(void);
static unsigned mutation, callbacks, substeps, segments;
static uint32_t contact_flags;
static bool native_boolean,segment_boolean;
static CollData observed;
static float segment_values[4],bounds[4];
static unsigned category_seen;
void ftCommon_8007F76C(Fighter_GObj* object){(void)object;abort();}
s32 ftLib_800874BC(Fighter_GObj* o){(void)o;abort();}
u8 ftLib_80086BE0(Fighter_GObj* o){(void)o;abort();}
static bool mpColl_IsEcbTiny;
static bool (*mpColl_804D64A0)(Fighter_GObj*,int);
static Fighter_GObj* mpColl_804D64A4;
int mpColl_804D64AC;
#define CollisionFlagAir_CanGrabLedge 4
#define max_inline(a,b) ((a)>(b)?(a):(b))
void mpBoundingCheck(float l,float b,float r,float t)
{bounds[0]=l;bounds[1]=b;bounds[2]=r;bounds[3]=t;}
void mpUncheckBounding(void) {}
int grDynamicAttr_801CA284(Vec3* p,int line){(void)p;CHECK(line==7);return 0;}
#include "terrain_leaves.h"
int mpJointFromLine(int line){return line<0?-1:terrain_joint_id;}
static void stage_callback(void* data,int joint,CollData* probe,int x,mpLib_GroundEnum kind,float dy)
{
 (void)data;(void)x;(void)dy;CHECK(joint==3);CHECK(kind==0||kind==1);CHECK(probe->x0_gobj==NULL);++callbacks;
 unsigned action=mutation;mutation=0;
 if(action==1)HSD_GObjFree(callback_source);
 if(action==2)nativeScriptFighter_Detach(callback_source);
 if(action==3)((Fighter*)callback_source->user_data)->x8_spawnNum++;
 if(action==4)callback_source->user_data=&fighters[63];
 if(action==5)bank.storage_layout.identity++;
 if(action==6){NativeScriptValue v;NativeScriptDiagnostic d;++vm_calls;CHECK(nativeScriptFighter_Helper(callback_source,((Fighter*)callback_source->user_data)->x8_spawnNum,1,NULL,0,&v,&d)==NativeScriptVM_Error);}
 if(action==7){HSD_GObjFree(callback_source);replacement=create();CHECK(replacement==callback_source);}
}
void mpJointGetCb1(int joint,mpLib_JointCollisionCallback* cb,void** data)
{CHECK(joint==3);*cb=stage_callback;*data=NULL;}
void mpJointGetCb2(int joint,mpLib_JointCollisionCallback* cb,void** data)
{CHECK(joint==3);*cb=stage_callback;*data=NULL;}
bool mpColl_80046904(CollData* probe,u32 flags)
{
 CHECK(flags==0);++substeps;category_seen=probe->x34_flags.b1234;
 probe->env_flags=contact_flags;probe->floor.index=7;probe->ceiling.index=8;observed=*probe;
 return native_boolean;
}
bool mpCheckMultiple(float x0,float y0,float x1,float y1,Vec3* p,int* line,u32* flags,Vec3* normal,u32 mask,int skip,int only)
{
 CHECK(!p&&!line&&!flags&&!normal&&mask==0x1f&&skip==-1&&only==-1);++segments;
 segment_values[0]=x0;segment_values[1]=y0;segment_values[2]=x1;segment_values[3]=y1;
 return segment_boolean;
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#include "native_stage.inc"
#pragma clang diagnostic pop

static ItemAttr item_attributes;
static ItemModelDesc item_model;
static Article article={.x0_common_attr=&item_attributes,.x10_modelDesc=&item_model};
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* bad[]={"stage.compare_floor_islands(0, 0, 0, 0, 0, 0, -20, true, found, same, x, x)","stage.compare_floor_islands(0, 0, 0, 0, 0, 0, -20, true, x, same, x, y)","stage.compare_floor_islands(0, 0, 0, 0, 0, 0, -20, true, false, same, x, y)","stage.read_floor(0, 0, 0, 0, x, x, line)","stage.read_floor(0, 0, 0, 0, x, y, x)","stage.read_floor(0, 0, 0, 0, x, y)","stage.read_line_island_bounds(0.0, x, y, u, v)","stage.read_line_island_bounds(0, x, y, x, v)","stage.read_line_island_bounds(0, x, y, z, u, v, x)","stage.read_line_island_bounds(0, x, y, z, u, v)","stage.same_line_island(0, 1.0)","stage.cpu_floor_excluded(0.0)","stage.cpu_island_blocked(0.0)","stage.sweep_box(0, 0, 0)","stage.segment_blocked(0, 0, 0)","stage.segment_blocked(0, 0, 0, 0, 0)","stage.sweep_box(0, 0, 0, 0, 0, 0, 1, -1, -1, 0, 1, 0, 0.5)","stage.segment_blocked(\"bad\", 0, 0, 0)"};
 for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){char invalid[512];snprintf(invalid,sizeof(invalid),"fighter:\n    behavior independent\nfunc Bad():\n    let float x = 0\n    let float y = 0\n    let float z = 0\n    let float u = 0\n    let float v = 0\n    let int line = 0\n    let bool found = false\n    let bool same = false\n    %s\n",bad[i]);NativeScriptCompileRequest r={.source=invalid,.source_bytes=(uint32_t)strlen(invalid),.motion_layout=&layout};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;bool success=nativeScriptCompile_Build(&bank,&r,&accepted,&d);if(success)fprintf(stderr,"negative admitted %u %s\n",i,bad[i]);CHECK(!success&&!accepted);++cases;++negative_cases;}
 const char* source="fighter:\n    behavior independent\nstate int evaluations\nstate bool terrain_found\nstate bool terrain_same\nstate int terrain_line\nstate float tx\nstate float ty\nstate float tz\nstate float ux\nstate float uy\nstate float uz\nstate float floor_y\nfunc Sweep() -> int:\n    return stage.sweep_box(0, 0, 3, 18, 9, 4, 2, -2, -2, 0, 2, 0, 5)\nfunc Fault():\n    let int zero = 0\n    let int broken = 1 / zero\nfunc Segment() -> bool:\n    return stage.segment_blocked(1, 2, 3, 4)\nfunc Next() -> float:\n    state.evaluations = state.evaluations + 1\n    return 0\nfunc Once() -> int:\n    return stage.sweep_box(Next(), 0, 3, 18, 9, 4, 2, -2, -2, 0, 2, 0, 5)\nfunc Floor() -> bool:\n    return stage.read_floor(0, state.floor_y, 0, -10, state.tx, state.ty, state.terrain_line)\nfunc CpuFloor() -> bool:\n    let float x = 0\n    let float y = 0\n    let int line = -1\n    let bool found = stage.read_floor(0, state.floor_y, 0, -state.floor_y, x, y, line)\n    return found and not stage.cpu_floor_excluded(line)\nfunc Bounds() -> bool:\n    return stage.read_line_island_bounds(state.terrain_line, state.tx, state.ty, state.ux, state.uy)\nfunc Bounds3D() -> bool:\n    return stage.read_line_island_bounds(state.terrain_line, state.tx, state.ty, state.tz, state.ux, state.uy, state.uz)\nfunc Same() -> bool:\n    return stage.same_line_island(state.terrain_line, 1)\nfunc Excluded() -> bool:\n    return stage.cpu_floor_excluded(state.terrain_line)\nfunc Blocked() -> bool:\n    return stage.cpu_island_blocked(state.terrain_line)\nfunc Island():\n    stage.island_bounds(Next(), 10, 0, -20, state.terrain_found, state.tx, state.ty, state.ux, state.uy)\nfunc Follow():\n    stage.follow_floor(state.terrain_line, state.tx, state.ty, state.tz, 12, 10, state.terrain_found, state.ux, state.uy, state.uz)\nfunc Compare():\n    stage.compare_floor_islands(Next(), 10, 0, 20, 10, 0, -20, true, state.terrain_found, state.terrain_same, state.tx, state.ux)\nfunc EmitEvent() -> bool:\n    return stage.event(StageEvent_Conversation_start, 1)\narticle Probe states 1:\n    state bool result\n    state bool terrain_found\n    state int terrain_line\n    state float tx\n    state float ty\n    state float tz\n    state float ux\n    state float uy\n    state float uz\n    on signal Island:\n        stage.island_bounds(0, 10, 0, -20, item.state.terrain_found, item.state.tx, item.state.ty, item.state.ux, item.state.uy)\n    on signal Follow:\n        stage.follow_floor(item.state.terrain_line, item.state.tx, item.state.ty, item.state.tz, 12, 10, item.state.terrain_found, item.state.ux, item.state.uy, item.state.uz)\n    on signal Terrain:\n        item.state.terrain_found = stage.read_line_island_bounds(item.state.terrain_line, item.state.tx, item.state.ty, item.state.tz, item.state.ux, item.state.uy, item.state.uz)\n    on signal Floor:\n        item.state.terrain_found = stage.read_floor(0, 10, 0, -10, item.state.tx, item.state.ty, item.state.terrain_line)\n    on spawned:\n        item.state.result = stage.segment_blocked(1, 2, 3, 4)\n    on created:\n        let int zero = 0\n        let int broken = 1 / zero\n    state 0 anim -1:\n";
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Link_Arrow,.article=&article};
 NativeScriptItemMetadataContext metadata={.motions=&layout,.assets=&asset,.asset_count=1};
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
 NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,&metadata,&package.program,&d)){fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.motions=motions;package.assets.facts.moves=layout;
 gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&package.program);
}
static HSD_GObj* create(void)
{
 CHECK(allocated<63);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);Fighter* f=&fighters[allocated++];memset(f,0,sizeof(*f));
 f->gobj=object;f->kind=f->behavior_kind=Ft_Kind_CustomFirst;f->ft_data=&package.assets.character.data;f->x8_spawnNum=allocated;f->x34_scale.y=1;
 GObj_InitUserData(object,4,userdata_remove,f);CHECK(nativeScriptFighter_Attach(object));return object;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x123456789abcdef)};}
static bool helper(HSD_GObj* o,uint32_t id,NativeScriptValue* out)
{++vm_calls;NativeScriptDiagnostic d={0};return nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,id,NULL,0,out,&d)==NativeScriptVM_Ok;}
static void inputs(NativeScriptValue* a)
{const float v[]={0,0,3,18,9,4,2,-2,-2,0,2,0};for(unsigned i=0;i<12;++i)a[i]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=v[i]};a[12]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=5};}
static void reference(const NativeScriptValue* a,CollData* p)
{
 *p=(CollData){0};mpColl_80041EE4(p);p->last_pos=p->cur_pos;p->cur_pos=(Vec3){a[0].as.f32,a[1].as.f32,a[2].as.f32};p->last_pos=p->cur_pos;p->cur_pos=(Vec3){a[3].as.f32,a[4].as.f32,a[5].as.f32};p->x34_flags.b1234=(uint64_t)a[12].as.u64;
 ftCollisionBox box={a[6].as.f32,a[7].as.f32,{a[8].as.f32,a[9].as.f32},{a[10].as.f32,a[11].as.f32}};(void)mpColl_8004730C(p,&box);
}
static void comparisons(HSD_GObj* o)
{
 NativeScriptRuntimeContext c=context(o);NativeScriptValue a[13];inputs(a);callback_source=o;
 const uint64_t modes[]={0,4,5,15,16,UINT64_C(0x8000000000000005),UINT64_MAX};
 const uint32_t flags[]={0,Collide_FloorPush,Collide_CeilingHug,Collide_LeftWallPush,Collide_RightWallPush,Collide_Edge};
 for(unsigned k=0;k<7;++k)for(unsigned f=0;f<6;++f)for(unsigned b=0;b<2;++b){
  a[12]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=modes[k]};contact_flags=flags[f];native_boolean=b;substeps=callbacks=0;
  CollData p;reference(a,&p);CollData expected=observed;unsigned expected_steps=substeps,expected_callbacks=callbacks;substeps=callbacks=0;
  NativeScriptValue out=sentinel();NativeScriptDiagnostic d={0};CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageSweepBox,a,13,&out,&d));CHECK(out.type==NativeScriptType_I32&&out.as.i64==p.env_flags);CHECK(!memcmp(&expected,&observed,sizeof(expected)));CHECK(expected_steps==substeps&&expected_callbacks==callbacks);CHECK(category_seen==(modes[k]&15));++pairs;++cases;
 }
 contact_flags=Collide_FloorPush;native_boolean=true;NativeScriptValue out=sentinel();CHECK(helper(o,0,&out)&&out.type==NativeScriptType_I32&&out.as.i64==Collide_FloorPush);++cases;
 for(unsigned b=0;b<2;++b){segment_boolean=b;CHECK(helper(o,2,&out)&&out.type==NativeScriptType_Bool&&out.as.u64==b);CHECK(segment_values[0]==1&&segment_values[1]==2&&segment_values[2]==3&&segment_values[3]==4);++pairs;++cases;}
 CHECK(helper(o,4,&out)&&out.type==NativeScriptType_I32&&out.as.i64==Collide_FloorPush);
 NativeScriptReceiverView view;NativeScriptDiagnostic d;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC,NULL,0,&out,&d)&&out.as.i64==1);++cases;
 // Signed wide categories retain the native low bits without an I32 funnel.
 for(unsigned k=0;k<2;++k){inputs(a);a[12]=(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=k?INT64_MIN+5:-1};CHECK(nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageSweepBox,a,13,&out,&d));CHECK(category_seen==(k?5:15));++cases;}
}
static void failures(void)
{
 NativeScriptValue a[13];inputs(a);NativeScriptDiagnostic d={0};
 for(unsigned which=0;which<9;++which){HSD_GObj* o=create();NativeScriptRuntimeContext c=context(o);NativeScriptValue out=sentinel(),saved=out;inputs(a);unsigned before=substeps;
  if(which==0)a[0].type=NativeScriptType_F64;
  if(which==1)a[0].as.f32=NAN;
  if(which==2)a[3].as.f32=INFINITY;
  if(which==3)a[10].as.f32=FLT_MAX;
  if(which==4){a[12].type=NativeScriptType_F32;a[12].as.f32=5;}
  if(which==5){a[12].type=NativeScriptType_Bool;a[12].as.u64=2;}
  if(which==6){a[12].type=NativeScriptType_U32;a[12].as.u64=UINT64_MAX;}
  if(which==7)c.scene_epoch++;
  if(which==8){DbLevel=DbLKind_DebugRom;a[3].as.f32=45000;}
  CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageSweepBox,a,13,&out,&d));CHECK(!memcmp(&out,&saved,sizeof(out))&&substeps==before);DbLevel=0;HSD_GObjFree(o);++cases;++negative_cases;
 }
 for(unsigned action=1;action<=7;++action){HSD_GObj* o=create();Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);NativeScriptValue out=sentinel(),saved=out;uint64_t identity=bank.storage_layout.identity;callback_source=o;mutation=action;native_boolean=true;contact_flags=Collide_FloorPush;inputs(a);
  CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageSweepBox,a,13,&out,&d));CHECK(!memcmp(&out,&saved,sizeof(out)));CHECK(mutation==0);bank.storage_layout.identity=identity;
  if(action==7)HSD_GObjFree(replacement);else if(action!=1){o->user_data=f;HSD_GObjFree(o);}++cases;++negative_cases;
 }
 HSD_GObj* rejected=create();NativeScriptRuntimeContext rejected_context=context(rejected);
 for(unsigned low=1;low<=3;++low)for(unsigned type=0;type<4;++type){inputs(a);
  if(type==0)a[12]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-16+(int)low};
  if(type==1)a[12]=(NativeScriptValue){.type=NativeScriptType_I64,.as.i64=INT64_MIN+low};
  if(type==2)a[12]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=UINT64_C(0x80000000)+low};
  if(type==3)a[12]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x8000000000000000)+low};
  NativeScriptValue out=sentinel(),saved=out;unsigned steps_before=substeps,callbacks_before=callbacks;
  CHECK(!nativeScriptBindingStage_Invoke(&rejected_context,NativeScriptBinding_StageSweepBox,a,13,&out,&d));CHECK(!memcmp(&out,&saved,sizeof(out))&&substeps==steps_before&&callbacks==callbacks_before);++cases;++negative_cases;
 }HSD_GObjFree(rejected);
 const unsigned vm_mutations[]={1,2,7};
 for(unsigned i=0;i<3;++i){HSD_GObj* o=create();callback_source=o;mutation=vm_mutations[i];NativeScriptValue out=sentinel(),saved=out;CHECK(!helper(o,0,&out));CHECK(!memcmp(&out,&saved,sizeof(out))&&mutation==0);if(vm_mutations[i]==7)HSD_GObjFree(replacement);else if(vm_mutations[i]==2)HSD_GObjFree(o);++cases;++negative_cases;}
}
#include "terrain_cases.h"
#include "cpu_floor_reference.inc"
#include "cpu_floor_cases.h"
#include "island_compare_cases.h"
#include "floor_follow_cases.h"
#include "island_bounds_cases.h"
static void item_remove(void* data){nativeScriptItem_Detach(((Item*)data)->entity);}
static void article_case(void)
{
 const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);static Item item;
 item=(Item){.entity=o,.kind=p->kind,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100,.xC4_article_data=(Article*)p->article,.xCC_item_attr=&item_attributes};GObj_InitUserData(o,6,item_remove,&item);
 CHECK(nativeScriptItem_Attach(o,p));terrain_article_cases(o);floor_follow_article_cases(o);island_bounds_article_cases(o);
 segment_boolean=true;NativeScriptValue out=sentinel();CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_Ok);++vm_calls;
 NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,100,&view));NativeScriptDiagnostic d;
 for(unsigned i=0;i<bank.declarations.value_count;++i)if(bank.declarations.values[i].owner==p->owner&&nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.values[i].name,"result")){CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&out,&d)&&out.type==NativeScriptType_Bool&&out.as.u64==1);}
 NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Item,.generation=100,.scene_epoch=nativeGObjPool.lifetime};NativeScriptValue a[13];inputs(a);out=sentinel();CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageSweepBox,a,13,&out,&d)&&out.type==NativeScriptType_U64);++negative_cases;
 NativeScriptValue saved=out;CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Created,NULL,0,&out)==NativeScriptVM_Error);++vm_calls;CHECK(!memcmp(&out,&saved,sizeof(out))&&nativeScriptItem_Status(o)==NativeScriptVM_Error);
 unsigned before=segments;CHECK(!nativeScriptBindingStage_Invoke(&c,NativeScriptBinding_StageSegmentBlocked,a,4,&out,&d));CHECK(!memcmp(&out,&saved,sizeof(out))&&segments==before);++cases;++negative_cases;HSD_GObjFree(o);++cases;
}
#include "event_cases.h"

int main(void)
{
 for(unsigned i=1;i<NativeScriptBinding_Count;++i){const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(i);CHECK(descriptor&&descriptor->name);}
 CHECK(!*nativeScriptBindings_Descriptor(NativeScriptBinding_StageReadLineIslandBounds3D)->name);CHECK(!*nativeScriptBindings_Descriptor(NativeScriptBinding_ItemBounceToGroundWithContactReset)->name);
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();HSD_GObj* o=create();event_cases(o);terrain_cases(o);cpu_floor_cases(o);island_compare_cases(o);floor_follow_cases(o);island_bounds_cases(o);comparisons(o);HSD_GObjFree(o);failures();article_case();CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u,\"negative_cases\":%u}\n",checks,cases,pairs,vm_calls,negative_cases);return 0;
}
