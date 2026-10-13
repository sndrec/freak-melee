/* Actual complete rope solver bodies versus generated resource policy.
 * Allocation, attachment transforms and model renderer are separate gates. */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/fighter.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/itCharItems.h>
#include <melee/it/kinds/itclimbersstring.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
static unsigned checks,pairs,count;
#define CHECK(v) do { ++checks;if(!(v)){fprintf(stderr,"pair%u line%d %s\n",pairs,__LINE__,#v);exit(1);} }while(0)
static Fighter fighter;
static Item item;
static Article article;
static HSD_GObj object,creator,*heads[32];
HSD_GObj**HSD_GObjPLinkHead=heads;
static ItemLink links[40],expected[40];
static itClimbersStringAttributes attributes;
static Vec3 anchor;
static HSD_GObj link_objects[40];
static HSD_JObj models[40],attachment;
static Mtx expected_matrices[40];
static Vec3 expected_translations[40];
static u32 expected_flags[40];
static float max_difference;
bool nativeModelEffect_Snapshot(HSD_GObj*g,uint32_t*id){(void)g;*id=0;return false;}
EF_Effect*nativeModelEffect_Resolve(HSD_GObj*g,uint32_t id){(void)g;(void)id;return NULL;}
static HSD_GObj* original_creator(Item_GObj*g){CHECK(g==&object);return &creator;}
static float read_resource(Item_GObj*g,unsigned index,NativeArticleResourceFloat channel)
{
    CHECK(g==&object&&index<count&&channel<6);Vec3*v=channel<3?&links[index].pos:&links[index].vel;
    return (&v->x)[channel%3];
}
static bool write_resource(Item_GObj*g,unsigned index,NativeArticleResourceFloat channel,float value)
{
    CHECK(g==&object&&index<count&&channel<6&&isfinite(value));Vec3*v=channel<3?&links[index].pos:&links[index].vel;
    (&v->x)[channel%3]=value;return true;
}
static bool draw_resource(Item_GObj*g,unsigned index,bool drawing)
{CHECK(g==&object&&index<count);CHECK(drawing||index!=count-1);return true;}
/* Shared affine/cross and HSD materialization are controlled boundaries.
 * The full original render/orthonormal policy bodies remain extracted. */
void C_MTXIdentity(Mtx m){memset(m,0,sizeof(Mtx));m[0][0]=m[1][1]=m[2][2]=1;}
void C_MTXCopy(Mtx a,Mtx b){memcpy(b,a,sizeof(Mtx));}
void C_MTXConcat(Mtx a,Mtx b,Mtx output)
{
    Mtx m;
    for(unsigned i=0;i<3;++i){
        for(unsigned j=0;j<3;++j)m[i][j]=a[i][0]*b[0][j]+a[i][1]*b[1][j]+a[i][2]*b[2][j];
        m[i][3]=a[i][0]*b[0][3]+a[i][1]*b[1][3]+a[i][2]*b[2][3]+a[i][3];
    }
    C_MTXCopy(m,output);
}
void C_VECCrossProduct(Vec*a,Vec*b,Vec*r)
{*r=(Vec){a->y*b->z-a->z*b->y,a->z*b->x-a->x*b->z,a->x*b->y-a->y*b->x};}
void HSD_JObjSetupMatrixSub(HSD_JObj*j){CHECK(j==&attachment);j->flags&=~JOBJ_MTX_DIRTY;}
void HSD_JObjSetMtxDirtySub(HSD_JObj*j){j->flags|=JOBJ_MTX_DIRTY;}
void __assert(char*file,u32 line,char*message){fprintf(stderr,"%s:%lu %s\n",file,line,message);exit(2);}
static bool attachment_matrix(Item_GObj*g,unsigned node,Mtx m)
{CHECK(g==&object&&node<count);C_MTXCopy(attachment.mtx,m);return true;}
static bool world_matrix(Item_GObj*g,unsigned node,const Mtx m)
{
    CHECK(g==&object&&node<count);HSD_JObjCopyMtx(&models[node],(MtxPtr)m);
    models[node].flags|=0x03800000;HSD_JObjSetMtxDirty(&models[node]);return true;
}
static void write_joint(Item_GObj*g,unsigned node,const char*path,unsigned channel,float v)
{
    CHECK(g==&object&&node<count&&!*path&&channel<3);
    Vec3 position=models[node].translate;(&position.x)[channel]=v;
    HSD_JObjSetTranslate(&models[node],&position);
}
#include "generated.inc"
#include "parameters.inc"
#include "aliases.inc"
#include "solver_actual.inc"
static unsigned field(const char*name)
{
    for(unsigned i=0;i<sizeof(scripted_article_2_state_fields)/sizeof(*scripted_article_2_state_fields);++i)
        if(!strcmp(name,scripted_article_2_state_fields[i].name))return scripted_article_2_state_fields[i].offset;
    CHECK(false);return 0;
}
static unsigned fighter_field(const char*name)
{
    for(unsigned i=0;i<sizeof(authored_state_fields)/sizeof(*authored_state_fields);++i)
        if(!strcmp(name,authored_state_fields[i].name))return authored_state_fields[i].offset;
    CHECK(false);return 0;
}
static void setf(const char*n,float v){memcpy(item.xDD4_itemVar.native_article.state+field(n),&v,4);}
static void seti(const char*n,int32_t v){memcpy(item.xDD4_itemVar.native_article.state+field(n),&v,4);}
static void setb(unsigned i,bool v){item.xDD4_itemVar.native_article.state[field("active")+i]=v;}
static bool getb(unsigned i){return item.xDD4_itemVar.native_article.state[field("active")+i]!=0;}
static void owner_float(const char*n,float v){memcpy(fighter.native_module_state+fighter_field(n),&v,4);}
static void reset(bool generated,unsigned first,unsigned scenario)
{
    memset(&fighter,0,sizeof fighter);memset(&item,0,sizeof item);memset(&object,0,sizeof object);
    memset(&creator,0,sizeof creator);memset(links,0,sizeof links);memset(heads,0,sizeof heads);
    memset(link_objects,0,sizeof link_objects);memset(models,0,sizeof models);memset(&attachment,0,sizeof attachment);
    object.user_data=&item;object.classifier=HSD_GOBJ_CLASS_ITEM;item.entity=&object;
    item.x1C=UINT32_C(0xf0edcba9);item.kind=It_Kind_IceClimber_GumStrings;item.owner=&creator;
    heads[HSD_GOBJ_PLINK_ITEM]=&object;heads[HSD_GOBJ_PLINK_FIGHTER]=&creator;
    creator.user_data=&fighter;creator.classifier=HSD_GOBJ_CLASS_FIGHTER;
    fighter.kind=generated?Ft_Kind_CustomFirst:Ft_Kind_Popo;fighter.x8_spawnNum=UINT32_C(0xfedcba98);
    fighter.gobj=&creator;item.xC4_article_data=&article;article.x4_specialAttributes=&attributes;
    fighter.u.pp.x2240=(Vec3){scenario&1?6.f:0,scenario&2?2.f:0,scenario&4?.625f:0};
    anchor=(Vec3){scenario&8?16.f:2.f,1.25f,.75f};
    C_MTXIdentity(attachment.mtx);attachment.mtx[0][0]=.8f;attachment.mtx[1][1]=1.2f;
    attachment.mtx[2][2]=.9f;attachment.mtx[0][3]=anchor.x;attachment.mtx[1][3]=anchor.y;attachment.mtx[2][3]=anchor.z;
    for(unsigned i=0;i<count;++i){
        links[i].prev=i+1<count?&links[i+1]:NULL;links[i].next=i?&links[i-1]:NULL;
        links[i].pos=(Vec3){(float)i*.325f,(float)(i%3)*.625f,(float)(i%2)*.125f};
        links[i].vel=(Vec3){.125f*(float)((int)i%4-2),-.25f*(float)((int)i%3-1),.0625f};
        links[i].x2C_b0=i>=first;
        links[i].jobj=&attachment;links[i].gobj=&link_objects[i];link_objects[i].hsd_obj=&models[i];
        C_MTXIdentity(models[i].mtx);
    }
    if(generated){
        item.native_article_author_kind=fighter.kind;item.xDD4_itemVar.native_article.attachment_owner=&creator;
        item.xDD4_itemVar.native_article.creator_spawn_id=(uint32_t)fighter.x8_spawnNum;
        seti("tail",count-1);setf("anchor_x",anchor.x);setf("anchor_y",anchor.y);setf("anchor_z",anchor.z);
        setf("retract_distance",scenario&16?2.25f:.5f);
        for(unsigned i=0;i<count;++i)setb(i,links[i].x2C_b0);
        owner_float("linked_rope_endpoint_x",fighter.u.pp.x2240.x);
        owner_float("linked_rope_endpoint_y",fighter.u.pp.x2240.y);
        owner_float("linked_rope_endpoint_z",fighter.u.pp.x2240.z);
    }else{
        item.xDD4_itemVar.climbersstring.xC=&creator;
        item.xDD4_itemVar.climbersstring.x4=&links[count-1];item.xDD4_itemVar.climbersstring.x8=links;
    }
}
static void eq(float a,float b)
{
    float difference=fabsf(a-b);if(difference>max_difference)max_difference=difference;
    if(memcmp(&a,&b,4))fprintf(stderr,"float original%.9g generated%.9g diff%.9g\n",a,b,difference);
    CHECK(!memcmp(&a,&b,4));
}
static void compare(void)
{
    ++pairs;
    for(unsigned i=0;i<count;++i){
        for(unsigned c=0;c<3;++c){eq((&expected[i].pos.x)[c],(&links[i].pos.x)[c]);eq((&expected[i].vel.x)[c],(&links[i].vel.x)[c]);}
        CHECK(expected[i].x2C_b0==getb(i));
    }
}
int main(void)
{
    NativeFighterHost host={0};host.callback_source_alive=nativeCallbackSource_Alive;host.sqrt_float=sqrtf;
    host.item_creator=original_creator;host.article_resource_read_float=read_resource;
    host.article_resource_write_float=write_resource;host.article_resource_set_draw=draw_resource;
    host.article_resource_set_world_matrix=world_matrix;host.article_resource_read_attachment_matrix=attachment_matrix;
    host.article_resource_write_joint=write_joint;engine=&host;
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/sizeof(*parameter_bits),parameter_schema));
#include "attributes.inc"
    const unsigned counts[]={2,3,6,20,40};
    for(unsigned c=0;c<5;++c){count=counts[c];attributes.x0_count=count;
        for(unsigned first=0;first<count;++first)for(unsigned scenario=0;scenario<32;++scenario)for(unsigned operation=0;operation<2;++operation){
            for(unsigned generated=0;generated<2;++generated){reset(generated,first,scenario);
                if(operation==0){if(generated)rope_Render(&object);else it_802C3520(&item,&anchor);}
                else{if(generated)rope_Fold(&object);else it_802C33B8(&item);}
                if(generated){++pairs;for(unsigned i=0;i<count;++i){
                    for(unsigned r=0;r<3;++r){eq((&expected_translations[i].x)[r],(&models[i].translate.x)[r]);
                        for(unsigned column=0;column<4;++column)eq(expected_matrices[i][r][column],models[i].mtx[r][column]);}
                    CHECK(expected_flags[i]==models[i].flags);CHECK(expected[i].x2C_b0==getb(i));
                }}else{memcpy(expected,links,sizeof links);for(unsigned i=0;i<count;++i){
                    memcpy(expected_matrices[i],models[i].mtx,sizeof(Mtx));expected_translations[i]=models[i].translate;expected_flags[i]=models[i].flags;
                }}
            }
        }
    }
    printf("PASS %u rope render pairs, %u checks, maximum_float_difference %.9g\n",pairs,checks,max_difference);
    return 0;
}
