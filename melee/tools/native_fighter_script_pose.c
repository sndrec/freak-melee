#include "native_fighter_script_pose.h"
#include "native_fighter_script_functions.h"
#include "native_fighter_script_signal.h"
#include <ctype.h>
#include <string.h>

static bool path_valid(const char* path)
{
    unsigned depth = 0;
    if (!*path) return true; // The imported graph root is also a valid recipe.
    while (*path) {
        if (++depth > 12 || !isdigit((unsigned char)*path)) return false;
        unsigned ordinal = 0, digits = 0;
        do {
            ordinal = ordinal * 10 + (unsigned)(*path++ - '0');
            if (++digits > 3 || ordinal > 255) return false;
        } while (isdigit((unsigned char)*path));
        if (!*path) return true;
        if (*path++ != '/' || !*path) return false;
    }
    return false;
}

bool nativeScriptPose_Parse(char* source, size_t size, ScriptPoseProgram* program,
    NativeFighterScriptError* error)
{
    memset(program, 0, sizeof(*program));
    bool code_seen = false;
    unsigned line = 0;
    for (size_t start = 0; start < size;) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        size_t length = end - start;
        while (length && (source[start + length - 1] == '\r' ||
            source[start + length - 1] == ' ' || source[start + length - 1] == '\t')) --length;
        ++line;
        const char* text = source + start;
        if (length >= 11 && !memcmp(text, "pose_graph ", 11)) {
            char declaration[160], name[32], path[SCRIPT_POSE_PATH_BYTES];
            unsigned slot; int consumed = 0;
            if (code_seen || program->count == SCRIPT_POSE_GRAPHS || length >= sizeof(declaration) || memchr(text, 0, length))
                return nativeScriptCode_Fail(error, line, "Pose graphs must precede code blocks (limit 16).");
            memcpy(declaration, text, length); declaration[length] = 0;
            // Empty quoted root path is handled separately because scanf's scanset
            // requires at least one character.
            if (sscanf(declaration, "pose_graph %31s slot %u path \"%63[^\"]\"%n",
                name, &slot, path, &consumed) != 3 || !consumed) {
                path[0] = 0; consumed = 0;
                if (sscanf(declaration, "pose_graph %31s slot %u path \"\"%n", name, &slot, &consumed) != 2 || !consumed)
                    return nativeScriptCode_Fail(error, line, "Expected pose_graph Name slot N path \"child/path\".");
            }
            if (declaration[consumed] || slot >= 16 ||
                !nativeScriptCode_Identifier(name, strlen(name), sizeof(name)) || !path_valid(path))
                return nativeScriptCode_Fail(error, line, "Invalid pose graph name, slot (0..15) or bounded child path.");
            uint32_t hash = nativeScriptSignal_Hash(name);
            unsigned order = 0;
            while (order < program->count && program->graphs[program->order[order]].hash < hash) ++order;
            if (!hash || (order < program->count && program->graphs[program->order[order]].hash == hash))
                return nativeScriptCode_Fail(error, line, "Duplicate or colliding pose graph name.");
            ScriptPoseGraph* graph = &program->graphs[program->count];
            strcpy(graph->name, name); strcpy(graph->path, path); graph->hash = hash; graph->slot = (uint16_t)slot;
            memmove(program->order + order + 1, program->order + order, program->count - order);
            program->order[order] = (uint8_t)program->count++;
            memset(source + start, ' ', end - start);
        } else {
            code_seen |= (length >= 5 && (!memcmp(text, "move ", 5) || !memcmp(text, "func ", 5))) ||
                (length >= 7 && !memcmp(text, "action ", 7)) ||
                (length >= 8 && !memcmp(text, "article ", 8)) ||
                (length >= 12 && !memcmp(text, "export func ", 12)) ||
                (length >= 13 && !memcmp(text, "model_effect ", 13));
        }
        start = end < size ? end + 1 : end;
    }
    return true;
}

void nativeScriptPose_Emit(const ScriptPoseProgram* program, FILE* output)
{
    if (!program || !program->count) return;
    fputs("static const NativeFighterPoseGraph script_pose_graphs[] = {\n", output);
    for (unsigned i = 0; i < program->count; ++i) {
        const ScriptPoseGraph* graph = &program->graphs[program->order[i]];
        fprintf(output, "    { UINT32_C(%u), %u, \"%s\" },\n", graph->hash, graph->slot, graph->path);
    }
    fputs("};\n", output);
}

void nativeScriptPose_EmitDefinition(const ScriptPoseProgram* program, FILE* output)
{
    fprintf(output, "    generated.pose_graph_count = %u; generated.pose_graphs = %s;\n",
        program ? program->count : 0, program && program->count ? "script_pose_graphs" : "NULL");
    if (program)
        for (unsigned i = 0; i < program->count; ++i)
            fprintf(output, "    generated.asset_layout.joint_graph_mask |= %uu;\n", 1u << program->graphs[i].slot);
}

const char* nativeScriptPose_Constant(const char* name)
{
    if (!strcmp(name, "Pose.live")) return "0";
    if (!strcmp(name, "Pose.blend")) return "1";
    return NULL;
}

static int graph_index(const ScriptCodeBlock* block, const char* name)
{
    const ScriptPoseProgram* program = block->functions ? block->functions->poses : NULL;
    if (program)
        for (unsigned i = 0; i < program->count; ++i)
            if (!strcmp(program->graphs[program->order[i]].name, name)) return (int)i;
    return -1;
}

bool nativeScriptPose_Action(const ScriptCodeBlock* block, const char* source,
    char* output, size_t capacity)
{
    if (block->model_effect) return false;
    static const struct { const char* name; const char* operation; unsigned arguments; } operations[] = {
        { "pose.load_animation", "LoadAnimation", 2 },
        { "pose.request_frame", "RequestFrame", 2 },
        { "pose.evaluate", "Evaluate", 1 },
        { "pose.copy_costume", "CopyCostume", 2 },
        { "pose.copy_graph", "CopyGraph", 3 },
        { "pose.blend_graph", "BlendGraph", 5 },
        { "pose.blend_to_live", "BlendToLive", 3 },
        { "pose.copy_to_live", "CopyToLive", 1 },
    };
    unsigned op;
    for (op = 0; op < sizeof(operations) / sizeof(*operations); ++op) {
        size_t n = strlen(operations[op].name);
        if (!strncmp(source, operations[op].name, n) && source[n] == '(') break;
    }
    if (op == sizeof(operations) / sizeof(*operations)) return false;
    char args[5][192], values[5][512]; const char* after;
    if (!nativeScriptCode_ParseArguments(source + strlen(operations[op].name), args,
        operations[op].arguments, &after) || *after) return false;
    bool destination = op <= 5;
    const char* dest = destination ? nativeScriptPose_Constant(args[0]) : "0";
    if (!dest) return false;
    unsigned numeric = destination ? 1 : 0;
    int graph = -1;
    if (op == 4 || op == 5) {
        graph = graph_index(block, args[2]);
        if (graph < 0) return false;
    }
    for (unsigned i = numeric; i < operations[op].arguments; ++i) {
        if ((op == 4 || op == 5) && i == 2) continue;
        ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
        bool integral = (op == 0 && i == 1) || (op >= 3 && i == numeric);
        if ((integral ? type != ScriptType_Int :
             (type != ScriptType_Int && type != ScriptType_Float)) ||
            !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
    }
    size_t used = 0; int n;
#define APPEND(...) do { n = snprintf(output + used, capacity - used, __VA_ARGS__); \
    if (n < 0 || (size_t)n >= capacity - used) return false; used += (size_t)n; } while (0)
    APPEND("{ ");
    if (op == 0) {
        APPEND("int32_t pose_clip_id = %s; FigaTree* pose_clip = engine->pose_clip(gobj, pose_clip_id); ", values[1]);
        APPEND("if (pose_clip != NULL && engine->pose_validate(gobj, NativeFighterPose_LoadAnimation, %s, 0, NULL, pose_clip)) engine->load_pose_animation(fighter, %s, pose_clip); }", dest, dest);
    } else if (op == 1) {
        APPEND("float pose_frame = %s; if (isfinite(pose_frame) && engine->pose_validate(gobj, NativeFighterPose_RequestFrame, %s, 0, NULL, NULL)) engine->request_pose_frame(%s, pose_frame); }",
            values[1], dest, !strcmp(dest,"0") ? "(HSD_JObj*)gobj->hsd_obj" : "fighter->x8AC_animSkeleton");
    } else if (op == 2) {
        APPEND("if (engine->pose_validate(gobj, NativeFighterPose_Evaluate, %s, 0, NULL, NULL)) engine->model_advance_animation(%s); }",
            dest, !strcmp(dest,"0") ? "(HSD_JObj*)gobj->hsd_obj" : "fighter->x8AC_animSkeleton");
    } else {
        APPEND("int32_t pose_start = %s; ", values[numeric]);
        if (op == 5 || op == 6) {
            unsigned first = op == 5 ? 3 : 1;
            APPEND("float pose_weight_a = %s; float pose_weight_b = %s; if (isfinite(pose_weight_a) && isfinite(pose_weight_b)) { ", values[first], values[first+1]);
        }
        if (op == 4 || op == 5) APPEND("HSD_Joint* pose_graph = engine->pose_graph(gobj, %d); ", graph);
        APPEND("if (engine->pose_validate(gobj, NativeFighterPose_%s, %s, pose_start, %s, NULL)) { ",
            operations[op].operation, dest, op == 4 || op == 5 ? "pose_graph" : "NULL");
        if (op == 3) {
            APPEND("HSD_Joint* pose_costume = engine->find_costume_pose(fighter, fighter->parts[pose_start].%s, fighter->x108_costume_joint); if (pose_costume != NULL) engine->%s(fighter, pose_start, pose_costume); ",
                !strcmp(dest,"0") ? "joint" : "x4_jobj2", !strcmp(dest,"0") ? "copy_bone_pose" : "copy_blend_pose");
        } else if (op == 4) APPEND("engine->%s(fighter, pose_start, pose_graph); ", !strcmp(dest,"0") ? "copy_bone_pose" : "copy_blend_pose");
        else if (op == 5) APPEND("engine->%s(fighter, pose_start, pose_weight_a, pose_weight_b, pose_graph); ", !strcmp(dest,"0") ? "blend_graph_live" : "blend_graph_alternate");
        else if (op == 6) APPEND("engine->blend_pose_to_live(fighter, pose_start, pose_weight_a, pose_weight_b); ");
        else APPEND("engine->copy_pose_to_live(fighter, pose_start); ");
        APPEND(op == 5 || op == 6 ? "} } }" : "} }");
    }
#undef APPEND
    return true;
}
