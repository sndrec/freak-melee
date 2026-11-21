#include "fixture.h"
#include <melee/ft/native_script_binding_model_graph.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftparts.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""

enum {N=1024,I=4096,F=32,V=64};
NativeSourcePackage graph_package;
#define bank graph_package.compiler.bank
static char source_bytes[8192];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch call_graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static MotionState motions[342];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=part_tables;
static struct Fighter_804D6540_t* omission_tables[Ft_Kind_RuntimeCapacity];struct Fighter_804D6540_t** Fighter_804D6540=omission_tables;
static Fighter fighters[128],substitute;static unsigned allocated,checks,pairs,vm_calls,native_failures,rejections,compiler_negatives;
static FighterBone bones[6],other_bones[6];static HSD_JObj roots[2],parents[2],children[2],siblings[2],created[2];
static HSD_DObj draw_objects[2];static HSD_DObj* draw_list[124],*other_draws[124];
static HSD_Joint descriptor,child_descriptor,sibling_descriptor;static HSD_DObjDesc mesh;
static Fighter* current_fighter;static unsigned loads,removals,registrations,fail_load,completion_mode;
static HSD_GObj* completed_source;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"Model graph %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)

static void configure(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},.declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=call_graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void prepare(void)
{
 static const char* bad[]={
  "fighter:\n    behavior independent\nfunc Invalid():\n    model_graphs.install(6.0, 0)\n",
  "fighter:\n    behavior independent\nfunc Invalid():\n    model_graphs.install(6, 0.0)\n",
  "fighter:\n    behavior independent\nfunc Invalid():\n    model_graphs.install(6)\n",
  "fighter:\n    behavior independent\nfunc Invalid():\n    model_graphs.install(6, 0, 1)\n"};
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};const NativeScriptCompileBank* accepted=NULL;
 for(unsigned n=0;n<sizeof(bad)/sizeof(*bad);++n){configure();NativeScriptCompileRequest request={.source=bad[n],.source_bytes=(uint32_t)strlen(bad[n]),.motion_layout=&layout};CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));++compiler_negatives;}
 configure();const char* source="fighter:\n    behavior independent\nstate bool installed\nstate int zero\nfunc Install():\n    state.installed = model_graphs.install(6, 0)\nfunc Fault():\n    state.zero = 1 / state.zero\naction Placeholder:\n    on tick:\n        pass\n";
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,NULL,&graph_package.program,&diagnostic));
 NativeSourcePackage* p=&graph_package;p->address=p;p->target=Ft_Kind_CustomFirst;p->ready=p->assets.ready=true;
 p->assets.facts.moves=layout;p->assets.facts.joints=5;p->assets.facts.omission_count=1;p->assets.facts.layout.joint_graph_mask=1u<<6;
 p->assets.parts=(FighterPartsTable){.parts_num=6,.joint_to_part=p->assets.joint_to_part,.part_to_joint=p->assets.part_to_joint};
 p->assets.omissions=(struct Fighter_804D6540_t){.x0=p->assets.omission_rows,.x4=1};p->assets.motions=motions;
 p->assets.character.data.x48_items=p->assets.character.fighter_items;
 part_tables[p->target]=&p->assets.parts;omission_tables[p->target]=&p->assets.omissions;
 gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 nativeScriptFighter_PublishProgram(p->target,&p->program);
}
static void reset(unsigned mode,unsigned ordinal)
{
 memset(bones,0,sizeof(bones));memset(roots,0,sizeof(roots));memset(parents,0,sizeof(parents));memset(children,0,sizeof(children));memset(siblings,0,sizeof(siblings));memset(created,0,sizeof(created));memset(draw_list,0,sizeof(draw_list));memset(draw_objects,0,sizeof(draw_objects));
 memset(&descriptor,0,sizeof(descriptor));memset(&child_descriptor,0,sizeof(child_descriptor));memset(&sibling_descriptor,0,sizeof(sibling_descriptor));
 loads=removals=registrations=fail_load=completion_mode=0;completed_source=NULL;
 graph_package.assets.omission_rows[0]=(struct Fighter_804D6540_x0_t){5,2,mode,ordinal};
 memset(graph_package.assets.detachable_joints,0,sizeof(graph_package.assets.detachable_joints));
 graph_package.assets.detachable_joints[6][ordinal/32]=UINT32_C(1)<<(ordinal%32);
 graph_package.assets.character.fighter_items[6]=&descriptor;graph_package.assets.character.joint_graphs[6]=&descriptor;
 bones[0].joint=&roots[0];bones[0].x4_jobj2=&roots[1];bones[2].joint=&parents[0];bones[2].x4_jobj2=&parents[1];bones[2].xC=7;
 for(unsigned n=0;n<2;++n){roots[n].child=&parents[n];parents[n].parent=&roots[n];parents[n].child=&children[n];parents[n].next=&siblings[n];children[n].parent=&parents[n];siblings[n].parent=&roots[n];}
 descriptor.scale=(Vec3){1,2,3};descriptor.position=(Vec3){4,5,6};child_descriptor.scale=(Vec3){7,8,9};child_descriptor.position=(Vec3){10,11,12};sibling_descriptor.scale=(Vec3){13,14,15};sibling_descriptor.position=(Vec3){16,17,18};descriptor.child=&child_descriptor;descriptor.next=&sibling_descriptor;
}
static void remove_data(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static HSD_GObj* actor(bool owned)
{
 CHECK(allocated<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
 Fighter* fp=&fighters[allocated++];*fp=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.x8_spawnNum=100+allocated,.ft_data=&graph_package.assets.character.data,.parts=bones,.x8AC_animSkeleton=&roots[1],.dobj_list={.data=draw_list}};
 o->hsd_obj=&roots[0];GObj_InitUserData(o,4,remove_data,fp);current_fighter=fp;if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static void release(HSD_GObj* o){o->hsd_obj=NULL;HSD_GObjFree(o);}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static bool invoke(HSD_GObj* o,int slot,int omission,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(o);NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=slot},{.type=NativeScriptType_I32,.as.i64=omission}};return nativeScriptBindings_Invoke(&c,NativeScriptBinding_ModelGraphsInstall,NULL,0,args,2,out,&diagnostic);}
static uint32_t function(const char* name)
{for(unsigned n=0;n<bank.declarations.function_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.functions[n].name,name))return n;CHECK(false);return 0;}
static NativeScriptVMStatus helper(HSD_GObj* o,const char* name)
{NativeScriptValue out;++vm_calls;return nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,function(name),NULL,0,&out,&diagnostic);}
static bool stored(HSD_GObj* o)
{NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));for(unsigned n=0;n<bank.declarations.value_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,bank.declarations.values[n].name,"installed")){NativeScriptValue v;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&v,&diagnostic));CHECK(v.type==NativeScriptType_Bool);return v.as.u64!=0;}CHECK(false);return false;}

HSD_JObj* ftParts_8007482C(HSD_Joint* source)
{
 ++loads;CHECK(!source->child&&!source->next);if(loads==fail_load)return NULL;CHECK(loads<=2);
 HSD_JObj* result=&created[loads-1];result->scale=source->scale;result->translate=source->position;if(source->u.dobjdesc)result->u.dobj=&draw_objects[loads-1];return result;
}
HSD_JObj* HSD_JObjRemove(HSD_JObj* joint){CHECK(joint==&created[0]);++removals;return NULL;}
void ftParts_80074194(Fighter* fp,FighterBone* bone,HSD_JObj* joint,int* index,u32 depth)
{CHECK(fp==current_fighter&&bone==&bones[5]);++registrations;bone->joint=joint;bone->flags_b1=true;bone->xC=depth;if(joint->u.dobj)draw_list[(*index)++]=joint->u.dobj;}
#include "native_parts.inc"
void graph_completed(HSD_GObj* o)
{
 if(o!=completed_source || !completion_mode)return;unsigned mode=completion_mode;completion_mode=0;Fighter* fp=o->user_data;
 switch(mode){
 case 1:CHECK(helper(o,"Fault")==NativeScriptVM_Error);break;
 case 2:release(o);break;
 case 3:fp->x8_spawnNum++;break;
 case 4:substitute=*fp;o->user_data=&substitute;break;
 case 5:graph_package.program.identity++;break;
 case 6:fp->parts=other_bones;break;
 case 7:fp->dobj_list.data=other_draws;break;
 case 8:omission_tables[graph_package.target]=NULL;break;
 case 9:graph_package.assets.omission_rows[0].x3=1;break;
 case 10:graph_package.assets.character.fighter_items[6]=&child_descriptor;break;
 }
}
static void result(unsigned mode,unsigned ordinal)
{
 CHECK(loads==2&&registrations==1&&removals==0);CHECK(bones[5].joint==&created[0]&&bones[5].x4_jobj2==&created[1]&&bones[5].flags_b2);CHECK(bones[5].xC==7+(mode<=1));
 float expected=ordinal==1?7:ordinal==2?13:1;CHECK(created[0].scale.x==expected&&created[1].scale.x==expected);
 for(unsigned n=0;n<2;++n){CHECK(created[n].parent==(mode<=1?&parents[n]:&roots[n]));if(mode==0)CHECK(parents[n].child==&created[n]&&created[n].child==&children[n]&&children[n].parent==&created[n]);if(mode==1)CHECK(parents[n].child==&created[n]&&created[n].next==&children[n]);if(mode==2)CHECK(parents[n].next==&created[n]&&created[n].child==&siblings[n]);if(mode==3)CHECK(parents[n].next==&created[n]&&created[n].next==&siblings[n]);}
}
static void comparisons(void)
{
 for(unsigned mode=0;mode<4;++mode)for(unsigned choice=0;choice<4;++choice){unsigned ordinal=choice==3?255:choice;
  reset(mode,ordinal);HSD_GObj* original=actor(false);ftParts_800753D4(original->user_data,graph_package.assets.omission_rows,&descriptor);result(mode,ordinal);release(original);
  reset(mode,ordinal);HSD_GObj* o=actor(true);NativeScriptValue out;CHECK(invoke(o,6,0,&out)&&out.type==NativeScriptType_Bool&&out.as.u64);result(mode,ordinal);CHECK(invoke(o,6,0,&out)&&!out.as.u64&&loads==2);release(o);
  reset(mode,ordinal);o=actor(true);CHECK(helper(o,"Install")==NativeScriptVM_Ok&&stored(o));result(mode,ordinal);CHECK(helper(o,"Install")==NativeScriptVM_Ok&&!stored(o)&&loads==2);release(o);++pairs;
 }
}
static void failure_cases(void)
{
 reset(0,255);
 HSD_GObj* dependent=actor(true);
 graph_package.assets.detachable_joints[6][7]=0;
 NativeScriptValue rejected;
 CHECK(invoke(dependent,6,0,&rejected)&&!rejected.as.u64&&!loads&&!registrations);
 release(dependent);
 for(unsigned scenario=0;scenario<15;++scenario){reset(0,255);HSD_GObj* o=actor(true);int slot=6,omission=0;Fighter* fp=o->user_data;
  switch(scenario){case 0:slot=-1;break;case 1:slot=16;break;case 2:omission=-1;break;case 3:omission=1;break;
  case 4:graph_package.assets.character.fighter_items[6]=graph_package.assets.character.joint_graphs[6]=NULL;break;
  case 5:graph_package.assets.omission_rows[0].x3=3;break;case 6:graph_package.assets.omission_rows[0].x0=6;break;case 7:graph_package.assets.omission_rows[0].x1=6;break;case 8:graph_package.assets.omission_rows[0].x2=4;break;
  case 9:bones[2].x4_jobj2=NULL;break;case 10:fail_load=1;break;case 11:fail_load=2;break;case 12:descriptor.flags=JOBJ_SPLINE;break;
  case 13:descriptor.u.dobjdesc=&mesh;fp->dobj_list.count=124;break;case 14:bones[5].joint=&created[0];break;}
  NativeScriptValue out;CHECK(invoke(o,slot,omission,&out)&&out.type==NativeScriptType_Bool&&!out.as.u64);CHECK(registrations==0&&parents[0].child==&children[0]);CHECK(scenario!=11||removals==1);++native_failures;release(o);
 }
 reset(0,255);HSD_GObj* o=actor(true);descriptor.u.dobjdesc=&mesh;((Fighter*)o->user_data)->dobj_list.count=3;draw_list[0]=&draw_objects[1];NativeScriptValue out;CHECK(invoke(o,6,0,&out)&&out.as.u64);CHECK(((Fighter*)o->user_data)->dobj_list.count==4&&draw_list[0]==&draw_objects[1]&&draw_list[3]==&draw_objects[0]);release(o);
 reset(0,255);o=actor(true);fail_load=2;CHECK(helper(o,"Install")==NativeScriptVM_Ok&&!stored(o)&&removals==1&&registrations==0);release(o);
}
static void invalid_cases(void)
{
 reset(0,255);HSD_GObj* o=actor(true);NativeScriptRuntimeContext c=context(o);NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel,args[2]={{.type=NativeScriptType_I32,.as.i64=6},{.type=NativeScriptType_I32,.as.i64=0}};
 c.generation++;CHECK(!nativeScriptBindingModelGraph_Invoke(&c,NativeScriptBinding_ModelGraphsInstall,args,2,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));++rejections;
 c=context(o);c.scene_epoch=0;CHECK(!nativeScriptBindingModelGraph_Invoke(&c,NativeScriptBinding_ModelGraphsInstall,args,2,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));++rejections;
 c=context(o);c.source_class=NativeCallbackSource_Item;CHECK(!nativeScriptBindingModelGraph_Invoke(&c,NativeScriptBinding_ModelGraphsInstall,args,2,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));++rejections;
 c=context(o);CHECK(!nativeScriptBindingModelGraph_Invoke(&c,NativeScriptBinding_ModelGraphsInstall,args,1,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));++rejections;
 args[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=6};CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ModelGraphsInstall,NULL,0,args,2,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));++rejections;
 args[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=6};part_tables[graph_package.target]=NULL;CHECK(!invoke(o,6,0,&out)&&!memcmp(&out,&sentinel,sizeof(out)));++rejections;part_tables[graph_package.target]=&graph_package.assets.parts;CHECK(!loads);release(o);
 reset(0,255);o=actor(false);CHECK(!invoke(o,6,0,&out)&&!memcmp(&out,&sentinel,sizeof(out))&&!loads);++rejections;release(o);
 for(unsigned mode=1;mode<=10;++mode){reset(0,255);o=actor(true);completed_source=o;completion_mode=mode;uint64_t identity=graph_package.program.identity;out=sentinel;
  CHECK(!invoke(o,6,0,&out)&&!memcmp(&out,&sentinel,sizeof(out))&&completion_mode==0&&registrations==1);++rejections;
  graph_package.program.identity=identity;omission_tables[graph_package.target]=&graph_package.assets.omissions;if(mode!=2)release(o);
 }
 reset(0,255);o=actor(true);completed_source=o;completion_mode=3;CHECK(helper(o,"Install")==NativeScriptVM_SourceRetired);++rejections;release(o);
}
int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 comparisons();failure_cases();invalid_cases();CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS Model graphs\",\"checks\":%u,\"original_native_vm_pairs\":%u,\"vm_calls\":%u,\"native_failure_cases\":%u,\"rejections\":%u,\"compiler_negatives\":%u}\n",checks,pairs,vm_calls,native_failures,rejections,compiler_negatives);return 0;
}
