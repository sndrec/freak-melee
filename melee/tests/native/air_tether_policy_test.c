#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ft/inlines.h>
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fighter fighter;
static HSD_GObj object;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static NativeFighterDefinition definition, replacement;
static const NativeFighterDefinition* active_definition;
static unsigned stock_entries, hook_calls, checks;
static unsigned invalidation;
static bool hook_result;
#define CHECK(v) do { ++checks; if (!(v)) { fprintf(stderr, "Air tether line %d\n", __LINE__); return 1; } } while (0)
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return active_definition; }
void ftCo_800C3BE8(Fighter_GObj* source)
{ if (source != &object) abort(); ++stock_entries; }
static bool authored_input(Fighter_GObj* source)
{
    ++hook_calls;
    switch (invalidation) {
    case 1: heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; break;
    case 2: fighter.x8_spawnNum++; break;
    case 3: fighter.kind++; break;
    case 4: active_definition = &replacement; break;
    case 5: source->user_data = NULL; break;
    }
    return hook_result;
}
static void reset(void)
{
    memset(&fighter, 0, sizeof(fighter)); memset(&object, 0, sizeof(object));
    fighter.kind = Ft_Kind_Link; fighter.x8_spawnNum = UINT32_C(0xFEDCBA98);
    object.user_data = &fighter; heads[HSD_GOBJ_PLINK_FIGHTER] = &object;
    stock_entries = hook_calls = invalidation = 0; hook_result = false;
    active_definition = NULL; definition.on_air_tether_input = NULL;
}
// Controlled stage boundary captures the actual copied ECB sent to the real
// left/right solver. The engine solver itself is not claimed by this fixture.
static CollData observed;
static unsigned probe_side;
static bool probe_result;
bool mpColl_80044164(CollData* collision, int* id)
{ observed = *collision; probe_side = 1; *id = 17; return probe_result; }
bool mpColl_800443C4(CollData* collision, int* id)
{ observed = *collision; probe_side = 2; *id = 19; return probe_result; }
#include "air_tether_stock_input.inc"
#include "air_tether_actual.inc"

int main(void)
{
    // Every shared stock gate, including ground/air, with both no Definition
    // and a Definition whose optional hook is absent.
    const FighterKind kinds[] = { Ft_Kind_Link, Ft_Kind_CLink, Ft_Kind_Samus, Ft_Kind_Mario, Ft_Kind_CustomFirst };
    for (unsigned k = 0; k < 5; ++k)
    for (unsigned mask = 0; mask < 256; ++mask)
    for (unsigned def = 0; def < 2; ++def) {
        reset(); fighter.kind = kinds[k]; fighter.used_tether = (mask & 1) != 0;
        fighter.accessory2_cb = mask & 2 ? ftCo_800C3BE8 : NULL;
        fighter.death1_cb = mask & 4 ? ftCo_800C3BE8 : NULL;
        fighter.accessory3_cb = mask & 8 ? ftCo_800C3BE8 : NULL;
        fighter.item_gobj = mask & 16 ? &object : NULL;
        fighter.input.held_buttons[0] = mask & 32 ? HSD_PAD_LR : 0;
        fighter.input.pressed_buttons = mask & 64 ? HSD_PAD_A : 0;
        fighter.ground_or_air = mask & 128 ? GA_Air : GA_Ground;
        active_definition = def ? &definition : NULL;
        Fighter initial = fighter;
        bool original = stock_air_tether_input(&object);
        unsigned entries = stock_entries; bool used = fighter.used_tether;
        fighter = initial; stock_entries = 0;
        CHECK(ftCo_800C3B10(&object) == original);
        CHECK(stock_entries == entries && fighter.used_tether == used && hook_calls == 0);
    }
    for (unsigned result = 0; result < 2; ++result)
    for (unsigned invalid = 0; invalid < 6; ++invalid) {
        reset(); active_definition = &definition;
        definition.on_air_tether_input = authored_input;
        // The script owns the full predicate, even if a stock gate would veto.
        fighter.used_tether = true; fighter.kind = Ft_Kind_CustomFirst;
        hook_result = result; invalidation = invalid;
        CHECK(ftCo_800C3B10(&object) == (invalid ? true : (bool)result));
        CHECK(hook_calls == 1 && stock_entries == 0);
    }
    for (unsigned side = 0; side < 3; ++side)
    for (unsigned hit = 0; hit < 2; ++hit) {
        reset(); fighter.facing_dir = side == 0 ? 1 : side == 1 ? -1 : 0;
        fighter.coll_data.ledge_snap_y = 3; fighter.coll_data.ledge_snap_height = 7;
        fighter.coll_data.env_flags = 0x123; fighter.self_vel = (Vec3){ 4, 5, 6 };
        probe_result = hit; Fighter initial = fighter;
        bool original = ftCo_800C3A14(&object);
        CollData stock_observed = observed, stock_output = fighter.coll_data;
        unsigned stock_side = probe_side;
        fighter = initial;
        CHECK(nativeFighterGeometry_ProbeLedge(&object, 5, 5) == original);
        CHECK(!memcmp(&observed, &stock_observed, sizeof(observed)) && probe_side == stock_side);
        CHECK(!memcmp(&fighter.coll_data, &stock_output, sizeof(stock_output)));
        CHECK(fighter.self_vel.x == 4 && fighter.self_vel.y == 5 && fighter.self_vel.z == 6);
        CHECK(nativeFighterGeometry_ProbeLedge(&object, -2, 11) == original);
        CHECK(observed.ledge_snap_y == 1 && observed.ledge_snap_height == 18);
    }
    CHECK(!nativeFighterGeometry_ProbeLedge(NULL, 5, 5));
    CHECK(!nativeFighterGeometry_ProbeLedge(&object, NAN, 5));
    CHECK(!nativeFighterGeometry_ProbeLedge(&object, 5, INFINITY));
    printf("Shared air tether policy/ECB checks: %u\n", checks);
    return 0;
}
