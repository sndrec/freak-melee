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
#include "cleanup.generated.c"

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
static FighterKind family;
typedef struct CleanupEvent {unsigned type,item,index;} CleanupEvent;
static CleanupEvent trace[2][100];static unsigned trace_count[2];
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"scenario%u side%u line%u: %s\n",scenario,current,__LINE__,#x);abort();}}while(0)
static void event(unsigned type,unsigned item,unsigned index)
{CHECK(trace_count[current]<100);trace[current][trace_count[current]++]=(CleanupEvent){type,item,index};}
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
 if(current){if(nativeCallbackSource_Alive(&creators[1],0x80001301u,NativeCallbackSource_Fighter) && fighters[1].kind==Ft_Kind_CustomFirst){CHECK(parts[1][139].joint!=g->hsd_obj);CHECK(fighters[1].mv.co.capturedamage.x18!=g->hsd_obj);for(unsigned h=0;h<4;++h)CHECK(fighters[1].x914[h].jobj!=g->hsd_obj);CHECK(fighters[1].x1064_thrownHitbox.jobj!=g->hsd_obj);}}
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
void HSD_JObjSetupMatrixSub(HSD_JObj* j){(void)j;abort();}
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
 FILE* file=fopen(argv[1],"rb");CHECK(file);fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);char* text=malloc(size+1);CHECK(text);CHECK(fread(text,1,size,file)==(size_t)size);fclose(file);NativeFighterParameters p;unsigned error;CHECK(nativeFighterParameters_Parse(text,size,&p,&error));free(text);uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<p.count;++i)bits[i]=p.fields[i].bits;CHECK(FreakMelee_SetFighterParameters(bits,p.count,p.schema_hash));
 NativeFighterHost host={0};
#include "unused_host.h"
 host.callback_source_alive=nativeCallbackSource_Alive;host.article_ref_exists=nativeArticleRef_Exists;host.signal_article=signal_article;host.item_creator=nativeArticle_Creator;host.item_current_owner=current_owner;host.target_call=nativeFighterEvent_Call;host.item_destroy_model=nativeArticleModel_Destroy;host.item_remove=Item_8026A8EC;host.article_resource_release=nativeArticleResource_Release;
 MotionState base_states[400]={0};NativeFighterDefinition base={.motion_states=base_states,.motion_state_count=400,.fighter_root=family==Ft_Kind_Link?"ftDataLink":"ftDataClink"};definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);
 HMODULE dll=LoadLibraryA(argv[3]);CHECK(dll);typedef const NativeFighterDefinition*(__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);Entry open=(Entry)GetProcAddress(dll,"FreakMelee_GetFighterModule");CHECK(open);typedef int(__cdecl* Set)(const uint32_t*,unsigned,uint64_t);Set set=(Set)GetProcAddress(dll,"FreakMelee_SetFighterParameters");CHECK(set);
 file=fopen(argv[4],"rb");CHECK(file);fseek(file,0,SEEK_END);size=ftell(file);rewind(file);text=malloc(size+1);CHECK(text);CHECK(fread(text,1,size,file)==(size_t)size);fclose(file);NativeFighterParameters rp;CHECK(nativeFighterParameters_Parse(text,size,&rp,&error));free(text);CHECK(rp.count==1 && rp.fields[0].bits==37);CHECK(set(&rp.fields[0].bits,1,rp.schema_hash));NativeFighterDefinition rb=base;rb.state_bytes=11;recipient_definition=open(&rb,&host);CHECK(recipient_definition);CHECK(field_offset(recipient_definition,"bow_handle")!=field_offset(definition,"bow_handle"));foreign_definition=*definition;missing_definition=*recipient_definition;missing_definition.callback_export_count=0;missing_definition.callback_exports=NULL;
 part_table.parts_num=family==Ft_Kind_Link?76:80;part_tables[Ft_Kind_CustomFirst]=&part_table;
 stock_logics[Boom].destroyed=itLinkBoomerang_Logic18_Destroyed;stock_logics[Arrow].destroyed=itLinkArrow_Logic98_Destroyed;stock_logics[Bow].destroyed=itLinkBow_Logic100_Destroyed;
 matrix();rejected_handles();borrower_safety();
 // Retune only the independent recipient's owned parameter storage. The
 // author DLL/C state and imported parameter bytes remain identical.
 uint32_t value=61;CHECK(set(&value,1,rp.schema_hash));
 reset(8,1,0,0,0,0);current=1;script_fn_Cleanup_AllArticles(&creators[1]);
 CHECK(get(recipient_definition,&recipients[1],"last_marker")==61);
 for(unsigned i=0;i<p.count;++i)CHECK(script_parameters[i].bits==bits[i]);
 current=0;ftLk_800EAF58(&creators[0]);no_leaks();++safety_cases;
 printf("{\"assertions\":%u,\"source_cases\":%u,\"generated_safety_cases\":%u,\"scope\":\"composed removal signals/destructors and resource teardown; controlled Item/HSD lifecycle\"}\n",checks,cases,safety_cases);FreeLibrary(dll);return 0;
}
