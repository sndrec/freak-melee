#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ftdynamics.h>
#include <melee/ft/ftcommon.h>
#include <melee/db/db.h>
#include <melee/ft/native_fighter_attachment.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_item_behavior.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_article_model.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_26B1.h>
#include <melee/it/inlines.h>
#include <melee/it/itarticle_data.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lbshadow.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efasync.h>
#include <melee/cm/camera.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum {N=1024,I=4096,F=64,V=64,ITEMS=64};
static char bytes[8192];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
NativeSourcePackage sleep_package;
#define bank sleep_package.compiler.bank
static MotionState motions[342];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_OnUserDataRemove[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable;
static Item item_data[ITEMS];static NativeMatchPool item_pool;static NativeMatchPoolSlot item_slots[ITEMS];static uint32_t item_free[ITEMS];
static Article article;static ItemAttr common_attributes;static ItemModelDesc model_descriptor;static NativeScriptItemProgram item_program;
static ItemLogicTable logic;static HSD_ObjAllocData item_alloc_data,item_dynamic_bones_alloc_data;
u32 it_804D6D10=1;
static unsigned checks,cases,load_callbacks,remove_callbacks,destroy_callbacks,interaction_callbacks,refusals,destructions,payload_frees,leaves;
static bool spawn_from_destroy;static HSD_GObj* probe_target;static HSD_GObj* other_owner;static unsigned mutation,load_action;static bool owned_creation=true;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"constructor items case%u line%u %s (%s)\n",cases,__LINE__,#x,diagnostic.message[0]?diagnostic.message:nativeScriptRuntime_VM()->diagnostic.message);exit(1);}}while(0)
static HSD_GObj* make_item(HSD_GObj*,HSD_GObj*,bool);
void Item_OnUserDataRemove(void*);
void it_8026B7F8(HSD_GObj*);
static bool fixture_factory_admission(HSD_GObj*,SpawnItem*,NativeItemBehaviorSelection,FighterKind,uint32_t);
static void foreign_destroyed(HSD_GObj* object){(void)object;++destroy_callbacks;}
static HSD_GObj* make_fighter(bool owned,bool constructing)
{
 HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
 NativeMatchPoolReference reference;NativeFighterPoolSlot* slot=nativeMatchPool_Allocate(&nativeFighterPool,&reference);CHECK(slot);
 Fighter* f=&slot->actor;f->gobj=object;f->kind=f->behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Mario;
 f->ft_data=&sleep_package.assets.character.data;f->native_constructing=constructing;
 GObj_InitUserData(object,4,Fighter_Unload_8006DABC,f);if(owned){bool accepted=nativeScriptFighter_Attach(object);CHECK(accepted==(load_action==0));}return object;
}
static HSD_GObj* make_item(HSD_GObj* creator,HSD_GObj* parent,bool owned)
{
 HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
 NativeMatchPoolReference ref;Item* item=nativeMatchPool_Allocate(&item_pool,&ref);CHECK(item);
 GObj_InitUserData(object,6,Item_OnUserDataRemove,item);
 SpawnItem spawn={.kind=It_Kind_Mario_Fire,.x0_parent_gobj=creator,.x4_parent_gobj2=parent?parent:creator};
 if(!fixture_factory_admission(object,&spawn,NativeItemBehavior_Parent,Ft_Kind_CustomFirst,owned?item_program.owner:0)){
  ++refusals;HSD_GObjFree(object);return NULL;
 }
 item->owner=creator;item->destroy_type=4;item->xB8_itemLogicTable=&logic;item->msid=-1;
 if(owned){item->native_article_author_kind=Ft_Kind_CustomFirst;item->native_script_owner=item_program.owner;item->xC4_article_data=&article;CHECK(nativeScriptItem_Attach(object,nativeScriptItem_Program(Ft_Kind_CustomFirst,item_program.owner)));}
 return object;
}
void sleep_probe(HSD_GObj* object)
{
 Fighter* f=object->user_data;
 if(f->cmd_vars[0]==1){++load_callbacks;CHECK(make_item(object,NULL,owned_creation));
  if(load_action==1)HSD_GObjFree(object);
  if(load_action!=1){NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,0,&view));bool written=false;for(unsigned i=0;i<view.program->value_count;++i)if(view.program->values[i].owner==0&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,"divisor")){NativeScriptValue zero={.type=NativeScriptType_I32,.as.i64=load_action==2?0:1};CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&zero,&diagnostic));written=true;}CHECK(written);}
 }
 else if(f->cmd_vars[0]==2){++remove_callbacks;HSD_GObj* created=make_item(object,NULL,owned_creation);CHECK((created!=NULL)==!f->native_constructing);}
 else abort();
}
void constructor_item_probe(HSD_GObj* object,uint32_t semantic)
{
 if(semantic==NativeScriptBinding_PositionY){++interaction_callbacks;return;}
 ++destroy_callbacks;
 if(spawn_from_destroy){CHECK(!make_item(((Item*)object->user_data)->native_creator,object,true));}
 if(mutation){unsigned mode=mutation;mutation=0;
  if(mode==1)((Item*)probe_target->user_data)->owner=other_owner;
  else if(mode==2){HSD_GObjFree(probe_target);CHECK(make_item(other_owner,NULL,false)==probe_target);}
  else if(mode==3)((Fighter*)((Item*)object->user_data)->owner->user_data)->native_constructing=false;
  else abort();
 }
}
void HSD_ObjFree(HSD_ObjAllocData* descriptor,void* data)
{CHECK(descriptor==&item_alloc_data);NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&item_pool,data,&ref));CHECK(nativeMatchPool_Free(&item_pool,ref));++payload_frees;}
void nativeArticleResource_CancelGeneration(Item_GObj* object,uint32_t generation){CHECK(object&&((Item*)object->user_data)->x1C==(s32)generation);++leaves;}
void nativeArticleResource_CancelCreator(Fighter_GObj* object,uint32_t generation){CHECK(object&&((Fighter*)object->user_data)->x8_spawnNum==(s32)generation);++leaves;}
void nativeArticleResource_CancelConstructingAttachment(Fighter_GObj* object,NativeMatchPoolReference a,NativeMatchPoolReference b){CHECK(nativeMatchPool_Resolve(a)==object&&nativeMatchPool_Resolve(b)==object->user_data);++leaves;}
void nativeArticleModel_Destroy(Item_GObj* object,unsigned model){CHECK(object&&model>=1&&model<=3);++leaves;}
void Item_80267978(Item_GObj* object){CHECK(object);}
bool it_80272D1C(Item_GObj* object){CHECK(object);++destructions;return true;}
void it_802725D4(Item_GObj* object){CHECK(object);++leaves;}
static void Item_802675A8(Item_GObj* object){CHECK(object);++leaves;}
void efLib_DestroyAll(HSD_GObj* object){CHECK(object);++leaves;}
void efAsync_QueueClear(void* queue){CHECK(queue);++leaves;}
void Item_8026B0B4(Item_GObj* object){CHECK(object);++leaves;}
void it_8027327C(Item_GObj* object,int id,int sound){(void)object;(void)id;(void)sound;abort();}
bool ftLib_80086960(Fighter_GObj* object){CHECK(object);return true;}
Item_GObj* ftLib_800867CC(Fighter_GObj* object){CHECK(object);return ((Fighter*)object->user_data)->x1984_heldItemSpec;}
void ftLib_80086764(Fighter_GObj* object){CHECK(object);((Fighter*)object->user_data)->x1984_heldItemSpec=NULL;}
bool ftLib_800867A0(Fighter_GObj* object,Item_GObj* item){CHECK(object);return ((Fighter*)object->user_data)->item_gobj==item;}
void ftLib_80086724(Fighter_GObj* object,Item_GObj* item){CHECK(object&&((Fighter*)object->user_data)->item_gobj==item);((Fighter*)object->user_data)->item_gobj=NULL;}
void ftLib_80087050(s32 lock){(void)lock;++leaves;}
void Camera_800290D4(CmSubject* camera){CHECK(camera);++leaves;}
void lb_8000FD18(DynamicsDesc* descriptor){CHECK(descriptor);++leaves;}
void ftColl_8007B8E8(Fighter_GObj* object){CHECK(object);++leaves;}
void nativeFighterAttachment_Clear(Fighter_GObj* object){CHECK(object);++leaves;}
void ftCo_UnloadDynamicBones(Fighter* f){CHECK(f);++leaves;}
void ftColl_800765AC(Fighter_GObj* object){CHECK(object);++leaves;}
void ft_80088C5C(Fighter_GObj* object){CHECK(object);++leaves;}
void lbShadow_8000EE8C(LbShadow* shadow){CHECK(shadow);++leaves;}
void HSD_JObjUnref(HSD_JObj* joint){CHECK(!joint);++leaves;}
void ftData_800859A8(Fighter* f){CHECK(f);++leaves;}
void Player_80031FB0(int player,s32 alternate,Fighter_GObj* object){(void)player;(void)alternate;CHECK(object);++leaves;}
void it_8026BB20(Item_GObj* object){(void)object;abort();}
void it_8026BB44(Item_GObj* object){(void)object;abort();}
void it_8026B724(Item_GObj* object){(void)object;abort();}
void it_8026B73C(Item_GObj* object){(void)object;abort();}
void ftCommon_8007E6DC(Fighter_GObj* owner,Item_GObj* object,s32 normal){(void)owner;(void)object;(void)normal;abort();}
Item_GObj* Item_CreateEngine(SpawnItem* spawn){(void)spawn;abort();}
void Item_8026AB54(Item_GObj* object,HSD_GObj* owner,Fighter_Part part){(void)object;(void)owner;(void)part;abort();}
void db_80225DD8(Item_GObj* object,Fighter_GObj* owner){(void)object;(void)owner;abort();}
void it_80274594(Item_GObj* object){(void)object;abort();}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#undef bank
#include "teardown.inc"
#define bank sleep_package.compiler.bank
#pragma clang diagnostic pop
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t node,NativeScriptDiagnostic* d)
{
 (void)unused;const NativeScriptSourceNode* n=&tree->nodes[node];
 if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 1");
 if(n->kind==NativeScriptSource_ArticleState)return nativeScriptSource_SpanEquals(tree,n->value,"0 anim -1");
 if(n->kind==NativeScriptSource_Callback){NativeScriptEventResolution event;return nativeScriptEvents_Resolve(tree,n,&event,d);}
 return nativeScriptCompile_MinimalMetadata(NULL,tree,node,d);
}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},.declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate int divisor\naction Lifecycle:\n    on load:\n        command(0) = 1\n        let int result = 1 / state.divisor\n    on user_data_remove:\n        command(0) = 2\narticle It_Kind_Mario_Fire states 1:\n    on destroyed:\n        position.x = 3\n    on interaction:\n        position.y = 4\n    state 0 anim -1:\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 article=(Article){.x0_common_attr=&common_attributes,.x10_modelDesc=&model_descriptor};
 NativeScriptItemAsset asset={.declaration="It_Kind_Mario_Fire",.kind=It_Kind_Mario_Fire,.article=&article,.owner_node=UINT32_MAX};
 NativeScriptItemMetadataContext item_metadata={.assets=&asset,.asset_count=1};
 CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,&item_metadata,&sleep_package.program,&diagnostic));
 sleep_package.assets.character.data.x0=&sleep_package.assets.character.common_attributes;sleep_package.address=&sleep_package;sleep_package.ready=sleep_package.assets.ready=true;sleep_package.target=Ft_Kind_CustomFirst;sleep_package.assets.motions=motions;sleep_package.assets.facts.moves=layout;
 gFtDataList[Ft_Kind_CustomFirst]=&sleep_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&sleep_package.program);
 unsigned owner=UINT32_MAX;for(unsigned i=0;i<bank.tree.count;++i)if(bank.tree.nodes[i].kind==NativeScriptSource_Article)owner=i;CHECK(owner!=UINT32_MAX);
 item_program=*nativeScriptItem_Program(Ft_Kind_CustomFirst,owner);
 CHECK(nativeScriptItem_ProgramValid(nativeScriptItem_Program(Ft_Kind_CustomFirst,item_program.owner)));logic.destroyed=foreign_destroyed;
}
static void clean_items(void)
{spawn_from_destroy=false;mutation=0;while(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM])Item_8026A8EC(HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();CHECK(nativeMatchPool_Bind(&item_pool,item_data,sizeof(item_data),item_slots,item_free,ITEMS,sizeof(Item),_Alignof(Item),nativeGObjPool.lifetime));
 for(unsigned owned=0;owned<2;++owned)for(unsigned failed=0;failed<2;++failed){++cases;owned_creation=owned;HSD_GObj* owner=make_fighter(true,failed);Fighter* f=owner->user_data;HSD_GObj* load=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];CHECK(load&&((Item*)load->user_data)->native_creator_generation==0);
  f->x8_spawnNum=123;nativeScriptFighter_NewLife(owner,123);CHECK(!nativeItem_Creator(load));HSD_GObj* current=make_item(owner,NULL,owned);CHECK(current);other_owner=make_fighter(false,false);
  HSD_GObj* transferred=make_item(owner,NULL,owned);((Item*)transferred->user_data)->owner=other_owner;
  HSD_GObj* unowned=make_item(owner,NULL,owned);((Item*)unowned->user_data)->owner=NULL;
  HSD_GObj* borrowed=make_item(other_owner,NULL,false);((Item*)borrowed->user_data)->owner=owner;
  NativeMatchPoolReference load_ref,current_ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,load,&load_ref)&&nativeMatchPool_Reference(&nativeGObjPool,current,&current_ref));
  unsigned before=refusals;spawn_from_destroy=owned&&failed;HSD_GObjFree(owner);
  CHECK((nativeMatchPool_Resolve(load_ref)==NULL)==(bool)failed);CHECK((nativeMatchPool_Resolve(current_ref)==NULL)==(bool)failed);
  CHECK(refusals-before==(failed?(owned?3u:1u):0u));CHECK(((Item*)transferred->user_data)->owner==other_owner&&((Item*)unowned->user_data)->owner==NULL&&((Item*)borrowed->user_data)->native_creator==other_owner);
  clean_items();HSD_GObjFree(other_owner);
 }
 // Old surviving raw pointers cannot identify a new allocation at the same address.
 ++cases;owned_creation=false;HSD_GObj* old=make_fighter(false,false);HSD_GObj* survivor=make_item(old,NULL,false);HSD_GObjFree(old);HSD_GObj* reused=make_fighter(false,true);CHECK(reused==old&&!nativeItem_Creator(survivor));NativeMatchPoolReference survivor_ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,survivor,&survivor_ref));HSD_GObjFree(reused);CHECK(nativeMatchPool_Resolve(survivor_ref)==survivor);clean_items();
 // Callback transfer and physical child reuse are rechecked before the next deletion.
 for(unsigned mode=1;mode<=2;++mode){++cases;owned_creation=false;HSD_GObj* owner=make_fighter(false,true);other_owner=make_fighter(false,false);HSD_GObj* first=make_item(owner,NULL,true);probe_target=make_item(owner,NULL,true);CHECK(first);mutation=mode;NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,probe_target,&ref));HSD_GObjFree(owner);CHECK(!mutation);CHECK(mode==1?nativeMatchPool_Resolve(ref)==probe_target:nativeMatchPool_Resolve(ref)==NULL);clean_items();HSD_GObjFree(other_owner);}
 // Source Item children inherit the same physical creator stamp; no creator
 // stamp cannot accidentally grant ownership merely through Item.owner.
 ++cases;HSD_GObj* owner=make_fighter(false,true);HSD_GObj* parent=make_item(owner,NULL,true);HSD_GObj* child=make_item(owner,parent,true);CHECK(child);
 Item* parent_data=parent->user_data;Item* child_data=child->user_data;CHECK(child_data->native_creator==owner&&child_data->native_creator_reference.generation==parent_data->native_creator_reference.generation&&child_data->native_creator_reference.slot==parent_data->native_creator_reference.slot);
 HSD_GObj* null_creator=make_item(NULL,NULL,false);CHECK(null_creator);((Item*)null_creator->user_data)->owner=owner;CHECK(!((Item*)null_creator->user_data)->native_creator&& !((Item*)null_creator->user_data)->native_creator_reference.pool);
 NativeMatchPoolReference child_ref,null_ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,child,&child_ref)&&nativeMatchPool_Reference(&nativeGObjPool,null_creator,&null_ref));HSD_GObjFree(owner);CHECK(!nativeMatchPool_Resolve(child_ref)&&nativeMatchPool_Resolve(null_ref)==null_creator);clean_items();
 // Load can destroy its in-progress owner before Create reaches its failure
 // label, or fault after publishing its first child. Neither loses cleanup.
 for(unsigned mode=1;mode<=2;++mode){++cases;owned_creation=true;load_action=mode;unsigned before=payload_frees;HSD_GObj* failed=make_fighter(true,true);load_action=0;
  if(mode==2){CHECK(nativeScriptFighter_Status(failed)==NativeScriptVM_Error);diagnostic=(NativeScriptDiagnostic){0};HSD_GObjFree(failed);}
  CHECK(payload_frees>before&&!nativeFighterPool.used&&!HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]);
 }
 CHECK(nativeScriptRuntime_Idle()&&!nativeGObjPool.used&&!nativeFighterPool.used&&!item_pool.used);
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"load_callbacks\":%u,\"remove_callbacks\":%u,\"destroy_callbacks\":%u,\"interactions\":%u,\"creation_refusals\":%u,\"item_teardowns\":%u,\"payload_frees\":%u}\n",checks,cases,load_callbacks,remove_callbacks,destroy_callbacks,interaction_callbacks,refusals,destructions,payload_frees);return 0;
}
