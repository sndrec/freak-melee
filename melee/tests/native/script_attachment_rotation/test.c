#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_binding_item_observations.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_item_owner.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
NativeSourcePackage packages[2];
static Article article;static ItemAttr attributes;static ItemModelDesc model;
static Fighter fighters[96];static Item items[96];
static FighterBone parts[96][4];static HSD_JObj joints[96][4];
static FighterPartsTable stock_table,*tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
HSD_JObjInfo hsdJObj;
static unsigned checks,cases,comparisons,vm_calls,negatives,mutations,fighter_count,item_count,mutation;
static HSD_GObj* observed_item,*observed_creator,*other_creator;
static NativeScriptDiagnostic diagnostic;
static const char* source=
 "fighter:\n    behavior independent\nstate int zero\nstate int sink\nfunc Fault():\n    state.sink = 1 / state.zero\n"
 "action Idle:\n    on tick:\n        pass\narticle Probe states 1:\n    state float x\n    state float y\n    state float z\n"
 "    on signal Read:\n        item.read_attachment_matrix_rotation(item.state.x, item.state.y, item.state.z)\n"
 "    on signal Local:\n        let float x = 41\n        let float y = 42\n        let float z = 43\n"
 "        item.read_attachment_matrix_rotation(x, y, z)\n        item.state.x = x\n        item.state.y = y\n        item.state.z = z\n"
 "    state 0 anim -1:\n";
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"attachment %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static uint32_t bits(float x){uint32_t n;memcpy(&n,&x,4);return n;}
static NativeScriptValue floating(float x){return(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=x};}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static void prepare(void)
{
 article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
 NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 for(unsigned n=0;n<2;++n){NativeSourcePackage* p=&packages[n];p->address=p;p->target=(FighterKind)(Ft_Kind_CustomFirst+n);
  CHECK(nativeSourceBank_Allocate(&p->compiler,&diagnostic));p->assets.facts.moves=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};
  NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&p->assets.facts.moves,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
  const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&p->compiler.bank,&request,&accepted,&diagnostic));
  CHECK(nativeScriptFighter_PrepareProgram(p->target,&p->compiler.bank,&p->assets.facts.moves,&metadata,&p->program,&diagnostic));
  p->assets.facts.joints=4;p->assets.parts=(FighterPartsTable){.parts_num=4,.part_to_joint=p->assets.part_to_joint,.joint_to_part=p->assets.joint_to_part};
  // Deliberately nonidentity semantic remaps: the attachment selector is raw.
  for(unsigned j=0;j<4;++j)p->assets.part_to_joint[j]=p->assets.joint_to_part[j]=(uint8_t)(3-j);
  p->ready=p->assets.ready=true;tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;
  ftData_CharacterStateTables[p->target]=p->assets.motions;nativeScriptFighter_PublishProgram(p->target,&p->program);
 }
 stock_table.parts_num=4;tables[Ft_Kind_Link]=&stock_table;
 HSD_CLASS_INFO(&hsdJObj)->head.flags=1;HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(int package)
{
 CHECK(fighter_count<96);unsigned n=fighter_count++;HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
 FighterKind kind=package<0?Ft_Kind_Link:packages[package].target;
 fighters[n]=(Fighter){.gobj=o,.kind=kind,.behavior_kind=kind,.x8_spawnNum=100+n,.ft_data=&packages[package<0?0:package].assets.character.data,.parts=parts[n]};
 GObj_InitUserData(o,4,fighter_remove,&fighters[n]);
 for(unsigned j=0;j<4;++j){HSD_JObj* joint=&joints[n][j];*joint=(HSD_JObj){.flags=JOBJ_MTX_DIRTY,.rotate={.x=9,.y=8,.z=7},.scale={2,3,4},.translate={91,92,93}};
  HSD_CLASS_METHOD(joint)=HSD_CLASS_INFO(&hsdJObj);parts[n][j].joint=joint;Vec3 rotation={.1f*j,.2f*j,-.3f*j};HSD_MkRotationMtx(joint->mtx,&rotation);}
 o->obj_kind=HSD_GObj_JObjKind;o->hsd_obj=&joints[n][0];if(package>=0)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(HSD_GObj* creator)
{
 CHECK(item_count<96);unsigned n=item_count++;HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(o);
 const NativeScriptItemProgram* p=&nativeScriptFighter_Program(packages[0].target)->items.programs[0];
 items[n]=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=500+n,.xC4_article_data=&article,.owner=creator};
 GObj_InitUserData(o,6,item_remove,&items[n]);nativeItem_CaptureCreator(o,creator);
 CHECK(nativeScriptItem_ProgramValid(p));CHECK(nativeScriptItem_NativeIdentity(o,p,items[n].x1C));
 CHECK(!creator||nativeItem_Creator(o)==creator);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Item,.generation=((Item*)o->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
 NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&view));
 const NativeScriptCompileBank* bank=nativeScriptItem_Spec(o)->bank;
 for(unsigned n=0;n<view.program->value_count;++n)if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank->tree,view.program->values[n].name,name)){
  NativeScriptValue v;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));
  CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&v,&diagnostic));return v;}
 CHECK(false);return(NativeScriptValue){0};
}
static void expected(HSD_GObj* o,Vec3* result)
{
 *result=(Vec3){0};Fighter_GObj* creator=nativeScriptItem_Creator(o);
 if(!creator||creator!=nativeScriptItem_AttachmentOwner(o))return;
 Fighter* f=creator->user_data;Item* i=o->user_data;
 if(!ftPartsTable||!ftPartsTable[f->kind]||!f->parts||i->xDC4>=ftPartsTable[f->kind]->parts_num)return;
 HSD_JObj* raw=f->parts[i->xDC4].joint;if(raw)HSD_MtxGetRotation(raw->mtx,result);
}
static void outputs(const NativeScriptValue* args,const Vec3* e)
{CHECK(args[0].type==NativeScriptType_F32&&bits(args[0].as.f32)==bits(e->x));CHECK(args[1].type==NativeScriptType_F32&&bits(args[1].as.f32)==bits(e->y));CHECK(args[2].type==NativeScriptType_F32&&bits(args[2].as.f32)==bits(e->z));}
static void compare(HSD_GObj* o)
{
 ++cases;Vec3 reference;expected(o,&reference);NativeScriptRuntimeContext c=context(o);unsigned before=definition_calls;
 NativeScriptValue args[]={floating(91),floating(92),floating(93)},out={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(nativeScriptBindingItemObservations_Invoke(&c,NativeScriptBinding_ItemReadAttachmentMatrixRotation,args,3,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void);outputs(args,&reference);++comparisons;
 for(unsigned i=0;i<3;++i)args[i]=floating(91+i);
 CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemReadAttachmentMatrixRotation,NULL,0,args,3,&out,&diagnostic));CHECK(out.type==NativeScriptType_Void);outputs(args,&reference);++comparisons;
 const char* names[]={"x","y","z"};
 for(unsigned mode=0;mode<2;++mode){for(unsigned i=0;i<3;++i){NativeScriptValue v=floating(91+i);field(o,names[i],&v);}
  CHECK(nativeScriptItem_RunSignal(o,hash(mode?"Local":"Read"),0,&out)==NativeScriptVM_Ok);++vm_calls;
  for(unsigned i=0;i<3;++i)args[i]=field(o,names[i],NULL);outputs(args,&reference);}
 CHECK(definition_calls==before);
}
static void matrix_cases(HSD_GObj* o)
{
 Fighter* f=nativeScriptItem_Creator(o)->user_data;Item* i=o->user_data;
 for(unsigned n=0;n<36;++n)for(unsigned part=0;part<4;++part){i->xDC4=part;HSD_JObj* joint=f->parts[part].joint;
  Vec3 angle={((int)(n%7)-3)*.22f,((int)(n%5)-2)*.31f,((int)(n%9)-4)*.27f};HSD_MkRotationMtx(joint->mtx,&angle);
  if(n<18){float scale[]={.5f+part,1.25f,2.5f};for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)joint->mtx[row][col]*=scale[col];
   Vec3 native;HSD_MtxGetRotation(joint->mtx,&native);CHECK(fabsf(native.x-angle.x)<.000002f&&fabsf(native.y-angle.y)<.000002f&&fabsf(native.z-angle.z)<.000002f);++comparisons;}
  if(n==18)for(unsigned row=0;row<3;++row)joint->mtx[row][0]*=-1;
  if(n==19)for(unsigned row=0;row<3;++row)joint->mtx[row][1]=0;
  if(n==20)for(unsigned row=0;row<3;++row)joint->mtx[row][2]=1e-30f;
  if(n==21)joint->mtx[0][0]=INFINITY;
  if(n==22)joint->mtx[2][0]=NAN;
  if(n==23||n==24){angle.y=n==23?(float)(M_PI/2):-(float)(M_PI/2);HSD_MkRotationMtx(joint->mtx,&angle);}
  joint->mtx[0][3]=91;HSD_JObj before=*joint;compare(o);CHECK(!memcmp(joint,&before,sizeof(before)));
 }
}
static NativeScriptItemActor* actor(HSD_GObj* o)
{for(unsigned n=0;n<NATIVE_SCRIPT_ITEM_ACTORS;++n)if(nativeScriptItemActors[n].active&&nativeScriptItemActors[n].object==o)return &nativeScriptItemActors[n];CHECK(false);return NULL;}
static void admission_mutation(void)
{
 HSD_CLASS_INFO(&hsdJObj)->head.flags=1;Item* i=observed_item->user_data;Fighter* f=observed_creator->user_data;
 switch(mutation){
 case 1:++i->x1C;break;
 case 2:actor(observed_item)->storage.owner=0;break;
 case 3:++f->x8_spawnNum;break;
 case 4:CHECK(nativeScriptItem_SetAttachmentOwner(observed_item,other_creator));break;
 case 5:++i->xDC4;break;
 case 6:f->parts[1].joint=f->parts[2].joint;break;
 case 7:--tables[f->kind]->parts_num;break;
 case 8:--HSD_CLASS_INFO(&hsdJObj)->head.obj_size;break;
 case 9:++f->kind;break;
 case 10:actor(observed_item)->faulted=true;break;
 case 11:HSD_GObjFree(observed_creator);break;
 case 12:HSD_GObjFree(observed_item);break;
 default:CHECK(false);}
 ++mutations;
}
static void admission_cases(void)
{
 for(unsigned vm=0;vm<2;++vm)for(unsigned n=1;n<=12;++n){observed_creator=fighter(0);observed_item=item(observed_creator);((Item*)observed_item->user_data)->xDC4=1;
  NativeScriptRuntimeContext c=context(observed_item);NativeScriptValue args[]={floating(91),floating(92),floating(93)},out={.type=NativeScriptType_U64,.as.u64=99};
  const char* names[]={"x","y","z"};for(unsigned j=0;j<3;++j)field(observed_item,names[j],&args[j]);
  NativeScriptItemActor* retained=actor(observed_item);static uint8_t before[NATIVE_SCRIPT_ITEM_STATE_BYTES];memcpy(before,retained->bytes,sizeof(before));
  mutation=n;HSD_CLASS_INFO(&hsdJObj)->head.info_init=admission_mutation;HSD_CLASS_INFO(&hsdJObj)->head.flags=0;
  if(vm){NativeScriptVMStatus status=nativeScriptItem_RunSignal(observed_item,hash("Read"),0,&out);CHECK(status==NativeScriptVM_Error||status==NativeScriptVM_SourceRetired);++vm_calls;}
  else{CHECK(!nativeScriptBindingItemObservations_Invoke(&c,NativeScriptBinding_ItemReadAttachmentMatrixRotation,args,3,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);for(unsigned j=0;j<3;++j)CHECK(bits(args[j].as.f32)==bits(91.0f+j));}
  CHECK(!memcmp(before,retained->bytes,sizeof(before)));++negatives;
  HSD_CLASS_INFO(&hsdJObj)->head.flags=1;HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);tables[packages[0].target]->parts_num=4;
 }
}
static void compiler_cases(void)
{
 const char* calls[]={"item.read_attachment_matrix_rotation(item.state.x, item.state.y)","item.read_attachment_matrix_rotation(item.state.x, item.state.y, item.state.z, item.state.x)","item.read_attachment_matrix_rotation(item.state.x, item.state.x, item.state.z)","item.read_attachment_matrix_rotation(0, item.state.y, item.state.z)","item.read_attachment_matrix_rotation(item.state.i, item.state.y, item.state.z)","item.read_attachment_matrix_rotations(item.state.x, item.state.y, item.state.z)"};
 NativeSourceBank rejected={0};CHECK(nativeSourceBank_Allocate(&rejected,&diagnostic));
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
 NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&packages[0].assets.facts.moves,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
 const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&rejected.bank,&request,&accepted,&diagnostic));
 for(unsigned n=0;n<sizeof(calls)/sizeof(calls[0]);++n){char text[2048];snprintf(text,sizeof(text),"fighter:\n    behavior independent\naction Idle:\n    on tick:\n        pass\narticle Probe states 1:\n    state float x\n    state float y\n    state float z\n    state int i\n    on signal Read:\n        %s\n    state 0 anim -1:\n",calls[n]);
  request.source=text;request.source_bytes=(uint32_t)strlen(text);accepted=NULL;
  CHECK(!nativeScriptCompile_Build(&rejected.bank,&request,&accepted,&diagnostic));++negatives;}
 nativeSourceBank_Release(&rejected);
}
int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 HSD_GObj* creators[]={fighter(0),fighter(1),fighter(-1)};other_creator=fighter(-1);
 for(unsigned n=0;n<3;++n){HSD_GObj* o=item(creators[n]);matrix_cases(o);Item* i=o->user_data;Fighter* f=creators[n]->user_data;
  i->owner=other_creator;compare(o);CHECK(nativeScriptItem_Creator(o)==creators[n]);
  CHECK(nativeScriptItem_ClearAttachmentOwner(o));compare(o);CHECK(nativeScriptItem_SetAttachmentOwner(o,other_creator));compare(o);CHECK(nativeScriptItem_SetAttachmentOwner(o,creators[n]));
  const uint32_t missing[]={UINT32_MAX,4,140,UINT32_C(0x80000000)};for(unsigned j=0;j<4;++j){i->xDC4=missing[j];compare(o);}
  i->xDC4=1;HSD_JObj* joint=f->parts[1].joint;f->parts[1].joint=NULL;compare(o);f->parts[1].joint=joint;
  FighterBone* saved=f->parts;f->parts=NULL;compare(o);f->parts=saved;FighterPartsTable* table=tables[f->kind];tables[f->kind]=NULL;compare(o);tables[f->kind]=table;
  table->parts_num=0;compare(o);table->parts_num=4;
 }
 compare(item(NULL));
 // Retired creator relations cannot resolve the replacement in a reused pool slot.
 HSD_GObj* retired=fighter(-1);HSD_GObj* survivor=item(retired);HSD_GObjFree(retired);HSD_GObj* replacement=fighter(-1);CHECK(replacement==retired);compare(survivor);CHECK(!nativeScriptItem_Creator(survivor));
 HSD_GObj* o=item(creators[0]);NativeScriptRuntimeContext c=context(o);NativeScriptValue args[]={floating(91),floating(92),floating(93)},out={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingItemObservations_Invoke(&c,NativeScriptBinding_ItemReadAttachmentMatrixRotation,args,2,&out,&diagnostic));++negatives;
 args[1].type=NativeScriptType_I32;CHECK(!nativeScriptBindingItemObservations_Invoke(&c,NativeScriptBinding_ItemReadAttachmentMatrixRotation,args,3,&out,&diagnostic));++negatives;args[1]=floating(92);
 ++c.scene_epoch;CHECK(!nativeScriptBindingItemObservations_Invoke(&c,NativeScriptBinding_ItemReadAttachmentMatrixRotation,args,3,&out,&diagnostic));++negatives;--c.scene_epoch;
 c.source_class=NativeCallbackSource_Fighter;CHECK(!nativeScriptBindingItemObservations_Invoke(&c,NativeScriptBinding_ItemReadAttachmentMatrixRotation,args,3,&out,&diagnostic));++negatives;
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);for(unsigned j=0;j<3;++j)CHECK(bits(args[j].as.f32)==bits(91.0f+j));
 admission_cases();compiler_cases();
 printf("{\"checks\":%u,\"cases\":%u,\"native_comparisons\":%u,\"vm_calls\":%u,\"negative_cases\":%u,\"admission_mutations\":%u}\n",checks,cases,comparisons,vm_calls,negatives,mutations);return 0;
}
