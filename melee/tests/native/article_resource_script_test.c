#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const char* positive =
    "state float smoothing\n"
    "article It_Kind_Link_HShot states 1:\n"
    "    resources capacity 90 reserve_part 139\n"
    "    model_root Segment 0\n"
    "    model_animation Extend 4\n"
    "    state int index\n    state int line\n    state bool succeeded\n"
    "    state float matrix[12]\n    state float x\n    state float y\n    state float z\n"
    "    state 0 anim -1:\n        on physics:\n"
    "            item.state.succeeded = item.resources.begin(2)\n"
    "            item.state.index = item.resources.count - 1\n"
    "            item.state.succeeded = item.resource(item.state.index).create(42, 1.5, 5)\n"
    "            item.resource(item.state.index).load_model(Segment)\n"
    "            item.resource(item.state.index).position.x = 2.5\n"
    "            item.resource(item.state.index).velocity.z = 1\n"
    "            item.resource(item.state.index).collision.previous.x = item.resource(item.state.index).collision.position.x\n"
    "            item.resource(item.state.index).surface_line = -1\n"
    "            item.state.line = item.resource(item.state.index).collision.environment\n"
    "            item.state.succeeded = item.resource(item.state.index).sweep_stage()\n"
    "            item.state.succeeded = item.resource(item.state.index).sweep_point()\n"
    "            item.resource(item.state.index).follow_floor()\n"
    "            item.resource(item.state.index).set_draw(true)\n"
    "            item.resource(item.state.index).bind_part(139)\n"
    "            item.resource(item.state.index).bind_capture_anchor()\n"
    "            item.resource(item.state.index).joint(\"0\").rotation.z = 1.5\n"
    "            item.state.succeeded = item.resource(item.state.index).joint(\"\").exists\n"
    "            item.resource(item.state.index).joint(\"0\").add_animation(Extend)\n"
    "            item.resource(item.state.index).joint(\"0\").request_animation(0)\n"
    "            item.resource(item.state.index).joint(\"0\").set_animation_rate(0.5)\n"
    "            item.resource(item.state.index).joint(\"0\").advance_animation()\n"
    "            item.resource(item.state.index).joint(\"0\").remove_animation()\n"
    "            item.resource(item.state.index).read_attachment_matrix(item.state.matrix)\n"
    "            item.resource(item.state.index).read_world_matrix(item.state.matrix)\n"
    "            item.resource(item.state.index).set_world_matrix(item.state.matrix)\n"
    "            item.resource(item.state.index).read_attachment_point(0, 0, 6, item.state.x, item.state.y, item.state.z)\n"
    "            item.resource(item.state.index).read_line_speed(item.state.line, item.state.x, item.state.y, item.state.z)\n"
    "            item.resources.release()\n"
    "action SharedControls:\n    on load:\n"
    "        state.smoothing = controls.stick_smoothing\n";

static bool generate(const char* source, FILE* output, bool diagnose)
{
    NativeFighterScriptError error = { 0 };
    bool valid = nativeFighterScript_GenerateC(source, strlen(source),
        "link_clone", NULL, NULL, NULL, output, &error);
    if (!valid && diagnose)
        fprintf(stderr, "resource script line %u: %s\n", error.line, error.message);
    return valid;
}

int main(int argc, char** argv)
{
    FILE* file = tmpfile(); assert(file);
    assert(generate(positive, file, true));
    assert(!fseek(file, 0, SEEK_END)); long size = ftell(file); assert(size > 0);
    assert(!fseek(file, 0, SEEK_SET)); char* code = malloc((size_t)size + 1); assert(code);
    assert(fread(code, 1, (size_t)size, file) == (size_t)size); code[size] = 0;
    assert(strstr(code, "host->article_resource_begin == 0"));
    assert(strstr(code, "host->article_resource_read_world_matrix == 0"));
    assert(strstr(code, "host->article_resource_sweep_point == 0"));
    assert(strstr(code, "engine->article_resource_sweep_point(gobj,"));
    assert(strstr(code, "NativeFighterCommon_StickSmoothing"));
    assert(strstr(code, "host->common_attribute == 0"));
    assert(strstr(code, "NativeArticleResource_EnvironmentFlags"));
    assert(strstr(code, ", 90, 139, NULL, NULL, NULL }"));
    if (argc == 2) {
        FILE* fixture = fopen(argv[1], "wb"); assert(fixture);
        assert(fwrite(code, 1, (size_t)size, fixture) == (size_t)size);
        fclose(fixture);
    } else assert(argc == 1);
    free(code); fclose(file);
    const char* invalid_lines[] = {
        "item.state.index = item.resources.begin(2.5)",
        "item.state.index = item.resource(1.5).surface_line",
        "item.resource(1).surface_line = 1.5",
        "item.resource(1).collision.environment = 0",
        "item.resource(1).collision.floor_normal.x = 0",
        "item.resource(1).joint(\"0\").exists = true",
        "item.resource(1).joint(\"256\").rotation.x = 0",
        "item.resource(1).load_model(Unknown)",
        "item.resource(1).set_world_matrix(item.state.matrix)",
        "item.resource(1).read_line_speed(1.5, item.state.x, item.state.y, item.state.z)",
    };
    for (unsigned i = 0; i < sizeof(invalid_lines) / sizeof(invalid_lines[0]); ++i) {
        char source[2048];
        snprintf(source, sizeof(source),
            "article It_Kind_Link_HShot states 1:\n    resources capacity 2\n"
            "    state int index\n    state float matrix[11]\n"
            "    state float x\n    state float y\n    state float z\n"
            "    state 0 anim -1:\n        on physics:\n            %s\n", invalid_lines[i]);
        file = tmpfile(); assert(file); assert(!generate(source, file, false)); fclose(file);
    }
    const char* invalid_declarations[] = {
        "resources capacity 1", "resources capacity 91", "resources capacity 2 reserve_part 140",
        "resources capacity 2\n    resources capacity 2", "resources capacity 2.5",
    };
    for (unsigned i = 0; i < sizeof(invalid_declarations) / sizeof(invalid_declarations[0]); ++i) {
        char source[512];
        snprintf(source, sizeof(source), "article It_Kind_Link_HShot states 1:\n    %s\n"
            "    state 0 anim -1:\n        on physics:\n            return\n", invalid_declarations[i]);
        file = tmpfile(); assert(file); assert(!generate(source, file, false)); fclose(file);
    }
    file = tmpfile(); assert(file);
    assert(!generate("article It_Kind_Link_HShot states 1:\n    state 0 anim -1:\n"
        "        on physics:\n            item.resources.begin(2)\n", file, false));
    fclose(file);
    const char* invalid_controls[] = {
        "action Bad:\n    on load:\n        controls.stick_smoothing = 0.5\n",
        "action Bad:\n    on load:\n        let float value = controls.stick_smoothing()\n",
        "action Bad:\n    on load:\n        let int value = controls.stick_smoothing\n",
    };
    for (unsigned i = 0; i < sizeof(invalid_controls) / sizeof(invalid_controls[0]); ++i) {
        file = tmpfile(); assert(file);
        assert(!generate(invalid_controls[i], file, false)); fclose(file);
    }
    puts("resource full compiler typed properties/metadata/probes contracts PASS");
    return 0;
}
