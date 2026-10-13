#include "../../tools/native_fighter_script_state.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "script_lifetime_fixture.h"
#include "../../tools/native_fighter_script.h"
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool capture_owner_compile(void)
{
    const char* source=
        "state int catches\naction Capture:\n"
        "    on grab_catch:\n        pass\n"
        "    on grabbed:\n        state.catches = state.catches + 1\n"
        "        if airborne: capture.begin_attached(284)\n"
        "        else: capture.begin(275, false, false, true, true)\n"
        "        let flags = 0\n"
        "        if capture.victim_freeze_state: flags = 0x00200000\n"
        "        capture.set_victim_attached_throw(303, flags, 0, true)\n";
    FILE* output=tmpfile();NativeFighterScriptError error={0};
    bool valid=output&&nativeFighterScript_GenerateC(source,strlen(source),
        "capture_owner_probe",NULL,NULL,NULL,output,&error);
    if(valid){
        char code[65536];rewind(output);size_t length=fread(code,1,sizeof(code)-1,output);code[length]=0;
        // The engine's native callback supplies (victim, holder); authored
        // state, airborne queries and pose operations must use the right one.
        valid=feof(output)&&strstr(code,"authored_grabbed(HSD_GObj* victim_gobj, HSD_GObj* gobj) {")&&
            strstr(code,"engine->initialize_attached_capture(victim_gobj, gobj, 284)")&&
            strstr(code,"engine->begin_capture(victim_gobj, gobj, 275")&&
            strstr(code,"((Fighter*)fighter->victim_gobj->user_data)->x2222_b6")&&
            strstr(code,"engine->enter_attached_throw(fighter->victim_gobj, 303");
    }
    if(output)fclose(output);
    if(!valid)fprintf(stderr,"Capture callback ownership failed: %u %s\n",error.line,error.message);
    return valid;
}

static bool policy_values_compile(bool state_callbacks)
{
    char script[2048];
    NativeFighterParameters parameters;
    const char* values = "int lifetime = 80\nfloat strength = 1\n"
        "float decay = 0.003\nfloat frequency = 1.0471976\n";
    if (!nativeFighterParameters_Parse(values, strlen(values), &parameters, NULL)) return false;
    snprintf(script, sizeof(script),
        "%s"
        "%s        effects.wind(Top, input.x * 3, attrs.lifetime, attrs.strength, attrs.decay, attrs.frequency)\n"
        "    on cpu_capture_decision:\n"
        "        return capture.has_victim and capture.victim_damage < 100\n",
        state_callbacks ? "" : "",
        state_callbacks ? "move Policy:\n    on taunt:\n" : "action Policy:\n    on taunt:\n");
    FILE* output = tmpfile();
    NativeFighterScriptError error = { 0 };
    bool valid = output && nativeFighterScript_GenerateC(script, strlen(script),
        "policy_values_probe", NULL, &parameters, state_callbacks ? &(const NativeFighterScriptConstruction){"ftDataCaptain",341,1} : NULL, output, &error);
    if (valid) {
        char code[65536]; rewind(output);
        size_t length = fread(code, 1, sizeof(code) - 1, output);
        code[length] = 0;
        valid = feof(output) && strstr(code, "wind_frames = (script_parameters[0].i)") &&
            strstr(code, "wind_strength = (script_parameters[1].f)") &&
            strstr(code, "host->spawn_wind == 0") &&
            strstr(code, "->dmg.x1830_percent");
    }
    if (output) fclose(output);
    if (!valid) fprintf(stderr, "Owned policy values %s failed: %u %s\n",
        state_callbacks ? "state" : "action", error.line, error.message);
    return valid;
}

static bool common_policies_compile(bool state_callbacks)
{
    char script[2048];
    snprintf(script, sizeof(script),
        "%smotion_policy 341 common 122\n"
        "%s Policy:\n"
        "    on item_swing_motion:\n"
        "        if event.swing_variant == 3: return 341 + event.swing_type\n"
        "        return -1\n"
        "    on cpu_capture_decision:\n"
        "        cpu.run_script(57 + int(capture.victim_damage >= 100))\n"
        "        return true\n"
        "    on taunt:\n"
        "        state.taunt(264, 265)\n",
        state_callbacks ? "" : "",
        state_callbacks ? "move" : "action");
    FILE* output = tmpfile();
    NativeFighterScriptError error = { 0 };
    bool valid = output && nativeFighterScript_GenerateC(script, strlen(script),
        "common_policies_probe", NULL, NULL, state_callbacks ? &(const NativeFighterScriptConstruction){"ftDataCaptain",341,23} : NULL, output, &error);
    if (valid) {
        char code[65536]; rewind(output);
        size_t length = fread(code, 1, sizeof(code) - 1, output);
        code[length] = 0;
        valid = feof(output) &&
            strstr(code, "int swing_type, int swing_variant") &&
            strstr(code, "host->cpu_run_script == 0") &&
            strstr(code, "script_index < FT_CPU_SCRIPT_COUNT") &&
            strstr(code, "engine->enter_taunt(gobj, 264, 265)") &&
            strstr(code, "host->common_motion_states == 0") &&
            strstr(code, "authored_states[341 - base->motion_state_first].anim_cb = host->common_motion_states[122].anim_cb;") &&
            strstr(code, "authored_states[341 - base->motion_state_first].coll_cb = host->common_motion_states[122].coll_cb;") &&
            !strstr(code, "common_motion_states[122].cam_cb");
    }
    if (output) fclose(output);
    if (!valid) fprintf(stderr, "Common policies %s failed: %u %s\n",
        state_callbacks ? "state" : "action", error.line, error.message);
    return valid;
}

static void stock_ground(HSD_GObj* object)
{
    Fighter* fighter = object->user_data;
    fighter->native_module_state[0] = 7;
}

static void stock_air(HSD_GObj* object)
{
    Fighter* fighter = object->user_data;
    fighter->native_module_state[0] = 9;
}

static bool owner_state_layout_matches(bool state_callbacks)
{
    char script[4096];
    snprintf(script, sizeof(script),
        "%s"
        "state bool first\nstate bool second\nstate bool third\n"
        "state int count\nstate float amount\n"
        "%s"
        "        state.first = true\n"
        "        state.second = true\n"
        "        state.third = true\n"
        "        state.count = 17\n"
        "        state.amount = 2.5\n"
        "article It_Kind_Mario_Fire states 1:\n"
        "    state 0 anim -1:\n"
        "        on physics:\n"
        "            owner.state.first = owner.state.second\n"
        "            owner.state.second = owner.state.third\n"
        "            owner.state.third = owner.state.first\n"
        "            owner.state.count = owner.state.count + 1\n"
        "            owner.state.amount = owner.state.amount + 0.5\n",
        state_callbacks ? "" : "",
        state_callbacks ? "move Probe:\n    on state_341_physics:\n"
                        : "action Probe:\n    on load:\n");
    FILE* output = tmpfile();
    NativeFighterScriptError error = { 0 };
    bool valid = output != NULL && nativeFighterScript_GenerateC(
        script, strlen(script), "owner_state_probe", NULL, NULL, state_callbacks ? &(const NativeFighterScriptConstruction){"ftDataMario",341,1} : NULL, output, &error);
    if (valid) {
        char generated[32768];
        rewind(output);
        size_t size = fread(generated, 1, sizeof(generated) - 1, output);
        generated[size] = '\0';
        unsigned first = state_callbacks ? 2 : 1;
        unsigned count = state_callbacks ? 8 : 4;
        unsigned amount = count + 4;
        char expected[256];
        for (unsigned i = 0; i < 3 && valid; ++i) {
            snprintf(expected, sizeof(expected),
                "fighter->native_module_state[script_state_base + %u] = (uint8_t) ((true) != 0)",
                first + i);
            valid = strstr(generated, expected) != NULL;
            snprintf(expected, sizeof(expected),
                "scripted_article_set_owner_state_bool(item, %u, scripted_article_owner_state_bool(item, %u))",
                first + i, first + (i + 1) % 3);
            valid = valid && strstr(generated, expected) != NULL;
        }
        snprintf(expected, sizeof(expected),
            "script_state_set_int(fighter, script_state_base + %u, 17)", count);
        valid = valid && strstr(generated, expected) != NULL;
        snprintf(expected, sizeof(expected),
            "scripted_article_set_owner_state_int(item, %u, scripted_article_owner_state_int(item, %u) + 1)",
            count, count);
        valid = valid && strstr(generated, expected) != NULL;
        snprintf(expected, sizeof(expected),
            "script_state_set_float(fighter, script_state_base + %u, 2.5)", amount);
        valid = valid && strstr(generated, expected) != NULL;
        snprintf(expected, sizeof(expected),
            "scripted_article_set_owner_state_float(item, %u, scripted_article_owner_state_float(item, %u) + 0.5f)",
            amount, amount);
        valid = valid && strstr(generated, expected) != NULL;
    }
    if (output != NULL) fclose(output);
    if (!valid) fprintf(stderr, "Owner state layout (%s) differs: line %u %s\n",
                        state_callbacks ? "base" : "special", error.line, error.message);
    return valid;
}

static bool independent_program_matches(bool state_callbacks)
{
    char source[4096];
    snprintf(source, sizeof(source),
        "# Declaration comments preserve source lines.\r\n"
        "fighter:\r\n    behavior independent\r\n"
        "%s"
        "%s"
        "        pass\n"
        "    on state_342_animation:\n        pass\n"
        "%s Load:\n    on load:\n        pass\n"
        "    on load_special_attrs:\n        pass\n",
        state_callbacks ? "" : "",
        state_callbacks ?
            "move Authored:\n    on state_341_animation:\n" :
            "action Authored:\n    on state_341_animation:\n",
        state_callbacks ? "move" : "action");
    FILE* output = tmpfile();
    NativeFighterScriptError error = { 0 };
    bool valid = output != NULL && nativeFighterScript_GenerateC(source,
        strlen(source), "independent_probe", NULL, NULL, state_callbacks ? &(const NativeFighterScriptConstruction){"ftDataTest",341,3} : NULL, output, &error);
    if (valid) {
        char generated[65536];
        rewind(output);
        size_t length = fread(generated, 1, sizeof(generated) - 1, output);
        generated[length] = 0;
        const char* cleared = strstr(generated, "generated.on_load = 0;");
        const char* authored = strstr(generated, "generated.on_load = authored_");
        valid = length < sizeof(generated) - 1 && cleared && authored &&
            cleared < authored &&
            strstr(generated, "generated.source_kind = Ft_Kind_None;") &&
            strstr(generated, "generated.neutral_article = 0;") &&
            strstr(generated, "generated.on_death1 = 0;") &&
            strstr(generated, "generated.special_air_lw = 0;") &&
            strstr(generated, "generated.article_logics = 0;") &&
            strstr(generated, "original->input_cb != 0 ? 2u : 0u") &&
            strstr(generated, "original->phys_cb != 0 ? 4u : 0u") &&
            strstr(generated, "original->coll_cb != 0 ? 8u : 0u") &&
            strstr(generated, "(authored_motion_fields[i] & required) != required") &&
            strstr(generated, state_callbacks ?
                "authored_motion_fields[0] |= 1u;" :
                "authored_motion_fields[341 - base->motion_state_first] |= 1u;") &&
            strstr(generated, state_callbacks ?
                "authored_motion_fields[1] |= 1u;" :
                "authored_motion_fields[342 - base->motion_state_first] |= 1u;") &&
            !strstr(generated, "original->cam_cb");
    }
    if (output) fclose(output);
    if (!valid) fprintf(stderr, "Independent %s emission failed: %u %s\n",
                        state_callbacks ? "state" : "special", error.line, error.message);
    return valid;
}

static void independent_change(Fighter_GObj* object, FtMotionId motion,
                               MotionFlags flags, float start, float speed,
                               float blend, Fighter_GObj* other)
{
    (void) flags; (void) start; (void) speed; (void) blend; (void) other;
    ((Fighter*) object->user_data)->motion_id = motion;
}

static void independent_start(Fighter_GObj* object)
{
    (void) object;
}

static bool independent_module_matches(const char* filename)
{
    typedef const NativeFighterDefinition* (__cdecl* Entry)(
        const NativeFighterDefinition*, const NativeFighterHost*);
    HMODULE library = LoadLibraryA(filename);
    Entry entry = library ? (Entry) GetProcAddress(library,
        "FreakMelee_GetFighterModule") : NULL;
    if (!entry) {
        if (library) FreeLibrary(library);
        return false;
    }
    MotionState motions[3] = {
        { .anim_id = 7, .x4_flags = 0x1234, ._ = 0x5678,
          .anim_cb = stock_ground, .input_cb = stock_ground,
          .phys_cb = stock_ground, .coll_cb = stock_ground,
          .cam_cb = stock_ground },
        { .anim_id = 8, .anim_cb = stock_ground, .cam_cb = stock_ground },
        { .anim_id = 9, .cam_cb = stock_ground },
    };
    const NativeFighterBone bones[] = { { FtPart_TopN, 0 } };
    const NativeFighterArticleAsset assets[] = { { It_Kind_Mario_Fire, 0, -1 } };
    const Fighter_DemoStrings demos = { "result", "intro", "ending", "wait" };
    const NativeItemDefinition inherited_item = { .on_thrown = stock_ground };
    const NativeFighterArticleLogic inherited_article = { 0 };
    NativeFighterDefinition base = {
        .code_key = "raw_catalog", .source_kind = Ft_Kind_Mario,
        .fighter_root = "ftDataTest", .costume_joint = "joint",
        .costume_matanim = "material", .action_count = 303, .demo_count = 16,
        .motion_state_first = 341, .motion_state_count = 3,
        .motion_states = motions, .joint_count = 61,
        .bones = bones, .bone_count = 1,
        .animation_source_kind = Ft_Kind_Mario,
        .sound_banks = UINT64_C(1) << 20, .effect_bank = 13,
        .asset_layout = { .article_mask = 1 },
        .article_asset_count = 1, .article_assets = assets,
        .demo_motion_names = &demos,
        .on_load = stock_ground, .on_load_special_attrs = stock_ground,
        .on_tick = stock_ground, .on_death = stock_ground,
        .on_death1 = stock_ground, .on_knockback_enter = stock_ground,
        .on_color_animation_reset = stock_ground,
        .on_cpu_charge_cancel = stock_ground,
        .special_ground_state = { 341, 0, 0, 0 },
        .special_n = stock_ground, .special_air_n = stock_ground,
        .special_s = stock_ground, .special_air_s = stock_ground,
        .special_hi = stock_ground, .special_air_hi = stock_ground,
        .special_lw = stock_ground, .special_air_lw = stock_ground,
        .neutral_article = stock_ground,
        .items = &inherited_item, .item_count = 1,
        .article_logics = &inherited_article, .article_logic_count = 1,
        .owns_model_attachments = true,
    };
    NativeFighterHost host = {
        .callback_source_alive = script_lifetime_alive,
        .change_motion_state = independent_change,
        .start_animation = independent_start,
    };
    const NativeFighterDefinition* authored = entry(&base, &host);
    bool valid = authored && authored->source_kind == Ft_Kind_None &&
        base.source_kind == Ft_Kind_Mario && base.special_n == stock_ground &&
        authored->on_load && authored->on_load != stock_ground &&
        authored->on_load_special_attrs && authored->on_load_special_attrs != stock_ground &&
        !authored->on_tick && !authored->on_death && !authored->on_death1 &&
        !authored->on_knockback_enter && !authored->on_color_animation_reset &&
        !authored->on_cpu_charge_cancel && !authored->neutral_article &&
        authored->special_n && authored->special_n != stock_ground &&
        !authored->special_air_n && !authored->special_s && !authored->special_air_s &&
        !authored->special_hi && !authored->special_air_hi &&
        !authored->special_lw && !authored->special_air_lw &&
        !authored->items && !authored->item_count &&
        !authored->article_logics && !authored->article_logic_count &&
        !authored->owns_model_attachments &&
        authored->bones == bones && authored->bone_count == 1 &&
        authored->joint_count == 61 && authored->action_count == 303 &&
        authored->demo_count == 16 && authored->demo_motion_names == &demos &&
        authored->fighter_root == base.fighter_root &&
        authored->costume_joint == base.costume_joint &&
        authored->costume_matanim == base.costume_matanim &&
        authored->animation_source_kind == Ft_Kind_Mario &&
        authored->sound_banks == base.sound_banks && authored->effect_bank == 13 &&
        authored->asset_layout.article_mask == 1 &&
        authored->article_assets == assets && authored->article_asset_count == 1 &&
        authored->motion_states != motions &&
        authored->motion_states[0].anim_id == 7 &&
        authored->motion_states[0].x4_flags == 0x1234 &&
        authored->motion_states[0]._ == 0x5678 &&
        authored->motion_states[0].cam_cb == stock_ground &&
        authored->motion_states[2].cam_cb == stock_ground;
    for (unsigned state = 0; state < 3 && valid; ++state) {
        MotionState original = motions[state];
        for (unsigned mask = 0; mask < 16 && valid; ++mask) {
            motions[state].anim_cb = mask & 1 ? stock_ground : NULL;
            motions[state].input_cb = mask & 2 ? stock_ground : NULL;
            motions[state].phys_cb = mask & 4 ? stock_ground : NULL;
            motions[state].coll_cb = mask & 8 ? stock_ground : NULL;
            unsigned covered = state == 0 ? 15 : state == 1 ? 1 : 0;
            bool expected = (mask & covered) == mask;
            bool accepted = entry(&base, &host) != NULL;
            valid = expected == accepted;
            if (!valid) fprintf(stderr,
                "Independent motion %u mask %u accepted=%d expected=%d\n",
                state, mask, accepted, expected);
        }
        motions[state] = original;
    }
    FreeLibrary(library);
    if (!valid) fputs("Independent module retained behavior or rejected owned metadata.\n", stderr);
    else puts("Independent module preserved metadata and passed 48 callback ownership combinations.");
    return valid;
}

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    if (strcmp(argv[1], "--independent") == 0)
        return independent_module_matches(argv[2]) ? 0 : 1;
    if (!owner_state_layout_matches(false) || !owner_state_layout_matches(true))
        return 1;
    if (!independent_program_matches(false) || !independent_program_matches(true))
        return 1;
    if (!policy_values_compile(false) || !policy_values_compile(true)) return 1;
    if (!common_policies_compile(false) || !common_policies_compile(true)) return 1;
    if (!capture_owner_compile()) return 1;
    ScriptStateTable maximum_state = { 0 };
    for (unsigned i = 0; i < SCRIPT_STATE_FIELDS; ++i) {
        char declaration[64];
        snprintf(declaration, sizeof(declaration), "state int value_%u", i);
        if (!nativeFighterScript_AddStateField(&maximum_state, declaration) ||
            maximum_state.fields[i].offset != i * sizeof(int32_t)) return 1;
    }
    if (maximum_state.bytes != 256 ||
        nativeFighterScript_AddStateField(&maximum_state, "state bool overflow"))
        return 1;
    FILE* source = fopen(argv[2], "rb");
    char source_bytes[65536];
    size_t source_size = source != NULL
        ? fread(source_bytes, 1, sizeof(source_bytes), source) : 0;
    if (source != NULL) fclose(source);
    if (source_size == 0 || source_size == sizeof(source_bytes)) return 1;
    static const NativeFighterScriptConstruction invalid_construction[]={{"ftDataRook",341,12},{"ftDataTest",341,1}};
    static const struct {
        const char* source;
        unsigned error_line; const NativeFighterScriptConstruction* construction;
    } invalid[] = {
        { "action Bad:\n    on cpu_capture_decision:\n        cpu.run_script(57.5)\n", 3, NULL },
        { "action Bad:\n    on cpu_capture_decision:\n        cpu.run_script(missing)\n", 3, NULL },
        { "action Bad:\n    on taunt:\n        return event.swing_type\n", 3, NULL },
        { "motion_policy 341 common 341\naction Policy:\n    on taunt:\n        pass\n", 1, NULL },
        { "motion_policy 341 common 122\nmotion_policy 341 common 126\naction Policy:\n    on taunt:\n        pass\n", 2, NULL },
        { "action Policy:\n    on taunt:\n        pass\nmotion_policy 341 common 122\n", 4, NULL },
        { "action Policy:\n    on taunt:\n        state.taunt(65536, 65536)\n", 3, NULL },
        { "action Bad:\n    on taunt:\n        effects.wind(Top, 0, 80.5, 1, 0.003, 1)\n", 3, NULL },
        { "action Bad:\n    on taunt:\n        effects.wind(Top, 0, missing, 1, 0.003, 1)\n", 3, NULL },
        { "action Bad:\n    on taunt:\n        effects.wind(MissingBone, 0, 80, 1, 0.003, 1)\n", 3, NULL },
        { "article It_Kind_Mario_Fire states 1:\n    on created:\n            effects.wind(Top, 0, 80, 1, 0.003, 1)\n", 3, NULL },
        { "fighter:\n    behavior independent\n    behavior independent\n", 3, NULL },
        { "fighter:\n    behavior stock\n", 2, NULL },
        { "fighter:\n    behavior independent\nfighter:\n    behavior independent\n", 3, NULL },
        { "fighter:\n", 1, NULL },
        { "fighter:\naction Empty:\n", 1, NULL },
        { "    behavior independent\n", 1, NULL },
        { "action Late:\n    on load:\n        pass\nfighter:\n    behavior independent\n", 4, NULL },
        { "fighter:\n    behavior independent\naction Missing:\n    on load:\n        pass\n", 0, NULL },
        { "fighter:\n    behavior independent\nmove Missing:\n    on load_special_attrs:\n        pass\n", 0, &invalid_construction[1] },
        { "fighter:\n    behavior independent\nmove Inherited:\n    on special_n:\n        stock.enter()\naction Load:\n    on load:\n        pass\n    on load_special_attrs:\n        pass\n", 4, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    state 0 anim 0:\n        on collision:\n            item.follow_ground()\n", 4, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    state 0 anim 0:\n        on collision:\n            item.bounce_to_ground()\n", 4, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    on signal update:\n            owner.call(missing, 1)\n", 3, NULL },
        { "func update(float speed):\n    velocity.x = speed\narticle It_Kind_Fox_Blaster states 1:\n    on signal update:\n            owner.call(update)\n", 5, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    on signal loop:\n            item.signal(\"loop\", 0)\n", 3, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    state float x\n    state float y\n    state float z\n    state bool found\n    on signal query:\n            targets.fighter_ahead(item.state.x, item.state.x, item.state.z, item.state.found)\n", 7, NULL },
        { "action WrongObserver:\n    on load:\n        velocity.x = other.position.x\n", 3, NULL },
        { "action WrongObserver:\n    on cpu_initialize:\n        velocity.x = other.cpu.body_half_width\n", 3, NULL },
        { "action WrongFloorOutput:\n    on load:\n        stage.read_floor(0, 0, 1, 1, state.missing, state.missing, state.missing)\n", 3, NULL },
        { "action WrongScalar:\n    on load:\n        velocity.x = event.value\n", 3, NULL },
        { "action BadCommand:\n    on cpu_initialize:\n        cpu.queue(Cpu.press_a, 2)\n", 3, NULL },
        { "action BadCommand:\n    on cpu_initialize:\n        cpu.queue(Cpu.wait)\n", 3, NULL },
        { "action BadCommand:\n    on cpu_initialize:\n        cpu.queue(127)\n", 3, NULL },
        { "action BadCommand:\n    on cpu_initialize:\n        cpu.queue(Cpu.unknown)\n", 3, NULL },
        { "action BadOutput:\n    on load:\n        stage.island_bounds(0, 0, 0, -10, state.missing, x, y, z, w)\n", 3, NULL },
        { "action InvalidClip:\n    on load:\n        velocity.x = animation.clip_length(-4294967295)\n", 3, NULL },
        { "action InvalidClip:\n    on load:\n        velocity.x = animation.clip_length(65536)\n", 3, NULL },
        { "action InvalidClip:\n    on load:\n        velocity.x = animation.clip_length(input.x)\n", 3, NULL },
        { "action Guard:\n    on load:\n        guard.create(0, 0, 0, 0, 1)\n", 3, NULL },
        { "move Guard:\n    on load:\n        guard.create(0, 0, 0, 0, 1)\n", 3, &invalid_construction[1] },
        { "action WrongContext:\n    on load:\n"
          "        trail.bone = 1\n", 3, NULL },
        { "action UnknownTrail:\n    on trail:\n"
          "        trail.missing = 1\n", 3, NULL },
        { "action MutatingTrail:\n    on trail:\n"
          "        velocity.x = 1\n", 3, NULL },
        { "state int age\naction MutatingTrail:\n    on trail:\n"
          "        state.age = state.age + 1\n", 4, NULL },
        { "action RandomTrail:\n    on trail:\n"
          "        trail.bone = random.int(3)\n", 3, NULL },
        { "action ConsumingTrail:\n    on trail:\n"
          "        if cue.article.consume(): trail.bone = 1\n", 3, NULL },
        { "action MutatingExpression:\n    on trail:\n"
          "        trail.inner_distance = position.x = 2\n", 3, NULL },
        { "action MutatingExpression:\n    on trail:\n"
          "        trail.inner_distance = ++position.x\n", 3, NULL },
        { "action MutatingCondition:\n    on trail:\n"
          "        if position.x = 2: trail.bone = 1\n", 3, NULL },
        { "action MutatingInline:\n    on trail:\n"
          "        if grounded: velocity.x = 2\n", 3, NULL },
        { "func Mutate():\n    velocity.x = 1\naction Trail:\n"
          "    on trail:\n        Mutate()\n", 5, NULL },
        { "action WrongContext:\n    on load:\n"
          "        velocity.x = event.motion\n", 3, NULL },
        { "action WrongPredicate:\n    on idle_variants:\n"
          "        return event.motion == 51\n", 3, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n        on animation:\n"
          "            item.life = item.age\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n        on animation:\n"
          "            item.joint(\"0/\").scale.x = 1\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n        on animation:\n"
          "            item.joint(\"256\").translation.y = 1\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n        on animation:\n"
          "            item.life = item.joint(\"\").scale.w\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state int counter\n    state bool counter\n", 3, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n        on animation:\n"
          "            item.state.missing = 1\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n        on animation:\n"
          "            item.state0 = 1\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state int counter\n    state 0 anim -1:\n"
          "        on animation:\n            item.life = item.state.missing\n", 5, NULL },
        { "move Init:\n    on load:\n"
          "        bones.root.rotate_y(1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1 + 1)\n", 3, &invalid_construction[0] },
        { "move Init:\n    on load:\n"
          "        callbacks.track_reflect_hit()\n", 0, &invalid_construction[0] },
        { "func Setup():\n    callbacks.track_reflect_hit()\nmove Bad:\n"
          "    on special_n:\n        stock.enter()\n", 0, NULL },
        { "func Bad():\n    while true:\n    pass\nmove Bad:\n"
          "    on special_n:\n        stock.enter()\n", 3, NULL },
        { "func Bad():\n    while false: pass\n    else: pass\nmove Bad:\n"
          "    on special_n:\n        stock.enter()\n", 3, NULL },
        { "func Duplicate():\n    pass\nfunc Duplicate():\n    pass\n", 3, NULL },
        { "func Empty():\nmove Bad:\n    on special_n:\n        stock.enter()\n", 1, NULL },
        { "move Init:\n    on load:\n"
          "        callbacks.track_reflect()\n", 0, &invalid_construction[0] },
        { "func Setup():\n    callbacks.track_reflect()\nmove Bad:\n"
          "    on special_n:\n        stock.enter()\n", 0, NULL },
        { "move Bad:\n    on load:\n"
          "        hitbox.set_enabled(4.5, true)\n", 3, &invalid_construction[0] },
        { "action Bad:\n    on load:\n        hitbox.set_enabled(0, 1)\n", 3, NULL },
        { "action Bad:\n    on load:\n        controls.interrupt_allowed = 1\n", 3, NULL },
        { "action Bad:\n    on load:\n        damage.song_immune = 1\n", 3, NULL },
        { "action Bad:\n    on up_smash_enter:\n        velocity.x = event.angle\n", 3, NULL },
        { "action Bad:\n    on load:\n        let float x = 0\n        let int y = 0\n        let float z = 0\n        bones.world_position(0, x, y, z)\n", 6, NULL },
        { "action Bad:\n    on load:\n        let float x = 0\n        bones.world_position(0, x, x, x)\n", 4, NULL },
        { "action Bad:\n    on load:\n        let float x = 0\n        let float y = 0\n        let float z = 0\n        bones.world_position(0.5, x, y, z)\n", 6, NULL },
        { "action Bad:\n    on load:\n        vectors.rotate(1, 2, 3, Axis.z, 0.5, x, y, z)\n", 3, NULL },
        { "action Bad:\n    on load:\n        let float x = 0\n        let float y = 0\n        vectors.rotate(1, 2, 3, Axis.z, 0.5, x, y, x)\n", 5, NULL },
        { "move Bad:\n    on load:\n"
          "        hitbox.set_group(0, missing_value)\n", 3, &invalid_construction[0] },
        { "move Bad:\n    on load:\n"
          "        velocity.x = 0x100000000\n", 3, &invalid_construction[0] },
        { "move Bad:\n    on load:\n"
          "        velocity.x = 0x\n", 3, &invalid_construction[0] },
        { "action Bad:\n    on jab_start:\n        return event.victim_motion\n", 3, NULL },
        { "func NeedsArgument(int value):\n    pass\naction Bad:\n"
          "    on jab_start:\n        callbacks.state_changed(NeedsArgument)\n", 5, NULL },
        { "func Wrong(string argument):\n    pass\n", 1, NULL },
        { "func Repeated(int value, float value):\n    pass\n", 1, NULL },
        { "func Recurse():\n    Recurse()\nmove Bad:\n"
          "    on special_n:\n        stock.enter()\n", 1, NULL },
        { "func First():\n    Second()\nfunc Second():\n    First()\n"
          "move Bad:\n    on special_n:\n        stock.enter()\n", 1, NULL },
        { "func One(int value):\n    pass\nmove Bad:\n"
          "    on special_n:\n        One()\n", 5, NULL },
        { "func None():\n    pass\nmove Bad:\n"
          "    on special_n:\n        None(1)\n", 5, NULL },
        { "func One(int value):\n    pass\nmove Bad:\n"
          "    on special_n:\n        One(1,)\n", 5, NULL },
        { "func Broken():\n    undefined_operation()\nmove Bad:\n"
          "    on special_n:\n        stock.enter()\n", 2, NULL },
        { "func Setup():\n    callbacks.track_damage()\nmove Bad:\n"
          "    on special_n:\n        stock.enter()\n", 5, NULL },
        { "move Bad:\n    on special_s:\n        state.enter(SideSpecial)\n"
          "    on side_special_cue:\n        pass\n", 4, NULL },
        { "move Bad:\n    on special_n:\n        state.enter(NeutralSpecial)\n"
          "    on accessory:\n        pass\n"
          "    on neutral_special_cue:\n        pass\n", 6, NULL },
        { "move Bad:\n    on special_n:\n        stock.enter()\n"
          "        state.enter_flags(343, 0)\n", 4, NULL },
        { "move Bad:\n"
          "    on state_341_collision:\n"
          "        callbacks.track_accessory(Up)\n", 3, &invalid_construction[0] },
        { "move Bad:\n    on special_n:\n"
          "        articles.spawn_cape(RightThumb)\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        callbacks.track_cape()\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        articles.remove_cape()\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        articles.forget_cape()\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        charge.cyclone = true\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        charge.tornado = true\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        charge.cape_boost = true\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        velocity.y = charge.tornado\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        velocity.y = charge.cape_boost\n", 3, NULL },
        { "move Bad:\n    on special_n:\n"
          "        velocity.y = attrs.specialhi.grav\n", 3, NULL },
        { "move Bad:\n    on special_n:\n        stock.enter()\n"
          "        motion.add_vertical_speed(attrs.unknown)\n", 4, NULL },
        { "move Bad:\n    on special_n:\n        stock.enter()\n"
          "        stock.enter()\n", 4, NULL },
        { "move Bad:\n    on special_n:\n        stock.enter()\n"
          "        state.enter(NeutralSpecial)\n", 4, NULL },
        { "move Bad:\n    on special_n:\n"
          "        motion.add_facing_speed(1.0)\n"
          "        stock.enter()\n", 4, NULL },
        { "move Neutral:\n    on special_n:\n        stock.enter()\n"
          "    on special_air_n:\n        stock.enter()\n\n"
          "action Taunt:\n    on state_341_animation:\n"
          "        callbacks.track_damage()\n", 9, NULL },
        { "move Init:\n    on load:\n"
          "        callbacks.track_death2()\n", 3, &invalid_construction[0] },
        { "move Init:\n    on load:\n"
          "        callbacks.track_death1()\n", 3, &invalid_construction[0] },
        { "action Init:\n    on load:\n"
          "        callbacks.track_death1()\n", 3, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n"
          "        on animation:\n"
          "            item.change_state(1)\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n"
          "        on animation:\n"
          "            item.set_lifetime(unknown_value)\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n"
          "        on animation:\n"
          "            item.spawn_effect_on_bone(1147, 0, 2)\n", 4, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    attribute float speed 0\n"
          "    attribute float angle 0\n", 3, NULL },
        { "article It_Kind_Mario_Fire states 1:\n"
          "    attribute float speed 0\n"
          "    state 0 anim -1:\n"
          "        on animation:\n"
          "            item.life = item.attr.unknown\n", 5, NULL },
        { "state int known\n"
          "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n"
          "        on animation:\n"
          "            item.life = owner.state.missing\n", 5, NULL },
        { "state int known\n"
          "article It_Kind_Mario_Fire states 1:\n"
          "    state 0 anim -1:\n"
          "        on animation:\n"
          "            owner.state.missing = 1\n", 5, NULL },
        { "move Init:\n"
          "    on tick:\n"
          "        if event.flag: fighter.self_damage(1)\n", 3, &invalid_construction[0] },
        { "article It_Kind_Fox_Laser states 1:\n"
          "    projectile appearance -1 state 0 speed 1 angle 0 lifetime 35 initial_velocity off initial_velocity on\n", 2, NULL },
        { "article It_Kind_Fox_Laser states 1:\n"
          "    projectile appearance -1 state 0 speed 1 angle 0 lifetime 35 initial_update maybe\n", 2, NULL },
        { "article It_Kind_Fox_Laser states 1:\n"
          "    on launched:\n"
          "            item.life = 35\n"
          "    state 0 anim 0:\n", 4, NULL },
        { "article It_Kind_Fox_Laser states 1:\n"
          "    state 0 anim 0:\n"
          "        on physics:\n"
          "            item.launch_speed = 3\n", 4, NULL },
        { "article It_Kind_Fox_Laser states 1:\n"
          "    state bool hit\n"
          "    state 0 anim 0:\n"
          "        on collision:\n"
          "            item.state.hit = item.sweep_stage(0, 0)\n", 5, NULL },
        { "action Fire:\n"
          "    on state_341_animation:\n"
          "        articles.launch_projectile(It_Kind_Fox_Laser, UnknownBone, 0, 0, 0, 0, 5, 0)\n", 3, NULL },
        { "action Fire:\n"
          "    on state_341_animation:\n"
          "        articles.launch_projectile(It_Kind_Fox_Laser, RightThumb, 0, 0, 0, 0, 5)\n", 3, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    on signal open:\n", 2, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    on signal open:\n            item.life = 1\n    on signal open:\n", 4, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    state 0 anim 0:\n        on physics:\n            item.life = signal.value\n", 4, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    table float weights = []\n", 2, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    table float weights = [1, nan]\n", 2, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    table float weights = [1, 2]\n    table float weights = [3]\n", 3, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    table float weights = [1, 2]\n    state 0 anim 0:\n        on physics:\n            item.life = table.missing[0]\n", 5, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    state 0 anim 0 use 0:\n", 2, NULL },
        { "article It_Kind_Fox_Blaster states 2:\n    state 0 anim 0 use 1:\n", 2, NULL },
        { "article It_Kind_Fox_Blaster states 2:\n    state 0 anim 0:\n    state 1 anim 1 use 0:\n        on physics:\n", 4, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    state 0 anim 0:\n        on physics:\n            item.joint(\"0\").exists = true\n", 4, NULL },
        { "action Send:\n    on state_341_animation:\n        articles.signal_owned(17, \"not a name\", 0)\n", 3, NULL },
        { "action Send:\n    on state_341_animation:\n        articles.signal_owned(17, \"open\")\n", 3, NULL },
        { "action Carry:\n    on heavy_throw_enter:\n        callbacks.physics(missing)\n", 3, NULL },
        { "func Needs(int value):\n    pass\naction Carry:\n    on heavy_throw_enter:\n        callbacks.physics(Needs)\n", 5, NULL },
        { "action Carry:\n    on heavy_throw_enter:\n        callbacks.physics(77)\n", 3, NULL },
        { "action Turn:\n    on load:\n        callbacks.reflect(missing)\n", 3, NULL },
        { "func Needs(int value):\n    pass\naction Turn:\n    on load:\n        callbacks.reflect(Needs)\n", 5, NULL },
        { "action Scale:\n    on load:\n        bones.set_scale(UnknownBone, 1, 1, 1)\n", 3, NULL },
        { "action Scale:\n    on load:\n        bones.set_scale(XRotN, 1, missing, 1)\n", 3, NULL },
        { "action Land:\n    on load:\n        if collision.check_ground_ledge(1, 0): pass\n", 3, NULL },
        { "func Needs(int value):\n    pass\naction Damage:\n    on load:\n        callbacks.damage(Needs)\n", 5, NULL },
        { "action Create:\n    on load:\n        articles.create_at_position(81, 0, 0, 0, 1, false, 7)\n", 3, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    on signal choose:\n            if signal.value > 0:\n    state 0 anim -1:\n", 4, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    on signal choose:\n            else:\n                item.life = 1\n", 3, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    on signal choose:\n            if signal.value > 0:\n                    item.life = 1\n", 4, NULL },
        { "article It_Kind_Fox_Blaster states 1:\n    on signal choose:\n            if signal.value > 0:\n                item.life = 1\n            else:\n                item.life = 2\n            elif signal.value < 0:\n                item.life = 3\n", 7, NULL },
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        FILE* rejected_output = tmpfile();
        NativeFighterScriptError error = { 0 };
        bool rejected = rejected_output != NULL &&
            !nativeFighterScript_GenerateC(
                invalid[i].source, strlen(invalid[i].source),
                "script_probe", NULL, NULL, invalid[i].construction, rejected_output, &error) &&
            error.line == invalid[i].error_line;
        if (rejected_output != NULL) fclose(rejected_output);
        if (!rejected) {
            fprintf(stderr, "Invalid script case %u: expected rejection at line %u, got line %u (%s).\n",
                    i, invalid[i].error_line, error.line, error.message);
            return 1;
        }
    }
    // Many owned motion callbacks must fit without overflowing the generated
    // host-check list. Exercise both source forms and reject one extra handler.
    char many_handlers[32768];
    for (unsigned form = 0; form < 2; ++form) {
        size_t used = (size_t) snprintf(many_handlers, sizeof(many_handlers),
            form ? "action Full:\n" : "move Full:\n");
        for (unsigned i = 0; i < 512; ++i) {
            used += (size_t) snprintf(many_handlers + used, sizeof(many_handlers) - used,
                "    on state_%u_animation:\n        pass\n", i);
        }
        for (unsigned extra = 0; extra < 2; ++extra) {
            if (extra) used += (size_t) snprintf(many_handlers + used,
                sizeof(many_handlers) - used,
                "    on idle_variants:\n        return true\n");
            FILE* generated = tmpfile();
            NativeFighterScriptError error = { 0 };
            bool accepted = generated != NULL && nativeFighterScript_GenerateC(
                many_handlers, used, "many_handlers", NULL, NULL, form ? NULL : &(const NativeFighterScriptConstruction){"ftDataTest",0,512}, generated, &error);
            if (generated != NULL) fclose(generated);
            if (accepted != (extra == 0)) {
                fprintf(stderr, "Handler boundary form %u extra %u accepted=%d: %s\n",
                        form, extra, accepted, error.message);
                return 1;
            }
        }
    }
    // Exercise the high half of the function-call bitset, including an
    // indirect recursion cycle that reaches the final supported helper.
    char many_functions[16384];
    for (unsigned cycle = 0; cycle < 2; ++cycle) {
        size_t used = 0;
        for (unsigned i = 0; i < 64; ++i) {
            used += snprintf(many_functions + used, sizeof(many_functions) - used,
                             "func Helper%u():\n", i);
            used += snprintf(many_functions + used, sizeof(many_functions) - used,
                i < 63 ? "    Helper%u()\n" : cycle ? "    Helper%u()\n" : "    pass\n",
                i < 63 ? i + 1 : 0);
        }
        used += snprintf(many_functions + used, sizeof(many_functions) - used,
            "action Many:\n    on state_341_animation:\n        Helper0()\n");
        FILE* generated = tmpfile();
        NativeFighterScriptError error = { 0 };
        bool accepted = generated != NULL && nativeFighterScript_GenerateC(
            many_functions, used, "many_helpers", NULL, NULL, NULL, generated, &error);
        if (generated != NULL) fclose(generated);
        if (accepted != (cycle == 0) || (cycle && error.line != 1)) {
            fprintf(stderr, "Helper boundary cycle %u accepted=%d line=%u: %s\n",
                    cycle, accepted, error.line, error.message);
            return 1;
        }
    }
    const char* authored_projectile_source =
        "move Fire:\n"
        "    on special_n:\n"
        "        state.enter(NeutralSpecial)\n"
        "    on special_air_n:\n"
        "        state.enter(NeutralSpecial)\n"
        "    on neutral_special_cue:\n"
        "        state.once()\n"
        "        articles.spawn_authored_projectile(It_Kind_Mario_Fire, LeftHand, 2)\n"
        "article It_Kind_Mario_Fire states 1:\n"
        "    attribute float speed 0\n"
        "    attribute float angle 4\n"
        "    attribute float lifetime 8\n"
        "    projectile appearance 0 state 0 speed speed angle angle lifetime lifetime\n"
        "    state 0 anim 0:\n"
        "        on animation:\n"
        "            return false\n";
    FILE* projectile_output = tmpfile();
    NativeFighterScriptError projectile_error = { 0 };
    bool projectile_compiled = projectile_output != NULL &&
        nativeFighterScript_GenerateC(authored_projectile_source,
            strlen(authored_projectile_source), "authored_projectile_probe",
            NULL, NULL, NULL, projectile_output, &projectile_error);
    if (projectile_compiled) {
        rewind(projectile_output);
        char generated[32768];
        size_t size = fread(generated, 1, sizeof(generated) - 1,
                            projectile_output);
        generated[size] = '\0';
        projectile_compiled =
            strstr(generated, "engine->spawn_authored_projectile(") != NULL &&
            strstr(generated, "host->spawn_authored_projectile == 0") != NULL;
    }
    if (projectile_output != NULL) fclose(projectile_output);
    if (!projectile_compiled) {
        fprintf(stderr, "Authored projectile cue did not compile: line %u %s\n",
                projectile_error.line, projectile_error.message);
        return 1;
    }
    NativeFighterParameters parameters;
    const char* parameter_source = "float lift = 1.25\n";
    if (!nativeFighterParameters_Parse(parameter_source,
            strlen(parameter_source), &parameters, NULL)) return 1;
    const char* parameter_script =
        "action Launch:\n"
        "    on state_341_physics:\n"
        "        motion.add_vertical_speed(attrs.lift)\n"
        "item Toss damage 1 angle 361 lifetime 60:\n"
        "    on thrown:\n"
        "        velocity.x = attrs.lift\n"
        "article It_Kind_Mario_Fire states 1:\n"
        "    state 0 anim -1:\n"
        "        on physics:\n"
        "            velocity.y = attrs.lift\n";
    FILE* parameter_output = tmpfile();
    NativeFighterScriptError parameter_error = { 0 };
    bool parameter_compiled = parameter_output != NULL &&
        nativeFighterScript_GenerateC(parameter_script,
            strlen(parameter_script), "parameter_probe", NULL,
            &parameters, NULL, parameter_output, &parameter_error);
    if (parameter_compiled) {
        rewind(parameter_output);
        char generated[32768];
        size_t size = fread(generated, 1, sizeof(generated) - 1,
                            parameter_output);
        generated[size] = '\0';
        parameter_compiled =
            strstr(generated, "FreakMelee_SetFighterParameters") != NULL &&
            strstr(generated, "fighter->self_vel.y += script_parameters[0].f") != NULL &&
            strstr(generated, "item->x40_vel.x = script_parameters[0].f") != NULL &&
            strstr(generated, "item->x40_vel.y = script_parameters[0].f") != NULL;
    }
    if (parameter_output != NULL) fclose(parameter_output);
    if (!parameter_compiled) {
        fprintf(stderr, "Project-owned parameter did not compile: %u %s\n",
                parameter_error.line, parameter_error.message);
        return 1;
    }
    MotionState state = { .anim_id = 0 };
    NativeFighterDefinition base = {
        .code_key = "mario_clone",
        .source_kind = Ft_Kind_Mario,
        .animation_source_kind = Ft_Kind_Mario,
        .fighter_root = "ftDataMario",
        .costume_joint = "PlyMario5K_Share_joint",
        .costume_matanim = "PlyMario5K_Share_matanim_joint",
        .action_count = 303,
        .demo_count = 16,
        .motion_state_count = 1,
        .motion_states = &state,
        .joint_count = 61,
        .special_n = stock_ground,
        .special_air_n = stock_air,
    };
    NativeFighterModule module = { 0 };
    NativeFighterDefinition missing_stock = base;
    missing_stock.special_n = NULL;
    NativeFighterModule rejected = { 0 };
    if (nativeFighterModule_Open(argv[1], &missing_stock, &rejected)) {
        nativeFighterModule_Close(&rejected);
        fputs("Generated move accepted a missing stock entry.\n", stderr);
        return 1;
    }
    if (!nativeFighterModule_Open(argv[1], &base, &module) ||
        module.definition->special_n == NULL ||
        module.definition->special_air_n == NULL) {
        fputs("Could not load generated move module.\n", stderr);
        return 1;
    }
    typedef unsigned long long (__cdecl* ScriptSourceHash)(void);
    ScriptSourceHash source_hash = (ScriptSourceHash) GetProcAddress(
        (HMODULE) module.library, "FreakMelee_GetScriptSourceHash");
    uint64_t expected_hash = nativeFighterScript_SourceHash(source_bytes,
                                                            source_size);
    bool hash_correct = source_hash != NULL &&
                        source_hash() == expected_hash;
    source_bytes[0] ^= 1;
    hash_correct = hash_correct &&
        nativeFighterScript_SourceHash(source_bytes, source_size) !=
            expected_hash;
    Fighter* fighters = calloc(2, sizeof(*fighters));
    if (fighters == NULL) return 1;
    HSD_GObj objects[2] = { 0 };
    objects[0].user_data = &fighters[0];
    objects[1].user_data = &fighters[1];
    HSD_GObj* live_sources[HSD_GOBJ_PLINK_ITEM + 1] = { 0 };
    HSD_GObj** previous_sources = HSD_GObjPLinkHead;
    objects[0].classifier = objects[1].classifier = HSD_GOBJ_CLASS_FIGHTER;
    objects[0].p_link = objects[1].p_link = HSD_GOBJ_PLINK_FIGHTER;
    objects[0].next = &objects[1]; objects[1].prev = &objects[0];
    fighters[0].x8_spawnNum = UINT32_C(0x80000001);
    fighters[1].x8_spawnNum = UINT32_C(16777217);
    live_sources[HSD_GOBJ_PLINK_FIGHTER] = &objects[0];
    HSD_GObjPLinkHead = live_sources;
    fighters[0].facing_dir = 1.0f;
    fighters[1].facing_dir = -1.0f;
    module.definition->special_n(&objects[0]);
    module.definition->special_air_n(&objects[1]);
    bool correct = hash_correct &&
                   fabsf(fighters[0].gr_vel - 0.8f) < 0.00001f &&
                   fabsf(fighters[1].self_vel.x + 0.4f) < 0.00001f &&
                   fighters[0].native_module_state[0] == 7 &&
                   fighters[1].native_module_state[0] == 9 &&
                   fighters[0].self_vel.x == 0.0f &&
                   fighters[1].gr_vel == 0.0f &&
                   fighters[0].self_vel.y == 0.0f &&
                   fabsf(fighters[1].self_vel.y - 1.25f) < 0.00001f;
    HSD_GObjPLinkHead = previous_sources;
    free(fighters);
    nativeFighterModule_Close(&module);
    if (!correct) {
        fputs("Generated callback changed the wrong velocity.\n", stderr);
        return 1;
    }
    puts("Generated script module applied facing-relative speeds.");
    return 0;
}
