#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftLink/types.h>
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

enum { Nodes = 90, AttachmentPart = 11 };
static HSD_GObj fighters[2], items_gobj[2], nodes_gobj[2][Nodes];
static Fighter fighters_data[2];
static Item items_data[2];
static ItemLink links[2][Nodes];
static HSD_JObj models[2][Nodes], attachment[2], root_models[2];
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
static itLinkHookshotAttributes tuning[2], source_tuning;
static ftLk_DatAttrs fighter_tuning;
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
void HSD_JObjAnimAll(HSD_JObj* j) { (void)j; abort(); }
void HSD_JObjRemoveAnimAll(HSD_JObj* j) { (void)j; abort(); }
void lb_8000BA0C(HSD_JObj* j,float rate) { (void)j; (void)rate; abort(); }
void it_80272F7C(HSD_JObj* j, float value) { j->scale = (Vec3){ value, value, value }; }
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
    c->floor.index = 7;
    return scenario % 3 == 0;
}
bool mpColl_8004B108(CollData* c)
{ ++floors[current_side]; c->cur_pos.y += .375f; c->env_flags = Collide_FloorMask; c->floor.index = 7; return scenario % 4 != 0; }
bool mpGetSpeed(int line, Vec3* position, Vec3* output)
{ (void)line; (void)position; *output = (Vec3){ 0 }; return false; }
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
{ unsigned s=side(object); CHECK(motion >= 0 && motion <= 8 && flags == ITEM_ANIM_UPDATE); items_data[s].msid = motion; ++state_changes[s]; }
static void scale_item(Item_GObj* object, float scale)
{ unsigned s=side(object); ++scale_calls[s]; items_data[s].scl = scale*common_attribute.x60_scale; it_80272F7C(object->hsd_obj,items_data[s].scl); }
void ftColl_8007AFF8(Fighter_GObj* object) { ++clear_calls[side(object)]; }
bool it_8026B894(Item_GObj* object, HSD_GObj* other) { (void)object; (void)other; return false; }
void Item_8026AB54(Item_GObj* object, Fighter_GObj* owner, int bone)
{
    CHECK(bone == AttachmentPart); ((Item*)object->user_data)->owner = owner;
    if (side(object)) authored->logic->picked_up(object);
    else itLinkHookshot_Logic20_PickedUp(object);
}
Item_GObj* Item_80268B18(SpawnItem* spawn)
{
    unsigned s = current_side;
    CHECK(spawn->kind == item_kind && spawn->x0_parent_gobj == &fighters[s]);
    CHECK(spawn->x4_parent_gobj2 == &fighters[s] && spawn->x44_flag.b0);
    if (fail_type == 4) return NULL;
    items_data[s].pos = spawn->pos; items_data[s].x40_vel = spawn->vel;
    items_data[s].facing_dir = spawn->facing_dir;
    roots_live[s] = true;
    live_heads[HSD_GOBJ_PLINK_ITEM] = roots_live[1] ? &items_gobj[1] : &items_gobj[0];
    items_gobj[1].next = roots_live[0] ? &items_gobj[0] : NULL;
    return &items_gobj[s];
}

#include "link_tether_deployed_originals.h"
#include "link_tether_model_import.h"

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
static s32 spawn_attached(Fighter_GObj* owner, ItemKind kind, Fighter_Part bone)
{
    CHECK(owner == &fighters[1] && kind == item_kind && bone == FtPart_RThumbNb);
    SpawnItem spawn = { .kind=kind, .x0_parent_gobj=owner, .x4_parent_gobj2=owner,
        .pos=fighters_data[1].cur_pos, .prev_pos=fighters_data[1].cur_pos,
        .facing_dir=fighters_data[1].facing_dir };
    spawn.x44_flag.b0 = true;
    Item_GObj* object=Item_80268B18(&spawn);
    if (object == NULL) return -1;
    authored->on_created(object);
    if (!roots_live[1]) return -1;
    Item_8026AB54(object,owner,AttachmentPart);
    return (s32)items_data[1].x1C;
}

static NativeFighterHost host = {
    .resolve_bone=ftParts_GetBoneIndex, .spawn_bone_attached_article=spawn_attached,
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
    host.air_accel_normal = (__typeof__(host.air_accel_normal))unused_host;
    host.air_catch_hit_collision = (__typeof__(host.air_catch_hit_collision))unused_host;
    host.air_collision = (__typeof__(host.air_collision))unused_host;
    host.animation_frames_remaining = (__typeof__(host.animation_frames_remaining))unused_host;
    host.article_resource_read_line_speed = (__typeof__(host.article_resource_read_line_speed))unused_host;
    host.bone_count = (__typeof__(host.bone_count))unused_host;
    host.change_motion_state = (__typeof__(host.change_motion_state))unused_host;
    host.check_fastfall = (__typeof__(host.check_fastfall))unused_host;
    host.clamp_self_vel_x = (__typeof__(host.clamp_self_vel_x))unused_host;
    host.clear_hitboxes = (__typeof__(host.clear_hitboxes))unused_host;
    host.enter_cliff_catch = (__typeof__(host.enter_cliff_catch))unused_host;
    host.enter_cliff_jump_phase2 = (__typeof__(host.enter_cliff_jump_phase2))unused_host;
    host.enter_damage_fall = (__typeof__(host.enter_damage_fall))unused_host;
    host.enter_freefall = (__typeof__(host.enter_freefall))unused_host;
    host.enter_landing = (__typeof__(host.enter_landing))unused_host;
    host.fall = (__typeof__(host.fall))unused_host;
    host.fastfall = (__typeof__(host.fastfall))unused_host;
    host.find_ledge_occupant = (__typeof__(host.find_ledge_occupant))unused_host;
    host.ground_to_air = (__typeof__(host.ground_to_air))unused_host;
    host.item_set_joint_value = (__typeof__(host.item_set_joint_value))unused_host;
    host.joint_position = (__typeof__(host.joint_position))unused_host;
    host.play_global_sound = (__typeof__(host.play_global_sound))unused_host;
    host.play_sound = (__typeof__(host.play_sound))unused_host;
    host.probe_ledge = (__typeof__(host.probe_ledge))unused_host;
    host.set_capture_mask = (__typeof__(host.set_capture_mask))unused_host;
    host.spawn_effect = (__typeof__(host.spawn_effect))unused_host;
    host.stage_line_exists = (__typeof__(host.stage_line_exists))unused_host;
    host.stage_segment_intersection = (__typeof__(host.stage_segment_intersection))unused_host;
    host.article_resource_read_line_speed = nativeArticleResource_ReadLineSpeed;
    host.stage_line_exists = mpLib_80054ED8;
    host.clear_hitboxes = ftColl_8007AFF8;
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
        fprintf(stderr,"Float mismatch %s[%u]: stock %.9g script %.9g (scenario %u)\n",field,index,stock,script,scenario);abort();
    }
}
static void compare(void)
{
    CHECK(roots_live[0]==roots_live[1]);
    if(!roots_live[0])return;
    unsigned count=(unsigned)tuning[0].x2C;
    CHECK((unsigned)item_int("count")==count);
    CHECK(objects_created[0]==objects_created[1] && models_created[0]==models_created[1]);
    close_float(tuning[0].x30,item_float("length"),"length",0);
    close_float(tuning[0].x38,item_float("launch_speed"),"launch",0);
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
        draws=0;nodes_gobj[0][i].render_cb(&nodes_gobj[0][i],0);unsigned expected=draws;
        draws=0;nodes_gobj[1][i].render_cb(&nodes_gobj[1][i],0);CHECK(draws==expected);
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
    memset(probes,0,sizeof(probes));memset(floors,0,sizeof(floors));
    memset(fighters_data,0,sizeof(fighters_data));memset(items_data,0,sizeof(items_data));memset(parts,0,sizeof(parts));
    memset(live_heads,0,sizeof(live_heads));
    parts_table.parts_num=family==Ft_Kind_Link?76:80;tables[family]=&parts_table;
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
        items_gobj[s]=(HSD_GObj){.user_data=&items_data[s],.hsd_obj=&root_models[s]};
        items_data[s].entity=&items_gobj[s];articles[s].x4_specialAttributes=&tuning[s];
        roots_live[s]=false;
    }
    fighters[1].next=&fighters[0];live_heads[HSD_GOBJ_PLINK_FIGHTER]=&fighters[1];
    items_data[1].xDD4_itemVar.native_article.attachment_owner=&fighters[1];
    items_data[1].xDD4_itemVar.native_article.creator_spawn_id=(uint32_t)fighters_data[1].x8_spawnNum;
    definition->on_load(&fighters[1]);
    island=(mp_UnkStruct0){.x8={20,30,0},.x14={-20,40,0}};
}

static void construct(void)
{
    current_side=0;
    Item_GObj* object=it_802A2BA4(&fighters[0],&fighters_data[0].cur_pos,fighters_data[0].facing_dir,item_kind);
    fighters_data[0].u.lk.xC=object;
    if(object) {fighters_data[0].accessory2_cb=it_802A7AF0;fighters_data[0].accessory3_cb=it_802A7B34;fighters_data[0].death1_cb=it_802A7AAC;}
    current_side=1;fighter_bool("tether_create_requested",true);definition->on_tick(&fighters[1]);
    if ((object!=NULL)!=roots_live[1]) fprintf(stderr,"Construction mismatch: created %u links %u models %u creator %p count %d scale %g\n",objects_created[1],links_created[1],models_created[1],nativeArticle_Creator(&items_gobj[1]),item_int("count"),fighters_data[1].x34_scale.y);
    CHECK((object!=NULL)==roots_live[1]);
}

static void retract_matrix(void)
{
    float scales[]={.5f,1,1.75f};
    for(unsigned capture=0;capture<2;++capture)
    for(unsigned sc=0;sc<3;++sc)for(unsigned face=0;face<2;++face)
    for(unsigned first_mode=0;first_mode<3;++first_mode)for(unsigned collision=0;collision<8;++collision)
    for(unsigned invalid_motion=0;invalid_motion<2;++invalid_motion) {
        scenario=collision;reset(360,scales[sc],face?-1:1);construct();
        unsigned count=(unsigned)tuning[0].x2C;
        unsigned first=first_mode==0?0:first_mode==1?count/2:count-1;
        for(unsigned i=0;i<count;++i)for(unsigned side_index=0;side_index<2;++side_index) {
            ItemLink* l=&links[side_index][i];
            l->pos=(Vec3){2+(float)i*.375f,3+(float)(i%3)*.5f,10+(float)(i%2)*.25f};
            l->vel=(Vec3){.03125f*(float)i,-.0625f,.015625f};
            l->coll_data.cur_pos=l->pos;l->coll_data.last_pos=(Vec3){-7,-8,-9};
            l->x2C_b0=i>=first;l->x2C_b1=collision%3==0&&i%2;l->coll_data.floor.index=7;
            if(side_index){set_item_bool("deployed",i,l->x2C_b0);set_item_bool("on_floor",i,l->x2C_b1);nativeArticleResource_SetDraw(&items_gobj[1],i,i==count-1||l->x2C_b0);}
        }
        if(invalid_motion)fighters_data[0].motion_id=fighters_data[1].motion_id=100;
        current_side=0;if(capture)it_802A7840(&items_gobj[0]);else it_802A77DC(&items_gobj[0]);
        current_side=1;CHECK(signal_article(&fighters[1],items_data[1].x1C,hash(capture?"retract_capture":"retract"),0));
        current_side=0;if(capture)itLinkhookshot_UnkMotion5_Phys(&items_gobj[0]);else itLinkhookshot_UnkMotion4_Phys(&items_gobj[0]);
        current_side=1;authored->logic->states[capture?5:4].physics_updated(&items_gobj[1]);
        for(unsigned frame=0;frame<4 && roots_live[0];++frame){
            current_side=0;fighters_data[0].accessory2_cb(&fighters[0]);
            current_side=1;fighters_data[1].accessory2_cb(&fighters[1]);compare();
            CHECK(items_data[0].msid==items_data[1].msid);CHECK(clear_calls[0]==clear_calls[1]);
            if(capture && roots_live[0])CHECK((items_data[0].xDD4_itemVar.linkhookshot.x14!=0)==item_bool("completed",0));
            if(!roots_live[0])break;
            for(unsigned i=0;i<count;++i)for(unsigned axis=0;axis<3;++axis)
                close_float(((float*)&links[0][i].vel)[axis],((float*)&links[1][i].vel)[axis],"velocity",i);
            if(items_data[0].msid==0)break;
        }
        if(roots_live[0]){current_side=0;fighters_data[0].death1_cb(&fighters[0]);}
        if(roots_live[1]){current_side=1;fighters_data[1].death1_cb(&fighters[1]);}
        CHECK(!roots_live[0]&&!roots_live[1]&&freed[0]==count&&freed[1]==count);
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
    u32 attribute_source=local(&dat,root+4),table=local(&dat,root+72),descriptor=local(&dat,table+8);
    u32 special=local(&dat,descriptor+4);
    for(unsigned i=0;i<=80;i+=4) {u32 value;CHECK(HSD_DatReadU32(&dat,special+i,&value));memcpy((uint8_t*)&source_tuning+i,&value,4);}
    CHECK(HSD_DatReadU32(&dat,attribute_source+136,(u32*)&fighter_tuning.x88));
    CHECK(HSD_DatReadU32(&dat,attribute_source+152,(u32*)&fighter_tuning.x98));
    CHECK(HSD_DatReadU32(&dat,attribute_source+168,(u32*)&fighter_tuning.xA8));
    fighter_data.ext_attr=&fighter_tuning;common_attribute.x60_scale=1;
    HSD_DatImport import;CHECK(HSD_DatImportInit(&import,&dat,arena,sizeof(arena),65536)==HSD_DAT_OK);
    articles[0]=(Article){.native_special_attribute_source=special};
    CHECK(itArticle_ImportModelRoots(&import,&articles[0],authored->model_roots,authored->model_root_count));
    CHECK(articles[0].native_model_root_count==3);
    for(unsigned i=0;i<3;++i) {
        CHECK(articles[0].native_model_roots[i].root);
        if(articles[0].native_model_roots[i].name_hash==hash("Odd"))source_tuning.x54=articles[0].native_model_roots[i].root;
        if(articles[0].native_model_roots[i].name_hash==hash("Even"))source_tuning.x58=articles[0].native_model_roots[i].root;
        if(articles[0].native_model_roots[i].name_hash==hash("Endpoint"))source_tuning.x5C=articles[0].native_model_roots[i].root;
    }
    articles[1]=articles[0];CHECK(source_tuning.x54&&source_tuning.x58&&source_tuning.x5C);
    CHECK(source_tuning.xC==(family==Ft_Kind_Link?15:10));
    // Keep the source DAT and imported arena alive for the entire graph lifetime.
}

int main(int argc,char** argv)
{
    if(argc!=5)return 2;
    family=!strcmp(argv[3],"link")?Ft_Kind_Link:Ft_Kind_CLink;
    item_kind=family==Ft_Kind_Link?It_Kind_Link_HShot:It_Kind_CLink_HShot;
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
    NativeFighterDefinition base={.motion_states=base_states,.motion_state_count=400,.fighter_root=family==Ft_Kind_Link?"ftDataLink":"ftDataClink"};
    initialize_unused_host();definition=entry(&base,&host);CHECK(definition&&definition->article_logic_count==1);
    authored=&definition->article_logics[0];CHECK(authored->kind==item_kind&&authored->resource_capacity==90&&authored->resource_reserved_part==139);
    import_assets(argv[2],base.fighter_root);
    retract_matrix();nativeArticleResource_Clear();FreeLibrary(library);free(model_dat_bytes);
    printf("{\"assertions\":%u,\"source_cases\":%u,\"safety_cases\":%u,\"max_float_difference\":%.9g}\n",checks,source_cases,safety_cases,max_float_difference);
    return 0;
}
