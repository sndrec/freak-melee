#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/gobj.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
static unsigned created;
static bool valid = true;

static void guard(Fighter_GObj* object, ShieldDesc* desc, HSD_GObjEvent callback)
{
    ++created;
    valid &= desc->bone == 3 && desc->pos.x == 1 && desc->pos.y == 2 &&
        desc->pos.z == 3 && desc->radius == 4 && callback != NULL;
    Fighter* fighter = object->user_data;
    fighter->shield_hit_cb = callback;
    fighter->x221B_b0 = true;
    fighter->x221B_b1 = false;
}

static void unused_callback(HSD_GObj* object) { (void) object; }
static unsigned bone_count(Fighter* fighter) { (void) fighter; return 4; }

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    Fighter* fighter = calloc(1, sizeof(*fighter));
    if (!fighter) return 1;
    HSD_GObj object = { .user_data = fighter };
    MotionState motions[1] = { 0 };
    NativeFighterDefinition base = { .code_key = "guard_probe",
        .fighter_root = "ftDataTest", .joint_count = 3,
        .motion_state_first = 341, .motion_state_count = 1,
        .motion_states = motions };
    // Runtime part 3 exists even though the costume has only three joints.
    NativeFighterHost host = { .create_guard = guard, .bone_count = bone_count };
    for (unsigned form = 0; form < 2; ++form) {
        memset(fighter, 0, sizeof(*fighter));
        HMODULE library = LoadLibraryA(argv[form + 1]);
        Entry entry = library ? (Entry) GetProcAddress(library,
            "FreakMelee_GetFighterModule") : NULL;
        const NativeFighterDefinition* definition = entry ? entry(&base, &host) : NULL;
        if (!definition || !definition->on_load ||
            !definition->on_death) return 1;
        created = 0;
        fighter->shield_hit.skip_update_pos = true;
        definition->on_load(&object);
        valid &= created == 1 && fighter->x221B_b0 && fighter->x221B_b1 &&
            fighter->shield_unk0 == 7 && fighter->shield_unk1 == 8 &&
            !fighter->shield_hit.skip_update_pos && fighter->shield_hit_cb != NULL;
        if (!fighter->shield_hit_cb) return 1;
        fighter->x19A4 = 11;
        fighter->specialn_facing_dir = -1;
        fighter->pre_hitlag_cb = unused_callback;
        fighter->post_hitlag_cb = unused_callback;
        fighter->shield_hit_cb(&object);
        valid &= fighter->facing_dir == -1 && fighter->self_vel.x == 16.5f &&
            !fighter->x221B_b0 && !fighter->x221B_b1 &&
            !fighter->pre_hitlag_cb && !fighter->post_hitlag_cb;
        definition->on_death(&object);
        valid &= created == 1; // Invalid bones/radii never reach the engine.
        FreeLibrary(library);
    }
    free(fighter);
    if (!valid) { fputs("Authored guard contract failed.\n", stderr); return 1; }
    puts("Both script forms bind owned guard reactions and collision metadata.");
    return 0;
}
