#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_AirCatch.h>
#include <melee/ft/kinds/ftCommon/ftCo_CliffJump.h>
#include <melee/ft/kinds/ftCommon/ftCo_DamageFall.h>
#include <melee/ef/efsync.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/native_model_joint.h>
#include <stdarg.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftSamus/types.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/inlines.h>
#include <melee/it/itarticle_data.h>
#include <melee/it/item.h>
#include <melee/it/native_article.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_article_model.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/lb/lbvector.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mpisland.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/dat_import_internal.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mtx.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum { Nodes=90, Parts=60 };
static HSD_GObj fighters[2], objects[2], nodes[2][Nodes];
static Fighter fighter[2]; static Item item[2]; static ItemLink links[2][Nodes];
static FighterBone parts[2][MAX_FT_PARTS];
static HSD_JObj models[2][Nodes][2], primary[2][2], attachment[2];
static bool node_live[2][Nodes], item_live[2];
static unsigned created[2], allocations[2], loaded[2], freed[2], current_side;
static HSD_GObj* heads[32]; HSD_GObj** HSD_GObjPLinkHead=heads;
static FighterPartsTable part_table={.parts_num=Parts};
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity]; FighterPartsTable** ftPartsTable=tables;
HSD_ObjAllocData item_link_alloc_data; u8 HSD_GObj_JObjKind;
const Vec3 it_803B8674={0};
static const NativeFighterDefinition* definition; static const NativeFighterArticleLogic* logic;
static const NativeFighterDefinition* active_definition;
static Article article; static ItemAttr common_attrs; static itSamusGrappleAttributes tuning[2], retail;
static unsigned checks,pairs,safety,scenario; static float max_error;
static int fail_kind,fail_index,retirement;
static unsigned attachment_calls[2],remove_calls[2];
static uint8_t arena[16*1024*1024]; static uint8_t* dat_bytes;
enum { Spawn=1,NewNode,Allocate,Initialize,Collision,Model,Object,GX,AddAnimation,RequestAnimation,PickupSound,Attach,State,Scale,PickupTail,Free,Remove };
typedef struct CtorEvent { unsigned kind; int a,b,c; float f[4]; } CtorEvent;
static CtorEvent events[2][2048];static unsigned event_count[2];
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u scenario%u: %s\n",(unsigned)__LINE__,scenario,#x);abort();}}while(0)
static void trace(unsigned kind,int a,int b,int c,float x,float y,float z,float w)
{CHECK(event_count[current_side]<2048);events[current_side][event_count[current_side]++]=(CtorEvent){kind,a,b,c,{x,y,z,w}};}
static uint32_t hash(const char* n){uint32_t h=2166136261u;for(;*n;++n)h=(h^(unsigned char)*n)*16777619u;return h;}
static unsigned side(HSD_GObj* o){return o==&fighters[1]||o==&objects[1]?1:0;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k){return k==Ft_Kind_Samus?active_definition:NULL;}
Fighter_GObj* nativeArticle_Creator(Item_GObj* o)
{if(!o||!o->user_data)return NULL;Item* i=o->user_data;HSD_GObj* g=i->xDD4_itemVar.native_article.attachment_owner;if(!nativeCallbackSource_Alive(g,i->xDD4_itemVar.native_article.creator_spawn_id,NativeCallbackSource_Fighter))return NULL;return nativeFighterRuntime_Definition(((Fighter*)g->user_data)->kind)==definition?g:NULL;}
static int model_id(HSD_Joint* desc){HSD_Joint* d[]={retail.x64,retail.x68,retail.x6C,retail.x70};for(int i=0;i<4;++i)if(d[i]==desc)return i;CHECK(false);return -1;}
static int graph_id(void* graph,unsigned channel)
{if(!graph)return -1;for(unsigned i=0;i<article.native_model_animation_count;++i){void* p=channel==0?(void*)article.native_model_animations[i].anim:channel==1?(void*)article.native_model_animations[i].matanim:(void*)article.native_model_animations[i].shapeanim;if(p==graph){uint32_t h=article.native_model_animations[i].name_hash;const char* n[]={"ChainAAnimation","ChainBAnimation","ChainCAnimation","EndpointAnimation","PrimaryAnimation"};for(int k=0;k<5;++k)if(hash(n[k])==h)return k;}}CHECK(false);return -2;}
static int joint_id(HSD_JObj* j)
{if(j==&primary[current_side][0])return -2;if(j==&primary[current_side][1])return -1;for(unsigned i=0;i<Nodes;++i)for(unsigned c=0;c<2;++c)if(j==&models[current_side][i][c])return (int)(i*2+c);CHECK(false);return -3;}
HSD_GObj* GObj_Create(u16 kind,u8 link,u8 priority)
{CHECK(kind==7&&link==10&&priority==0);unsigned n=created[current_side]++;CHECK(n<Nodes);trace(NewNode,n,kind,link,priority,0,0,0);if(fail_kind==1&&(int)n==fail_index)return NULL;node_live[current_side][n]=true;return &nodes[current_side][n];}
void* HSD_ObjAlloc(HSD_ObjAllocData* p)
{CHECK(p==&item_link_alloc_data);unsigned n=allocations[current_side]++;trace(Allocate,n,0,0,0,0,0,0);if(fail_kind==2&&(int)n==fail_index)return NULL;return &links[current_side][n];}
void HSD_ObjFree(HSD_ObjAllocData* p,void* v){CHECK(p==&item_link_alloc_data&&v);}
void GObj_InitUserData(HSD_GObj* g,u8 kind,void(*destroy)(void*),void* v)
{CHECK(kind==6);g->user_data=v;g->user_data_remove_func=destroy;trace(Initialize,allocations[current_side]-1,kind,0,0,0,0,0);}
void HSD_GObjFree(HSD_GObj* g)
{for(unsigned n=0;n<Nodes;++n)if(g==&nodes[current_side][n]){CHECK(node_live[current_side][n]);node_live[current_side][n]=false;trace(Free,n,0,0,0,0,0,0);++freed[current_side];if(g->user_data_remove_func)g->user_data_remove_func(g->user_data);g->user_data=NULL;g->hsd_obj=NULL;return;}CHECK(false);}
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* d)
{unsigned n=loaded[current_side]++;CHECK(n<Nodes&&d);trace(Model,n,model_id(d),0,0,0,0,0);if(fail_kind==3&&(int)n==fail_index)return NULL;HSD_JObj* m=models[current_side][n];m[0].flags=d->flags;m[0].scale=d->scale;m[0].translate=d->position;m[0].child=&m[1];if(d->child){m[1].flags=d->child->flags;m[1].scale=d->child->scale;m[1].translate=d->child->position;}return m;}
void HSD_GObjObject_80390A70(HSD_GObj* g,u8 kind,void* obj)
{CHECK(kind==HSD_GObj_JObjKind);g->hsd_obj=obj;trace(Object,loaded[current_side]-1,kind,0,0,0,0,0);}
void GObj_SetupGXLink(HSD_GObj* g,void(*draw)(HSD_GObj*,int),u8 link,u32 priority)
{CHECK(link==6&&priority==0);g->render_cb=draw;trace(GX,loaded[current_side]-1,link,priority,0,0,0,0);}
void HSD_GObj_JObjCallback(HSD_GObj* g,int pass){(void)g;(void)pass;}
static void invalidate(unsigned action);
void HSD_JObjAddAnimAll(HSD_JObj* j,HSD_AnimJoint* a,HSD_MatAnimJoint* m,HSD_ShapeAnimJoint* s)
{CHECK(j);trace(AddAnimation,joint_id(j),graph_id(a,0),graph_id(m,1),graph_id(s,2),0,0,0);if(current_side==1&&joint_id(j)==-1&&retirement){unsigned action=retirement;retirement=0;invalidate(action);}}
void HSD_JObjReqAnimAll(HSD_JObj* j,float f){CHECK(j);trace(RequestAnimation,joint_id(j),0,0,f,0,0,0);}
void HSD_JObjAnimAll(HSD_JObj* j){(void)j;abort();}
void HSD_JObjRemoveAnimAll(HSD_JObj* j){(void)j;abort();}
void lb_8000BA0C(HSD_JObj* j,float r){(void)j;(void)r;abort();}
void mpColl_80041EE4(CollData* c){c->floor.index=c->ceiling.index=c->left_facing_wall.index=c->right_facing_wall.index=-1;}
void mpColl_SetECBSource_Fixed(CollData* c,HSD_GObj* g,float a,float b,float l,float r)
{CHECK(!g&&a==b&&a==l&&a==r);trace(Collision,allocations[current_side]-1,0,0,a,0,0,0);(void)c;}
bool mpColl_800471F8(CollData* c){(void)c;abort();}
bool mpColl_80048844(CollData* c){(void)c;abort();}
bool mpColl_8004B108(CollData* c){(void)c;abort();}
bool mpGetSpeed(int line,Vec3* p,Vec3* o){(void)line;(void)p;(void)o;abort();}
void it_80272F7C(HSD_JObj* j,float scale){CHECK(j==primary[current_side]);j->scale=(Vec3){scale,scale,scale};trace(Scale,0,0,0,scale,0,0,0);}
HSD_JObj* ftLib_80086630(HSD_GObj* g,Fighter_Part part){CHECK(part==FtPart_ThrowN);return ((Fighter*)g->user_data)->parts[part].joint;}
void it_80273168(Item_GObj* g){CHECK(g==&objects[current_side]);trace(PickupSound,0,0,0,0,0,0,0);}
void it_802742F4(Item_GObj* g,HSD_GObj* owner,Fighter_Part part)
{CHECK(g==&objects[current_side]&&owner==&fighters[current_side]&&part==4);item[current_side].owner=owner;item[current_side].xDC4=part;trace(Attach,part,0,0,0,0,0,0);++attachment_calls[current_side];}
void Item_8026B074(Item* i){CHECK(i==&item[current_side]);trace(PickupTail,0,0,0,0,0,0,0);}
void Item_80268E5C(Item_GObj* g,enum_t state,Item_StateChangeFlags flags)
{CHECK(g==&objects[current_side]&&state==0&&flags==ITEM_ANIM_UPDATE);item[current_side].msid=state;trace(State,state,flags,0,0,0,0,0);if(current_side==1&&retirement){unsigned action=retirement;retirement=0;invalidate(action);}}
static void update_registry(void){heads[HSD_GOBJ_PLINK_ITEM]=item_live[1]?&objects[1]:item_live[0]?&objects[0]:NULL;objects[1].next=item_live[0]?&objects[0]:NULL;}
void Item_8026A8EC(Item_GObj* g)
{CHECK(g==&objects[current_side]);if(current_side==1)nativeArticleResource_CancelGeneration(g,item[1].x1C);item_live[current_side]=false;update_registry();++remove_calls[current_side];trace(Remove,0,0,0,0,0,0,0);}
static void invalidate(unsigned action)
{
    if(action==1)Item_8026A8EC(&objects[1]);
    if(action==2){nativeArticleResource_CancelGeneration(&objects[1],item[1].x1C);++item[1].x1C;}
    if(action==3)heads[HSD_GOBJ_PLINK_FIGHTER]=&fighters[0];
    if(action==4)++fighter[1].x8_spawnNum;
    if(action==5)fighter[1].kind=Ft_Kind_Mario;
    if(action==6)active_definition=NULL;
}
#include "shared_math.inc"
#include "original.inc"
static ItemLogicTable stock_logic={.picked_up=itSamusGrapple_Logic53_PickedUp};
Item_GObj* Item_80268B18(SpawnItem* s)
{CHECK(s->kind==It_Kind_Samus_GBeam);trace(Spawn,s->kind,s->x3C_damage,s->x4_parent_gobj2!=NULL,s->pos.x,s->pos.y,s->pos.z,s->facing_dir);CHECK(s->vel.x==0&&s->vel.y==0&&s->vel.z==0&&s->x0_parent_gobj==&fighters[current_side]&&s->x4_parent_gobj2==&fighters[current_side]&&s->x44_flag.b0);CHECK(s->prev_pos.x==s->pos.x&&s->prev_pos.y==s->pos.y&&s->prev_pos.z==s->pos.z);if(fail_kind==4)return NULL;item_live[current_side]=true;update_registry();item[current_side].pos=s->pos;item[current_side].facing_dir=s->facing_dir;item[current_side].owner=s->x0_parent_gobj;if(current_side==0)itSamusGrapple_Logic53_Spawned(&objects[0]);else{CHECK(logic->on_created);logic->on_created(&objects[1]);}return &objects[current_side];}
static int32_t create_article(Fighter_GObj* g,ItemKind kind,Vec3* p,Vec3* v,float facing,bool center,uint32_t group,float value,bool collide)
{CHECK(g==&fighters[1]&&!center&&group==0&&value==0&&collide);SpawnItem s={.kind=kind,.pos=*p,.prev_pos=*p,.vel=*v,.facing_dir=facing,.x0_parent_gobj=g,.x4_parent_gobj2=g};s.x44_flag.b0=collide;uint32_t generation=(uint32_t)item[1].x1C;Item_GObj* result=Item_80268B18(&s);return result&&nativeCallbackSource_Alive(result,generation,NativeCallbackSource_Item)?(int32_t)generation:-1;}
static bool exists(HSD_GObj* g,uint32_t id,ItemKind kind){(void)g;return kind==It_Kind_Samus_GBeam&&nativeCallbackSource_Alive(&objects[1],id,NativeCallbackSource_Item);}
static bool signal_article(HSD_GObj* g,uint32_t id,uint32_t name,float value)
{if(!exists(g,id,It_Kind_Samus_GBeam))return false;for(unsigned n=0;n<logic->signal_count;++n)if(logic->signals[n].name_hash==name){logic->signals[n].callback(&objects[1],value);return true;}return false;}
static Item_GObj* find_created_item(Fighter_GObj* source,uint32_t handle)
{if(!nativeCallbackSource_Alive(&objects[1],handle,NativeCallbackSource_Item))return NULL;return nativeArticle_Creator(&objects[1])==source?&objects[1]:NULL;}
void db_80225DD8(Item_GObj* item_gobj,Fighter_GObj* source){(void)item_gobj;(void)source;abort();}
#include "attachment.inc"
static uint32_t owner_id(Item_GObj* g){Item* i=g->user_data;HSD_GObj* owner=i->owner;uint32_t id;return nativeCallbackSource_Snapshot(owner,NativeCallbackSource_Fighter,&id)?id:0;}
static float target_scale(uint32_t id){for(unsigned s=0;s<2;++s)if((uint32_t)fighter[s].x8_spawnNum==id)return fighter[s].x34_scale.y;return 1;}
static void joint_position(HSD_JObj* j,Vec3* offset,Vec3* out){CHECK(!offset&&j==&attachment[1]);*out=fighter[1].cur_pos;}
static unsigned bone_count(Fighter* f){(void)f;return Parts;}
static void trap(void){fprintf(stderr,"Unexpected Host boundary\n");abort();}
EF_SpawnResult efSync_Spawn(s32 gfx_id,HSD_GObj* g,...)
{(void)gfx_id;(void)g;abort();}
static NativeFighterHost host;

#include "model_animation.inc"
static void hosts(void)
{
#include "unused_host.h"
    host.bone_count=bone_count;host.joint_position=joint_position;host.callback_source_alive=nativeCallbackSource_Alive;host.item_creator=nativeArticle_Creator;
    host.create_article_at=create_article;host.article_ref_exists=exists;host.signal_article=signal_article;
    host.attach_created_item=nativeItem_Attach;host.item_current_owner=owner_id;host.target_instance_scale_y=target_scale;
    host.item_scale_by_attribute=it_8026BAE8;host.item_change_state_flags=Item_80268E5C;host.item_remove=Item_8026A8EC;
    host.article_resource_begin=nativeArticleResource_Begin;host.article_resource_create=nativeArticleResource_CreateNode;
    host.article_resource_load_model=nativeArticleResource_LoadModel;host.article_resource_bind_external_part=nativeArticleResource_BindExternalPart;
    host.article_resource_release=nativeArticleResource_Release;host.article_resource_add_animation=nativeArticleResource_AddAnimation;
    host.article_resource_request_animation=nativeArticleResource_RequestAnimation;
    host.item_add_model_animation=nativeArticleModel_AddAnimation;host.item_request_model_animation=nativeArticleModel_RequestAnimation;
}

static unsigned offset(const NativeFighterStateField* fields,unsigned count,const char* name)
{for(unsigned i=0;i<count;++i)if(!strcmp(fields[i].name,name))return fields[i].offset;fprintf(stderr,"Missing state %s\n",name);abort();}
static void state_int(const char* n,int value){memcpy(fighter[1].native_module_state+offset(definition->state_fields,definition->state_field_count,n),&value,4);}
static int state_read(const char* n){int v;memcpy(&v,fighter[1].native_module_state+offset(definition->state_fields,definition->state_field_count,n),4);return v;}
static int item_int(const char* n){int v;memcpy(&v,item[1].xDD4_itemVar.native_article.state+offset(logic->state_fields,logic->state_field_count,n),4);return v;}
static float item_float(const char* n){float v;memcpy(&v,item[1].xDD4_itemVar.native_article.state+offset(logic->state_fields,logic->state_field_count,n),4);return v;}
static void close_float(float a,float b){float error=fabsf(a-b);if(error>max_error)max_error=error;CHECK(isfinite(a)&&isfinite(b));if(error>2e-6f+2e-5f*fmaxf(fabsf(a),fabsf(b))){fprintf(stderr,"%.9g != %.9g\n",a,b);CHECK(false);}}
static void reset(int motion,float scale,float facing,unsigned sequence,bool grounded)
{
    current_side=1;nativeArticleResource_Clear();
    memset(nodes,0,sizeof(nodes));memset(links,0,sizeof(links));memset(models,0,sizeof(models));
    memset(node_live,0,sizeof(node_live));memset(created,0,sizeof(created));memset(allocations,0,sizeof(allocations));memset(loaded,0,sizeof(loaded));memset(freed,0,sizeof(freed));
    memset(events,0,sizeof(events));memset(event_count,0,sizeof(event_count));memset(attachment_calls,0,sizeof(attachment_calls));memset(remove_calls,0,sizeof(remove_calls));
    memset(fighter,0,sizeof(fighter));memset(item,0,sizeof(item));memset(parts,0,sizeof(parts));memset(heads,0,sizeof(heads));memset(primary,0,sizeof(primary));
    active_definition=definition;tables[Ft_Kind_Samus]=&part_table;fail_kind=0;fail_index=-1;retirement=0;
    for(unsigned s=0;s<2;++s){
        tuning[s]=retail;fighter[s]=(Fighter){.kind=Ft_Kind_Samus,.motion_id=motion,.x8_spawnNum=(s32)(0xFEDCBA10u+s),.parts=parts[s],.cur_pos={2,-3,4},.facing_dir=facing,.ground_or_air=grounded?GA_Ground:GA_Air,.x34_scale={scale,scale,scale}};
        fighter[s].u.ss.x2240=sequence;fighters[s]=(HSD_GObj){.user_data=&fighter[s]};
        parts[s][4].joint=parts[s][51].joint=&attachment[s];primary[s][0].child=&primary[s][1];primary[s][0].scale=(Vec3){1,1,1};
        item[s]=(Item){.kind=It_Kind_Samus_GBeam,.x1C=(u32)(0x81234567u+s),.xCC_item_attr=&common_attrs,.xC4_article_data=&article,.xB8_itemLogicTable=s?logic->logic:&stock_logic,.owner=&fighters[s],.native_article_author_kind=Ft_Kind_Samus,.scl=1,.msid=-1};
        objects[s]=(HSD_GObj){.user_data=&item[s],.hsd_obj=&primary[s][0]};item_live[s]=false;
        if(s){item[s].xDD4_itemVar.native_article.attachment_owner=&fighters[s];item[s].xDD4_itemVar.native_article.creator_spawn_id=(uint32_t)fighter[s].x8_spawnNum;}
    }
    fighters[1].next=&fighters[0];heads[HSD_GOBJ_PLINK_FIGHTER]=&fighters[1];
    state_int("tether_handle",-1);state_int("tether_code_sequence",sequence);update_registry();
}
static void compare(void)
{
    CHECK(item_live[0]==item_live[1]);CHECK(event_count[0]==event_count[1]);
    for(unsigned i=0;i<event_count[0];++i){CtorEvent* a=&events[0][i],*b=&events[1][i];if(a->kind!=b->kind)fprintf(stderr,"trace[%u] %u/%u\n",i,a->kind,b->kind);CHECK(a->kind==b->kind&&a->a==b->a&&a->b==b->b&&a->c==b->c);for(unsigned c=0;c<4;++c)close_float(a->f[c],b->f[c]);}
    CHECK(created[0]==created[1]&&loaded[0]==loaded[1]&&freed[0]==freed[1]);CHECK(attachment_calls[0]==attachment_calls[1]);
    if(!item_live[0]){CHECK(state_read("tether_handle")==-1);return;}
    CHECK(item_int("count")==tuning[0].x34&&item_int("endpoint")==tuning[0].x34-1);CHECK(item_int("modified_mode")==item[0].xDD4_itemVar.samusgrapple.x16);CHECK(item[0].xDD4_itemVar.samusgrapple.x15==item_int("count"));CHECK(fighter[0].u.ss.x2240==state_read("tether_code_sequence"));
    const char* names[]={"length","min_length","launch_speed","gravity","launch_auxiliary","retract_speed","retract_auxiliary","pull_speed","friction"};
    for(unsigned i=0;i<9;++i)close_float(((float*)&tuning[0].x38)[i],item_float(names[i]));
    close_float(item[0].scl,item[1].scl);CHECK(fighter[0].parts[139].joint==&models[0][tuning[0].x34-1][0]);CHECK(fighter[1].parts[139].joint==&models[1][tuning[0].x34-1][0]);CHECK(item[0].msid==item[1].msid);
    for(int n=0;n<tuning[0].x34;++n){CHECK(node_live[0][n]&&node_live[1][n]);CHECK(links[0][n].jobj==&attachment[0]&&links[1][n].jobj==&attachment[1]);CHECK(links[0][n].x1CC==-1&&links[1][n].x1CC==-1);CHECK(links[0][n].x2C_b0==links[1][n].x2C_b0&&links[0][n].x2C_b1==links[1][n].x2C_b1&&links[0][n].x2C_b2==links[1][n].x2C_b2);for(unsigned c=0;c<3;++c){close_float(((float*)&links[0][n].pos)[c],((float*)&links[1][n].pos)[c]);close_float(((float*)&links[0][n].vel)[c],((float*)&links[1][n].vel)[c]);close_float(((float*)&links[0][n].coll_data.cur_pos)[c],((float*)&links[1][n].coll_data.cur_pos)[c]);close_float(((float*)&links[0][n].coll_data.last_pos)[c],((float*)&links[1][n].coll_data.last_pos)[c]);}CHECK((n?links[0][n].next==&links[0][n-1]:links[0][n].next==NULL)&&(n?links[1][n].next==&links[1][n-1]:links[1][n].next==NULL));}
}
static uint8_t* read_file(const char* path,size_t* size){FILE* f=fopen(path,"rb");CHECK(f);fseek(f,0,SEEK_END);*size=ftell(f);rewind(f);uint8_t* p=malloc(*size+1);CHECK(p&&fread(p,1,*size,f)==*size);fclose(f);p[*size]=0;return p;}
static u32 local(const HSD_Dat* d,u32 slot){u32 target;const char* external;CHECK(HSD_DatResolve(d,slot,&target,&external)==HSD_DAT_REF_LOCAL);return target;}
#include "model_import.inc"
static void import_assets(const char* path)
{
    size_t size;dat_bytes=read_file(path,&size);HSD_Dat dat;CHECK(HSD_DatOpen(&dat,dat_bytes,size)==HSD_DAT_OK);u32 root;CHECK(HSD_DatFindRoot(&dat,"ftDataSamus",&root));u32 table=local(&dat,root+72),descriptor=local(&dat,table+12),special=local(&dat,descriptor+4);
    for(unsigned i=0;i<=96;i+=4){u32 value;CHECK(HSD_DatReadU32(&dat,special+i,&value));memcpy((uint8_t*)&retail+i,&value,4);}
    HSD_DatImport import;CHECK(HSD_DatImportInit(&import,&dat,arena,sizeof(arena),65536)==HSD_DAT_OK);article.native_special_attribute_source=special;
    CHECK(itArticle_ImportModelRoots(&import,&article,logic->model_roots,logic->model_root_count));CHECK(itArticle_ImportModelAnimations(&import,&article,logic->model_animations,logic->model_animation_count));
    HSD_Joint** roots[]={&retail.x64,&retail.x68,&retail.x6C,&retail.x70};const char* root_names[]={"ChainA","ChainB","ChainC","Endpoint"};
    for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)if(article.native_model_roots[i].name_hash==hash(root_names[j]))*roots[j]=article.native_model_roots[i].root;
    // Store pointer arrays consumed by the complete original bodies. These are
    // the same actual imported graph objects, not synthetic graph substitutes.
    static HSD_AnimJoint* joints[5];static HSD_MatAnimJoint* materials[5];
    const char* names[]={"ChainAAnimation","ChainBAnimation","ChainCAnimation","EndpointAnimation","PrimaryAnimation"};
    for(unsigned i=0;i<5;++i)for(unsigned j=0;j<article.native_model_animation_count;++j)if(article.native_model_animations[j].name_hash==hash(names[i])){joints[i]=article.native_model_animations[j].anim;materials[i]=article.native_model_animations[j].matanim;CHECK(!article.native_model_animations[j].shapeanim);}
    retail.x74=&joints[0];retail.x78=&materials[0];retail.x80=&joints[1];retail.x84=&materials[1];retail.x8C=&joints[2];retail.x90=&materials[2];retail.x98=&joints[3];retail.xA4=&joints[4];CHECK(retail.xC==30);common_attrs.x60_scale=1.25f;article.x4_specialAttributes=&tuning[0];
}
static void failure_matrix(void)
{
    // Defined GObj-allocation failure traces; freeing retains fixture pool bytes
    // so original post-free next read is observable without real freed-memory UB.
    for(int fail=0;fail<12;++fail){++scenario;reset(212,1,1,0,true);fail_kind=1;fail_index=fail;
        current_side=0;CHECK(!it_802B7C18(&fighters[0],&fighter[0].cur_pos,1));current_side=1;definition->on_load(&fighters[1]);compare();++pairs;}
    ++scenario;reset(212,1,1,0,true);fail_kind=4;current_side=0;CHECK(!it_802B7C18(&fighters[0],&fighter[0].cur_pos,1));current_side=1;definition->on_load(&fighters[1]);compare();++pairs;
}
static void safety_matrix(void)
{
    // Original NULL ItemLink/model/count boundaries are undefined; authored-only
    // assertions verify cleanup, no attachment and no lingering borrowed part.
    for(unsigned fault=0;fault<8;++fault){++scenario;reset(212,1,1,0,true);current_side=1;
        if(fault==0){fail_kind=2;fail_index=0;}
        if(fault==1){fail_kind=2;fail_index=4;}
        if(fault==2){fail_kind=3;fail_index=0;}
        if(fault==3){fail_kind=3;fail_index=4;}
        if(fault==4)parts[1][4].joint=NULL;
        if(fault==5)primary[1][0].child=NULL;
        if(fault==6)parts[1][139].joint=&attachment[1];
        if(fault==7)heads[HSD_GOBJ_PLINK_FIGHTER]=&fighters[0];
        definition->on_load(&fighters[1]);CHECK(!item_live[1]);CHECK(state_read("tether_handle")==-1);CHECK(!attachment_calls[1]);CHECK(freed[1]==allocations[1]-(fail_kind==2?1:0));for(unsigned n=0;n<Nodes;++n)CHECK(!node_live[1][n]);CHECK(parts[1][139].joint==(fault==6?&attachment[1]:NULL));++safety;
    }
    for(unsigned action=1;action<=6;++action){++scenario;reset(212,1,1,0,true);current_side=1;retirement=action;
        definition->on_load(&fighters[1]);CHECK(state_read("tether_handle")==-1);CHECK(!attachment_calls[1]);if(action<=2)CHECK(freed[1]==allocations[1]);else CHECK(!item_live[1]&&freed[1]==allocations[1]);++safety;
    }
}
static void healthy_matrix(void)
{
    const int motions[]={357,358,212,214,14};const float scales[]={.5f,1,1.5f,2};const unsigned sequences[]={0,3,4,7};
    for(unsigned m=0;m<5;++m)for(unsigned z=0;z<4;++z)for(unsigned seq=0;seq<4;++seq)for(unsigned ground=0;ground<2;++ground)for(unsigned facing=0;facing<2;++facing){
        ++scenario;reset(motions[m],scales[z],facing?-1:1,sequences[seq],ground);float coeff=motions[m]==357?retail.x60:retail.x5C;float length=retail.x10*scales[z];float value=scales[z]>1?retail.x8*(retail.xC*length)+(1-retail.x8)*(retail.xC*retail.x10):retail.xC*length;int count=(int)(value*coeff/length);if(sequences[seq]>=4&&ground)count*=2;if(motions[m]!=14&&(count<2||count>90))continue;
        current_side=0;Item_GObj* result=it_802B7C18(&fighters[0],&fighter[0].cur_pos,fighter[0].facing_dir);current_side=1;definition->on_load(&fighters[1]);CHECK((result!=NULL)==item_live[1]);compare();++pairs;
    }
}
int main(int argc,char** argv)
{
    if(argc!=4)return 2;HMODULE dll=LoadLibraryA(argv[1]);CHECK(dll);typedef const NativeFighterDefinition*(__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);Entry entry=(Entry)GetProcAddress(dll,"FreakMelee_GetFighterModule");CHECK(entry);typedef int(__cdecl* Bind)(const uint32_t*,unsigned,uint64_t);Bind bind=(Bind)GetProcAddress(dll,"FreakMelee_SetFighterParameters");CHECK(bind);size_t size;uint8_t* data=read_file(argv[3],&size);NativeFighterParameters params;unsigned line;CHECK(nativeFighterParameters_Parse((char*)data,size,&params,&line));uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned n=0;n<params.count;++n)bits[n]=params.fields[n].bits;CHECK(bind(bits,params.count,params.schema_hash));free(data);
    MotionState motions[400]={0};NativeFighterDefinition base={.motion_states=motions,.motion_state_count=400,.fighter_root="ftDataSamus"};base.special_ground_state[0]=343;base.special_air_state[0]=347;base.special_ground_state[1]=349;base.special_air_state[1]=351;base.special_ground_state[2]=353;base.special_air_state[2]=354;base.special_ground_state[3]=355;base.special_air_state[3]=356;hosts();definition=entry(&base,&host);CHECK(definition&&definition->on_load);active_definition=definition;for(unsigned n=0;n<definition->article_logic_count;++n)if(definition->article_logics[n].kind==It_Kind_Samus_GBeam)logic=&definition->article_logics[n];CHECK(logic&&logic->on_created&&logic->model_animation_count==5&&logic->local_state_bytes<=512);import_assets(argv[2]);healthy_matrix();failure_matrix();safety_matrix();current_side=1;nativeArticleResource_Clear();FreeLibrary(dll);free(dat_bytes);printf("{\"pairs\":%u,\"safety_cases\":%u,\"checks\":%u,\"max_error\":%.9g}\n",pairs,safety,checks,max_error);return 0;
}
