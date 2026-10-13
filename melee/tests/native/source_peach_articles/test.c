#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_binding_fighter_items.h>
#include <melee/ft/ftparts.h>
#include <melee/it/item.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/itcommon_data.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/lb/native_disc_archive.h>
#include <melee/cm/camera.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/memory.h>
#include <dolphin/os/OSAlloc.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,created,states,vm_calls;
static NativeMatchPoolReference fighter_reference;
static Item_GObj* items[5];
static NativeMatchPoolReference item_refs[5];

static ItCommonAssets item_common;
static FtCommonAssets common;
static unsigned char heap_bytes[16*1024*1024] ATTRIBUTE_ALIGN(64);
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"Peach Articles line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static void free_fighter(void* data){Fighter* f=data;nativeScriptFighter_Detach(f->gobj);CHECK(nativeMatchPool_Free(&nativeFighterPool,fighter_reference));}
static void root_integer(Fighter_GObj* object,const char* name,int32_t value){
 NativeScriptReceiverView view;NativeScriptDiagnostic diagnostic={0};CHECK(nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view));
 for(unsigned n=0;n<view.program->value_count;++n){const NativeScriptValueDeclaration* v=&view.program->values[n];if(!v->owner&&!v->parameter&&nativeScriptSource_SpanEquals(view.program->tree,v->name,name)){NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=value};CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&input,&diagnostic));return;}}
 CHECK(false);
}
static void bind_held(Fighter_GObj* parent,Item_GObj* item){NativeScriptRuntimeContext context={.source=parent,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)parent->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};NativeScriptDiagnostic diagnostic={0};NativeScriptValue result={0},handle={.type=NativeScriptType_U32,.as.u64=((Item*)item->user_data)->x1C};CHECK(nativeScriptBindingFighterItems_Invoke(&context,NativeScriptBinding_ItemsBindHeld,&handle,1,&result,&diagnostic));CHECK(result.type==NativeScriptType_Bool&&result.as.u64==1);}
static void stage(const char* name){printf("{\"stage\":\"%s\",\"checks\":%u}\n",name,checks);fflush(stdout);}
int main(int argc,char** argv)
{
 CHECK(argc==4&&aurora_dvd_open(argv[1]));
 void* bytes=NULL;size_t count=0;CHECK(nativeDiscArchive_Read("PlCo.dat",&bytes,&count));
 void* arena=malloc(16*1024*1024);CHECK(arena&&ftCommonAssets_Import(bytes,count,arena,16*1024*1024,32768,&common));
 ftPartsTable=common.parts;Fighter_804D6540=common.omissions;Fighter_804D64FC=&common.cpu_data.descriptor;
 NativeContentId id;CHECK(nativeContentStore_IdFromHex(argv[3],&id));
 CHECK(nativeFighterRuntime_Load(&id,ChKind_CustomFirst,Ft_Kind_CustomFirst,argv[2],false,false));
 const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst);CHECK(package&&package->ready&&package->program.items.count==5);
 CHECK(nativeScriptFighter_Program(Ft_Kind_CustomFirst)->bank==&package->compiler.bank);CHECK(!nativeFighterRuntime_Definition(Ft_Kind_CustomFirst));CHECK(nativeSourcePackage_CanActivate(package)&&package->leases==1);stage("real-package-publication");
 void* start=OSInitAlloc(heap_bytes,heap_bytes+sizeof(heap_bytes),1);OSHeapHandle heap=OSCreateHeap(start,heap_bytes+sizeof(heap_bytes));CHECK(heap>=0);HSD_SetHeap(heap);OSSetCurrentHeap(heap);HSD_ObjSetHeap(sizeof(heap_bytes),NULL);HSD_ObjInit();HSD_IDSetup();
 HSD_GObjLibInitDataType setup={.p_link_max=63,.gx_link_max=63,.gproc_pri_max=24};HSD_GObj_80391304(&setup);Camera_Init(32);
 void* it_bytes=NULL;size_t it_count=0;CHECK(nativeDiscArchive_Read("ItCo.dat",&it_bytes,&it_count));void* it_arena=malloc(IT_COMMON_IMPORT_ARENA_SIZE);CHECK(it_arena&&itCommonAssets_Import(it_bytes,it_count,it_arena,IT_COMMON_IMPORT_ARENA_SIZE,IT_COMMON_IMPORT_RECORD_CAPACITY,&item_common));
 // Common data is genuinely imported. Fighter-specific source constructors must
 // resolve the owning package, independently of the stock Article bank.
 it_804D6D20=&item_common.public_data;it_804D6D24=item_common.common_articles;it_804D6D28=&item_common.common_data;it_804D6D30=NULL;it_804D6D38=item_common.character_articles;it_804D6D40=&item_common.zako_data;it_804D6D04=item_common.color_entries;Item_80266FCC();stage("real-heap-and-common-item-assets");
 Fighter_GObj* parent=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(parent);
 NativeFighterPoolSlot* slot=nativeMatchPool_Allocate(&nativeFighterPool,&fighter_reference);CHECK(slot);memset(slot,0,sizeof(*slot));Fighter* fighter=&slot->actor;
 *fighter=(Fighter){.gobj=parent,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,.x8_spawnNum=10,.ft_data=(ftData*)&package->assets.character.data,.parts=slot->parts,.dat_attrs=slot->attributes,.dat_attrs_backup=slot->attributes,.x18=package->assets.facts.moves.first,.x1C_actionStateList=(MotionState*)package->assets.common_motions,.x20_actionStateList=package->assets.motions,.x24=package->assets.character.data.xC,.x58C=package->assets.facts.actions,.facing_dir=1,.x34_scale={1,1,1}};
 GObj_InitUserData(parent,4,free_fighter,fighter);CHECK(nativeScriptFighter_Attach(parent));++vm_calls;CHECK(nativeScriptFighter_Status(parent)==NativeScriptVM_Ok);stage("real-fighter-actor-load-hook");
 root_integer(parent,"saved_handle",-1);root_integer(parent,"parasol_handle",-1);root_integer(parent,"toad_handle",-1);root_integer(parent,"vegetable_handle",-1);
 // No renderer/audio/contact substitutes are linked. These explicit native
 // stages do not run stage collision, graphics or effect-producing Created hooks.
 for(unsigned n=0;n<package->program.items.count;++n){
  const NativeScriptItemProgram* prepared=&package->program.items.programs[n];
  const NativeScriptItemProgram* spec=nativeScriptItem_Program(Ft_Kind_CustomFirst,prepared->owner);
  CHECK(spec&&spec!=prepared&&spec->article==prepared->article&&spec->bank==prepared->bank);
  CHECK(nativeScriptItem_Program(Ft_Kind_CustomFirst,spec->owner)==spec);CHECK(spec->article&&spec->article->x10_modelDesc&&spec->article->x0_common_attr);CHECK(spec->drawing==NativeScriptItemDrawing_Standard&&!spec->recipe);
  SpawnItem spawn={.x0_parent_gobj=parent,.x4_parent_gobj2=parent,.kind=spec->kind,.hold_kind=1,.facing_dir=1,.pos={10,20,0},.prev_pos={10,20,0},.vel={1,2,0}};
  printf("{\"construct_kind\":%d,\"owner\":%u,\"null_root\":%s}\n",spec->kind,spec->owner,spec->article->x10_modelDesc->x0_joint?"false":"true");fflush(stdout);
  Item_GObj* object=Item_CreateAuthored(&spawn,Ft_Kind_CustomFirst,spec->owner);CHECK(object);++created;items[n]=object;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&item_refs[n]));
  Item* item=object->user_data;CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok&&nativeScriptItem_Spec(object)==spec);CHECK(item->xC4_article_data==spec->article&&!item->xBC_itemStateContainer);CHECK(object->hsd_obj);CHECK(!item->xBBC_dynamicBoneTable&&!item->x374_dynamicBonesNum);CHECK(!nativeFighterRuntime_CanClear());
  if(item->kind==It_Kind_Peach_Parasol){bind_held(parent,object);fighter->motion_id=144;}
  for(unsigned state=0;state<spec->state_count;++state){
   printf("{\"select_kind\":%d,\"state\":%u,\"animation\":%d}\n",item->kind,state,spec->states[state].animation_id);fflush(stdout);
   Item_80268E5C(object,(int)state,ITEM_ANIM_UPDATE|ITEM_CMD_UPDATE|ITEM_COLANIM_PRESERVE|ITEM_SFX_PRESERVE);
   CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok&&item->msid==(int)state);
   CHECK(!item->animated&&!item->physics_updated&&!item->collided);
   if(spec->states[state].animation_id>=0){CHECK(item->xD0_itemStateDesc==&spec->article->xC_itemStates->x0_itemStateDesc[spec->states[state].animation_id]);}
   else CHECK(!item->x524_cmd.u);
   Item_802694CC(object);CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok);
   NativeScriptValue phase={0};CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Animation,&phase)==NativeScriptVM_Ok);++vm_calls;CHECK(phase.type==NativeScriptType_Bool);++states;
  }
  CHECK(nativeScriptItem_Creator(object)==parent&&nativeScriptItem_AttachmentOwner(object)==parent);
  NativeScriptValue result={0};
  if(item->kind==It_Kind_Peach_Turnip){CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_Created,NULL,0,&result)==NativeScriptVM_Ok);++vm_calls;CHECK(item->xD44_lifeTimer>0);}
  if(item->kind==It_Kind_Peach_Parasol){CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_HeldStatus,NULL,0,&result)==NativeScriptVM_Ok);++vm_calls;CHECK(result.type==NativeScriptType_I32&&result.as.i64==6);}
  if(item->kind==It_Kind_Peach_Explode||item->kind==It_Kind_Peach_ToadSpore){CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Animation,&result)==NativeScriptVM_Ok);++vm_calls;CHECK(result.type==NativeScriptType_Bool);}
  if(item->kind==It_Kind_Peach_Parasol)CHECK(nativeItem_ClearBinding(parent,false));
  printf("{\"constructed_kind\":%d,\"creation_id\":%lu,\"state_count\":%u,\"aobjs\":%u,\"fobjs\":%u}\n",item->kind,(unsigned long)item->x1C,spec->state_count,nativeAObjPool.used,nativeFObjPool.used);fflush(stdout);
 }
 stage("all-five-real-constructors");
 void* snapshot=malloc(1024*1024);void* again=malloc(1024*1024);CHECK(snapshot&&again);size_t snapshot_bytes=0,again_bytes=0;
 CHECK(nativeScriptItem_Save(snapshot,1024*1024,&snapshot_bytes));CHECK(((NativeScriptItemSnapshotHeader*)snapshot)->count==5);CHECK(nativeScriptItem_ValidateSnapshot(snapshot,snapshot_bytes));
 CHECK(nativeScriptItem_SetCreationValue(items[0],3.25f));CHECK(nativeScriptItem_Load(snapshot,snapshot_bytes));float creation_value=99;CHECK(nativeScriptItem_GetCreationValue(items[0],&creation_value)&&creation_value==0);
 CHECK(nativeScriptItem_Save(again,1024*1024,&again_bytes)&&again_bytes==snapshot_bytes&&!memcmp(snapshot,again,snapshot_bytes));free(snapshot);free(again);stage("real-owned-item-component-roundtrip");
 for(unsigned n=0;n<5;++n){Item_8026A8EC(items[n]);CHECK(!nativeMatchPool_Resolve(item_refs[n]));CHECK(!nativeScriptItem_IsAttached(items[n]));CHECK(nativeScriptFighter_Status(parent)==NativeScriptVM_Ok);}
 CHECK(nativeAObjPool.used==0&&nativeFObjPool.used==0);CHECK(nativeGObjProcPool.used==0);stage("all-five-native-removals");
 HSD_GObjFree(parent);CHECK(nativeGObjPool.used==0&&nativeFighterPool.used==0);CHECK(nativeGObjPool_RetireScene());CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());printf("{\"checks\":%u,\"created\":%u,\"states\":%u,\"explicit_vm_calls\":%u,\"item_component_bytes\":%zu}\n",checks,created,states,vm_calls,snapshot_bytes);return 0;
}
