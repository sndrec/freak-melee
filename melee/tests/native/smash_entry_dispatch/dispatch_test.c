// Actual extracted dispatch bodies. Engine transitions/stock handlers are
// controlled boundaries; callback-generation lookup links production C.
#include <melee/ft/fighter.h>
#include <melee/ft/inlines.h>
#include <melee/ft/types.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_0DF1.h>
#include <melee/ft/ftswing.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/it/it_26B1.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <windows.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fighter* fighter;
static HSD_GObj* object;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static struct ftCommonData common;
struct ftCommonData* p_ftCommonData = &common;
static NativeFighterDefinition definition;
static bool definition_present, cstick_input, item_throw, extra_throw, item_fire;
static enum_t use_kind;
static HSD_GObj item_object;
static float left_angle = 0.37f, right_angle = -0.61f;
static size_t fighter_bytes, object_bytes;
static int hazard, hook_axis;
static float expected_facing, expected_angle;
static unsigned checks, cases, replacement_cases, hazard_cases, baseline_rejections;
static unsigned definition_queries;
#define CHECK(x) do { ++checks; assert(x); } while (0)
enum { DefaultS=1, DefaultHi, DefaultLw, NessS, NessHi, NessLw,
       PeachS, GameWatchS, Presentation, Throw, Swing, Shoot, Replacement };
static struct Trace { int events[8]; unsigned count; int throw_motion;
    float facing, angle; bool effect_hitlag; } trace;
static unsigned char replacement_fighter[sizeof(Fighter)];
static unsigned char replacement_object[sizeof(HSD_GObj)];

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ CHECK(kind == fighter->kind); ++definition_queries; return definition_present ? &definition : NULL; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* gobj, uint32_t id)
{ (void)gobj; (void)id; abort(); }
bool nativeModelEffect_Snapshot(HSD_GObj* gobj, uint32_t* id)
{ (void)gobj; (void)id; abort(); }
static void record(int event)
{
    CHECK(nativeCallbackSource_Alive(object, 77, NativeCallbackSource_Fighter));
    CHECK(trace.count < 8); trace.events[trace.count++] = event;
    trace.facing = fighter->facing_dir;
}
static void default_entry(int axis, float angle)
{
    record(axis == 0 ? DefaultS : axis == 1 ? DefaultHi : DefaultLw);
    trace.angle = angle; fighter->allow_interrupt = false;
    fighter->cmd_vars[0] = 0; fighter->motion_id = 100 + axis;
}
static void new_s4_default(Fighter_GObj* g, float a) { CHECK(g == object); default_entry(0,a); }
static void old_s4_default(Fighter_GObj* g, float a) { CHECK(g == object); default_entry(0,a); }
static void new_hi4_default(Fighter_GObj* g) { CHECK(g == object); default_entry(1,0); }
static void old_hi4_default(Fighter_GObj* g) { CHECK(g == object); default_entry(1,0); }
static void new_lw4_default(Fighter_GObj* g) { CHECK(g == object); default_entry(2,0); }
static void old_lw4_default(Fighter_GObj* g) { CHECK(g == object); default_entry(2,0); }
void ftNs_AttackS4_Enter(Fighter_GObj* g) { CHECK(g == object); record(NessS); }
void ftNs_AttackHi4_Enter(Fighter_GObj* g) { CHECK(g == object); record(NessHi); }
void ftNs_AttackLw4_Enter(Fighter_GObj* g) { CHECK(g == object); record(NessLw); }
void ftPe_AttackS4_Enter(Fighter_GObj* g) { CHECK(g == object); record(PeachS); }
void ftGw_AttackS4_Enter(Fighter_GObj* g) { CHECK(g == object); record(GameWatchS); }
// The actual Fighter_SetEffectHitlagCallbacks inline assigns these pointers.
// Its body executes unchanged; dispatch does not invoke either effect callback.
void efLib_PauseAll(HSD_GObj* g) { (void)g; abort(); }
void efLib_ResumeAll(HSD_GObj* g) { (void)g; abort(); }
static void presentation(Fighter_GObj* g) { CHECK(g == object); record(Presentation); }
float ftCo_GetLStickAngle(Fighter* f) { CHECK(f == fighter); return left_angle; }
float ftCo_GetCStickAngle(Fighter* f) { CHECK(f == fighter); return right_angle; }
bool ftCo_800DF1C8(Fighter* f) { CHECK(f == fighter); return cstick_input; }
bool ftCo_800DF2D8(Fighter* f) { CHECK(f == fighter); return cstick_input; }
bool ftCo_800DF3A8(Fighter* f) { CHECK(f == fighter); return cstick_input; }
bool ftCo_800DF21C(Fighter* f) { CHECK(f == fighter); return extra_throw; }
bool ftCo_800DF30C(Fighter* f) { CHECK(f == fighter); return extra_throw; }
bool ftCo_800DF3DC(Fighter* f) { CHECK(f == fighter); return extra_throw; }
bool ftCo_80094E54(Fighter* f) { CHECK(f == fighter); return item_throw; }
enum_t itGetUseKind(Item_GObj* g) { CHECK(g == &item_object); return use_kind; }
bool it_8026B594(Item_GObj* g) { CHECK(g == &item_object); return item_fire; }
void ftCo_800957F4(Fighter_GObj* g, FtMotionId m)
{ CHECK(g == object); record(Throw); trace.throw_motion = m; }
void ftCo_Attack_800CCF58(Fighter_GObj* g, enum_t category)
{ CHECK(g == object && category == 2); record(Swing); }
void ftCo_Attack_800CDD14(Fighter_GObj* g) { CHECK(g == object); record(Shoot); }

#include "new_bodies.h"
#include "old_bodies.h"

static void set_protection(DWORD protection)
{
    DWORD previous;
    CHECK(VirtualProtect(fighter, fighter_bytes, protection, &previous));
    CHECK(VirtualProtect(object, object_bytes, protection, &previous));
}
static void mutate_source(void)
{
    if (hazard == 1) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
        set_protection(PAGE_NOACCESS);
    } else if (hazard == 2) {
        memset(fighter, 0xA5, sizeof(*fighter));
        fighter->x8_spawnNum = 78;
        memset(object, 0x5A, sizeof(*object));
        object->user_data = fighter; object->next = NULL;
        memcpy(replacement_fighter, fighter, sizeof(*fighter));
        memcpy(replacement_object, object, sizeof(*object));
    }
}
static void replacement(void)
{
    CHECK(trace.count == 0); // No common entry, presentation or stock handler first.
    CHECK(fighter->allow_interrupt && fighter->cmd_vars[0] == 19);
    CHECK(fighter->motion_id == 17);
    CHECK(fighter->facing_dir == expected_facing);
    CHECK(fighter->pre_hitlag_cb == NULL && fighter->post_hitlag_cb == NULL);
    record(Replacement); ++replacement_cases;
    mutate_source();
}
static void replace_s(Fighter_GObj* g, float angle)
{ CHECK(g == object && hook_axis == 0); CHECK(angle == expected_angle); trace.angle = angle; replacement(); }
static void replace_hi(Fighter_GObj* g)
{ CHECK(g == object && hook_axis == 1); replacement(); }
static void replace_lw(Fighter_GObj* g)
{ CHECK(g == object && hook_axis == 2); replacement(); }

static void reset(FighterKind kind, int hooks, bool present)
{
    set_protection(PAGE_READWRITE);
    memset(fighter,0,sizeof(*fighter)); memset(object,0,sizeof(*object));
    memset(heads,0,sizeof(heads)); memset(&definition,0,sizeof(definition));
    memset(&trace,0,sizeof(trace)); hazard=0; definition_queries=0;
    fighter->kind=kind; fighter->x8_spawnNum=77; fighter->allow_interrupt=true;
    fighter->cmd_vars[0]=19; fighter->motion_id=17; fighter->facing_dir=-1;
    object->user_data=fighter; heads[HSD_GOBJ_PLINK_FIGHTER]=object;
    definition_present=present;
    if (hooks & 1) {
        definition.on_forward_smash_enter=replace_s;
        definition.on_up_smash_enter=replace_hi;
        definition.on_down_smash_enter=replace_lw;
    }
    if (hooks & 2) definition.on_forward_smash_start=presentation;
    cstick_input=item_throw=extra_throw=item_fire=false; use_kind=4;
}
static bool call(int variant, bool old)
{
    switch (variant) {
    case 0: return old ? old_s4_ftCo_AttackS4_CheckInput(object) : new_s4_ftCo_AttackS4_CheckInput(object);
    case 1: return old ? old_s4_ftCo_AttackS4_8008C114(object) : new_s4_ftCo_AttackS4_8008C114(object);
    case 2: return old ? old_hi4_ftCo_AttackHi4_CheckInput(object) : new_hi4_ftCo_AttackHi4_CheckInput(object);
    case 3: return old ? old_hi4_ftCo_AttackHi4_CheckInputNoD0(object) : new_hi4_ftCo_AttackHi4_CheckInputNoD0(object);
    case 4: return old ? old_lw4_ftCo_AttackLw4_CheckInput(object) : new_lw4_ftCo_AttackLw4_CheckInput(object);
    default: abort();
    }
}
static void configure(int variant, unsigned input, unsigned held, float stick, int timer)
{
    fighter->input.pressed_buttons=(input & 1) ? HSD_PAD_A : 0;
    fighter->input.held_buttons[0]=(input & 2) ? HSD_PAD_PROFILE_CSTICK_TILT : 0;
    cstick_input=(input & 4) != 0;
    fighter->input.lstick[0].x=stick; fighter->input.lstick[0].y=stick;
    fighter->input.cstick[0].x=-stick;
    fighter->x670_timer_lstick_tilt_x=timer; fighter->x671_timer_lstick_tilt_y=timer;
    fighter->item_gobj=held ? &item_object : NULL;
    use_kind=held == 1 ? 0 : held == 2 ? 2 : held == 3 ? 3 : 4;
    item_throw=held == 5; extra_throw=held == 6; item_fire=held == 7;
    if (held == 8) fighter->input.held_buttons[0] |= HSD_PAD_LR;
    hook_axis=variant < 2 ? 0 : variant < 4 ? 1 : 2;
    expected_facing=-1; expected_angle=0;
    if (variant < 2) {
        bool left = variant == 0 ? ((input & 1) && fabsf(stick)>=common.dash_smash_stick_threshold && timer<common.dash_smash_window)
            : ((input & 1) && stick * -1 >= common.dash_smash_stick_threshold);
        expected_facing=left ? (variant == 0 ? (stick>=0 ? 1 : -1) : -1) : (-stick>=0 ? 1 : -1);
        expected_angle=left ? left_angle : right_angle;
    }
}
static bool is_item_route(const struct Trace* t)
{ return t->count && (t->events[0] == Throw || t->events[0] == Swing || t->events[0] == Shoot); }

static void matrix(void)
{
    FighterKind kinds[]={Ft_Kind_CustomFirst,Ft_Kind_Ness,Ft_Kind_Peach,
        Ft_Kind_GameWatch,Ft_Kind_Pikachu,Ft_Kind_Pichu};
    float sticks[]={-1,-0.6f,0,0.6f,1};
    for(int variant=0;variant<5;variant++)
    for(unsigned input=0;input<8;input++)
    for(unsigned held=0;held<9;held++)
    for(unsigned si=0;si<5;si++)
    for(int timer=0;timer<2;timer++)
    for(unsigned ki=0;ki<6;ki++)
    for(unsigned mode=0;mode<5;mode++) {
        // mode0 no Definition,1 empty,2 presentation,3 replacement,4 both.
        bool present=mode!=0; int hooks=mode==2?2:mode==3?1:mode==4?3:0;
        reset(kinds[ki],hooks,present); configure(variant,input,held,sticks[si],timer?20:0);
        bool old_result=call(variant,true);
        trace.effect_hitlag=fighter->pre_hitlag_cb==efLib_PauseAll && fighter->post_hitlag_cb==efLib_ResumeAll;
        struct Trace old_trace=trace;
        reset(kinds[ki],hooks,present); configure(variant,input,held,sticks[si],timer?20:0);
        bool new_result=call(variant,false);
        trace.effect_hitlag=fighter->pre_hitlag_cb==efLib_PauseAll && fighter->post_hitlag_cb==efLib_ResumeAll;
        CHECK(new_result==old_result); ++cases;
        bool replaces=present && (hooks & 1) && old_result && !is_item_route(&old_trace);
        if (!old_result || is_item_route(&old_trace)) CHECK(definition_queries==0);
        else CHECK(definition_queries==1);
        if (replaces) {
            CHECK(trace.count==1 && trace.events[0]==Replacement);
            CHECK(old_trace.count && old_trace.events[0]!=Replacement);
            ++baseline_rejections;
        } else CHECK(memcmp(&trace,&old_trace,sizeof(trace))==0);
        if (!old_result) CHECK(trace.count==0);
    }
}
static void invalidation(void)
{
    for(int variant=0;variant<5;variant++)
    for(int kind=0;kind<2;kind++)
    for(int mode=1;mode<=2;mode++) {
        reset(kind?Ft_Kind_Ness:Ft_Kind_CustomFirst,3,true);
        configure(variant,1,0,variant<2?-1:variant<4?1:-1,0);
        hazard=mode; CHECK(call(variant,false)); ++hazard_cases;
        if (mode==1) set_protection(PAGE_READWRITE);
        else {
            CHECK(memcmp(fighter,replacement_fighter,sizeof(*fighter))==0);
            CHECK(memcmp(object,replacement_object,sizeof(*object))==0);
            CHECK(!nativeCallbackSource_Alive(object,77,NativeCallbackSource_Fighter));
        }
        CHECK(trace.count==1 && trace.events[0]==Replacement);
    }
}
int main(void)
{
    SYSTEM_INFO info; GetSystemInfo(&info);
    fighter_bytes=(sizeof(Fighter)+info.dwPageSize-1)&~((size_t)info.dwPageSize-1);
    object_bytes=(sizeof(HSD_GObj)+info.dwPageSize-1)&~((size_t)info.dwPageSize-1);
    fighter=VirtualAlloc(NULL,fighter_bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    object=VirtualAlloc(NULL,object_bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    CHECK(fighter && object);
    common.dash_smash_stick_threshold=0.6f; common.dash_smash_window=5;
    common.xCC=0.6f; common.xD0=5; common.xD4=-0.6f; common.xD8=5;
    matrix(); invalidation();
    CHECK(baseline_rejections && replacement_cases && hazard_cases==20);
    printf("{\"checks\":%u,\"input_cases\":%u,\"replacement_cases\":%u,\"baseline42_rejections\":%u,\"protected_or_recycled_cases\":%u}\n",
        checks,cases,replacement_cases,baseline_rejections,hazard_cases);
    VirtualFree(fighter,0,MEM_RELEASE); VirtualFree(object,0,MEM_RELEASE);
    return 0;
}
