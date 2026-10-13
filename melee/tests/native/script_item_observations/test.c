#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_binding_item_observations.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/inlines.h>
#include <melee/it/itmaplib.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/it_2725.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbvector.h>
#include <melee/mp/mplib.h>
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <placeholder.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

enum{Bytes=4096,Nodes=256,Expressions=1024,Functions=32,Values=16,Instructions=1024};
static char source[Bytes];static NativeScriptSourceNode nodes[Nodes];
static NativeScriptIRNode expressions[Expressions];static uint32_t arguments[Expressions];
static NativeScriptFunctionDeclaration functions[Functions];static NativeScriptParameterDeclaration parameters[Values];
static NativeScriptValueDeclaration values[Values];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[Instructions];static NativeScriptFunctionCode code[Functions];
static NativeScriptExecutionFunction execution[Functions];static NativeScriptCompileFunctionScratch graph[Functions];
static NativeScriptStorageField fields[Values];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[Values];
static NativeScriptCompileBank bank;static NativeScriptDiagnostic diagnostic;
static NativeScriptItemProgram program;static NativeScriptStorageInstance storage;
static Item item;static Article article;static ItemAttr attributes;static HSD_JObj joints[4];static HSD_AObj animations[4];
static ItemModelDesc model_descriptor;static struct DynamicBoneTable bones;
static HSD_ClassInfo subclass,wrong;HSD_JObjInfo hsdJObj;
NativeMatchPool nativeGObjPool;static HSD_GObj objects[2];static NativeMatchPoolSlot pool_slots[2];static uint32_t order[2];
static HSD_GObj* actor;static HSD_GObj* lists[32];HSD_GObj** HSD_GObjPLinkHead=lists;u8 HSD_GObj_JObjKind=3;
static unsigned checks,pairs,cases,vm_calls,events,mutation,class_initializations;static bool observed;static int collision_result;
static uint32_t observation_semantic;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"observation %u: %s (%u:%u %s)\n",__LINE__,#x,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}while(0)
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* object){return object==actor?&program:NULL;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner){return author==program.author_kind&&owner==program.owner?&program:NULL;}
bool nativeScriptItem_IsAttached(Item_GObj* object){return object==actor&&item.native_script_owner!=0;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* object){return object==actor&&storage.live&&!item.native_removal_in_progress?NativeScriptVM_Ok:NativeScriptVM_SourceRetired;}
bool nativeScriptItem_View(Item_GObj* object,uint32_t generation,NativeScriptReceiverView* view){if(object!=actor||generation!=17||!storage.live)return false;*view=(NativeScriptReceiverView){.program=&bank.declarations,.storage=&storage};return true;}
Fighter_GObj* nativeScriptItem_Creator(Item_GObj* object){(void)object;return NULL;}
Fighter_GObj* nativeScriptItem_AttachmentOwner(Item_GObj* object){(void)object;return NULL;}
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&nativeCallbackSource_Alive(c->source,c->generation,c->source_class);}
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* a,uint32_t n,NativeScriptValue* out,NativeScriptDiagnostic* d){(void)a;if(n)return false;return nativeScriptBindingItemObservations_Read(c,id,out,d);}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* a,uint32_t n,const NativeScriptValue* out,NativeScriptDiagnostic* d){(void)a;if(n)return false;return nativeScriptBindingItemObservations_Write(c,id,out,d);}
static void continuation(void){if(!observed)return;switch(mutation){case 1:++item.x1C;break;case 2:storage.live=false;break;case 3:actor->hsd_obj=&joints[1];break;case 4:++program.identity;break;case 5:++bank.storage_parameters.identity;break;case 6:item.native_removal_in_progress=true;break;case 7:{NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,actor,&ref));CHECK(nativeMatchPool_Free(&nativeGObjPool,ref));CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&ref)==actor);break;}case 8:HSD_CLASS_METHOD(&joints[0])=&wrong;break;case 9:subclass.head.obj_size=1;break;}}
static void observation(uint32_t semantic){if(semantic==observation_semantic)continuation();}
static void class_initialize(void){++class_initializations;HSD_CLASS_INFO(&hsdJObj)->head.flags|=1;continuation();}
void it_80276278(Item_GObj* object){CHECK(object==actor);events=events*10+1;}
bool mpColl_800471F8(CollData* coll){CHECK(coll==&item.x378_itemColl);events=events*10+2;coll->cur_pos=(Vec3){4,5,6};return collision_result!=0;}
bool mpColl_800477E0(CollData* coll){CHECK(coll==&item.x378_itemColl);events=events*10+3;coll->cur_pos=(Vec3){7,8,9};return collision_result!=0;}
#if !defined(NDEBUG)
void __assert(char* file,u32 line,char* text){fprintf(stderr,"native assert %s:%lu %s\n",file,line,text);abort();}
#endif
#include "launch_sweep_source.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wswitch-bool"
#include "native_bodies.inc"
#pragma clang diagnostic pop
static bool metadata(void* opaque,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d){(void)opaque;const NativeScriptSourceNode* n=&tree->nodes[at];if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 1");if(n->kind==NativeScriptSource_ArticleState)return nativeScriptSource_SpanEquals(tree,n->value,"0 anim -1");if(n->kind==NativeScriptSource_Function&&tree->nodes[n->parent].kind==NativeScriptSource_Article)return true;return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);}
static void compile(void){bank=(NativeScriptCompileBank){.source_storage=source,.source_capacity=Bytes,.tree={.nodes=nodes,.capacity=Nodes},.expressions={.nodes=expressions,.node_capacity=Expressions,.arguments=arguments,.argument_capacity=Expressions},.declarations={.functions=functions,.function_capacity=Functions,.parameters=parameters,.parameter_capacity=Values,.values=values,.value_capacity=Values},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=Instructions,.function_code=code,.function_code_capacity=Functions,.execution_functions=execution,.execution_function_capacity=Functions,.graph=graph,.graph_capacity=Functions,.storage_layout={.fields=fields,.field_capacity=Values,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=Values};
 const char* text="fighter:\n    behavior independent\narticle Probe states 1:\n    func Sweep() -> int:\n        return item.sweep_stage_flags()\n    func Finished() -> bool:\n        return item.animation_finished\n    func Disable():\n        item.disable_surface_response()\n    func Floor():\n        item.sweep_floor()\n    func Walls() -> int:\n        return item.wall_contact_flags()\n    func Ceiling() -> int:\n        return item.ceiling_contact_flags()\n    func Reflect() -> bool:\n        return item.reflect_from_wall()\n    func Align():\n        item.align_to_floor()\n    func Facing():\n        item.align_facing_to_velocity()\n" LAUNCH_SWEEP_SOURCE "    state 0 anim -1:\n";NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(accepted==&bank);uint32_t owner=0;for(unsigned i=0;i<bank.tree.count;++i)if(bank.tree.nodes[i].kind==NativeScriptSource_Article)owner=i;CHECK(owner);
 program=(NativeScriptItemProgram){.bank=&bank,.identity=71,.owner=owner,.author_kind=Ft_Kind_CustomFirst,.kind=It_Kind_Link_Bomb,.article=&article};storage=(NativeScriptStorageInstance){.live=true,.owner=owner,.generation=17,.layout=&bank.storage_layout,.parameters=&bank.storage_parameters};}
static NativeScriptRuntimeContext context(void){return(NativeScriptRuntimeContext){.source=actor,.source_class=NativeCallbackSource_Item,.generation=17,.scene_epoch=64};}
static void seed(void){NativeMatchPoolReference ref;nativeGObjPool=(NativeMatchPool){0};CHECK(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),pool_slots,order,2,sizeof(HSD_GObj),_Alignof(HSD_GObj),64));actor=nativeMatchPool_Allocate(&nativeGObjPool,&ref);CHECK(actor);memset(actor,0,sizeof(*actor));actor->classifier=HSD_GOBJ_CLASS_ITEM;actor->obj_kind=HSD_GObj_JObjKind;actor->hsd_obj=&joints[0];actor->user_data=&item;lists[HSD_GOBJ_PLINK_ITEM]=actor;item=(Item){.entity=actor,.x1C=17,.kind=program.kind,.native_script_owner=program.owner,.native_article_author_kind=program.author_kind,.xC4_article_data=&article,.xCC_item_attr=&attributes,.xD5C=2,.pos={1,2,3}};
 storage.live=true;program.identity=71;bank.storage_parameters.identity=75;observed=false;mutation=events=0;collision_result=0;memset(joints,0,sizeof(joints));memset(animations,0,sizeof(animations));memset(&bones,0,sizeof(bones));model_descriptor=(ItemModelDesc){0};article.x10_modelDesc=&model_descriptor;attributes=(ItemAttr){.x58=0.75f};*HSD_CLASS_INFO(&hsdJObj)=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj)}};subclass=(HSD_ClassInfo){0};subclass.head.obj_size=sizeof(HSD_JObj)+16;subclass.head.parent=HSD_CLASS_INFO(&hsdJObj);for(unsigned i=0;i<4;++i)HSD_CLASS_METHOD(&joints[i])=&subclass;joints[0].child=&joints[1];joints[1].parent=&joints[0];joints[1].next=&joints[2];joints[2].parent=&joints[0];joints[1].child=&joints[3];joints[3].parent=&joints[1];}
static bool call(uint32_t semantic,NativeScriptValue* out){NativeScriptRuntimeContext c=context();observation_semantic=semantic;bool result=semantic==NativeScriptBinding_ItemAnimationFinished?nativeScriptBindings_Read(&c,semantic,NULL,0,out,&diagnostic):nativeScriptBindings_Invoke(&c,semantic,NULL,0,NULL,0,out,&diagnostic);observation_semantic=0;return result;}
static NativeScriptVMStatus run(const char* name,NativeScriptValue* out){uint32_t id=UINT32_MAX;for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[i].name,name))id=i;CHECK(id!=UINT32_MAX);NativeScriptValue registers[64];NativeScriptVMFrame frames[8];NativeScriptVM vm={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=8};NativeScriptRuntimeContext c=context();NativeScriptVMHost host={.source_alive=nativeScriptBindings_SourceAlive,.read=nativeScriptBindings_Read,.write=nativeScriptBindings_Write,.invoke=nativeScriptBindings_Invoke};static const char* const observations[]={"Sweep","Floor","Walls","Ceiling","Reflect","Align","Facing"};static const uint32_t semantics[]={NativeScriptBinding_ItemSweepStageFlags,NativeScriptBinding_ItemSweepFloor,NativeScriptBinding_ItemWallContactFlags,NativeScriptBinding_ItemCeilingContactFlags,NativeScriptBinding_ItemReflectFromWall,NativeScriptBinding_ItemAlignToFloor,NativeScriptBinding_ItemAlignFacingToVelocity};observation_semantic=0;for(unsigned i=0;i<7;++i)if(!strcmp(name,observations[i]))observation_semantic=semantics[i];++vm_calls;NativeScriptVMStatus status=nativeScriptVM_Run(&vm,&bank.code,&host,&c,id,NULL,0,out);diagnostic=vm.diagnostic;observation_semantic=0;return status;}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};}
static void rejected(uint32_t semantic){NativeScriptValue out=sentinel();CHECK(!call(semantic,&out));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;}
static uint32_t environment(unsigned bits)
{return(bits&1?Collide_LeftWallMask:0)|(bits&2?Collide_RightWallMask:0)|(bits&4?Collide_CeilingMask:0)|(bits&8?Collide_FloorMask:0);}
static void terrain(unsigned bits)
{
 item.x378_itemColl.env_flags=environment(bits);item.xC30=-77;
 item.x378_itemColl.floor.index=11;item.x378_itemColl.left_facing_wall.index=12;
 item.x378_itemColl.right_facing_wall.index=13;item.x378_itemColl.ceiling.index=14;
 item.x378_itemColl.floor.normal=(Vec3){0.6f,0.8f,0};
 item.x378_itemColl.left_facing_wall.normal=(Vec3){1,0,0};
 item.x378_itemColl.right_facing_wall.normal=(Vec3){-1,0,0};
 item.x378_itemColl.ceiling.normal=(Vec3){0,-1,0};
}
static void compare(uint32_t semantic)
{
 Item before=item;HSD_JObj original[4];memcpy(original,joints,sizeof(original));
 NativeScriptValue expected={.type=NativeScriptType_Void};
 switch(semantic){
 case NativeScriptBinding_ItemSweepFloor:it_8026E0F4(actor);break;
 case NativeScriptBinding_ItemWallContactFlags:expected=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=it_80276308(actor)};break;
 case NativeScriptBinding_ItemCeilingContactFlags:expected=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=it_802763E0(actor)};break;
 case NativeScriptBinding_ItemReflectFromWall:expected=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=it_8027770C(actor)};break;
 case NativeScriptBinding_ItemAlignToFloor:it_80276CB8(actor);break;
 case NativeScriptBinding_ItemAlignFacingToVelocity:it_80272980(actor);break;
 default:CHECK(false);break;
 }
 Item after=item;HSD_JObj changed[4];memcpy(changed,joints,sizeof(changed));unsigned expected_events=events;
 item=before;memcpy(joints,original,sizeof(original));events=0;NativeScriptValue out=sentinel();
 CHECK(call(semantic,&out));CHECK(out.type==expected.type);
 if(out.type!=NativeScriptType_Void)CHECK(out.as.u64==expected.as.u64);
 CHECK(!memcmp(&after,&item,sizeof(item)));CHECK(!memcmp(changed,joints,sizeof(changed)));CHECK(events==expected_events);
 ++cases;++pairs;
}
static void extensions(void)
{
 static const uint32_t semantics[]={NativeScriptBinding_ItemSweepFloor,NativeScriptBinding_ItemWallContactFlags,
 NativeScriptBinding_ItemCeilingContactFlags,NativeScriptBinding_ItemReflectFromWall,
 NativeScriptBinding_ItemAlignToFloor,NativeScriptBinding_ItemAlignFacingToVelocity};
 static const char* const names[]={"Floor","Walls","Ceiling","Reflect","Align","Facing"};
 for(unsigned flags=0;flags<16;++flags){seed();terrain(flags);compare(semantics[1]);seed();terrain(flags);compare(semantics[2]);}
 for(unsigned landed=0;landed<2;++landed)for(unsigned adjust=0;adjust<2;++adjust){seed();terrain(15);collision_result=landed;item.xDCE_flag.b7=adjust;compare(semantics[0]);}
 static const Vec3 velocities[]={{2,3,4},{-2,3,4},{0,0,0},{0,-5,7},{4,-2,-3},{-0.0f,2,1}};
 static const float multipliers[]={0,0.5f,1,-0.25f};
 for(unsigned walls=0;walls<4;++walls)for(unsigned v=0;v<6;++v)for(unsigned scale=0;scale<4;++scale){seed();terrain(walls);item.x40_vel=velocities[v];attributes.x58=multipliers[scale];compare(semantics[3]);}
 static const Vec3 normals[]={{0,1,0},{0.6f,0.8f,0},{-0.6f,0.8f,0},{0,-1,0},{0,0,0}};
 for(unsigned flags=0;flags<16;++flags)for(unsigned n=0;n<5;++n)for(unsigned dynamic=0;dynamic<2;++dynamic)for(unsigned axis=0;axis<2;++axis)for(unsigned facing=0;facing<2;++facing){
  seed();terrain(flags);item.x378_itemColl.floor.normal=normals[n];item.facing_dir=facing?1:-1;item.xDC8_word.flags.x17=axis;
  if(dynamic){model_descriptor.x4_bone_count=1;item.xBBC_dynamicBoneTable=&bones;bones.bones[0]=&joints[1];}
  compare(semantics[4]);
 }
 static const float speeds[]={-2,-0.000011f,-0.00001f,-0.000009f,-0.0f,0,0.000009f,0.00001f,0.000011f,2,INFINITY,NAN};
 for(unsigned speed=0;speed<12;++speed)for(int facing=-1;facing<=1;++facing){seed();item.x40_vel.x=speeds[speed];item.facing_dir=(float)facing;compare(semantics[5]);}
 for(unsigned op=0;op<6;++op){
  seed();terrain(15);collision_result=1;item.x40_vel=(Vec3){-2,3,4};NativeScriptValue out=sentinel();CHECK(run(names[op],&out)==NativeScriptVM_Ok);
  CHECK(out.type==((op==1||op==2)?NativeScriptType_I32:op==3?NativeScriptType_Bool:NativeScriptType_Void));++cases;
  for(unsigned m=1;m<=7;++m){seed();terrain(15);observed=true;mutation=m;rejected(semantics[op]);seed();terrain(15);observed=true;mutation=m;out=sentinel();CHECK(run(names[op],&out)!=NativeScriptVM_Ok);CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;}
  for(unsigned m=0;m<5;++m){seed();switch(m){case 0:storage.live=false;break;case 1:++item.x1C;break;case 2:item.native_removal_in_progress=true;break;case 3:actor->user_data=NULL;break;case 4:actor->classifier=HSD_GOBJ_CLASS_FIGHTER;break;}rejected(semantics[op]);}
  seed();NativeScriptRuntimeContext c=context();NativeScriptValue extra={.type=NativeScriptType_Bool,.as.u64=1};out=sentinel();CHECK(!nativeScriptBindingItemObservations_Invoke(&c,semantics[op],&extra,1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 }
 // Native branches that do not read attributes or graph retain their genuine no-op defaults.
 seed();item.xCC_item_attr=NULL;NativeScriptValue out=sentinel();CHECK(call(semantics[3],&out)&&out.type==NativeScriptType_Bool&&!out.as.u64);++cases;
 seed();terrain(1);item.xCC_item_attr=NULL;rejected(semantics[3]);
 seed();actor->hsd_obj=NULL;article.x10_modelDesc=NULL;CHECK(call(semantics[4],&out)&&out.type==NativeScriptType_Void);++cases;
 seed();terrain(8);article.x10_modelDesc=NULL;rejected(semantics[4]);
 seed();terrain(8);actor->hsd_obj=NULL;rejected(semantics[4]);
 seed();terrain(8);model_descriptor.x4_bone_count=1;rejected(semantics[4]);
 seed();terrain(8);model_descriptor.x4_bone_count=1;item.xBBC_dynamicBoneTable=&bones;rejected(semantics[4]);
 seed();terrain(8);wrong.head.obj_size=sizeof(HSD_JObj);HSD_CLASS_METHOD(&joints[0])=&wrong;rejected(semantics[4]);
 seed();terrain(8);subclass.head.obj_size=1;rejected(semantics[4]);
 for(unsigned m=0;m<=9;++m){seed();terrain(8);observed=true;mutation=m;HSD_CLASS_INFO(&hsdJObj)->head.flags=0;HSD_CLASS_INFO(&hsdJObj)->head.info_init=class_initialize;unsigned before=class_initializations;if(m)rejected(semantics[4]);else{CHECK(call(semantics[4],&out)&&out.type==NativeScriptType_Void);++cases;}CHECK(class_initializations==before+1);}
}
#include "launch_sweep_cases.h"
#include "normal_sweep_cases.h"
int main(void){compile();launch_sweep_cases();normal_sweep_cases();
 for(unsigned bits=0;bits<16;++bits)for(unsigned floor=0;floor<2;++floor)for(unsigned adjust=0;adjust<2;++adjust){seed();collision_result=floor;item.xDCE_flag.b7=adjust;item.x378_itemColl.env_flags=(bits&1?Collide_LeftWallMask:0)|(bits&2?Collide_RightWallMask:0)|(bits&4?Collide_CeilingMask:0)|(bits&8?Collide_FloorMask:0);item.x378_itemColl.floor.index=11;item.x378_itemColl.left_facing_wall.index=12;item.x378_itemColl.right_facing_wall.index=13;item.x378_itemColl.ceiling.index=14;Item before=item;int expected=it_8026DAA8(actor);Item after=item;unsigned expected_events=events;item=before;events=0;NativeScriptValue out=sentinel();CHECK(call(NativeScriptBinding_ItemSweepStageFlags,&out));CHECK(out.type==NativeScriptType_I32&&out.as.i64==expected);CHECK(!memcmp(&after,&item,sizeof(item)));CHECK(events==expected_events);++pairs;++cases;}
 for(unsigned flags=0;flags<16;++flags)for(unsigned active=0;active<16;++active){seed();for(unsigned i=0;i<4;++i){joints[i].flags=flags&(1u<<i)?JOBJ_INSTANCE:0;joints[i].aobj=&animations[i];animations[i].flags=active&(1u<<i)?0:AOBJ_NO_ANIM;}bool expected=!it_80272C6C(actor);Item before=item;NativeScriptValue out=sentinel();CHECK(call(NativeScriptBinding_ItemAnimationFinished,&out));CHECK(out.type==NativeScriptType_Bool&&out.as.u64==expected);CHECK(!memcmp(&before,&item,sizeof(item)));++pairs;++cases;}
 seed();actor->hsd_obj=NULL;NativeScriptValue out=sentinel();CHECK(call(NativeScriptBinding_ItemAnimationFinished,&out)&&out.type==NativeScriptType_Bool&&out.as.u64);++cases;
 seed();Item expected=item;expected.xD5C=0;CHECK(call(NativeScriptBinding_ItemDisableSurfaceResponse,&out)&&out.type==NativeScriptType_Void);CHECK(!memcmp(&expected,&item,sizeof(item)));++pairs;++cases;
 seed();collision_result=1;CHECK(run("Sweep",&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_I32&&out.as.i64==1);CHECK(run("Finished",&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Bool&&out.as.u64);CHECK(run("Disable",&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Void&&item.xD5C==0);
 for(unsigned m=1;m<=7;++m){seed();observed=true;mutation=m;rejected(NativeScriptBinding_ItemSweepStageFlags);seed();observed=true;mutation=m;out=sentinel();CHECK(run("Sweep",&out)!=NativeScriptVM_Ok);CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;}
 unsigned semantics[]={NativeScriptBinding_ItemDisableSurfaceResponse,NativeScriptBinding_ItemSweepStageFlags,NativeScriptBinding_ItemAnimationFinished};for(unsigned op=0;op<3;++op)for(unsigned m=0;m<7;++m){seed();switch(m){case 0:++item.x1C;break;case 1:storage.live=false;break;case 2:item.native_removal_in_progress=true;break;case 3:program.owner++;break;case 4:actor->classifier=HSD_GOBJ_CLASS_FIGHTER;break;case 5:program.article=NULL;break;case 6:bank.valid=false;break;}rejected(semantics[op]);if(m==3)program.owner--;if(m==5)program.article=&article;if(m==6)bank.valid=true;}
 seed();wrong.head.obj_size=sizeof(HSD_JObj);HSD_CLASS_METHOD(&joints[0])=&wrong;rejected(NativeScriptBinding_ItemAnimationFinished);seed();subclass.head.obj_size=1;rejected(NativeScriptBinding_ItemAnimationFinished);
 for(unsigned m=0;m<=9;++m){seed();observed=true;mutation=m;HSD_CLASS_INFO(&hsdJObj)->head.flags=0;HSD_CLASS_INFO(&hsdJObj)->head.info_init=class_initialize;unsigned before=class_initializations;if(m)rejected(NativeScriptBinding_ItemAnimationFinished);else{CHECK(call(NativeScriptBinding_ItemAnimationFinished,&out)&&out.type==NativeScriptType_Bool&&out.as.u64);++cases;}CHECK(class_initializations==before+1);}
 seed();NativeScriptRuntimeContext invalid=context();invalid.scene_epoch++;out=sentinel();CHECK(!nativeScriptBindingItemObservations_Read(&invalid,NativeScriptBinding_ItemAnimationFinished,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 invalid=context();invalid.source_class=NativeCallbackSource_Fighter;out=sentinel();CHECK(!nativeScriptBindingItemObservations_Invoke(&invalid,NativeScriptBinding_ItemDisableSurfaceResponse,NULL,0,&out,&diagnostic));CHECK(item.xD5C==2&&out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 NativeScriptRuntimeContext c=context();NativeScriptValue extra={.type=NativeScriptType_F32,.as.f32=1};out=sentinel();CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemSweepStageFlags,NULL,0,&extra,1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 extensions();printf("{\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;}
