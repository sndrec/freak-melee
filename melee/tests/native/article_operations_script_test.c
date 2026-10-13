#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_signal.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool generate(const char* source, char* output, size_t capacity,
                      NativeFighterScriptError* error)
{
    FILE* file = tmpfile();
    if (file == NULL) return false;
    bool result = nativeFighterScript_GenerateC(source, strlen(source),
        "article_model_probe", NULL, NULL, NULL, file, error);
    if (result) {
        rewind(file);
        size_t size = fread(output, 1, capacity - 1, file);
        output[size] = '\0';
        result = feof(file) != 0;
    }
    fclose(file);
    return result;
}

int main(void)
{
    const char* source =
        "article It_Kind_Mario_Fire states 1:\n"
        "    attachment origin center previous bone plane off initial_collision on held on\n"
        "    model_root first 36\n"
        "    model_root second 40\n"
        "    model_animation first_anim 76\n"
        "    model_animation second_anim 88\n"
        "    state float angle\n"
        "    state float nx\n    state float ny\n    state float nz\n    state float radius\n"
        "    state int line\n    state int flags\n    state int target\n    state bool hit\n"
        "    on created:\n"
        "            item.add_animation_descriptor(0)\n"
        "            item.clear_animation_commands()\n"
        "            item.clear_current_owner()\n"
        "            item.load_model(1, first)\n"
        "            item.load_model(2, second)\n"
        "            item.add_model_animation(1, first_anim)\n"
        "            item.request_model_animation(1, 0)\n"
        "            item.advance_model_animation(1)\n"
        "            item.remove_model_animation(2)\n"
        "            item.set_model_animation_rate(1, \"0\", 2)\n"
        "            item.set_model_animation_rate(1)\n"
        "            item.set_model_draw_enabled(1, false)\n"
        "    on stage_landed:\n"
        "            item.airborne = false\n"
        "    state 0 anim -1:\n"
        "        on animation:\n"
        "            item.state.angle = item.joint(1, \"\").quaternion.w\n"
        "            while item.state.angle > 3:\n"
        "                item.state.angle = item.state.angle - 3\n"
        "            item.joint(2, \"\").quaternion.w = item.state.angle\n"
        "            return item.creator_present\n"
        "        on physics:\n"
        "            item.state.angle = vectors.angle_xy(1, 2, 3, 4)\n"
        "            item.state.angle = vectors.angle(1, 2, 3, 4, 5, 6)\n"
        "            item.set_joint_transform(1, \"\", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, true)\n"
        "            item.sweep_segment(1, 2, 3, item.state.nx, item.state.ny, item.state.nz, item.state.line, item.state.flags, item.state.hit)\n"
        "            if item.state.hit and stage.line_exists(item.state.line):\n"
        "                item.bind_stage_line(item.state.line)\n"
        "                item.read_stage_line_speed(item.state.line)\n"
        "                item.state.hit = item.read_stage_line_speed(item.state.line + 1)\n"
        "                stage.line_normal(item.state.line, item.state.nx, item.state.ny, item.state.nz)\n"
        "            item.state.target = int(item.contact_fighter)\n"
        "            if targets.is_fighter(item.state.target) and targets.guard_active(item.state.target):\n"
        "                targets.shield(item.state.target, item.state.nx, item.state.ny, item.state.nz, item.state.radius)\n"
        "                item.state.radius = targets.model_scale(item.state.target) * item.state.radius\n"
        "            item.read_joint_matrix_rotation(0, \"0\", item.state.nx, item.state.ny, item.state.nz)\n"
        "            item.read_attachment_matrix_rotation(item.state.nx, item.state.ny, item.state.nz)\n"
        "            item.state.hit = creator.hitlag\n"
        "            item.mirror_velocity(item.state.nx, item.state.ny, item.state.nz)\n"
        "            item.state.target = int(item.current_owner) + int(item.reflector)\n"
        "            targets.read_camera_target_position(item.state.target, item.state.nx, item.state.ny, item.state.nz, item.state.hit)\n"
        "            item.state.angle = item.half_life\n"
        "            item.state.hit = item.reflection_owner_transfer_disabled\n"
        "            item.set_animation_frame(item.animation_frame)\n"
        "            hitbox.set_damage(0, 7)\n"
        "            item.spawn_effect(1057, 0, 0, 0, 0, 0, 0, 0, true, 0)\n"
        "            item.spawn_joint_effect_at(1096, \"\", 0, 0, 5)\n"
        "            item.suppress_destruction_effects = true\n";
    const char* item_source =
        "article It_Kind_Ness_PKFire states 1:\n"
        "    state int child\n"
        "    on stage_landed:\n"
        "            item.airborne = false\n"
        "    state 0 anim -1:\n"
        "        on physics:\n"
        "            item.state.child = item.create_at_position(It_Kind_Ness_PKFire, position.x, position.y + 1, 0, facing)\n"
        "            let int parent = item.link_id(2)\n"
        "            let int child = item.create_child(It_Kind_Ness_PKFire, position.x, position.y, 0, facing, item.current_owner, uint(0xDEADBEEF), 5, 2)\n"
        "            item.state.child = child + parent\n"
        "            if item.hits_stage():\n"
        "                item.land_on_stage()\n"
        "            item.airborne = item.accumulated_damage > 1\n"
        "            item.set_scale(item.base_scale)\n";
    char generated[65536];
    NativeFighterScriptError error = { 0 };
    if (!generate(source, generated, sizeof(generated), &error)) {
        fprintf(stderr, "Model parser fixture failed at%u: %s\n", error.line, error.message);
        return 1;
    }
    char expected[100];
    snprintf(expected, sizeof(expected), "UINT32_C(%u), 36", nativeScriptSignal_Hash("first"));
    assert(strstr(generated, expected));
    snprintf(expected, sizeof(expected), "UINT32_C(%u), 40", nativeScriptSignal_Hash("second"));
    assert(strstr(generated, expected));
    assert(strstr(generated, "host->item_load_model == 0"));
    assert(strstr(generated, "engine->vector_angle_xy(&(Vec3){ 1, 2, 0 }, &(Vec3){ 3, 4, 0 })"));
    assert(strstr(generated, "engine->vector_angle(&(Vec3){ 1, 2, 3 }, &(Vec3){ 4, 5, 6 })"));
    assert(strstr(generated, "host->item_set_joint_transform == 0"));
    assert(strstr(generated, "engine->item_joint_value(gobj, 1, \"\", 14)"));
    assert(strstr(generated, "engine->item_set_joint_value(gobj, 2, \"\", 14,"));
    assert(strstr(generated, "native_model_draw_disabled_mask"));
    assert(strstr(generated, "while (scripted_article_state_float"));
    assert(strstr(generated, "&item->pos, &item->x40_vel))"));
    static const char* guarded[] = { "item_segment_sweep", "stage_line_exists",
        "set_collision_line", "stage_line_speed", "stage_line_normal",
        "item_contact_fighter", "target_is_fighter", "target_guard_active",
        "item_current_owner", "item_reflector", "target_camera_target_position",
        "item_add_model_animation", "item_request_model_animation", "item_advance_model_animation",
        "item_remove_model_animation", "item_set_model_animation_rate", "item_read_joint_matrix_rotation",
        "item_add_animation_descriptor", "item_read_attachment_matrix_rotation", "item_creator", "vector_mirror",
        "target_model_scale", "target_shield", "item_set_animation_frame",
        "item_set_hitbox_damage", "item_spawn_effect", "item_spawn_joint_effect_at" };
    for (unsigned i = 0; i < sizeof(guarded) / sizeof(guarded[0]); ++i) {
        snprintf(expected, sizeof(expected), "host->%s == 0", guarded[i]);
        assert(strstr(generated, expected));
    }
    assert(strstr(generated, ".origin_at_center = true"));
    assert(strstr(generated, ".previous_at_bone = true"));
    assert(strstr(generated, "item->x524_cmd.u = NULL"));
    assert(strstr(generated, "scripted_article_creator(item)->x2219_b5"));
    const char* lifecycle =
        "func cleanup():\n    pass\n"
        "action CommonInputs:\n"
        "    on state_341_interrupt:\n"
        "        callbacks.death1(cleanup)\n"
        "        callbacks.death2(cleanup)\n"
        "        callbacks.death3(cleanup)\n"
        "        if abs(input.x) >= controls.horizontal_smash_threshold and input.horizontal_smash_age < controls.horizontal_smash_window + controls.horizontal_smash_extension:\n"
        "            if controls.try_guard(): return\n"
        "            if controls.try_dash(): return\n"
        "            if controls.try_air_special(): return\n"
        "        state.basic_landing()\n";
    assert(generate(lifecycle, generated, sizeof(generated), &error));
    assert(strstr(generated, "fighter->death1_cb = script_fn_cleanup"));
    assert(strstr(generated, "fighter->death2_cb = script_fn_cleanup"));
    assert(strstr(generated, "fighter->death3_cb = script_fn_cleanup"));
    assert(strstr(generated, "host->check_guard == 0"));
    assert(strstr(generated, "host->check_dash == 0"));
    assert(strstr(generated, "host->check_air_special == 0"));
    assert(strstr(generated, "host->enter_landing_basic == 0"));
    assert(strstr(generated, "NativeFighterCommon_HorizontalSmashThreshold"));
    assert(strstr(generated, "NativeFighterCommon_HorizontalSmashWindow"));
    assert(strstr(generated, "NativeFighterCommon_HorizontalSmashExtension"));
    const char* invalid[] = {
        "    attachment origin center previous missing plane off initial_collision on held on\n",
        "    state float x\n    state float y\n    state float z\n    state 0 anim -1:\n        on physics:\n            item.read_attachment_matrix_rotation(item.state.x, item.state.x, item.state.z)\n",
        "    state float x\n    state float y\n    state int z\n    state 0 anim -1:\n        on physics:\n            item.read_attachment_matrix_rotation(item.state.x, item.state.y, item.state.z)\n",
        "    model_root duplicate 36\n    model_root duplicate 40\n",
        "    model_root bad 37\n",
        "    model_animation duplicate 76\n    model_animation duplicate 88\n",
        "    model_animation bad 77\n",
        "    model_animation bad 4294967296\n",
        "    model_animation first 76\n    state 0 anim -1:\n        on physics:\n            item.add_model_animation(1, absent)\n",
        "    model_animation first 76\n    state 0 anim -1:\n        on physics:\n            item.add_model_animation(4, first)\n",
        "    state 0 anim -1:\n        on physics:\n            item.set_model_animation_rate(0, \"0/\", 1)\n",

        "    model_root bad 1024\n",
        "    model_root bad-name 36\n",
        "    model_root bad 36 trailing\n",
        "    state 0 anim -1:\n    model_root late 36\n",
        "    model_root first 36\n    state 0 anim -1:\n        on physics:\n            item.load_model(1, absent)\n",
        "    model_root first 36\n    state 0 anim -1:\n        on physics:\n            item.load_model(0, first)\n",
        "    state 0 anim -1:\n        on physics:\n            item.set_joint_transform(1, \"0/\", 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, true)\n",
        "    state 0 anim -1:\n        on physics:\n            while true:\n                item.life = 0\n            else:\n                item.life = 1\n",
        "    state float n\n    state int line\n    state int flags\n    state bool hit\n    state 0 anim -1:\n        on physics:\n            item.sweep_segment(0, 0, 0, item.state.n, item.state.n, item.state.n, item.state.line, item.state.flags, item.state.hit)\n",
        "    state float x\n    state float y\n    state float z\n    state bool hit\n    state 0 anim -1:\n        on physics:\n            stage.line_normal(0, item.state.x, item.state.y, item.state.hit)\n",
        "    state 0 anim -1:\n        on physics:\n            hitbox.set_damage(4, 2)\n",
        "    state bool hit\n    state 0 anim -1:\n        on physics:\n            item.state.hit = item.read_stage_line_speed(1, 2)\n",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        char script[4096];
        snprintf(script, sizeof(script), "article It_Kind_Mario_Fire states 1:\n%s", invalid[i]);
        assert(!generate(script, generated, sizeof(generated), &error));
    }
    char capacity_script[2048] = "article It_Kind_Mario_Fire states 1:\n";
    for (unsigned i = 0; i < 9; ++i) {
        size_t used = strlen(capacity_script);
        snprintf(capacity_script + used, sizeof(capacity_script) - used,
                 "    model_root root%u %u\n", i, i * 4);
    }
    assert(!generate(capacity_script, generated, sizeof(generated), &error));
    if (!generate(item_source, generated, sizeof(generated), &error)) {
        fprintf(stderr, "Item parser fixture failed at %u: %s\n", error.line, error.message);
        return 1;
    }
    assert(strstr(generated, "engine->create_item_at_position(gobj, (ItemKind)(It_Kind_Ness_PKFire)"));
    assert(strstr(generated, "engine->item_hits_stage(gobj)"));
    assert(strstr(generated, "engine->item_land_on_stage(gobj, scripted_article_0_logic_18)"));
    assert(strstr(generated, "item->ground_or_air = (item->xC9C > 1) ? GA_Air : GA_Ground"));
    assert(strstr(generated, "item->xCC_item_attr->x60_scale"));
    assert(!strstr(generated, "engine->item_set_airborne(item)"));
    assert(strstr(generated, "host->create_item_at_position == 0"));
    assert(strstr(generated, "engine->item_link_id(gobj, (unsigned)(2))"));
    assert(strstr(generated, "engine->item_create_child(gobj, (ItemKind)(It_Kind_Ness_PKFire)"));
    assert(strstr(generated, "3735928559u"));
    assert(strstr(generated, "host->item_link_id == 0"));
    assert(strstr(generated, "host->item_create_child == 0"));
    assert(strstr(generated, "host->item_hits_stage == 0"));
    assert(strstr(generated, "host->item_land_on_stage == 0"));
    const char* invalid_item[] = {
        "item.land_on_stage()",
        "item.state.child = item.create_at_position(It_Kind_Ness_PKFire, 0, 0, 0)",
        "item.state.child = item.create_at_position(It_Kind_Ness_PKFire, 0, 0, 0, 1, 2)",
        "item.state.child = item.link_id(1.5)",
        "item.state.child = item.link_id()",
        "item.state.child = item.create_child(It_Kind_Ness_PKFire, 0, 0, 0, 1, 0.5, 0, 0, 0)",
        "item.state.child = item.create_child(It_Kind_Ness_PKFire, 0, 0, 0, 1, 0, 0.5, 0, 0)",
        "item.state.child = item.create_child(It_Kind_Ness_PKFire, 0, 0, 0, 1, 0, 0, 0, 0.5)",
        "item.state.child = item.create_child(It_Kind_Ness_PKFire, 0, 0, 0, 1, 0, 0, 0)",
        "item.state.child = item.hits_stage(1)",
        "item.base_scale = 1",
        "item.accumulated_damage = 1",
        "let float angle = vectors.angle_xy(1, 2, 3, 4, 5, 6)",
    };
    for (unsigned i = 0; i < sizeof(invalid_item) / sizeof(invalid_item[0]); ++i) {
        char script[1024];
        snprintf(script, sizeof(script), "article It_Kind_Ness_PKFire states 1:\n    state int child\n    state 0 anim -1:\n        on physics:\n            %s\n", invalid_item[i]);
        assert(!generate(script, generated, sizeof(generated), &error));
    }
    puts("Article named-root metadata, quaternion/full-transform parser and guards passed");
    return 0;
}
