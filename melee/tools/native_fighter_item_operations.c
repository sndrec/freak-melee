#include "native_fighter_item_operations.h"
#include "native_fighter_item_program.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

const char* nativeItemOperations_Property(const char* token, bool writable)
{
    static const struct { const char* name; const char* value; bool writable; } fields[] = {
        { "item.damage_received", "item->xCA0", false },
        { "item.reverse_spin", "item->xDC8_word.flags.x19", true },
        { "item.damage_behavior", "item->xC98", true },
        { "item.dynamic_ecb", "item->xDCE_flag.b7", true },
        { "item.damage_direction", "item->xCCC_incDamageDirection", false },
        { "item.scale", "item->scl", false },
        { "item.throw_scale", "item->throw_scale", false },
        { "item.harm_owner", "item->xDCD_flag.b5", true },
        { "item.harm_teammates", "item->xDCD_flag.b6", true },
        { "item.harm_sibling_items", "item->xDCD_flag.b7", true },
        { "item.can_be_hit_by_owner", "item->xDCE_flag.b0", true },
        { "item.can_be_hit_by_teammates", "item->xDCE_flag.b1", true },
        { "item.can_be_hit_by_sibling_items", "item->xDCE_flag.b2", true },
        { "item.subaction_sounds_muted", "item->xDCD_flag.b2", true },
        { "item.stop_on_surface", "item->xDCD_flag.b4", true },
        { "item.surface_restitution", "item->xCC_item_attr->x58", false },
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        if ((!writable || fields[i].writable) && strcmp(token, fields[i].name) == 0)
            return fields[i].value;
    return NULL;
}

bool nativeItemOperations_DynamicJoint(const char* source, char index[192],
                                        unsigned* channel, const char** after)
{
    if (strncmp(source, "item.dynamic_joint(", 19) != 0) return false;
    char arguments[1][192]; const char* cursor;
    if (!nativeScriptCode_ParseArguments(source + 18, arguments, 1, &cursor) ||
        *cursor++ != '.') return false;
    memcpy(index, arguments[0], 192);
    static const char* groups[] = { "translation.", "rotation.", "scale." };
    for (unsigned i = 0; i < 3; ++i) {
        size_t length = strlen(groups[i]);
        if (strncmp(cursor, groups[i], length) != 0) continue;
        cursor += length;
        if (*cursor < 'x' || *cursor > 'z') return false;
        *channel = i * 3 + (unsigned)(*cursor++ - 'x');
        if (isalnum((unsigned char)*cursor) || *cursor == '_' || *cursor == '.') return false;
        *after = cursor;
        return true;
    }
    return false;
}

const ScriptFunction* nativeItemOperations_FighterFunction(const char* source,
    const ScriptFunctionProgram* functions, char arguments[9][192])
{
    if (functions == NULL || (strncmp(source, "owner.call(", 11) != 0 && strncmp(source, "creator.call(", 13) != 0)) return NULL;
    for (unsigned i = 0; i < functions->count; ++i) {
        const ScriptFunction* function = &functions->functions[i];
        if (function->disabled || function->model_effect_only) continue;
        const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(source, '('), arguments,
                function->argument_count + 1, &after) || *after ||
            strcmp(arguments[0], function->name) != 0) continue;
        return function;
    }
    return NULL;
}

bool nativeItemOperations_Action(const char* source, const ArticleDeclaration* article,
                                 const NativeFighterParameters* parameters,
                                 char* output, size_t capacity)
{
    const ScriptStateTable* state = article ? &article->local_state : NULL;
    bool scale = strncmp(source, "item.set_instance_scale(", 24) == 0;
    bool response = strncmp(source, "item.respond_stage_mask(", 24) == 0;
    if (scale || response) {
        char args[1][192], value[512]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, 1, &after) || *after)
            return false;
        ScriptExpressionType type = nativeFighterItemScript_ExpressionType(args[0], article, parameters);
        if (scale ? (type != ScriptType_Float && type != ScriptType_Int) : type != ScriptType_Int)
            return false;
        if (!nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters))
            return false;
        int written = scale ? snprintf(output, capacity,
            "{ float instance_scale = %s; engine->item_set_instance_scale(gobj, gobj->hsd_obj, instance_scale); }", value) :
            snprintf(output, capacity,
            "{ int32_t contact_mask = %s; engine->item_respond_stage_mask(gobj, contact_mask); }", value);
        return written >= 0 && (size_t)written < capacity;
    }
    static const struct { const char* script; const char* code; } calls[] = {
        { "item.gravity()", "engine->item_gravity(gobj);" },
        { "item.reset_spin()", "engine->item_reset_spin(gobj);" },
        { "item.refresh_physics()", "engine->item_refresh_physics(gobj);" },
        { "item.advance_animation()", "engine->item_advance_animation(gobj);" },
        { "item.advance_model_animation()", "engine->model_advance_animation(gobj->hsd_obj);" },
        { "item.inherit_owner_debug_display()", "engine->item_set_debug_owner(gobj, item->owner);" },
        { "item.destroy_effects()", "engine->destroy_effects(gobj);" },
        { "item.spawn_food()", "engine->item_spawn_food(gobj);" },
        { "item.land()", "engine->item_land(gobj);" },
        { "item.bounce()", "engine->item_bounce(gobj);" },
        { "item.stage_collision()", "engine->item_stage_collision(gobj);" },
        { "item.begin_expiry()", "engine->item_begin_expiry(gobj);" },
        { "item.stop_motion()", "engine->item_stop_motion(gobj);" },
        { "item.reflect()", "engine->item_reflect(gobj);" },
        { "item.shield_bounce()", "engine->item_shield_bounce(gobj);" },
        { "item.align_to_floor()", "engine->item_align_to_floor(gobj);" },
        { "item.align_facing_to_velocity()", "engine->item_align_facing(gobj);" },
        { "item.clear_attachment_owner()", "item->xDD4_itemVar.native_article.attachment_owner = NULL; item->xDD4_itemVar.native_article.creator_spawn_id = 0;" },
        { "item.clear_current_owner()", "item->owner = NULL;" },
        { "item.clear_animation_commands()", "item->x524_cmd.u = NULL;" },
        { "item.reset_default_ecb()", "engine->item_reset_default_ecb(gobj, &item->xC0C);" },
    };
    for (unsigned i = 0; i < sizeof(calls) / sizeof(calls[0]); ++i)
        if (strcmp(source, calls[i].script) == 0)
            return snprintf(output, capacity, "%s", calls[i].code) < capacity;
    if (state == NULL) return false;
    bool fighter = strncmp(source, "targets.fighter_ahead(", 22) == 0;
    bool target_item = strncmp(source, "targets.item_ahead(", 19) == 0;
    bool rotate = strncmp(source, "vectors.rotate_z(", 17) == 0;
    if (!fighter && !target_item && !rotate) return false;
    char arguments[4][192]; const char* after;
    if (!nativeScriptCode_ParseArguments(strchr(source, '('), arguments, 4, &after) || *after)
        return false;
    const ScriptStateField* fields[4];
    for (unsigned i = 0; i < 4; ++i) {
        if (strncmp(arguments[i], "item.state.", 11) != 0) return false;
        fields[i] = nativeFighterScript_StateField(state, arguments[i] + 11);
        if (fields[i] == NULL || fields[i]->array || fields[i]->type != (i == 3 && !rotate ? ScriptState_Bool : ScriptState_Float))
            return false;
        for (unsigned j = 0; j < i; ++j)
            if (fields[i] == fields[j]) return false;
    }
    if (rotate)
        return snprintf(output, capacity,
            "{ Vec3 v = { scripted_article_state_float(item, %u), scripted_article_state_float(item, %u), scripted_article_state_float(item, %u) }; "
            "engine->vector_rotate(&v, 4, scripted_article_state_float(item, %u)); "
            "scripted_article_set_state_float(item, %u, v.x); scripted_article_set_state_float(item, %u, v.y); scripted_article_set_state_float(item, %u, v.z); }",
            fields[0]->offset, fields[1]->offset, fields[2]->offset, fields[3]->offset,
            fields[0]->offset, fields[1]->offset, fields[2]->offset) < capacity;
    char query[384];
    if (fighter)
        snprintf(query, sizeof(query), "bool found = engine->item_find_fighter(&item->pos, &v, item->owner, item->facing_dir) != NULL;");
    else
        snprintf(query, sizeof(query), "HSD_GObj* target = engine->item_find_target(&item->pos, item->facing_dir); bool found = target != NULL; if (found) engine->item_center_position(target, &v);");
    return snprintf(output, capacity,
        "{ Vec3 v = { 0 }; %s scripted_article_set_state_float(item, %u, v.x); scripted_article_set_state_float(item, %u, v.y); "
        "scripted_article_set_state_float(item, %u, v.z); scripted_article_set_state_bool(item, %u, found); }",
        query, fields[0]->offset, fields[1]->offset, fields[2]->offset, fields[3]->offset) < capacity;
}
