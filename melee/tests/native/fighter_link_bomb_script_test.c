#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/native_fighter_module.h>
#include <melee/ft/kinds/ftLink/ftlinkspeciallw.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/it/itCharItems.h>
#include <melee/it/item.h>
#include <melee/it/kinds/itlinkbomb.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/ithitbox.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* ParameterEntry)(const uint32_t*, unsigned, uint64_t);

typedef struct BombEvent {
    int op, a, b;
    float x, y, z;
} BombEvent;
typedef struct Trace { BombEvent events[64]; unsigned count; } Trace;
enum { Change = 1, Descriptor, Advance, Falling, Rotation, StageSweep,
    Grounded, ResetECB, CollisionFacing, BounceGround, FollowGround,
    SlideGround, Release, Stop, Hidden, Expiry, ExpireTick, Effect, Sound,
    Wind, Visible, PersistentVisible, Random, Align, Reflect, Shield, Bounce,
    Interaction, Create, Lifetime, Pickup, FighterMotion, AnimationStart,
    FighterWait, FighterFall, GroundFriction, AirFriction, GroundCheck,
    AirCheck, ToAir, ToGround, ThrowHeld, PickupNotify, JointPosition,
    AdvanceCommands };

static Trace* trace;
static HSD_GObj owners[2];
static Fighter owner_data[2];
static Item items[2];
static HSD_GObj objects[2];
static bool callback_source_alive(HSD_GObj* object, uint32_t generation, uint32_t source_class)
{
    if (object == NULL || object->user_data == NULL) return false;
    for (unsigned i = 0; i < 2; ++i) {
        if (source_class == NativeCallbackSource_Fighter && object == &owners[i])
            return object->user_data == &owner_data[i] &&
                (uint32_t)owner_data[i].x8_spawnNum == generation;
        if (source_class == NativeCallbackSource_Item && object == &objects[i])
            return object->user_data == &items[i] && items[i].x1C == generation;
    }
    return false;
}
static uint32_t fixture_generation;
static HSD_JObj roots[2], fuse_joints[2];
static DynamicBoneTable dynamic[2];
static itLinkBombAttributes attributes;
static ItemAttr common;
static ItemStateDesc descriptors[4];
static ItemStateArray animations = { descriptors, 4 };
static Article article;
static bool activity[2] = { true, true };
static unsigned activity_cursor;
static int collision_flags;
static bool callback_contact, expiry_result;
static bool frames_remaining, fighter_grounded, fighter_landed, spawn_success=true;
static FighterBone parts[80];
static const NativeFighterArticleLogic* authored;
static Item_GObj* creating;
static unsigned failures, comparisons;
void __assert(char* file, u32 line, char* message)
{ fprintf(stderr,"%s:%lu: %s\n",file,line,message); abort(); }

static void event(int op, int a, int b, float x, float y, float z)
{
    if (trace->count == 64) abort();
    trace->events[trace->count++] = (BombEvent){ op, a, b, x, y, z };
}
static Item* data(HSD_GObj* gobj) { return gobj->user_data; }
static Fighter_GObj* fixture_creator(Item_GObj* gobj) { return gobj ? data(gobj)->xDD4_itemVar.native_article.attachment_owner : NULL; }
static HSD_JObj* fuse(HSD_GObj* gobj)
{ return data(gobj)->xBBC_dynamicBoneTable->bones[3]; }

void Item_80268E5C(HSD_GObj* gobj, enum_t state, Item_StateChangeFlags flags)
{
    Item* item = data(gobj);
    event(Change, state, flags, 0, 0, 0);
    item->msid = state;
    /* Model reload intentionally disturbs the fuse transform. The source and
     * authored helper must restore it, rather than merely change a state ID. */
    if (flags & ITEM_ANIM_UPDATE) {
        fuse(gobj)->translate.y = 100 + state;
        fuse(gobj)->rotate.x = 200 + state;
    }
}
void Item_80268D34(HSD_GObj* gobj, ItemStateDesc* descriptor)
{ (void) gobj; event(Descriptor, (int)(descriptor - descriptors), 0, 0, 0, 0); }
void HSD_JObjAnimAll(HSD_JObj* joint)
{ (void) joint; event(Advance, 0, 0, 0, 0, 0); }
bool lb_8000B09C(HSD_JObj* joint)
{ (void) joint; return activity[activity_cursor++ != 0]; }
bool it_80272C6C(Item_GObj* gobj) { return lb_8000B09C(gobj->hsd_obj); }
void it_80275158(Item_GObj* gobj, float lifetime)
{ event(Lifetime, 0, 0, lifetime, 0, 0); data(gobj)->xD44_lifeTimer = lifetime; }
void it_8026BD0C(Item_GObj* gobj) { data(gobj)->xDCD_flag.b2 = true; }
void it_8026BCF4(Item_GObj* gobj) { data(gobj)->xDCD_flag.b2 = false; }
void it_80275414(Item_GObj* gobj) { data(gobj)->xDCD_flag.b7 = true; }
void it_802754A4(Item_GObj* gobj) { data(gobj)->xDCE_flag.b2 = true; }
void it_80275444(Item_GObj* gobj)
{ data(gobj)->xDCD_flag.b5 = data(gobj)->xDCD_flag.b6 = data(gobj)->xDCD_flag.b7 = true; }
void it_802756D0(Item_GObj* gobj) { data(gobj)->xD0C = 2; }
void it_8026B3A8(Item_GObj* gobj) { data(gobj)->xDC8_word.flags.x15 = false; }
void it_8026BD24(Item_GObj* gobj) { data(gobj)->xDD0_flag.b3 = true; }
void it_80274C60(Item_GObj* gobj) { data(gobj)->xDC8_word.flags.xC = false; }
void it_80273454(Item_GObj* gobj)
{ event(Stop, 0, 0, 0, 0, 0); data(gobj)->x40_vel = (Vec3){ 0 }; }
void it_8027518C(Item_GObj* gobj)
{ (void) gobj; event(Expiry, 0, 0, 0, 0, 0); }
bool it_802751D8(Item_GObj* gobj)
{ (void) gobj; event(ExpireTick, 0, 0, 0, 0, 0); return expiry_result; }
void it_8027429C(Item_GObj* gobj, Vec3* velocity)
{
    event(Release, 0, 0, velocity->x, velocity->y, velocity->z);
    data(gobj)->x40_vel = *velocity; data(gobj)->xDC8_word.flags.x13 = false;
}
EF_SpawnResult efSync_Spawn(s32 effect, HSD_GObj* gobj, ...)
{
    va_list args; va_start(args, gobj); Vec3* position = va_arg(args, Vec3*);
    va_end(args); event(Effect, effect, 0, position->x, position->y, position->z);
    return (EF_SpawnResult){ 0 };
}
void Item_8026AE84(Item* item, enum_t sound, u8 pan, u8 volume)
{ (void) item; event(Sound, sound, pan, volume, 0, 0); }
void it_80272A60(Item_GObj* gobj)
{ efSync_Spawn(1038, gobj, &data(gobj)->pos); Item_8026AE84(data(gobj),116,127,64); it_80274C60(gobj); }
void lb_800119DC(Vec3* position, int kind, float strength, float radius, float angle)
{ event(Wind, kind, 0, strength, radius, angle); (void) position; }
void ftParts_80074A4C(Fighter_GObj* gobj, int group, int value)
{ (void) gobj; event(Visible, group, value, 0, 0, 0); }
void ftParts_80074B0C(Fighter_GObj* gobj, int group, int value)
{ (void) gobj; event(PersistentVisible, group, value, 0, 0, 0); }
void ftLk_AttackAir_SetupParts(Fighter_GObj* gobj)
{ ftParts_80074A4C(gobj,2,0); ftParts_80074B0C(gobj,2,0); }
void it_80272860(Item_GObj* gobj, float gravity, float terminal)
{ event(Falling, 0, 0, gravity, terminal, 0); data(gobj)->x40_vel.y -= gravity; }
int it_80274658(Item_GObj* gobj, float rotation)
{ (void) gobj; event(Rotation, 0, 0, rotation, 0, 0); return 0; }
ItemCommonData* it_804D6D28;
s32 it_8026DAA8(Item_GObj* gobj)
{
    event(StageSweep, collision_flags, 0, 0, 0, 0);
    /* Impact thresholds must use the saved incoming velocity. */
    data(gobj)->x40_vel.x *= 0.5f; data(gobj)->x40_vel.y *= 0.5f;
    return collision_flags;
}
void it_802762B0(Item* item)
{ event(Grounded,0,0,0,0,0); item->ground_or_air = GA_Ground; }
void it_80275D5C(Item_GObj* gobj, itECB* ecb)
{ event(ResetECB,0,0,ecb->top,ecb->bottom,ecb->right); data(gobj)->xC1C = *ecb; }
void mpCollSetFacingDir(CollData* coll, int direction)
{ (void) coll; event(CollisionFacing,direction,0,0,0,0); }
void it_8026E15C(Item_GObj* gobj, HSD_GObjEvent callback)
{ event(BounceGround,0,0,0,0,0); if (callback_contact) callback(gobj); }
void it_8026D62C(Item_GObj* gobj, HSD_GObjEvent callback)
{ event(FollowGround,0,0,0,0,0); if (callback_contact) callback(gobj); }
void it_8026E8C4(Item_GObj* gobj, HSD_GObjEvent landed, HSD_GObjEvent left)
{ event(SlideGround,0,0,0,0,0); if (callback_contact) (collision_flags & 1 ? landed : left)(gobj); }
float HSD_Randf(void) { event(Random,0,0,0,0,0); return 0.25f; }
void it_80272980(Item_GObj* gobj)
{ event(Align,0,0,0,0,0); data(gobj)->facing_dir = data(gobj)->x40_vel.x < 0 ? -1 : 1; }
bool it_80273030(Item_GObj* gobj)
{ (void) gobj; event(Reflect,0,0,0,0,0); return true; }
void itColl_BounceOffVictim(Item_GObj* gobj)
{ (void) gobj; event(Bounce,0,0,0,0,0); }
bool itColl_BounceOffShield(Item_GObj* gobj)
{ (void) gobj; event(Shield,0,0,0,0,0); return true; }
bool it_8026B894(Item_GObj* gobj, HSD_GObj* other)
{
    event(Interaction,other == &owners[0],0,0,0,0);
    bool owned = data(gobj)->owner == other;
    if (owned) data(gobj)->owner = NULL;
    return owned;
}
void it_8026BB68(HSD_GObj* owner, Vec3* position)
{ (void) owner; *position = (Vec3){ 4, 5, 6 }; }
Item_GObj* Item_80268B18(SpawnItem* spawn)
{
    event(Create,spawn->kind,spawn->x44_flag.b0,spawn->prev_pos.x,spawn->prev_pos.y,spawn->prev_pos.z);
    if (!spawn_success) return NULL;
    data(creating)->pos = spawn->pos; data(creating)->owner = spawn->x0_parent_gobj;
    data(creating)->facing_dir = spawn->facing_dir;
    return creating;
}
void Item_8026AB54(Item_GObj* gobj, HSD_GObj* owner, Fighter_Part part)
{
    event(Pickup,part,0,0,0,0); data(gobj)->owner = owner;
    data(gobj)->xDC8_word.flags.x13 = true;
    if (gobj == &objects[0]) it_8029DEB0(gobj); else authored->logic->picked_up(gobj);
}

static void hide(Item_GObj* gobj)
{ HSD_JObjSetFlagsAll(gobj->hsd_obj,JOBJ_HIDDEN); }
static void advance(Item_GObj* gobj)
{
    HSD_JObjAnimAll(gobj->hsd_obj);
    event(AdvanceCommands,0,0,0,0,0);
    data(gobj)->x5CC_currentAnimFrame+=1;
}
static void gravity(Item_GObj* gobj)
{ it_80272860(gobj,common.x10_fall_speed,common.x14_fall_speed_max); it_80274658(gobj,it_804D6D28->x68_float); }
static void sound(Item_GObj* gobj, s32 id, u8 pan, u8 volume)
{ Item_8026AE84(data(gobj),id,pan,volume); }
static float joint_value(Item_GObj* gobj, unsigned index, unsigned channel)
{ if (index != 3) abort(); return channel == 1 ? fuse(gobj)->translate.y : fuse(gobj)->rotate.x; }
static void set_joint_value(Item_GObj* gobj, unsigned index, unsigned channel, float value)
{ if (index != 3) abort(); if (channel == 1) HSD_JObjSetTranslateY(fuse(gobj),value); else HSD_JObjSetRotationX(fuse(gobj),value); }
static void descriptor(Item_GObj* gobj, unsigned index)
{ data(gobj)->xD0_itemStateDesc = &descriptors[index]; Item_80268D34(gobj,&descriptors[index]); }
static int32_t create_held(Fighter_GObj* owner, ItemKind kind, Vec3* previous, unsigned bone, float facing)
{
    SpawnItem spawn = { 0 }; spawn.kind = kind; spawn.prev_pos = *previous;
    it_8026BB68(owner,&spawn.pos); spawn.facing_dir = facing;
    spawn.x0_parent_gobj = spawn.x4_parent_gobj2 = owner; spawn.x44_flag.b0 = true;
    Item_GObj* gobj = Item_80268B18(&spawn);
    if(gobj==NULL) { ((Fighter*)owner->user_data)->item_gobj=NULL;return 0; }
    data(gobj)->xDD4_itemVar.native_article.attachment_owner = owner;
    authored->on_created(gobj); Item_8026AB54(gobj,owner,bone);
    ((Fighter*)owner->user_data)->item_gobj=gobj;
    return data(gobj)->x1C;
}
static void noop(Fighter_GObj* gobj) { (void) gobj; }
static bool no_bool(Fighter_GObj* gobj) { (void) gobj; return false; }
static void airborne(Item* item) { item->ground_or_air = GA_Air; }
void HSD_JObjSetFlagsAll(HSD_JObj* joint, u32 flags) { joint->flags |= flags; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void (HSD_JObjSetMtxDirty)(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
static bool ground_ledge(Fighter_GObj* gobj,int direction) { (void) gobj; (void) direction; return false; }
void Fighter_ChangeMotionState(Fighter_GObj* gobj,FtMotionId id,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* other)
{ (void)other;event(FighterMotion,id,flags,frame,rate,blend);Fighter* fighter=gobj->user_data;fighter->motion_id=id;fighter->cur_anim_frame=frame; }
void ftAnim_8006EBA4(Fighter_GObj* gobj) { (void)gobj;event(AnimationStart,0,0,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* gobj) { (void)gobj;return frames_remaining; }
void ft_8008A2BC(Fighter_GObj* gobj) { (void)gobj;event(FighterWait,0,0,0,0,0); }
void ftCo_Fall_Enter(Fighter_GObj* gobj) { (void)gobj;event(FighterFall,0,0,0,0,0); }
void ft_80084F3C(Fighter_GObj* gobj) { (void)gobj;event(GroundFriction,0,0,0,0,0); }
void ft_80084EEC(Fighter_GObj* gobj) { (void)gobj;event(AirFriction,0,0,0,0,0); }
bool ft_80082708(Fighter_GObj* gobj) { (void)gobj;event(GroundCheck,0,0,0,0,0);return fighter_grounded; }
GroundOrAir ft_80081D0C(Fighter_GObj* gobj) { (void)gobj;event(AirCheck,0,0,0,0,0);return fighter_landed?GA_Air:GA_Ground; }
void ftCommon_8007D5D4(Fighter* fighter) { event(ToAir,0,0,0,0,0);fighter->ground_or_air=GA_Air; }
void ftCommon_8007D7FC(Fighter* fighter) { event(ToGround,0,0,0,0,0);fighter->ground_or_air=GA_Ground; }
Fighter_Part ftParts_GetBoneIndex(Fighter* fighter,Fighter_Part part) { (void) fighter; return part==FtPart_LThumbNb?12:part; }
static unsigned bone_count(Fighter* fighter) { (void) fighter; return 80; }
static void between(Fighter_GObj* gobj,s32 id,s32 first,s32 second) { (void) gobj;(void) id;(void) first;(void) second; }
static void fall(Fighter* fighter,float a,float b) { (void) fighter;(void) a;(void) b; }
static void drift(Fighter* fighter,float a,float b,float c) { (void) fighter;(void) a;(void) b;(void) c; }
static void force_air(Fighter* fighter) { (void) fighter; }
static void landing(Fighter_GObj* gobj,bool flag,float lag) { (void) gobj;(void) flag;(void) lag; }
static void freefall(Fighter_GObj* gobj,int a,int b,bool c,float d,float e) { (void) gobj;(void) a;(void) b;(void) c;(void) d;(void) e; }
void ftCo_800957F4(Fighter_GObj* gobj,FtMotionId id) { (void) gobj;event(ThrowHeld,id,0,0,0,0); }
ItemKind itGetKind(Item_GObj* gobj) { return data(gobj)->kind; }
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* position) { (void) joint;(void) offset;event(JointPosition,0,0,0,0,0);*position=(Vec3){1,2,3}; }
static bool owned(Fighter_GObj* owner,s32 number) { (void) number;return ((Fighter*)owner->user_data)->item_gobj!=NULL; }
void ftpickupitem_80094818(Fighter_GObj* gobj,bool held) { (void) gobj;event(PickupNotify,held,0,0,0,0); }
static bool same(const char* phase,Trace* expected,Trace* actual,bool result_expected,bool result_actual)
{
    ++comparisons;
    Item* a=items; Item* b=items+1;
    bool ok = expected->count == actual->count &&
        memcmp(expected->events,actual->events,expected->count*sizeof(BombEvent)) == 0 &&
        result_expected == result_actual && a->msid == b->msid &&
        a->xD44_lifeTimer == b->xD44_lifeTimer && a->facing_dir == b->facing_dir &&
        a->x5CC_currentAnimFrame == b->x5CC_currentAnimFrame &&
        a->xD0_itemStateDesc == b->xD0_itemStateDesc &&
        memcmp(&a->x40_vel,&b->x40_vel,sizeof(Vec3)) == 0 &&
        a->ground_or_air == b->ground_or_air && a->xD0C == b->xD0C &&
        a->xDCD_flag.b2 == b->xDCD_flag.b2 && a->xDCD_flag.b5 == b->xDCD_flag.b5 &&
        a->xDCD_flag.b6 == b->xDCD_flag.b6 && a->xDCD_flag.b7 == b->xDCD_flag.b7 &&
        a->xDCE_flag.b2 == b->xDCE_flag.b2 && a->xDD0_flag.b3 == b->xDD0_flag.b3 &&
        a->xDC8_word.flags.x13 == b->xDC8_word.flags.x13 &&
        a->xDC8_word.flags.x15 == b->xDC8_word.flags.x15 &&
        a->xDC8_word.flags.xC == b->xDC8_word.flags.xC && a->owner == b->owner &&
        fuse_joints[0].translate.y == fuse_joints[1].translate.y &&
        fuse_joints[0].rotate.x == fuse_joints[1].rotate.x &&
        roots[0].flags == roots[1].flags;
    if (!ok) {
        ++failures; fprintf(stderr,"Bomb parity failed %s: events %u/%u state %d/%d life %g/%g fuse %g,%g/%g,%g\n",phase,expected->count,actual->count,a->msid,b->msid,a->xD44_lifeTimer,b->xD44_lifeTimer,fuse_joints[0].translate.y,fuse_joints[0].rotate.x,fuse_joints[1].translate.y,fuse_joints[1].rotate.x);
        for (unsigned i=0;i<expected->count || i<actual->count;++i) {
            BombEvent e=expected->events[i],r=actual->events[i];
            fprintf(stderr,"  %u: %d,%d,%d,%g,%g,%g / %d,%d,%d,%g,%g,%g\n",i,e.op,e.a,e.b,e.x,e.y,e.z,r.op,r.a,r.b,r.x,r.y,r.z);
        }
    }
    return ok;
}
static bool predicate(HSD_GObjPredicate native,HSD_GObjPredicate script,const char* phase)
{
    Trace a={0},b={0}; activity_cursor=0;trace=&a;bool x=native(&objects[0]);
    activity_cursor=0;trace=&b;bool y=script(&objects[1]);return same(phase,&a,&b,x,y);
}
static bool callback(HSD_GObjEvent native,HSD_GObjEvent script,const char* phase)
{
    Trace a={0},b={0};activity_cursor=0;trace=&a;native(&objects[0]);
    activity_cursor=0;trace=&b;script(&objects[1]);return same(phase,&a,&b,false,false);
}
static void reset(ItemKind kind,int state)
{
    memset(items,0,sizeof(items));memset(roots,0,sizeof(roots));memset(fuse_joints,0,sizeof(fuse_joints));
    ++fixture_generation;
    for (unsigned i=0;i<2;++i) {
        items[i].x1C=fixture_generation;
        items[i].kind=kind;items[i].msid=state;items[i].scl=0.75f;items[i].xD44_lifeTimer=250;
        items[i].xC4_article_data=&article;items[i].xCC_item_attr=&common;
        items[i].xBBC_dynamicBoneTable=&dynamic[i];dynamic[i].bones[3]=&fuse_joints[i];
        items[i].x40_vel=(Vec3){0.4f,-0.5f,0};items[i].facing_dir=1;
        items[i].pos=(Vec3){7,8,9};items[i].owner=&owners[0];
        items[i].xDC8_word.flags.x15=true;items[i].xDC8_word.flags.xC=true;
        items[i].xC0C=(itECB){1,2,3,4};items[i].ground_or_air=GA_Air;
        fuse_joints[i].translate.y=2.25f;fuse_joints[i].rotate.x=0.125f;
        objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};items[i].entity=&objects[i];
    }
    items[0].xDD4_itemVar.linkbomb.x10=&owners[0];
    items[0].xDD4_itemVar.linkbomb.x8=(-2.8000002f/(float)(attributes.lifetime-attributes.xC))*items[0].scl;
    items[0].xDD4_itemVar.linkbomb.xC=(-0.15707964f/(float)(attributes.lifetime-attributes.xC))*items[0].scl;
    items[1].xDD4_itemVar.native_article.attachment_owner=&owners[0];
    Trace ignored={0};trace=&ignored;authored->on_created(&objects[1]);
    items[1].xD44_lifeTimer=250;items[1].xDCD_flag.b2=false;
    activity[0]=activity[1]=true;callback_contact=false;collision_flags=0;expiry_result=false;
}

static void constructor(ItemKind kind, unsigned joint, float facing)
{
    reset(kind,0); Trace expected={0},actual={0}; Vec3 previous={1,2,3};
    trace=&expected;creating=&objects[0];
    Item_GObj* result=it_8029DD58(&owners[0],&previous,joint,kind,facing);
    trace=&actual;creating=&objects[1];
    create_held(&owners[0],kind,&previous,joint,facing);
    same("constructor initializes before pickup",&expected,&actual,result!=NULL,true);
}

static void interaction(ItemKind kind,unsigned owner,unsigned other)
{
    reset(kind,2);items[0].owner=items[1].owner=&owners[owner];
    Trace expected={0},actual={0};
    trace=&expected;it_8029FD84(&objects[0],&owners[other]);
    trace=&actual;authored->logic->evt_unk(&objects[1],&owners[other]);
    same("creator cleanup and current-owner interaction",&expected,&actual,false,false);
    if ((items[0].xDD4_itemVar.linkbomb.x10==NULL) !=
        (items[1].xDD4_itemVar.native_article.attachment_owner==NULL)) {
        fputs("Bomb creator ownership mismatch\n",stderr);++failures;
    }
}

static bool fighter_callback(HSD_GObjEvent native,HSD_GObjEvent script,const char* phase)
{
    Trace expected={0},actual={0};trace=&expected;creating=&objects[0];native(&owners[0]);
    trace=&actual;creating=&objects[1];script(&owners[1]);++comparisons;
    Fighter* a=&owner_data[0];Fighter* b=&owner_data[1];
    bool ok=expected.count==actual.count &&
        memcmp(expected.events,actual.events,expected.count*sizeof(BombEvent))==0 &&
        a->motion_id==b->motion_id && a->cur_anim_frame==b->cur_anim_frame &&
        a->ground_or_air==b->ground_or_air && a->throw_flags==b->throw_flags &&
        (a->item_gobj==NULL)==(b->item_gobj==NULL) &&
        (a->accessory4_cb==NULL)==(b->accessory4_cb==NULL);
    if(!ok) {
        ++failures;fprintf(stderr,"Bomb fighter parity failed %s events %u/%u motion %d/%d flags %lu/%lu\n",phase,expected.count,actual.count,a->motion_id,b->motion_id,a->throw_flags,b->throw_flags);
        for(unsigned i=0;i<expected.count || i<actual.count;++i) {
            BombEvent e=expected.events[i],r=actual.events[i];
            fprintf(stderr,"  %u: %d,%d,%d,%g,%g,%g / %d,%d,%d,%g,%g,%g\n",i,e.op,e.a,e.b,e.x,e.y,e.z,r.op,r.a,r.b,r.x,r.y,r.z);
        }
    }
    return ok;
}

static void fighter_reset(ItemKind kind,FighterKind fighter_kind)
{
    reset(kind,0);
    static ftLk_DatAttrs dat_attrs;
    static struct ftData_x8 source_parts;
    static ftData source;
    memset(owner_data,0,sizeof(owner_data));
    parts[12].joint=&roots[0];
    dat_attrs.x48=kind;source_parts.x10=fighter_kind==Ft_Kind_CLink?68:64;source.x8=&source_parts;
    for(unsigned i=0;i<2;++i) {
        owner_data[i].x8_spawnNum=101;
        owner_data[i].kind=fighter_kind;owner_data[i].dat_attrs=&dat_attrs;
        owner_data[i].ft_data=&source;owner_data[i].parts=parts;owner_data[i].x5AC.model_num=3;
        owner_data[i].facing_dir=-1;owner_data[i].cur_anim_frame=5.25f;
        owner_data[i].throw_flags=0xFFFFFFFF;
    }
    frames_remaining=false;fighter_grounded=false;fighter_landed=false;spawn_success=true;
}

int main(int argc,char** argv)
{
    if (argc != 2) return 2;
    HMODULE library=LoadLibraryExA(argv[1],NULL,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library==NULL) return 2;
    ModuleEntry entry=(ModuleEntry)GetProcAddress(library,"FreakMelee_GetFighterModule");
    ParameterEntry bind=(ParameterEntry)GetProcAddress(library,"FreakMelee_SetFighterParameters");
    if (entry==NULL || bind==NULL) return 2;
    MotionState motions[21]={0};
    NativeFighterDefinition base={.source_kind=Ft_Kind_Link,.motion_state_first=341,.motion_state_count=21,.motion_states=motions,.joint_count=75,
        .special_ground_state={[NativeFighterSpecial_Up]=356,[NativeFighterSpecial_Down]=358},
        .special_air_state={[NativeFighterSpecial_Up]=357,[NativeFighterSpecial_Down]=359}};
    NativeFighterHost host={.callback_source_alive=callback_source_alive,.item_creator=fixture_creator,
        .change_motion_state=Fighter_ChangeMotionState,.start_animation=ftAnim_8006EBA4,.animation_frames_remaining=ftAnim_IsFramesRemaining,
        .finish_ground_action=ft_8008A2BC,.ground_friction=ft_80084F3C,.fall=fall,.air_accel_friction=drift,
        .force_air_and_use_jumps=force_air,.ground_collision=ft_80082708,.check_ground_and_ledge=ground_ledge,
        .check_cliff_catch=no_bool,.enter_landing_fall_special=landing,.enter_freefall=freefall,
        .resolve_bone=ftParts_GetBoneIndex,.bone_count=bone_count,.spawn_effect_between_parts=between,
        .pause_effects=noop,.resume_effects=noop,.set_visibility_group=ftParts_80074A4C,
        .set_current_visibility_group=ftParts_80074B0C,.held_item_kind=itGetKind,.throw_held_item=ftCo_800957F4,
        .create_held_article_at_center=create_held,.joint_position=lb_8000B1CC,
        .notify_item_pickup=ftpickupitem_80094818,.owned_article_exists=owned,.enter_fall=ftCo_Fall_Enter,.air_gravity_and_friction=ft_80084EEC,
        .ground_to_air=ftCommon_8007D5D4,.air_to_ground=ftCommon_8007D7FC,.air_collision=ft_80081D0C,
        .item_change_state_flags=Item_80268E5C,.item_set_lifetime=it_80275158,
        .model_animation_active=lb_8000B09C,.item_advance_animation=advance,
        .model_advance_animation=HSD_JObjAnimAll,
        .item_dynamic_joint_value=joint_value,.item_set_dynamic_joint_value=set_joint_value,
        .item_set_animation_descriptor=descriptor,.item_release_attachment=it_8027429C,
        .item_use_manual_lifetime=it_8026B3A8,.item_stop_motion=it_80273454,.item_hide_model=hide,
        .item_begin_expiry=it_8027518C,.item_expire_tick=it_802751D8,.spawn_effect=efSync_Spawn,
        .item_play_sound=sound,.spawn_wind=lb_800119DC,.item_sweep_stage_flags=it_8026DAA8,
        .item_set_grounded=it_802762B0,.item_set_airborne=airborne,.item_reset_default_ecb=it_80275D5C,
        .set_collision_facing=mpCollSetFacingDir,.item_bounce_stage_to_ground=it_8026E15C,
        .item_fall=it_80272860,.item_gravity=gravity,.item_follow_ground=it_8026D62C,
        .item_slide_on_ground=it_8026E8C4,.item_align_facing=it_80272980,.random_float=HSD_Randf,
        .abs_float=fabsf,.item_reflect=it_80273030,.item_shield_bounce=itColl_BounceOffShield,
        .item_bounce=itColl_BounceOffVictim,.item_interact=it_8026B894,
    };
    attributes=(itLinkBombAttributes){.lifetime=300,.xC=96,.x10=6,.x14=.67f,.x18=.6f,.x1C=.26f,.x20=.67f,.x24=.8f,.x28=.7f,.x2C=-.03f,.x30=.03f};
    common.x10_fall_speed=.1f;common.x14_fall_speed_max=2;
    ItemCommonData global={0};global.x68_float=.125f;it_804D6D28=&global;
    article=(Article){.x4_specialAttributes=&attributes,.xC_itemStates=&animations};
    for (unsigned i=0;i<2;++i) {owners[i]=(HSD_GObj){.user_data=&owner_data[i]};owner_data[i].x5AC.model_num=3;owner_data[i].x8_spawnNum=101;}
    const char* values="int spin_effect_joint = 24\nint bomb_holding_joint = 64\n";
    NativeFighterParameters parameters;unsigned line;
    if (!nativeFighterParameters_Parse(values,strlen(values),&parameters,&line)) return 2;
    uint32_t parameter_bits[2]={parameters.fields[0].bits,parameters.fields[1].bits};
    if(!bind(parameter_bits,parameters.count,parameters.schema_hash)) return 2;
    const NativeFighterDefinition* module=entry(&base,&host);
    if (module==NULL) { fputs("Bomb module rejected comparison host\n",stderr); return 2; }
    NativeFighterHost missing_model=host;missing_model.model_advance_animation=NULL;
    if(entry(&base,&missing_model)!=NULL) {
        fputs("Bomb module accepted a missing model-only animation primitive\n",stderr);return 1;
    }
    module=entry(&base,&host);
    if(module==NULL) return 2;
    HSD_GObjPredicate anim[]={itLinkbomb_UnkMotion0_Anim,itLinkbomb_UnkMotion1_Anim,itLinkbomb_UnkMotion2_Anim,itLinkbomb_UnkMotion3_Anim,itLinkbomb_UnkMotion4_Anim,itLinkbomb_UnkMotion5_Anim,itLinkbomb_UnkMotion6_Anim};
    HSD_GObjEvent phys[]={itLinkbomb_UnkMotion0_Phys,itLinkbomb_UnkMotion1_Phys,itLinkbomb_UnkMotion2_Phys,itLinkbomb_UnkMotion3_Phys,itLinkbomb_UnkMotion4_Phys,itLinkbomb_UnkMotion5_Phys,itLinkbomb_UnkMotion6_Phys};
    HSD_GObjPredicate coll[]={NULL,itLinkbomb_UnkMotion1_Coll,itLinkbomb_UnkMotion2_Coll,itLinkbomb_UnkMotion3_Coll,itLinkbomb_UnkMotion4_Coll,itLinkbomb_UnkMotion5_Coll,itLinkbomb_UnkMotion6_Coll};
    for (unsigned family=0;family<2;++family) {
        ItemKind kind=family?It_Kind_CLink_Bomb:It_Kind_Link_Bomb;authored=NULL;
        parameter_bits[0]=family?26:24;parameter_bits[1]=family?68:64;
        if(!bind(parameter_bits,parameters.count,parameters.schema_hash)) return 2;
        for (unsigned i=0;i<module->article_logic_count;++i) if(module->article_logics[i].kind==kind) authored=&module->article_logics[i];
        if (authored==NULL || authored->state_count!=7 || authored->on_out_of_bounds==NULL) return 2;
        constructor(kind,family?68:64,1);constructor(kind,family?68:64,-1);
        itLinkBombAttributes saved_attributes=attributes;
        attributes.lifetime=16777217;attributes.xC=1;
        reset(kind,0);fuse_joints[0].translate.y=fuse_joints[1].translate.y=0;
        fuse_joints[0].rotate.x=fuse_joints[1].rotate.x=0;
        predicate(anim[0],authored->logic->states[0].animated,"integer subtraction precedes fuse-step float conversion");
        attributes=saved_attributes;
        for (unsigned state=0;state<7;++state) {
            char phase[80];snprintf(phase,sizeof(phase),"family%u state%u",family,state);
            reset(kind,state);predicate(anim[state],authored->logic->states[state].animated,phase);
            reset(kind,state);activity[0]=activity[1]=false;predicate(anim[state],authored->logic->states[state].animated,"animation restart");
            reset(kind,state);items[0].xD44_lifeTimer=items[1].xD44_lifeTimer=96;predicate(anim[state],authored->logic->states[state].animated,"fast fuse start");
            if (state!=5) predicate(anim[state],authored->logic->states[state].animated,"fast fuse continuation");
            reset(kind,state);items[0].xD44_lifeTimer=items[1].xD44_lifeTimer=0;predicate(anim[state],authored->logic->states[state].animated,"fuse expiry");
            reset(kind,state);callback(phys[state],authored->logic->states[state].physics_updated,"physics before rolling entry");
            if (coll[state]) { reset(kind,state);predicate(coll[state],authored->logic->states[state].collided,"collision no contact"); }
        }
        for (unsigned state=2;state<=3;++state) for(unsigned threshold=0;threshold<4;++threshold) {
            reset(kind,state);collision_flags=threshold?1:2;
            if(threshold==2) items[0].x40_vel.x=items[1].x40_vel.x=.81f;
            if(threshold==3) items[0].x40_vel.y=items[1].x40_vel.y=-.71f;
            predicate(coll[state],authored->logic->states[state].collided,"saved impact velocity / floor response");
        }
        reset(kind,0);items[0].xDC8_word.flags.x13=items[1].xDC8_word.flags.x13=true;
        callback(it_8029D968,authored->on_out_of_bounds,"bound bounds callback suppresses explosion");
        /* Bounds decisions compare their source dispatcher, not unconditional explosion. */
        reset(kind,1);callback(it_8029D968,authored->on_out_of_bounds,"unbound bounds explosion");
        reset(kind,5);callback(it_8029D968,authored->on_out_of_bounds,"already exploding bounds");
        reset(kind,0);callback(it_8029DEB0,authored->logic->picked_up,"pickup");
        reset(kind,1);callback(it_8029EC34,authored->logic->dropped,"drop");
        reset(kind,1);callback(it_8029E5D0,authored->logic->thrown,"throw");
        reset(kind,4);callback(itLinkBomb_Logic16_EnteredAir,authored->logic->entered_air,"entered air");
        reset(kind,2);predicate(it_8029F960,authored->logic->dmg_dealt,"contact bounce");
        reset(kind,2);predicate(itLinkBomb_Logic16_DmgReceived,authored->logic->dmg_received,"random hop");
        predicate(itLinkBomb_Logic16_DmgReceived,authored->logic->dmg_received,"second hit preserves hop flag");
        reset(kind,2);items[0].xCA0=items[1].xCA0=6;predicate(itLinkBomb_Logic16_DmgReceived,authored->logic->dmg_received,"damage detonation");
        reset(kind,2);predicate(itLinkBomb_Logic16_Reflected,authored->logic->reflected,"reflection");
        reset(kind,2);predicate(itLinkBomb_Logic16_HitShield,authored->logic->hit_shield,"shield hit");
        reset(kind,2);predicate(itLinkBomb_Logic16_ShieldBounced,authored->logic->shield_bounced,"shield bounce");
        for(unsigned contact=0;contact<4;++contact) {
            unsigned state=contact==0?1:contact==1?4:6;
            reset(kind,state);callback_contact=true;collision_flags=contact==3?0:1;
            predicate(coll[state],authored->logic->states[state].collided,"ground contact callback");
        }
        const float rolling_speeds[]={.4f,-.4f,.04f,-.04f,0};
        for(unsigned speed=0;speed<5;++speed) {
            reset(kind,1);callback_contact=true;
            items[0].x40_vel.x=items[1].x40_vel.x=rolling_speeds[speed];
            predicate(coll[1],authored->logic->states[1].collided,"rolling direction entry");
            callback(phys[4],authored->logic->states[4].physics_updated,"rolling acceleration and stop");
        }
        for(unsigned direction=0;direction<2;++direction) {
            reset(kind,2);items[0].xCCC_incDamageDirection=items[1].xCCC_incDamageDirection=direction?1:-1;
            predicate(itLinkBomb_Logic16_DmgReceived,authored->logic->dmg_received,"signed damage hop");
            items[0].x40_vel.x=items[1].x40_vel.x=-.4f;
            predicate(itLinkBomb_Logic16_DmgReceived,authored->logic->dmg_received,"second hit does not reroll");
            reset(kind,2);items[0].x40_vel.x=items[1].x40_vel.x=-.4f;
            predicate(itLinkBomb_Logic16_DmgReceived,authored->logic->dmg_received,"negative velocity random hop");
        }
        for(unsigned owner=0;owner<2;++owner) for(unsigned other=0;other<2;++other) interaction(kind,owner,other);
        for(unsigned owner=0;owner<2;++owner) {
            reset(kind,0);items[0].owner=items[1].owner=&owners[owner];
            items[0].xDC8_word.flags.x13=items[1].xDC8_word.flags.x13=true;
            items[0].xD44_lifeTimer=items[1].xD44_lifeTimer=0;
            predicate(anim[0],authored->logic->states[0].animated,"held detonation after transfer");
        }
        for(unsigned air=0;air<2;++air) {
            HSD_GObjEvent enter=air?ftLk_SpecialAirLw_Enter:ftLk_SpecialLw_Enter;
            HSD_GObjEvent script_enter=air?module->special_air_lw:module->special_lw;
            const MotionState* state=&module->motion_states[(air?359:358)-341];
            for(unsigned held=0;held<4;++held) {
                fighter_reset(kind,family?Ft_Kind_CLink:Ft_Kind_Link);
                if(held) {items[0].kind=items[1].kind=held==1?It_Kind_Link_Bomb:held==2?It_Kind_CLink_Bomb:It_Kind_Capsule;
                    owner_data[0].item_gobj=&objects[0];owner_data[1].item_gobj=&objects[1];}
                fighter_callback(enter,script_enter,"pull / throw bomb / preserve other held item");
            }
            for(unsigned success=0;success<2;++success) {
                fighter_reset(kind,family?Ft_Kind_CLink:Ft_Kind_Link);
                fighter_callback(enter,script_enter,"pull entry");
                owner_data[0].throw_flags=owner_data[1].throw_flags=1;spawn_success=success;
                fighter_callback(owner_data[0].accessory4_cb,owner_data[1].accessory4_cb,"pull accessory creation and allocation failure");
                fighter_callback(owner_data[0].accessory4_cb,owner_data[1].accessory4_cb,"accessory cue consumed once");
            }
            for(unsigned remaining=0;remaining<2;++remaining) {
                fighter_reset(kind,family?Ft_Kind_CLink:Ft_Kind_Link);frames_remaining=remaining;
                fighter_callback(air?ftLk_SpecialAirLw_Anim:ftLk_SpecialLw_Anim,state->anim_cb,"pull animation completion");
            }
            fighter_reset(kind,family?Ft_Kind_CLink:Ft_Kind_Link);
            fighter_callback(air?ftLk_SpecialAirLw_Phys:ftLk_SpecialLw_Phys,state->phys_cb,"pull fighter physics");
            for(unsigned contact=0;contact<2;++contact) {
                fighter_reset(kind,family?Ft_Kind_CLink:Ft_Kind_Link);
                fighter_grounded=fighter_landed=contact;
                fighter_callback(air?ftLk_SpecialAirLw_Coll:ftLk_SpecialLw_Coll,state->coll_cb,"pull surface continuation preserves frame/accessory");
            }
        }
    }
    FreeLibrary(library);if(failures) return 1;
    printf("Bomb stock callback comparison passed %u cases.\n",comparisons);return 0;
}
