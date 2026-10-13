#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_script_math.h>
#include <melee/it/kinds/itfoxlaser.h>
#include <melee/it/itCharItems.h>
#include <melee/it/native_article_model.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Only explicitly registered fixture objects are live; compare full IDs/classes.
static HSD_GObj* live_fighter;
static HSD_GObj* live_item;
static bool callback_source_alive(HSD_GObj* object, uint32_t generation, uint32_t source_class)
{
    if (source_class == NativeCallbackSource_Fighter && object == live_fighter && object && object->user_data)
        return (uint32_t)((Fighter*)object->user_data)->x8_spawnNum == generation;
    if (source_class == NativeCallbackSource_Item && object == live_item && object && object->user_data)
        return (uint32_t)((Item*)object->user_data)->x1C == generation;
    return false;
}

static bool callback_host_contract(void)
{
    Fighter source = { .x8_spawnNum = UINT32_C(0xFEDCBA98) };
    Item article = { .x1C = UINT32_C(0x80000001) };
    HSD_GObj owner = { .user_data = &source }, object = { .user_data = &article };
    live_fighter = &owner; live_item = &object;
    bool result = callback_source_alive(&owner, UINT32_C(0xFEDCBA98), NativeCallbackSource_Fighter) &&
        callback_source_alive(&object, UINT32_C(0x80000001), NativeCallbackSource_Item) &&
        !callback_source_alive(&owner, UINT32_C(0x7EDCBA98), NativeCallbackSource_Fighter) &&
        !callback_source_alive(&object, UINT32_C(1), NativeCallbackSource_Item) &&
        !callback_source_alive(&owner, source.x8_spawnNum, NativeCallbackSource_Item) &&
        !callback_source_alive(&object, article.x1C, NativeCallbackSource_Fighter) &&
        !callback_source_alive(NULL, 0, NativeCallbackSource_Item);
    live_fighter = NULL; live_item = NULL;
    return result;
}

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);

static unsigned samples, sweeps, lifetimes, launches;
static Vec3 expected_previous;
static bool stage_hit;
static bool same(float a, float b) { return memcmp(&a, &b, sizeof(a)) == 0; }
static void require(bool ok, const char* message)
{
    if (ok) return;
    fprintf(stderr, "%s (sample %u)\n", message, samples);
    exit(1);
}
static float field(Item* item, unsigned offset)
{
    float value;
    memcpy(&value, item->xDD4_itemVar.native_article.state + offset, 4);
    return value;
}
static bool sweep(Item_GObj* object, Vec3* previous, Vec3* current, Vec3* normal)
{
    require(normal == NULL && current == &((Item*) object->user_data)->pos,
            "Stage sweep destination must be the item's actual position");
    require(memcmp(previous, &expected_previous, sizeof(Vec3)) == 0,
            "Stage sweep must use the launch origin / preceding physics position");
    ++sweeps;
    if (stage_hit) *current = (Vec3) { 100, -100, 40 };
    return stage_hit;
}
static void lifetime(Item_GObj* object, float value)
{
    Item* item = object->user_data;
    item->xD44_lifeTimer = value;
    item->xD48_halfLifeTimer = value * 0.5f;
    ++lifetimes;
}
static bool interact(Item_GObj* object, HSD_GObj* other)
{ (void) object; (void) other; return false; }

static void compare(Item* stock, Item* script, HSD_JObj* a, HSD_JObj* b)
{
    ++samples;
    require(memcmp(&stock->x40_vel, &script->x40_vel, sizeof(Vec3)) == 0,
            "Laser velocity differs from the original C");
    require(same(stock->facing_dir, script->facing_dir) &&
            same(stock->xD44_lifeTimer, script->xD44_lifeTimer),
            "Laser facing or lifetime differs from the original C");
    require(memcmp(&a->rotate, &b->rotate, sizeof(a->rotate)) == 0 &&
            memcmp(&a->scale, &b->scale, sizeof(Vec3)) == 0,
            "Laser model transform differs from the original C");
    require(same(stock->xDD4_itemVar.foxlaser.scale, field(script, 0)) &&
            same(stock->xDD4_itemVar.foxlaser.angle, field(script, 4)) &&
            same(stock->xDD4_itemVar.foxlaser.speed, field(script, 8)),
            "Laser named state differs from the original C");
    require(memcmp(&stock->xDD4_itemVar.foxlaser.pos,
            script->xDD4_itemVar.native_article.state + 16, sizeof(Vec3)) == 0,
            "Laser previous position differs from the original C");
}

static void run_article(const NativeFighterArticleLogic* article)
{
    static const float angles[] = {
        0, 0.1f, -0.1f, (float) M_PI_2, (float) M_PI,
        (float) (M_PI * 1.5), (float) (M_PI * 2), 5.25f,
    };
    static const float speeds[] = { 0, 0.25f, 2, 5.5f, -2 };
    static const float limits[] = { 0.000001f, 3, 5 };
    const NativeFighterProjectile* projectile = article->projectile;
    require(article->state_count == 2 && projectile != NULL &&
            projectile->on_launched != NULL && !projectile->initial_velocity &&
            !projectile->initial_update && projectile->auto_lifetime &&
            projectile->appearance_state == -1 && projectile->read_values != NULL,
            "Laser launch descriptor does not preserve stock initialization");
    for (unsigned state = 0; state < 2; ++state)
    for (unsigned ai = 0; ai < sizeof(angles) / sizeof(angles[0]); ++ai)
    for (unsigned si = 0; si < sizeof(speeds) / sizeof(speeds[0]); ++si)
    for (unsigned li = 0; li < sizeof(limits) / sizeof(limits[0]); ++li) {
        FoxLaserAttr attr = { .lifetime = 35, .scale = limits[li] };
        Article data = { .x4_specialAttributes = &attr };
        HSD_JObj a = { .flags = JOBJ_MTX_INDEP_SRT, .scale = { 1, 1, 1 } };
        HSD_JObj b = a;
        Item stock = { .xC4_article_data = &data, .pos = { 5, 6, 7 },
                       .xD44_lifeTimer = 35, .facing_dir = 1 };
        Item script = stock;
        Vec3 origin = { 1, 2, 3 };
        float angle = nativeScriptMath_WrapAngle(angles[ai]);
        stock.facing_dir = script.facing_dir =
            angle < M_PI_2 || angle > M_PI * 3 / 2 ? 1 : -1;
        stock.xDD4_itemVar.foxlaser.angle = angle;
        stock.xDD4_itemVar.foxlaser.speed = speeds[si];
        stock.xDD4_itemVar.foxlaser.pos = origin;
        script.xDD4_itemVar.native_article.launch_angle = angle;
        script.xDD4_itemVar.native_article.launch_speed = speeds[si];
        script.xDD4_itemVar.native_article.launch_origin = origin;
        HSD_GObj stock_object = { .user_data = &stock, .hsd_obj = &a };
        HSD_GObj script_object = { .user_data = &script, .hsd_obj = &b };
            live_item = &script_object;
        projectile->on_launched(&script_object);
        compare(&stock, &script, &a, &b);
        expected_previous = origin;
        stage_hit = false;
        require(!article->logic->states[state].collided(&script_object),
                "A clear stage sweep should retain the laser");
        for (unsigned frame = 0; frame < 35; ++frame) {
            bool expected = itFoxlaser_UnkMotion1_Anim(&stock_object);
            bool observed = article->logic->states[state].animated(&script_object);
            require(expected == observed, "Laser expiry result differs");
            compare(&stock, &script, &a, &b);
            itFoxlaser_UnkMotion1_Phys(&stock_object);
            article->logic->states[state].physics_updated(&script_object);
            compare(&stock, &script, &a, &b);
            if (frame == 5) {
                stock.xC58 = script.xC58 = (Vec3) { 0.6f, 0.8f, 0 };
                require(itFoxLaser_Logic94_ShieldBounced(&stock_object) ==
                        article->logic->shield_bounced(&script_object),
                        "Shield bounce return differs");
                compare(&stock, &script, &a, &b);
            }
            if (frame == 10 || frame == 15) {
                stock.xC68 = script.xC68 = frame == 10 ? -1 : 1;
                require(itFoxLaser_Logic94_Reflected(&stock_object) ==
                        article->logic->reflected(&script_object),
                        "Reflection return differs");
                compare(&stock, &script, &a, &b);
            }
            if (!expected) {
                stock.pos.x += stock.x40_vel.x;
                script.pos = stock.pos;
            }
        }
        expected_previous = stock.xDD4_itemVar.foxlaser.pos;
        Vec3 before = script.pos;
        stage_hit = true;
        unsigned old_lifetimes = lifetimes;
        require(!article->logic->states[state].collided(&script_object) &&
                same(script.xD44_lifeTimer, 1) && same(script.xD48_halfLifeTimer, 0.5f) &&
                lifetimes == old_lifetimes + 1 && memcmp(&script.pos, &before, sizeof(Vec3)) == 0,
                "Stage hit must set one-frame lifetime and restore the pre-sweep position");
        require(article->logic->dmg_dealt != NULL &&
                article->logic->dmg_dealt(&script_object) == itFoxLaser_Logic94_Clanked(&stock_object) &&
                article->logic->clanked(&script_object) == itFoxLaser_Logic94_Clanked(&stock_object) &&
                article->logic->absorbed(&script_object) == itFoxLaser_Logic94_Absorbed(&stock_object) &&
                article->logic->hit_shield(&script_object) == itFoxLaser_Logic94_HitShield(&stock_object),
                "Laser interaction removal differs");
    }
}

static void change_motion(Fighter_GObj* object, FtMotionId state, MotionFlags flags,
                           float start, float speed, float blend, Fighter_GObj* other)
{
    (void) object; (void) state; (void) flags; (void) start;
    (void) speed; (void) blend; (void) other;
    require(false, "A launch-only handler should not change fighter motion");
}
static void start_animation(Fighter_GObj* object)
{ (void) object; require(false, "A launch-only handler should not restart animation"); }

static Fighter_Part resolve_bone(Fighter* fighter, Fighter_Part bone)
{ (void) fighter; require(bone == FtPart_RThumbNb, "Incorrect source bone"); return 0; }
static void joint_position(HSD_JObj* joint, Vec3* offset, Vec3* result)
{
    (void) joint;
    require(offset && same(offset->x, 0) && same(offset->y, 1.2325000762939453f) &&
            same(offset->z, 4.263599872589111f), "Launch bone offset differs");
    *result = (Vec3) { 7, 8, 9 };
}
static void launch(Fighter_GObj* owner, Vec3* pos, ItemKind kind, s32 variant,
                    float facing, const NativeFighterProjectileLaunch* input)
{
    require(owner != NULL && pos->x == 7 && pos->y == 8 && pos->z == 9 &&
            kind == It_Kind_Fox_Laser && variant == 0 && facing == -1 && input &&
            same(input->angle, (float) M_PI) && same(input->speed, 5.5f) && input->item_state == 1,
            "Explicit projectile launch arguments differ");
    ++launches;
}

int main(int argc, char** argv)
{
    if (!callback_host_contract()) return 1;
    if (argc != 3) return 2;
    MotionState states[1] = { 0 };
    NativeFighterDefinition base = { .motion_states = states,
        .motion_state_count = 1, .motion_state_first = 341 };
    NativeFighterHost host = { .callback_source_alive = callback_source_alive,
        .wrap_angle = nativeScriptMath_WrapAngle,
        .item_joint_value = nativeArticleModel_Read,
        .item_set_joint_value = nativeArticleModel_Write,
        .vector_mirror = lbVector_Mirror, .item_sweep_stage = sweep,
        .item_set_lifetime = lifetime, .item_interact = interact,
        .resolve_bone = resolve_bone, .joint_position = joint_position,
        .change_motion_state = change_motion, .start_animation = start_animation,
        .spawn_authored_projectile = launch,
    };
    // Resolve the CRT exports at runtime: constant C function-pointer
    // initializers otherwise reference thunks named like lbtrigf's console
    // functions and accidentally pull that unused object into this test.
    HMODULE math = GetModuleHandleA("ucrtbase.dll");
    require(math != NULL, "CRT is not loaded");
    host.sin_float = (float (*)(float)) GetProcAddress(math, "sinf");
    host.cos_float = (float (*)(float)) GetProcAddress(math, "cosf");
    host.abs_float = fabsf;
    host.angle_atan2_float = (float (*)(float, float)) GetProcAddress(math, "atan2f");
    require(host.sin_float && host.cos_float && host.abs_float && host.angle_atan2_float,
            "Missing CRT math export");
    HMODULE modules[2];
    for (unsigned i = 0; i < 2; ++i) {
        modules[i] = LoadLibraryExA(argv[i + 1], NULL,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        require(modules[i] != NULL, "Unable to load generated module");
        ModuleEntry entry = (ModuleEntry) GetProcAddress(modules[i], "FreakMelee_GetFighterModule");
        require(entry != NULL, "Missing module entry");
        const NativeFighterDefinition* definition = entry(&base, &host);
        require(definition != NULL, "Generated module rejected its host");
        if (i == 0) {
            require(definition->article_logic_count == 2, "Both laser variants must be authored");
            run_article(&definition->article_logics[0]);
            run_article(&definition->article_logics[1]);
            NativeFighterHost incomplete = host;
            incomplete.item_sweep_stage = NULL;
            require(entry(&base, &incomplete) == NULL, "Missing stage sweep should reject the module");
        } else {
            FighterBone parts[1] = { 0 };
            Fighter fighter = { .parts = parts, .facing_dir = -1 };
            HSD_GObj object = { .user_data = &fighter };
            live_fighter = &object;
            definition->motion_states[0].anim_cb(&object);
            require(launches == 1, "Explicit launch was not called");
            const NativeFighterProjectile* config = definition->article_logics[0].projectile;
            require(!config->auto_lifetime && !config->initial_velocity && !config->initial_update &&
                    config->on_launched != NULL, "Projectile flags were not compiled");
            Item item = { 0 };
            HSD_GObj item_object = { .user_data = &item };
            live_item = &item_object;
            config->on_launched(&item_object);
            require(item.xDD4_itemVar.native_article.state[0] == 1,
                    "The launch callback cannot write declared article state");
        }
        FreeLibrary(modules[i]);
    }
    for (int i = -1000; i <= 1000; ++i) {
        float expected = (float) i;
        while (expected < 0) expected += M_TAU;
        while (expected > M_TAU) expected -= M_TAU;
        require(same(expected, nativeScriptMath_WrapAngle((float) i)),
                "Angle wrapping differs from stock float assignment rounding");
    }
    require(isfinite(nativeScriptMath_WrapAngle(3.402823466e38f)) &&
            nativeScriptMath_WrapAngle(NAN) == 0 && nativeScriptMath_WrapAngle(INFINITY) == 0,
            "Angle wrapping must terminate for large / nonfinite input");
    printf("Laser parity: %u exact samples, %u stage sweeps, %u launch calls.\n",
           samples, sweeps, launches);
    return 0;
}
