// Compare the original enclosure behavior with the shared native implementation.
// Captured-Item coverage lives in the actual-actor script_world_feedback fixture.
// Engine stubs record call order/arguments; no game or module is loaded.
// Original references only normalize native pointer-displaced console aliases.
#include <melee/ft/kinds/ftCommon/ftCo_YoshiEgg.h>
#include <melee/ft/kinds/ftCommon/ftCo_CaptureKoopa.h>
#include <melee/ft/kinds/ftCommon/ftCo_Bury.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftYoshi/ftyoshispecialn.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test-only original getter declarations; production tick getter chain is gone.
float ftYs_SpecialN_GetDatAttr1C(Fighter_GObj*);
float ftYs_SpecialN_GetDatAttr20(Fighter_GObj*);
float ftYs_SpecialN_GetExtAttr28(void);
float ftYs_SpecialN_GetExtAttr2C(void);
float ftYs_SpecialN_8012CD40(void);
float ftYs_SpecialN_GetExtAttr34(void);
bool ftYs_SpecialN_GetExtAttr38(void);
void ftYs_SpecialN_8012CD88(Vec3*);
HSD_Joint* ftYs_SpecialN_8012CDD4(Fighter_GObj*);

#define ftCo_800BBC88 original_hide
#define ftCo_800BBCC0 original_shrink
#define ftCo_800BBED4 original_enter
#define ftCo_YoshiEgg_Anim original_animation
#define ftCo_YoshiEgg_IASA original_interrupt
#define ftCo_YoshiEgg_Phys original_physics
#define ftCo_YoshiEgg_Coll original_collision
#define ftCo_800BC388 original_land
#define ftCo_800BC3AC original_depart
#define ftCo_800BC3D0 original_damage
void original_land(Fighter_GObj*);
void original_depart(Fighter_GObj*);
void original_damage(Fighter_GObj*);
#include "yoshi_enclosure_original.inc"
#undef ftCo_800BBC88
#undef ftCo_800BBCC0
#undef ftCo_800BBED4
#undef ftCo_YoshiEgg_Anim
#undef ftCo_YoshiEgg_IASA
#undef ftCo_YoshiEgg_Phys
#undef ftCo_YoshiEgg_Coll
#undef ftCo_800BC388
#undef ftCo_800BC3AC
#undef ftCo_800BC3D0

// Kirby's original getters select the same roles in its own DAT.
#define ftKb_SpecialNYs_80109260 ftYs_SpecialN_SetupItemVel
#define ftKb_SpecialNYs_80109290 ftYs_SpecialN_GetFacingDir
#define ftKb_SpecialNYs_8010929C ftYs_SpecialN_GetDatAttr18
#define ftKb_SpecialNYs_801092AC ftYs_SpecialN_GetDatAttr1C
#define ftKb_SpecialNYs_801092BC ftYs_SpecialN_GetDatAttr20
#define ftKb_SpecialNYs_801092CC ftYs_SpecialN_GetDatAttr24
#define ftKb_SpecialNYs_801092DC ftYs_SpecialN_GetExtAttr28
#define ftKb_SpecialNYs_801092F4 ftYs_SpecialN_GetExtAttr2C
#define ftKb_SpecialNYs_8010930C ftYs_SpecialN_8012CD40
#define ftKb_SpecialNYs_80109324 ftYs_SpecialN_GetExtAttr34
#define ftKb_SpecialNYs_80109354 ftYs_SpecialN_8012CD88
#define ftKb_SpecialNYs_80109380 ftYs_SpecialN_8012CDB4
#define ftKb_SpecialNYs_801093A0 ftYs_SpecialN_8012CDD4
void fn_8010B124(Fighter_GObj*);
void fn_8010B148(Fighter_GObj*);
void fn_8010B16C(Fighter_GObj*);
void fn_8010B1D4(Fighter_GObj*);
void ftCo_Fall_Enter_YoshiEgg_Kirby(Fighter_GObj*);
int ftKb_SpecialNYs_8010933C(void);
#include "kirby_enclosure_original.inc"
#include "capture_enclosure_boundary.h"
#include <melee/ft/kinds/ftCommon/ftCo_YoshiEgg.c>

typedef struct CaptureEvent { unsigned kind; float a, b, c, d; } CaptureEvent;
static CaptureEvent events[64];
static unsigned event_count, comparisons;
static NativeCaptureEnclosureSettings reference_settings;
static bool mash, forced_escape, collision_event;
static HSD_JObj roots[2], accessories[2], child;
static HSD_Joint model_descriptor;
static ftCo_DatAttrs_xBC_t effect_descriptor;
static ftHurtboxInit hurt_descriptor;
static unsigned fixture_index;

static void require(bool valid, const char* message)
{
    if (valid) return;
    fprintf(stderr, "%s at comparison %u\n", message, comparisons);
    exit(1);
}
static void event(unsigned kind, float a, float b, float c, float d)
{
    require(event_count < 64, "Event overflow");
    events[event_count++] = (CaptureEvent){ kind, a, b, c, d };
}
void __assert(char* file, u32 line, char* text)
{
    fprintf(stderr, "%s:%lu %s\n", file, line, text);
    abort();
}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) {}
void Fighter_UpdateModelScale(Fighter_GObj* object) { event(1,0,0,0,0); }
void ftCo_800BC438(Fighter_GObj* object) { Fighter_UpdateModelScale(object); }
void ftCommon_8007D5D4(Fighter* fp) { event(2,0,0,0,0); fp->ground_or_air=GA_Air; }
void ftCommon_8007D7FC(Fighter* fp) { event(3,0,0,0,0); fp->ground_or_air=GA_Ground; }
void Fighter_ChangeMotionState(Fighter_GObj* object, FtMotionId motion,
                               MotionFlags flags, float frame, float rate,
                               float blend, Fighter_GObj* donor)
{
    event(4,(float)motion,(float)flags,frame,rate);
    event(5,blend,donor!=NULL,0,0);
    GET_FIGHTER(object)->motion_id=motion;
}
void ftCommon_8007E2F4(Fighter* fp, s16 mask) { event(6,mask,0,0,0); }
void ftCommon_SetAccessory(Fighter* fp, HSD_Joint* model)
{
    require(model==&model_descriptor,"Wrong enclosure descriptor");
    event(7,0,0,0,0); fp->x20A0_accessory=&accessories[fixture_index];
}
Fighter_Part ftParts_GetBoneIndex(Fighter* fp, Fighter_Part part) { return part; }
void lb_8000C2F8(HSD_JObj* joint, HSD_JObj* constraint) { event(8,joint==&child,0,0,0); }
void ftColl_8007B0C0(Fighter_GObj* object, HurtCapsuleState status) { event(9,status,0,0,0); }
void ftColl_HurtboxInit(Fighter* fp, FighterHurtCapsule* capsule, ftHurtboxInit* hurt)
{
    hurt_descriptor=*hurt;
    event(10,hurt->bone_idx,hurt->height,hurt->is_grabbable,hurt->scale);
}
void ftColl_8007B62C(Fighter_GObj* object, enum_t status) { event(11,status,0,0,0); }
void ftCommon_8007EFC0(Fighter* fp, u32 mode) { event(12,mode,0,0,0); }
void ftCommon_InitGrab(Fighter* fp, bool mode, float timer) { event(13,mode,timer,0,0); fp->grab_timer=timer; }
bool ftCommon_GrabMash(Fighter* fp, float reduction)
{
    event(14,reduction,0,0,0);
    if (mash) fp->grab_timer-=reduction;
    return mash;
}
void ft_PlaySFX(Fighter* fp, enum_t sound, u8 volume, u8 pan) { event(15,sound,volume,pan,0); }
void efAsync_Spawn(HSD_GObj* object, void* queue, u32 mode, u32 effect, HSD_JObj* joint, ...)
{
    va_list args; va_start(args,joint);
    ftCo_DatAttrs_xBC_t* descriptor=va_arg(args,ftCo_DatAttrs_xBC_t*);
    require(descriptor==&GET_FIGHTER(object)->co_attrs.xBC,"Effect must receive full co_attrs.xBC");
    effect_descriptor=*descriptor; va_end(args);
    event(16,mode,effect,joint==GET_FIGHTER(object)->parts[FtPart_TopN].joint,descriptor->size);
}
void ftColl_8007B760(Fighter_GObj* object, int mode) { event(17,mode,0,0,0); }
void ftCo_Fall_Enter_YoshiEgg(Fighter_GObj* object) { event(18,0,0,0,0); }
void ftCo_Fall_Enter_YoshiEgg_Kirby(Fighter_GObj* object) { event(18,0,0,0,0); }
void ftAnim_SetAnimRate(Fighter_GObj* object, float rate) { event(19,rate,0,0,0); }
void ft_80084F3C(Fighter_GObj* object) { event(20,0,0,0,0); }
void ft_80084DB0(Fighter_GObj* object) { event(21,0,0,0,0); }
void ft_8008403C(Fighter_GObj* object, HSD_GObjEvent callback) { event(22,0,0,0,0); if(collision_event)callback(object); }
void ft_80082C74(Fighter_GObj* object, HSD_GObjEvent callback) { event(23,0,0,0,0); if(collision_event)callback(object); }
bool ftCo_800C0C88(ftCommon_BuryType mode) { event(24,mode,0,0,0); return forced_escape; }

void ftYs_SpecialN_SetupItemVel(Fighter_GObj* object, Vec3* velocity) { *velocity=reference_settings.launch_velocity; }
float ftYs_SpecialN_GetFacingDir(Fighter_GObj* object) { return GET_FIGHTER(object)->facing_dir; }
float ftYs_SpecialN_GetDatAttr18(Fighter_GObj* object) { return reference_settings.damage_behavior; }
float ftYs_SpecialN_GetDatAttr1C(Fighter_GObj* object) { return reference_settings.shrink_fraction; }
float ftYs_SpecialN_GetDatAttr20(Fighter_GObj* object) { return reference_settings.shrink_frames; }
float ftYs_SpecialN_GetDatAttr24(Fighter_GObj* object) { return reference_settings.lifetime; }
float ftYs_SpecialN_GetExtAttr28(void) { return reference_settings.timer_decay; }
float ftYs_SpecialN_GetExtAttr2C(void) { return reference_settings.mash_reduction; }
float ftYs_SpecialN_8012CD40(void) { return reference_settings.mash_pause_frames; }
float ftYs_SpecialN_GetExtAttr34(void) { return reference_settings.mash_animation_rate; }
bool ftYs_SpecialN_GetExtAttr38(void) { return reference_settings.escape_invincibility_frames; }
int ftKb_SpecialNYs_8010933C(void) { return reference_settings.escape_invincibility_frames; }
void ftYs_SpecialN_8012CD88(Vec3* velocity) { *velocity=reference_settings.escape_velocity; }
float ftYs_SpecialN_8012CDB4(void) { return reference_settings.damage_time_scale; }
HSD_Joint* ftYs_SpecialN_8012CDD4(Fighter_GObj* object) { return &model_descriptor; }

static Fighter fighters[2], holder;
static FighterBone parts[2][MAX_FT_PARTS];
static HSD_GObj objects[3];
#define holder_object objects[2]
static CaptureEvent expected_events[64];
static unsigned expected_count;
static void remember(void) { expected_count=event_count; memcpy(expected_events,events,sizeof(events)); event_count=0; }
static void compare_events(void)
{
    require(event_count==expected_count,"Engine call count differs");
    require(memcmp(events,expected_events,event_count*sizeof(CaptureEvent))==0,"Engine call order/arguments differ");
    event_count=0;
}
static void setup(void)
{
    memset(fighters,0,sizeof(fighters)); memset(roots,0,sizeof(roots));
    memset(accessories,0,sizeof(accessories)); event_count=0;
    require(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),
        enclosure_pool_slots,enclosure_pool_order,3,sizeof(HSD_GObj),
        _Alignof(HSD_GObj),++enclosure_pool_epoch),"Physical pool bind failed");
    for(unsigned i=0;i<3;++i){NativeMatchPoolReference reference;
        require(nativeMatchPool_Allocate(&nativeGObjPool,&reference)==&objects[i],"Physical pool allocation order");}
    enclosure_reference_parts.parts_num=MAX_FT_PARTS;
    enclosure_reference_parts.part_to_joint=enclosure_reference_map;
    for(unsigned i=0;i<MAX_FT_PARTS;++i)enclosure_reference_map[i]=(u8)i;
    enclosure_reference_tables[Ft_Kind_Mario]=&enclosure_reference_parts;
    memset(&holder,0,sizeof(holder));holder.facing_dir=-1;holder.gobj=&holder_object;
    holder.kind=Ft_Kind_Mario;holder.x8_spawnNum=1;
    holder_object=(HSD_GObj){.user_data=&holder,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    for(unsigned i=0;i<2;++i) {
        objects[i]=(HSD_GObj){.user_data=&fighters[i],.hsd_obj=&roots[i],.classifier=HSD_GOBJ_CLASS_FIGHTER};
        fighters[i].gobj=&objects[i];fighters[i].kind=Ft_Kind_Mario;
        fighters[i].x8_spawnNum=1;fighters[i].x18=ftCo_MS_Count;
        fighters[i].ground_or_air=GA_Ground;
        fighters[i].parts=parts[i];
        fighters[i].parts[FtPart_TopN].joint=&roots[i];
        fighters[i].parts[FtPart_TransN].joint=&roots[i];
        fighters[i].co_attrs.xBC.size=2.5f;
        fighters[i].co_attrs.xBC.x4=(Vec3){1,2,3};
        fighters[i].co_attrs.xBC.x10=(Vec3){4,5,6};
        fighters[i].co_attrs.xBC.x1C=7;
        roots[i].scale=(Vec3){1.25f,2,0.75f};
        roots[i].flags=JOBJ_MTX_INDEP_SRT;
        accessories[i].flags=JOBJ_MTX_INDEP_SRT; accessories[i].child=&child;
    }
}
static void compare_fighters(void)
{
    ++comparisons;
    require(memcmp(&fighters[0].self_vel,&fighters[1].self_vel,sizeof(Vec3))==0,"Velocity differs");
    require(memcmp(&fighters[0].grab_timer,&fighters[1].grab_timer,sizeof(float))==0,"Grab timer differs");
    require(fighters[0].facing_dir==fighters[1].facing_dir,"Facing differs");
    require(fighters[0].ground_or_air==fighters[1].ground_or_air,"Ground/air differs");
    require(fighters[0].invisible==fighters[1].invisible,"Visibility differs");
    require(fighters[0].dmg.x182c_behavior==fighters[1].dmg.x182c_behavior,"Damage behavior differs");
    require(fighters[0].x1828==fighters[1].x1828,"Damage mode differs");
    compare_events();
}
static void compare_enclosure(void)
{
    const float durations[]={-1,0,0.5f,1,4,17};
    const float fractions[]={-0.2f,0,0.5f,1,1.2f};
    for(unsigned copied=0;copied<2;++copied)
    for(unsigned escape_frames=0;escape_frames<3;++escape_frames)
    for(unsigned started_air=0;started_air<2;++started_air)
    for(unsigned direction=0;direction<2;++direction)
    for(unsigned d=0;d<6;++d) for(unsigned f=0;f<5;++f) {
        setup();
        holder.facing_dir=direction?-1:1;
        fighters[0].ground_or_air=fighters[1].ground_or_air=started_air?GA_Air:GA_Ground;
        reference_settings=(NativeCaptureEnclosureSettings){
            .launch_velocity={-2,3,0},.escape_velocity={7,-4,0},
            .damage_behavior=3,.shrink_fraction=fractions[f],.shrink_frames=durations[d],
            .lifetime=80,.damage_time_scale=1.25f,.timer_decay=0.75f,
            .mash_reduction=2,.mash_pause_frames=1.5f,.mash_animation_rate=0.25f,
            .model_slot=3,.escape_invincibility_frames=escape_frames==2?7:escape_frames};
        fixture_index=0;
        if(copied)ftKb_SpecialNYs_8010AC78(&objects[0],&holder_object);
        else original_enter(&objects[0],&holder_object);
        ftHurtboxInit expected_hurt=hurt_descriptor;
        remember(); fixture_index=1;
        NativeCaptureEnclosureSettings owned=reference_settings;
        if(!copied)owned.escape_invincibility_frames=owned.escape_invincibility_frames!=0;
        require(ftCo_CaptureEnclosure_Enter(&objects[1],&holder_object,
            copied?ftCo_MS_KirbyYoshiEgg:ftCo_MS_YoshiEgg,
            &owned,&model_descriptor,holder.facing_dir),"Shared entry failed");
        compare_fighters();
        require(memcmp(&expected_hurt.a_offset,&hurt_descriptor.a_offset,sizeof(Vec3))==0&&
                memcmp(&expected_hurt.b_offset,&hurt_descriptor.b_offset,sizeof(Vec3))==0,
                "Enclosure hurtbox endpoints differ");
        for(unsigned tick=0;tick<21;++tick) {
            fixture_index=0; fighters[0].accessory4_cb(&objects[0]); remember();
            fixture_index=1;
            if(fighters[1].accessory4_cb)fighters[1].accessory4_cb(&objects[1]);
            require(memcmp(&roots[0].scale,&roots[1].scale,sizeof(Vec3))==0,"Root shrink differs");
            require(memcmp(&accessories[0].scale,&accessories[1].scale,sizeof(Vec3))==0,"Accessory shrink differs");
            require((fighters[0].accessory4_cb==NULL)==(fighters[1].accessory4_cb==NULL),"Shrink callback ends on wrong tick");
            ++comparisons; compare_events();
            if(!fighters[0].accessory4_cb)break;
        }
        for(unsigned tick=0;tick<13;++tick) {
            mash=(tick%3)==0;
            if(copied)ftCo_KirbyYoshiEgg_Anim(&objects[0]); else original_animation(&objects[0]);
            remember(); ftCo_CaptureEnclosure_Anim(&objects[1]); compare_fighters();
            require(fighters[0].mv.co.yoshiegg.x8==fighters[1].capture_enclosure.mash_pause_remaining,"Mash pause differs");
        }
        // Snapshot independence: mutate reference/holder tuning after entry.
        NativeCaptureEnclosureSettings saved=reference_settings;
        holder.capture_enclosure_settings=reference_settings;
        holder.capture_enclosure_settings.timer_decay=1234;
        holder.capture_settings.timer_decay=5678;
        require(fighters[1].capture_enclosure.settings.timer_decay==saved.timer_decay,"Snapshot borrowed holder tuning");
        if(copied)ftCo_KirbyYoshiEgg_Anim(&objects[0]);else original_animation(&objects[0]);
        remember();ftCo_CaptureEnclosure_Anim(&objects[1]);compare_fighters();
        reference_settings=saved;
        for(unsigned force=0;force<2;++force) {
            forced_escape=force; fighters[0].dmg.x18CC=fighters[1].dmg.x18CC=3;
            fighters[0].dmg.x1838_percentTemp=fighters[1].dmg.x1838_percentTemp=5.5f;
            if(copied)fn_8010B16C(&objects[0]);else original_damage(&objects[0]);
            remember();ftCo_CaptureEnclosure_Damage(&objects[1]);compare_fighters();
        }
        if(copied)ftCo_KirbyYoshiEgg_Anim(&objects[0]);else original_animation(&objects[0]);
        ftCo_DatAttrs_xBC_t expected=effect_descriptor;
        remember();ftCo_CaptureEnclosure_Anim(&objects[1]);compare_fighters();
        require(memcmp(&expected,&effect_descriptor,sizeof(expected))==0,"Effect descriptor differs");
        for(unsigned air=0;air<2;++air) {
            fighters[0].ground_or_air=fighters[1].ground_or_air=air?GA_Air:GA_Ground;
            original_physics(&objects[0]);remember();ftCo_CaptureEnclosure_Phys(&objects[1]);compare_fighters();
            collision_event=true;original_collision(&objects[0]);remember();ftCo_CaptureEnclosure_Coll(&objects[1]);compare_fighters();
        }
    }
}

int main(void)
{
    compare_enclosure();
    printf("Capture enclosure original-C comparisons: %u exact samples.\n",comparisons);
    return 0;
}
