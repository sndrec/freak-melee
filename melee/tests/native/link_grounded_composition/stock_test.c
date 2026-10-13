// Full generated cleanup/article callbacks versus unmodified stock cleanup bodies.
// HSD allocation, engine scene housekeeping and immutable package lookup are
// controlled; resource ownership, typed refs, signals, events, joints and life
// checks are production bodies. This is source-only, not a gameplay fixture.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_events.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/fighter.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftYoshi/types.h>
#include <melee/ft/kinds/ftYoshi/ftyoshi.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/ft/kinds/ftLink/ftlink.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialn.h>
#include <melee/ft/kinds/ftLink/ftlinkspecials.h>
#include <melee/ft/kinds/ftCLink/ftclink.h>
#include <melee/ft/kinds/ftCommon/ftCo_AirCatch.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/native_article.h>
#include <melee/it/native_article_refs.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_article_model.h>
#include <melee/it/kinds/itlinkarrow.h>
#include <melee/it/kinds/itlinkbow.h>
#include <melee/it/kinds/itlinkboomerang.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/it/kinds/itclinkmilk.h>
#include <melee/lb/lb_00B0.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/ef/eflib.h>
#include <melee/cm/camera.h>
#include <melee/lb/lb_00F9.h>
#include <melee/it/it_2725.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/mtx.h>
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grounded.generated.c"

enum { Boom, Hook, Arrow, Bow, Milk, Articles=5, Pool=180 };
enum { Removed=1, TrailFreed, NodeFreed };
static Fighter fighters[2], recipients[2];
static HSD_GObj creators[2], recipient_objects[2], article_objects[2][Articles];
static Item article_items[2][Articles];
static bool article_live[2][Articles];
static HSD_GObj nodes[Pool];static ItemLink links[Pool];
static HSD_JObj models[Pool], trails[2][2][2], roots[2][Articles], common_joints[2], attachment_joints[2], replacement;
static bool node_live[Pool], link_live[Pool], model_live[Pool], trail_live[2][2][2];
static Article articles[Articles];static HSD_Joint model_descriptor;
static ItemLogicTable stock_logics[Articles];
static FighterBone parts[2][MAX_FT_PARTS];static ftData fdata[2];static ftData_x8 common_data[2];
static FighterPartsTable part_table;static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=part_tables;
HSD_ObjAllocData item_link_alloc_data;u8 HSD_GObj_JObjKind;
static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static const NativeFighterDefinition *definition,*recipient_definition;
static NativeFighterDefinition foreign_definition,missing_definition;
static unsigned checks,cases,safety_cases,scenario,current,removed[2],freed[2];
static unsigned grounded_cases,timelines,ground_events;
static void ground_matrix(void);
static void init_ground_host(NativeFighterHost* host);
static FighterKind family;
static NativeFighterParameters ground_parameters;
typedef struct CleanupEvent {unsigned type,item,index;} CleanupEvent;
static CleanupEvent trace[2][100];static unsigned trace_count[2];
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"scenario%u side%u line%u: %s\n",scenario,current,__LINE__,#x);abort();}}while(0)
static void event(unsigned type,unsigned item,unsigned index)
{CHECK(trace_count[current]<100);trace[current][trace_count[current]++]=(CleanupEvent){type,item,index};}
static void boundary_failure(unsigned line){fprintf(stderr,"uncontrolled boundary scenario%u side%u line%u\n",scenario,current,line);exit(125);}
#define abort() boundary_failure(__LINE__)
static void unused_host(void){fprintf(stderr,"unexpected Host scenario%u\n",scenario);abort();}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{if(kind==Ft_Kind_CustomFirst)return definition;if(kind==Ft_Kind_CustomFirst+1)return recipient_definition;if(kind==Ft_Kind_CustomFirst+2)return &foreign_definition;if(kind==Ft_Kind_CustomFirst+3)return &missing_definition;return NULL;}
HSD_GObj* ftLib_8008741C(u32 id)
{for(HSD_GObj* g=heads[HSD_GOBJ_PLINK_FIGHTER];g;g=g->next){Fighter* f=g->user_data;if(f && (u32)f->x8_spawnNum==id)return g;}return NULL;}
u32 ftLib_80087460(HSD_GObj* g){Fighter* f=g?g->user_data:NULL;return f?(u32)f->x8_spawnNum:0;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t id){(void)g;(void)id;abort();}
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* id){(void)g;(void)id;abort();}
#include "dispatch_originals.h"
static unsigned article_index(HSD_GObj* g,unsigned* side)
{for(unsigned s=0;s<2;++s)for(unsigned i=0;i<Articles;++i)if(g==&article_objects[s][i]){*side=s;return i;}CHECK(false);return 0;}
static void unlink_item(HSD_GObj* g)
{HSD_GObj** p=&heads[HSD_GOBJ_PLINK_ITEM];while(*p && *p!=g)p=&(*p)->next;CHECK(*p==g);*p=g->next;}
// Scene housekeeping boundaries. The production removal body below retains
// actual in-progress guard, callback dispatch, generation guards and tail order.
bool it_80272D1C(Item_GObj* g){unsigned side,index=article_index(g,&side);return article_live[side][index];}
void it_802725D4(Item_GObj* g){unsigned side,index=article_index(g,&side);CHECK(side==current);event(Removed,index,0);++removed[side];}
static void Item_802675A8(HSD_GObj* g){(void)g;} // Global spawn counters.
static void Item_8026B0B4(HSD_GObj* g){(void)g;} // Sound channel cleanup.
void efLib_DestroyAll(HSD_GObj* g){(void)g;}
void efAsync_QueueClear(void* g){(void)g;}
void lb_8000FD18(DynamicsDesc* d){(void)d;abort();} // Dynamic bones disabled.
void Camera_800290D4(CmSubject* c){(void)c;abort();} // No camera borrower.
void it_8027327C(Item_GObj* g,enum_t a,enum_t b){(void)g;(void)a;(void)b;abort();}
void ftLib_80087050(s32 slot){(void)slot;abort();} // ECB locking disabled.
bool ftLib_80086960(HSD_GObj* g){(void)g;abort();} // Held-item owner bit disabled.
HSD_GObj* ftLib_800867CC(HSD_GObj* g){(void)g;abort();}
void ftLib_80086764(HSD_GObj* g){(void)g;abort();}
bool ftLib_800867A0(HSD_GObj* g,HSD_GObj* i){(void)g;(void)i;abort();}
void ftLib_80086724(HSD_GObj* g,HSD_GObj* i){(void)g;(void)i;abort();}
void OSReport(char* message,...){(void)message;abort();}
#include "item_removal_originals.h"
void ftKb_SpecialNLk800FB418(HSD_GObj* g){(void)g;abort();}
void ftKb_SpecialNLk800FB444(HSD_GObj* g){(void)g;abort();}
void ftCo_800D94D8(Fighter_GObj*);
#include "cleanup_originals.h"
static uint32_t current_owner(Item_GObj* g){Item* item=g->user_data;return ftLib_80087460(item->owner);}
static unsigned field_offset(const NativeFighterDefinition* d,const char* name)
{for(unsigned i=0;i<d->state_field_count;++i)if(!strcmp(name,d->state_fields[i].name))return d->state_fields[i].offset;CHECK(false);return 0;}
static void put(const NativeFighterDefinition* d,Fighter* f,const char* name,int32_t v){memcpy(f->native_module_state+field_offset(d,name),&v,4);}
static int32_t get(const NativeFighterDefinition* d,Fighter* f,const char* name){int32_t v;memcpy(&v,f->native_module_state+field_offset(d,name),4);return v;}
static void article_state(unsigned index,const char* name,int32_t value)
{const NativeFighterArticleLogic* logic=NULL;for(unsigned i=0;i<definition->article_logic_count;++i)if(definition->article_logics[i].kind==article_items[1][index].kind)logic=&definition->article_logics[i];CHECK(logic);for(unsigned i=0;i<logic->state_field_count;++i)if(!strcmp(name,logic->state_fields[i].name)){const NativeFighterStateField* f=&logic->state_fields[i];memcpy(article_items[1][index].xDD4_itemVar.native_article.state+f->offset,&value,f->stride);return;}CHECK(false);}
static HSD_GObj* alloc_node(void)
{for(unsigned i=0;i<Pool;++i)if(!node_live[i]){node_live[i]=true;memset(&nodes[i],0,sizeof(nodes[i]));return &nodes[i];}CHECK(false);return NULL;}
HSD_GObj* GObj_Create(u16 classifier,u8 link,u8 priority){CHECK(classifier==HSD_GOBJ_CLASS_ITEMLINK && link==10 && priority==0);return alloc_node();}
void* HSD_ObjAlloc(HSD_ObjAllocData* a){CHECK(a==&item_link_alloc_data);for(unsigned i=0;i<Pool;++i)if(!link_live[i]){link_live[i]=true;memset(&links[i],0xA5,sizeof(links[i]));return &links[i];}CHECK(false);return NULL;}
void HSD_ObjFree(HSD_ObjAllocData* a,void* p){CHECK(a==&item_link_alloc_data);ptrdiff_t i=(ItemLink*)p-links;CHECK(i>=0 && i<Pool && link_live[i]);link_live[i]=false;memset(p,0xDD,sizeof(ItemLink));}
void GObj_InitUserData(HSD_GObj* g,u8 k,void(*remove)(void*),void* data){CHECK(k==6);g->user_data=data;g->user_data_remove_func=remove;}
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* d){CHECK(d==&model_descriptor);for(unsigned i=0;i<Pool;++i)if(!model_live[i]){model_live[i]=true;memset(&models[i],0,sizeof(models[i]));models[i].scale=(Vec3){1,1,1};return &models[i];}CHECK(false);return NULL;}
void HSD_GObjObject_80390A70(HSD_GObj* g,u8 kind,void* model){CHECK(kind==HSD_GObj_JObjKind);g->hsd_obj=model;}
void GObj_SetupGXLink(HSD_GObj* g,GObj_RenderFunc draw,u8 link,u32 priority){CHECK(link==6 && priority==0 && draw);g->render_cb=draw;}
void HSD_GObj_JObjCallback(HSD_GObj* g,int pass){(void)g;(void)pass;abort();}
void HSD_GObjFree(HSD_GObj* g)
{
 for(unsigned side=0;side<2;++side)for(unsigned index=0;index<Articles;++index)
 if(g==&article_objects[side][index]){CHECK(side==current && article_live[side][index]);Item* item=g->user_data;nativeArticleResource_CancelGeneration(g,item->x1C);unlink_item(g);article_live[side][index]=false;memset(item,0xDD,sizeof(*item));g->user_data=NULL;return;}
 ptrdiff_t i=g-nodes;CHECK(i>=0 && i<Pool && node_live[i]);ItemLink* l=g->user_data;unsigned ordinal=l->x1CC;event(NodeFreed,Hook,ordinal);++freed[current];
 if(current){if(nativeCallbackSource_Alive(&creators[1],0x80001301u,NativeCallbackSource_Fighter) && fighters[1].kind==Ft_Kind_CustomFirst){CHECK(parts[1][139].joint!=g->hsd_obj);
  /* The capture anchor union has meaning only in common capture motions.
   * Refused grab input can retain an article while the fighter is in Wait. */
  FtMotionId motion=fighters[1].motion_id;
  if(motion==ftCo_MS_Catch||motion==ftCo_MS_CatchDash||motion==ftCo_MS_CatchPull||motion==ftCo_MS_CatchDashPull||motion==ftCo_MS_CatchWait||motion==ftCo_MS_CatchAttack)CHECK(fighters[1].mv.co.capturedamage.x18!=g->hsd_obj);for(unsigned h=0;h<4;++h)CHECK(fighters[1].x914[h].jobj!=g->hsd_obj);CHECK(fighters[1].x1064_thrownHitbox.jobj!=g->hsd_obj);}}
 if(g->hsd_obj){ptrdiff_t j=(HSD_JObj*)g->hsd_obj-models;CHECK(j>=0 && j<Pool && model_live[j]);model_live[j]=false;memset(&models[j],0xDD,sizeof(models[j]));}
 if(g->user_data_remove_func)g->user_data_remove_func(g->user_data);else{ptrdiff_t j=l-links;CHECK(j>=0 && j<Pool && link_live[j]);link_live[j]=false;memset(l,0xDD,sizeof(*l));}
 node_live[i]=false;memset(g,0xDD,sizeof(*g));}
void HSD_JObjRemoveAll(HSD_JObj* joint)
{for(unsigned s=0;s<2;++s)for(unsigned k=0;k<2;++k)for(unsigned i=0;i<2;++i)if(joint==&trails[s][k][i]){CHECK(s==current && trail_live[s][k][i]);event(TrailFreed,k?Arrow:Boom,i);trail_live[s][k][i]=false;memset(joint,0xDD,sizeof(*joint));return;}CHECK(false);}
void mpColl_80041EE4(CollData* c){Vec3 p=c->cur_pos;memset(c,0,sizeof(*c));c->cur_pos=c->prev_pos=c->last_pos=c->x28_vec=p;}
void mpColl_SetECBSource_Fixed(CollData* c,HSD_GObj* g,float a,float b,float d,float e){CHECK(!g && a>0 && a==b && b==d && d==e);(void)c;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* j){j->flags|=JOBJ_MTX_DIRTY;}
#undef HSD_JObjSetMtxDirty
void HSD_JObjSetMtxDirty(HSD_JObj* j){j->flags|=JOBJ_MTX_DIRTY;}
void HSD_JObjSetFlags(HSD_JObj* j,u32 flags){j->flags|=flags;}
void HSD_JObjClearFlags(HSD_JObj* j,u32 flags){j->flags&=~flags;}
void HSD_JObjSetFlagsAll(HSD_JObj* j,u32 flags){for(;j;j=j->next){HSD_JObjSetFlags(j,flags);HSD_JObjSetFlagsAll(j->child,flags);}}
void HSD_JObjClearFlagsAll(HSD_JObj* j,u32 flags){for(;j;j=j->next){HSD_JObjClearFlags(j,flags);HSD_JObjClearFlagsAll(j->child,flags);}}
void C_MTXCopy(Mtx a,Mtx b){memcpy(b,a,sizeof(Mtx));}
bool mpColl_800471F8(CollData* c){(void)c;abort();}
bool mpColl_8004B108(CollData* c){(void)c;abort();}
bool mpGetSpeed(int l,Vec3* p,Vec3* s){(void)l;(void)p;(void)s;abort();}
void C_MTXMultVec(Mtx a,Vec3* p,Vec3* out){(void)a;(void)p;(void)out;abort();}
void HSD_JObjSetupMatrixSub(HSD_JObj* j){j->flags&=~JOBJ_MTX_DIRTY;}
void HSD_JObjAddAnimAll(HSD_JObj* j,HSD_AnimJoint* a,HSD_MatAnimJoint* m,HSD_ShapeAnimJoint* s){(void)j;(void)a;(void)m;(void)s;abort();}
void HSD_JObjReqAnimAll(HSD_JObj* j,float f){(void)j;(void)f;abort();}
void HSD_JObjAnimAll(HSD_JObj* j){(void)j;abort();}
void HSD_JObjRemoveAnimAll(HSD_JObj* j){(void)j;abort();}
void lb_8000BA0C(HSD_JObj* j,float f){(void)j;(void)f;abort();}
void HSD_MtxGetRotation(Mtx m,Vec3* v){(void)m;(void)v;abort();}
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* g,...){(void)id;(void)g;abort();}
void __assert(char* file,u32 line,char* expr){fprintf(stderr,"%s:%u %s\n",file,(unsigned)line,expr);abort();}
static void no_leaks(void)
{for(unsigned i=0;i<Pool;++i)CHECK(!node_live[i] && !link_live[i] && !model_live[i]);for(unsigned s=0;s<2;++s)for(unsigned k=0;k<2;++k)for(unsigned i=0;i<2;++i)CHECK(!trail_live[s][k][i]);}
static const NativeFighterArticleLogic* logic(ItemKind kind)
{for(unsigned i=0;i<definition->article_logic_count;++i)if(definition->article_logics[i].kind==kind)return &definition->article_logics[i];CHECK(false);return NULL;}
static void construct_graph(unsigned count)
{
    current=0;ItemLink* prev=NULL;
    for(unsigned i=0;i<count;++i){HSD_GObj* g=alloc_node();ItemLink* l=HSD_ObjAlloc(&item_link_alloc_data);memset(l,0,sizeof(*l));g->user_data=l;l->gobj=g;l->next=prev;l->x1CC=i;g->hsd_obj=HSD_JObjLoadJoint(&model_descriptor);prev=l;}
    article_items[0][Hook].xDD4_itemVar.linkhookshot.x0=prev;
    article_items[0][Hook].xDD4_itemVar.linkhookshot.xC=prev->gobj->hsd_obj;
    current=1;CHECK(nativeArticleResource_Begin(&article_objects[1][Hook],count));
    articles[Hook].native_model_root_count=1;articles[Hook].native_model_roots[0].name_hash=123;articles[Hook].native_model_roots[0].root=&model_descriptor;
    for(unsigned i=0;i<count;++i){CHECK(nativeArticleResource_CreateNode(&article_objects[1][Hook],i,&(NativeArticleResourceInit){0,.5f,5,{0}}));CHECK(nativeArticleResource_LoadModel(&article_objects[1][Hook],i,123));}
    // A trace ordinal is not a script property; it identifies exact free order.
    unsigned ordinal=0;for(unsigned i=0;i<Pool;++i)if(node_live[i] && nodes[i].user_data && ((ItemLink*)nodes[i].user_data)->jobj==&attachment_joints[1])((ItemLink*)nodes[i].user_data)->x1CC=ordinal++;
    CHECK(ordinal==count);CHECK(nativeArticleResource_BindExternalPart(&article_objects[1][Hook],count-1,139));CHECK(nativeArticleResource_SetCaptureAnchor(&article_objects[1][Hook],count-1));
    for(unsigned h=0;h<4;++h){fighters[1].x914[h].jobj=parts[1][139].joint;fighters[1].x914[h].state=HitCapsule_Enabled;}
    fighters[1].x1064_thrownHitbox.jobj=parts[1][139].joint;fighters[1].x1064_thrownHitbox.state=HitCapsule_Enabled;
}
static const char* handles[]={"boomerang_handle","tether_handle","bow_arrow_handle","bow_handle","milk_handle"};
static void reset(unsigned mask,unsigned ownership,unsigned trails_mask,unsigned launch,unsigned mode,unsigned used)
{
    no_leaks();++scenario;memset(fighters,0,sizeof(fighters));memset(recipients,0,sizeof(recipients));memset(article_items,0,sizeof(article_items));memset(article_objects,0,sizeof(article_objects));memset(article_live,0,sizeof(article_live));memset(trace_count,0,sizeof(trace_count));memset(removed,0,sizeof(removed));memset(freed,0,sizeof(freed));memset(parts,0,sizeof(parts));
    heads[HSD_GOBJ_PLINK_ITEM]=NULL;
    for(unsigned s=0;s<2;++s){creators[s]=(HSD_GObj){.user_data=&fighters[s]};recipient_objects[s]=(HSD_GObj){.user_data=&recipients[s]};fighters[s].kind=s?Ft_Kind_CustomFirst:family;fighters[s].x8_spawnNum=0x80001300u+s;fighters[s].parts=parts[s];fighters[s].ft_data=&fdata[s];fdata[s].x8=&common_data[s];common_data[s].x11=1;parts[s][0].joint=&attachment_joints[s];parts[s][1].joint=&common_joints[s];fighters[s].motion_id=213;
        recipients[s].kind=s?Ft_Kind_CustomFirst+1:family;recipients[s].x8_spawnNum=0x80001400u+s;
    }
    creators[0].next=&creators[1];creators[1].next=&recipient_objects[0];recipient_objects[0].next=&recipient_objects[1];heads[HSD_GOBJ_PLINK_FIGHTER]=&creators[0];
    memset(recipients[1].native_module_state,0xA5,12);put(recipient_definition,&recipients[1],"unrelated",16777219);put(recipient_definition,&recipients[1],"bow_handle",77);put(recipient_definition,&recipients[1],"last_marker",0);recipients[0].u.lk.x14=(void*)(uintptr_t)1;
    const ItemKind kinds[2][Articles]={{It_Kind_Link_Boomerang,It_Kind_Link_HShot,It_Kind_Link_Arrow,It_Kind_Link_Bow,It_Kind_CLink_Milk},{It_Kind_CLink_Boomerang,It_Kind_CLink_HShot,It_Kind_CLink_Arrow,It_Kind_CLink_Bow,It_Kind_CLink_Milk}};
    for(unsigned s=0;s<2;++s)for(unsigned i=0;i<Articles;++i){bool present=(mask&(1u<<i))!=0;article_live[s][i]=present;HSD_GObj* g=&article_objects[s][i];Item* item=&article_items[s][i];g->user_data=item;g->hsd_obj=&roots[s][i];item->entity=g;item->x1C=0xFEDCBA90u+i;item->kind=kinds[family==Ft_Kind_CLink][i];item->owner=ownership==0?&creators[s]:ownership==1?&recipient_objects[s]:NULL;item->xC64_reflectGObj=ownership==3?&recipient_objects[s]:NULL;item->xC4_article_data=&articles[i];item->xDC8_word.flags.x13=0;item->destroy_type=3;
        if(present){g->next=heads[HSD_GOBJ_PLINK_ITEM];heads[HSD_GOBJ_PLINK_ITEM]=g;}
        if(s){if(i==Milk && family==Ft_Kind_Link)continue;item->native_article_author_kind=Ft_Kind_CustomFirst;item->xB8_itemLogicTable=(ItemLogicTable*)logic(item->kind)->logic;item->xDD4_itemVar.native_article.attachment_owner=&creators[1];item->xDD4_itemVar.native_article.creator_spawn_id=(u32)fighters[1].x8_spawnNum;put(definition,&fighters[1],handles[i],present?(int32_t)item->x1C:-1);}
        else {item->xB8_itemLogicTable=&stock_logics[i];if(i==Boom){item->xDD4_itemVar.linkboomerang.xF98=&creators[0];item->xDD4_itemVar.linkboomerang.xDE8=mode;}if(i==Arrow){item->xDD4_itemVar.linkarrow.xE0=&creators[0];item->xDD4_itemVar.linkarrow.xA0=launch;}if(i==Hook)item->xDD4_itemVar.linkhookshot.x8=&creators[0];if(i==Bow)item->xDD4_itemVar.linkbow.x4=&creators[0];if(i==Milk)item->xDD4_itemVar.clinkmilk.x0=&creators[0];}
    }
    fighters[0].u.lk.boomerang_gobj=(mask&1)?&article_objects[0][Boom]:NULL;fighters[0].u.lk.xC=(mask&2)?&article_objects[0][Hook]:NULL;fighters[0].u.lk.arrow_gobj=(mask&4)?&article_objects[0][Arrow]:NULL;fighters[0].u.lk.x14=(mask&8)?&article_objects[0][Bow]:NULL;fighters[0].u.lk.x18=(mask&16)?&article_objects[0][Milk]:NULL;fighters[0].u.lk.used_boomerang=used;fighters[1].native_module_state[field_offset(definition,"boomerang_used")]=used;
    article_state(Boom,"mode",mode);article_state(Arrow,"launched",launch);
    fighters[0].accessory2_cb=(void*)1;fighters[0].accessory3_cb=(void*)2;fighters[0].death1_cb=(void*)3;fighters[1].accessory2_cb=script_fn_Tether_Update;fighters[1].accessory3_cb=script_fn_Tether_Hitlag;fighters[1].death1_cb=script_fn_Tether_Remove;
    for(unsigned s=0;s<2;++s)for(unsigned k=0;k<2;++k)for(unsigned i=0;i<2;++i){unsigned which=k?Arrow:Boom;if(!(mask&(1u<<which)) || !(trails_mask&(1u<<i)))continue;trail_live[s][k][i]=true;memset(&trails[s][k][i],0,sizeof(HSD_JObj));if(s)article_items[s][which].native_models[i]=&trails[s][k][i];else if(k)article_items[s][Arrow].xDD4_itemVar.linkarrow.xB4[i]=&trails[s][k][i];else article_items[s][Boom].xDD4_itemVar.linkboomerang.xF90[i]=&trails[s][k][i];}
    if(mask&2)construct_graph(family==Ft_Kind_Link?22:15);
    memset(trace_count,0,sizeof(trace_count));current=0;
}
static void compare(void)
{++cases;CHECK(trace_count[0]==trace_count[1]);CHECK(!memcmp(trace[0],trace[1],trace_count[0]*sizeof(CleanupEvent)));CHECK(removed[0]==removed[1] && freed[0]==freed[1]);for(unsigned i=0;i<Articles;++i)CHECK(article_live[0][i]==article_live[1][i]);
 CHECK(fighters[0].u.lk.used_boomerang==(fighters[1].native_module_state[field_offset(definition,"boomerang_used")]!=0));
 for(unsigned i=0;i<Articles;++i){if(i==Milk && family==Ft_Kind_Link)continue;CHECK(get(definition,&fighters[1],handles[i])==-1);}
 CHECK((recipients[0].u.lk.x14==NULL)==(get(recipient_definition,&recipients[1],"bow_handle")==-1));CHECK(get(recipient_definition,&recipients[1],"unrelated")==16777219);for(unsigned i=0;i<12;++i)CHECK(recipients[1].native_module_state[i]==0xA5);
 CHECK(parts[1][139].joint==NULL);CHECK(fighters[1].mv.co.capturedamage.x18==NULL || fighters[1].mv.co.capturedamage.x18==&common_joints[1]);no_leaks();}
static void matrix(void)
{
 unsigned masks=family==Ft_Kind_Link?16:32;
 for(unsigned mask=0;mask<masks;++mask)for(unsigned owner=0;owner<4;++owner)
 for(unsigned trail=0;trail<4;++trail)for(unsigned launched=0;launched<2;++launched)
 for(unsigned mode=0;mode<2;++mode)for(unsigned used=0;used<2;++used)
 for(unsigned context=0;context<3;++context){
    reset(mask,owner,trail,launched,mode,used);
    fighters[0].take_dmg_cb=ftLk_800EAF58;fighters[0].death2_cb=ftLk_800EAF58;
    fighters[0].death3_cb=ftLk_SpecialS_RemoveBoomerang1;fighters[0].death1_cb=it_802A7AAC;
    current=1;script_fn_bow_callbacks(&creators[1]);fighters[1].death3_cb=script_fn_boomerang_remove;
    current=0;
    if(context==0)ftLk_800EAF58(&creators[0]);
    else if(context==1){fighters[0].take_dmg_cb(&creators[0]);if(fighters[0].death1_cb)fighters[0].death1_cb(&creators[0]);}
    else {fighters[0].death2_cb(&creators[0]);if(fighters[0].death3_cb)fighters[0].death3_cb(&creators[0]);if(fighters[0].death1_cb)fighters[0].death1_cb(&creators[0]);}
    current=1;
    if(context==0)script_fn_Cleanup_AllArticles(&creators[1]);
    else if(context==1){fighters[1].take_dmg_cb(&creators[1]);if(fighters[1].death1_cb)fighters[1].death1_cb(&creators[1]);}
    else {fighters[1].death2_cb(&creators[1]);if(fighters[1].death3_cb)fighters[1].death3_cb(&creators[1]);if(fighters[1].death1_cb)fighters[1].death1_cb(&creators[1]);}
    CHECK(get(recipient_definition,&recipients[1],"last_marker")==((mask&8)&&owner==1?37:0));
    compare();
    // Repeat the complete cleanup after destruction: no duplicate model frees.
    unsigned stock_before=trace_count[0],script_before=trace_count[1];
    current=0;ftLk_800EAF58(&creators[0]);current=1;script_fn_Cleanup_AllArticles(&creators[1]);
    CHECK(trace_count[0]==stock_before && trace_count[1]==script_before);no_leaks();
 }
}
static void rejected_handles(void)
{
 // Stock retains raw pointers; retired/recycled cases are generated safety
 // checks only. Their defined source counterpart is not invoked.
 for(unsigned kind=0;kind<Articles;++kind){
    if(kind==Milk && family==Ft_Kind_Link)continue;
    for(unsigned reason=0;reason<6;++reason){
        reset(0,0,0,0,0,0);current=1;uint32_t id=0xFEDCBA90u+kind;
        put(definition,&fighters[1],handles[kind],(int32_t)id);
        Item* item=&article_items[1][kind];HSD_GObj* g=&article_objects[1][kind];
        if(reason==0)put(definition,&fighters[1],handles[kind],-1);
        else if(reason>1){article_live[1][kind]=true;g->next=heads[HSD_GOBJ_PLINK_ITEM];heads[HSD_GOBJ_PLINK_ITEM]=g;
            if(reason==2)item->x1C=id+1;
            if(reason==3)item->native_article_author_kind=Ft_Kind_CustomFirst+2;
            if(reason==4)item->xB8_itemLogicTable=&stock_logics[kind];
            if(reason==5)item->kind=kind==Arrow?article_items[1][Bow].kind:article_items[1][Arrow].kind;
        }
        Item before=*item;script_fn_Cleanup_AllArticles(&creators[1]);
        CHECK(!memcmp(item,&before,sizeof(*item)));CHECK(trace_count[1]==0);
        if(kind!=Boom)CHECK(get(definition,&fighters[1],handles[kind])==-1);
        if(reason>1)unlink_item(g);article_live[1][kind]=false;++safety_cases;
    }
 }
 reset(8,1,0,0,0,0);current=1;recipients[1].kind=Ft_Kind_CustomFirst+3;
 unsigned char before[256];memcpy(before,recipients[1].native_module_state,256);
 script_fn_Cleanup_AllArticles(&creators[1]);CHECK(!article_live[1][Bow]);
 CHECK(!memcmp(before,recipients[1].native_module_state,256));++safety_cases;
}
static void replacement_callback(HSD_GObj* g){(void)g;abort();}
static void borrower_safety(void)
{
 for(unsigned mode=0;mode<4;++mode){
    reset(2,1,0,0,0,0);current=1;
    unsigned char before[sizeof(Fighter)];
    if(mode==0){parts[1][139].joint=&replacement;fighters[1].mv.co.capturedamage.x18=&replacement;for(unsigned h=0;h<4;++h)fighters[1].x914[h].jobj=&replacement;fighters[1].x1064_thrownHitbox.jobj=&replacement;}
    if(mode==1){fighters[1].accessory2_cb=replacement_callback;fighters[1].accessory3_cb=replacement_callback;fighters[1].death1_cb=replacement_callback;}
    if(mode==2)fighters[1].kind=Ft_Kind_CustomFirst+2;
    if(mode==3)creators[0].next=&recipient_objects[0]; // Creator generation is absent.
    memcpy(before,&fighters[1],sizeof(Fighter));
    CHECK(signal_article(&article_objects[1][Hook],article_items[1][Hook].x1C,3683784189u,0));
    CHECK(!article_live[1][Hook]);CHECK(freed[1]==(family==Ft_Kind_Link?22u:15u));
    if(mode==0){CHECK(parts[1][139].joint==&replacement);CHECK(fighters[1].mv.co.capturedamage.x18==&replacement);for(unsigned h=0;h<4;++h)CHECK(fighters[1].x914[h].jobj==&replacement);CHECK(fighters[1].x1064_thrownHitbox.jobj==&replacement);}
    if(mode==1){CHECK(fighters[1].accessory2_cb==replacement_callback);CHECK(fighters[1].accessory3_cb==replacement_callback);CHECK(fighters[1].death1_cb==replacement_callback);}
    if(mode>=2)CHECK(!memcmp(before,&fighters[1],sizeof(Fighter)));
    // Stock side was not invoked in this safety-only case; release its live
    // graphs through its actual body after the safety assertions.
    current=0;it_802A2B10(&article_objects[0][Hook]);no_leaks();++safety_cases;
 }
}
int main(int argc,char** argv)
{
 if(argc!=5)return 2;family=!strcmp(argv[2],"link")?Ft_Kind_Link:Ft_Kind_CLink;
 FILE* file=fopen(argv[1],"rb");CHECK(file);fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);char* text=malloc(size+1);CHECK(text);CHECK(fread(text,1,size,file)==(size_t)size);fclose(file);NativeFighterParameters p;unsigned error;CHECK(nativeFighterParameters_Parse(text,size,&p,&error));ground_parameters=p;free(text);uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<p.count;++i)bits[i]=p.fields[i].bits;CHECK(FreakMelee_SetFighterParameters(bits,p.count,p.schema_hash));
 NativeFighterHost host={0};
#include "unused_host.h"
 host.callback_source_alive=nativeCallbackSource_Alive;host.article_ref_exists=nativeArticleRef_Exists;host.signal_article=signal_article;host.item_creator=nativeArticle_Creator;host.item_current_owner=current_owner;host.target_call=nativeFighterEvent_Call;host.item_destroy_model=nativeArticleModel_Destroy;host.item_remove=Item_8026A8EC;host.article_resource_release=nativeArticleResource_Release;
 init_ground_host(&host);
 MotionState base_states[400]={0};NativeFighterDefinition base={.motion_states=base_states,.motion_state_count=400,.fighter_root=family==Ft_Kind_Link?"ftDataLink":"ftDataClink"};definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);
 HMODULE dll=LoadLibraryA(argv[3]);CHECK(dll);typedef const NativeFighterDefinition*(__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);Entry open=(Entry)GetProcAddress(dll,"FreakMelee_GetFighterModule");CHECK(open);typedef int(__cdecl* Set)(const uint32_t*,unsigned,uint64_t);Set set=(Set)GetProcAddress(dll,"FreakMelee_SetFighterParameters");CHECK(set);
 file=fopen(argv[4],"rb");CHECK(file);fseek(file,0,SEEK_END);size=ftell(file);rewind(file);text=malloc(size+1);CHECK(text);CHECK(fread(text,1,size,file)==(size_t)size);fclose(file);NativeFighterParameters rp;CHECK(nativeFighterParameters_Parse(text,size,&rp,&error));free(text);CHECK(rp.count==1 && rp.fields[0].bits==37);CHECK(set(&rp.fields[0].bits,1,rp.schema_hash));NativeFighterDefinition rb=base;rb.state_bytes=11;recipient_definition=open(&rb,&host);CHECK(recipient_definition);CHECK(field_offset(recipient_definition,"bow_handle")!=field_offset(definition,"bow_handle"));foreign_definition=*definition;missing_definition=*recipient_definition;missing_definition.callback_export_count=0;missing_definition.callback_exports=NULL;
 part_table.parts_num=family==Ft_Kind_Link?76:80;part_tables[Ft_Kind_CustomFirst]=&part_table;
 stock_logics[Boom].destroyed=itLinkBoomerang_Logic18_Destroyed;stock_logics[Arrow].destroyed=itLinkArrow_Logic98_Destroyed;stock_logics[Bow].destroyed=itLinkBow_Logic100_Destroyed;
 ground_matrix();matrix();rejected_handles();borrower_safety();
 // Retune only the independent recipient's owned parameter storage. The
 // author DLL/C state and imported parameter bytes remain identical.
 uint32_t value=61;CHECK(set(&value,1,rp.schema_hash));
 reset(8,1,0,0,0,0);current=1;script_fn_Cleanup_AllArticles(&creators[1]);
 CHECK(get(recipient_definition,&recipients[1],"last_marker")==61);
 for(unsigned i=0;i<p.count;++i)CHECK(script_parameters[i].bits==bits[i]);
 current=0;ftLk_800EAF58(&creators[0]);no_leaks();++safety_cases;
 printf("{\"assertions\":%u,\"source_cases\":%u,\"generated_safety_cases\":%u,\"grounded_cases\":%u,\"ordered_timelines\":%u,\"ground_events\":%u,\"scope\":\"common Catch hooks, grounded tether transitions and composed article teardown; controlled motion/Item/HSD boundaries\"}\n",checks,cases,safety_cases,grounded_cases,timelines,ground_events);FreeLibrary(dll);return 0;
}

// Additional common-policy evidence. Motion, allocation and collision queries
// are controlled; complete common/native and generated decisions execute.
static void fn_800D8BFC(Fighter_GObj*);
void fn_800D9CE8(Fighter_GObj*);
static struct ftCommonData common;struct ftCommonData* p_ftCommonData=&common;
static ftLk_DatAttrs catch_attrs;static struct TetherAttributes hook_attrs;
static ItemAttr hook_common;
static bool input_item_priority,animation_remaining,obstructed,creation_failure,directional_input,compare_ground_counter;
static uint32_t parameter_value(const char* name)
{for(unsigned i=0;i<ground_parameters.count;i++)if(!strcmp(ground_parameters.fields[i].name,name))return ground_parameters.fields[i].bits;CHECK(false);return 0;}
static float parameter_float(const char* name){uint32_t b=parameter_value(name);float f;memcpy(&f,&b,4);return f;}
static uint32_t float_bits(float value){uint32_t b;memcpy(&b,&value,4);return b;}
static unsigned creator_side(HSD_GObj* g){CHECK(g==&creators[0]||g==&creators[1]||g==&recipient_objects[0]||g==&recipient_objects[1]);return g==&creators[1]||g==&recipient_objects[1];}
static void ground_event(unsigned type,unsigned a,unsigned b){event(type,a,b);++ground_events;}
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* other)
{CHECK(creator_side(g)==current);CHECK(!other&&rate==1&&blend==0);ground_event(10,motion,float_bits(frame));Fighter* f=g->user_data;f->motion_id=motion;f->cur_anim_frame=frame;(void)flags;}
void ftCommon_8007E2D0(Fighter* f,s16 type,HSD_GObjEvent grab,HSD_GObjEvent pickup,void(*caught)(HSD_GObj*,HSD_GObj*))
{CHECK(f==&fighters[current]&&type==1&&grab==fn_800D9CE8&&pickup==fn_800D8BFC&&caught);ground_event(11,0,0);}
void fn_800DAADC(Fighter_GObj* a,Fighter_GObj* b){(void)a;(void)b;abort();}
bool ftCo_800951D0(Fighter_GObj* g){CHECK(creator_side(g)==current);ground_event(12,0,0);return input_item_priority;}
bool ftCo_80095254(Fighter_GObj* g){CHECK(creator_side(g)==current);ground_event(12,0,0);return input_item_priority;}
bool ftCo_800952DC(Fighter_GObj* g){CHECK(creator_side(g)==current);ground_event(12,0,0);return input_item_priority;}
bool fn_800D9558(Fighter_GObj* g){CHECK(creator_side(g)==current);return false;}
bool fn_800D9930(Fighter_GObj* g){CHECK(creator_side(g)==current);return false;}
void it_802B7B84(Item_GObj* g){(void)g;abort();}
void it_802BAC3C(Fighter_GObj* g){(void)g;abort();}
void it_802BAA94(Item_GObj* g){(void)g;abort();}
bool ftAnim_IsFramesRemaining(Fighter_GObj* g){CHECK(creator_side(g)==current);return animation_remaining;}
void ft_8008A2BC(Fighter_GObj* g){CHECK(creator_side(g)==current);ground_event(13,0,0);((Fighter*)g->user_data)->motion_id=14;}
void ftCo_Fall_Enter(Fighter_GObj* g){CHECK(creator_side(g)==current);ground_event(14,0,0);((Fighter*)g->user_data)->motion_id=29;}
void ftAnim_SetAnimRate(Fighter_GObj* g,float rate){(void)g;(void)rate;abort();}
void ftAnim_8006EBA4(Fighter_GObj* g){(void)g;abort();}
void ftpickupitem_800948A8(Fighter_GObj* g,Item_GObj* item){(void)g;(void)item;abort();}
void ftpickupitem_80094694(Fighter_GObj* g,FtMotionId id,bool value){(void)g;(void)id;(void)value;abort();}
void fn_800DA4FC(Fighter_GObj* g){CHECK(creator_side(g)==current);ground_event(15,0,0);}
bool ftCo_800DD1E4(Fighter_GObj* g){CHECK(creator_side(g)==current);ground_event(16,0,0);return directional_input;}
void ftCo_800DA698(Fighter_GObj* g,bool force){(void)g;(void)force;abort();}
Fighter_Part ftParts_GetBoneIndex(Fighter* f,Fighter_Part part){CHECK(f==&fighters[0]||f==&fighters[1]);CHECK(part==FtPart_RThumbNb||part==FtPart_XRotN);return 0;}
void lb_8000B1CC(HSD_JObj* j,Vec3* offset,Vec3* out){CHECK(!offset&&out);*out=(Vec3){j->mtx[0][3],j->mtx[1][3],j->mtx[2][3]};}
static unsigned bone_count(Fighter* f){CHECK(f==&fighters[1]);return MAX_FT_PARTS;}
void efAsync_Spawn(HSD_GObj* g,void* queue,u32 kind,u32 id,HSD_JObj* joint,...){CHECK(creator_side(g)==current&&queue&&kind==1&&id==0x41D&&joint==&common_joints[current]);ground_event(17,0,0);}
void ftCommon_8007EBAC(Fighter* f,u32 kind,u32 value){CHECK(kind==3&&!value);CHECK(f==&fighters[current]||f==&recipients[current]);ground_event(18,f==&recipients[current],0);}
void ftCommon_8007E2F4(Fighter* f,s16 bits){CHECK(bits==511);CHECK(f==&fighters[current]||f==&recipients[current]);ground_event(19,f==&recipients[current],0);}
float ftYs_Init_8012BAC0(Fighter* f){(void)f;abort();}
void nativeCapture_EncloseCaught(Fighter_GObj* g,float radius){(void)g;(void)radius;abort();}
void ftCo_800DB368(Fighter* a,Fighter* b){(void)a;(void)b;abort();}
void Item_80268E5C(Item_GObj* g,enum_t state,Item_StateChangeFlags flags){unsigned s,i=article_index(g,&s);CHECK(s==current&&i==Hook&&flags==2);article_items[s][Hook].msid=state;ground_event(20,state,flags);}
void ftColl_8007AFF8(Fighter_GObj* g){CHECK(creator_side(g)==current);ground_event(21,0,0);}
void it_80272F7C(HSD_JObj* joint,float scale){CHECK(joint==&roots[current][Hook]);joint->scale=(Vec3){scale,scale,scale};ground_event(22,float_bits(scale),0);}
void it_8026BAE8(Item_GObj* g,float scale);
static float root_scale(Fighter_GObj* g){return ((HSD_JObj*)g->hsd_obj)->scale.y;}
bool mpCheckAllRemap(Vec3* pos,int* line,u32* flags,Vec3* normal,int skip,int only,float x0,float y0,float x1,float y1)
{CHECK(!pos&&!line&&!flags&&!normal&&skip==-1&&only==-1);ground_event(23,float_bits(x0),float_bits(x1));CHECK(y0==y1);return obstructed;}
void ft_PlaySFX(Fighter* f,enum_t sound,u8 volume,u8 pan){CHECK(f==&fighters[current]&&volume==127&&pan==64);ground_event(24,sound,0);}
static void ensure_created(void)
{
 for(unsigned s=0;s<2;s++){
   HSD_GObj* g=&article_objects[s][Hook];Item* item=&article_items[s][Hook];CHECK(!article_live[s][Hook]);article_live[s][Hook]=true;g->next=heads[HSD_GOBJ_PLINK_ITEM];heads[HSD_GOBJ_PLINK_ITEM]=g;
   if(s==0)fighters[0].u.lk.xC=g;
 }
 unsigned saved=current;construct_graph(15);current=saved;
 article_state(Hook,"count",15);article_state(Hook,"endpoint",14);article_state(Hook,"launch_speed",(int32_t)float_bits(hook_attrs.pos_x_0));
}
Item_GObj* it_802A2BA4(Fighter_GObj* g,Vec3* position,float facing,s32 kind)
{
 CHECK(g==&creators[0]&&kind==catch_attrs.xBC&&position->x==11&&facing==fighters[0].facing_dir);ground_event(25,kind,float_bits(facing));if(creation_failure)return NULL;
 ensure_created();current=0;it_8026BAE8(&article_objects[0][Hook],fighters[0].x34_scale.y);return &article_objects[0][Hook];
}
static s32 spawn_attached(Fighter_GObj* g,ItemKind kind,Fighter_Part part)
{CHECK(g==&creators[1]&&kind==catch_attrs.xBC&&part==FtPart_RThumbNb);ground_event(25,kind,float_bits(fighters[1].facing_dir));return creation_failure?-1:(s32)article_items[1][Hook].x1C;}
void it_802A7AF0(Fighter_GObj* g){(void)g;abort();}
void it_802A7B34(Fighter_GObj* g){(void)g;abort();}
#include "common_originals.h"
static void init_ground_host(NativeFighterHost* host)
{
 host->article_resource_read_float=nativeArticleResource_ReadFloat;host->article_resource_write_float=nativeArticleResource_WriteFloat;host->article_ref_bool=nativeArticleRef_Bool;
 host->resolve_bone=ftParts_GetBoneIndex;host->bone_count=bone_count;host->joint_position=lb_8000B1CC;host->spawn_bone_attached_article=spawn_attached;host->stage_segment_intersection=mpCheckAllRemap;host->play_sound=ft_PlaySFX;host->finish_ground_action=ft_8008A2BC;
 host->item_change_state_flags=Item_80268E5C;host->item_scale_by_attribute=it_8026BAE8;host->clear_hitboxes=ftColl_8007AFF8;host->fighter_root_scale_y=root_scale;host->article_resource_set_capture_anchor=nativeArticleResource_SetCaptureAnchor;
}
static void ground_reset(unsigned mask,float counter,int motion)
{
 reset(mask,0,0,0,0,0);memset(trace_count,0,sizeof(trace_count));input_item_priority=obstructed=creation_failure=directional_input=false;animation_remaining=true;compare_ground_counter=true;
 hook_common.x60_scale=.75f;
 for(unsigned s=0;s<2;s++){
   Fighter* f=&fighters[s];f->motion_id=motion;f->cur_anim_frame=3;f->dat_attrs=&catch_attrs;f->x34_scale=(Vec3){1,1.25f,1};f->x74_self_accel=(Vec3){3,4,5};f->mv.co.catch.x0=counter;f->facing_dir=1;f->coll_data.cur_pos=(Vec3){7,8,9};
   f->victim_gobj=&recipient_objects[s];recipients[s].victim_gobj=&creators[s];recipients[s].motion_id=ftCo_MS_CapturePulledHi;recipients[s].parts=parts[s];recipients[s].ft_data=&fdata[s];
   creators[s].hsd_obj=&common_joints[s];common_joints[s].scale=(Vec3){1,2,1};attachment_joints[s].flags=0;attachment_joints[s].mtx[0][3]=11;attachment_joints[s].mtx[1][3]=13;attachment_joints[s].mtx[2][3]=17;
   articles[Hook].x4_specialAttributes=&hook_attrs;article_items[s][Hook].xCC_item_attr=&hook_common;
 }
 put(definition,&fighters[1],"tether_counter",(int32_t)float_bits(counter));
 if(mask&2){unsigned count=nativeArticleResource_Count(&article_objects[1][Hook]);article_state(Hook,"count",count);article_state(Hook,"endpoint",count-1);article_state(Hook,"launch_speed",(int32_t)float_bits(hook_attrs.pos_x_0));}
}
static void ground_compare(void)
{
 ++grounded_cases;CHECK(trace_count[0]==trace_count[1]);CHECK(!memcmp(trace[0],trace[1],trace_count[0]*sizeof(CleanupEvent)));
 CHECK(fighters[0].motion_id==fighters[1].motion_id);CHECK(fighters[0].cur_anim_frame==fighters[1].cur_anim_frame);CHECK(fighters[0].throw_flags==fighters[1].throw_flags);
 if(compare_ground_counter)CHECK(fighters[0].mv.co.catch.x0==({float value;int32_t b=get(definition,&fighters[1],"tether_counter");memcpy(&value,&b,4);value;}));
 CHECK(!memcmp(&fighters[0].x74_self_accel,&fighters[1].x74_self_accel,sizeof(Vec3)));
 CHECK(article_live[0][Hook]==article_live[1][Hook]);CHECK((fighters[0].accessory1_cb!=NULL&&fighters[0].accessory1_cb!=fn_800DA190)==(fighters[1].accessory1_cb!=NULL));
 if(article_live[0][Hook]){CHECK(article_items[0][Hook].msid==article_items[1][Hook].msid);CHECK(article_items[0][Hook].scl==article_items[1][Hook].scl);}
}
static void finish_ground(void)
{current=0;ftLk_800EAF58(&creators[0]);current=1;script_fn_Cleanup_AllArticles(&creators[1]);no_leaks();}
static void ground_matrix(void)
{
 catch_attrs.x84=parameter_value("tether_standing_create");catch_attrs.x88=parameter_value("tether_standing_launch");catch_attrs.x8C=parameter_value("tether_standing_retract");catch_attrs.x90=parameter_value("tether_standing_remove");
 catch_attrs.x94=parameter_value("tether_dash_create");catch_attrs.x98=parameter_value("tether_dash_launch");catch_attrs.x9C=parameter_value("tether_dash_retract");catch_attrs.xA0=parameter_value("tether_dash_remove");catch_attrs.xBC=family==Ft_Kind_Link?It_Kind_Link_HShot:It_Kind_CLink_HShot;
 hook_attrs.pos_x_0=parameter_float(family==Ft_Kind_Link?"It_Kind_Link_HShot.source_launch_speed":"It_Kind_CLink_HShot.source_launch_speed");
 for(unsigned dash=0;dash<2;dash++)for(unsigned flags=0;flags<8;flags++){
  ground_reset(0,0,dash?214:212);creation_failure=flags&1;obstructed=flags&2;animation_remaining=true;
  fighters[0].facing_dir=fighters[1].facing_dir=(flags&4)?-1:1;
  current=0;ftCo_800D8C54(&creators[0],dash?214:212);current=1;ftCo_800D8C54(&creators[1],dash?214:212);ground_compare();++timelines;
  for(unsigned tick=1;tick<=(unsigned)(dash?catch_attrs.xA0:catch_attrs.x90)+2;tick++){
   memset(trace_count,0,sizeof(trace_count));current=0;if(dash)ftCo_CatchDash_Anim(&creators[0]);else ftCo_Catch_Anim(&creators[0]);
   current=1;if(dash)ftCo_CatchDash_Anim(&creators[1]);else ftCo_Catch_Anim(&creators[1]);ground_compare();if(fighters[0].motion_id==14)break;
  }
  finish_ground();
 }
 for(unsigned mask=0;mask<2;mask++)for(unsigned command=0;command<16;command++)for(unsigned path=0;path<4;path++){
  ground_reset(mask?2:0,2,14);compare_ground_counter=false;input_item_priority=command&1;
  for(unsigned s=0;s<2;s++){fighters[s].input.held_buttons[0]=(command&2)?HSD_PAD_LR:0;fighters[s].input.pressed_buttons=(command&4)?HSD_PAD_A:0;fighters[s].mv.co.common.x0=(command&8)?0:3;fighters[s].mv.co.guard.x24=(command&8)?0:3;}
  bool results[2];for(current=0;current<2;current++){
   if(path==0)results[current]=ftCo_Catch_CheckInput(&creators[current]);
   if(path==1)results[current]=ftCo_800D8A38(&creators[current]);
   if(path==2)results[current]=ftCo_800D8AE0(&creators[current]);
   if(path==3)results[current]=ftCo_800D8B9C(&creators[current]);
  }
  current=1;CHECK(results[0]==results[1]);ground_compare();
  CHECK(fighters[0].mv.co.common.x0==fighters[1].mv.co.common.x0&&fighters[0].mv.co.guard.x24==fighters[1].mv.co.guard.x24);
  if(results[0]&&(path==3||!input_item_priority))CHECK(get(definition,&fighters[1],"tether_counter")==0);
  finish_ground();
 }
 for(unsigned completed=0;completed<2;completed++)for(unsigned dash=0;dash<2;dash++)for(unsigned low=0;low<2;low++){
  ground_reset(2,0,dash?214:212);article_items[0][Hook].xDD4_itemVar.linkhookshot.x14=completed;article_state(Hook,"completed",completed);
  for(unsigned side=0;side<2;side++)recipients[side].motion_id=low?ftCo_MS_CapturePulledLw:ftCo_MS_CapturePulledHi;
  current=0;fn_800D9CE8(&creators[0]);current=1;fn_800D9CE8(&creators[1]);ground_compare();
  CHECK(!fighters[1].accessory1_cb);
  /* Native Link/CLink bind DA190, whose actual body does nothing. The
   * authored false follow policy intentionally leaves accessory1 NULL. */
  current=0;unsigned before=trace_count[0];Fighter retained=fighters[0];fn_800DA190(&creators[0]);CHECK(trace_count[0]==before&&!memcmp(&retained,&fighters[0],sizeof(retained)));
  current=1;CHECK(fighters[1].mv.co.capturedamage.x18==parts[1][139].joint);CHECK(fighters[0].mv.co.capturedamage.x18==article_items[0][Hook].xDD4_itemVar.linkhookshot.xC);
  memset(trace_count,0,sizeof(trace_count));current=0;ftCo_CatchPull_Anim(&creators[0]);current=1;ftCo_CatchPull_Anim(&creators[1]);ground_compare();finish_ground();
 }
 // Common animation completion remains after the authored frame policy.
 for(unsigned dash=0;dash<2;dash++)for(unsigned remaining=0;remaining<2;remaining++){
  ground_reset(0,(float)(dash?catch_attrs.xA0:catch_attrs.x90)+1,dash?214:212);animation_remaining=remaining;
  current=0;if(dash)ftCo_CatchDash_Anim(&creators[0]);else ftCo_Catch_Anim(&creators[0]);
  current=1;if(dash)ftCo_CatchDash_Anim(&creators[1]);else ftCo_Catch_Anim(&creators[1]);ground_compare();finish_ground();
 }
 // Missing article readiness is defined in the original; common CatchWait
 // entry and victim CaptureWait notification still execute on both paths.
 ground_reset(0,0,213);current=0;ftCo_CatchPull_Anim(&creators[0]);current=1;ftCo_CatchPull_Anim(&creators[1]);ground_compare();finish_ground();
 for(unsigned present=0;present<2;present++){
  ground_reset(present?2:0,0,212);current=0;fn_800D8E30(&creators[0]);current=1;fn_800D8E30(&creators[1]);ground_compare();finish_ground();
 }
 for(unsigned present=0;present<2;present++)for(unsigned pummel=0;pummel<2;pummel++)for(unsigned direction=0;direction<2;direction++){
  ground_reset(present?2:0,0,216);fighters[0].input.pressed_buttons=fighters[1].input.pressed_buttons=pummel?HSD_PAD_A:0;directional_input=direction;
  current=0;ftCo_CatchWait_IASA(&creators[0]);current=1;ftCo_CatchWait_IASA(&creators[1]);ground_compare();finish_ground();
 }
 // Living article without a collection: common anchor is the explicit
 // authored safety fallback; the original NULL endpoint has no safe follow.
 ground_reset(2,0,212);current=1;nativeArticleResource_Release(&article_objects[1][Hook]);
 memset(trace_count,0,sizeof(trace_count));fn_800D9CE8(&creators[1]);
 CHECK(fighters[1].mv.co.capturedamage.x18==&common_joints[1]);CHECK(!fighters[1].accessory1_cb);finish_ground();++safety_cases;
 ground_reset(0,0,212);current=1;fn_800D9CE8(&creators[1]);CHECK(fighters[1].mv.co.capturedamage.x18==&common_joints[1]);CHECK(!fighters[1].accessory1_cb);finish_ground();++safety_cases;
}
