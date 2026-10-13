#include "native_fighter_script_projectile.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

bool nativeScriptProjectile_CreateExpression(const ScriptCodeBlock* block, const char* arguments, bool velocity,
                                             char* output, size_t capacity, const char** after)
{
    char args[12][192], values[12][256];
    unsigned count = velocity ? 12 : 8;
    if (!nativeScriptCode_ParseArguments(arguments, args, count, after)) {
        if (velocity) return false;
        count = 6;
        if (!nativeScriptCode_ParseArguments(arguments, args, count, after)) return false;
    }
    if (count == 12 && nativeScriptCode_ExpressionType(block, args[11]) != ScriptType_Bool)
        return false;
    for (unsigned i = 0; i < count; ++i)
        if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
    unsigned facing = velocity ? 7 : 4;
    int written = snprintf(output, capacity,
        "engine->create_article_at(gobj, (ItemKind)(%s), &(Vec3){ %s, %s, %s }, &(Vec3){ %s, %s, %s }, %s, %s, (uint32_t)(%s), %s, %s)",
        values[0], values[1], values[2], values[3],
        velocity ? values[4] : "0", velocity ? values[5] : "0", velocity ? values[6] : "0",
        values[facing], values[facing + 1],
        count != 6 ? values[facing + 2] : "0", count != 6 ? values[facing + 3] : "0",
        count == 12 ? values[11] : "true");
    return written >= 0 && (size_t)written < capacity;
}

bool nativeScriptProjectile_Emit(const ScriptCodeBlock* block, const char* source,
                                 char* output, size_t capacity)
{
    static const char create[] = "articles.create_at_position(";
    static const char create_velocity[] = "articles.create_with_velocity(";
    bool velocity = strncmp(source, create_velocity, sizeof(create_velocity) - 1) == 0;
    if (velocity || strncmp(source, create, sizeof(create) - 1) == 0) {
        const char* after;
        if (!nativeScriptProjectile_CreateExpression(block, strchr(source, '('), velocity,
                                                      output, capacity, &after) || *after) return false;
        size_t used = strlen(output);
        if (used + 2 > capacity) return false;
        output[used] = ';'; output[used + 1] = 0;
        return true;
    }
    static const char world[] = "articles.launch_at_position(";
    if (strncmp(source, world, sizeof(world) - 1) == 0) {
        char args[10][192], values[10][256]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(world) - 2, args, 10, &after) || *after)
            return false;
        for (unsigned i = 0; i < 10; ++i)
            if (!nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        return snprintf(output, capacity,
            "{ Vec3 pos = { %s, %s, %s }; NativeFighterProjectileLaunch launch = { %s, %s, (s32)(%s), (u32)(%s), true }; "
            "engine->spawn_authored_projectile(gobj, &pos, (ItemKind)(%s), (s32)(%s), %s, &launch); }",
            values[1], values[2], values[3], values[4], values[5], values[6], values[8], values[0], values[7], values[9]) < capacity;
    }
    static const char launch[] = "articles.launch_projectile(";
    if (strncmp(source, launch, sizeof(launch) - 1) == 0) {
        const char* cursor = source + sizeof(launch) - 1;
        char arguments[8][192];
        for (unsigned i = 0; i < 8; ++i) {
            const char* start = cursor;
            unsigned nested = 0;
            while (*cursor) {
                if (*cursor == '(') ++nested;
                else if (*cursor == ')' && nested) --nested;
                else if (!nested && (*cursor == ',' || *cursor == ')')) break;
                ++cursor;
            }
            if (*cursor != (i == 7 ? ')' : ',') || cursor == start ||
                cursor - start >= sizeof(arguments[i])) return false;
            size_t length = (size_t) (cursor - start);
            memcpy(arguments[i], start, length);
            arguments[i][length] = '\0';
            ++cursor;
            if (i < 7) while (*cursor == ' ') ++cursor;
        }
        if (*cursor) return false;
        const char* bone = nativeScriptCode_BoneSymbol(arguments[1], strlen(arguments[1]));
        if (bone == NULL) return false;
        char values[8][384];
        for (unsigned i = 0; i < 8; ++i) {
            if (i == 1) continue;
            if (!nativeScriptCode_Expression(block, arguments[i], values[i], sizeof(values[i])))
                return false;
        }
        int length = snprintf(output, capacity,
            "{ Fighter_Part part = engine->resolve_bone(fighter, %s); "
            "Vec3 offset = { %s, %s, %s }, pos; "
            "NativeFighterProjectileLaunch launch = { %s, %s, (s32)(%s) }; "
            "engine->joint_position(fighter->parts[part].joint, &offset, &pos); "
            "engine->spawn_authored_projectile(gobj, &pos, (ItemKind)(%s), 0, fighter->facing_dir, &launch); }",
            bone, values[2], values[3], values[4], values[5], values[6], values[7], values[0]);
        return length >= 0 && (size_t) length < capacity;
    }
    static const char projectile[] = "articles.spawn_projectile(";
    static const char authored_projectile[] =
        "articles.spawn_authored_projectile(";
    size_t projectile_size = strlen(source);
    bool authored_spawn = strncmp(source, authored_projectile,
                                  sizeof(authored_projectile) - 1) == 0;
    size_t projectile_prefix = authored_spawn
        ? sizeof(authored_projectile) - 1 : sizeof(projectile) - 1;
    if (authored_spawn ||
        strncmp(source, projectile, projectile_prefix) == 0) {
        if (projectile_size <= projectile_prefix + 1 ||
            source[projectile_size - 1] != ')') return false;
        const char* kind_end = strchr(source + projectile_prefix, ',');
        if (kind_end == NULL || kind_end[1] != ' ') return false;
        const char* bone_start = kind_end + 2;
        const char* bone_end = strchr(bone_start, ',');
        if (bone_end == NULL || bone_end[1] != ' ') return false;
        const char* variant_start = bone_end + 2;
        char kind[96], variant[192], translated[512];
        size_t kind_size = (size_t) (kind_end - (source + projectile_prefix));
        size_t variant_size = (size_t) (source + projectile_size - 1 -
                                         variant_start);
        if (kind_size < sizeof("It_Kind_A") - 1 ||
            kind_size >= sizeof(kind) || variant_size == 0 ||
            variant_size >= sizeof(variant) ||
            strncmp(source + projectile_prefix, "It_Kind_", 8) != 0)
            return false;
        memcpy(kind, source + projectile_prefix, kind_size);
        kind[kind_size] = '\0';
        for (size_t i = 0; i < kind_size; ++i)
            if (!isalnum((unsigned char) kind[i]) && kind[i] != '_')
                return false;
        const char* bone = nativeScriptCode_BoneSymbol(bone_start,
                                       (size_t) (bone_end - bone_start));
        if (bone == NULL) return false;
        memcpy(variant, variant_start, variant_size);
        variant[variant_size] = '\0';
        if (!nativeScriptCode_Expression(block, variant, translated,
                                  sizeof(translated))) return false;
        return snprintf(output, capacity,
            "{ Fighter_Part part = engine->resolve_bone(fighter, %s); "
            "Vec3 pos; engine->joint_position(fighter->parts[part].joint, "
            "0, &pos); engine->%s(gobj, &pos, %s, "
            "(s32) (%s), fighter->facing_dir%s); }",
            bone, authored_spawn ? "spawn_authored_projectile" :
                                   "spawn_projectile", kind, translated, authored_spawn ? ", NULL" : "")
            < capacity;
    }
    return false;
}
