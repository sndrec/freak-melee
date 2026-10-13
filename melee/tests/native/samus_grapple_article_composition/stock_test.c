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

enum { Nodes = 90, AttachmentPart = 51 };
static HSD_GObj fighters[2], items_gobj[2], nodes_gobj[2][Nodes];
static Fighter fighters_data[2];
static Item items_data[2];
static ItemLink links[2][Nodes];
static HSD_JObj models[2][Nodes], attachment[2], root_models[2], root_children[2][2];
static bool nodes_live[2][Nodes], model_live[2][Nodes], roots_live[2];
static FighterBone parts[2][MAX_FT_PARTS];
static FighterPartsTable parts_table;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable = tables;
static HSD_GObj* live_heads[32];
HSD_GObj** HSD_GObjPLinkHead = live_heads;
HSD_ObjAllocData item_link_alloc_data;
u8 HSD_GObj_JObjKind;
const Vec4 it_803B8650 = { 0 };
f32 it_804D6D48 = 6;
static Article articles[2];
static ItemAttr common_attribute;
static itSamusGrappleAttributes tuning[2], source_tuning;
static ftSs_DatAttrs fighter_tuning;
static ftData fighter_data;
static const NativeFighterDefinition* definition;
static const NativeFighterArticleLogic* authored;
static unsigned clear_calls[2];
static unsigned current_side, objects_created[2], links_created[2], models_created[2];
static unsigned freed[2], state_changes[2], scale_calls[2], probes[2], floors[2];
static int fail_type, fail_index;
static unsigned scenario, checks, source_cases, safety_cases;
static float max_float_difference;
static FighterKind family;
static ItemKind item_kind;
static uint8_t arena[8 * 1024 * 1024];
static uint8_t* model_dat_bytes;
static mp_UnkStruct0 island;
static NativeFighterDefinition replacement;
enum { EventState=1, EventScale, EventClear, EventCapture, EventHit, EventEffect,
       EventDamage, EventProbe, EventOccupant, EventCatch, EventJump, EventSound, EventRay };
typedef struct TraceEvent { int id; float value[4]; } TraceEvent;
static TraceEvent trace[2][512];
static unsigned trace_count[2], phase_count[9], transition_count[9], animation_cases;
static unsigned observed_event[EventRay+1];
static unsigned ub_constraints, substituted_cases, rng_count[2], animations[2];
static unsigned base_case, interaction_cases, hitlag_cases;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "Check failed at %s:%u: %s (scenario %u)\n", __FILE__, __LINE__, #x, scenario); abort(); } } while (0)

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind == family ? definition : NULL; }

Fighter_GObj* nativeArticle_Creator(Item_GObj* gobj)
{
    if (!gobj || !gobj->user_data) return NULL;
    Item* item = gobj->user_data;
    Fighter_GObj* creator = item->xDD4_itemVar.native_article.attachment_owner;
    if (!nativeCallbackSource_Alive(creator, item->xDD4_itemVar.native_article.creator_spawn_id,
                                   NativeCallbackSource_Fighter)) return NULL;
    return nativeFighterRuntime_Definition(((Fighter*)creator->user_data)->kind) ==
        nativeFighterRuntime_Definition(item->native_article_author_kind) ? creator : NULL;
}

static unsigned side(HSD_GObj* object)
{ return object == &fighters[1] || object == &items_gobj[1] ? 1 : 0; }
static void record(unsigned s,int id,float a,float b,float c,float d)
{ CHECK(trace_count[s]<512);trace[s][trace_count[s]++]=(TraceEvent){id,{a,b,c,d}}; }

HSD_GObj* GObj_Create(u16 kind, u8 link, u8 priority)
{
    CHECK(kind == HSD_GOBJ_CLASS_ITEMLINK && link == 10 && priority == 0);
    unsigned index = objects_created[current_side]++;
    CHECK(index < Nodes);
    if (fail_type == 1 && (int)index == fail_index) return NULL;
    nodes_live[current_side][index] = true;
    return &nodes_gobj[current_side][index];
}
void* HSD_ObjAlloc(HSD_ObjAllocData* pool)
{
    CHECK(pool == &item_link_alloc_data);
    unsigned index = links_created[current_side]++;
    CHECK(index < Nodes);
    if (fail_type == 2 && (int)index == fail_index) return NULL;
    return &links[current_side][index];
}
void HSD_ObjFree(HSD_ObjAllocData* pool, void* data)
{ CHECK(pool == &item_link_alloc_data && data); }
void GObj_InitUserData(HSD_GObj* gobj, u8 kind, void(*destroy)(void*), void* data)
{ CHECK(kind == 6); gobj->user_data = data; gobj->user_data_remove_func = destroy; }
void HSD_GObjFree(HSD_GObj* object)
{
    for (unsigned s = 0; s < 2; ++s) for (unsigned i = 0; i < Nodes; ++i) {
        if (object != &nodes_gobj[s][i]) continue;
        CHECK(nodes_live[s][i]); nodes_live[s][i] = false; model_live[s][i] = false;
        ++freed[s];
        if (object->user_data_remove_func) object->user_data_remove_func(object->user_data);
        object->user_data = NULL; object->hsd_obj = NULL;
        return;
    }
    abort();
}
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* desc)
{
    unsigned i = models_created[current_side]++;
    CHECK(i < Nodes && desc);
    if (fail_type == 3 && (int)i == fail_index) return NULL;
    HSD_JObj* model = &models[current_side][i];
    model->flags = desc->flags;
    model->scale = desc->scale; model->translate = desc->position;
    model->rotate.x = desc->rotation.x; model->rotate.y = desc->rotation.y;
    model->rotate.z = desc->rotation.z;
    model_live[current_side][i] = true;
    return model;
}
void HSD_GObjObject_80390A70(HSD_GObj* object, u8 kind, void* model)
{ CHECK(kind == HSD_GObj_JObjKind); object->hsd_obj = model; }
void GObj_SetupGXLink(HSD_GObj* object, GObj_RenderFunc draw, u8 link, u32 priority)
{ CHECK(link == 6 && priority == 0); object->render_cb = draw; }
static unsigned draws;
void HSD_GObj_JObjCallback(HSD_GObj* object, int pass)
{ (void)object; (void)pass; ++draws; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void HSD_JObjSetupMatrixSub(HSD_JObj* joint) { joint->flags &= ~JOBJ_MTX_DIRTY; }
void HSD_JObjSetFlags(HSD_JObj* joint, u32 flags) { joint->flags |= flags; }
void HSD_JObjClearFlags(HSD_JObj* joint, u32 flags) { joint->flags &= ~flags; }
void HSD_JObjSetFlagsAll(HSD_JObj* joint, u32 flags)
{ for (; joint; joint = joint->next) { joint->flags |= flags; if (!(joint->flags & JOBJ_INSTANCE)) HSD_JObjSetFlagsAll(joint->child, flags); } }
void HSD_JObjClearFlagsAll(HSD_JObj* joint, u32 flags)
{ for (; joint; joint = joint->next) { joint->flags &= ~flags; if (!(joint->flags & JOBJ_INSTANCE)) HSD_JObjClearFlagsAll(joint->child, flags); } }
void HSD_JObjAddAnimAll(HSD_JObj* j,HSD_AnimJoint* a,HSD_MatAnimJoint* m,HSD_ShapeAnimJoint* s)
{ (void)j; (void)a; (void)m; (void)s; abort(); }
void HSD_JObjReqAnimAll(HSD_JObj* j,float frame) { (void)j; (void)frame; abort(); }
void HSD_JObjAnimAll(HSD_JObj* j) { CHECK(j); ++animations[current_side]; }
void HSD_JObjRemoveAnimAll(HSD_JObj* j) { (void)j; abort(); }
void lb_8000BA0C(HSD_JObj* j,float rate) { (void)j; (void)rate; abort(); }
void it_80272F7C(HSD_JObj* j, float value) { j->scale = (Vec3){ value, value, value };for(unsigned s=0;s<2;++s)if(j==&root_models[s])record(s,EventScale,value,0,0,0); }
void __assert(char* file, u32 line, char* condition)
{ fprintf(stderr, "%s:%u: %s\n", file, (unsigned)line, condition); abort(); }

void C_MTXCopy(Mtx a, Mtx b) { memcpy(b, a, sizeof(Mtx)); }
void C_MTXIdentity(Mtx m) { memset(m, 0, sizeof(Mtx)); for (unsigned i = 0; i < 3; ++i) m[i][i] = 1; }
void C_MTXConcat(Mtx a, Mtx b, Mtx result)
{
    Mtx value;
    for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c) {
        value[r][c] = a[r][0]*b[0][c] + a[r][1]*b[1][c] + a[r][2]*b[2][c];
        if (c == 3) value[r][c] += a[r][3];
    }
    C_MTXCopy(value, result);
}
void C_MTXMultVec(Mtx m, Vec3* p, Vec3* out)
{
    Vec3 value = { m[0][0]*p->x + m[0][1]*p->y + m[0][2]*p->z + m[0][3],
                   m[1][0]*p->x + m[1][1]*p->y + m[1][2]*p->z + m[1][3],
                   m[2][0]*p->x + m[2][1]*p->y + m[2][2]*p->z + m[2][3] };
    *out = value;
}
void C_VECCrossProduct(Vec3* a, Vec3* b, Vec3* out)
{ *out = (Vec3){ a->y*b->z-a->z*b->y, a->z*b->x-a->x*b->z, a->x*b->y-a->y*b->x }; }
float lbVector_Normalize(Vec3* v)
{
    float length = sqrtf(v->x*v->x + v->y*v->y + v->z*v->z);
    float inverse = length ? 1.0f/length : 0;
    v->x *= inverse; v->y *= inverse; v->z *= inverse;
    return length;
}
void mpColl_80041EE4(CollData* c)
{ memset(c, 0, sizeof(*c)); c->floor.index = c->ceiling.index = c->left_facing_wall.index = c->right_facing_wall.index = -1; }
void mpColl_SetECBSource_Fixed(CollData* c,HSD_GObj* g,float top,float bottom,float left,float right)
{ (void)c; CHECK(!g && top == bottom && bottom == left && left == right); }
bool mpColl_800471F8(CollData* c)
{
    ++probes[current_side];
    c->cur_pos.x += scenario % 3 * .125f; c->cur_pos.y -= scenario % 5 * .25f;
    c->env_flags = scenario % 3 == 0 ? Collide_FloorMask : scenario % 3 == 1 ? Collide_WallMask : 0;
    if(scenario&16)c->env_flags=Collide_RightWallMask;
    c->floor.index = 7;
    return scenario % 3 == 0;
}
bool mpColl_8004B108(CollData* c)
{ ++floors[current_side]; c->cur_pos.y += .375f; c->env_flags = Collide_FloorMask; c->floor.index = 7; return scenario % 4 != 0; }
bool mpGetSpeed(int line, Vec3* position, Vec3* output)
{ CHECK(line==7 && position); *output = (scenario&32)?(Vec3){.125f,-.25f,.0625f}:(Vec3){0}; return (scenario&32)!=0; }
bool mpLib_80054ED8(int line) { return line == 7; }
mp_UnkStruct0* mpIsland_8005AB54(int line) { return line == 7 && scenario % 2 ? &island : NULL; }
Fighter_Part ftParts_GetBoneIndex(Fighter* fighter, Fighter_Part part)
{ (void)fighter; CHECK(part == FtPart_RThumbNb); return AttachmentPart; }

static void remove_item(Item_GObj* object)
{
    unsigned s = side(object);
    nativeArticleResource_CancelGeneration(object, items_data[s].x1C);
    roots_live[s] = false;
    live_heads[HSD_GOBJ_PLINK_ITEM] = roots_live[1] ? &items_gobj[1] : roots_live[0] ? &items_gobj[0] : NULL;
    items_gobj[1].next = roots_live[0] ? &items_gobj[0] : NULL;
}
void Item_8026A8EC(Item_GObj* object) { remove_item(object); }
void Item_80268E5C(Item_GObj* object, enum_t motion, Item_StateChangeFlags flags)
{ unsigned s=side(object); CHECK(motion >= 0 && motion <= 8 && flags == ITEM_ANIM_UPDATE); items_data[s].msid = motion; ++state_changes[s];record(s,EventState,motion,flags,0,0);++transition_count[motion]; }
static void scale_item(Item_GObj* object, float scale)
{ unsigned s=side(object); ++scale_calls[s]; items_data[s].scl = scale*common_attribute.x60_scale; it_80272F7C(object->hsd_obj,items_data[s].scl); }
void ftColl_8007AFF8(Fighter_GObj* object) { ++clear_calls[side(object)];record(side(object),EventClear,0,0,0,0); }
bool it_8026B894(Item_GObj* object, HSD_GObj* other) { (void)object; (void)other; return false; }

bool mpCheckAllRemap(Vec3* p,int* line,u32* flags,Vec3* normal,int skip,int only,float x0,float y0,float x1,float y1)
{ CHECK(!p&&!line&&!flags&&!normal&&skip==-1&&only==-1);record(current_side,EventRay,x0,y0,x1,y1);unsigned ordinal=0;for(unsigned i=0;i<trace_count[current_side];++i)ordinal+=trace[current_side][i].id==EventRay;return (scenario&64)?ordinal==1:(scenario&128)?ordinal==2:false; }
int lbAudioAx_800237A8(enum_t sound,int volume,int pan)
{ record(current_side,EventSound,sound,volume,pan,0);return 0; }
void ftCommon_8007E2F4(Fighter* f,s16 mask)
{ record(f==&fighters_data[1],EventCapture,mask,0,0,0); }
void ftCo_800C3CC0(Fighter_GObj* object)
{ unsigned s=side(object);record(s,EventHit,358,1,0,0);fighters_data[s].motion_id=358;fighters_data[s].self_vel.x=fmaxf(-fighters_data[s].co_attrs.air_drift_max,fminf(fighters_data[s].co_attrs.air_drift_max,fighters_data[s].self_vel.x));fighters_data[s].mv.co.aircatchhit.x0=20;fighters_data[s].mv.co.aircatchhit.x4=0;fighters_data[s].ground_or_air=GA_Air; }
static void change_motion(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* other)
{ CHECK(motion==358&&flags==1&&frame==0&&rate==1&&blend==0&&!other);record(side(object),EventHit,motion,flags,frame,0);((Fighter*)object->user_data)->motion_id=motion; }
static void clamp(Fighter* f,float maximum) { f->self_vel.x=fmaxf(-maximum,fminf(maximum,f->self_vel.x)); }
static void force_air(Fighter* f) { f->ground_or_air=GA_Air; }
void ftCo_80090780(Fighter_GObj* object) { record(side(object),EventDamage,0,0,0,0);fighters_data[side(object)].motion_id=38; }
bool ftCo_800C3A14(Fighter_GObj* object)
{ record(side(object),EventProbe,5,5,0,0);if(scenario&256){fighters_data[side(object)].self_vel.x=fighters_data[side(object)].self_vel.y=0;return true;}return false; }
static bool probe_ledge(Fighter_GObj* object,float y,float height)
{ CHECK(y==5&&height==5);record(side(object),EventProbe,y,height,0,0);return (scenario&256)!=0; }
Fighter_GObj* ft_80082E3C(Fighter_GObj* object)
{ record(side(object),EventOccupant,0,0,0,0);return (scenario&512)?object:NULL; }
void ftCliffCommon_80081370(Fighter_GObj* object) { record(side(object),EventCatch,0,0,0,0);fighters_data[side(object)].motion_id=253; }
void ftCo_8009B390(Fighter_GObj* object,float force) { record(side(object),EventJump,force,0,0,0);fighters_data[side(object)].motion_id=265; }
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj* gobj,...)
{ (void)gobj;va_list args;va_start(args,gobj);Vec3* position=va_arg(args,Vec3*);record(current_side,EventEffect,effect,position->x,position->y,position->z);va_end(args);return (EF_SpawnResult){0}; }
static void item_effect(Item_GObj* gobj,s32 id,s32 bone,Vec3* p,Vec3* rotation,s32 flags,float scale)
{ (void)gobj;(void)bone;(void)rotation;(void)flags;(void)scale;CHECK(id==1052&&p);record(current_side,EventEffect,id,p->x,p->y,p->z); }
float HSD_Randf(void)
{ unsigned n=rng_count[current_side]++;return (float[]){.9375f,.25f,.5f,.96875f,.75f,.125f}[n%6]; }
Fighter* ftCo_800A4A40(Fighter* f){(void)f;abort();}
void it_802B7160(Fighter_GObj* g,itSamusGrapple_HitboxData* d){(void)g;(void)d;abort();}
const itSamusGrapple_Hitbox it_803B8660={0}; // unsupported modified-mode descriptor boundary traps
#include "samus_joint_access.h"
#include "samus_originals.h"
#include "samus_model_import.h"

static uint32_t hash(const char* name)
{ uint32_t h = UINT32_C(2166136261); for (; *name; ++name) h = (h ^ (unsigned char)*name) * UINT32_C(16777619); return h; }
static bool ref_exists(HSD_GObj* source, uint32_t id, ItemKind kind)
{ (void)source; return roots_live[1] && id == items_data[1].x1C && kind == item_kind; }
static bool signal_article(HSD_GObj* source,uint32_t id,uint32_t name,float value)
{
    if (!ref_exists(source,id,item_kind)) return false;
    for (unsigned i = 0; i < authored->signal_count; ++i) if (authored->signals[i].name_hash == name) {
        CHECK(nativeCallbackDispatch_Enter());
        authored->signals[i].callback(&items_gobj[1],value);
        nativeCallbackDispatch_Leave(); return true;
    }
    return false;
}
static float root_scale(Fighter_GObj* object) { return ((Fighter*)object->user_data)->x34_scale.y; }
static bool island_bounds(int32_t line,Vec3* a,Vec3* b)
{ *a=*b=(Vec3){0}; mp_UnkStruct0* found=mpIsland_8005AB54(line); if(!found)return false;*a=found->x8;*b=found->x14;return true; }
static NativeFighterHost host = {
    .resolve_bone=ftParts_GetBoneIndex,
    .article_ref_exists=ref_exists, .signal_article=signal_article,
    .callback_source_alive=nativeCallbackSource_Alive, .fighter_root_scale_y=root_scale,
    .item_interact=it_8026B894, .sqrt_float=sqrtf, .item_remove=remove_item,
    .item_scale_by_attribute=scale_item, .stage_line_island_bounds=island_bounds,
    .item_creator=nativeArticle_Creator, .item_change_state_flags=Item_80268E5C,
    .article_resource_begin=nativeArticleResource_Begin,
    .article_resource_create=nativeArticleResource_CreateNode,
    .article_resource_load_model=nativeArticleResource_LoadModel,
    .article_resource_set_draw=nativeArticleResource_SetDraw,
    .article_resource_bind_external_part=nativeArticleResource_BindExternalPart,
    .article_resource_release=nativeArticleResource_Release,
    .article_resource_write_joint=nativeArticleResource_WriteJoint,
    .article_resource_set_world_matrix=nativeArticleResource_SetWorldMatrix,
    .article_resource_read_float=nativeArticleResource_ReadFloat,
    .article_resource_write_float=nativeArticleResource_WriteFloat,
    .article_resource_read_int=nativeArticleResource_ReadInt,
    .article_resource_write_int=nativeArticleResource_WriteInt,
    .article_resource_sweep_stage=nativeArticleResource_SweepStage,
    .article_resource_follow_floor=nativeArticleResource_FollowFloor,
    .article_resource_read_attachment_point=nativeArticleResource_ReadAttachmentPoint,
    .article_resource_read_attachment_matrix=nativeArticleResource_ReadAttachmentMatrix,
};

static void unused_host(void) { fprintf(stderr,"Unexercised Host operation called\n"); abort(); }
static void initialize_unused_host(void)
{
    #include "unused_host.h"
    host.resolve_bone=ftParts_GetBoneIndex;
    host.article_ref_exists=ref_exists;
    host.signal_article=signal_article;
    host.callback_source_alive=nativeCallbackSource_Alive;
    host.fighter_root_scale_y=root_scale;
    host.item_interact=it_8026B894;
    host.sqrt_float=sqrtf;
    host.item_remove=remove_item;
    host.item_scale_by_attribute=scale_item;
    host.stage_line_island_bounds=island_bounds;
    host.item_creator=nativeArticle_Creator;
    host.item_change_state_flags=Item_80268E5C;
    host.article_resource_begin=nativeArticleResource_Begin;
    host.article_resource_create=nativeArticleResource_CreateNode;
    host.article_resource_load_model=nativeArticleResource_LoadModel;
    host.article_resource_set_draw=nativeArticleResource_SetDraw;
    host.article_resource_bind_external_part=nativeArticleResource_BindExternalPart;
    host.article_resource_release=nativeArticleResource_Release;
    host.article_resource_write_joint=nativeArticleResource_WriteJoint;
    host.article_resource_set_world_matrix=nativeArticleResource_SetWorldMatrix;
    host.article_resource_read_float=nativeArticleResource_ReadFloat;
    host.article_resource_write_float=nativeArticleResource_WriteFloat;
    host.article_resource_read_int=nativeArticleResource_ReadInt;
    host.article_resource_write_int=nativeArticleResource_WriteInt;
    host.article_resource_sweep_stage=nativeArticleResource_SweepStage;
    host.article_resource_follow_floor=nativeArticleResource_FollowFloor;
    host.article_resource_read_attachment_point=nativeArticleResource_ReadAttachmentPoint;
    host.article_resource_read_attachment_matrix=nativeArticleResource_ReadAttachmentMatrix;
    host.article_resource_read_line_speed = nativeArticleResource_ReadLineSpeed;
    host.stage_line_exists = mpLib_80054ED8;
    host.clear_hitboxes = ftColl_8007AFF8;
    host.item_set_joint_value = nativeArticleModel_Write;
    host.play_global_sound = lbAudioAx_800237A8;
    host.stage_segment_intersection = mpCheckAllRemap;
    host.change_motion_state = change_motion;
    host.clamp_self_vel_x = clamp;
    host.ground_to_air = force_air;
    host.set_capture_mask = ftCommon_8007E2F4;
    host.enter_damage_fall = ftCo_80090780;
    host.probe_ledge = probe_ledge;
    host.find_ledge_occupant = ft_80082E3C;
    host.enter_cliff_catch = ftCliffCommon_80081370;
    host.enter_cliff_jump_phase2 = ftCo_8009B390;
    host.spawn_effect = efSync_Spawn;
    host.item_spawn_effect = item_effect;
    host.random_float=HSD_Randf;
    host.fighter_model_scale=root_scale;
    host.model_advance_animation=HSD_JObjAnimAll;
    host.article_resource_advance_animation=nativeArticleResource_AdvanceAnimation;
}

static unsigned offset(const NativeFighterStateField* fields,unsigned count,const char* name)
{ for(unsigned i=0;i<count;++i)if(!strcmp(fields[i].name,name))return fields[i].offset; CHECK(false);return 0; }
static void fighter_int(const char* name,int value)
{ memcpy(fighters_data[1].native_module_state+offset(definition->state_fields,definition->state_field_count,name),&value,4); }
static void fighter_float(const char* name,float value)
{ memcpy(fighters_data[1].native_module_state+offset(definition->state_fields,definition->state_field_count,name),&value,4); }
static void fighter_bool(const char* name,bool value)
{ fighters_data[1].native_module_state[offset(definition->state_fields,definition->state_field_count,name)]=value; }
static int item_int(const char* name)
{ int value;memcpy(&value,items_data[1].xDD4_itemVar.native_article.state+offset(authored->state_fields,authored->state_field_count,name),4);return value; }
static float item_float(const char* name)
{ float value;memcpy(&value,items_data[1].xDD4_itemVar.native_article.state+offset(authored->state_fields,authored->state_field_count,name),4);return value; }
static bool item_bool(const char* name,unsigned index)
{ return items_data[1].xDD4_itemVar.native_article.state[offset(authored->state_fields,authored->state_field_count,name)+index]!=0; }
static void set_item_bool(const char* name,unsigned index,bool value)
{ items_data[1].xDD4_itemVar.native_article.state[offset(authored->state_fields,authored->state_field_count,name)+index]=value; }
static void close_float(float stock,float script,const char* field,unsigned index)
{
    float difference=fabsf(stock-script);
    if(difference>max_float_difference)max_float_difference=difference;
    ++checks;
    if (!isfinite(stock)||!isfinite(script)||difference>2e-6f+2e-5f*fmaxf(fabsf(stock),fabsf(script))) {
        fprintf(stderr,"Float mismatch %s[%u]: stock %.9g script %.9g (scenario %u)\n",field,index,stock,script,scenario);fprintf(stderr,"timeline %u mode %u/%u rng %u/%u counts %d first-node active %u\n",base_case,(unsigned)items_data[0].msid,(unsigned)items_data[1].msid,rng_count[0],rng_count[1],(int)tuning[0].x34,links[0][0].x2C_b0);abort();
    }
}
static void compare(void)
{
    CHECK(roots_live[0]==roots_live[1]);
    if(trace_count[0]!=trace_count[1]){fprintf(stderr,"Trace counts %u/%u modes %d/%d\n",trace_count[0],trace_count[1],items_data[0].msid,items_data[1].msid);for(unsigned s=0;s<2;++s){for(unsigned i=0;i<trace_count[s];++i)fprintf(stderr,"side%u event%u %g %g %g %g\n",s,trace[s][i].id,trace[s][i].value[0],trace[s][i].value[1],trace[s][i].value[2],trace[s][i].value[3]);}}
    CHECK(trace_count[0]==trace_count[1]);
    for(unsigned e=0;e<trace_count[0];++e){CHECK(trace[0][e].id==trace[1][e].id);++observed_event[trace[0][e].id];for(unsigned v=0;v<4;++v)close_float(trace[0][e].value[v],trace[1][e].value[v],"event",e);}
    if(!roots_live[0])return;
    unsigned count=(unsigned)tuning[0].x34;
    CHECK((unsigned)item_int("count")==count);
    CHECK(objects_created[0]==objects_created[1] && models_created[0]==models_created[1]);
    close_float(tuning[0].x38,item_float("length"),"length",0);
    close_float(tuning[0].x40,item_float("launch_speed"),"launch",0);
    close_float(items_data[0].scl,items_data[1].scl,"item scale",0);
    CHECK(state_changes[0]==state_changes[1]);
    CHECK(probes[0]==probes[1] && floors[0]==floors[1]);
    for(unsigned i=0;i<count;++i) {
        ItemLink* a=&links[0][i];ItemLink* b=&links[1][i];
        CHECK(a->x1CC==b->x1CC);
        CHECK(a->x2C_b0==item_bool("deployed",i));
        CHECK(a->x2C_b1==item_bool("on_floor",i));

        CHECK(a->coll_data.env_flags==b->coll_data.env_flags);
        for(unsigned c=0;c<3;++c) {
            close_float(((float*)&a->pos)[c],((float*)&b->pos)[c],"position",i);
            close_float(((float*)&a->coll_data.cur_pos)[c],((float*)&b->coll_data.cur_pos)[c],"collision",i);
            close_float(((float*)&a->coll_data.last_pos)[c],((float*)&b->coll_data.last_pos)[c],"previous",i);
        }
        for(unsigned c=0;c<12;++c)close_float(((float*)models[0][i].mtx)[c],((float*)models[1][i].mtx)[c],"matrix",i);
        CHECK((models[0][i].flags&(JOBJ_MTX_DIRTY|0x03800000))==(models[1][i].flags&(JOBJ_MTX_DIRTY|0x03800000)));
        // Native draw callback and indirect animation assets remain a boundary.
    }
    ++source_cases;
}

static void reset(int motion,float scale,float facing)
{
    nativeArticleResource_Clear();
    memset(nodes_gobj,0,sizeof(nodes_gobj));memset(links,0,sizeof(links));memset(models,0,sizeof(models));
    memset(nodes_live,0,sizeof(nodes_live));memset(model_live,0,sizeof(model_live));
    memset(objects_created,0,sizeof(objects_created));memset(links_created,0,sizeof(links_created));
    memset(models_created,0,sizeof(models_created));memset(freed,0,sizeof(freed));
    memset(state_changes,0,sizeof(state_changes));memset(scale_calls,0,sizeof(scale_calls));
    memset(clear_calls,0,sizeof(clear_calls));
    memset(trace,0,sizeof(trace));memset(trace_count,0,sizeof(trace_count));
    memset(probes,0,sizeof(probes));memset(floors,0,sizeof(floors));
    memset(fighters_data,0,sizeof(fighters_data));memset(items_data,0,sizeof(items_data));memset(parts,0,sizeof(parts));
    memset(live_heads,0,sizeof(live_heads));
    parts_table.parts_num=60;tables[family]=&parts_table;
    fail_type=0;fail_index=-1;
    for(unsigned s=0;s<2;++s) {
        tuning[s]=source_tuning;
        fighters_data[s]=(Fighter){.kind=family,.motion_id=motion,.x8_spawnNum=(s32)(0x80000001u+s),.facing_dir=facing,
                                  .cur_pos={2,3,4},.x34_scale={scale,scale,scale},.parts=parts[s],.ft_data=&fighter_data};
        fighters[s]=(HSD_GObj){.user_data=&fighters_data[s]};
        C_MTXIdentity(attachment[s].mtx);attachment[s].flags=0;
        attachment[s].mtx[0][0]=facing*.75f;attachment[s].mtx[0][2]=.25f;
        attachment[s].mtx[1][1]=1.25f;attachment[s].mtx[2][0]=-.5f;
        attachment[s].mtx[0][3]=2;attachment[s].mtx[1][3]=-3;attachment[s].mtx[2][3]=4;
        parts[s][AttachmentPart].joint=&attachment[s];
        items_data[s]=(Item){.kind=item_kind,.x1C=0x81234567u,.owner=&fighters[s],
                            .xCC_item_attr=&common_attribute,.xC4_article_data=&articles[s],
                            .xB8_itemLogicTable=authored->logic,.native_article_author_kind=family};
        root_models[s]=(HSD_JObj){.scale={1,1,1}};
        root_models[s].child=&root_children[s][0];root_children[s][0]=(HSD_JObj){.scale={1,1,1},.child=&root_children[s][1]};root_children[s][1]=(HSD_JObj){.scale={1,1,1}};
        items_gobj[s]=(HSD_GObj){.user_data=&items_data[s],.hsd_obj=&root_models[s]};
        items_data[s].entity=&items_gobj[s];articles[s].x4_specialAttributes=&tuning[s];
        roots_live[s]=false;
    }
    fighters[1].next=&fighters[0];live_heads[HSD_GOBJ_PLINK_FIGHTER]=&fighters[1];
    items_data[1].xDD4_itemVar.native_article.attachment_owner=&fighters[1];
    items_data[1].xDD4_itemVar.native_article.creator_spawn_id=(uint32_t)fighters_data[1].x8_spawnNum;
    memset(rng_count,0,sizeof(rng_count));memset(animations,0,sizeof(animations));
    roots_live[0]=roots_live[1]=true;items_gobj[1].next=&items_gobj[0];live_heads[HSD_GOBJ_PLINK_ITEM]=&items_gobj[1];
    fighter_int("tether_handle",(int)items_data[1].x1C);definition->on_load(&fighters[1]);
    island=(mp_UnkStruct0){.x8={20,30,0},.x14={-20,40,0}};
}

static void set_item_float(const char* name,float value)
{ memcpy(items_data[1].xDD4_itemVar.native_article.state+offset(authored->state_fields,authored->state_field_count,name),&value,4); }
static void set_item_int(const char* name,int value)
{ memcpy(items_data[1].xDD4_itemVar.native_article.state+offset(authored->state_fields,authored->state_field_count,name),&value,4); }
// Constructor/indirect animation import boundary. Defined imported scalar
// derivation is shared fixture setup, not counted as constructor acceptance.
static void construct(void)
{
    float scale=fighters_data[0].x34_scale.y;
    float coeff=fighters_data[0].motion_id==357?source_tuning.x60:source_tuning.x5C;
    float length=source_tuning.x10*scale;
    float total=source_tuning.xC*length;
    if(scale>1)total=source_tuning.x8*total+(1-source_tuning.x8)*(source_tuning.xC*source_tuning.x10);
    unsigned count=(unsigned)((total*coeff)/length);CHECK(count>=2&&count<=90);
    for(unsigned s=0;s<2;++s){
        tuning[s].x34=(int)count;tuning[s].x38=length;tuning[s].x3C=source_tuning.x14*scale;
        tuning[s].x40=coeff*(source_tuning.x18*scale);tuning[s].x44=source_tuning.x1C*scale;
        tuning[s].x4C=coeff*(source_tuning.x24*scale);tuning[s].x54=coeff*(source_tuning.x2C*scale);tuning[s].x58=source_tuning.x30*scale;
    }
    set_item_int("count",count);set_item_int("endpoint",count-1);
    const char* names[]={"length","min_length","launch_speed","gravity","retract_speed","pull_speed","friction"};
    float values[]={tuning[0].x38,tuning[0].x3C,tuning[0].x40,tuning[0].x44,tuning[0].x4C,tuning[0].x54,tuning[0].x58};
    for(unsigned i=0;i<7;++i)set_item_float(names[i],values[i]);
    current_side=0;
    for(unsigned i=0;i<count;++i){
        HSD_GObj* node=GObj_Create(7,10,0);ItemLink* l=HSD_ObjAlloc(&item_link_alloc_data);
        GObj_InitUserData(node,6,it_802A2474,l);l->gobj=node;l->jobj=&attachment[0];
        l->next=i?&links[0][i-1]:NULL;l->prev=i+1<count?&links[0][i+1]:NULL;
        mpColl_80041EE4(&l->coll_data);l->x1CC=-1;
        HSD_Joint* desc=i==count-1?tuning[0].x70:(HSD_Joint*[]){tuning[0].x64,tuning[0].x68,tuning[0].x6C}[i%3];
        HSD_GObjObject_80390A70(node,HSD_GObj_JObjKind,HSD_JObjLoadJoint(desc));
        GObj_SetupGXLink(node,HSD_GObj_JObjCallback,6,0);
    }
    items_data[0].xDD4_itemVar.samusgrapple.x0=&links[0][count-1];
    items_data[0].xDD4_itemVar.samusgrapple.x4=&links[0][0];
    items_data[0].xDD4_itemVar.samusgrapple.x8=&fighters[0];
    fighters_data[0].u.ss.x223C=&items_gobj[0];fighters_data[0].accessory2_cb=it_802BAC80;fighters_data[0].death1_cb=it_802BAC3C;fighters_data[0].accessory3_cb=it_802BACC4;
    current_side=1;CHECK(nativeArticleResource_Begin(&items_gobj[1],count));
    for(unsigned i=0;i<count;++i){
        CHECK(nativeArticleResource_CreateNode(&items_gobj[1],i,&(NativeArticleResourceInit){.attachment_part=AttachmentPart,.radius=i==count-1?1.5f:1.0f,.collision_category=6,.position={0}}));
        CHECK(nativeArticleResource_LoadModel(&items_gobj[1],i,hash(i==count-1?"Endpoint":(const char*[]){"ChainA","ChainB","ChainC"}[i%3])));
        CHECK(nativeArticleResource_SetDraw(&items_gobj[1],i,true));
    }
    CHECK(nativeArticleResource_BindExternalPart(&items_gobj[1],count-1,139));
}

static void configure_nodes(unsigned first,unsigned collision)
{
    unsigned count=(unsigned)tuning[0].x34;
    for(unsigned i=0;i<count;++i)for(unsigned s=0;s<2;++s){
        ItemLink* l=&links[s][i];
        l->pos=(Vec3){2+(float)i*.375f,3+(float)(i%3)*.5f,10+(float)(i%2)*.25f};
        l->vel=(Vec3){.03125f*(float)i,-.0625f,.015625f};
        l->coll_data.cur_pos=l->pos;l->coll_data.last_pos=(Vec3){-7,-8,-9};
        l->x2C_b0=i>=first;l->x2C_b1=collision%3==0&&i%2;l->coll_data.floor.index=7;l->coll_data.floor.normal=(scenario&32)?(Vec3){.25f,.75f,0}:(Vec3){0,1,0};
        l->coll_data.left_facing_wall.index=7;l->coll_data.right_facing_wall.index=9;
        l->x1CC=7;
        if(s){set_item_bool("deployed",i,l->x2C_b0);set_item_bool("on_floor",i,l->x2C_b1);nativeArticleResource_SetDraw(&items_gobj[1],i,true);}
    }
}
static void remaining_matrix(void)
{
    HSD_GObjEvent physics[]={itSamusgrapple_UnkMotion0_Phys,itSamusgrapple_UnkMotion1_Phys,itSamusgrapple_UnkMotion2_Phys,
        itSamusgrapple_UnkMotion3_Phys,itSamusgrapple_UnkMotion4_Phys,itSamusgrapple_UnkMotion5_Phys,
        itSamusgrapple_UnkMotion6_Phys,itSamusgrapple_UnkMotion7_Phys,itSamusgrapple_UnkMotion8_Phys};
    unsigned responses[]={0,1,2,16,32,64,128,256,768};
    for(unsigned mode=0;mode<9;++mode)for(unsigned sc=0;sc<3;++sc)for(unsigned face=0;face<2;++face)
    for(unsigned deployment=0;deployment<3;++deployment)for(unsigned response=0;response<9;++response)
    for(unsigned policy=0;policy<8;++policy)
    for(unsigned launch=0;launch<(mode==1?3:1);++launch){
        // Resource-GObj particles are not available in53: exclude only AirCatch
        // wall-contact dispatch, not its floor/free extension geometry.
        if(mode==1&&launch==2&&(responses[response]==1||responses[response]==16||responses[response]==32||responses[response]==64||responses[response]==256||responses[response]==768))continue;
        scenario=responses[response];++base_case;ub_constraints=0;
        int motion=mode==1?(int[]){212,214,357}[launch]:mode<4?357:358;
        reset(motion,(float[]){.5f,1,1.75f}[sc],face?-1:1);construct();
        unsigned count=(unsigned)tuning[0].x34;configure_nodes(deployment==0?0:deployment==1?count/2:count-1,scenario);
        for(unsigned s=0;s<2;++s){items_data[s].msid=mode;fighters_data[s].ground_or_air=(policy&1)?GA_Ground:GA_Air;
            fighters_data[s].input.pressed_buttons=(policy&2)?HSD_PAD_A:0;fighters_data[s].co_attrs.air_drift_max=1.5f;
            fighters_data[s].self_vel=(Vec3){2.5f,.75f,.5f};fighters_data[s].pos_delta=(Vec3){.25f,.5f,.75f};
            fighters_data[s].motion_id=(policy&4)?100:motion;fighters_data[s].dat_attrs=&fighter_tuning;
            fighters_data[s].mv.ss.grapple.x4=(float[]){-1,0,1,180}[response%4];}
        fighter_float("tether_hang_timer",fighters_data[0].mv.ss.grapple.x4);
        float frame=motion==212?fighter_tuning.xA0:motion==214?fighter_tuning.xB0:fighter_tuning.xC0;
        fighter_float("tether_counter",frame);fighters_data[0].mv.ss.unk7.x0=frame;
        for(unsigned tick=0;tick<3&&roots_live[0];++tick){
            unsigned active=items_data[0].msid;CHECK(active<=8);memset(trace_count,0,sizeof(trace_count));
            current_side=0;physics[active](&items_gobj[0]);current_side=1;authored->logic->states[active].physics_updated(&items_gobj[1]);
            CHECK(item_int("solver")==active);compare();
            current_side=0;fighters_data[0].accessory2_cb(&fighters[0]);current_side=1;fighters_data[1].accessory2_cb(&fighters[1]);
            ++phase_count[mode];compare();CHECK(items_data[0].msid==items_data[1].msid);CHECK(clear_calls[0]==clear_calls[1]);
            CHECK(rng_count[0]==rng_count[1]);CHECK(animations[0]==animations[1]);
            CHECK(fighters_data[0].motion_id==fighters_data[1].motion_id);CHECK(fighters_data[0].ground_or_air==fighters_data[1].ground_or_air);
            for(unsigned axis=0;axis<3;++axis){close_float(((float*)&fighters_data[0].cur_pos)[axis],((float*)&fighters_data[1].cur_pos)[axis],"fighter position",axis);
                close_float(((float*)&fighters_data[0].self_vel)[axis],((float*)&fighters_data[1].self_vel)[axis],"fighter velocity",axis);
                close_float(((float*)&fighters_data[0].pos_delta)[axis],((float*)&fighters_data[1].pos_delta)[axis],"fighter delta",axis);}
            if(!roots_live[0])break;
            for(unsigned i=0;i<count;++i)for(unsigned axis=0;axis<3;++axis)close_float(((float*)&links[0][i].vel)[axis],((float*)&links[1][i].vel)[axis],"velocity",i);
            CHECK(items_data[0].xDD4_itemVar.samusgrapple.x14==item_bool("completed",0));
            if(active==8&&items_data[0].msid==8)close_float(fighters_data[0].mv.ss.grapple.x4,
                ({float v;memcpy(&v,fighters_data[1].native_module_state+offset(definition->state_fields,definition->state_field_count,"tether_hang_timer"),4);v;}),"hang timer",0);
            if(items_data[0].msid==0)break;
        }
        if(ub_constraints)++substituted_cases;
        if(roots_live[0]){current_side=0;fighters_data[0].death1_cb(&fighters[0]);}
        if(roots_live[1]){current_side=1;fighters_data[1].death1_cb(&fighters[1]);}
        CHECK(!roots_live[0]&&!roots_live[1]&&freed[0]==count&&freed[1]==count);
    }
    for(unsigned i=0;i<9;++i)CHECK(phase_count[i]);
}

static void hitlag_matrix(void)
{
    for(unsigned motion_case=0;motion_case<5;++motion_case)for(unsigned sc=0;sc<3;++sc)for(unsigned face=0;face<2;++face)
    for(unsigned deployment=0;deployment<3;++deployment)for(unsigned clock=0;clock<3;++clock){
        scenario=2;int motion=(int[]){212,214,357,358,100}[motion_case];reset(motion,(float[]){.5f,1,1.75f}[sc],face?-1:1);construct();
        unsigned count=tuning[0].x34;configure_nodes(deployment==0?0:deployment==1?count/2:count-1,2);
        float frame=motion==212?fighter_tuning.xA0:motion==214?fighter_tuning.xB0:fighter_tuning.xC0;
        frame+=(float)clock-1;fighters_data[0].mv.ss.unk7.x0=frame;fighter_float("tether_counter",frame);
        current_side=0;fighters_data[0].accessory3_cb(&fighters[0]);current_side=1;fighters_data[1].accessory3_cb(&fighters[1]);
        compare();CHECK(rng_count[0]==rng_count[1]);CHECK(animations[0]==0&&animations[1]==0);++hitlag_cases;
        current_side=0;fighters_data[0].death1_cb(&fighters[0]);current_side=1;fighters_data[1].death1_cb(&fighters[1]);
        CHECK(!roots_live[0]&&!roots_live[1]&&freed[0]==count&&freed[1]==count);
    }
}

static void interaction_matrix(void)
{
    for(unsigned owner=0;owner<3;++owner)for(unsigned other=0;other<3;++other){
        scenario=2;reset(357,1,1);construct();
        for(unsigned side_index=0;side_index<2;++side_index){
            Item* item=&items_data[side_index];item->owner=owner==0?&fighters[side_index]:owner==1?&fighters[1-side_index]:NULL;
            item->xC64_reflectGObj=&fighters[1-side_index];
            HSD_GObj* recipient=other==0?NULL:other==1?&fighters[side_index]:&fighters[1-side_index];
            current_side=side_index;
            if(side_index)authored->logic->evt_unk(&items_gobj[1],recipient);
            else itSamusGrapple_Logic53_EvtUnk(&items_gobj[0],recipient);
        }
        CHECK((items_data[0].xDD4_itemVar.samusgrapple.x8==NULL)==(items_data[1].xDD4_itemVar.native_article.attachment_owner==NULL));
        CHECK(roots_live[0]&&roots_live[1]);CHECK(freed[0]==0&&freed[1]==0);++interaction_cases;
        // Interaction only detaches creator field; no removal is authored here.
        // Ordinary pool cancellation cleans fixture storage independently.
        current_side=0;unsigned count=tuning[0].x34;for(unsigned i=0;i<count;++i)HSD_GObjFree(&nodes_gobj[0][i]);
        current_side=1;nativeArticleResource_CancelGeneration(&items_gobj[1],items_data[1].x1C);
    }
}

static uint8_t* read_file(const char* path,size_t* size)
{ FILE* f=fopen(path,"rb");CHECK(f);fseek(f,0,SEEK_END);*size=(size_t)ftell(f);rewind(f);uint8_t* bytes=malloc(*size);CHECK(bytes&&fread(bytes,1,*size,f)==*size);fclose(f);return bytes; }
static u32 local(const HSD_Dat* dat,u32 slot)
{ u32 target;const char* external;CHECK(HSD_DatResolve(dat,slot,&target,&external)==HSD_DAT_REF_LOCAL);return target; }
static void import_assets(const char* path,const char* root_name)
{
    size_t size;uint8_t* bytes=model_dat_bytes=read_file(path,&size);HSD_Dat dat;
    CHECK(HSD_DatOpen(&dat,bytes,size)==HSD_DAT_OK);u32 root;CHECK(HSD_DatFindRoot(&dat,root_name,&root));
    u32 attribute_source=local(&dat,root+4),table=local(&dat,root+72),descriptor=local(&dat,table+12);
    u32 special=local(&dat,descriptor+4);
    for(unsigned i=0;i<=96;i+=4){u32 value;CHECK(HSD_DatReadU32(&dat,special+i,&value));memcpy((uint8_t*)&source_tuning+i,&value,4);}
    CHECK(HSD_DatReadU32(&dat,attribute_source+160,(u32*)&fighter_tuning.xA0));
    CHECK(HSD_DatReadU32(&dat,attribute_source+176,(u32*)&fighter_tuning.xB0));
    CHECK(HSD_DatReadU32(&dat,attribute_source+192,(u32*)&fighter_tuning.xC0));
    CHECK(HSD_DatReadU32(&dat,attribute_source+204,(u32*)&fighter_tuning.xCC));
    CHECK(HSD_DatReadU32(&dat,attribute_source+208,(u32*)&fighter_tuning.tether_hang_frames));
    fighter_data.ext_attr=&fighter_tuning;common_attribute.x60_scale=1;
    HSD_DatImport import;CHECK(HSD_DatImportInit(&import,&dat,arena,sizeof(arena),65536)==HSD_DAT_OK);
    articles[0]=(Article){.native_special_attribute_source=special};
    CHECK(itArticle_ImportModelRoots(&import,&articles[0],authored->model_roots,authored->model_root_count));
    CHECK(articles[0].native_model_root_count==4);
    HSD_Joint** roots[]={&source_tuning.x64,&source_tuning.x68,&source_tuning.x6C,&source_tuning.x70};
    for(unsigned i=0;i<4;++i)for(unsigned k=0;k<4;++k)if(articles[0].native_model_roots[i].name_hash==hash((const char*[]){"ChainA","ChainB","ChainC","Endpoint"}[k]))*roots[k]=articles[0].native_model_roots[i].root;
    articles[1]=articles[0];CHECK(source_tuning.x64&&source_tuning.x68&&source_tuning.x6C&&source_tuning.x70);
    CHECK(source_tuning.xC==30);
    // Keep the source DAT and imported arena alive for the entire graph lifetime.
}

int main(int argc,char** argv)
{
    if(argc!=5)return 2;
    family=Ft_Kind_Samus;
    item_kind=It_Kind_Samus_GBeam;
    HMODULE library=LoadLibraryA(argv[1]);CHECK(library);
    typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");CHECK(entry);
    typedef int (__cdecl* ParameterEntry)(const uint32_t*, unsigned, uint64_t);
    ParameterEntry bind=(ParameterEntry)GetProcAddress(library,"FreakMelee_SetFighterParameters");CHECK(bind);
    size_t parameter_size;uint8_t* parameter_text=read_file(argv[4],&parameter_size);
    NativeFighterParameters parameters;unsigned parameter_line;
    CHECK(nativeFighterParameters_Parse((const char*)parameter_text,parameter_size,&parameters,&parameter_line));
    uint32_t parameter_bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for(unsigned i=0;i<parameters.count;++i)parameter_bits[i]=parameters.fields[i].bits;
    CHECK(bind(parameter_bits,parameters.count,parameters.schema_hash));free(parameter_text);
    MotionState base_states[400] = {0};
    NativeFighterDefinition base={.motion_states=base_states,.motion_state_count=400,.fighter_root="ftDataSamus"};
    base.special_ground_state[0]=343;base.special_air_state[0]=347;base.special_ground_state[1]=349;base.special_air_state[1]=351;base.special_ground_state[2]=353;base.special_air_state[2]=354;base.special_ground_state[3]=355;base.special_air_state[3]=356;
    initialize_unused_host();definition=entry(&base,&host);CHECK(definition&&definition->article_logic_count==4);
    for(unsigned i=0;i<definition->article_logic_count;++i)if(definition->article_logics[i].kind==item_kind)authored=&definition->article_logics[i];CHECK(authored);CHECK(authored->kind==item_kind&&authored->resource_capacity==90&&authored->resource_reserved_part==139);
    import_assets(argv[2],base.fighter_root);
    remaining_matrix();hitlag_matrix();interaction_matrix();nativeArticleResource_Clear();FreeLibrary(library);free(model_dat_bytes);
    printf("{\"assertions\":%u,\"phase_comparisons\":%u,\"timelines\":%u,\"defined_timelines\":%u,\"interaction_cases\":%u,\"hitlag_cases\":%u,\"substituted_undefined_timelines\":%u,\"max_float_difference\":%.9g,\"state_phases\":[%u,%u,%u,%u,%u,%u,%u,%u,%u]}\n",
        checks,source_cases,base_case,base_case-substituted_cases,interaction_cases,hitlag_cases,substituted_cases,max_float_difference,phase_count[0],phase_count[1],phase_count[2],phase_count[3],phase_count[4],phase_count[5],phase_count[6],phase_count[7],phase_count[8]);
    return 0;
}
