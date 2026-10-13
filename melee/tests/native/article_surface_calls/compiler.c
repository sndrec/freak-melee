#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned programs;
static const char article[] =
    "article It_Kind_Link_HShot states 1:\n"
    "    state int walls\n    state int ceiling\n    state bool reflected\n"
    "    state float restitution\n    state bool stopped\n    state int effect\n"
    "    state 0 anim -1:\n        on physics:\n";
static const char valid[] =
    "            item.state.walls = item.wall_contact_flags()\n"
    "            item.state.ceiling = item.ceiling_contact_flags()\n"
    "            item.state.reflected = item.reflect_from_wall()\n"
    "            item.align_to_floor()\n"
    "            item.stop_on_surface = true\n"
    "            item.state.stopped = item.stop_on_surface\n"
    "            item.state.restitution = item.surface_restitution\n"
    "            item.spawn_async_joint_effect(item.state.effect, \"0/1\", 0)\n"
    "            item.spawn_async_joint_effect(item.state.effect + 1, \"\", 3)\n";

static void generate(const char* body, unsigned route, bool expected, const char* path)
{
    char source[8192];
    int length = snprintf(source, sizeof(source), "%s%s%s", route ?
        "move Probe:\n    on state_341_physics:\n        pass\n" :
        "action Probe:\n    on load:\n        pass\n", article, body);
    assert(length > 0 && (size_t)length < sizeof(source));
    FILE* out = path ? fopen(path, "w") : tmpfile(); assert(out);
    NativeFighterScriptError error = {0}; ++programs;
    bool ok = nativeFighterScript_GenerateC(source, (size_t)length,
        "article_surfaces", NULL, NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL, out, &error);
    if (ok != expected) fprintf(stderr, "Unexpected %s at %u: %s\n%s\n",
        ok ? "accept" : "reject", error.line, error.message, source);
    assert(ok == expected); if (!ok) assert(error.line);
    fclose(out);
}

int main(int argc, char** argv)
{
    assert(argc == 1 || argc == 3);
    static const char* bad[] = {
        "item.wall_contact_flags(1)", "item.ceiling_contact_flags(false)",
        "item.reflect_from_wall(1)", "item.align_to_floor(1)",
        "item.surface_restitution = 2.0", "item.wall_contact_flags = 1",
        "item.spawn_async_joint_effect(1.5, \"0\", 0)",
        "item.spawn_async_joint_effect(true, \"0\", 0)",
        "item.spawn_async_joint_effect(1, \"256\", 0)",
        "item.spawn_async_joint_effect(1, \"0//1\", 0)",
        "item.spawn_async_joint_effect(1, \"0\", 1)",
        "item.spawn_async_joint_effect(1, \"0\", 4)",
        "item.spawn_async_joint_effect(1, \"0\", -3)",
        "item.spawn_async_joint_effect(1, \"0\", 3.0)",
        "item.spawn_async_joint_effect(1, \"0\", item.state.effect)",
        "item.spawn_async_joint_effect(1, \"0\")",
        "item.spawn_async_joint_effect(1, \"0\", 0, 3)",
        "let int wrong = item.surface_restitution",
    };
    for (unsigned route = 0; route < 2; ++route) {
        generate(valid, route, true, argc == 3 ? argv[route + 1] : NULL);
        for (unsigned i = 0; i < sizeof(bad) / sizeof(*bad); ++i) {
            char body[1024]; snprintf(body, sizeof(body), "            %s\n", bad[i]);
            generate(body, route, false, NULL);
        }
        generate("            item.wall_contact_flags()\n"
                 "            item.ceiling_contact_flags()\n"
                 "            item.reflect_from_wall()\n"
                 "            let int bool_as_int = item.reflect_from_wall()\n"
                 "            let bool int_as_bool = item.wall_contact_flags()\n", route, true, NULL);
    }
    puts("Article surfaces/effects compiler: both routes, typed operands and bounded joint paths PASS");
    printf("%u compiler programs\n", programs);
    return 0;
}
