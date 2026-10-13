/* Controlled allocation/HSD/AB54 subdependencies, with the real resource unit
 * and real complete constructors/AB54/pickup/solver bodies in the executable. */
enum { RopeAllocate=32, RopeModel, RopeAttach, RopeState, RopeVisible, RopeTail, RopeFree, IceRemove, RopeRemove, EffectsClear };
static HSD_GObj rope_nodes[40];
static ItemLink rope_links[40], expected_links[40];
static HSD_JObj rope_models[40], rope_root;
static HSD_Joint segment_descriptor, tail_descriptor;
static bool node_live[40], link_live[40], model_live[40], allocation_failure;
static unsigned node_count, link_count, model_count, resource_frees;
static int fail_node=-1,fail_link=-1,fail_model=-1;
static SpawnItem last_spawn, expected_spawn;
static ItemLogicTable stock_rope_logic;
static const NativeFighterArticleLogic* owned_rope, *owned_ice;
static HSD_GObj ice_object;
static Item ice_item;
void it_2725_Logic90_Destroyed(Item_GObj*);
HSD_ObjAllocData item_link_alloc_data;
u8 HSD_GObj_JObjKind;
static uint32_t hash_name(const char*name){uint32_t h=2166136261u;while(*name)h=(h^(unsigned char)*name++)*16777619u;return h;}
Article* nativeFighterRuntime_ArticleAssets(FighterKind kind,ItemKind article_kind)
{CHECK(kind==Ft_Kind_CustomFirst&&article_kind==It_Kind_IceClimber_GumStrings);return &rope_article;}
static Item_GObj* allocate_item(SpawnItem*spawn,FighterKind author)
{
    event(RopeAllocate,0,spawn->kind,spawn->x44_flag.b0,spawn->pos.x,spawn->pos.y,spawn->pos.z);
    last_spawn=*spawn;if(allocation_failure)return NULL;
    memset(&rope_item,0,sizeof rope_item);memset(&rope_object,0,sizeof rope_object);
    rope_object.user_data=&rope_item;rope_object.hsd_obj=&rope_root;rope_object.classifier=HSD_GOBJ_CLASS_ITEM;
    rope_item.entity=&rope_object;rope_item.x1C=UINT32_C(0xe123abcd);rope_item.kind=spawn->kind;
    rope_item.pos=spawn->pos;rope_item.x40_vel=spawn->vel;rope_item.facing_dir=spawn->facing_dir;
    rope_item.owner=spawn->x0_parent_gobj;rope_item.xC4_article_data=&rope_article;
    rope_item.native_article_author_kind=author;
    rope_item.xB8_itemLogicTable=author==Ft_Kind_None?&stock_rope_logic:owned_rope->logic;
    heads[HSD_GOBJ_PLINK_ITEM]=&rope_object;return &rope_object;
}
Item_GObj* Item_80268B18(SpawnItem*spawn){return allocate_item(spawn,Ft_Kind_None);}
Item_GObj* Item_CreateAuthored(SpawnItem*spawn,FighterKind author){return allocate_item(spawn,author);}
HSD_GObj*GObj_Create(u16 classifier,u8 link,u8 priority)
{
    CHECK(classifier==7&&link==10&&priority==0);unsigned i=node_count++;
    if((int)i==fail_node)return NULL;CHECK(i<40);node_live[i]=true;memset(&rope_nodes[i],0,sizeof rope_nodes[i]);return &rope_nodes[i];
}
void*HSD_ObjAlloc(HSD_ObjAllocData*data)
{
    CHECK(data==&item_link_alloc_data);unsigned i=link_count++;
    if((int)i==fail_link)return NULL;CHECK(i<40);link_live[i]=true;memset(&rope_links[i],0xa5,sizeof rope_links[i]);return &rope_links[i];
}
void HSD_ObjFree(HSD_ObjAllocData*data,void*link)
{
    CHECK(data==&item_link_alloc_data);ptrdiff_t i=(ItemLink*)link-rope_links;
    CHECK(i>=0&&i<40&&link_live[i]);link_live[i]=false;memset(link,0xdd,sizeof(ItemLink));
}
void GObj_InitUserData(HSD_GObj*g,u8 kind,void(*remove)(void*),void*value)
{CHECK(kind==6&&remove);g->user_data=value;g->user_data_remove_func=remove;}
void HSD_GObjFree(HSD_GObj*g)
{
    ptrdiff_t i=g-rope_nodes;CHECK(i>=0&&i<40&&node_live[i]);
    if(g->hsd_obj){ptrdiff_t m=(HSD_JObj*)g->hsd_obj-rope_models;CHECK(m>=0&&m<40&&model_live[m]);model_live[m]=false;}
    if(g->user_data_remove_func)g->user_data_remove_func(g->user_data);
    node_live[i]=false;memset(g,0xdd,sizeof(*g));++resource_frees;
}
HSD_JObj*HSD_JObjLoadJoint(HSD_Joint*descriptor)
{
    CHECK(descriptor==&segment_descriptor||descriptor==&tail_descriptor);unsigned i=model_count++;
    event(RopeModel,0,i,descriptor==&tail_descriptor,0,0,0);
    if((int)i==fail_model)return NULL;CHECK(i<40);model_live[i]=true;memset(&rope_models[i],0,sizeof(HSD_JObj));return &rope_models[i];
}
void HSD_GObjObject_80390A70(HSD_GObj*g,u8 kind,void*model){CHECK(kind==HSD_GObj_JObjKind);g->hsd_obj=model;}
void GObj_SetupGXLink(HSD_GObj*g,GObj_RenderFunc callback,u8 link,u32 priority)
{CHECK(callback&&link==6&&priority==0);g->render_cb=callback;}
void HSD_GObj_JObjCallback(HSD_GObj*g,int pass){CHECK(g->hsd_obj&&pass==0);}
void mpColl_80041EE4(CollData*cd)
{Vec3 p=cd->cur_pos;memset(cd,0,sizeof(*cd));cd->cur_pos=cd->prev_pos=cd->last_pos=cd->x28_vec=p;}
void mpColl_SetECBSource_Fixed(CollData*cd,HSD_GObj*g,float a,float b,float c,float d)
{CHECK(!g&&a==b&&b==c&&c==d&&a>0);(void)cd;}
bool mpColl_800471F8(CollData*cd){(void)cd;unexpected();return false;}
bool mpColl_80048844(CollData*cd){(void)cd;unexpected();return false;}
bool mpColl_8004B108(CollData*cd){(void)cd;unexpected();return false;}
bool mpGetSpeed(int line,Vec3*p,Vec3*v){(void)line;(void)p;(void)v;unexpected();return false;}
void HSD_JObjSetupMatrixSub(HSD_JObj*j){j->flags&=~JOBJ_MTX_DIRTY;}
void HSD_JObjSetMtxDirtySub(HSD_JObj*j){j->flags|=JOBJ_MTX_DIRTY;}
void C_MTXIdentity(Mtx m){memset(m,0,sizeof(Mtx));m[0][0]=m[1][1]=m[2][2]=1;}
void C_MTXCopy(Mtx a,Mtx b){memcpy(b,a,sizeof(Mtx));}
void C_MTXConcat(Mtx a,Mtx b,Mtx out)
{Mtx m;for(unsigned i=0;i<3;++i){for(unsigned j=0;j<3;++j)m[i][j]=a[i][0]*b[0][j]+a[i][1]*b[1][j]+a[i][2]*b[2][j];m[i][3]=a[i][0]*b[0][3]+a[i][1]*b[1][3]+a[i][2]*b[2][3]+a[i][3];}C_MTXCopy(m,out);}
void C_MTXMultVec(Mtx m,Vec3*p,Vec3*r){*r=(Vec3){m[0][0]*p->x+m[0][1]*p->y+m[0][2]*p->z+m[0][3],m[1][0]*p->x+m[1][1]*p->y+m[1][2]*p->z+m[1][3],m[2][0]*p->x+m[2][1]*p->y+m[2][2]*p->z+m[2][3]};}
void C_VECCrossProduct(Vec*a,Vec*b,Vec*r){*r=(Vec){a->y*b->z-a->z*b->y,a->z*b->x-a->x*b->z,a->x*b->y-a->y*b->x};}
void HSD_JObjSetFlags(HSD_JObj*j,u32 flags){j->flags|=flags;}
void HSD_JObjClearFlags(HSD_JObj*j,u32 flags){j->flags&=~flags;}
void HSD_JObjSetFlagsAll(HSD_JObj*j,u32 flags){if(j){j->flags|=flags;HSD_JObjSetFlagsAll(j->child,flags);}}
void HSD_JObjClearFlagsAll(HSD_JObj*j,u32 flags){if(j){j->flags&=~flags;HSD_JObjClearFlagsAll(j->child,flags);}}
void HSD_JObjAddAnimAll(HSD_JObj*j,HSD_AnimJoint*a,HSD_MatAnimJoint*b,HSD_ShapeAnimJoint*c){(void)j;(void)a;(void)b;(void)c;unexpected();}
void HSD_JObjReqAnimAll(HSD_JObj*j,float frame){(void)j;(void)frame;unexpected();}
void HSD_JObjAnimAll(HSD_JObj*j){(void)j;unexpected();}
void HSD_JObjRemoveAnimAll(HSD_JObj*j){(void)j;unexpected();}
void lb_8000BA0C(HSD_JObj*j,float rate){(void)j;(void)rate;unexpected();}
HSD_JObj*nativeModelJoint_Resolve(HSD_JObj*j,const char*path){CHECK(!*path);return j;}
float nativeModelJoint_Read(HSD_JObj*j,unsigned channel){CHECK(j&&channel<3);return (&j->translate.x)[channel];}
void nativeModelJoint_Write(HSD_JObj*j,unsigned channel,float v){CHECK(j&&channel<3);Vec3 p=j->translate;(&p.x)[channel]=v;HSD_JObjSetTranslate(j,&p);}
void it_80273168(Item_GObj*g){CHECK(g==&rope_object);event(RopeAttach,0,0,0,0,0,0);}
void it_802742F4(Item_GObj*g,HSD_GObj*holder,Fighter_Part part)
{CHECK(g==&rope_object&&holder==&objects[0]&&part==29);event(RopeAttach,0,1,part,0,0,0);rope_item.owner=holder;rope_item.xDC4=part;}
void Item_8026B074(Item*item){CHECK(item==&rope_item);event(RopeTail,0,0,0,0,0,0);}
void Item_80268E5C(Item_GObj*g,enum_t state,Item_StateChangeFlags flags)
{CHECK(g==&rope_object);event(RopeState,0,state,flags,0,0,0);rope_item.msid=state;
    if(invalidation==4||invalidation==14){nativeArticleResource_CancelGeneration(g,rope_item.x1C);
        if(invalidation==4){heads[HSD_GOBJ_PLINK_ITEM]=NULL;rope_object.user_data=NULL;}else ++rope_item.x1C;
    }
    if(invalidation==5||invalidation==15){nativeArticleResource_CancelCreator(&objects[0],fighters[0].x8_spawnNum);
        if(invalidation==5){present[0]=false;objects[0].user_data=NULL;registry();}else ++fighters[0].x8_spawnNum;
    }
}
void it_8026BB44(Item_GObj*g){CHECK(g==&rope_object);event(RopeVisible,0,0,0,0,0,0);rope_root.flags|=JOBJ_HIDDEN;}
void it_8026BB20(Item_GObj*g){CHECK(g==&rope_object);event(RopeVisible,0,1,0,0,0,0);rope_root.flags&=~JOBJ_HIDDEN;}
static void joint_value(Item_GObj*g,unsigned model,const char*path,unsigned channel,float value)
{CHECK(g==&rope_object&&model==0&&!*path&&channel==10);if(value)it_8026BB20(g);else it_8026BB44(g);}
bool it_8026B894(Item_GObj*g,HSD_GObj*other){CHECK(g==&rope_object);(void)other;return false;}
FtMotionId ftLib_GetMotionId(Fighter_GObj*g){return fighters[role_of(g)].motion_id;}
void it_8026BB68(HSD_GObj*g,Vec3*pos){(void)g;(void)pos;unexpected();}
void Item_8026A8EC(Item_GObj*g)
{
    CHECK(g==&rope_object||g==&ice_object);Item*item=g->user_data;CHECK(item);
    if(item->native_removal_in_progress)return;item->native_removal_in_progress=true;
    if(g==&ice_object){event(IceRemove,0,0,0,0,0,0);
        if(scripted)owned_ice->logic->destroyed(g);else it_2725_Logic90_Destroyed(g);
    }else{event(RopeRemove,0,0,0,0,0,0);
        if(scripted){nativeArticleResource_CancelGeneration(g,item->x1C);if(owned_rope->logic->destroyed)owned_rope->logic->destroyed(g);}
    }
    HSD_GObj**link=&heads[HSD_GOBJ_PLINK_ITEM];while(*link&&*link!=g)link=&(*link)->next;if(*link)*link=g->next;
    g->user_data=NULL;
}
void efLib_DestroyAll(HSD_GObj*g){event(EffectsClear,role_of(g),0,0,0,0,0);}
void ftKb_SpecialNIc_80108CE8(HSD_GObj*g,Item_GObj*item){(void)g;(void)item;unexpected();}
static uint32_t current_owner(Item_GObj*g){CHECK(g==&rope_object);return nativeFighterTarget_Id(rope_item.owner);}
static bool bind_auxiliary(Fighter_GObj*g,uint32_t id,bool auxiliary)
{CHECK(g==&objects[0]&&auxiliary);if(!nativeCallbackSource_Alive(&rope_object,(uint32_t)id,NativeCallbackSource_Item))return false;fighters[0].x1984_heldItemSpec=&rope_object;return true;}
static bool clear_auxiliary(Fighter_GObj*g,bool auxiliary){CHECK(g==&objects[0]&&auxiliary);fighters[0].x1984_heldItemSpec=NULL;return true;}
