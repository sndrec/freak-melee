#include "native_fighter_article_resources.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "article_resource_compiler_arguments.h"

// Literal translation is the fixture boundary; actual argument splitting,
// expression typing and all resource declaration/emission bodies are linked.
static ScriptExpressionType literal_atom(const void* context, const char* atom)
{
    (void)context;
    return !strcmp(atom, "true") || !strcmp(atom, "false") ?
        ScriptType_Bool : ScriptType_Invalid;
}

ScriptExpressionType nativeFighterItemScript_ExpressionType(const char* value,
    const ArticleDeclaration* article, const NativeFighterParameters* params)
{
    (void)article; (void)params;
    return nativeScriptTypes_Expression(value, literal_atom, NULL);
}

bool nativeFighterItemScript_Expression(const char* value, char* output, size_t capacity,
    const ArticleDeclaration* article, const NativeFighterParameters* params)
{
    if (nativeFighterItemScript_ExpressionType(value, article, params) == ScriptType_Invalid)
        return false;
    int size = snprintf(output, capacity, "%s", value);
    return size >= 0 && (size_t)size < capacity;
}

const ScriptStateField* nativeFighterScript_StateField(const ScriptStateTable* table,
                                                       const char* name)
{
    for (unsigned i = 0; i < table->count; ++i)
        if (!strcmp(table->fields[i].name, name)) return &table->fields[i];
    return NULL;
}

static unsigned checks;
#define CHECK(condition) do { ++checks; assert(condition); } while (0)

int main(void)
{
    NativeArticleScriptResources resources = { 0 }, absent = { 0 };
    CHECK(nativeArticleResources_Declare("    resources capacity 90 reserve_part 139", &resources));
    CHECK(resources.capacity == 90 && resources.reserved_part == 139 && resources.declared);
    CHECK(!nativeArticleResources_Declare("    resources capacity 2", &resources));
    const char* bad_declarations[] = {
        "    resources capacity 1", "    resources capacity 91",
        "    resources capacity -2", "    resources capacity +2",
        "    resources capacity 2.0", "    resources capacity 2 reserve_part -1",
        "    resources capacity 2 reserve_part 140", "    resources capacity 2 reserve_part 2.0",
        "    resources capacity 2 reserve_part 0 trailing", "    resources capacity 2 nonsense 0",
        "    resources capacity 99999999999999999999999999999",
    };
    for (unsigned i = 0; i < sizeof(bad_declarations) / sizeof(bad_declarations[0]); ++i) {
        NativeArticleScriptResources bad = { 0 };
        CHECK(!nativeArticleResources_Declare(bad_declarations[i], &bad));
        CHECK(!bad.declared);
    }
    NativeArticleScriptResources no_reservation = { 0 };
    CHECK(nativeArticleResources_Declare("    resources capacity 2", &no_reservation));
    CHECK(no_reservation.reserved_part == -1);
    NativeArticleScriptNames roots = { .entries = { { "Segment", 123, 0 } }, .count = 1 };
    NativeArticleScriptNames animations = { .entries = { { "Extend", 456, 4 } }, .count = 1 };
    ScriptStateTable state = { .count = 7, .fields = {
        { .name = "matrix", .type = ScriptState_Float, .offset = 0, .count = 12, .array = true },
        { .name = "wrongSize", .type = ScriptState_Float, .offset = 48, .count = 11, .array = true },
        { .name = "wrongType", .type = ScriptState_Int, .offset = 96, .count = 12, .array = true },
        { .name = "x", .type = ScriptState_Float, .offset = 144 },
        { .name = "y", .type = ScriptState_Float, .offset = 148 },
        { .name = "z", .type = ScriptState_Float, .offset = 152 },
        { .name = "line", .type = ScriptState_Int, .offset = 156 },
    } };
    const struct { const char* source; ScriptExpressionType type; const char* host; } valid[] = {
        { "item.resources.begin(2)", ScriptType_Bool, "article_resource_begin" },
        { "item.resources.count", ScriptType_Int, "article_resource_count" },
        { "item.resource(1).exists", ScriptType_Bool, "article_resource_exists" },
        { "item.resource(1).create(42, 1.5, 0)", ScriptType_Bool, ".position = {0, 0, 0}" },
        { "item.resource(1).create(42, 1.5, 0, 2, -3, 4)", ScriptType_Bool, ".position = {2, (-3), 4}" },
        { "item.resource(1).create(42, 1.5, 0, item.position.x, item.position.y, item.position.z)", ScriptType_Bool, "NativeArticleResourceInit" },
        { "item.resource(1).load_model(Segment)", ScriptType_Bool, "UINT32_C(123)" },
        { "item.resource(1).position.x", ScriptType_Float, "NativeArticleResource_PositionX" },
        { "item.resource(1).collision.previous.z", ScriptType_Float, "NativeArticleResource_CollisionPreviousZ" },
        { "item.resource(1).collision.floor_normal.y", ScriptType_Float, "NativeArticleResource_FloorNormalY" },
        { "item.resource(1).surface_line", ScriptType_Int, "NativeArticleResource_SurfaceLine" },
        { "item.resource(1).collision.environment", ScriptType_Int, "NativeArticleResource_EnvironmentFlags" },
        { "item.resource(1).sweep_stage()", ScriptType_Bool, "article_resource_sweep_stage" },
        { "item.resource(1).sweep_point()", ScriptType_Bool, "article_resource_sweep_point" },
        { "item.resource(1).follow_floor()", ScriptType_Bool, "article_resource_follow_floor" },
        { "item.resource(1).bind_part(139)", ScriptType_Bool, "article_resource_bind_external_part" },
        { "item.resource(1).bind_capture_anchor()", ScriptType_Bool, "article_resource_set_capture_anchor" },
        { "item.resource(1).set_draw(true)", ScriptType_Bool, "article_resource_set_draw" },
        { "item.resource(1).joint(\"\").rotation.z", ScriptType_Float, "article_resource_read_joint" },
        { "item.resource(1).joint(\"0/255\").exists", ScriptType_Bool, "article_resource_read_joint" },
        { "item.resource(1).joint(\"0\").add_animation(Extend)", ScriptType_Bool, "UINT32_C(456)" },
        { "item.resource(1).joint(\"0\").request_animation(1.5)", ScriptType_Bool, "article_resource_request_animation" },
        { "item.resource(1).joint(\"0\").advance_animation()", ScriptType_Bool, "article_resource_advance_animation" },
        { "item.resource(1).joint(\"0\").remove_animation()", ScriptType_Bool, "article_resource_remove_animation" },
        { "item.resource(1).joint(\"0\").set_animation_rate(0.5)", ScriptType_Bool, "article_resource_set_animation_rate" },
    };
    char output[4096]; const char* after; ScriptExpressionType type;
    for (unsigned i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
        CHECK(nativeArticleResources_Expression(valid[i].source, NULL, NULL, &resources,
            &roots, &animations, output, sizeof(output), &after, &type) == 1);
        CHECK(!*after && type == valid[i].type && strstr(output, valid[i].host));
        CHECK(nativeArticleResources_Expression(valid[i].source, NULL, NULL, &absent,
            &roots, &animations, output, sizeof(output), &after, &type) == -1);
        CHECK(nativeArticleResources_Expression(valid[i].source, NULL, NULL, &resources,
            &roots, &animations, output, 4, &after, &type) == -1);
    }
    const char* invalid[] = {
        "item.resources.begin(2.5)", "item.resources.count.fake",
        "item.resource(0.5).exists", "item.resource(1).exists_missing",
        "item.resource(1).create(42.5, 1, 0)", "item.resource(1).create(42, 1, 0.5)",
        "item.resource(1).create(42, 1, 0, 2)", "item.resource(1).create(42, 1, 0, 2, 3)",
        "item.resource(1).create(42, 1, 0, 2, 3, 4, 5)",
        "item.resource(1).create(42, 1, 0, true, 3, 4)",
        "item.resource(1).create(42, 1, 0, 2, false, 4)",
        "item.resource(1).create(42, 1, 0, 2, 3, true)",
        "item.resource(1).load_model(Unknown)", "item.resource(1).load_model(Extend)",
        "item.resource(1).bind_part(139.5)", "item.resource(1).joint(\"256\").exists",
        "item.resource(1).joint(\"0/\").exists", "item.resource(1).joint(\"-1\").exists",
        "item.resource(1).joint(\"999999999999\").exists", "item.resource(1).joint(\"0\").add_animation(Segment)",
        "item.resource(1).joint(\"0\").position.x", "item.resource(1).joint(\"0\").sweep_stage()",
        "item.resource(1).rotation.x", "item.resource(1).collision.fake", "item.resources.begin(2, 3)",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        CHECK(nativeArticleResources_Expression(invalid[i], NULL, NULL, &resources,
            &roots, &animations, output, sizeof(output), &after, &type) == -1);
    CHECK(nativeArticleResources_Expression("item.other", NULL, NULL, &resources,
        &roots, &animations, output, sizeof(output), &after, &type) == 0);
    const struct { const char* target; const char* value; bool valid; } assignments[] = {
        { "item.resource(1).position.x", "1.5", true },
        { "item.resource(1).velocity.z", "2", true },
        { "item.resource(1).collision.previous.y", "2.5", true },
        { "item.resource(1).surface_line", "-1", true },
        { "item.resource(1).surface_line", "1.5", false },
        { "item.resource(1).collision.environment", "0", false },
        { "item.resource(1).collision.floor_line", "0", false },
        { "item.resource(1).collision.floor_normal.x", "0", false },
        { "item.resource(1).exists", "true", false },
        { "item.resource(1).joint(\"0\").rotation.z", "1.5", true },
        { "item.resource(1).joint(\"0\").visible", "false", true },
        { "item.resource(1).joint(\"0\").exists", "true", false },
    };
    for (unsigned i = 0; i < sizeof(assignments) / sizeof(assignments[0]); ++i)
        CHECK(nativeArticleResources_Assignment(assignments[i].target, assignments[i].value,
            NULL, NULL, &resources, output, sizeof(output)) == (assignments[i].valid ? 1 : -1));
    const struct { const char* line; bool valid; const char* contains; } actions[] = {
        { "item.resources.release()", true, "article_resource_release" },
        { "item.resource(1).spawn_effect_at_position(1011, 1, 2.5, -3)", true, "article_resource_spawn_effect_at_position" },
        { "item.resource(1).spawn_effect_facing_at_position(1009, 1, 2, 3, -1)", true, "article_resource_spawn_effect_facing_at_position" },
        { "item.resource(true).spawn_effect_at_position(1011, 1, 2, 3)", false, "" },
        { "item.resource(1).spawn_effect_at_position(1011.5, 1, 2, 3)", false, "" },
        { "item.resource(1).spawn_effect_at_position(true, 1, 2, 3)", false, "" },
        { "item.resource(1).spawn_effect_at_position(1011, true, 2, 3)", false, "" },
        { "item.resource(1).spawn_effect_at_position(1011, 1, 2)", false, "" },
        { "item.resource(1).spawn_effect_facing_at_position(1009, 1, 2, 3, false)", false, "" },
        { "item.resource(1).spawn_effect_facing_at_position(1009, 1, 2, 3)", false, "" },
        { "item.resource(1).spawn_effect_at_position(1011, 1, 2, 3) tail", false, "" },
        { "item.resource(1).read_attachment_matrix(item.state.matrix)", true, "state + 0, matrix" },
        { "item.resource(1).read_world_matrix(item.state.matrix)", true, "article_resource_read_world_matrix" },
        { "item.resource(1).set_world_matrix(item.state.matrix)", true, "state + 0, sizeof(matrix)" },
        { "item.resource(1).read_attachment_matrix(item.state.wrongSize)", false, "" },
        { "item.resource(1).set_world_matrix(item.state.wrongType)", false, "" },
        { "item.resource(1).read_world_matrix(item.state.matrix[0])", false, "" },
        { "item.resource(1).read_attachment_point(0, 1.5, 2, item.state.x, item.state.y, item.state.z)", true, "result.z" },
        { "item.resource(1).read_attachment_point(0, 1, 2, item.state.x, item.state.x, item.state.z)", false, "" },
        { "item.resource(1).read_line_speed(-1, item.state.x, item.state.y, item.state.z)", true, "(int)(-1)" },
        { "item.resource(1).read_line_speed(1.5, item.state.x, item.state.y, item.state.z)", false, "" },
        { "item.resource(1).read_line_speed(1, item.state.x, item.state.y, item.state.line)", false, "" },
        { "item.resource(1).sweep_stage() trailing", false, "" },
        { "item.resource(1).exists", true, "article_resource_exists" },
    };
    for (unsigned i = 0; i < sizeof(actions) / sizeof(actions[0]); ++i) {
        CHECK(nativeArticleResources_Action(actions[i].line, NULL, NULL, &resources,
            &roots, &animations, &state, output, sizeof(output)) == (actions[i].valid ? 1 : -1));
        if (actions[i].valid) CHECK(strstr(output, actions[i].contains));
    }
    printf("{\"checks\":%u,\"expression_cases\":%zu,\"rejected_expressions\":%zu,\"assignment_cases\":%zu,\"action_cases\":%zu}\n",
        checks, sizeof(valid)/sizeof(valid[0]), sizeof(invalid)/sizeof(invalid[0]),
        sizeof(assignments)/sizeof(assignments[0]), sizeof(actions)/sizeof(actions[0]));
    return 0;
}
