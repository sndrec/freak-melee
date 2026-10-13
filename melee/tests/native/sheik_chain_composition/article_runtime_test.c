/* Actual original article entry/accessory/render/owner cleanup bodies compared
 * with generated callbacks. Matrix/joint/resource/notification engine boundaries
 * are controlled; this does not accept retail model import or draw behavior. */
#include "article_harness.h"
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftlib.h>
#include <melee/it/it_26B1.h>
#include <melee/it/inlines.h>
#include <melee/it/it_2725.h>
#include <melee/lb/lbvector.h>
#include <melee/lb/lbcollision.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjgxlink.h>
static HSD_GObj link_objects[30];static HSD_JObj joints[30],attachment;
static const Vec3 it_803B868C={0,0,.1f};
static unsigned endpoints[128],endpoint_count,released,removed,notified,paused,resumed;
static int notification_mutation,transition_mutation;
static bool constructor_mode,primary_hidden;static unsigned allocation_index,pool_created,loaded_models[30],attached;static int fail_creation,fail_model;
static HSD_JObj primary;static HSD_Joint link_recipe,tip_recipe;static FighterBone parts[58];
static const Vec3 it_803B8680={0};
HSD_ObjAllocData item_link_alloc_data;u8 HSD_GObj_JObjKind;
static float length(const Vec3*v){return sqrtf(v->x*v->x+v->y*v->y+v->z*v->z);}
void C_MTXIdentity(Mtx m){memset(m,0,sizeof(Mtx));m[0][0]=m[1][1]=m[2][2]=1;}
void C_MTXCopy(Mtx a,Mtx b){memcpy(b,a,sizeof(Mtx));}
void C_MTXConcat(Mtx a,Mtx b,Mtx result)
{Mtx m;for(unsigned i=0;i<3;++i){for(unsigned j=0;j<3;++j)m[i][j]=a[i][0]*b[0][j]+a[i][1]*b[1][j]+a[i][2]*b[2][j];m[i][3]=a[i][0]*b[0][3]+a[i][1]*b[1][3]+a[i][2]*b[2][3]+a[i][3];}C_MTXCopy(m,result);}
void C_VECCrossProduct(Vec*a,Vec*b,Vec*r){*r=(Vec){a->y*b->z-a->z*b->y,a->z*b->x-a->x*b->z,a->x*b->y-a->y*b->x};}

void HSD_JObjSetupMatrixSub(HSD_JObj*j){CHECK(j==&attachment);j->flags&=~JOBJ_MTX_DIRTY;}
void HSD_JObjSetMtxDirtySub(HSD_JObj*j){j->flags|=JOBJ_MTX_DIRTY;}
void __assert(char*file,u32 line,char*message){fprintf(stderr,"assert %s:%lu %s\n",file,line,message);exit(2);}
void ftColl_8007B8A8(HitCapsule*h,Vec3*p)
{CHECK(h>=fighter.x914&&h<fighter.x914+4&&endpoint_count<128);endpoints[endpoint_count++]=(unsigned)(h-fighter.x914);h->x4C=*p;}
void ftColl_8007AFF8(HSD_GObj*g){CHECK(g==&creator);for(unsigned i=0;i<4;++i)fighter.x914[i].state=HitCapsule_Disabled;fighter.x2219_b3=false;}
void Item_80268E5C(HSD_GObj*g,enum_t motion,Item_StateChangeFlags flags)
{CHECK(g==&object&&flags==ITEM_ANIM_UPDATE);item.msid=motion;if(transition_mutation==1)heads[HSD_GOBJ_PLINK_ITEM]=NULL;else if(transition_mutation==2)++item.x1C;}
void it_8026BB68(HSD_GObj*g,Vec3*p){CHECK(g==&creator);*p=fighter.cur_pos;p->y+=.5f*(fighter.coll_data.ecb.top.y+fighter.coll_data.ecb.bottom.y);}
void it_8026B724(HSD_GObj*g){CHECK(g==&object);++paused;item.xDC8_word.flags.x3=true;}
void it_8026B73C(HSD_GObj*g){CHECK(g==&object);++resumed;item.xDC8_word.flags.x3=false;if(item.xDC8_word.flags.x7)item.xDC8_word.flags.x5=true;}
void it_802BAF0C(HSD_GObj*g){it_8026B73C(g);}
void HSD_GObjFree(HSD_GObj*g){CHECK(g>=link_objects&&g<link_objects+count);++released;}
void Item_8026A8EC(HSD_GObj*g){CHECK(g==&object);++removed;heads[HSD_GOBJ_PLINK_ITEM]=NULL;}
enum_t ftLib_GetMotionId(HSD_GObj*g){CHECK(g==&creator);return fighter.motion_id;}
bool it_8026B894(HSD_GObj*g,HSD_GObj*other){CHECK(g==&object);if(item.owner==other)item.owner=NULL;return true;}
static bool interact(HSD_GObj*g,HSD_GObj*other){it_8026B894(g,other);return true;}
static uint32_t owner_id(HSD_GObj*g)
{CHECK(g==&object);for(HSD_GObj*c=heads[HSD_GOBJ_PLINK_FIGHTER];c;c=c->next)if(c==item.owner&&c->user_data&&c->classifier==HSD_GOBJ_CLASS_FIGHTER)return ((Fighter*)c->user_data)->x8_spawnNum;return UINT32_MAX;}
static int32_t motion(uint32_t id){return nativeCallbackSource_Alive(&creator,id,NativeCallbackSource_Fighter)?fighter.motion_id:-1;}
static bool notify(uint32_t id,uint32_t hash)
{CHECK(hash==UINT32_C(278448985));if(!nativeCallbackSource_Alive(&creator,id,NativeCallbackSource_Fighter))return false;++notified;script_fn_chain_lost(&creator);if(notification_mutation==1)heads[HSD_GOBJ_PLINK_ITEM]=NULL;else if(notification_mutation==2)++item.x1C;return true;}
static bool signal(HSD_GObj*g,uint32_t id,uint32_t hash,float value)
{CHECK(g==&creator&&id==item.x1C&&value==0&&hash==UINT32_C(4281967466));it_8026B73C(&object);return true;}
static void release_resources(HSD_GObj*g){CHECK(g==&object);released+=constructor_mode?pool_created:count;pool_created=0;count=0;}
static bool world_matrix(HSD_GObj*g,unsigned node,const Mtx m)
{CHECK(g==&object&&node<count);memcpy(joints[node].mtx,m,sizeof(Mtx));joints[node].flags|=JOBJ_USER_DEF_MTX|JOBJ_MTX_INDEP_PARENT|JOBJ_MTX_INDEP_SRT;HSD_JObjSetMtxDirty(&joints[node]);return true;}
static bool attachment_matrix(HSD_GObj*g,unsigned node,Mtx m){CHECK(g==&object&&node<count);C_MTXCopy(attachment.mtx,m);return true;}
static bool attachment_point(HSD_GObj*g,unsigned node,const Vec3*offset,Vec3*result)
{CHECK(g==&object&&node<count);*result=(Vec3){attachment.mtx[0][0]*offset->x+attachment.mtx[0][1]*offset->y+attachment.mtx[0][2]*offset->z+attachment.mtx[0][3],attachment.mtx[1][0]*offset->x+attachment.mtx[1][1]*offset->y+attachment.mtx[1][2]*offset->z+attachment.mtx[1][3],attachment.mtx[2][0]*offset->x+attachment.mtx[2][1]*offset->y+attachment.mtx[2][2]*offset->z+attachment.mtx[2][3]};return true;}
static void write_joint(HSD_GObj*g,unsigned node,const char*path,unsigned channel,float value)
{CHECK(g==&object&&node<count&&!*path&&channel<3);if(channel==0)joints[node].translate.x=value;else if(channel==1)joints[node].translate.y=value;else joints[node].translate.z=value;}
void it_802BCB88(Item*,Vec3*);
#include "article_runtime_actual.inc"
static void runtime_reset(bool generated,unsigned first,unsigned scenario)
{
    reset(generated,first,scenario);memset(joints,0,sizeof joints);memset(link_objects,0,sizeof link_objects);memset(&attachment,0,sizeof attachment);C_MTXIdentity(attachment.mtx);attachment.mtx[0][0]=.8f;attachment.mtx[1][1]=1.2f;attachment.mtx[2][2]=.9f;attachment.mtx[0][3]=anchor.x;attachment.mtx[1][3]=anchor.y;attachment.mtx[2][3]=anchor.z-.09f;
    for(unsigned i=0;i<count;++i){links[i].jobj=&attachment;links[i].gobj=&link_objects[i];link_objects[i].hsd_obj=&joints[i];joints[i].flags=JOBJ_MTX_DIRTY;C_MTXIdentity(joints[i].mtx);}
    endpoint_count=released=removed=notified=paused=resumed=0;notification_mutation=transition_mutation=0;fighter.cmd_vars[0]=scenario&4?1:0;fighter.motion_id=350;fighter.cur_pos=(Vec3){1,2,3};fighter.coll_data.ecb.top.y=3;fighter.coll_data.ecb.bottom.y=-1;
    fighter.u.sk.x8=&object;script_state_set_int(&fighter,24,(int32_t)item.x1C);script_state_set_int(&fighter,52,3);fighter.mv.sk.specials.x1C=3;
    if(!generated)item.xDD4_itemVar.seakchain.parent_gobj=&creator;
}
/* Constructor dependencies are real typed declarations with controlled fixed
 * allocations and model attachment. No retail asset/import claim is made. */
void mpColl_80041EE4(CollData*c){memset(c,0,sizeof *c);}
void mpColl_SetECBSource_Fixed(CollData*c,HSD_GObj*g,float top,float bottom,float left,float right)
{CHECK(g==NULL&&top==1&&bottom==1&&left==1&&right==1);c->ecb.top.y=top;c->ecb.bottom.y=-bottom;c->ecb.left.x=-left;c->ecb.right.x=right;}
HSD_GObj*GObj_Create(u16 classifier,u8 link,u8 priority)
{CHECK(classifier==7&&link==10&&priority==0);if((int)allocation_index==fail_creation)return NULL;CHECK(allocation_index<count);return &link_objects[allocation_index++];}
void*HSD_ObjAlloc(HSD_ObjAllocData*data){CHECK(data==&item_link_alloc_data&&allocation_index>0);return &links[allocation_index-1];}
void GObj_InitUserData(HSD_GObj*g,u8 kind,void(*destroy)(void*),void*data){CHECK(kind==6&&destroy==it_802A2474);g->user_data=data;}
void it_802A2474(void*p){(void)p;}
void it_802A24A0(HSD_GObj*g,int pass){(void)g;(void)pass;}
void HSD_GObj_JObjCallback(HSD_GObj*g,int pass){(void)g;(void)pass;}
void GObj_SetupGXLink(HSD_GObj*g,GObj_RenderFunc callback,u8 link,u32 priority)
{if(!(link==6&&priority==0&&(callback==it_802A24A0||callback==HSD_GObj_JObjCallback)))fprintf(stderr,"gx %u %lu %p a%p b%p\n",link,priority,(void*)callback,(void*)it_802A24A0,(void*)HSD_GObj_JObjCallback);CHECK(link==6&&priority==0&&(callback==it_802A24A0||callback==HSD_GObj_JObjCallback));g->render_cb=callback;}
void HSD_GObjObject_80390A70(HSD_GObj*g,u8 kind,void*model){CHECK(kind==HSD_GObj_JObjKind);g->hsd_obj=model;}
HSD_JObj*HSD_JObjLoadJoint(HSD_Joint*recipe)
{CHECK(allocation_index>0&&(recipe==&link_recipe||recipe==&tip_recipe));unsigned index=allocation_index-1;loaded_models[index]=recipe==&tip_recipe?2:1;return &joints[index];}
void it_80272A3C(HSD_JObj*j){CHECK(j==&primary);primary_hidden=true;}
Item_GObj*Item_80268B18(SpawnItem*spawn)
{CHECK(spawn->kind==It_Kind_Seak_Chain&&spawn->x0_parent_gobj==&creator&&spawn->x4_parent_gobj2==&creator&&spawn->x44_flag.b0&&spawn->x40==0&&spawn->x3C_damage==0);eqvec(spawn->pos,anchor);eqvec(spawn->prev_pos,anchor);eqvec(spawn->vel,(Vec3){0});CHECK(spawn->facing_dir==item.facing_dir);return &object;}
void Item_8026AB54(HSD_GObj*g,HSD_GObj*parent,Fighter_Part part){CHECK(g==&object&&parent==&creator&&part==26);++attached;}
#include "article_constructor_actual.inc"
static unsigned constructor_bones(Fighter*f){CHECK(f==&fighter);return 58;}
static bool begin_resources(HSD_GObj*g,unsigned requested){CHECK(g==&object&&requested==count);pool_created=0;return true;}
static bool create_resource(HSD_GObj*g,unsigned node,const NativeArticleResourceInit*init)
{CHECK(g==&object&&node==allocation_index&&init->attachment_part==26&&init->radius==1&&init->collision_category==5);eqvec(init->position,(Vec3){0});HSD_GObj*created=GObj_Create(7,10,0);if(!created)return false;it_80275328(created,&links[node]);links[node].jobj=&attachment;it_802A24D0(&links[node],1);++pool_created;return true;}
static bool load_resource(HSD_GObj*g,unsigned node,uint32_t name)
{CHECK(g==&object&&node+1==allocation_index);uint32_t link_hash=2166136261u,tip_hash=2166136261u;for(const char*s="Link";*s;++s)link_hash=(link_hash^(uint8_t)*s)*16777619u;for(const char*s="Tip";*s;++s)tip_hash=(tip_hash^(uint8_t)*s)*16777619u;CHECK(name==link_hash||name==tip_hash);if((int)node==fail_model)return false;HSD_GObjObject_80390A70(&link_objects[node],HSD_GObj_JObjKind,HSD_JObjLoadJoint(name==tip_hash?&tip_recipe:&link_recipe));return true;}
static bool write_resource_int(HSD_GObj*g,unsigned node,NativeArticleResourceInt channel,int32_t value)
{CHECK(g==&object&&node<count&&channel==NativeArticleResource_SurfaceLine&&value==0);links[node].x1CC=value;return true;}
static void primary_value(HSD_GObj*g,unsigned model,const char*path,unsigned channel,float value)
{CHECK(g==&object&&model==0&&!*path&&channel==10&&value==0);it_80272A3C(&primary);}
static void constructor_reset(bool generated,unsigned configured)
{count=configured;runtime_reset(generated,0,1);constructor_mode=true;memset(links,0,sizeof links);allocation_index=pool_created=attached=0;memset(loaded_models,0,sizeof loaded_models);object.hsd_obj=&primary;primary_hidden=false;attributes.x0=configured;attributes.x64_joint=&link_recipe;attributes.x68_joint=&tip_recipe;fighter.parts=parts;parts[26].joint=&attachment;for(unsigned p=0;p<sizeof(parameter_names)/sizeof(*parameter_names);++p)if(!strcmp(parameter_names[p],"It_Kind_Seak_Chain.link_count"))script_parameters[p].i=configured;}
static void test_constructor(void)
{
    const unsigned counts[]={3,6,12,20,30};
    for(unsigned c=0;c<5;++c)for(int failure=-1;failure<(int)counts[c];++failure){unsigned old_allocated=0,old_released=0,old_attached=0,old_removed=0;unsigned old_models[30];ItemLink expected[30];
        for(int generated=0;generated<2;++generated){constructor_reset(generated,counts[c]);fail_creation=failure;fail_model=-1;
            if(generated)scripted_article_2_logic_16(&object);else (void)itSeakChain_Spawn(&creator,&anchor,item.facing_dir);
            if(!generated){old_allocated=allocation_index;old_released=released;old_attached=attached;old_removed=removed;memcpy(old_models,loaded_models,sizeof old_models);memcpy(expected,links,sizeof expected);}
            else{CHECK(primary_hidden&&allocation_index==old_allocated&&released==old_released&&attached==old_attached&&removed==old_removed);CHECK(!memcmp(old_models,loaded_models,sizeof old_models));for(unsigned i=0;i<allocation_index;++i){eqvec(expected[i].pos,links[i].pos);eqvec(expected[i].vel,links[i].vel);eqvec(expected[i].coll_data.cur_pos,links[i].coll_data.cur_pos);eqvec(expected[i].coll_data.last_pos,links[i].coll_data.last_pos);CHECK(expected[i].coll_data.x34_flags.b1234==links[i].coll_data.x34_flags.b1234&&expected[i].x1CC==links[i].x1CC);CHECK(!get_bool("active",i)&&!get_bool("touching",i)&&!get_bool("sound_latched",i));}if(failure<0){CHECK(get_int("tip")==((int)counts[c]-1));for(unsigned i=0;i<15;++i){eq(0,get_float("history_x",i));eq(0,get_float("history_y",i));}}++pairs;}
        }
    }
    /* The stock constructor does not check a NULL imported model. The authored
     * constructor rejects failed model activation and releases all created nodes;
     * this is an explicit allocation/model failure safety group, not parity. */
    for(unsigned c=0;c<5;++c)for(unsigned failure=0;failure<counts[c];++failure){constructor_reset(true,counts[c]);fail_creation=-1;fail_model=failure;scripted_article_2_logic_16(&object);CHECK(removed==1&&released==failure+1&&attached==0);++pairs;}
    constructor_mode=false;fail_creation=fail_model=-1;
}
static void test_accessory_render(void)
{
    const unsigned counts[]={3,6,12,20,30};
    for(unsigned c=0;c<5;++c){unsigned configured=counts[c];attributes.x0=configured;for(unsigned p=0;p<sizeof(parameter_names)/sizeof(*parameter_names);++p)if(!strcmp(parameter_names[p],"It_Kind_Seak_Chain.link_count"))script_parameters[p].i=configured;
    for(unsigned first=0;first<configured;++first)for(unsigned scenario=0;scenario<16;++scenario)for(unsigned operation=0;operation<5;++operation){
        if(operation==3&&first==configured-1)continue;
        HSD_JObj expected_joints[30];HitCapsule expected_hits[4];unsigned expected_endpoints[128],expected_endpoint_count=0;Vec3 expected_published[4];int expected_motion=0;
        for(int generated=0;generated<2;++generated){count=configured;runtime_reset(generated,first,scenario);item.msid=operation;contact_mode=scenario%4;
            if(generated)scripted_article_2_logic_15(&object);else switch(operation){case 0:fn_802BB428(&object);break;case 1:fn_802BB44C(&object);break;case 2:fn_802BB574(&object);break;case 3:fn_802BB694(&object);break;case 4:fn_802BB784(&object);break;}
            if(!generated){save();memcpy(expected_joints,joints,sizeof joints);memcpy(expected_hits,fighter.x914,sizeof expected_hits);memcpy(expected_published,fighter.u.sk.xC,sizeof expected_published);memcpy(expected_endpoints,endpoints,sizeof endpoints);expected_endpoint_count=endpoint_count;expected_motion=item.msid;}
            else{compare(operation==3);CHECK(expected_motion==item.msid&&expected_endpoint_count==endpoint_count);CHECK(!memcmp(expected_endpoints,endpoints,endpoint_count*sizeof*endpoints));for(unsigned i=0;i<configured;++i){eqvec(expected_joints[i].translate,joints[i].translate);for(unsigned r=0;r<3;++r)for(unsigned col=0;col<4;++col)eq(expected_joints[i].mtx[r][col],joints[i].mtx[r][col]);CHECK(expected_joints[i].flags==joints[i].flags);}for(unsigned i=0;i<4;++i){CHECK(expected_hits[i].state==fighter.x914[i].state);eqvec(expected_hits[i].x4C,fighter.x914[i].x4C);eqvec(expected_hits[i].x58,fighter.x914[i].x58);eq(expected_published[i].x,script_state_array_float(fighter.native_module_state,60,i,4));eq(expected_published[i].y,script_state_array_float(fighter.native_module_state,76,i,4));eq(expected_published[i].z,script_state_array_float(fighter.native_module_state,92,i,4));}}
        }
    }}
}
static void test_entries_cleanup(void)
{
    const int motions[]={-1,348,349,350,351,352,353,354,355,0x12345};
    for(unsigned m=0;m<10;++m)for(int owner=0;owner<2;++owner){unsigned original_release=0,original_resume=0;bool expected=false;
        for(int generated=0;generated<2;++generated){count=6;runtime_reset(generated,0,7);fighter.motion_id=motions[m];if(!owner)item.owner=NULL;
            bool result=generated?scripted_article_2_state_0_0(&object):itSeakchain_UnkMotion4_Anim(&object);
            if(!generated){expected=result;original_release=released;original_resume=resumed;}else{CHECK(result==expected&&released==original_release&&resumed==original_resume);if(result)CHECK(item.owner==NULL&&item.xDD4_itemVar.native_article.attachment_owner==NULL);++pairs;}
        }
    }
    for(unsigned action=0;action<4;++action){ItemLink expected_tip;int expected_motion=0;unsigned original_released=0,original_resume=0;
        for(int generated=0;generated<2;++generated){count=6;runtime_reset(generated,0,7);Vec3 velocity={((itChainSegment*)&attributes)->x50*item.facing_dir,0,0};
            if(action==0){if(generated)scripted_article_2_signal_0(&object,0);else it_802BCFC4(&object,&velocity);}
            if(action==1){if(generated)scripted_article_2_signal_1(&object,0);else it_802BCF84(&object);}
            if(action==2){if(generated)scripted_article_2_logic_2(&object);else it_2725_Logic54_PickedUp(&object);}
            if(action==3){if(generated)scripted_article_2_signal_4(&object,0);else it_802BB20C(&object);}
            if(!generated){expected_tip=links[5];expected_motion=item.msid;original_released=released;original_resume=resumed;}else{CHECK(item.msid==expected_motion&&released==original_released&&resumed==original_resume);eqvec(expected_tip.pos,links[5].pos);eqvec(expected_tip.vel,links[5].vel);eqvec(expected_tip.coll_data.cur_pos,links[5].coll_data.cur_pos);eqvec(expected_tip.coll_data.last_pos,links[5].coll_data.last_pos);if(action==3)CHECK(removed==1&&item.owner==NULL);++pairs;}
        }
    }
    for(int creator_removed=0;creator_removed<2;++creator_removed)for(int generated=0;generated<2;++generated){count=6;runtime_reset(generated,0,7);HSD_GObj other={0};HSD_GObj*recipient=creator_removed?&creator:&other;if(generated)scripted_article_2_logic_13(&object,recipient);else itSeakChain_Logic54_EvtUnk(&object,recipient);CHECK((generated?item.xDD4_itemVar.native_article.attachment_owner:item.xDD4_itemVar.seakchain.parent_gobj)==(creator_removed?NULL:&creator));CHECK(item.owner==(creator_removed?NULL:&creator));++pairs;}
    for(int invalidation=1;invalidation<=2;++invalidation){count=6;runtime_reset(true,0,7);notification_mutation=invalidation;chain_Cleanup(&object);CHECK(notified==1&&released==0&&item.owner==&creator);++pairs;count=6;runtime_reset(true,0,7);transition_mutation=invalidation;scripted_article_2_signal_1(&object,0);CHECK(item.on_accessory==NULL);++pairs;}
    count=6;runtime_reset(true,0,7);heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;CHECK(scripted_article_2_state_0_0(&object));CHECK(notified==0&&released==6&&item.owner==NULL);++pairs;
    count=6;runtime_reset(true,0,7);chain_Cleanup(&object);CHECK(released==6&&notified==1);chain_Cleanup(&object);CHECK(released==6&&notified==1);++pairs;
}
int main(void)
{
    NativeFighterHost host={0};host.callback_source_alive=nativeCallbackSource_Alive;host.sqrt_float=sqrtf;host.abs_float=fabsf;host.article_resource_read_float=resource_read;host.article_resource_write_float=resource_write;host.article_resource_read_int=resource_int;host.article_resource_sweep_point=resource_sweep;host.article_resource_set_draw=resource_draw;host.play_global_sound=lbAudioAx_800237A8;host.item_creator=retained_creator;host.article_resource_set_world_matrix=world_matrix;host.article_resource_read_attachment_matrix=attachment_matrix;host.article_resource_read_attachment_point=attachment_point;host.article_resource_write_joint=write_joint;host.set_hitbox_position=ftColl_8007B8A8;host.clear_hitboxes=ftColl_8007AFF8;host.item_change_state_flags=Item_80268E5C;host.item_current_owner=owner_id;host.target_motion=motion;host.target_call=notify;host.signal_article=signal;host.item_pause=it_8026B724;host.item_resume=it_8026B73C;host.article_resource_release=release_resources;host.item_remove=Item_8026A8EC;host.item_interact=interact;host.article_resource_begin=begin_resources;host.article_resource_create=create_resource;host.article_resource_load_model=load_resource;host.article_resource_write_int=write_resource_int;host.item_set_joint_value=primary_value;host.item_attach_to_owner=Item_8026AB54;host.bone_count=constructor_bones;engine=&host;
    CHECK(FreakMelee_SetFighterParameters(parameter_bits,sizeof(parameter_bits)/sizeof(*parameter_bits),parameter_schema));
#include "article_attributes.inc"
    test_accessory_render();test_entries_cleanup();test_constructor();
    printf("PASS %u article runtime pairs, %u checks, maximum_float_difference %.9g\n",pairs,checks,maximum_difference);return 0;
}
