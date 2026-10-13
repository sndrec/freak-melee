#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#if CALLBACK_LIFETIME_CALLBACKS
#include "script_callback_lifetime_callback_fixture.h"
#else
#include "script_callback_lifetime_action_fixture.h"
#endif

static HSD_GObj fighter_object, item_object;
static uint32_t fighter_generation, item_generation, target_id;
static bool fighter_live, item_live, dispatch_result, kill_item, recycle;
static unsigned operations, kill_at, guards, jump_checks, expire_checks;
static unsigned transfer_owner;
static bool creator_present;

static uint32_t name_hash(const char* name)
{
    uint32_t hash = UINT32_C(2166136261);
    while (*name) { hash ^= (unsigned char)*name++; hash *= UINT32_C(16777619); }
    return hash;
}

static void boundary(void)
{
    if (++operations != kill_at) return;
    if (kill_item) {
        item_live = false;
        if (recycle) { item_live = true; ++((Item*)item_object.user_data)->x1C; }
    } else {
        fighter_live = false;
        if (recycle) { fighter_live = true; ++((Fighter*)fighter_object.user_data)->x8_spawnNum; }
    }
}

static bool alive(HSD_GObj* object, uint32_t generation, uint32_t source_class)
{
    ++guards;
    if (source_class == NativeCallbackSource_Fighter) {
        assert(object == &fighter_object && generation == fighter_generation);
        return fighter_live && ((Fighter*)object->user_data)->x8_spawnNum == generation;
    }
    assert(source_class == NativeCallbackSource_Item && object == &item_object && generation == item_generation);
    return item_live && ((Item*)object->user_data)->x1C == generation;
}

static bool dispatch(uint32_t target, uint32_t event)
{
    assert(target == target_id && event == name_hash("receive"));
    boundary();
    if (transfer_owner) ((Item*)item_object.user_data)->owner = transfer_owner == 1 ? &fighter_object : NULL;
    return dispatch_result;
}
static int32_t dispatch_int(uint32_t target, uint32_t event)
{ dispatch(target,event); return 16777217; }
static float dispatch_float(uint32_t target, uint32_t event)
{ dispatch(target,event); return 1.25f; }

static void landing(Fighter_GObj* object)
{
    assert(object == &fighter_object); boundary();
}

static bool jump(Fighter_GObj* object)
{
    assert(object == &fighter_object); ++jump_checks; return true;
}

static bool expire(Item_GObj* object)
{
    assert(object == &item_object); ++expire_checks; return true;
}

static void remove_item(Item_GObj* object)
{
    assert(object == &item_object); ++operations; item_live = false;
}

static void grounded(Item* item) { assert(item == item_object.user_data); boundary(); }
static void airborne(Item* item) { assert(item == item_object.user_data); boundary(); }
static uint32_t current_owner(Item_GObj* object) { assert(object == &item_object); return target_id; }
static Fighter_GObj* creator(Item_GObj* object) { assert(object == &item_object); return creator_present ? &fighter_object : NULL; }

static unsigned offset(const NativeFighterStateField* fields, unsigned count, const char* name)
{
    for (unsigned i = 0; i < count; ++i)
        if (!strcmp(fields[i].name, name)) return fields[i].offset;
    assert(0); return 0;
}

static uint32_t word(const uint8_t* bytes, unsigned at)
{
    uint32_t result; memcpy(&result, bytes + at, 4); return result;
}

static void reset(Fighter* fighter, Item* item, unsigned stop, bool item_source, bool reuse, uint32_t id)
{
    memset(fighter, 0, sizeof(*fighter)); memset(item, 0, sizeof(*item));
    fighter_generation = UINT32_C(0x80000001); item_generation = UINT32_C(16777217);
    fighter->x8_spawnNum = fighter_generation; item->x1C = item_generation;
    fighter_object.user_data = fighter; item_object.user_data = item;
    item->entity = &item_object; item->owner = &fighter_object;
    fighter_live = item_live = dispatch_result = true;
    operations = guards = jump_checks = expire_checks = 0;
    kill_at = stop; kill_item = item_source; recycle = reuse; target_id = id;
    transfer_owner = 0;
    creator_present = true;
}

int main(void)
{
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .fighter_root = "ftDataNess",
        .motion_state_first = 341, .motion_state_count = 1, .motion_states = &motion };
    NativeFighterHost host = { .target_call = dispatch, .callback_source_alive = alive,
        .target_call_bool=dispatch, .target_call_int=dispatch_int, .target_call_float=dispatch_float,
        .enter_landing_basic = landing, .check_ground_jump = jump,
        .item_creator = creator, .item_current_owner = current_owner,
        .item_remove = remove_item, .item_set_grounded = grounded,
        .item_set_airborne = airborne, .item_expire_tick = expire };
    const NativeFighterDefinition* definition = FreakMelee_GetFighterModule(&base, &host);
    assert(definition && definition->article_logic_count == 1);
    NativeFighterHost incomplete = host; incomplete.callback_source_alive = NULL;
    assert(!FreakMelee_GetFighterModule(&base, &incomplete));
    incomplete = host; incomplete.target_call = NULL;
    assert(!FreakMelee_GetFighterModule(&base, &incomplete));
    incomplete=host; incomplete.target_call_bool=NULL; assert(!FreakMelee_GetFighterModule(&base,&incomplete));
    incomplete=host; incomplete.target_call_int=NULL; assert(!FreakMelee_GetFighterModule(&base,&incomplete));
    incomplete=host; incomplete.target_call_float=NULL; assert(!FreakMelee_GetFighterModule(&base,&incomplete));
    Fighter* fighter = calloc(1, sizeof(*fighter));
    Item* item = calloc(1, sizeof(*item)); assert(fighter && item);
    const NativeFighterArticleLogic* article = &definition->article_logics[0];
    unsigned target = offset(definition->state_fields, definition->state_field_count, "target");
#define FWORD(name) word(fighter->native_module_state, offset(definition->state_fields, definition->state_field_count, name))
#define FBOOL(name) fighter->native_module_state[offset(definition->state_fields, definition->state_field_count, name)]
#define IWORD(name) word(item->xDD4_itemVar.native_article.state, offset(article->state_fields, article->state_field_count, name))
#define IBOOL(name) item->xDD4_itemVar.native_article.state[offset(article->state_fields, article->state_field_count, name)]
    uint32_t ids[] = { UINT32_C(16777217), UINT32_C(0x80000001), UINT32_MAX };
    for (unsigned stop=0; stop<2; ++stop)
        for (unsigned reuse=0; reuse<2; ++reuse)
            for (unsigned i=0; i<article->signal_count; ++i) {
                const NativeFighterArticleSignal* signal=&article->signals[i];
                if (signal->name_hash!=name_hash("typed_int") &&
                    signal->name_hash!=name_hash("typed_float") &&
                    signal->name_hash!=name_hash("typed_bool")) continue;
                reset(fighter,item,stop,true,reuse,ids[2]);
                signal->callback(&item_object,0);
                assert(operations==1);
                if (signal->name_hash==name_hash("typed_int")) assert(IWORD("after")== (stop?0:16777217));
                if (signal->name_hash==name_hash("typed_float")) assert(IWORD("after")== (stop?0:15));
                if (signal->name_hash==name_hash("typed_bool")) assert(IBOOL("ready")== !stop);
            }
    for (unsigned id = 0; id < 3; ++id)
        for (unsigned reused = 0; reused < 2; ++reused)
            for (unsigned stop = 0; stop <= 2; ++stop) {
                reset(fighter, item, stop, false, reused, ids[id]);
                memcpy(fighter->native_module_state + target, &target_id, 4);
#if CALLBACK_LIFETIME_CALLBACKS
                definition->motion_states[0].phys_cb(&fighter_object);
#else
                definition->on_load(&fighter_object);
#endif
                assert(FWORD("before") == 1 && !jump_checks && !FBOOL("skipped"));
                assert(operations == (stop == 1 ? 1 : 2));
                assert(FWORD("visits") == (stop == 1 ? 0 : stop == 2 ? 1 : 111));
                assert(FWORD("after") == (stop ? 0 : 9));
                assert(FBOOL("ready") == (stop ? 0 : 1));
                assert(guards >= 1);
            }
    // Caller result types all return their neutral value after invalidation.
    for (unsigned stop = 0; stop <= 1; ++stop) {
        reset(fighter, item, stop, false, false, ids[1]);
        memcpy(fighter->native_module_state + target, &target_id, 4);
        assert(definition->on_jab_start(&fighter_object) == (stop ? 0 : 16777217));
        reset(fighter, item, stop, false, false, ids[1]);
        memcpy(fighter->native_module_state + target, &target_id, 4);
        assert(definition->on_shield_scale(&fighter_object) == (stop ? 0.0f : 1.25f));
        reset(fighter, item, stop, false, false, ids[1]);
        memcpy(fighter->native_module_state + target, &target_id, 4);
        assert(definition->on_worn_item_jump_sound(&fighter_object) == !stop);
    }
    for (unsigned reused = 0; reused < 2; ++reused)
        for (unsigned stop = 0; stop <= 2; ++stop) {
            reset(fighter, item, stop, true, reused, ids[2]);
            memcpy(fighter->native_module_state + target, &target_id, 4);
            article->on_created(&item_object);
            assert(operations == (stop == 1 ? 1 : 2));
            assert(IBOOL("ready") == (stop == 1 ? 0 : 1));
            assert(IWORD("after") == (stop ? 0 : 2));
            // Creator remains alive when its dispatch removes the calling item;
            // its own helper completes, then the item boundary aborts.
            assert(FWORD("visits") == (stop == 1 ? 0 : 111));
        }
    for (unsigned stop = 0; stop <= 1; ++stop) {
        reset(fighter, item, stop, true, false, ids[0]);
        article->logic->spawned(&item_object);
        assert(operations == 1 && IWORD("after") == (stop ? 0 : 3));
        reset(fighter, item, stop, true, false, ids[0]);
        dispatch_result = false;
        assert(article->logic->dmg_dealt(&item_object) == !stop);
        assert(IWORD("after") == (stop ? 0 : 5));
    }
    reset(fighter, item, 0, true, false, ids[0]);
    article->logic->destroyed(&item_object);
    assert(operations == 1 && IWORD("after") == 0);
    reset(fighter, item, 0, true, false, ids[0]);
    article->logic->picked_up(&item_object);
    assert(operations == 1 && IWORD("after") == 0);
    for (unsigned stop = 0; stop <= 2; ++stop) {
        reset(fighter, item, stop, true, false, ids[1]);
        dispatch_result = false;
        article->logic->states[0].physics_updated(&item_object);
        assert(operations == (stop == 1 ? 1 : 2) && !expire_checks);
        assert(IWORD("after") == (stop ? 0 : 8));
    }
    // The item survives recipient removal. External scalar/array writes must
    // skip a dead or reused creator rather than touching its cached storage.
    for (unsigned reused = 0; reused < 2; ++reused)
        for (unsigned stop = 0; stop <= 1; ++stop)
            for (unsigned owner = 0; owner < 2; ++owner)
            for (unsigned array = 0; array < 2; ++array) {
                reset(fighter, item, stop, false, reused, ids[1]);
                if (owner) {
                    if (array) assert(!article->logic->states[0].animated(&item_object));
                    else article->logic->entered_air(&item_object);
                } else (array ? article->logic->thrown : article->logic->dropped)(&item_object);
                unsigned result_offset = offset(definition->state_fields, definition->state_field_count,
                    array ? "flags" : "ready");
                assert(fighter->native_module_state[result_offset] == !stop);
                assert(IWORD("after") == (owner ? array ? 14 : 13 : array ? 12 : 11) && item_live);
                assert(operations == 1);
            }
    // Current owner is deliberately re-evaluated after RHS. Becoming the live
    // immutable creator allows the write; leaving it skips the write.
    for (unsigned mode = 0; mode <= 2; ++mode)
        for (unsigned array = 0; array < 2; ++array) {
            reset(fighter, item, 0, false, false, ids[0]);
            if (mode == 1) item->owner = NULL;
            transfer_owner = mode;
            if (array) assert(!article->logic->states[0].animated(&item_object));
            else article->logic->entered_air(&item_object);
            unsigned result_offset = offset(definition->state_fields, definition->state_field_count,
                array ? "flags" : "ready");
            assert(fighter->native_module_state[result_offset] == (mode != 2));
            assert(IWORD("after") == (array ? 14 : 13) && operations == 1);
        }
    for (unsigned owner = 0; owner < 2; ++owner)
        for (unsigned array = 0; array < 2; ++array) {
            reset(fighter, item, 0, false, false, ids[0]);
            creator_present = false;
            if (owner) {
                if (array) assert(!article->logic->states[0].animated(&item_object));
                else article->logic->entered_air(&item_object);
            } else (array ? article->logic->thrown : article->logic->dropped)(&item_object);
            unsigned result_offset = offset(definition->state_fields, definition->state_field_count,
                array ? "flags" : "ready");
            assert(fighter->native_module_state[result_offset] == 0);
            assert(operations == (owner || array ? 1 : 0));
            assert(IWORD("after") == (owner ? array ? 14 : 13 : array ? 12 : 11));
        }
    free(item); free(fighter);
    puts("Generated callbacks stop at each source removal/reuse boundary, preserve short circuit and full-bit IDs in all result contexts.");
    return 0;
}
