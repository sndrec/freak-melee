// Actual complete point kernel and resource lookup/retirement bodies. Only the
// bounded stage-query geometry, package publication and HSD free are controlled.
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/native_article.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/it/it_26B1.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ef/native_model_effect.h>
#include <melee/gr/grdynamicattr.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/lb/lbvector.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>

#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned checks,pairs,safety_cases,ecb_calls,free_count;
static HSD_GObj owner,creator,node_objects[2];static Item item;static Fighter fighter;
static ItemLink* links[2];static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
FighterPartsTable** ftPartsTable;
static ItemLogicTable item_logic;static NativeFighterArticleLogic authored;
static NativeFighterDefinition definition,replacement;static const NativeFighterDefinition* active_definition;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{return kind==Ft_Kind_Seak?active_definition:NULL;}
Fighter_GObj* nativeArticle_Creator(Item_GObj* object) {(void)object;return &creator;}
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* id){(void)g;(void)id;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t id){(void)g;(void)id;return NULL;}
DbLKind DbLevel=DbLKind_Master;int mpColl_804D64AC=10;
static bool (*mpColl_804D64A0)(Fighter_GObj*,int),(*original_mpColl_804D64A0)(Fighter_GObj*,int);
static Fighter_GObj *mpColl_804D64A4,*original_mpColl_804D64A4;
static bool mpColl_IsEcbTiny,original_mpColl_IsEcbTiny;
#define CollisionFlagAir_CanGrabLedge 0x4
void OSReport(char* format,...){(void)format;abort();}
void __assert(char* file,u32 line,char* condition){(void)file;(void)line;(void)condition;abort();}
bool ftLib_80086960(HSD_GObj* g){(void)g;abort();}
u8 ftLib_80086BE0(HSD_GObj* g){(void)g;abort();}
s32 ftLib_800874BC(HSD_GObj* g){(void)g;abort();}
ItemKind itGetKind(Item_GObj* g){(void)g;abort();}

// Four analytic, finite axis surfaces. Real mpLib topology/moving remap and
// intersection arithmetic are outside this fixture; the kernel and its stage
// result application/substep interpolation execute complete production bodies.
static unsigned surface_mask,query_limit;static bool resolution_fails,stage_callbacks,free_reenter;
static int dynamic_flags;static unsigned action;
typedef struct Trace {unsigned queries,remaps,bounds,unbounds,resolves,callbacks;float bound[4];int line,mode;float dy,weight;} Trace;
static Trace trace;
static void contact_callback(void*,int,CollData*,int,mpLib_GroundEnum,float);
static bool query(Vec3* output,int* line,int skip,int only,float ax,float ay,float bx,float by,bool remap) {
    ++trace.queries;if(remap)++trace.remaps;
    if(query_limit&&trace.queries>query_limit)return false;
    float best=2;int selected=-1;
    for(int i=0;i<4;++i) {
        if(!(surface_mask&(1u<<i))||skip==i||(only!=-1&&only!=i))continue;
        float a=i<2?ay:ax,b=i<2?by:bx,plane=i==0?0:i==1?10:i==2?-5:5;
        float d=b-a;if(d==0)continue;
        bool toward=(i==0||i==2)?d<0:d>0;
        if(!toward)continue;
        float t=(plane-a)/d;
        if(t>=0&&t<=1&&t<best){best=t;selected=i;}
    }
    if(selected<0)return false;
    *output=(Vec3){ax+(bx-ax)*best,ay+(by-ay)*best,0};*line=selected;return true;
}
bool mpCheckAll(Vec3* p,int* line,u32* flags,Vec3* normal,int skip,int only,float ax,float ay,float bx,float by)
{CHECK(flags==NULL&&normal==NULL);return query(p,line,skip,only,ax,ay,bx,by,false);}
bool mpCheckAllRemap(Vec3* p,int* line,u32* flags,Vec3* normal,int skip,int only,float ax,float ay,float bx,float by)
{CHECK(flags==NULL&&normal==NULL);return query(p,line,skip,only,ax,ay,bx,by,true);}
enum_t mpLineGetKind(int line)
{CHECK(line>=0&&line<4);static const int kinds[]={CollLine_Floor,CollLine_Ceiling,CollLine_LeftWall,CollLine_RightWall};return kinds[line];}
static int resolve(int line,Vec3* p,float* offset,u32* flags,Vec3* normal) {
    ++trace.resolves;trace.line=line;CHECK(line>=0&&line<4);
    float plane=line==0?0:line==1?10:line==2?-5:5;
    *offset=plane-(line<2?p->y:p->x);*flags=UINT32_C(0x81230000)+(unsigned)line;
    static const Vec3 normals[]={{0,1,0},{0,-1,0},{1,0,0},{-1,0,0}};*normal=normals[line];
    return resolution_fails?-1:line;
}
int mpLib_8004DD90_Floor(int l,Vec3* p,float* o,u32* f,Vec3* n){return resolve(l,p,o,f,n);}
int mpLib_8004E090_Ceiling(int l,Vec3* p,float* o,u32* f,Vec3* n){return resolve(l,p,o,f,n);}
int mpLib_8004E398_LeftWall(int l,Vec3* p,float* o,u32* f,Vec3* n){return resolve(l,p,o,f,n);}
int mpLib_8004E684_RightWall(int l,Vec3* p,float* o,u32* f,Vec3* n){return resolve(l,p,o,f,n);}
void mpBoundingCheck(float l,float b,float r,float t){++trace.bounds;trace.bound[0]=l;trace.bound[1]=b;trace.bound[2]=r;trace.bound[3]=t;}
void mpUncheckBounding(void){++trace.unbounds;}
int grDynamicAttr_801CA284(Vec3* p,int line){CHECK(p&&line>=0&&line<4);return dynamic_flags;}
int mpJointFromLine(int line){return stage_callbacks&&line>=0?line:-1;}
void mpJointGetCb1(int id,mpLib_JointCollisionCallback* cb,void** data){CHECK(id>=0);*cb=contact_callback;*data=&definition;}
void mpJointGetCb2(int id,mpLib_JointCollisionCallback* cb,void** data){CHECK(id>=0);*cb=contact_callback;*data=&definition;}
#include "vectors.inc"
#include "kernel_original.inc"
#include "kernel_current.inc"

// ECB sweep remains a separate controlled dispatch probe, not a substituted
// point kernel. Its sentinel detects a resource operation choosing the wrong API.
bool mpColl_800471F8(CollData* coll){++ecb_calls;coll->env_flags=Collide_FloorPush;coll->cur_pos.y+=17;coll->floor.index=0;if(stage_callbacks)mpCollEnd(coll,true,true);return true;}
void HSD_GObjFree(HSD_GObj* object) {
    CHECK(object==&node_objects[0]||object==&node_objects[1]);
    unsigned i=(unsigned)(object-node_objects);CHECK(object->user_data==links[i]);
    ++free_count;DWORD old;CHECK(VirtualProtect(links[i],4096,PAGE_NOACCESS,&old));object->user_data=NULL;
    if(free_reenter){CHECK(!nativeArticleResource_Begin(&owner,2));nativeArticleResource_Clear();}
}
#include "resource.inc"

static void contact_callback(void* data,int id,CollData* coll,int weight,mpLib_GroundEnum mode,float dy) {
    CHECK(data==&definition&&coll);++trace.callbacks;trace.mode=mode;trace.dy=dy;trace.weight=weight;
    unsigned selected=action;action=0;
    if(selected==1)nativeArticleResource_Release(&owner);
    if(selected==2)nativeArticleResource_CancelGeneration(&owner,item.x1C);
    if(selected==3)nativeArticleResource_CancelCreator(&creator,(uint32_t)fighter.x8_spawnNum);
    if(selected==4)nativeArticleResource_CancelDefinition(&definition);
    if(selected==5)nativeArticleResource_Clear();
    if(selected==6){uint32_t old=item.x1C;item.x1C++;nativeArticleResource_CancelGeneration(&owner,old);}
    if(selected==7){
        // Nested collision pins the same set. Its callback retires every node;
        // returning the inner call must not poison storage held by the outer.
        action=5;CHECK(nativeArticleResource_SweepPoint(&owner,1));
        CHECK(free_count==0);
    }
    if(selected==8){heads[HSD_GOBJ_PLINK_ITEM]=NULL;nativeArticleResource_CancelGeneration(&owner,item.x1C);}
    if(selected==9){action=5;CHECK(nativeArticleResource_SweepStage(&owner,1));CHECK(free_count==0);}
    if(selected){
        CHECK(resources.collections[0].releasing);
        CHECK(collection(&owner)==NULL);
        CHECK(!nativeArticleResource_Begin(&owner,2));
        CHECK(!nativeArticleResource_SweepPoint(&owner,0));
        CHECK(!nativeArticleResource_SweepStage(&owner,0));
        CHECK(free_count==0);CHECK(isfinite(coll->cur_pos.y));
    }
    (void)id;
}
static void vec_equal(Vec3 a,Vec3 b){CHECK(a.x==b.x);CHECK(a.y==b.y);CHECK(a.z==b.z);}
static void ecb_equal(ftECB a,ftECB b){CHECK(a.top.x==b.top.x&&a.top.y==b.top.y);CHECK(a.bottom.x==b.bottom.x&&a.bottom.y==b.bottom.y);CHECK(a.left.x==b.left.x&&a.left.y==b.left.y);CHECK(a.right.x==b.right.x&&a.right.y==b.right.y);}
static void surface_equal(SurfaceData a,SurfaceData b){CHECK(a.index==b.index);CHECK(a.flags==b.flags);vec_equal(a.normal,b.normal);}
static void same(CollData* a,CollData* b){
    vec_equal(a->cur_pos,b->cur_pos);vec_equal(a->last_pos,b->last_pos);vec_equal(a->prev_pos,b->prev_pos);vec_equal(a->x28_vec,b->x28_vec);vec_equal(a->contact,b->contact);
    CHECK(a->env_flags==b->env_flags&&a->prev_env_flags==b->prev_env_flags&&a->x38==b->x38);
    CHECK(a->x34_flags.b5==b->x34_flags.b5&&a->x34_flags.b6==b->x34_flags.b6&&a->x34_flags.b1234==b->x34_flags.b1234);
    ecb_equal(a->ecb,b->ecb);ecb_equal(a->prev_ecb,b->prev_ecb);ecb_equal(a->desired_ecb,b->desired_ecb);ecb_equal(a->x64_ecb,b->x64_ecb);
    surface_equal(a->floor,b->floor);surface_equal(a->ceiling,b->ceiling);surface_equal(a->left_facing_wall,b->left_facing_wall);surface_equal(a->right_facing_wall,b->right_facing_wall);
}
static CollData initial(int line,float distance,unsigned flags) {
    CollData c={0};c.x0_gobj=&owner;c.last_pos=(Vec3){0,5,7};c.cur_pos=c.last_pos;
    if(line==0)c.cur_pos.y-=distance;else if(line==1)c.cur_pos.y+=distance;else if(line==2)c.cur_pos.x-=distance;else c.cur_pos.x+=distance;
    c.cur_pos.z=11;c.prev_pos=(Vec3){13,14,15};c.x28_vec=(Vec3){16,17,18};c.contact=(Vec3){19,20,21};
    c.floor.index=c.ceiling.index=c.left_facing_wall.index=c.right_facing_wall.index=-1;
    c.floor.normal=(Vec3){.6f,.8f,0};c.ceiling.normal=(Vec3){.1f,-.9f,0};c.left_facing_wall.normal=(Vec3){.9f,.1f,0};c.right_facing_wall.normal=(Vec3){-.9f,.1f,0};
    c.env_flags=(s32)UINT32_C(0x81234567);c.prev_env_flags=97;c.x38=(flags&1)?mpColl_804D64AC:mpColl_804D64AC-1;
    c.joint_id_skip=c.joint_id_only=-1;c.x50=71.5f;c.x34_flags.b1234=(flags>>1)&15;c.x34_flags.b6=(flags>>5)&1;
    c.ecb=(ftECB){.left={-1,1},.right={1,1},.bottom={0,0},.top={0,2}};
    c.desired_ecb=c.ecb;c.desired_ecb.top.y=(flags&64)?12:2;c.x64_ecb=c.ecb;c.x64_ecb.left.x=-2;
    c.ecb_source.kind=ECBSource_Fixed;c.ecb_source.x10C_joint[0]=(HSD_JObj*)(uintptr_t)UINT32_C(0xFEDCBA98);
    return c;
}
static void reset_resource(void) {
    memset(&resources,0,sizeof(resources));memset(&trace,0,sizeof(trace));free_count=ecb_calls=0;action=0;
    memset(&fighter,0,sizeof(fighter));fighter.kind=Ft_Kind_Seak;fighter.x8_spawnNum=UINT32_C(0x87654321);
    creator=(HSD_GObj){.user_data=&fighter};authored=(NativeFighterArticleLogic){.kind=It_Kind_Seak_Chain,.logic=&item_logic,.resource_capacity=2,.resource_reserved_part=-1};
    definition=(NativeFighterDefinition){.article_logics=&authored,.article_logic_count=1};replacement=definition;active_definition=&definition;
    item=(Item){.kind=It_Kind_Seak_Chain,.x1C=UINT32_C(0xfedcba98),.native_article_author_kind=Ft_Kind_Seak,.xB8_itemLogicTable=&item_logic};
    owner=(HSD_GObj){.user_data=&item};heads[HSD_GOBJ_PLINK_ITEM]=&owner;heads[HSD_GOBJ_PLINK_FIGHTER]=&creator;
    CHECK(nativeArticleResource_Begin(&owner,2));ResourceCollection* set=&resources.collections[0];
    for(unsigned i=0;i<2;++i){DWORD old;CHECK(VirtualProtect(links[i],4096,PAGE_READWRITE,&old));memset(links[i],0,sizeof(*links[i]));node_objects[i]=(HSD_GObj){.user_data=links[i]};links[i]->gobj=&node_objects[i];links[i]->coll_data=initial(0,18,0);set->nodes[i]=(ResourceNode){.link=links[i],.generation=set->generation};}
    set->created=2;surface_mask=1;query_limit=0;resolution_fails=false;stage_callbacks=false;free_reenter=false;dynamic_flags=0;
}
int main(void) {
    for(unsigned i=0;i<2;++i){links[i]=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(links[i]);}
    reset_resource();
    for(int line=0;line<4;++line)for(unsigned flags=0;flags<128;++flags)for(int amount=0;amount<7;++amount)for(int miss=0;miss<2;++miss) {
        float distance=(float[]){0,2,5,6,6.1f,18,97}[amount];CollData a=initial(line,distance,flags),b=a;
        surface_mask=1u<<line;resolution_fails=miss;dynamic_flags=(flags&8)?0x45:0;stage_callbacks=(flags&4)!=0;action=0;
        trace=(Trace){0};bool old=original_mpColl_80048844(&a);Trace old_trace=trace;
        trace=(Trace){0};bool current=mpColl_80048844(&b);CHECK(old==current);same(&a,&b);CHECK(!memcmp(&old_trace,&trace,sizeof(trace)));
        CHECK(b.x28_vec.x==initial(line,distance,flags).cur_pos.x);CHECK(b.prev_env_flags==(s32)UINT32_C(0x81234567));
        CHECK(b.ecb_source.x10C_joint[0]==(HSD_JObj*)(uintptr_t)UINT32_C(0xFEDCBA98));
        CHECK(trace.bounds==trace.unbounds&&trace.queries>=1);++pairs;
    }
    // Independent contract anchors prevent equal empty kernels passing parity.
    for(int line=0;line<4;++line){CollData c=initial(line,18,0);surface_mask=1u<<line;resolution_fails=false;stage_callbacks=false;dynamic_flags=0;trace=(Trace){0};CHECK(mpColl_80048844(&c));static const int masks[]={Collide_FloorPush,Collide_CeilingPush,Collide_LeftWallPush,Collide_RightWallPush};CHECK(c.env_flags==masks[line]);CHECK(trace.queries==4&&trace.remaps==1&&trace.resolves>=1);if(line<2)CHECK(c.cur_pos.y==(line?10:0));else CHECK(c.cur_pos.x==(line==2?-5:5));}
    // The original kernel returns the final substep result, even when an
    // earlier contact remains in env_flags. Do not replace it with any-hit.
    {CollData c=initial(0,97,0);surface_mask=1;query_limit=1;trace=(Trace){0};CHECK(!mpColl_80048844(&c));CHECK(c.env_flags==Collide_FloorPush&&trace.queries==17);query_limit=0;}
    for(int filter=0;filter<3;++filter){CollData c=initial(0,18,0);surface_mask=filter==2?0:1;if(filter==0)c.joint_id_skip=0;if(filter==1)c.joint_id_only=1;trace=(Trace){0};CHECK(!mpColl_80048844(&c));CHECK(c.env_flags==0&&trace.resolves==0);}
    reset_resource();CHECK(nativeArticleResource_SweepPoint(&owner,0));CHECK(ecb_calls==0);CHECK(links[0]->coll_data.floor.index==0);CHECK(links[0]->coll_data.env_flags==Collide_FloorPush);
    reset_resource();CHECK(nativeArticleResource_SweepStage(&owner,0));CHECK(ecb_calls==1&&links[0]->coll_data.cur_pos.y==4);
    for(unsigned fail=0;fail<8;++fail){reset_resource();ResourceCollection* set=&resources.collections[0];
        if(fail==0)heads[HSD_GOBJ_PLINK_ITEM]=NULL;if(fail==1)item.x1C++;if(fail==2)active_definition=&replacement;if(fail==3)item.kind=It_Kind_Seak_NeedleThrow;
        if(fail==4)item.xB8_itemLogicTable=NULL;if(fail==5)set->nodes[0].generation++;if(fail==6)set->nodes[0].link=NULL;if(fail==7)set->releasing=true;
        trace=(Trace){0};CHECK(!nativeArticleResource_SweepPoint(&owner,0));CHECK(!nativeArticleResource_SweepStage(&owner,0));CHECK(trace.queries==0&&ecb_calls==0);++safety_cases;}
    reset_resource();CHECK(!nativeArticleResource_SweepPoint(&owner,2));CHECK(!nativeArticleResource_SweepPoint(&owner,UINT32_MAX));CHECK(!nativeArticleResource_SweepPoint(NULL,0));
    for(unsigned outer_stage=0;outer_stage<2;++outer_stage)for(unsigned retirement=1;retirement<=9;++retirement){reset_resource();stage_callbacks=true;free_reenter=true;action=retirement;uint32_t prior=resources.collections[0].generation;CHECK(outer_stage?nativeArticleResource_SweepStage(&owner,0):nativeArticleResource_SweepPoint(&owner,0));CHECK(free_count==2);CHECK(resources.collections[0].owner==NULL);CHECK(!nativeArticleResource_SweepPoint(&owner,0));if(retirement!=8){CHECK(nativeArticleResource_Begin(&owner,2));CHECK(resources.collections[0].generation!=prior);}++safety_cases;}
    reset_resource();resources.collections[0].active_operations=UINT16_MAX;CHECK(!nativeArticleResource_SweepPoint(&owner,0));CHECK(!nativeArticleResource_SweepStage(&owner,0));CHECK(trace.queries==0&&ecb_calls==0);++safety_cases;
    printf("PASS %u original/current swept-point pairs, %u checks, %u resource identity/retirement safety cases; controlled analytic stage queries and allocation\n",pairs,checks,safety_cases);
    for(unsigned i=0;i<2;++i)CHECK(VirtualFree(links[i],0,MEM_RELEASE));return 0;
}
