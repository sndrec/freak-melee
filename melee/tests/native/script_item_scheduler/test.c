#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <melee/it/item.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/kinds/types.h>
#include <melee/it/it_26B1.h>
#include <melee/ft/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
static unsigned checks,cases;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"scheduler check %u: %s\n",__LINE__,#v);exit(1);}}while(0)
enum{N=2048,F=64,V=128};
static char source_bytes[65536];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptItemProgram program;static NativeScriptItemState states[2];static Article assets;static ItemModelDesc model;static ItemAttr attributes;
static unsigned next_item;static bool deferred_seen;
static unsigned removed_tail;static bool native_deferred_stopped,hitlag_deferred_stopped;
static HSD_GObj* replacement;
static ItemLogicTable unowned_logic;
static void native_accessory(HSD_GObj* object){(void)object;++scheduler_native;}
static void fighter_userdata_remove(void* data){(void)data;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner){return program.bank&&author==program.author_kind&&owner==program.owner?&program:NULL;}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
 (void)unused;const NativeScriptSourceNode* n=&tree->nodes[at];if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 2");if(n->kind==NativeScriptSource_ArticleState)return true;
 if(n->kind==NativeScriptSource_State||n->kind==NativeScriptSource_Function)return n->parent<tree->count&&tree->nodes[n->parent].kind==NativeScriptSource_Article;
 if(n->kind==NativeScriptSource_Callback){NativeScriptEventResolution e;return nativeScriptEvents_Resolve(tree,n,&e,d);}return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);
}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* text="fighter:\n    behavior independent\narticle It_Kind_Mario_Fire states 2:\n    state int count\n    state bool marked\n    func Extra():\n        item.state.count = item.state.count + 3\n    on spawned:\n            item.state.count = 1\n            position.x = 1\n    on destroyed:\n            item.state.count = item.state.count + 100\n            item.state.marked = true\n    on picked_up:\n            Extra()\n    on dropped:\n            Extra()\n    on thrown:\n            Extra()\n    on entered_air:\n            Extra()\n    on accessory:\n            Extra()\n    on entered_hitlag:\n            Extra()\n    on exited_hitlag:\n            Extra()\n    on damage_received:\n            Extra()\n            return false\n    on reflected:\n            Extra()\n            return false\n    on clanked:\n            return false\n    on shield_bounced:\n            return false\n    on hit_shield:\n            return false\n    on absorbed:\n            return true\n    on damage_dealt:\n            return true\n    on touched:\n            Extra()\n            return false\n    on jumped_on:\n            Extra()\n            return false\n    on grab_dealt:\n            Extra()\n            position.x = item.state.count\n    on grabbed_for_victim:\n            if victim != None: Extra()\n            position.x = item.state.count\n    on interaction:\n            if other != None: Extra()\n            position.x = item.state.count\n    on out_of_bounds:\n            Extra()\n    state 0 anim -1:\n        on animation:\n            item.state.count = item.state.count + 1\n            position.x = item.state.count\n            return false\n        on physics:\n            Extra()\n        on collision:\n            position.x = item.state.count\n            return true\n    state 1 anim -1:\n        on animation:\n            return false\n";
 NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=metadata};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)){fprintf(stderr,"compile %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 assets.x0_common_attr=&attributes;assets.x10_modelDesc=&model;
 program=(NativeScriptItemProgram){.bank=&bank,.identity=88,.author_kind=Ft_Kind_Mario,.kind=It_Kind_Mario_Fire,.recipe_index=UINT32_MAX,.article=&assets,.states=states,.state_count=2};
 for(unsigned i=0;i<NativeScriptItemHook_Count;++i)program.hooks[i]=UINT32_MAX;for(unsigned s=0;s<2;++s){states[s].animation_id=-1;for(unsigned p=0;p<3;++p)states[s].functions[p]=UINT32_MAX;}
 for(unsigned n=0;n<bank.tree.count;++n)if(bank.tree.nodes[n].kind==NativeScriptSource_Article)program.owner=n;
 for(unsigned f=0;f<bank.declarations.function_count;++f){NativeScriptFunctionDeclaration* fn=&bank.declarations.functions[f];if(!fn->callback)continue;NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&bank.tree,&bank.tree.nodes[fn->source_node],&e,&d));const char* name=e.descriptor->name;
  if(e.descriptor->kind==NativeScriptEvent_ArticleState){unsigned phase=!strcmp(name,"animation")?0:!strcmp(name,"physics")?1:2;states[e.motion].functions[phase]=f;continue;}
  static const struct{const char* name;unsigned hook;} map[]={ {"spawned",NativeScriptItemHook_Spawned},{"destroyed",NativeScriptItemHook_Destroyed},{"picked_up",NativeScriptItemHook_PickedUp},{"dropped",NativeScriptItemHook_Dropped},{"thrown",NativeScriptItemHook_Thrown},{"entered_air",NativeScriptItemHook_EnteredAir},{"accessory",NativeScriptItemHook_Accessory},{"entered_hitlag",NativeScriptItemHook_EnteredHitlag},{"exited_hitlag",NativeScriptItemHook_ExitedHitlag},{"damage_received",NativeScriptItemHook_DamageReceived},{"damage_dealt",NativeScriptItemHook_DamageDealt},{"reflected",NativeScriptItemHook_Reflected},{"clanked",NativeScriptItemHook_Clanked},{"shield_bounced",NativeScriptItemHook_ShieldBounced},{"hit_shield",NativeScriptItemHook_HitShield},{"absorbed",NativeScriptItemHook_Absorbed},{"interaction",NativeScriptItemHook_Interaction},{"out_of_bounds",NativeScriptItemHook_OutOfBounds},{"touched",NativeScriptItemHook_Touched},{"jumped_on",NativeScriptItemHook_JumpedOn},{"grab_dealt",NativeScriptItemHook_GrabDealt},{"grabbed_for_victim",NativeScriptItemHook_GrabbedForVictim}};
  for(unsigned i=0;i<sizeof(map)/sizeof(map[0]);++i)if(!strcmp(name,map[i].name))program.hooks[map[i].hook]=f;
 }
 CHECK(nativeScriptItem_ProgramValid(&program));scheduler_spec=&program;
}
static NativeScriptValue state(HSD_GObj* object,const char* name)
{
 for(unsigned a=0;a<NATIVE_SCRIPT_ITEM_ACTORS;++a)if(nativeScriptItemActors[a].active&&nativeScriptItemActors[a].object==object){NativeScriptItemActor* actor=&nativeScriptItemActors[a];for(unsigned f=0;f<bank.declarations.value_count;++f)if(values[f].owner==program.owner&&nativeScriptSource_SpanEquals(&bank.tree,values[f].name,name)){NativeScriptValue result;NativeScriptDiagnostic d;CHECK(nativeScriptStorage_Read(&actor->storage,NATIVE_SCRIPT_STATE_SEMANTIC|f,NULL,0,&result,&d));return result;}}
 CHECK(false);return(NativeScriptValue){0};
}
static HSD_GObj* create(void)
{CHECK(next_item<32);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);NativeMatchPoolReference item_ref;Item* item=nativeMatchPool_Allocate(&nativeItemPool,&item_ref);CHECK(item);++next_item;item->entity=object;item->kind=program.kind;item->x1C=next_item;item->msid=-1;item->native_article_author_kind=program.author_kind;item->native_script_owner=program.owner;item->xC4_article_data=&assets;item->xCC_item_attr=&attributes;item->xBBC_dynamicBoneTable=NULL;item->ground_or_air=GA_Air;GObj_InitUserData(object,6,scheduler_userdata_remove,item);static HSD_JObj joints[32];object->hsd_obj=&joints[next_item-1];CHECK(nativeScriptItem_Attach(object,&program));CHECK(scheduler_spawned(object));scheduler_state(object,0);return object;}
static void deferred_proc(HSD_GObj* object)
{scheduler_remove=true;scheduler_animation(object);deferred_seen=nativeScriptItem_IsAttached(object)&&((Item*)object->user_data)->native_removal_in_progress;}
static bool native_remove_animation(HSD_GObj* object)
{Item_8026A8EC(object);removed_tail=scheduler_tail;return false;}
static void native_remove_hitlag(HSD_GObj* object)
{Item_8026A8EC(object);}
static void native_deferred_proc(HSD_GObj* object)
{((Item*)object->user_data)->animated=native_remove_animation;scheduler_animation(object);native_deferred_stopped=scheduler_tail==removed_tail;}
static void hitlag_deferred_proc(HSD_GObj* object)
{Item* item=object->user_data;item->entered_hitlag=native_remove_hitlag;item->xDC8_word.flags.x4=1;scheduler_damage_pipeline(object);hitlag_deferred_stopped=item->xDC8_word.flags.x4&&!item->xDC8_word.flags.x8;}
static HSD_GObj* replace_on_write(void)
{replacement=create();return replacement;}
#include "capture_inert_cases.h"
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);CHECK(nativeScriptItemSceneEpoch==nativeGObjPool.lifetime);scheduler_reset();
 HSD_GObj* a=create();CHECK(state(a,"count").as.i64==1);scheduler_animation(a);CHECK(state(a,"count").as.i64==2&&((Item*)a->user_data)->pos.x==2);++cases;
 HSD_GObj* moving=create();Item* moving_item=moving->user_data;moving_item->x40_vel=(Vec3){2,3,0};scheduler_physics(moving);CHECK(state(moving,"count").as.i64==4&&moving_item->pos.x==3&&moving_item->pos.y==3);moving_item->xDC8_word.flags.x9=1;scheduler_physics(moving);CHECK(state(moving,"count").as.i64==4&&moving_item->pos.x==3&&moving_item->pos.y==3);Item_8026A8EC(moving);++cases;
 HSD_GObj* phase=create();scheduler_phase_transition=true;unsigned tail_before=scheduler_tail;scheduler_animation(phase);CHECK(((Item*)phase->user_data)->msid==1&&state(phase,"count").as.i64==2&&scheduler_tail>tail_before);Item_8026A8EC(phase);++cases;
 uint32_t fn=nativeScriptItem_CallbackFunction(a,NativeScriptItemCallback_Accessory);CHECK(nativeScriptItem_BindCallback(a,NativeScriptItemCallback_Accessory,fn));scheduler_accessory(a);CHECK(state(a,"count").as.i64==5);scheduler_nested=true;scheduler_state(a,0);CHECK(((Item*)a->user_data)->msid==1&&nativeScriptItem_Revision(a)>=3&&!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));++cases;
 CHECK(nativeScriptItem_BindCallback(a,NativeScriptItemCallback_Accessory,fn));((Item*)a->user_data)->on_accessory=native_accessory;scheduler_accessory(a);CHECK(scheduler_native==1&&state(a,"count").as.i64==5&&!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));((Item*)a->user_data)->on_accessory=NULL;++cases;
 scheduler_damage(a,NativeScriptItemHook_DamageReceived);CHECK(state(a,"count").as.i64==8);++cases;
 Vec3 zero={0};Item_8026AB54(a,NULL,FtPart_TopN);Item_8026ABD8(a,&zero,1);Item_8026AC74(a,&zero,&zero,1);Item_8026AD20(a,&zero,&zero,1,false);Item_8026ADC0(a);CHECK(state(a,"count").as.i64==23&&((Item*)a->user_data)->xDC8_word.flags.x1F);++cases;
 fn=nativeScriptItem_CallbackFunction(a,NativeScriptItemCallback_EnteredHitlag);CHECK(nativeScriptItem_BindCallback(a,NativeScriptItemCallback_EnteredHitlag,fn));fn=nativeScriptItem_CallbackFunction(a,NativeScriptItemCallback_ExitedHitlag);CHECK(nativeScriptItem_BindCallback(a,NativeScriptItemCallback_ExitedHitlag,fn));scheduler_hitlag(a,true);CHECK(((Item*)a->user_data)->xDC8_word.flags.x9&&state(a,"count").as.i64==26);scheduler_hitlag(a,false);CHECK(!((Item*)a->user_data)->xDC8_word.flags.x9&&state(a,"count").as.i64==29);++cases;
 scheduler_damage(a,NativeScriptItemHook_Reflected);CHECK(state(a,"count").as.i64==32);scheduler_damage(a,NativeScriptItemHook_Clanked);scheduler_damage(a,NativeScriptItemHook_ShieldBounced);scheduler_damage(a,NativeScriptItemHook_HitShield);CHECK(nativeScriptItem_IsAttached(a));++cases;
 ((Item*)a->user_data)->x378_itemColl.x34_flags.b7=true;it_8026C368(a);CHECK(state(a,"count").as.i64==35);((Item*)a->user_data)->x378_itemColl.x34_flags.b7=false;++cases;
 HSD_GObj* target=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);static Fighter fighter;fighter.gobj=target;fighter.kind=Ft_Kind_Mario;fighter.x8_spawnNum=900;GObj_InitUserData(target,4,fighter_userdata_remove,&fighter);it_8026B7F8(target);CHECK(state(a,"count").as.i64==38);++cases;
 HSD_GObj* member=create();NativeMatchPoolReference member_reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,member,&member_reference));scheduler_remove_other=member;scheduler_spawn_on_write=replace_on_write;it_8026B7F8(target);CHECK(nativeMatchPool_Resolve(member_reference)==NULL&&replacement&&state(replacement,"count").as.i64==1&&state(a,"count").as.i64==41);Item_8026A8EC(replacement);++cases;
 NativeMatchPoolReference target_ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,target,&target_ref));scheduler_retire_target=target;it_8026B7F8(target);CHECK(nativeMatchPool_Resolve(target_ref)==NULL&&state(a,"count").as.i64==44);++cases;
 target=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);static Fighter grab_target;grab_target.gobj=target;grab_target.kind=Ft_Kind_Mario;grab_target.x8_spawnNum=901;GObj_InitUserData(target,4,fighter_userdata_remove,&grab_target);
 member=create();((Item*)member->user_data)->owner=target;((Item*)member->user_data)->xDC8_word.flags.x13=true;CHECK(nativeMatchPool_Reference(&nativeGObjPool,member,&member_reference));scheduler_clear_owner=member;it_8026B7F8(target);CHECK(nativeMatchPool_Resolve(member_reference)==NULL&&scheduler_destroyed==104);++cases;
 HSD_GObj* grabbing=create();Item* grab_item=grabbing->user_data;grab_item->grab_victim=target;grab_item->xDD0_flag.b5=true;
 fn=nativeScriptItem_CallbackFunction(grabbing,NativeScriptItemCallback_GrabDealt);CHECK(nativeScriptItem_BindCallback(grabbing,NativeScriptItemCallback_GrabDealt,fn));fn=nativeScriptItem_CallbackFunction(grabbing,NativeScriptItemCallback_GrabbedForVictim);CHECK(nativeScriptItem_BindCallback(grabbing,NativeScriptItemCallback_GrabbedForVictim,fn));scheduler_grab(grabbing);CHECK(state(grabbing,"count").as.i64==7&&grab_item->pos.x==7);++cases;
 CHECK(nativeMatchPool_Reference(&nativeGObjPool,target,&target_ref));scheduler_retire_target=target;scheduler_grab(grabbing);CHECK(nativeMatchPool_Resolve(target_ref)==NULL&&state(grabbing,"count").as.i64==10&&grab_item->pos.x==10);++cases;
 fn=nativeScriptItem_CallbackFunction(grabbing,NativeScriptItemCallback_Touched);CHECK(nativeScriptItem_BindCallback(grabbing,NativeScriptItemCallback_Touched,fn));grab_item->xDCE_flag.b6=true;scheduler_damage_pipeline(grabbing);CHECK(state(grabbing,"count").as.i64==13);grab_item->xDCE_flag.b6=false;
 fn=nativeScriptItem_CallbackFunction(grabbing,NativeScriptItemCallback_JumpedOn);CHECK(nativeScriptItem_BindCallback(grabbing,NativeScriptItemCallback_JumpedOn,fn));grab_item->xCFC=grabbing;scheduler_jumped(grabbing);CHECK(state(grabbing,"count").as.i64==16);Item_8026A8EC(grabbing);++cases;
 HSD_GObj* b=create();NativeMatchPoolReference removed;CHECK(nativeMatchPool_Reference(&nativeGObjPool,b,&removed));scheduler_damage(b,NativeScriptItemHook_DamageDealt);CHECK(nativeMatchPool_Resolve(removed)==NULL&&!nativeScriptItem_IsAttached(b)&&scheduler_destroyed==101);++cases;
 b=create();CHECK(nativeMatchPool_Reference(&nativeGObjPool,b,&removed));scheduler_phase_transition=true;scheduler_collision(b);CHECK(nativeMatchPool_Resolve(removed)==NULL&&scheduler_destroyed==101);++cases;
 b=create();CHECK(nativeMatchPool_Reference(&nativeGObjPool,b,&removed));scheduler_damage(b,NativeScriptItemHook_Absorbed);CHECK(nativeMatchPool_Resolve(removed)==NULL&&scheduler_destroyed==101);++cases;
 HSD_GObj* c=create();CHECK(nativeMatchPool_Reference(&nativeGObjPool,c,&removed));HSD_GObj_SetupProc(c,deferred_proc,0);HSD_GObj_80390CFC();CHECK(deferred_seen&&nativeMatchPool_Resolve(removed)==NULL&&!nativeScriptItem_IsAttached(c)&&nativeScriptRuntime_Idle());++cases;
 c=create();nativeScriptItem_Detach(c);((Item*)c->user_data)->xB8_itemLogicTable=&unowned_logic;CHECK(nativeMatchPool_Reference(&nativeGObjPool,c,&removed));HSD_GObj_SetupProc(c,native_deferred_proc,0);HSD_GObj_80390CFC();CHECK(native_deferred_stopped&&nativeMatchPool_Resolve(removed)==NULL);++cases;
 c=create();CHECK(nativeMatchPool_Reference(&nativeGObjPool,c,&removed));HSD_GObj_SetupProc(c,hitlag_deferred_proc,0);HSD_GObj_80390CFC();CHECK(hitlag_deferred_stopped&&nativeMatchPool_Resolve(removed)==NULL);++cases;
 SpawnItem spawn={0};spawn.kind=program.kind;unsigned before=nativeItemPool.used;scheduler_fail_selector=true;CHECK(scheduler_construct(&spawn)==NULL&&nativeItemPool.used==before&&nativeMatchPool_Validate(&nativeItemPool)&&scheduler_models==0);scheduler_fail_selector=false;++cases;
 HSD_GObj* made=scheduler_construct(&spawn);CHECK(made&&nativeScriptItem_IsAttached(made)&&scheduler_models==1&&state(made,"count").as.i64==1);Item_8026A8EC(made);++cases;
 before=nativeItemPool.used;scheduler_fail_write=true;CHECK(scheduler_construct(&spawn)==NULL&&nativeItemPool.used==before&&nativeMatchPool_Validate(&nativeItemPool));scheduler_fail_write=false;++cases;
 Item_8026A8EC(a);CHECK(!nativeScriptItem_IsAttached(a));
 capture_inert_cases();
 // Transition-only fixture publication with no remaining pins: no Destroyed
 // body must still retire the owned view through the actual removal path.
 CHECK(!nativeScriptItem_ReferencesProgram(&bank));program.hooks[NativeScriptItemHook_Destroyed]=UINT32_MAX;
 HSD_GObj* missing=create();CHECK(nativeMatchPool_Reference(&nativeGObjPool,missing,&removed));deferred_seen=false;HSD_GObj_SetupProc(missing,deferred_proc,0);HSD_GObj_80390CFC();CHECK(deferred_seen&&nativeMatchPool_Resolve(removed)==NULL&&scheduler_destroyed==2);++cases;
 CHECK(scheduler_stock_assets==0); // Explicit source assets never touch a donor bank.
 printf("{\"status\":\"PASS actual Item scheduler/VM lifecycle\",\"checks\":%u,\"cases\":%u}\n",checks,cases);return 0;
}
