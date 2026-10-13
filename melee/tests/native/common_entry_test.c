// Actual common entry bodies. Motion, held animation and friction are controlled
// boundaries; production live-registry/generation checks execute unchanged.
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/native_held_article.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <windows.h>
#include <math.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static Fighter fighter;
static HSD_GObj object;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static ftCommonData common;
ftCommonData* p_ftCommonData = &common;
static ftData data;
static struct Fighter_WaitAnimData held_animation;
static NativeFighterDefinition definition, replacement;
static char trace[32];
static unsigned trace_size, phase, hazard, checks;
static bool held, hammer, definition_changed;
static Fighter* grounded;
static HSD_GObj grounded_object;
static unsigned grounded_cases;
static unsigned smash_cases;
static bool angled_metadata;
static unsigned animation_mask, animation_queries;
static FtMotionId queried_motion[4];
static struct Fighter_WaitAnimData angled_animation[4];
#define CHECK(value) do { ++checks; assert(value); } while (0)

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    if (kind != Ft_Kind_CustomFirst) return NULL;
    return definition_changed ? &replacement : &definition;
}
static void record(char value)
{
    CHECK(trace_size + 1 < sizeof(trace));
    trace[trace_size++] = value; trace[trace_size] = 0;
}
static void retire(unsigned at)
{
    if (at != phase) return;
    if (hazard == 1) heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
    else if (hazard == 2) ++fighter.x8_spawnNum;
    else if (hazard == 3) fighter.kind = Ft_Kind_Mario;
    else if (hazard == 4) definition_changed = true;
}
static void authored(Fighter_GObj* gobj)
{ CHECK(gobj == &object); record('E'); retire(3); }
void Fighter_ChangeMotionState(Fighter_GObj* gobj, FtMotionId motion,
    MotionFlags flags, float start, float speed, float blend, Fighter_GObj* other)
{
    CHECK(gobj == &object && start == 0 && speed == 1 && other == NULL);
    (void)flags; (void)blend;
    fighter.motion_id = motion; record('M'); retire(1);
}
void ftAnim_8006EBA4(Fighter_GObj* gobj)
{ CHECK(gobj == &object); record('A'); retire(2); }
bool ftCo_8008A698(Fighter* fp) { CHECK(fp == &fighter); return held; }
struct Fighter_WaitAnimData* ftData_80085FD4(Fighter* fp, FtMotionId motion)
{
    CHECK(fp == &fighter);
    if (!angled_metadata) return &held_animation;
    const FtMotionId query[] = { ftCo_MS_AttackS4S, ftCo_MS_AttackS4LwS,
                                ftCo_MS_AttackLw4, ftCo_MS_AttackHi4 };
    CHECK(animation_queries < 4);
    queried_motion[animation_queries++] = motion;
    for (unsigned i=0;i<4;++i) if (motion == query[i]) {
        angled_animation[i].x8 = !!(animation_mask & (1u << i));
        return &angled_animation[i];
    }
    CHECK(false); return NULL;
}
void ftCo_8008A6D8(Fighter_GObj* gobj, s32 motion)
{ CHECK(gobj == &object); (void)motion; record('H'); retire(2); }
void ftCommon_8007EFC0(Fighter* fp, u32 value)
{ CHECK(fp == &fighter && value == 123); record('F'); }
void ftCommon_8007D7FC(Fighter* fp)
{ CHECK(fp == &fighter); fighter.ground_or_air = GA_Ground; record('G'); }
void ftCo_DownSpot_Enter(Fighter_GObj* gobj)
{ CHECK(gobj == &object); record('D'); }
bool ftCo_800C5240(Fighter_GObj* gobj) { CHECK(gobj == &object); return hammer; }
void ftCo_800C4ED8(Fighter_GObj* gobj) { CHECK(gobj == &object); record('W'); }
ItemKind itGetKind(Item_GObj* item) { (void)item; return It_Kind_Peach_Parasol; }
void it_802BDB94(Item_GObj* item) { (void)item; record('P'); }
// This common-entry fixture has no authored held articles. Production foreign
// author dispatch is exercised separately by held_article_backend_test.
bool nativeHeldArticle_NotifyNeutral(Fighter_GObj* gobj, bool* claimed)
{ CHECK(gobj == &object); if (claimed) *claimed = false; return false; }
void ftLk_AttackAir_800EB3BC(Fighter_GObj* gobj) { CHECK(gobj == &object); record('L'); }
void ftCl_Init_8014919C(Fighter_GObj* gobj) { CHECK(gobj == &object); record('Y'); }
void ftNs_AttackS4_Enter(Fighter_GObj* gobj) { CHECK(gobj == &object); record('N'); }
void ftPe_AttackS4_Enter(Fighter_GObj* gobj) { CHECK(gobj == &object); record('P'); }
void ftGw_AttackS4_Enter(Fighter_GObj* gobj) { CHECK(gobj == &object); record('Q'); }
// Actual inline effect-callback binding executes; callbacks themselves are not
// invoked by this startup fixture.
void efLib_PauseAll(HSD_GObj* gobj) { (void)gobj; CHECK(false); }
void efLib_ResumeAll(HSD_GObj* gobj) { (void)gobj; CHECK(false); }
static void smash_started(Fighter_GObj* gobj)
{
    CHECK(gobj == &object && !fighter.allow_interrupt);
    CHECK(!fighter.cmd_vars[0] && !fighter.throw_flags);
    CHECK(nativeCallbackSource_Alive(gobj,77,NativeCallbackSource_Fighter));
    record('S'); retire(3);
}
static void smash_entered(Fighter_GObj* gobj, float angle)
{
    CHECK(gobj == &object && angle == 0.2f && fighter.facing_dir == -1);
    record('R'); retire(3);
}

static void grounded_authored(Fighter_GObj* gobj)
{
    CHECK(gobj == &grounded_object);
    // Source variables are restored while the common reset flag is still set.
    CHECK(grounded->x2221_b5);
    CHECK(grounded->x2221_b7 && grounded->x2221_b6 && grounded->x2224_b1);
    CHECK(grounded->native_module_state[0] == 7);
    grounded->native_module_state[0] = 23;
    grounded->u.pe.has_float = false;
    record('B');
    if (hazard == 1 || hazard == 5) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
        if (hazard == 5) {
            DWORD previous;
            CHECK(VirtualProtect(grounded,sizeof(*grounded),PAGE_NOACCESS,&previous));
            grounded_object.user_data = (void*)1;
        }
    } else if (hazard == 2) ++grounded->x8_spawnNum;
    else if (hazard == 3) grounded->kind = Ft_Kind_Mario;
    else if (hazard == 4) definition_changed = true;
}

#include "common_entry_actual.inc"

static void reset(void)
{
    memset(&fighter, 0, sizeof(fighter)); memset(heads, 0, sizeof(heads));
    memset(&definition, 0, sizeof(definition));
    object.user_data = &fighter; object.next = NULL;
    fighter.gobj = &object; fighter.kind = Ft_Kind_CustomFirst;
    fighter.x8_spawnNum = 77; fighter.ft_data = &data;
    heads[HSD_GOBJ_PLINK_FIGHTER] = &object;
    common.x5F0 = 123; held_animation.x8 = 1;
    trace_size = 0; trace[0] = 0; phase = hazard = 0;
    held = hammer = definition_changed = false;
    angled_metadata=false; animation_queries=0;
    definition.on_wait_enter = definition.on_crouch_wait_enter =
        definition.on_aerial_attack_enter = authored;
}
static void enter(unsigned mode)
{
    if (mode == 0) ft_8008A348(&object, 0);
    else if (mode == 1) ftCo_SquatWait_Enter_inline(&object, Ft_MF_None, true);
    else ftCo_AttackAir_EnterFromMsid(&object, ftCo_MS_AttackAirLw);
}

static void grounded_reset(FighterKind kind)
{
    DWORD previous;
    CHECK(VirtualProtect(grounded,sizeof(*grounded),PAGE_READWRITE,&previous));
    reset(); memset(grounded,0,sizeof(*grounded));
    grounded_object = (HSD_GObj){ .user_data = grounded };
    heads[HSD_GOBJ_PLINK_FIGHTER] = &grounded_object;
    grounded->kind = kind; grounded->x8_spawnNum = (int32_t)0xFEDCBA98u;
    grounded->ground_or_air = GA_Ground;
    grounded->x2221_b5 = true;
    grounded->x2221_b7 = grounded->x2221_b6 = grounded->x2224_b1 = true;
    grounded->native_module_state[0] = 7;
}

static void grounded_prefix_cases(void)
{
    for (unsigned peach=0;peach<2;++peach) {
        grounded_reset(peach?Ft_Kind_Peach:Ft_Kind_Mario);
        common_entry_grounded_fragment(&grounded_object);
        CHECK(grounded->u.pe.has_float == !!peach);
        CHECK(!grounded->x2221_b5 && !trace_size);
        CHECK(grounded->native_module_state[0] == 7); ++grounded_cases;
    }
    grounded_reset(Ft_Kind_CustomFirst); // Absent hook still performs common reset.
    common_entry_grounded_fragment(&grounded_object);
    CHECK(!grounded->x2221_b5 && !trace_size); ++grounded_cases;
    for (unsigned why=0;why<=5;++why) {
        grounded_reset(Ft_Kind_CustomFirst);
        definition.on_grounded_motion_setup = grounded_authored; hazard = why;
        common_entry_grounded_fragment(&grounded_object);
        CHECK(!strcmp(trace,"B"));
        if (why == 5) { // Make the retired page readable solely for post-call assertions.
            DWORD previous;
            CHECK(VirtualProtect(grounded,sizeof(*grounded),PAGE_READWRITE,&previous));
        }
        CHECK(grounded->native_module_state[0] == 23);
        CHECK(grounded->x2221_b5 == (why != 0));
        CHECK(grounded->x2221_b7 && grounded->x2221_b6 && grounded->x2224_b1);
        ++grounded_cases;
    }
    grounded_reset(Ft_Kind_CustomFirst);
    definition.on_grounded_motion_setup = grounded_authored;
    grounded->ground_or_air = GA_Air;
    common_entry_grounded_fragment(&grounded_object);
    CHECK(!trace_size && grounded->x2221_b5);
    CHECK(grounded->native_module_state[0] == 7); ++grounded_cases;
}
static void smash_entry(unsigned mode)
{
    if (mode == 0) common_S4_enter(&object,0);
    else if (mode == 1) common_Hi4_enter(&object);
    else if (mode == 2) common_Lw4_enter(&object);
    else common_S4_decide(&object,-1,0);
}
static void smash_prefix_cases(void)
{
    for (unsigned mode=0;mode<4;++mode) {
        reset(); definition.on_forward_smash_start=smash_started;
        fighter.allow_interrupt=true; fighter.cmd_vars[0]=3; fighter.throw_flags=5;
        // Angles0 selects the ordinary middle animation, all variants present.
        common.xB8_radians=1; common.xBC_radians=0.5f;
        common.xC0_radians=-0.5f; common.xC4_radians=-1;
        smash_entry(mode);
        CHECK(!strcmp(trace,mode==3?"MAS":"MA"));
        CHECK(!fighter.allow_interrupt);
        CHECK(fighter.motion_id==(mode==1?ftCo_MS_AttackHi4:
                                 mode==2?ftCo_MS_AttackLw4:ftCo_MS_AttackS4S));
        ++smash_cases;
        for (unsigned stop=1;stop<=2;++stop)
        for (unsigned why=1;why<=4;++why) {
            reset(); definition.on_forward_smash_start=smash_started;
            phase=stop; hazard=why; smash_entry(mode);
            CHECK(!strcmp(trace,stop==1?"M":"MA")); ++smash_cases;
        }
    }
    // Full authored entry has priority over the post-common presentation hook.
    reset(); definition.on_forward_smash_enter=smash_entered;
    definition.on_forward_smash_start=smash_started;
    common_S4_decide(&object,-1,0.2f); CHECK(!strcmp(trace,"R")); ++smash_cases;
    // Raw Pika/Pichu effect-callback tail is reached only after a live startup.
    for (unsigned pichu=0;pichu<2;++pichu) {
        for (unsigned stop=0;stop<=2;++stop)
        for (unsigned why=1;why<=4;++why) {
            reset(); fighter.kind=pichu?Ft_Kind_Pichu:Ft_Kind_Pikachu;
            fighter.pre_hitlag_cb=fighter.post_hitlag_cb=authored;
            phase=stop; hazard=why; common_S4_decide(&object,1,0);
            // Definition replacement alone doesn't change NULL raw-kind lookup.
            bool stopped=stop!=0 && why!=4;
            CHECK(!strcmp(trace,!stopped?"MA":stop==1?"M":"MA"));
            CHECK(fighter.pre_hitlag_cb==(stopped?authored:efLib_PauseAll));
            CHECK(fighter.post_hitlag_cb==(stopped?authored:efLib_ResumeAll));
            ++smash_cases;
        }
    }
    for (unsigned kind=0;kind<3;++kind) {
        reset(); fighter.kind=kind==0?Ft_Kind_Ness:kind==1?Ft_Kind_Peach:Ft_Kind_GameWatch;
        common_S4_decide(&object,1,0);
        CHECK(!strcmp(trace,kind==0?"N":kind==1?"P":"Q")); ++smash_cases;
    }
}
static void smash_angles(void)
{
    const float angles[] = {
        -2,nextafterf(-1,-INFINITY),-1,nextafterf(-1,INFINITY),-0.75f,
        nextafterf(-0.5f,-INFINITY),-0.5f,nextafterf(-0.5f,INFINITY),0,
        nextafterf(0.5f,-INFINITY),0.5f,nextafterf(0.5f,INFINITY),0.75f,
        nextafterf(1,-INFINITY),1,nextafterf(1,INFINITY),2
    };
    const FtMotionId query[] = { ftCo_MS_AttackS4S, ftCo_MS_AttackS4LwS,
                                ftCo_MS_AttackLw4, ftCo_MS_AttackHi4 };
    const FtMotionId selected[] = { ftCo_MS_AttackS4Hi, ftCo_MS_AttackS4HiS,
                                   ftCo_MS_AttackS4Lw, ftCo_MS_AttackS4LwS };
    unsigned motions_seen=0;
    for (unsigned a=0;a<sizeof(angles)/sizeof(*angles);++a)
    for (unsigned mask=0;mask<16;++mask)
    for (unsigned facing=0;facing<2;++facing) {
        reset(); angled_metadata=true; animation_mask=mask;
        fighter.allow_interrupt=true; fighter.cmd_vars[0]=7; fighter.throw_flags=9;
        common.xB8_radians=1; common.xBC_radians=0.5f;
        common.xC0_radians=-0.5f; common.xC4_radians=-1;
        bool threshold[] = { angles[a]>1, angles[a]>0.5f,
                             angles[a]<-1, angles[a]<-0.5f };
        FtMotionId expected=ftCo_MS_AttackS4S;
        unsigned queries=0;
        FtMotionId expected_queries[4];
        for (unsigned i=0;i<4;++i) {
            if (!threshold[i]) continue;
            expected_queries[queries++]=query[i];
            if (mask & (1u << i)) { expected=selected[i]; break; }
        }
        common_S4_decide(&object,facing?1:-1,angles[a]);
        CHECK(fighter.motion_id==expected);
        CHECK(animation_queries==queries);
        CHECK(memcmp(queried_motion,expected_queries,queries*sizeof(*query))==0);
        CHECK(!strcmp(trace,"MA"));
        CHECK(!fighter.allow_interrupt && !fighter.cmd_vars[0] && !fighter.throw_flags);
        CHECK(fighter.facing_dir==(facing?1:-1));
        CHECK(expected>=58 && expected<=62);
        motions_seen |= 1u << (expected-58); ++smash_cases;
    }
    CHECK(motions_seen==31); // All five common S4 motion records58..62 reached.
}
int main(void)
{
    grounded = VirtualAlloc(NULL,sizeof(*grounded),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    CHECK(grounded != NULL);
    for (unsigned mode = 0; mode < 3; ++mode) {
        reset(); held = true; enter(mode);
        CHECK(!strcmp(trace, mode == 2 ? "MAE" : "MHFE"));
        if (mode == 2) CHECK(!fighter.allow_interrupt && !fighter.cmd_vars[0] && !fighter.throw_flags);
        for (unsigned stop = 1; stop <= 3; ++stop)
            for (unsigned why = 1; why <= 4; ++why) {
                reset(); held = true; phase = stop; hazard = why; enter(mode);
                CHECK(!strcmp(trace, stop == 1 ? "M" : stop == 2 ?
                    (mode == 2 ? "MA" : "MH") : (mode == 2 ? "MAE" : "MHFE")));
            }
        reset(); definition.on_wait_enter = definition.on_crouch_wait_enter =
            definition.on_aerial_attack_enter = NULL;
        enter(mode); CHECK(!strcmp(trace, mode == 2 ? "MA" : "MF"));
    }
    reset(); ftCo_SquatWait_Enter_inline(&object, Ft_MF_None, false);
    CHECK(!strcmp(trace, "ME"));
    reset(); fighter.x2224_b2 = true; enter(0); CHECK(!strcmp(trace, "D"));
    reset(); hammer = true; enter(0); CHECK(!strcmp(trace, "W"));
    reset(); fighter.ground_or_air = GA_Air; enter(0); CHECK(!strcmp(trace, "GMFE"));
    for (unsigned mode = 0; mode < 2; ++mode)
        for (unsigned young = 0; young < 2; ++young) {
            reset(); fighter.kind = young ? Ft_Kind_CLink : Ft_Kind_Link; enter(mode);
            CHECK(!strcmp(trace, young ? "MFY" : "MFL"));
        }
    reset(); fighter.x5AC.model_num = 3;
    fighter.x5F4_arr[2].idx = 1; fighter.x5F4_arr[2].prev = 0;
    CHECK(nativeFighterVisuals_CurrentGroup(&fighter, 2) == 1);
    CHECK(nativeFighterVisuals_PreviousGroup(&fighter, 2) == 0);
    CHECK(nativeFighterVisuals_PreviousGroup(&fighter, -1) == -1);
    CHECK(nativeFighterVisuals_PreviousGroup(&fighter, 3) == -1);
    fighter.x5AC.model_num = 99;
    CHECK(nativeFighterVisuals_CurrentGroup(&fighter, 12) == -1);
    CHECK(nativeFighterVisuals_PreviousGroup(&fighter, 12) == -1);
    grounded_prefix_cases();
    smash_prefix_cases();
    smash_angles();
    printf("Common entry ordering, fallback, retirement and model-group checks: %u; grounded prefix cases: %u; smash startup cases: %u\n", checks,grounded_cases,smash_cases);
    VirtualFree(grounded,0,MEM_RELEASE);
    return 0;
}
