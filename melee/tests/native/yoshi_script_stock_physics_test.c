// Compare the compiled full template's selected callbacks with original C.
// This does not run a game, and does not substitute copied reference formulas.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftYoshi/ftyoshi_clone.h>
#include <melee/ft/kinds/ftYoshi/ftyoshispeciallw.h>
#include <melee/ft/kinds/ftYoshi/ftyoshispecials.h>
#include <melee/ft/kinds/ftYoshi/types.h>
#include <melee/it/kinds/ityoshieggthrow.h>
#include <melee/it/kinds/ityoshistar.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

_Static_assert(offsetof(struct ftYs_DatAttrs, specialhi_base_angle) == 0xF8,
               "Yoshi Egg Throw DAT angle offset must remain serialized");
_Static_assert(offsetof(struct ftYs_DatAttrs, speciallw_star_offset) == 0x118,
               "Yoshi star DAT offset must remain serialized");
_Static_assert(sizeof(struct ftYs_DatAttrs) == 0x120,
               "Yoshi special DAT view must contain only serialized words");

static unsigned comparisons;
static void require(bool valid, const char* message)
{
    if (valid) return;
    fprintf(stderr, "%s (comparison %u)\n", message, comparisons);
    exit(1);
}

static void read_parameters(const char* path, NativeFighterParameters* parameters)
{
    FILE* file = fopen(path, "rb");
    require(file != NULL, "Cannot open owned parameter file");
    char text[32769];
    size_t size = fread(text, 1, sizeof(text), file);
    require(!ferror(file) && size > 0 && size < sizeof(text), "Invalid parameter size");
    require(fclose(file) == 0, "Cannot close parameter file");
    unsigned line = 0;
    require(nativeFighterParameters_Parse(text, size, parameters, &line),
            "Cannot parse owned parameters");
}

static void parameter_float(NativeFighterParameters* parameters,
                            const char* name, float value)
{
    for (unsigned i = 0; i < parameters->count; ++i) {
        NativeFighterParameter* field = &parameters->fields[i];
        if (strcmp(field->name, name) != 0) continue;
        require(field->type == NativeFighterParameter_Float, "Expected float parameter");
        memcpy(&field->bits, &value, sizeof(value));
        return;
    }
    require(false, "Missing named parameter");
}

static const NativeFighterArticleLogic* article(
    const NativeFighterDefinition* definition, ItemKind kind)
{
    for (unsigned i = 0; i < definition->article_logic_count; ++i)
        if (definition->article_logics[i].kind == kind)
            return &definition->article_logics[i];
    require(false, "Missing authored article");
    return NULL;
}

static void compare_pound(NativeFighterModule* module,
                          NativeFighterParameters* parameters)
{
    static const float delta[] = { -8, -0.125f, 0, 0.125f, 8 };
    static const float limits[] = { -12, -4.25f, 0, 3 };
    Fighter* fighters = calloc(2, sizeof(*fighters));
    require(fighters != NULL, "Cannot allocate fixture fighters");
    HSD_GObj objects[2] = { 0 };
    objects[0].user_data = &fighters[0];
    objects[1].user_data = &fighters[1];
    const NativeFighterDefinition* definition = module->definition;
    const MotionState* startup = &definition->motion_states[366 - 341];
    const MotionState* dive = &definition->motion_states[368 - 341];
    require(startup->phys_cb != ftYs_SpecialLw_Phys &&
            dive->phys_cb != ftYs_SpecialAirLw_Phys,
            "Ground Pound must replace both stock physics callbacks");
    for (unsigned li = 0; li < sizeof(limits) / sizeof(limits[0]); ++li) {
        ftYoshiAttributes attrs = { .x114 = limits[li] };
        parameter_float(parameters, "pound_dive_speed", limits[li]);
        require(nativeFighterModule_SetParameters(module, parameters), "Cannot bind dive tuning");
        for (unsigned air = 0; air < 2; ++air)
        for (unsigned command = 0; command < 2; ++command)
        for (unsigned direction = 0; direction < 2; ++direction)
        for (unsigned x = 0; x < sizeof(delta) / sizeof(delta[0]); ++x)
        for (unsigned y = 0; y < sizeof(delta) / sizeof(delta[0]); ++y) {
            fighters[0] = (Fighter){ .dat_attrs = &attrs,
                .facing_dir = direction ? -1 : 1,
                .self_vel = { 100, -100, 9 },
                .x6A4_transNOffset = { 7, delta[y], delta[x] } };
            fighters[0].cmd_vars[1] = command;
            fighters[1] = fighters[0];
            if (air) {
                ftYs_SpecialAirLw_Phys(&objects[0]);
                dive->phys_cb(&objects[1]);
            } else {
                ftYs_SpecialLw_Phys(&objects[0]);
                startup->phys_cb(&objects[1]);
            }
            ++comparisons;
            require(memcmp(&fighters[0].self_vel, &fighters[1].self_vel,
                           sizeof(Vec3)) == 0, "Pound velocity differs from original C");
        }
    }
    free(fighters);
}

static void compare_roll(NativeFighterModule* module,
                         NativeFighterParameters* parameters)
{
    static const float speeds[] = { -8, -3, -0.01f, 0, 0.01f, 3, 8 };
    static const float air_speeds[] = { -8, -0.25f, 0, 0.25f, 8 };
    static const float slopes[] = { -0.8f, -0.1f, 0, 0.1f, 0.8f };
    static const float sticks[] = { -1, -0.25f, 0, 0.25f, 1 };
    static const unsigned motions[] = { 360, 361, 362, 363 };
    static HSD_GObjEvent const air_stock[] = {
        ftYs_SpecialAirSStart_1_Phys, ftYs_SpecialAirSLoop_2_Phys,
        ftYs_SpecialAirSLoop_3_Phys, ftYs_SpecialAirSLanding_Phys,
    };
    Fighter* fighters = calloc(2, sizeof(Fighter));
    require(fighters != NULL, "Cannot allocate roll fixture fighters");
    HSD_GObj objects[2] = { { .user_data = &fighters[0] },
                            { .user_data = &fighters[1] } };
    const NativeFighterDefinition* definition = module->definition;
    for (unsigned tuning = 0; tuning < 3; ++tuning) {
        ftYoshiAttributes attrs = {
            .specials_start_gravity = 0.05f + tuning * 0.11f,
            .specials_start_terminal_vel = 1.5f + tuning * 2,
            .x74 = 0.05f + tuning * 0.04f,
            .x78 = 0.025f + tuning * 0.02f,
            .x7C = 1 + tuning * 0.75f,
            .x80 = 3 + tuning * 1.25f,
            .x90 = 2 + tuning * 0.9f,
            .x94 = 0.5f + tuning * 0.5f,
            .xE0 = 0.03f + tuning * 0.05f,
        };
        parameter_float(parameters, "roll_gravity", attrs.specials_start_gravity);
        parameter_float(parameters, "roll_terminal_velocity", attrs.specials_start_terminal_vel);
        parameter_float(parameters, "roll_ground_acceleration", attrs.x74);
        parameter_float(parameters, "roll_ground_deceleration", attrs.x78);
        parameter_float(parameters, "roll_ground_target_speed", attrs.x7C);
        parameter_float(parameters, "roll_ground_max_speed", attrs.x80);
        parameter_float(parameters, "roll_air_max_speed", attrs.x90);
        parameter_float(parameters, "roll_slope_multiplier", attrs.x94);
        parameter_float(parameters, "roll_air_acceleration", attrs.xE0);
        require(nativeFighterModule_SetParameters(module, parameters), "Cannot bind roll tuning");
        for (unsigned phase = 0; phase < 4; ++phase) {
            HSD_GObjEvent script = definition->motion_states[motions[phase] - 341].phys_cb;
            require(script != air_stock[phase], "Egg Roll must own all air physics callbacks");
            for (unsigned input = 0; input < 5; ++input)
            for (unsigned x = 0; x < 5; ++x)
            for (unsigned y = 0; y < 5; ++y) {
                fighters[0] = (Fighter){ .dat_attrs = &attrs,
                    .self_vel = { air_speeds[x], air_speeds[y], 2 },
                    .x74_self_accel = { 3, 4, 5 } };
                fighters[0].input.lstick[0].x = sticks[input];
                fighters[1] = fighters[0];
                air_stock[phase](&objects[0]);
                script(&objects[1]);
                ++comparisons;
                require(memcmp(&fighters[0].self_vel, &fighters[1].self_vel, sizeof(Vec3)) == 0 &&
                        memcmp(&fighters[0].x74_self_accel, &fighters[1].x74_self_accel,
                               sizeof(Vec3)) == 0, "Roll air physics differs from original C");
            }
        }
        for (unsigned slope = 0; slope < 5; ++slope)
        for (unsigned speed = 0; speed < 7; ++speed)
        for (unsigned material = 0; material < 4; ++material) {
            fighters[0] = (Fighter){ .dat_attrs = &attrs, .gr_vel = speeds[speed],
                .xE4_ground_accel_1 = 0.1f, .x74_self_accel = { 3, 4, 5 },
                .self_vel = { 6, 7, 8 } };
            fighters[0].coll_data.floor.normal.x = slopes[slope];
            fighters[0].coll_data.floor.normal.y = sqrtf(1 - slopes[slope] * slopes[slope]);
            fighters[0].coll_data.floor.index = -1;
            fighters[0].coll_data.floor.flags = material;
            fighters[1] = fighters[0];
            ftYs_SpecialAirSLoop_0_Phys(&objects[0]);
            definition->motion_states[357 - 341].phys_cb(&objects[1]);
            ++comparisons;
            require(memcmp(&fighters[0].self_vel, &fighters[1].self_vel, sizeof(Vec3)) == 0 &&
                    memcmp(&fighters[0].x74_self_accel, &fighters[1].x74_self_accel,
                           sizeof(Vec3)) == 0 && fighters[0].gr_vel == fighters[1].gr_vel &&
                    fighters[0].xE4_ground_accel_1 == fighters[1].xE4_ground_accel_1,
                    "Roll slope physics differs from original C");
            if (tuning != 0) continue;
            for (unsigned phase = 0; phase < 2; ++phase) {
                fighters[0].gr_vel = speeds[speed];
                fighters[0].co_attrs.ground_friction = 0.075f;
                fighters[1] = fighters[0];
                HSD_GObjEvent stock = phase ? ftYs_SpecialAirSEnd_Phys : ftYs_SpecialAirSStart_0_Phys;
                stock(&objects[0]);
                definition->motion_states[(phase ? 359 : 356) - 341].phys_cb(&objects[1]);
                ++comparisons;
                require(memcmp(&fighters[0].self_vel, &fighters[1].self_vel, sizeof(Vec3)) == 0 &&
                        memcmp(&fighters[0].x74_self_accel, &fighters[1].x74_self_accel,
                               sizeof(Vec3)) == 0 && fighters[0].gr_vel == fighters[1].gr_vel &&
                        fighters[0].xE4_ground_accel_1 == fighters[1].xE4_ground_accel_1,
                        "Roll start/end ground physics differs from original C");
            }
        }
    }
    free(fighters);
}

static void compare_roll_materials(const NativeFighterModule* module)
{
    // Execute the real startup animation callback with an active joint track.
    // Material tracks contain no FObjs, so this observes frame/flag resets
    // without requiring rendering, allocation or fabricated engine callbacks.
    FighterPartsTable parts = { .parts_num = 1 };
    FighterPartsTable* tables[Ft_Kind_RuntimeCapacity] = { 0 };
    tables[Ft_Kind_Yoshi] = &parts;
    FighterPartsTable** saved = ftPartsTable;
    ftPartsTable = tables;
    Fighter* fighters = calloc(2, sizeof(Fighter));
    require(fighters != NULL, "Cannot allocate roll material fixtures");
    for (unsigned frame = 0; frame < 20; ++frame) {
        HSD_AObj tracks[2][3] = { 0 };
        HSD_MObj materials[2][3] = { 0 };
        HSD_DObj meshes[2][3] = { 0 };
        HSD_DObj* mesh_pointers[2][3];
        u8 indices[2] = { 0, 2 };
        TempS variants[2][2] = {
            { { .x0 = 0 }, { .x0 = 1, .x4 = &indices[0] } },
            { { .x0 = 0 }, { .x0 = 1, .x4 = &indices[1] } },
        };
        FtPartsVisLookup groups[2] = {
            { .x0 = 2, .x4 = variants[0] },
            { .x0 = 2, .x4 = variants[1] },
        };
        HSD_AObj active = { 0 };
        HSD_JObj joint = { .aobj = &active };
        FighterBone fighter_part = { .joint = &joint, .flags_b1 = true };
        for (unsigned variant = 0; variant < 2; ++variant) {
            fighters[variant] = (Fighter){ .kind = Ft_Kind_Yoshi,
                .parts = &fighter_part };
            fighters[variant].cmd_vars[0] = 1;
            fighters[variant].x5F4_arr[0].idx = 1;
            fighters[variant].x5AC.model_num = 1;
            fighters[variant].x5AC.xC[0] = &groups[0];
            fighters[variant].x5AC.xC[1] = &groups[1];
            fighters[variant].dobj_list = (DObjList){ .count = 3,
                                                    .data = mesh_pointers[variant] };
            for (unsigned mesh = 0; mesh < 3; ++mesh) {
                tracks[variant][mesh] = (HSD_AObj){ .flags = AOBJ_NO_ANIM,
                    .curr_frame = frame + mesh + 0.5f, .end_frame = 50 };
                materials[variant][mesh].aobj = &tracks[variant][mesh];
                meshes[variant][mesh].mobj = &materials[variant][mesh];
                mesh_pointers[variant][mesh] = &meshes[variant][mesh];
            }
        }
        HSD_GObj objects[2] = { { .user_data = &fighters[0] },
                                { .user_data = &fighters[1] } };
        ftYs_SpecialAirSStart_1_Anim(&objects[0]);
        module->definition->motion_states[360 - 341].anim_cb(&objects[1]);
        ++comparisons;
        for (unsigned mesh = 0; mesh < 3; ++mesh)
            require(tracks[0][mesh].curr_frame == tracks[1][mesh].curr_frame &&
                    tracks[0][mesh].flags == tracks[1][mesh].flags,
                    "Roll grouped material reset differs from original C");
        require(fighters[0].x221E_b3 == fighters[1].x221E_b3 &&
                fighters[0].x221E_b4 == fighters[1].x221E_b4,
                "Roll held/attached visibility differs from original C");
    }
    free(fighters);
    ftPartsTable = saved;
}

static void compare_articles(NativeFighterModule* module,
                             NativeFighterParameters* parameters)
{
    static const float acceleration[] = { -0.1f, 0, 0.025f, 0.25f };
    static const float initial_speed[] = { -5, -0.125f, 0, 0.125f, 5 };
    const NativeFighterArticleLogic* star = article(module->definition, It_Kind_Yoshi_Star);
    const NativeFighterArticleLogic* egg = article(module->definition, It_Kind_Yoshi_EggThrow);
    require(star->state_count == 1 && egg->state_count == 3,
            "Article state tables differ from stock");
    ItemAttr common = { .x10_fall_speed = -0.25f, .x14_fall_speed_max = 7 };
    for (unsigned ai = 0; ai < sizeof(acceleration) / sizeof(acceleration[0]); ++ai) {
        float stock_attrs[2] = { 4, acceleration[ai] };
        Article data = { .x4_specialAttributes = stock_attrs };
        parameter_float(parameters, "It_Kind_Yoshi_Star.acceleration", acceleration[ai]);
        require(nativeFighterModule_SetParameters(module, parameters), "Cannot bind star tuning");
        for (unsigned facing = 0; facing < 2; ++facing)
        for (unsigned yi = 0; yi < sizeof(initial_speed) / sizeof(initial_speed[0]); ++yi) {
            Item stock = { .xCC_item_attr = &common, .xC4_article_data = &data,
                .facing_dir = facing ? -1 : 1,
                .spin_spd = 3, .xD3C_spinSpeed = 0.625f,
                .x40_vel = { 2, initial_speed[yi], 0.75f } };
            Item script = stock;
            HSD_GObj stock_object = { .user_data = &stock };
            HSD_GObj script_object = { .user_data = &script };
            for (unsigned tick = 0; tick < 32; ++tick) {
                itYoshistar_UnkMotion0_Phys(&stock_object);
                star->logic->states[0].physics_updated(&script_object);
                ++comparisons;
                require(memcmp(&stock.x40_vel, &script.x40_vel, sizeof(Vec3)) == 0,
                        "Star physics differs from original C");
                require(memcmp(&stock.xD3C_spinSpeed, &script.xD3C_spinSpeed,
                               sizeof(float)) == 0, "Star gravity changed spin");
            }
            stock.x40_vel.y = script.x40_vel.y = initial_speed[yi];
            for (unsigned tick = 0; tick < 32; ++tick) {
                itYoshieggthrow_UnkMotion1_Phys(&stock_object);
                egg->logic->states[1].physics_updated(&script_object);
                ++comparisons;
                require(memcmp(&stock.x40_vel, &script.x40_vel, sizeof(Vec3)) == 0,
                        "Egg gravity differs from original C");
                require(memcmp(&stock.xD3C_spinSpeed, &script.xD3C_spinSpeed,
                               sizeof(float)) == 0, "Egg gravity overwrote release spin");
            }
            HSD_JObj stock_joint = { .flags = JOBJ_MTX_INDEP_SRT,
                                    .scale = { 1, 1, 1 } };
            HSD_JObj script_joint = stock_joint;
            stock_object.hsd_obj = &stock_joint;
            script_object.hsd_obj = &script_joint;
            stock.xC70 = script.xC70 = 1.25f;
            stock.xD48_halfLifeTimer = script.xD48_halfLifeTimer = 19;
            bool expected = it_802B314C(&stock_object);
            bool observed = star->logic->reflected(&script_object);
            ++comparisons;
            require(expected == observed &&
                    memcmp(&stock.x40_vel, &script.x40_vel, sizeof(Vec3)) == 0 &&
                    memcmp(&stock.facing_dir, &script.facing_dir, sizeof(float)) == 0 &&
                    memcmp(&stock.xD44_lifeTimer, &script.xD44_lifeTimer, sizeof(float)) == 0 &&
                    memcmp(&stock_joint.rotate, &script_joint.rotate,
                           sizeof(stock_joint.rotate)) == 0,
                    "Star reflection differs from original C");
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        fputs("Usage: yoshi_script_stock_physics_test MODULE.dll PARAMETERS.fmp\n", stderr);
        return 2;
    }
    NativeFighterModule module = { 0 };
    NativeFighterParameters parameters;
    read_parameters(argv[2], &parameters);
    require(nativeFighterModule_Open(argv[1], &ftYoshiClone_Definition, &module),
            "Cannot bind the compiled Yoshi template");
    require(nativeFighterModule_SetParameters(&module, &parameters),
            "Cannot bind owned parameters");
    compare_pound(&module, &parameters);
    compare_roll(&module, &parameters);
    compare_roll_materials(&module);
    compare_articles(&module, &parameters);
    nativeFighterModule_Close(&module);
    printf("Yoshi original-C physics comparison: %u exact samples.\n", comparisons);
    return 0;
}
