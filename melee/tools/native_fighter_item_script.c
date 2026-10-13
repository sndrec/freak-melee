#include "native_fighter_item_program.h"
#include "native_fighter_item_functions.h"
#include "native_fighter_item_tables.h"
#include "native_fighter_script_code.h"
#include "native_fighter_script_signal.h"
#include "native_fighter_script_vectors.h"
#include "native_fighter_item_presentation.h"
#include "native_fighter_script_lifetime.h"
#include "native_fighter_script_store.h"
#include "native_fighter_item_links.h"
#include "native_fighter_item_operations.h"
#include "native_fighter_article_operations.h"
#include "native_fighter_article_resources.h"
#include "native_fighter_item_terrain.h"
#include "native_fighter_script_spatial.h"
#include "native_fighter_script_colors.h"
#include "native_fighter_script_queries.h"
#include <melee/ft/native_fighter_runtime.h>

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static bool error_at(NativeFighterScriptError* error, unsigned line,
                     const char* message)
{
    error->line = line;
    snprintf(error->message, sizeof(error->message), "%s", message);
    return false;
}

static bool identifier(const char* name)
{
    if (!isalpha((unsigned char) *name) && *name != '_') return false;
    for (const char* p = name + 1; *p; ++p)
        if (!isalnum((unsigned char) *p) && *p != '_') return false;
    return true;
}

static bool parse_joint_property(const char* source, char* path,
                                 unsigned* model, unsigned* channel, const char** after)
{
    if (strncmp(source, "item.joint(", 11) != 0) return false;
    const char* cursor = source + 11;
    *model = 0;
    if (*cursor >= '0' && *cursor <= '3') {
        *model = (unsigned)(*cursor++ - '0');
        if (*cursor++ != ',') return false;
        while (*cursor == ' ') ++cursor;
    }
    if (*cursor++ != '"') return false;
    unsigned used = 0, depth = 0;
    while (*cursor != '"') {
        if (++depth > 12 || *cursor < '0' || *cursor > '9') return false;
        unsigned ordinal = 0;
        do {
            if (used >= 39) return false;
            ordinal = ordinal * 10 + (unsigned) (*cursor - '0');
            if (ordinal > 255) return false;
            path[used++] = *cursor++;
        } while (*cursor >= '0' && *cursor <= '9');
        if (*cursor == '"') break;
        if (*cursor != '/' || used >= 39) return false;
        path[used++] = *cursor++;
        if (*cursor < '0' || *cursor > '9') return false;
    }
    path[used] = '\0';
    if (strncmp(cursor, "\").", 3) != 0) return false;
    cursor += 3;
    static const char* properties[] = { "exists", "visible", "self_visible" };
    static const unsigned channels[] = { 9, 10, 15 };
    for (unsigned i = 0; i < 3; ++i) {
        size_t length = strlen(properties[i]);
        if (strncmp(cursor, properties[i], length) != 0) continue;
        char next = cursor[length];
        if (isalnum((unsigned char)next) || next == '_' || next == '.') return false;
        *channel = channels[i];
        *after = cursor + length;
        return true;
    }
    static const char* groups[] = { "translation.", "rotation.", "scale." };
    if (strncmp(cursor, "quaternion.", 11) == 0) {
        cursor += 11;
        const char* component = strchr("xyzw", *cursor);
        if (*cursor == '\0' || component == NULL) return false;
        *channel = 11 + (unsigned)(component - "xyzw");
        ++cursor;
        if (isalnum((unsigned char)*cursor) || *cursor == '_' || *cursor == '.') return false;
        *after = cursor;
        return true;
    }
    for (unsigned group = 0; group < 3; ++group) {
        size_t length = strlen(groups[group]);
        if (strncmp(cursor, groups[group], length) != 0) continue;
        cursor += length;
        if (*cursor < 'x' || *cursor > 'z') return false;
        *channel = group * 3 + (unsigned) (*cursor++ - 'x');
        if (isalnum((unsigned char) *cursor) || *cursor == '_' || *cursor == '.')
            return false;
        *after = cursor;
        return true;
    }
    return false;
}

static int article_parameter(const ArticleDeclaration*,
                              const NativeFighterParameters*, const char*);

typedef struct ArticleTypeContext {
    const ArticleDeclaration* article;
    const NativeFighterParameters* parameters;
} ArticleTypeContext;

static ScriptExpressionType article_atom_type(const void* opaque, const char* atom)
{
    ScriptExpressionType target_type = nativeScriptLifetime_TargetType(atom);
    if (target_type != ScriptType_Invalid) return target_type;
    const ArticleTypeContext* context = opaque;
    const ArticleDeclaration* article = context->article;
    {
        char emitted[1024]; const char* after;
        int query = nativeScriptQueries_Article(article, context->parameters, atom,
            &after, emitted, sizeof(emitted));
        if (query) return query > 0 && !*after ? ScriptType_Bool : ScriptType_Invalid;
    }
    if (article) {
        if (!strncmp(atom, "creator.call(", 13) || !strncmp(atom, "owner.call(", 11))
            return nativeItemFunctions_RecipientResult(article, atom);
        bool helper;
        ScriptExpressionType helper_type = nativeItemFunctions_Result(article, atom, &helper);
        if (helper) return helper_type;
        char emitted[4096]; const char* after; ScriptExpressionType resource_type, terrain_type;
        int held = nativeArticleOperations_HeldExpression(atom, article, context->parameters,
            emitted, sizeof(emitted), &after, &resource_type);
        if (held < 0) return ScriptType_Invalid;
        if (held > 0) return *after ? ScriptType_Invalid : resource_type;
        if (strncmp(atom, "event.", 6) == 0) {
            if (article->expression_callback != &article->callbacks[ARTICLE_HELD_REQUEST]) return ScriptType_Invalid;
            if (strcmp(atom, "event.action") == 0) return ScriptType_Int;
            if (strcmp(atom, "event.frame_rate") == 0 || strcmp(atom, "event.duration") == 0) return ScriptType_Float;
            return ScriptType_Invalid;
        }
        if (nativeItemTerrain_Constant(atom) != NULL) return ScriptType_Int;
        int terrain = nativeItemTerrain_Expression(atom, article,
            context->parameters, emitted, sizeof(emitted), &after, &terrain_type);
        if (terrain < 0) return ScriptType_Invalid;
        if (terrain > 0) return *after ? ScriptType_Invalid : terrain_type;
        int resource = nativeArticleResources_Expression(atom, article, context->parameters,
            &article->resources, &article->model_roots, &article->model_animations,
            emitted, sizeof(emitted), &after, &resource_type);
        if (resource < 0) return ScriptType_Invalid;
        if (resource > 0) return *after ? ScriptType_Invalid : resource_type;
        NativeScriptArticleReference ref;
        int found = nativeScriptArticleRefs_Reference(article->catalog, atom, &ref);
        if (found < 0) return ScriptType_Invalid;
        if (found > 0) {
            if (*ref.after ||
                nativeFighterItemScript_ExpressionType(ref.handle, article, context->parameters) != ScriptType_Int ||
                (ref.field && nativeFighterItemScript_ExpressionType(ref.index, article, context->parameters) != ScriptType_Int))
                return ScriptType_Invalid;
            return nativeScriptTypes_State(ref.type);
        }
        const char* prefixes[] = { "item.state.", "owner.state.", "creator.state." };
        for (unsigned i = 0; i < 3; ++i) {
            NativeScriptStateReference reference;
            int found = nativeScriptState_Reference(i == 0 ? &article->local_state :
                article->fighter_state, atom, prefixes[i], &reference);
            if (found < 0) return ScriptType_Invalid;
            if (found > 0 && *reference.after == 0)
                return nativeScriptTypes_State(reference.field->type);
        }
        const ItemCallback* callback = article->expression_callback;
        if (callback)
            for (unsigned i = 0; i < callback->local_count; ++i)
                if (strcmp(callback->locals[i], atom) == 0) return callback->local_types[i];
        if (strncmp(atom, "item.attr.", 10) == 0)
            for (unsigned i = 0; i < article->attribute_count; ++i)
                if (strcmp(article->attributes[i].name, atom + 10) == 0)
                    return article->attributes[i].integer ? ScriptType_Int : ScriptType_Float;
    }
    if (context->parameters) {
        int index = strncmp(atom, "item.params.", 12) == 0 && article
            ? article_parameter(article, context->parameters, atom + 12) : -1;
        const char* root_parameter = strncmp(atom, "program.attrs.", 14) == 0
            ? atom + 14 : strncmp(atom, "attrs.", 6) == 0 ? atom + 6 : NULL;
        if (root_parameter)
            for (unsigned i = 0; i < context->parameters->count; ++i)
                if (strcmp(context->parameters->fields[i].name, root_parameter) == 0) index = (int)i;
        if (index >= 0) {
            NativeFighterParameterType type = context->parameters->fields[index].type;
            return type == NativeFighterParameter_Float ? ScriptType_Float :
                   type == NativeFighterParameter_Bool ? ScriptType_Bool : ScriptType_Int;
        }
    }
    if (strncmp(atom, "program.attrs.", 14) == 0) return ScriptType_Invalid;
    return nativeScriptTypes_Builtin(atom);
}

ScriptExpressionType nativeFighterItemScript_ExpressionType(const char* source,
    const ArticleDeclaration* article, const NativeFighterParameters* parameters)
{
    ArticleTypeContext context = { article, parameters };
    return nativeScriptTypes_Expression(source, article_atom_type, &context);
}

bool nativeFighterItemScript_Expression(const char* source, char* output,
                            size_t capacity, const ArticleDeclaration* article,
                            const NativeFighterParameters* parameters)
{
    char args[2][192]; bool negate;
    int target = nativeScriptLifetime_TargetCall(source, args, &negate);
    if (target < 0) return false;
    if (target > 0) {
        char handle[1024];
        if (!article || !article->expression_callback ||
            nativeFighterItemScript_ExpressionType(args[0], article, parameters) != ScriptType_Int ||
            !nativeFighterItemScript_Expression(args[0], handle, sizeof(handle), article, parameters)) return false;
        args[1][strlen(args[1]) - 1] = 0;
        int written = snprintf(output, capacity, "%sengine->%s((uint32_t)(%s), UINT32_C(%u))",
            negate ? "!" : "", nativeScriptLifetime_TargetHost(source), handle,
            nativeScriptSignal_Hash(args[1] + 1));
        return written >= 0 && (size_t)written < capacity;
    }
    size_t used = 0;
    while (*source != '\0') {
        {
            char translated[1024]; const char* after;
            int query = nativeScriptQueries_Article(article, parameters, source,
                &after, translated, sizeof(translated));
            if (query < 0) return false;
            if (query > 0) {
                size_t length = strlen(translated);
                if (length >= capacity - used) return false;
                memcpy(output + used, translated, length); used += length;
                source = after; continue;
            }
        }
        if (article) {
            char translated[1024]; const char* after;
            int helper = nativeItemFunctions_RecipientCall(article, source, parameters, true,
                translated, sizeof(translated), &after);
            if (!helper) helper = nativeItemFunctions_Expression(article, source, parameters,
                translated, sizeof(translated), &after);
            if (helper < 0) return false;
            if (helper > 0) {
                size_t length = strlen(translated);
                if (length >= capacity - used) return false;
                memcpy(output + used, translated, length); used += length;
                source = after; continue;
            }
        }
        if (article != NULL) {
            NativeScriptArticleReference ref;
            int article_found = nativeScriptArticleRefs_Reference(article->catalog, source, &ref);
            if (article_found < 0) return false;
            if (article_found > 0) {
                char handle[1024], index[1024] = "0", translated[2500];
                if (nativeFighterItemScript_ExpressionType(ref.handle, article, parameters) != ScriptType_Int ||
                    !nativeFighterItemScript_Expression(ref.handle, handle, sizeof(handle), article, parameters) ||
                    (ref.field &&
                     (nativeFighterItemScript_ExpressionType(ref.index, article, parameters) != ScriptType_Int ||
                      !nativeFighterItemScript_Expression(ref.index, index, sizeof(index), article, parameters))) ||
                    !nativeScriptArticleRefs_Emit(&ref, handle, index, translated, sizeof(translated))) return false;
                size_t length = strlen(translated);
                if (length >= capacity - used) return false;
                memcpy(output + used, translated, length); used += length;
                source = ref.after; continue;
            }
            NativeScriptStateReference reference;
            bool owner = strncmp(source, "owner.state.", 12) == 0;
            bool creator = strncmp(source, "creator.state.", 14) == 0;
            int found = nativeScriptState_Reference((owner || creator) ? article->fighter_state :
                &article->local_state, source, creator ? "creator.state." : owner ? "owner.state." : "item.state.", &reference);
            if (found < 0) return false;
            if (found > 0 && reference.field->array) {
                char index[1024], access[1400];
                const char* bytes = creator ?
                    "(scripted_article_creator(item) != NULL ? scripted_article_creator(item)->native_module_state + script_state_base : NULL)" : owner ?
                    "(scripted_article_attached_owner(item) != NULL ? scripted_article_attached_owner(item)->native_module_state + script_state_base : NULL)" :
                    "item->xDD4_itemVar.native_article.state";
                if (!nativeFighterItemScript_Expression(reference.index, index, sizeof(index), article, parameters) ||
                    !nativeScriptState_ArrayRead(reference.field, bytes, index, access, sizeof(access))) return false;
                size_t length = strlen(access);
                if (length >= capacity - used) return false;
                memcpy(output + used, access, length);
                used += length; source = reference.after; continue;
            }
        }
        if (article) {
            char translated[1024]; const char* after;
            ScriptExpressionType resource_type;
            int resource = nativeArticleResources_Expression(source, article, parameters,
                &article->resources, &article->model_roots, &article->model_animations,
                translated, sizeof(translated), &after, &resource_type);
            if (resource < 0) return false;
            if (resource > 0) {
                size_t length = strlen(translated);
                if (length >= capacity - used) return false;
                memcpy(output + used, translated, length);
                used += length; source = after; continue;
            }
            ScriptExpressionType held_type;
            int held = nativeArticleOperations_HeldExpression(source, article, parameters,
                translated, sizeof(translated), &after, &held_type);
            if (held < 0) return false;
            if (held > 0) {
                size_t length = strlen(translated);
                if (length >= capacity - used) return false;
                memcpy(output + used, translated, length);
                used += length; source = after; continue;
            }
            int operation = nativeArticleOperations_Expression(source, article,
                parameters, translated, sizeof(translated), &after);
            if (operation < 0) return false;
            if (operation > 0) {
                size_t length = strlen(translated);
                if (length >= capacity - used) return false;
                memcpy(output + used, translated, length);
                used += length; source = after; continue;
            }
            ScriptExpressionType terrain_type;
            int terrain = nativeItemTerrain_Expression(source, article,
                parameters, translated, sizeof(translated), &after, &terrain_type);
            if (terrain < 0) return false;
            if (terrain > 0) {
                size_t length = strlen(translated);
                if (length >= capacity - used) return false;
                memcpy(output + used, translated, length);
                used += length; source = after; continue;
            }
            if (nativeItemLinks_Expression(source, translated, sizeof(translated), &after)) {
                size_t length = strlen(translated);
                if (length >= capacity - used) return false;
                memcpy(output + used, translated, length); used += length; source = after;
                continue;
            }
        }
        if (article && strncmp(source, "item.spawn_link(", 16) == 0) {
            char args[7][192], values[7][192], translated[1024]; const char* after;
            if (!nativeScriptCode_ParseArguments(source + 15, args, 7, &after)) return false;
            char* end; unsigned long slot = strtoul(args[0], &end, 10);
            if (*end || slot >= 4) return false;
            unsigned long backlink = strtoul(args[1], &end, 10);
            if (*end || backlink >= 4) return false;
            for (unsigned i = 2; i < 7; ++i)
                if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
            int written = snprintf(translated, sizeof(translated),
                "engine->item_spawn_link(gobj, %lu, %lu, (ItemKind)(%s), &(Vec3){ %s, %s, %s }, %s)",
                slot, backlink, values[2], values[3], values[4], values[5], values[6]);
            if (written < 0 || written >= sizeof(translated) || written >= capacity - used) return false;
            memcpy(output + used, translated, written); used += written; source = after; continue;
        }
        if (article && strncmp(source, "table.", 6) == 0) {
            const char* name = source + 6;
            const char* bracket = strchr(name, '[');
            if (bracket == NULL) return false;
            unsigned table = article->table_count;
            for (unsigned i = 0; i < article->table_count; ++i)
                if (strlen(article->tables[i].name) == (size_t)(bracket - name) &&
                    strncmp(name, article->tables[i].name, (size_t)(bracket - name)) == 0) {
                    table = i; break;
                }
            if (table == article->table_count) return false;
            const char* end = bracket + 1;
            unsigned nested = 0;
            while (*end) {
                if (*end == '[') ++nested;
                else if (*end == ']' && nested) --nested;
                else if (*end == ']') break;
                ++end;
            }
            char expression[192], index[384], translated[480];
            size_t length = (size_t)(end - bracket - 1);
            if (*end != ']' || length == 0 || length >= sizeof(expression)) return false;
            memcpy(expression, bracket + 1, length); expression[length] = '\0';
            if (!nativeFighterItemScript_Expression(expression, index, sizeof(index), article, parameters)) return false;
            int written = snprintf(translated, sizeof(translated),
                "scripted_article_%u_lookup_%u((int32_t)(%s))", article->index, table, index);
            if (written < 0 || (size_t)written >= sizeof(translated) ||
                (size_t)written >= capacity - used) return false;
            memcpy(output + used, translated, (size_t)written); used += written;
            source = end + 1;
            continue;
        }
        if (article && strncmp(source, "item.dynamic_joint(", 19) == 0) {
            char index[192], value[384], translated[512];
            unsigned channel; const char* after;
            if (!nativeItemOperations_DynamicJoint(source, index, &channel, &after) ||
                !nativeFighterItemScript_Expression(index, value, sizeof(value), article, parameters)) return false;
            int length = snprintf(translated, sizeof(translated),
                "engine->item_dynamic_joint_value(gobj, (unsigned)(%s), %u)", value, channel);
            if (length < 0 || (size_t)length >= capacity - used) return false;
            memcpy(output + used, translated, length); used += length; source = after;
            continue;
        }
        if (article && strncmp(source, "item.joint(", 11) == 0) {
            char path[40], translated[128];
            unsigned model, channel;
            const char* after;
            if (!parse_joint_property(source, path, &model, &channel, &after))
                return false;
            int length = snprintf(translated, sizeof(translated),
                "engine->item_joint_value(gobj, %u, \"%s\", %u)", model, path, channel);
            if (length < 0 || (size_t) length >= capacity - used) return false;
            memcpy(output + used, translated, (size_t) length);
            used += (size_t) length;
            source = after;
            continue;
        }
        if (article && (strncmp(source, "item.sweep_stage(", 17) == 0 ||
                        strncmp(source, "item.sweep_stage_normal(", 24) == 0)) {
            bool normal = strncmp(source, "item.sweep_stage_normal(", 24) == 0;
            source += normal ? 24 : 17;
            char values[3][160];
            for (unsigned i = 0; i < 3; ++i) {
                const char* begin = source;
                unsigned nested = 0;
                while (*source) {
                    if (*source == '(') ++nested;
                    else if (*source == ')' && nested) --nested;
                    else if (nested == 0 && (*source == ',' || *source == ')')) break;
                    ++source;
                }
                char argument[192];
                if (*source != (i == 2 ? ')' : ',') ||
                    source - begin >= sizeof(argument)) return false;
                memcpy(argument, begin, (size_t) (source - begin));
                argument[source - begin] = '\0';
                if (!nativeFighterItemScript_Expression(argument, values[i], sizeof(values[i]),
                                     article, parameters)) return false;
                ++source;
            }
            char translated[600];
            int length = snprintf(translated, sizeof(translated),
                "engine->item_sweep_stage(gobj, &(Vec3){ %s, %s, %s }, &item->pos, %s)",
                values[0], values[1], values[2], normal ? "&item->xDD4_itemVar.native_article.surface_normal" : "0");
            if (length < 0 || (size_t) length >= sizeof(translated) ||
                (size_t) length >= capacity - used) return false;
            memcpy(output + used, translated, (size_t) length);
            used += (size_t) length;
            continue;
        }
        if (article && strncmp(source, "vectors.cross_normalized_", sizeof("vectors.cross_normalized_")-1) == 0) {
            const char* open=strchr(source,'('); char name[40];
            if (!open || open-source >= sizeof(name)) return false;
            memcpy(name,source,open-source);name[open-source]=0;
            char args[6][192],values[6][128],translated[960];const char* after;
            if (!nativeScriptCode_ParseArguments(open,args,6,&after)) return false;
            for(unsigned i=0;i<6;++i)
                if (!nativeFighterItemScript_Expression(args[i],values[i],sizeof(values[i]),article,parameters)) return false;
            if (nativeScriptVectors_CrossComponent(name,values,translated,sizeof(translated)) != 1) return false;
            size_t length=strlen(translated);if(length>=capacity-used)return false;
            memcpy(output+used,translated,length);used+=length;source=after;continue;
        }
        if (article && (strncmp(source, "vectors.angle(", 14) == 0 || strncmp(source, "vectors.angle_xy(", 17) == 0)) {
            char args[6][192], values[6][128], translated[960]; const char* after;
            bool xy = strncmp(source, "vectors.angle_xy(", 17) == 0;
            unsigned count = xy ? 4 : 6;
            if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, count, &after)) return false;
            for (unsigned i = 0; i < count; ++i)
                if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
            int length = snprintf(translated, sizeof(translated),
                "engine->%s(&(Vec3){ %s, %s, %s }, &(Vec3){ %s, %s, %s })",
                xy ? "vector_angle_xy" : "vector_angle",
                values[0], values[1], xy ? "0" : values[2],
                values[xy ? 2 : 3], values[xy ? 3 : 4], xy ? "0" : values[5]);
            if (length < 0 || length >= sizeof(translated) || length >= capacity - used) return false;
            memcpy(output + used, translated, length); used += length; source = after; continue;
        }
        if (article && strncmp(source, "item.overlaps_owner(", 20) == 0) {
            char* end; unsigned long index = strtoul(source + 20, &end, 10);
            if (end == source + 20 || *end != ')' || index >= 4) return false;
            int length = snprintf(output + used, capacity - used, "engine->item_owner_overlap(gobj, %lu)", index);
            if (length < 0 || length >= capacity - used) return false;
            used += length; source = end + 1; continue;
        }
        if (article && (strncmp(source, "hitbox.enabled(", 15) == 0 || strncmp(source, "hitbox.radius(", 14) == 0)) {
            bool enabled = source[7] == 'e';
            unsigned prefix = enabled ? 15 : 14;
            char* end; unsigned long index = strtoul(source + prefix, &end, 10);
            if (end == source + prefix || *end != ')' || index >= 4) return false;
            char value[96]; int length = snprintf(value, sizeof(value), enabled
                ? "(item->x5D4_hitboxes[%lu].hit.state != HitCapsule_Disabled)"
                : "item->x5D4_hitboxes[%lu].hit.scale", index);
            if (length < 0 || length >= capacity - used) return false;
            memcpy(output + used, value, length); used += length; source = end + 1; continue;
        }
        if (isalpha((unsigned char) *source) || *source == '_') {
            char token[160]; // Full parameter name plus program.attrs prefix.
            size_t length = 0;
            while (isalnum((unsigned char) source[length]) ||
                   source[length] == '_' || source[length] == '.') {
                if (length + 1 >= sizeof(token)) return false;
                ++length;
            }
            memcpy(token, source, length);
            token[length] = '\0';
            source += length;
            const char* value = NULL;
            bool allows_parenthesis = false;
            char state[4096];
            if (article && article->expression_callback)
                for (unsigned i = 0; i < article->expression_callback->local_count; ++i)
                    if (strcmp(article->expression_callback->locals[i], token) == 0) {
                        snprintf(state, sizeof(state), "local_%s", token);
                        value = state;
                        break;
                    }
            if (value != NULL) { /* The callback owns this ordinary local. */ }
            else if (article && nativeScriptColors_Property(token, true) != NULL)
                value = nativeScriptColors_Property(token, true);
            else if (article && (!strcmp(token,"color.rgba") ||
                     !strcmp(token,"targets.controller_color") ||
                     !strcmp(token,"targets.controller_color_present"))) {
                if (!nativeScriptColors_ItemExpression(article,parameters,token,&source,state,sizeof(state))) return false;
                value = state;
            }
            else if (article && nativeItemTerrain_Constant(token) != NULL) {
                const char* next = source;
                while (isspace((unsigned char)*next)) ++next;
                if (*next == '(') return false;
                value = nativeItemTerrain_Constant(token);
            }
            else if (strncmp(token, "event.", 6) == 0) {
                if (article == NULL || article->expression_callback != &article->callbacks[ARTICLE_HELD_REQUEST]) return false;
                if (strcmp(token, "event.action") == 0) value = "held_action";
                else if (strcmp(token, "event.frame_rate") == 0) value = "held_frame_rate";
                else if (strcmp(token, "event.duration") == 0) value = "held_duration";
                else return false;
            }
            else if (article && article->in_signal && strcmp(token, "signal.value") == 0)
                value = "signal_value";
            else if (article && strncmp(token, "item.state.", 11) == 0) {
                const ScriptStateField* field = nativeFighterScript_StateField(
                    &article->local_state, token + 11);
                if (field != NULL && !field->array) {
                    const char* type = field->type == ScriptState_Float
                        ? "float" : field->type == ScriptState_Int
                        ? "int" : "bool";
                    snprintf(state, sizeof(state),
                             "scripted_article_state_%s(item, %u)",
                             type, field->offset);
                    value = state;
                }
            } else if ((strncmp(token, "attrs.", 6) == 0 ||
                        strncmp(token, "program.attrs.", 14) == 0) && parameters != NULL) {
                const char* name = token + (strncmp(token, "program.attrs.", 14) == 0 ? 14 : 6);
                for (unsigned i = 0; i < parameters->count; ++i)
                    if (strcmp(name, parameters->fields[i].name) == 0) {
                        snprintf(state, sizeof(state), "script_parameters[%u].%s",
                            i, parameters->fields[i].type ==
                                NativeFighterParameter_Float ? "f" : "i");
                        value = state;
                        break;
                    }
            } else if (strcmp(token, "Motion.catch_dash") == 0) value = "ftCo_MS_CatchDash";
            else if (strcmp(token, "Motion.throw_forward") == 0) value = "ftCo_MS_ThrowF";
            else if (strcmp(token, "Motion.throw_back") == 0) value = "ftCo_MS_ThrowB";
            else if (strcmp(token, "Motion.throw_up") == 0) value = "ftCo_MS_ThrowHi";
            else if (strcmp(token, "Motion.throw_down") == 0) value = "ftCo_MS_ThrowLw";
            else if (strcmp(token, "velocity.x") == 0)
                value = "item->x40_vel.x";
            else if (strcmp(token, "velocity.y") == 0)
                value = "item->x40_vel.y";
            else if (strcmp(token, "velocity.z") == 0)
                value = "item->x40_vel.z";
            else if (strcmp(token, "position.x") == 0)
                value = "item->pos.x";
            else if (strcmp(token, "position.y") == 0)
                value = "item->pos.y";
            else if (strcmp(token, "position.z") == 0)
                value = "item->pos.z";
            else if (strcmp(token, "facing") == 0)
                value = "item->facing_dir";
            else if (article && nativeItemOperations_Property(token, false) != NULL)
                value = nativeItemOperations_Property(token, false);
            else if (article && strcmp(token, "item.sweep_stage_flags") == 0 && strncmp(source, "()", 2) == 0) {
                source += 2; value = "engine->item_sweep_stage_flags(gobj)";
            }
            else if (article && strcmp(token, "item.attachment_owner_is") == 0 && strncmp(source, "(other)", 7) == 0) {
                source += 7; value = "(other != NULL && other == item->xDD4_itemVar.native_article.attachment_owner)";
            }
            else if (article && strcmp(token, "item.reflect_facing") == 0)
                value = "item->xC68";
            else if (article && strcmp(token, "item.launch_angle") == 0)
                value = "item->xDD4_itemVar.native_article.launch_angle";
            else if (article && strcmp(token, "item.launch_speed") == 0)
                value = "item->xDD4_itemVar.native_article.launch_speed";
            else if (article && strncmp(token, "item.launch_origin.", 19) == 0 &&
                     token[19] >= 'x' && token[19] <= 'z' && token[20] == '\0') {
                snprintf(state, sizeof(state),
                    "item->xDD4_itemVar.native_article.launch_origin.%c", token[19]);
                value = state;
            }
            else if (article && strncmp(token, "item.surface_normal.", 20) == 0 &&
                     token[20] >= 'x' && token[20] <= 'z' && token[21] == '\0') {
                snprintf(state, sizeof(state), "item->xDD4_itemVar.native_article.surface_normal.%c", token[20]);
                value = state;
            }
            else if (article && strcmp(token, "item.kind") == 0) value = "item->kind";
            else if (strncmp(token, "It_Kind_", 8) == 0) value = token;
            else if (!article && strcmp(token, "item.age") == 0)
                value = "item->xDD4_itemVar.native_holdable.age";
            else if (strcmp(token, "item.life") == 0)
                value = "item->xD44_lifeTimer";
            else if (article && strcmp(token, "item.motion_state") == 0)
                value = "item->msid";
            else if (article && strcmp(token, "item.collision_group") == 0)
                value = "item->xAC4_ignoreItemID";
            else if (article && strcmp(token, "stage.internal_kind") == 0)
                value = "engine->stage_internal_kind()";
            else if (article && strcmp(token, "item.remove_at_bounds") == 0)
                value = "item->xDCC_flag.b3";
            else if (article && strcmp(token, "owner.incapacitated") == 0)
                value = "(scripted_article_attached_owner(item) != NULL && engine->fighter_incapacitated(item->xDD4_itemVar.native_article.attachment_owner))";
            else if (article && strcmp(token, "owner.action_detaches_items") == 0)
                value = "(scripted_article_attached_owner(item) != NULL && scripted_article_attached_owner(item)->x2070.x2071_b6)";
            else if (article && strcmp(token, "item.creation_id") == 0)
                value = "item->x1C";
            else if (article && strcmp(token, "item.contact_fighter") == 0)
                value = "engine->item_contact_fighter(gobj)";
            else if (article && strcmp(token, "item.animation_frame") == 0)
                value = "item->x5CC_currentAnimFrame";
            else if (article && strcmp(token, "item.suppress_destruction_effects") == 0)
                value = "item->xDCF_flag.b2";
            else if (article && strcmp(token, "item.creation_value") == 0)
                value = "item->xDD4_itemVar.native_article.creation_value";
            else if (article && strcmp(token, "item.exit_category") == 0)
                value = "item->xDCC_flag.b4567";
            else if (article && strcmp(token, "item.damage_hitlag") == 0)
                value = "item->xDC8_word.flags.xD";
            else if (article && strcmp(token, "item.contact_hitlag") == 0)
                value = "item->xDC8_word.flags.xC";
            else if (article && strcmp(token, "item.physics_disabled") == 0)
                value = "item->xDD0_flag.b3";
            else if (article && strcmp(token, "item.custom_floor_motion") == 0)
                value = "item->xDCD_flag.b3";
            else if (article && strcmp(token, "item.reflection_multiplier") == 0)
                value = "item->xC70";
            else if (article && strcmp(token, "item.drop_speed") == 0) value = "item->xCC_item_attr->x18";
            else if (article && strcmp(token, "item.animation_rate") == 0) value = "item->x5D0_animFrameSpeed";
            else if (article && strcmp(token, "item.animation_finished") == 0) value = "(!engine->model_animation_active(gobj->hsd_obj))";
            else if (article && strcmp(token, "item.base_spin_speed") == 0) value = "item->spin_spd";
            else if (article && strcmp(token, "item.spin_speed") == 0) value = "item->xD3C_spinSpeed";
            else if (article && strcmp(token, "item.reverse_spin") == 0) value = "item->xDC8_word.flags.x19";
            else if (article && strcmp(token, "owner.facing") == 0) value = "(scripted_article_attached_owner(item) != NULL ? scripted_article_attached_owner(item)->facing_dir : 0.0f)";
            else if (article && strncmp(token, "slide_velocity.", 15) == 0 &&
                     token[15] && strchr("xyz", token[15]) && !token[16]) {
                snprintf(state, sizeof(state), "item->x7C.%c", token[15]); value = state;
            }
            else if (article && strcmp(token, "item.expire_tick") == 0 && strncmp(source, "()", 2) == 0) {
                source += 2; value = "engine->item_expire_tick(gobj)";
            }
            else if (article && strcmp(token, "item.throw_speed_damage") == 0)
                value = "item->xDC8_word.flags.x14";
            else if (article && strcmp(token, "owner.action_category") == 0)
                value = "(scripted_article_attached_owner(item) != NULL ? scripted_article_attached_owner(item)->x2070.x2071_b0_3 : 0)";
            else if (article && strcmp(token, "item.check_ground") == 0 && strncmp(source, "()", 2) == 0) {
                value = "engine->item_check_ground(gobj)";
                source += 2;
            }
            else if (strcmp(token, "item.bounce_stage") == 0 &&
                     strncmp(source, "()", 2) == 0) {
                value = "engine->item_bounce_stage(gobj)";
                source += 2;
            }
            else if (article && strcmp(token, "item.attachment_active") == 0)
                value = "(scripted_article_attached_owner(item) != NULL)";
            else if (article && strcmp(token, "item.creator_present") == 0)
                value = "(item->xDD4_itemVar.native_article.attachment_owner != NULL)";
            else if (article && strcmp(token, "owner.motion") == 0)
                value = "scripted_article_owner_motion(item)";
            else if (article && strcmp(token, "owner.model_scale") == 0)
                value = "(scripted_article_attached_owner(item) != NULL ? engine->fighter_model_scale(item->xDD4_itemVar.native_article.attachment_owner) : 1.0f)";
            else if (article && strcmp(token, "owner.instance_scale_y") == 0)
                value = "(scripted_article_attached_owner(item) != NULL ? scripted_article_attached_owner(item)->x34_scale.y : 1.0f)";
            else if (article && strcmp(token, "owner.root_scale_y") == 0)
                value = "(scripted_article_attached_owner(item) != NULL ? engine->fighter_root_scale_y(item->xDD4_itemVar.native_article.attachment_owner) : 1.0f)";
            else if (article && strcmp(token, "holder.motion") == 0)
                value = "engine->target_motion(engine->item_current_owner(gobj))";
            else if (article && strcmp(token, "holder.root_scale_y") == 0)
                value = "(scripted_article_holder(item) != NULL ? engine->fighter_root_scale_y(item->owner) : 1.0f)";
            else if (article && strcmp(token, "holder.instance_scale_y") == 0)
                value = "engine->target_instance_scale_y(engine->item_current_owner(gobj))";
            else if (article && strcmp(token, "holder.incapacitated") == 0)
                value = "(scripted_article_holder(item) != NULL && engine->fighter_incapacitated(item->owner))";
            else if (article && strncmp(token, "creator.state.", 14) == 0) {
                const ScriptStateField* field = nativeFighterScript_StateField(article->fighter_state, token + 14);
                if (field == NULL || field->array) return false;
                if (field->type == ScriptState_Bool)
                    snprintf(state, sizeof(state), "(scripted_article_creator(item) != NULL && scripted_article_creator(item)->native_module_state[script_state_base + %u] != 0)", field->offset);
                else snprintf(state, sizeof(state), "(scripted_article_creator(item) != NULL ? script_state_get_%s(scripted_article_creator(item), script_state_base + %u) : 0)", field->type == ScriptState_Float ? "float" : "int", field->offset);
                value = state;
            }
            else if (article && strcmp(token, "creator.exists") == 0) value = "(scripted_article_creator(item) != NULL)";
            else if (article && (strcmp(token, "creator.position.x") == 0 || strcmp(token, "creator.position.y") == 0 || strcmp(token, "creator.position.z") == 0)) {
                snprintf(state, sizeof(state), "(scripted_article_creator(item) != NULL ? scripted_article_creator(item)->cur_pos.%c : 0.0f)", token[17]);
                value = state;
            }
            else if (article && strcmp(token, "creator.motion") == 0) value = "(scripted_article_creator(item) != NULL ? scripted_article_creator(item)->motion_id : -1)";
            else if (article && strncmp(token, "creator.input.", 14) == 0 &&
                     (token[14] == 'x' || token[14] == 'y') && token[15] == '\0') {
                snprintf(state, sizeof(state), "(scripted_article_creator(item) != NULL ? scripted_article_creator(item)->input.lstick[0].%c : 0.0f)", token[14]);
                value = state;
            }
            else if (article && strcmp(token, "creator.model_scale") == 0) value = "(scripted_article_creator(item) != NULL ? engine->fighter_model_scale(item->xDD4_itemVar.native_article.attachment_owner) : 1.0f)";
            else if (article && strcmp(token, "creator.instance_scale_y") == 0) value = "(scripted_article_creator(item) != NULL ? scripted_article_creator(item)->x34_scale.y : 1.0f)";
            else if (article && strcmp(token, "creator.root_scale_y") == 0) value = "(scripted_article_creator(item) != NULL ? engine->fighter_root_scale_y(item->xDD4_itemVar.native_article.attachment_owner) : 1.0f)";
            else if (article && strcmp(token, "creator.incapacitated") == 0) value = "(scripted_article_creator(item) != NULL && engine->fighter_incapacitated(item->xDD4_itemVar.native_article.attachment_owner))";
            else if (article && strcmp(token, "creator.action.category") == 0) value = "(scripted_article_creator(item) != NULL ? scripted_article_creator(item)->x2070.x2071_b0_3 : 0)";
            else if (article && strcmp(token, "creator.hitlag") == 0) value = "(scripted_article_creator(item) != NULL && scripted_article_creator(item)->x2219_b5)";
            else if (article && strcmp(token, "item.current_owner") == 0) value = "engine->item_current_owner(gobj)";
            else if (article && strcmp(token, "item.reflector") == 0) value = "engine->item_reflector(gobj)";
            else if (article && strcmp(token, "item.half_life") == 0) value = "item->xD48_halfLifeTimer";
            else if (article && strcmp(token, "item.reflection_owner_transfer_disabled") == 0) value = "item->xDCC_flag.b1";
            else if (article && strncmp(token, "owner.state.", 12) == 0) {
                const ScriptStateField* field = nativeFighterScript_StateField(
                    article->fighter_state, token + 12);
                if (field != NULL && !field->array) {
                    const char* type = field->type == ScriptState_Float
                        ? "float" : field->type == ScriptState_Int
                        ? "int" : "bool";
                    snprintf(state, sizeof(state),
                             "scripted_article_owner_state_%s(item, %u)",
                             type, field->offset);
                    value = state;
                }
            }
            else if (article && strncmp(token, "creator.command", 15) == 0 &&
                     token[15] >= '0' && token[15] <= '3' && token[16] == '\0') {
                snprintf(state, sizeof(state),
                    "(scripted_article_creator(item) != NULL ? scripted_article_creator(item)->cmd_vars[%c] : 0)", token[15]);
                value = state;
            }
            else if (article && strncmp(token, "owner.command", 13) == 0 &&
                     token[13] >= '0' && token[13] <= '3' &&
                     token[14] == '\0') {
                snprintf(state, sizeof(state),
                    "scripted_article_owner_command(item, %c)",
                    token[13]);
                value = state;
            } else if (article && strncmp(token, "item.command", 12) == 0 &&
                       token[12] >= '0' && token[12] <= '3' &&
                       token[13] == '\0') {
                snprintf(state, sizeof(state),
                    "item->xD%02X_itcmd_var%c", 0xAC + (token[12] - '0') * 4,
                    '0' + (token[12] - '0'));
                value = state;
            } else if (article && strcmp(token, "item.collision_mode") == 0)
                value = "item->xD0C";
            else if (article && strcmp(token, "item.owner_bound") == 0)
                value = "item->xDC8_word.flags.x13";
            else if (article && strcmp(token, "item.world_interaction") == 0)
                value = "item->xDC8_word.flags.x15";
            else if (article && strncmp(token, "item.params.", 12) == 0) {
                int index = article_parameter(article, parameters, token + 12);
                if (index < 0) return false;
                snprintf(state, sizeof(state), "script_parameters[%u].%s", index,
                    parameters->fields[index].type == NativeFighterParameter_Float
                        ? "f" : "i");
                value = state;
            }
            else if (article && strncmp(token, "item.attr.", 10) == 0) {
                for (unsigned i = 0; i < article->attribute_count; ++i)
                    if (strcmp(token + 10, article->attributes[i].name) == 0) {
                        snprintf(state, sizeof(state),
                                 "((const %s*)item->xC4_article_data->x4_specialAttributes)[%u]",
                                 article->attributes[i].integer ? "int32_t" : "float",
                                 article->attributes[i].offset / 4);
                        value = state;
                        break;
                    }
            }
            else if (strcmp(token, "item.grounded") == 0)
                value = article ? "(item->ground_or_air == GA_Ground)"
                                : "(item->msid == 1)";
            else if (strcmp(token, "item.airborne") == 0)
                value = article ? "(item->ground_or_air == GA_Air)"
                                : "(item->msid == 2 || item->msid == 3)";
            else if (article && strcmp(token, "item.accumulated_damage") == 0)
                value = "item->xC9C";
            else if (article && strcmp(token, "item.base_scale") == 0)
                value = "item->xCC_item_attr->x60_scale";
            else if (!article && strcmp(token, "item.ground_contact") == 0)
                value = "((item->xDD4_itemVar.native_holdable.collision_flags & 0xE) != 0)";
            else if (!article && strncmp(token, "item.state", 10) == 0 &&
                     token[10] >= '0' && token[10] <= '7' && token[11] == '\0') {
                snprintf(state, sizeof(state),
                    "item->xDD4_itemVar.native_holdable.state[%c]",
                    token[10]);
                value = state;
            } else if (article && strcmp(token, "collision.floor_contact") == 0)
                value = "((item->x378_itemColl.env_flags & Collide_FloorMask) != 0)";
            else if (article && strcmp(token, "collision.ceiling_contact") == 0)
                value = "((item->x378_itemColl.env_flags & Collide_CeilingMask) != 0)";
            else if (article && strcmp(token, "collision.left_wall") == 0)
                value = "((item->x378_itemColl.env_flags & Collide_LeftWallMask) != 0)";
            else if (article && strcmp(token, "collision.right_wall") == 0)
                value = "((item->x378_itemColl.env_flags & Collide_RightWallMask) != 0)";
            else if (article && (strcmp(token, "ecb.up") == 0 || strcmp(token, "ecb.down") == 0 ||
                     strcmp(token, "ecb.front") == 0 || strcmp(token, "ecb.back") == 0)) {
                snprintf(state, sizeof(state), "item->x378_itemColl.ecb_source.%s", token + 4); value = state;
            }
            else if (article && (strncmp(token, "floor.normal.", 13) == 0 ||
                     strncmp(token, "ceiling.normal.", 15) == 0 ||
                     strncmp(token, "wall.left.normal.", 17) == 0 ||
                     strncmp(token, "wall.right.normal.", 18) == 0)) {
                size_t length = strlen(token);
                unsigned expected = strncmp(token, "floor.", 6) == 0 ? 14 :
                    strncmp(token, "ceiling.", 8) == 0 ? 16 :
                    strncmp(token, "wall.left.", 10) == 0 ? 18 : 19;
                if (length != expected) return false;
                char axis = token[length - 1];
                if ((axis != 'x' && axis != 'y') || token[length - 2] != '.') return false;
                const char* field = strncmp(token, "floor.", 6) == 0 ? "floor" :
                    strncmp(token, "ceiling.", 8) == 0 ? "ceiling" :
                    strncmp(token, "wall.left.", 10) == 0 ? "left_facing_wall" : "right_facing_wall";
                snprintf(state, sizeof(state), "item->x378_itemColl.%s.normal.%c", field, axis); value = state;
            }
            else {
                static const struct { const char* name; const char* code; bool parenthesis; } syntax[] = {
                    { "random.float", "engine->random_float", true },
                    { "random.int", "engine->random_int", true },
                    { "wrap_signed_angle", "engine->wrap_signed_angle", true },
                    { "sin", "engine->sin_float", true }, { "cos", "engine->cos_float", true },
                    { "abs", "engine->abs_float", true }, { "sqrt", "engine->sqrt_float", true },
                    { "min", "engine->min_float", true }, { "max", "engine->max_float", true },
                    { "atan2", "engine->angle_atan2_float", true },
                    { "wrap_angle", "engine->wrap_angle", true },
                    { "float", "(float)", true }, { "uint", "(uint32_t)", true },
                    { "int", "(int32_t)", true },
                    { "pi", "3.14159265358979323846", false },
                    { "tau", "6.28318530717958647692", false },
                    { "and", "&&", true }, { "or", "||", true }, { "not", "!", true },
                    { "true", "true", false }, { "false", "false", false },
                };
                for (unsigned i = 0; i < sizeof(syntax) / sizeof(syntax[0]); ++i)
                    if (strcmp(token, syntax[i].name) == 0) {
                        value = syntax[i].code;
                        allows_parenthesis = syntax[i].parenthesis;
                        break;
                    }
            }
            if (value == NULL) return false;
            const char* next = source;
            while (isspace((unsigned char)*next)) ++next;
            if (*next == '(' && !allows_parenthesis) return false;
            length = strlen(value);
            if (length >= capacity - used) return false;
            memcpy(output + used, value, length);
            used += length;
            continue;
        }
        if (isdigit((unsigned char)*source) || (*source == '.' && isdigit((unsigned char)source[1]))) {
            if (source[0] == '0' && (source[1] == 'x' || source[1] == 'X')) {
                char* end;
                unsigned long long bits = strtoull(source, &end, 16);
                if (end == source + 2 || bits > UINT32_MAX ||
                    isalnum((unsigned char)*end) || *end == '_') return false;
                int length = snprintf(output + used, capacity - used, "%lluu", bits);
                if (length < 0 || (size_t)length >= capacity - used) return false;
                used += length; source = end; continue;
            }
            const char* begin = source; char* end;
            double number = strtod(begin, &end);
            if (end == begin || !isfinite(number) || !isfinite((float)number)) return false;
            bool fractional = false;
            for (const char* p = begin; p != end; ++p)
                if (*p == '.' || *p == 'e' || *p == 'E') fractional = true;
            size_t length = end - begin;
            if (length + fractional >= capacity - used) return false;
            memcpy(output + used, begin, length); used += length;
            if (fractional) output[used++] = 'f';
            source = end; continue;
        }
        if (!strchr("0123456789. +-*/%()<>!=,&|^~", *source) ||
            used + 1 >= capacity) return false;
        output[used++] = *source++;
    }
    output[used] = '\0';
    return used != 0;
}

bool nativeFighterItemScript_Action(const char* line, unsigned callback,
                        const ArticleDeclaration* article, bool interaction,
                        const NativeFighterParameters* parameters,
                        char* output, size_t capacity)
{
    int helper = nativeItemFunctions_Call(article, line, parameters, output, capacity);
    if (helper != 0) return helper > 0;
    if (strcmp(line, "pass") == 0)
        return snprintf(output, capacity, ";") < capacity;

    if (nativeScriptLifetime_TargetHost(line) != NULL) {
        char value[1024];
        return nativeFighterItemScript_Expression(line, value, sizeof(value), article, parameters) &&
            snprintf(output, capacity, "%s;", value) < capacity;
    }
    if (strncmp(line, "articles.ref(", 13) == 0 && strstr(line, " = ")) return false;
    if (article) {
        int presentation = nativeItemPresentation_Action(line, article, parameters, output, capacity);
        int spatial = nativeScriptSpatial_ItemAction(article, parameters, line, output, capacity);
        int color = nativeScriptColors_ItemAction(article, parameters, line, output, capacity);
        if (color) return color > 0;
        if (spatial != 0) return spatial > 0;
        if (presentation != 0) return presentation > 0;
    }
    if (article && strncmp(line, "item.spawn_effect_at_position(", 30) == 0) {
        char args[4][192], values[4][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 29, args, 4, &after) || *after) return false;
        for (unsigned i = 0; i < 4; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity, "engine->spawn_effect((s32)(%s), gobj, &(Vec3){ %s, %s, %s });",
                        values[0], values[1], values[2], values[3]) < capacity;
    }
    if (article && strncmp(line, "owner.quake(", 12) == 0) {
        char args[1][192], value[192]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 11, args, 1, &after) || *after ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity,
            "{ Fighter* owner = scripted_article_attached_owner(item); if (owner) engine->camera_quake(%s, &owner->cur_pos); }", value) < capacity;
    }
    if (article && strncmp(line, "owner.rumble(", 13) == 0) {
        char args[2][192], values[2][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 12, args, 2, &after) || *after) return false;
        for (unsigned i = 0; i < 2; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "{ Fighter* owner = scripted_article_attached_owner(item); if (owner) engine->rumble(owner, %s, %s); }", values[0], values[1]) < capacity;
    }
    if (strncmp(line, "if ", 3) == 0) {
        const char* colon = strstr(line + 3, ": ");
        if (colon == NULL || colon - line >= 192) return false;
        char condition[192], translated[512], action[1024];
        memcpy(condition, line + 3, (size_t) (colon - line - 3));
        condition[colon - line - 3] = '\0';
        if (!nativeFighterItemScript_Expression(condition, translated, sizeof(translated),
                             article, parameters) ||
            !nativeFighterItemScript_Action(colon + 2, callback, article, interaction,
                         parameters, action, sizeof(action)))
            return false;
        return snprintf(output, capacity, "if (%s) { %s }", translated,
                        action) < capacity;
    }
    if (strcmp(line, "return") == 0)
        return article != NULL && callback != ITEM_GROUND && callback != ITEM_HIT &&
               snprintf(output, capacity, "return;") < capacity;
    if (strcmp(line, "return true") == 0 ||
        strcmp(line, "return false") == 0) {
        if (callback != ITEM_GROUND && callback != ITEM_HIT) return false;
        return snprintf(output, capacity, "%s;", line) < capacity;
    }
    if (strcmp(line, "item.pause()") == 0)
        return snprintf(output, capacity, "engine->item_pause(gobj);") < capacity;
    if (strcmp(line, "item.resume()") == 0)
        return snprintf(output, capacity, "engine->item_resume(gobj);") < capacity;
    if (strcmp(line, "return item.reflect()") == 0) {
        if (callback != ITEM_GROUND && callback != ITEM_HIT) return false;
        return snprintf(output, capacity,
                        "return engine->item_reflect(gobj);") < capacity;
    }
    if (strcmp(line, "return item.shield_bounce()") == 0) {
        if (callback != ITEM_GROUND && callback != ITEM_HIT) return false;
        return snprintf(output, capacity,
                        "return engine->item_shield_bounce(gobj);") < capacity;
    }
    if (strncmp(line, "return ", 7) == 0) {
        char value[512];
        return (callback == ITEM_GROUND || callback == ITEM_HIT) &&
            nativeFighterItemScript_Expression(line + 7, value, sizeof(value), article, parameters) &&
            snprintf(output, capacity, "return %s;", value) < capacity;
    }
    if (article != NULL) {
        char terrain[1024]; const char* after;
        int query = nativeScriptQueries_Article(article, parameters, line,
            &after, terrain, sizeof(terrain));
        if (query < 0 || (query > 0 && *after)) return false;
        if (query > 0) return snprintf(output, capacity, "%s;", terrain) < capacity;
        ScriptExpressionType terrain_type;
        int found = nativeItemTerrain_Expression(line, article, parameters,
                                                 terrain, sizeof(terrain), &after, &terrain_type);
        if (found < 0 || (found > 0 && *after)) return false;
        if (found > 0)
            return snprintf(output, capacity, "%s;", terrain) < capacity;
    }
    if (strcmp(line, "item.interact(other)") == 0)
        return interaction && article != NULL &&
            snprintf(output, capacity, "engine->item_interact(gobj, other);")
                < capacity;
    if (article && nativeItemLinks_Action(line, output, capacity)) return true;
    if (article && (strncmp(line, "owner.call(", 11) == 0 || strncmp(line, "creator.call(", 13) == 0)) {
        const char* after;
        return nativeItemFunctions_RecipientCall(article, line, parameters, false,
            output, capacity, &after) == 1;
    }
    if (article && strncmp(line, "item.signal(", 12) == 0) {
        char args[2][192], value[384]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 11, args, 2, &after) || *after ||
            !nativeFighterItemScript_Expression(args[1], value, sizeof(value), article, parameters)) return false;
        size_t length = strlen(args[0]);
        if (length < 3 || args[0][0] != '"' || args[0][length - 1] != '"') return false;
        args[0][length - 1] = 0;
        for (unsigned i = 0; i < article->signal_count; ++i)
            if (strcmp(args[0] + 1, article->signals[i].name) == 0 &&
                !(article->in_signal && i + 1 == article->signal_count)) {
                if (!article->expression_callback) return false;
                article->expression_callback->signal_calls |= UINT32_C(1) << i;
                return snprintf(output, capacity, "scripted_article_%u_signal_%u(gobj, %s);", article->index, i, value) < capacity;
            }
        return false;
    }
    if (article && strncmp(line, "item.signal_link(", 17) == 0) {
        char args[3][192], value[384]; const char* after; char* end;
        if (!nativeScriptCode_ParseArguments(line + 16, args, 3, &after) || *after) return false;
        unsigned long slot = strtoul(args[0], &end, 10);
        size_t length = strlen(args[1]);
        if (*end || slot >= 4 || length < 3 || args[1][0] != '"' || args[1][length - 1] != '"') return false;
        args[1][length - 1] = '\0';
        if (!nativeScriptCode_Identifier(args[1] + 1, length - 2, 48) ||
            !nativeFighterItemScript_Expression(args[2], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity, "engine->item_signal_link(gobj, %lu, UINT32_C(%u), %s);",
            slot, nativeScriptSignal_Hash(args[1] + 1), value) < capacity;
    }
    if (article && strncmp(line, "item.change_state_flags(", 24) == 0) {
        char args[2][192], values[2][384]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 23, args, 2, &after) || *after) return false;
        for (unsigned i = 0; i < 2; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "{ int32_t next = (int32_t)(%s); if ((uint32_t)next < %u) engine->item_change_state_flags(gobj, next, (Item_StateChangeFlags)(%s)); }",
            values[0], article->state_count, values[1]) < capacity;
    }
    if (article && strcmp(line, "item.show_model()") == 0)
        return snprintf(output, capacity, "engine->item_show_model(gobj);") < capacity;
    if (article && strcmp(line, "item.hide_model()") == 0)
        return snprintf(output, capacity, "engine->item_hide_model(gobj);") < capacity;
    if (article && strncmp(line, "item.stage_collision(", 21) == 0 && strcmp(line, "item.stage_collision()") != 0) {
        char args[1][192], value[384]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 20, args, 1, &after) || *after ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity, "if (%s) engine->item_stage_collision(gobj); else engine->item_stage_collision_land(gobj);", value) < capacity;
    }
    if (article && strncmp(line, "item.attach_to_owner(", 21) == 0) {
        char args[2][192], value[384], held[384]; const char* after;
        bool registers = nativeScriptCode_ParseArguments(line + 20, args, 2, &after);
        if ((!registers && !nativeScriptCode_ParseArguments(line + 20, args, 1, &after)) || *after ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters) ||
            (registers && !nativeFighterItemScript_Expression(args[1], held, sizeof(held), article, parameters))) return false;
        return snprintf(output, capacity,
            "{ int32_t bone = %s; Fighter* owner = scripted_article_attached_owner(item); "
            "if (owner != NULL && (uint32_t)bone < engine->bone_count(owner)) { "
            "engine->item_attach_to_owner(gobj, item->xDD4_itemVar.native_article.attachment_owner, (Fighter_Part)bone); "
            "if (%s) owner->x1984_heldItemSpec = gobj; } }", value, registers ? held : "false") < capacity;
    }
    if (article && strncmp(line, "item.release_attachment(", 24) == 0) {
        char args[4][192], values[4][192]; const char* after;
        bool preserve = nativeScriptCode_ParseArguments(line + 23, args, 4, &after);
        if ((!preserve && !nativeScriptCode_ParseArguments(line + 23, args, 3, &after)) || *after) return false;
        for (unsigned i = 0; i < (preserve ? 4 : 3); ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "{ Fighter* owner = scripted_article_attached_owner(item); bool preserve = %s; Item_GObj* held = owner != NULL ? owner->item_gobj : NULL; "
            "engine->item_release_attachment(gobj, &(Vec3){ %s, %s, %s }); if (preserve && owner != NULL) owner->item_gobj = held; }",
            preserve ? values[3] : "false", values[0], values[1], values[2]) < capacity;
    }
    if (article && strncmp(line, "item.spawn_scaled_root_effect(", 30) == 0) {
        char args[2][192], values[2][384]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 2, &after) || *after) return false;
        for (unsigned i = 0; i < 2; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity, "{ float scale = %s; engine->spawn_async_effect(gobj, &item->xBC0, 4, %s, gobj->hsd_obj, &scale); }", values[1], values[0]) < capacity;
    }
    if (article && (strncmp(line, "item.spawn_effect_on_joint(", 27) == 0 ||
                   strncmp(line, "item.spawn_async_joint_effect(", 29) == 0)) {
        bool async = strncmp(line, "item.spawn_async_joint_effect(", 29) == 0;
        char args[3][192], value[384], path[40], property[256]; const char* after;
        unsigned model, channel;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, async ? 3 : 2, &after) || *after ||
            (async && nativeFighterItemScript_ExpressionType(args[0], article, parameters) != ScriptType_Int) ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return false;
        if (snprintf(property, sizeof(property), "item.joint(%s).exists", args[1]) >= sizeof(property) ||
            !parse_joint_property(property, path, &model, &channel, &after) || *after || model) return false;
        if (async) {
            char* end;
            unsigned long mode = strtoul(args[2], &end, 10);
            if (end == args[2] || *end || (mode != 0 && mode != 3)) return false;
            return snprintf(output, capacity, "engine->item_spawn_async_joint_effect(gobj, %s, \"%s\", %lu);", value, path, mode) < capacity;
        }
        return snprintf(output, capacity, "engine->item_spawn_joint_effect(gobj, %s, \"%s\");", value, path) < capacity;
    }
    if (strcmp(line, "item.activate_attached()") == 0)
        return article != NULL &&
            snprintf(output, capacity,
                     "engine->item_activate_attached(gobj);") < capacity;
    if (article && (strncmp(line, "vectors.normalize(", 18) == 0 || strncmp(line, "vectors.normalize_xy(", 21) == 0)) {
        char args[3][192]; const char* after;
        const ScriptStateField* fields[3];
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 3, &after) || *after) return false;
        for (unsigned i = 0; i < 3; ++i) {
            if (strncmp(args[i], "item.state.", 11) != 0) return false;
            fields[i] = nativeFighterScript_StateField(&article->local_state, args[i] + 11);
            if (fields[i] == NULL || fields[i]->array || fields[i]->type != ScriptState_Float) return false;
            for (unsigned j = 0; j < i; ++j) if (fields[i] == fields[j]) return false;
        }
        return snprintf(output, capacity,
            "{ Vec3 v = { scripted_article_state_float(item, %u), scripted_article_state_float(item, %u), scripted_article_state_float(item, %u) }; "
            "engine->%s(&v); scripted_article_set_state_float(item, %u, v.x); "
            "scripted_article_set_state_float(item, %u, v.y); scripted_article_set_state_float(item, %u, v.z); }",
            fields[0]->offset, fields[1]->offset, fields[2]->offset,
            strstr(line, "normalize_xy") ? "vector_normalize_xy" : "vector_normalize",
            fields[0]->offset, fields[1]->offset, fields[2]->offset) < capacity;
    }
    if (article && strncmp(line, "hitbox.set_radius(", 18) == 0) {
        char args[2][192], value[384]; const char* after; char* end;
        if (!nativeScriptCode_ParseArguments(line + 17, args, 2, &after) || *after) return false;
        unsigned long index = strtoul(args[0], &end, 10);
        if (end == args[0] || *end || index >= 4 ||
            !nativeFighterItemScript_Expression(args[1], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity, "{ float radius = %s; if (isfinite(radius) && radius >= 0) item->x5D4_hitboxes[%lu].hit.scale = radius; }", value, index) < capacity;
    }
    if (article && strcmp(line, "hitboxes.disable_all()") == 0)
        return snprintf(output, capacity, "engine->item_disable_hitboxes(gobj);") < capacity;
    if (article && strncmp(line, "item.disable_hitbox(", 20) == 0) {
        char args[1][192], value[384]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 19, args, 1, &after) || *after ||
            nativeFighterItemScript_ExpressionType(args[0], article, parameters) != ScriptType_Int ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return false;
        int n = snprintf(output, capacity,
            "{ int32_t capsule=(%s); if((uint32_t)capsule<4) engine->item_disable_hitbox(gobj,capsule); }", value);
        return n >= 0 && (size_t)n < capacity;
    }
    if (article && strncmp(line, "item.spawn_root_effect(", 23) == 0) {
        char args[1][192], value[384]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 22, args, 1, &after) || *after ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity, "engine->spawn_effect((s32)(%s), gobj, gobj->hsd_obj);", value) < capacity;
    }
    if (article && strncmp(line, "item.mirror_velocity(", 21) == 0 && strcmp(line, "item.mirror_velocity()") != 0) {
        char args[3][192], values[3][512]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 3, &after) || *after) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity, "engine->vector_mirror(&item->x40_vel, &(Vec3){ %s, %s, %s });", values[0], values[1], values[2]) < capacity;
    }
    if (strcmp(line, "item.mirror_velocity()") == 0)
        return article != NULL && snprintf(output, capacity,
            "engine->vector_mirror(&item->x40_vel, &item->xC58);") < capacity;
    if (strncmp(line, "item.spawn_effect_on_bone(", 26) == 0) {
        unsigned effect, bone, mode = 0;
        int consumed = 0;
        int fields = sscanf(line,
                            "item.spawn_effect_on_bone(%u, %u, %u)%n",
                            &effect, &bone, &mode, &consumed);
        if (fields != 3 || line[consumed] != '\0') {
            consumed = 0;
            mode = 0;
            fields = sscanf(line, "item.spawn_effect_on_bone(%u, %u)%n",
                            &effect, &bone, &consumed);
        }
        if (article == NULL ||
            fields < 2 || line[consumed] != '\0' ||
            effect > 0xFFFF || bone >= 100 || mode > 1) return false;
        return snprintf(output, capacity,
                        "engine->item_spawn_effect_on_bone(gobj, %u, %u, %u);",
                        effect, bone, mode) < capacity;
    }
    if (strcmp(line, "owner.clear_damage_death()") == 0)
        return article != NULL &&
            snprintf(output, capacity,
                     "{ Fighter* owner = scripted_article_attached_owner(item); if (owner != NULL) { owner->take_dmg_cb = 0; owner->death2_cb = 0; } }")
                < capacity;
    if (article && (strncmp(line, "item.clone_model(", 17) == 0 ||
                    strncmp(line, "item.destroy_model(", 19) == 0)) {
        unsigned model; int used = 0;
        bool clone = strncmp(line, "item.clone_model(", 17) == 0;
        if (sscanf(line, clone ? "item.clone_model(%u)%n" : "item.destroy_model(%u)%n",
                   &model, &used) != 1 || !used || line[used] || model < 1 || model > 3)
            return false;
        return snprintf(output, capacity, "engine->item_%s_model(gobj, %u);",
                        clone ? "clone" : "destroy", model) < capacity;
    }
    if (article && strcmp(line, "item.cancel_hitlag_request()") == 0)
        return snprintf(output, capacity, "item->xCA8 = 0;") < capacity;
    if (article && strcmp(line, "item.disable_surface_response()") == 0)
        return snprintf(output, capacity, "item->xD5C = 0;") < capacity;
    if (article && strcmp(line, "item.inherit_owner_scale()") == 0)
        return snprintf(output, capacity, "engine->item_inherit_owner_scale(gobj);") < capacity;
    if (article && strcmp(line, "item.use_manual_lifetime()") == 0)
        return snprintf(output, capacity, "engine->item_use_manual_lifetime(gobj);") < capacity;
    if (article && strcmp(line, "item.respond_to_stage(1)") == 0)
        return snprintf(output, capacity, "engine->item_stage_contact_response(gobj, scripted_article_%u_logic_%u);", article->index, ARTICLE_STAGE_CONTACT) < capacity;
    if (article && strcmp(line, "item.respond_to_stage()") == 0)
        return snprintf(output, capacity, "engine->item_stage_response(gobj, scripted_article_%u_logic_%u);", article->index, ARTICLE_STAGE_CONTACT) < capacity;
    if (article && strcmp(line, "item.follow_ground()") == 0)
        return snprintf(output, capacity, "engine->item_follow_ground(gobj, scripted_article_%u_logic_%u);", article->index, ARTICLE_STAGE_LEFT) < capacity;
    if (article && strcmp(line, "item.bounce_to_ground()") == 0)
        return snprintf(output, capacity, "engine->item_bounce_to_ground(gobj, scripted_article_%u_logic_%u);", article->index, ARTICLE_STAGE_LANDED) < capacity;
    if (article && strcmp(line, "item.bounce_to_ground(1)") == 0)
        return snprintf(output, capacity, "engine->item_bounce_stage_to_ground(gobj, scripted_article_%u_logic_%u);", article->index, ARTICLE_STAGE_LANDED) < capacity;
    if (article && strncmp(line, "item.wind_at_position(", 22) == 0) {
        char args[4][192], values[4][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 4, &after) || *after) return false;
        for (unsigned i = 0; i < 4; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity, "engine->spawn_wind(&item->pos, %s, %s, %s, %s);", values[0], values[1], values[2], values[3]) < capacity;
    }
    if (article && (strncmp(line, "item.set_collision_facing(", 26) == 0 ||
                    strncmp(line, "item.set_animation_descriptor(", 30) == 0)) {
        bool facing = strncmp(line, "item.set_collision_facing(", 26) == 0;
        char args[1][192], value[384]; const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(line, '('), args, 1, &after) || *after ||
            !nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return false;
        return snprintf(output, capacity, facing
            ? "engine->set_collision_facing(&item->x378_itemColl, (int)(%s));"
            : "engine->item_set_animation_descriptor(gobj, (unsigned)(%s));", value) < capacity;
    }
    if (article && strcmp(line, "item.slide_on_ground()") == 0)
        return snprintf(output, capacity, "engine->item_slide_on_ground(gobj, scripted_article_%u_logic_%u, scripted_article_%u_logic_%u);", article->index, ARTICLE_STAGE_LANDED, article->index, ARTICLE_STAGE_LEFT) < capacity;
    if (article && strncmp(line, "item.set_model_animation_rate(", 30) == 0) {
        char args[1][192], value[384]; const char* after;
        if (nativeScriptCode_ParseArguments(strchr(line, '('), args, 1, &after) && !*after) {
            if (!nativeFighterItemScript_Expression(args[0], value, sizeof(value), article, parameters)) return false;
            return snprintf(output, capacity, "engine->model_animation_rate(gobj->hsd_obj, %s);", value) < capacity;
        }
    }
    if (article && strcmp(line, "item.sweep_floor()") == 0)
        return snprintf(output, capacity, "engine->item_sweep_floor(gobj);") < capacity;
    if (article && strcmp(line, "item.clear_accessory()") == 0)
        return snprintf(output, capacity, "item->on_accessory = NULL;") < capacity;
    if (article && strcmp(line, "item.track_accessory()") == 0)
        return snprintf(output, capacity, "item->on_accessory = scripted_article_%u_logic_%u;", article->index, ARTICLE_ACCESSORY) < capacity;
    if (article && strcmp(line, "item.detach_owner()") == 0)
        return snprintf(output, capacity, "item->owner = NULL; item->xDD4_itemVar.native_article.attachment_owner = NULL; item->xDD4_itemVar.native_article.creator_spawn_id = 0;") < capacity;
    if (article && strcmp(line, "item.remove()") == 0)
        return snprintf(output, capacity, "engine->item_remove(gobj); return%s;",
                        article->expression_callback && article->expression_callback->helper &&
                        article->expression_callback->result_type != ScriptType_Invalid ?
                            (article->expression_callback->result_type == ScriptType_Float ? " 0.0f" :
                             article->expression_callback->result_type == ScriptType_Bool ? " false" : " 0") :
                        callback == ITEM_GROUND || callback == ITEM_HIT ? " false" : "") < capacity;
    if (article && strncmp(line, "item.spawn_directional_effect(", 30) == 0) {
        char args[6][192], values[6][256];
        const char* after;
        if (!nativeScriptCode_ParseArguments(line + 29, args, 6, &after) || *after) return false;
        const char* bone = nativeScriptCode_BoneSymbol(args[1], strlen(args[1]));
        if (bone == NULL) return false;
        for (unsigned i = 0; i < 6; ++i)
            if (i != 1 && !nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "engine->item_spawn_directional_effect(gobj, (s32)(%s), %s, &(Vec3){ %s, %s, %s }, %s);",
            values[0], bone, values[2], values[3], values[4], values[5]) < capacity;
    }
    if (strncmp(line, "item.change_state(", 18) == 0) {
        char args[1][192], translated[384];
        const char* after;
        if (article == NULL || !nativeScriptCode_ParseArguments(line + 17, args, 1, &after) || *after)
            return false;
        char* end;
        long literal = strtol(args[0], &end, 10);
        if (*end == '\0' && (literal < 0 || literal >= article->state_count)) return false;
        if (!nativeFighterItemScript_Expression(args[0], translated, sizeof(translated), article, parameters)) return false;
        return snprintf(output, capacity,
            "{ int32_t next_state = (int32_t)(%s); if ((uint32_t)next_state < %u) engine->item_change_state(gobj, next_state); }",
            translated, article->state_count) < capacity;
    }
    if (strncmp(line, "item.set_lifetime(", 18) == 0) {
        const char* end = strrchr(line, ')');
        char source[192], translated[512];
        if (end == NULL || end[1] != '\0' ||
            end - line <= 18 || end - line - 18 >= sizeof(source))
            return false;
        memcpy(source, line + 18, (size_t) (end - line - 18));
        source[end - line - 18] = '\0';
        if (!nativeFighterItemScript_Expression(source, translated, sizeof(translated), article,
                             parameters))
            return false;
        return snprintf(output, capacity,
                        "engine->item_set_lifetime(gobj, %s);", translated)
            < capacity;
    }
    if (article && strcmp(line, "item.gravity(false)") == 0)
        return snprintf(output, capacity, "engine->item_fall(gobj, item->xCC_item_attr->x10_fall_speed, item->xCC_item_attr->x14_fall_speed_max);") < capacity;
    if (article && strncmp(line, "item.loop_sound(", 16) == 0) {
        char args[3][192], values[3][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 15, args, 3, &after) || *after) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "{ int32_t sound = %s, pan = %s, volume = %s; "
            "if ((uint32_t)sound <= 0xFFFFFF && (uint32_t)pan <= 255 && (uint32_t)volume <= 255) "
            "engine->item_loop_sound(item, sound, pan, volume); }",
            values[0], values[1], values[2]) < capacity;
    }
    if (strncmp(line, "item.play_sound(", 16) == 0) {
        char args[3][192], values[3][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(line + 15, args, 3, &after) || *after) return false;
        for (unsigned i = 0; i < 3; ++i)
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return false;
        return snprintf(output, capacity,
            "{ int32_t sound = %s, pan = %s, volume = %s; "
            "if ((uint32_t)sound <= 0xFFFFFF && (uint32_t)pan <= 255 && (uint32_t)volume <= 255) "
            "engine->item_play_sound(gobj, sound, pan, volume); }",
            values[0], values[1], values[2]) < capacity;
    }
    static const struct {
        const char* prefix;
        const char* host;
    } visual_calls[] = {
        { "item.set_scale(", "item_set_uniform_scale" },
        { "item.scale_by_attribute(", "item_scale_by_attribute" },
        { "item.child_visible(", "item_set_child_visible" },
    };
    for (unsigned i = 0; i < sizeof(visual_calls) / sizeof(visual_calls[0]);
         ++i) {
        size_t prefix = strlen(visual_calls[i].prefix);
        if (strncmp(line, visual_calls[i].prefix, prefix) != 0) continue;
        const char* end = strrchr(line, ')');
        char source[192], translated[512];
        if (end == NULL || end[1] != '\0') return false;
        size_t end_offset = (size_t) (end - line);
        if (end_offset <= prefix || end_offset - prefix >= sizeof(source))
            return false;
        memcpy(source, line + prefix, end_offset - prefix);
        source[end_offset - prefix] = '\0';
        if (!nativeFighterItemScript_Expression(source, translated, sizeof(translated), article,
                             parameters))
            return false;
        return snprintf(output, capacity, "engine->%s(gobj, %s);",
                        visual_calls[i].host, translated) < capacity;
    }
    if (nativeItemOperations_Action(line, article, parameters, output, capacity)) return true;
    if (article) {
        int resource = nativeArticleResources_Action(line, article, parameters,
            &article->resources, &article->model_roots, &article->model_animations,
            &article->local_state, output, capacity);
        if (resource != 0) return resource > 0;
    }
    if (article && nativeArticleOperations_Action(line, article, parameters,
            &article->local_state, &article->model_roots, &article->model_animations, article->index,
            ARTICLE_STAGE_LANDED, output, capacity)) return true;
    const char* assign = strstr(line, " = ");
    if (assign == NULL || assign - line >= 96) return false;
    char target[96], translated_target[160], translated[ITEM_LINE_BYTES - 128];
    memcpy(target, line, (size_t) (assign - line));
    target[assign - line] = '\0';
    if (article) {
        int resource = nativeArticleResources_Assignment(target, assign + 3, article,
            parameters, &article->resources, output, capacity);
        if (resource != 0) return resource > 0;
    }
    if (strstr(assign + 3, "targets.call(") &&
        nativeFighterItemScript_ExpressionType(target, article, parameters) != ScriptType_Bool) return false;
    if (article != NULL && strcmp(target, "item.grounded") == 0) {
        if (!nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated), article, parameters)) return false;
        return snprintf(output, capacity, "if (%s) engine->item_set_grounded(item); else engine->item_set_airborne(item);", translated) < capacity;
    }
    if (article != NULL && strcmp(target, "item.airborne") == 0) {
        if (!nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated), article, parameters)) return false;
        return snprintf(output, capacity, "item->ground_or_air = (%s) ? GA_Air : GA_Ground;", translated) < capacity;
    }
    if (article != NULL && strncmp(target, "item.dynamic_joint(", 19) == 0) {
        char index[192], value[384]; unsigned channel; const char* after;
        if (!nativeItemOperations_DynamicJoint(target, index, &channel, &after) || *after ||
            !nativeFighterItemScript_Expression(index, value, sizeof(value), article, parameters) ||
            !nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated), article, parameters)) return false;
        return snprintf(output, capacity, "engine->item_set_dynamic_joint_value(gobj, (unsigned)(%s), %u, %s);", value, channel, translated) < capacity;
    }
    if (article != NULL && nativeItemOperations_Property(target, true) != NULL) {
        if (!nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated), article, parameters)) return false;
        return snprintf(output, capacity,
            nativeFighterItemScript_ExpressionType(target, article, parameters) == ScriptType_Bool ? "%s = (%s) != 0;" : "%s = %s;",
            nativeItemOperations_Property(target, true), translated) < capacity;
    }
    if (article != NULL && strncmp(target, "item.joint(", 11) == 0) {
        char path[40];
        unsigned model, channel;
        const char* after;
        if (!parse_joint_property(target, path, &model, &channel, &after) ||
            *after != '\0' || channel == 9 ||
            !nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated),
                             article, parameters)) return false;
        return snprintf(output, capacity,
                        "engine->item_set_joint_value(gobj, %u, \"%s\", %u, %s);",
                        model, path, channel, translated) < capacity;
    }
    if (article != NULL && (strncmp(target, "item.state.", 11) == 0 ||
                            strncmp(target, "owner.state.", 12) == 0 || strncmp(target, "creator.state.", 14) == 0)) {
        bool owner = strncmp(target, "owner.state.", 12) == 0;
        bool creator = strncmp(target, "creator.state.", 14) == 0;
        NativeScriptStateReference reference;
        int found = nativeScriptState_Reference((owner || creator) ? article->fighter_state :
            &article->local_state, target, creator ? "creator.state." : owner ? "owner.state." : "item.state.", &reference);
        if (found < 0 || *reference.after != '\0') return false;
        if (owner || creator) {
            char index[1024] = "0";
            if (!nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated), article, parameters) ||
                (reference.field->array && !nativeFighterItemScript_Expression(reference.index, index, sizeof(index), article, parameters))) return false;
            if (nativeScriptLifetime_LineMayInvalidate(translated, article->functions, ScriptCallback_Item) ||
                nativeScriptLifetime_LineMayInvalidate(index, article->functions, ScriptCallback_Item))
                return nativeScriptStore_External(reference.field, creator, index, translated, output, capacity);
        }
        if (reference.field->array) {
            char index[1024];
            const char* bytes = creator ?
                "(scripted_article_creator(item) != NULL ? scripted_article_creator(item)->native_module_state + script_state_base : NULL)" : owner ?
                "(scripted_article_attached_owner(item) != NULL ? scripted_article_attached_owner(item)->native_module_state + script_state_base : NULL)" :
                "item->xDD4_itemVar.native_article.state";
            return nativeFighterItemScript_Expression(reference.index, index, sizeof(index), article, parameters) &&
                nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated), article, parameters) &&
                nativeScriptState_ArrayWrite(reference.field, bytes, index, translated, output, capacity);
        }
    }
    if (article != NULL && strncmp(target, "creator.state.", 14) == 0) {
        const ScriptStateField* field = nativeFighterScript_StateField(article->fighter_state, target + 14);
        if (field == NULL || field->array || !nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated), article, parameters)) return false;
        if (field->type == ScriptState_Bool)
            return snprintf(output, capacity, "{ Fighter* creator = scripted_article_creator(item); if (creator != NULL) creator->native_module_state[script_state_base + %u] = (uint8_t)((%s) != 0); }", field->offset, translated) < capacity;
        return snprintf(output, capacity, "{ Fighter* creator = scripted_article_creator(item); if (creator != NULL) script_state_set_%s(creator, script_state_base + %u, %s); }", field->type == ScriptState_Float ? "float" : "int", field->offset, translated) < capacity;
    }
    if (article != NULL && strncmp(target, "item.state.", 11) == 0) {
        const ScriptStateField* field = nativeFighterScript_StateField(
            &article->local_state, target + 11);
        if (field == NULL || field->array ||
            !nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated),
                             article, parameters)) return false;
        const char* type = field->type == ScriptState_Float
            ? "float" : field->type == ScriptState_Int ? "int" : "bool";
        return snprintf(output, capacity,
                        "scripted_article_set_state_%s(item, %u, %s);",
                        type, field->offset, translated) < capacity;
    }
    if (article != NULL && strncmp(target, "owner.state.", 12) == 0) {
        const ScriptStateField* field = nativeFighterScript_StateField(
            article->fighter_state, target + 12);
        if (field == NULL || field->array ||
            !nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated),
                             article, parameters)) return false;
        const char* type = field->type == ScriptState_Float
            ? "float" : field->type == ScriptState_Int ? "int" : "bool";
        return snprintf(output, capacity,
                        "scripted_article_set_owner_state_%s(item, %u, %s);",
                        type, field->offset, translated) < capacity;
    }
    if (strncmp(target, "velocity.", 9) != 0 && strncmp(target, "slide_velocity.", 15) != 0 &&
        strncmp(target, "item.state", 10) != 0 &&
        !(article != NULL &&
          (strcmp(target, "ecb.up") == 0 || strcmp(target, "ecb.down") == 0 ||
           strcmp(target, "ecb.front") == 0 || strcmp(target, "ecb.back") == 0 ||
           strcmp(target, "item.life") == 0 ||
           strcmp(target, "item.animation_rate") == 0 ||
           strcmp(target, "item.spin_speed") == 0 ||
           strcmp(target, "position.x") == 0 ||
           strcmp(target, "position.y") == 0 ||
           strcmp(target, "position.z") == 0 ||
           strcmp(target, "facing") == 0 ||
           strcmp(target, "item.collision_mode") == 0 ||
           strcmp(target, "item.exit_category") == 0 ||
           strcmp(target, "item.damage_hitlag") == 0 ||
           strcmp(target, "item.contact_hitlag") == 0 ||
           strcmp(target, "item.throw_speed_damage") == 0 ||
           strcmp(target, "item.physics_disabled") == 0 ||
           strcmp(target, "item.custom_floor_motion") == 0 ||
           strcmp(target, "item.suppress_destruction_effects") == 0 ||
           strcmp(target, "item.world_interaction") == 0 ||
           strcmp(target, "item.owner_bound") == 0 ||
           strcmp(target, "item.surface_normal.x") == 0 ||
           strcmp(target, "item.surface_normal.y") == 0 ||
           strcmp(target, "item.surface_normal.z") == 0 ||
           strcmp(target, "item.remove_at_bounds") == 0 ||
           (strncmp(target, "item.command", 12) == 0 &&
            target[12] >= '0' && target[12] <= '3' &&
            target[13] == '\0'))))
        return false;
    if (!nativeFighterItemScript_Expression(target, translated_target,
                         sizeof(translated_target), article, parameters) ||
        !nativeFighterItemScript_Expression(assign + 3, translated, sizeof(translated), article,
                         parameters))
        return false;
    return snprintf(output, capacity, "%s = %s;", translated_target,
                    translated) < capacity;
}

static bool article_callback_is_predicate(unsigned callback)
{
    return callback == ARTICLE_DAMAGE_DEALT ||
           callback == ARTICLE_DAMAGE_RECEIVED ||
           callback == ARTICLE_REFLECTED || callback == ARTICLE_CLANKED ||
           callback == ARTICLE_ABSORBED ||
           callback == ARTICLE_SHIELD_BOUNCED ||
           callback == ARTICLE_HIT_SHIELD || callback == ARTICLE_HELD_REQUEST;
}

// Article fields share the project parameter store, with a qualified scope.
static int article_parameter(const ArticleDeclaration* article,
                              const NativeFighterParameters* parameters,
                              const char* name)
{
    if (parameters == NULL) return -1;
    char qualified[128];
    int length = snprintf(qualified, sizeof(qualified), "%s.%s", article->kind, name);
    if (length <= 0 || length >= sizeof(qualified)) return -1;
    for (unsigned i = 0; i < parameters->count; ++i)
        if (strcmp(qualified, parameters->fields[i].name) == 0) return (int) i;
    return -1;
}

static bool parse_article_line(ArticleDeclaration* article, const char* line,
                               int* current_state, ItemCallback** active,
                               bool* predicate,
                               const NativeFighterParameters* parameters)
{
    const char* content = line;
    while (*content == ' ') ++content;
    if (*content == '\0' || *content == '#') return true;
    if (content - line < 12 && !nativeItemStatements_Finish(*active)) return false;
    if (content - line < 12) article->expression_callback = NULL;
    if (strncmp(line, "    table float ", 16) == 0) {
        if (*current_state >= 0 || *active != NULL ||
            article->table_count == SCRIPT_ITEM_TABLE_LIMIT) return false;
        ScriptItemTable* table = &article->tables[article->table_count];
        if (!nativeScriptItemTable_Parse(table, line)) return false;
        for (unsigned i = 0; i < article->table_count; ++i)
            if (strcmp(article->tables[i].name, table->name) == 0) return false;
        ++article->table_count;
        return true;
    }
    if (strncmp(line, "    state bool ", 15) == 0 ||
        strncmp(line, "    state float ", 16) == 0 ||
        strncmp(line, "    state int ", 14) == 0) {
        // The catalog already validates and owns the complete field layout.
        return *current_state < 0 && *active == NULL;
    }
    if (strncmp(line, "    attribute ", 14) == 0) {
        char name[48], extra;
        char type[8];
        unsigned offset;
        if (*current_state >= 0 || *active != NULL ||
            article->attribute_count >= ARTICLE_ATTRIBUTE_LIMIT ||
            sscanf(line, "    attribute %7s %47s %u %c", type, name, &offset,
                   &extra) != 3 || (strcmp(type, "float") && strcmp(type, "int")) || !identifier(name) ||
            offset > 1020 || (offset & 3) != 0)
            return false;
        for (unsigned i = 0; i < article->attribute_count; ++i)
            if (strcmp(article->attributes[i].name, name) == 0 ||
                article->attributes[i].offset == offset) return false;
        strcpy(article->attributes[article->attribute_count].name, name);
        article->attributes[article->attribute_count].integer = strcmp(type, "int") == 0;
        article->attributes[article->attribute_count++].offset = offset;
        return true;
    }
    if (strncmp(line, "    resources ", 14) == 0)
        return *current_state < 0 && *active == NULL &&
               nativeArticleResources_Declare(line, &article->resources);
    if (strncmp(line, "    model_animation ", 20) == 0)
        return *current_state < 0 && *active == NULL &&
               nativeArticleOperations_DeclareModel(line, "model_animation", &article->model_animations);
    if (strncmp(line, "    model_root ", 15) == 0)
        return *current_state < 0 && *active == NULL &&
               nativeArticleOperations_DeclareModel(line, "model_root", &article->model_roots);
    if (strcmp(line, "    render standard") == 0) {
        if (*current_state >= 0 || *active != NULL || article->standard_render) return false;
        article->standard_render = true;
        return true;
    }
    if (strncmp(line, "    attachment ", 15) == 0) {
        char origin[16], previous[16] = "origin", plane[8], collision[8], held[8];
        int used = 0;
        int fields = sscanf(line,
            "    attachment origin %15s previous %15s plane %7s initial_collision %7s held %7s%n",
            origin, previous, plane, collision, held, &used);
        if (fields != 5) {
            strcpy(previous, "origin"); used = 0;
            fields = sscanf(line,
                "    attachment origin %15s plane %7s initial_collision %7s held %7s%n",
                origin, plane, collision, held, &used);
            if (fields != 4) return false;
        }
        if (*current_state >= 0 || *active != NULL || article->attachment_declared ||
            !used || line[used] || (strcmp(origin, "owner") && strcmp(origin, "bone") && strcmp(origin, "center")) ||
            (strcmp(previous, "origin") && strcmp(previous, "bone")) ||
            (strcmp(plane, "on") && strcmp(plane, "off")) ||
            (strcmp(collision, "on") && strcmp(collision, "off")) ||
            (strcmp(held, "on") && strcmp(held, "off"))) return false;
        article->attachment_declared = true;
        article->attachment = (NativeFighterAttachmentSpawn) {
            .origin_at_owner = strcmp(origin, "owner") == 0,
            .origin_at_center = strcmp(origin, "center") == 0,
            .flatten_z = strcmp(plane, "on") == 0,
            .initial_collision = strcmp(collision, "on") == 0,
            .register_as_held = strcmp(held, "on") == 0,
            .previous_at_bone = strcmp(previous, "bone") == 0
        };
        return true;
    }
    if (strncmp(line, "    projectile appearance ", 26) == 0) {
        int appearance, consumed = 0;
        unsigned state;
        char names[3][48], lifetime_mode[32];
        int fields = sscanf(line,
                   "    projectile appearance %d state %u speed %47s angle %47s lifetime %47s%n",
                   &appearance, &state, names[0], names[1], names[2], &consumed);
        if (*current_state >= 0 || *active != NULL ||
            article->projectile_declared || fields != 5 ||
            appearance < -1 || appearance > 1023 || state >= article->state_count)
            return false;
        article->projectile_auto_lifetime = true;
        article->projectile_initial_velocity = true;
        article->projectile_initial_update = true;
        unsigned seen = 0;
        const char* options = line + consumed;
        while (*options) {
            char option[32];
            consumed = 0;
            if (sscanf(options, " %31s %31s%n", option, lifetime_mode, &consumed) != 2 ||
                (strcmp(lifetime_mode, "on") != 0 && strcmp(lifetime_mode, "off") != 0))
                return false;
            bool enabled = strcmp(lifetime_mode, "on") == 0;
            unsigned bit;
            if (strcmp(option, "auto_lifetime") == 0) {
                bit = 1; article->projectile_auto_lifetime = enabled;
            } else if (strcmp(option, "initial_velocity") == 0) {
                bit = 2; article->projectile_initial_velocity = enabled;
            } else if (strcmp(option, "initial_update") == 0) {
                bit = 4; article->projectile_initial_update = enabled;
            } else return false;
            if (seen & bit) return false;
            seen |= bit;
            options += consumed;
        }
        for (unsigned field = 0; field < 3; ++field) {
            int parameter = article_parameter(article, parameters, names[field]);
            if (parameter >= 0) {
                if (parameters->fields[parameter].type != NativeFighterParameter_Float)
                    return false;
                snprintf(article->projectile_values[field], 192,
                         "script_parameters[%u].f", parameter);
                continue;
            }
            bool found = false;
            for (unsigned i = 0; i < article->attribute_count; ++i)
                if (strcmp(names[field], article->attributes[i].name) == 0) {
                    if (article->attributes[i].integer) return false;
                    snprintf(article->projectile_values[field], 192,
                        "((const float*)source->x4_specialAttributes)[%u]",
                        article->attributes[i].offset / 4);
                    found = true;
                    break;
                }
            if (!found) {
                char* end;
                float literal = strtof(names[field], &end);
                if (end == names[field] || *end != '\0' || !isfinite(literal))
                    return false;
                snprintf(article->projectile_values[field], 192, "%.9gf", literal);
                // C requires a decimal point/exponent for the float suffix.
                if (!strchr(article->projectile_values[field], '.') &&
                    !strchr(article->projectile_values[field], 'e'))
                    snprintf(article->projectile_values[field], 192, "%.1ff", literal);
            }
        }
        article->projectile_appearance = appearance;
        article->projectile_state = state;
        article->projectile_declared = true;
        return true;
    }
    if (strncmp(line, "    state ", 10) == 0) {
        unsigned state = 0;
        int animation, consumed = 0;
        unsigned callback_state;
        int fields = sscanf(line, "    state %u anim %d use %u:%n", &state,
                            &animation, &callback_state, &consumed);
        bool shared = fields == 3 && consumed != 0;
        if (!shared) {
            consumed = 0;
            fields = sscanf(line, "    state %u anim %d:%n", &state, &animation, &consumed);
            callback_state = state;
        }
        if (fields != (shared ? 3 : 2) || consumed == 0 ||
            line[consumed] != '\0' || state >= article->state_count ||
            (shared && (callback_state >= article->state_count || !article->states[callback_state].declared)) ||
            animation < -1 || animation > 1023 ||
            article->states[state].declared) return false;
        article->states[state].declared = true;
        article->states[state].animation = animation;
        article->states[state].callback_state = shared
            ? article->states[callback_state].callback_state : state;
        article->in_signal = false;
        *current_state = (int) state;
        *active = NULL;
        return true;
    }
    static const char* state_names[] = {
        "animation", "physics", "collision"
    };
    if (strncmp(line, "        on ", 11) == 0) {
        if (*current_state < 0 ||
            article->states[*current_state].callback_state != (unsigned)*current_state)
            return false;
        for (unsigned i = 0; i < ARTICLE_STATE_CALLBACKS; ++i) {
            char expected[48];
            snprintf(expected, sizeof(expected), "        on %s:",
                     state_names[i]);
            if (strcmp(line, expected) != 0) continue;
            *active = &article->states[*current_state].callbacks[i];
            *predicate = i != ARTICLE_PHYSICS;
            return (*active)->count == 0;
        }
        return false;
    }
    static const char* logic_names[] = {
        "spawned", "destroyed", "picked_up", "dropped", "thrown",
        "damage_dealt", "damage_received", "entered_air", "reflected",
        "clanked", "absorbed", "shield_bounced", "hit_shield",
        "interaction", "launched", "accessory", "created", "stage_contact", "stage_landed", "stage_left", "out_of_bounds", "held_status", "held_request", "holder_neutral"
    };
    if (strncmp(line, "    on signal ", 14) == 0) {
        char name[48];
        int consumed = 0;
        if (article->signal_count == NATIVE_FIGHTER_ARTICLE_SIGNAL_LIMIT ||
            sscanf(line + 14, "%47[a-zA-Z0-9_]:%n", name, &consumed) != 1 ||
            consumed == 0 || line[14 + consumed] || !identifier(name)) return false;
        uint32_t hash = nativeScriptSignal_Hash(name);
        for (unsigned i = 0; i < article->signal_count; ++i)
            if (article->signals[i].hash == hash) return false;
        unsigned signal = article->signal_count++;
        strcpy(article->signals[signal].name, name);
        article->signals[signal].hash = hash;
        *active = &article->signals[signal].callback;
        *predicate = false;
        *current_state = -1;
        article->in_signal = true;
        return true;
    }
    if (strncmp(line, "    on ", 7) == 0) {
        article->in_signal = false;
        for (unsigned i = 0; i < ARTICLE_LOGIC_CALLBACKS; ++i) {
            char expected[48];
            snprintf(expected, sizeof(expected), "    on %s:",
                     logic_names[i]);
            if (strcmp(line, expected) != 0) continue;
            *current_state = -1;
            *active = &article->callbacks[i];
            *predicate = article_callback_is_predicate(i);
            (*active)->result_type = i == ARTICLE_HELD_STATUS ? ScriptType_Int :
                i == ARTICLE_HELD_REQUEST ? ScriptType_Bool : ScriptType_Invalid;
            return (*active)->count == 0;
        }
        return false;
    }
    if (*active != NULL && content - line >= 12) {
        bool valid = nativeItemStatements_Add(*active, content, (unsigned)(content - line),
                                 *predicate, article,
                                 *active == &article->callbacks[ARTICLE_INTERACTION],
                                 parameters);
        article->expression_callback = NULL;
        return valid;
    }
    return false;
}

static bool callback_uses(const ItemCallback* callback, const char* call)
{
    for (unsigned i = 0; i < callback->count; ++i)
        if (strstr(callback->lines[i], call) != NULL ||
            (strcmp(call, "engine->item_creator(") == 0 &&
             (strstr(callback->lines[i], "scripted_article_creator(") ||
              strstr(callback->lines[i], "scripted_article_attached_owner(") ||
              strstr(callback->lines[i], "scripted_article_owner_") ||
              strstr(callback->lines[i], "scripted_article_set_owner_")))) return true;
    return false;
}

static bool article_uses(const ArticleDeclaration* article, const char* call)
{
    for (unsigned c = 0; c < ARTICLE_LOGIC_CALLBACKS; ++c)
        if (callback_uses(&article->callbacks[c], call)) return true;
    for (unsigned s = 0; s < article->signal_count; ++s)
        if (callback_uses(&article->signals[s].callback, call)) return true;
    for (unsigned s = 0; s < article->state_count; ++s)
        for (unsigned c = 0; c < ARTICLE_STATE_CALLBACKS; ++c)
            if (callback_uses(&article->states[s].callbacks[c], call)) return true;
    if (article->item_functions)
        for (unsigned i = 0; i < article->item_functions->count; ++i)
            if (callback_uses(&article->item_functions->functions[i].body, call)) return true;
    return false;
}

NativeFighterItemProgram* nativeFighterItemScript_Parse(
    const char* source, size_t size, char** fighter_source,
    unsigned fighter_reserved_state_bytes,
    const NativeFighterParameters* parameters, const ScriptFunctionProgram* functions,
    NativeFighterScriptError* error)
{
    NativeFighterItemProgram* program = calloc(1, sizeof(*program));
    char* stripped = malloc(size + 1);
    if (program == NULL || stripped == NULL) {
        free(program); free(stripped);
        error_at(error, 0, "Out of memory parsing items.");
        return NULL;
    }
    memcpy(stripped, source, size);
    stripped[size] = '\0';
    if (!nativeScriptArticleRefs_CatalogParse(&program->catalog, source, size, error)) goto invalid;
    program->fighter_state.bytes = fighter_reserved_state_bytes;
    unsigned state_line = 0;
    for (size_t start = 0; start < size;) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        ++state_line;
        size_t length = end - start;
        if (length && source[start + length - 1] == '\r') --length;
        if (length >= ITEM_LINE_BYTES) {
            error_at(error, state_line, "Item script line is too long.");
            goto invalid;
        }
        if (length >= 6 && memcmp(source + start, "state ", 6) == 0) {
            char line[ITEM_LINE_BYTES];
            memcpy(line, source + start, length);
            line[length] = '\0';
            while (length && line[length - 1] == ' ') line[--length] = '\0';
            if (!nativeFighterScript_AddStateField(&program->fighter_state,
                                                   line)) {
                error_at(error, state_line,
                         "Expected a unique fighter state field.");
                goto invalid;
            }
        }
        start = end < size ? end + 1 : end;
    }
    bool in_item = false;
    bool in_article = false;
    int article_state = -1;
    ItemCallback* article_callback = NULL;
    bool article_predicate = false;
    int callback = -1;
    unsigned line_number = 0;
    for (size_t start = 0; start < size;) {
        size_t line_start = start;
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        ++line_number;
        size_t length = end - start;
        if (length && source[start + length - 1] == '\r') --length;
        if (length >= ITEM_LINE_BYTES) {
            error_at(error, line_number, "Item script line is too long.");
            goto invalid;
        }
        char line[ITEM_LINE_BYTES];
        memcpy(line, source + start, length);
        line[length] = '\0';
        while (length && line[length - 1] == ' ') line[--length] = '\0';
        start = end < size ? end + 1 : end;
        if (in_article && line[0] != ' ' && line[0] != '\0' && line[0] != '#' &&
            !nativeItemStatements_Finish(article_callback)) {
            error_at(error, line_number, "Article block needs an indented body.");
            goto invalid;
        }
        if (strncmp(line, "article ", 8) == 0) {
            char kind[64];
            unsigned state_count;
            int consumed = 0;
            if (program->article_count == ARTICLE_LIMIT ||
                sscanf(line, "article %63s states %u:%n", kind,
                       &state_count, &consumed) != 2 ||
                line[consumed] != '\0' ||
                strncmp(kind, "It_Kind_", 8) != 0 ||
                !identifier(kind) || state_count == 0 ||
                state_count > ARTICLE_STATE_LIMIT) {
                error_at(error, line_number,
                         "Expected `article It_Kind_Name states N:`.");
                goto invalid;
            }
            for (unsigned i = 0; i < program->article_count; ++i)
                if (strcmp(program->articles[i].kind, kind) == 0) {
                    error_at(error, line_number,
                             "Article kind is declared twice.");
                    goto invalid;
                }
            ArticleDeclaration* article =
                &program->articles[program->article_count++];
            article->catalog = &program->catalog;
            article->local_state = nativeScriptArticleRefs_Declaration(&program->catalog, kind)->state;
            strcpy(article->kind, kind);
            article->index = program->article_count - 1;
            article->functions = functions;
            article->fighter_state = &program->fighter_state;
            article->state_count = state_count;
            if (!nativeItemFunctions_Parse(article, source, size, start, line_number, error)) goto invalid;
            in_article = true;
            in_item = false;
            article_state = -1;
            article_callback = NULL;
        } else if (strncmp(line, "item ", 5) == 0) {
            char name[48]; unsigned damage, angle, lifetime;
            int consumed = 0;
            if (program->count == ITEM_LIMIT ||
                sscanf(line, "item %47s damage %u angle %u lifetime %u:%n",
                       name, &damage, &angle, &lifetime, &consumed) != 4 ||
                line[consumed] != '\0' || !identifier(name) ||
                damage > 999 || angle > 361 || lifetime > 65535) {
                error_at(error, line_number,
                         "Expected `item Name damage N angle N lifetime N:`.");
                goto invalid;
            }
            ItemDeclaration* item = &program->items[program->count++];
            strcpy(item->name, name);
            item->damage = damage;
            item->angle = angle;
            item->lifetime = lifetime;
            item->billboard_size = 8.0f;
            item->hitbox_size = 4.0f;
            item->knockback_growth = 70;
            item->base_knockback = 25;
            in_item = true;
            in_article = false;
            callback = -1;
        } else if (in_item && line[0] != ' ' && line[0] != '\0' &&
                   line[0] != '#') {
            in_item = false;
        }
        if (in_article && line[0] != ' ' && line[0] != '\0' &&
            line[0] != '#' && strncmp(line, "article ", 8) != 0)
            in_article = false;
        if (in_article) {
            ArticleDeclaration* current = &program->articles[program->article_count - 1];
            bool helper_declaration = false;
            if (nativeItemFunctions_Contains(current, line_start, &helper_declaration)) {
                if (helper_declaration) {
                    if (!nativeItemStatements_Finish(article_callback)) {
                        error_at(error, line_number, "Article callback needs a complete body."); goto invalid;
                    }
                    article_callback = NULL; article_state = -1;
                }
                for (size_t i = line_start; i < end; ++i) stripped[i] = ' ';
                continue;
            }
            if (strncmp(line, "article ", 8) != 0 &&
                !parse_article_line(
                    &program->articles[program->article_count - 1], line,
                    &article_state, &article_callback,
                    &article_predicate, parameters)) {
                error_at(error, line_number, article_callback && article_callback->result_type == ScriptType_Int
                    ? "held_status is a read-only query returning Int."
                    : "Invalid article state, callback, statement, return type or event context.");
                goto invalid;
            }
            for (size_t i = line_start; i < end; ++i) stripped[i] = ' ';
            continue;
        }
        if (in_item) {
            if (strncmp(line, "    icon ", 9) == 0) {
                const char* path = line + 9;
                if (callback >= 0 ||
                    program->items[program->count - 1].icon[0] != '\0' ||
                    strlen(path) >= sizeof(program->items[0].icon) || path[0] == '\0' ||
                    path[0] == '/' || path[0] == '\\' ||
                    strstr(path, "..") != NULL || strchr(path, ':') != NULL ||
                    strpbrk(path, "\"' ") != NULL) {
                    error_at(error, line_number, "Expected a project-relative item icon PNG path.");
                    goto invalid;
                }
                strcpy(program->items[program->count - 1].icon, path);
            } else if (strncmp(line, "    billboard_size ", 19) == 0) {
                float size;
                char extra;
                if (callback >= 0 || sscanf(line + 19, "%f %c", &size, &extra) != 1 ||
                    !(size >= 1.0f && size <= 64.0f)) {
                    error_at(error, line_number, "Billboard size must be between 1 and 64.");
                    goto invalid;
                }
                program->items[program->count - 1].billboard_size = size;
            } else if (strncmp(line, "    hitbox_size ", 16) == 0) {
                float size;
                char extra;
                if (callback >= 0 || sscanf(line + 16, "%f %c", &size, &extra) != 1 ||
                    !(size >= 0.5f && size <= 64.0f)) {
                    error_at(error, line_number, "Hitbox size must be between 0.5 and 64.");
                    goto invalid;
                }
                program->items[program->count - 1].hitbox_size = size;
            } else if (strncmp(line, "    knockback_growth ", 21) == 0 ||
                       strncmp(line, "    base_knockback ", 19) == 0) {
                bool growth = line[4] == 'k';
                unsigned value;
                char extra;
                const char* input = line + (growth ? 21 : 19);
                if (callback >= 0 || sscanf(input, "%u %c", &value, &extra) != 1 ||
                    value > 999) {
                    error_at(error, line_number, "Knockback value must be between 0 and 999.");
                    goto invalid;
                }
                ItemDeclaration* item = &program->items[program->count - 1];
                if (growth) item->knockback_growth = value;
                else item->base_knockback = value;
            } else if (strncmp(line, "    on ", 7) == 0) {
                const char* names[] = { "thrown", "tick", "ground_collision",
                                        "hit" };
                callback = -1;
                for (unsigned i = 0; i < ITEM_CALLBACKS; ++i) {
                    char expected[40];
                    snprintf(expected, sizeof(expected), "    on %s:", names[i]);
                    if (strcmp(line, expected) == 0) callback = (int) i;
                }
                if (callback < 0) {
                    error_at(error, line_number, "Unknown item callback.");
                    goto invalid;
                }
            } else if (length > 8 &&
                       strncmp(line, "        ", 8) == 0 &&
                       line[8] != ' ') {
                if (callback < 0) {
                    error_at(error, line_number, "Item statement needs a callback.");
                    goto invalid;
                }
                ItemCallback* cb = &program->items[program->count - 1]
                                        .callbacks[callback];
                if (cb->count == CALLBACK_LIMIT ||
                    !nativeFighterItemScript_Action(line + 8, (unsigned) callback, NULL, false,
                                 parameters,
                                 cb->lines[cb->count], ITEM_LINE_BYTES)) {
                    error_at(error, line_number, "Invalid item statement.");
                    goto invalid;
                }
                ++cb->count;
            } else if (line[0] != '\0' && line[0] != '#' &&
                       strncmp(line, "item ", 5) != 0) {
                error_at(error, line_number, "Invalid item indentation.");
                goto invalid;
            }
            // Strip bytes while preserving the original newline layout.
            for (size_t i = line_start; i < end; ++i) stripped[i] = ' ';
        }
    }
    if (!nativeItemStatements_Finish(article_callback)) {
        error_at(error, line_number, "Article block needs an indented body.");
        goto invalid;
    }
    for (unsigned i = 0; i < program->article_count; ++i)
        if (!nativeItemFunctions_Compile(&program->articles[i], parameters, error)) goto invalid;
    for (unsigned i = 0; i < program->article_count; ++i) {
        ArticleDeclaration* article = &program->articles[i];
        if ((article->callbacks[ARTICLE_HELD_STATUS].count != 0) !=
            (article->callbacks[ARTICLE_HELD_REQUEST].count != 0)) {
            error_at(error, line_number, "Authored held_status and held_request must be declared together.");
            goto invalid;
        }
        char assignment[80];
        snprintf(assignment, sizeof(assignment), "item->on_accessory = scripted_article_%u_logic_%u;", i, ARTICLE_ACCESSORY);
        if (article_uses(article, assignment) && !article->callbacks[ARTICLE_ACCESSORY].count) {
            error_at(error, line_number, "Tracking an accessory requires an accessory callback.");
            goto invalid;
        }
        static const struct { const char* call; unsigned callback; const char* message; } dependencies[] = {
            { "engine->item_stage_response(", ARTICLE_STAGE_CONTACT, "Stage response requires an on stage_contact callback." },
            { "engine->item_stage_contact_response(", ARTICLE_STAGE_CONTACT, "Stage response requires an on stage_contact callback." },
            { "engine->item_follow_ground(", ARTICLE_STAGE_LEFT, "Ground following requires an on stage_left callback." },
            { "engine->item_bounce_to_ground(", ARTICLE_STAGE_LANDED, "Air bouncing requires an on stage_landed callback." },
            { "engine->item_land_on_stage(", ARTICLE_STAGE_LANDED, "Stage landing requires an on stage_landed callback." },
            { "engine->item_bounce_stage_to_ground(", ARTICLE_STAGE_LANDED, "Stage bouncing requires an on stage_landed callback." },
            { "engine->item_slide_on_ground(", ARTICLE_STAGE_LANDED, "Ground sliding requires an on stage_landed callback." },
            { "engine->item_slide_on_ground(", ARTICLE_STAGE_LEFT, "Ground sliding requires an on stage_left callback." },
        };
        for (unsigned d = 0; d < sizeof(dependencies) / sizeof(dependencies[0]); ++d)
            if (article_uses(article, dependencies[d].call) && !article->callbacks[dependencies[d].callback].count) {
                error_at(error, line_number, dependencies[d].message);
                goto invalid;
            }
        for (unsigned signal = 0; signal < program->articles[i].signal_count; ++signal)
            if (program->articles[i].signals[signal].callback.count == 0) {
                error_at(error, line_number, "An article signal needs a callback body.");
                goto invalid;
            }
        if (program->articles[i].callbacks[ARTICLE_LAUNCHED].count != 0 &&
            !program->articles[i].projectile_declared) {
            error_at(error, line_number, "A launched callback requires a projectile declaration.");
            goto invalid;
        }
    }
    for (unsigned i = 0; i < program->article_count; ++i)
        for (unsigned j = 0; j < program->articles[i].state_count; ++j)
            if (!program->articles[i].states[j].declared) {
                error_at(error, line_number,
                         "Every article state needs an animation index.");
                goto invalid;
            }
    *fighter_source = stripped;
    return program;
invalid:
    free(stripped);
    nativeFighterItemScript_Free(program);
    return NULL;
}

bool nativeFighterItemScript_Emit(const NativeFighterItemProgram* program,
                                  const char* asset_root, FILE* output,
                                  NativeFighterScriptError* error)
{
    if (program->count == 0 && program->article_count == 0) return true;
    fputs("#include <melee/it/types.h>\n"
          "#include <melee/it/kinds/types.h>\n", output);
    for (unsigned i = 0; i < program->count; ++i) {
        const ItemDeclaration* item = &program->items[i];
        if (item->icon[0] == '\0' || asset_root == NULL) continue;
        char path[512];
        int n = snprintf(path, sizeof(path), "%s/%s", asset_root, item->icon);
        FILE* png = n > 0 && n < sizeof(path) ? fopen(path, "rb") : NULL;
        if (png == NULL || fseek(png, 0, SEEK_END) != 0) {
            if (png != NULL) fclose(png);
            return error_at(error, 0, "Could not read item icon PNG from project.");
        }
        long size = ftell(png);
        if (size < 24 || size > 262144 || fseek(png, 0, SEEK_SET) != 0) {
            fclose(png);
            return error_at(error, 0, "Item icon PNG must be at most 256 KiB.");
        }
        unsigned char* bytes = malloc((size_t) size);
        if (bytes == NULL || fread(bytes, 1, (size_t) size, png) != (size_t) size) {
            free(bytes); fclose(png);
            return error_at(error, 0, "Could not read item icon PNG bytes.");
        }
        fclose(png);
        const unsigned char signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
        unsigned width = (unsigned) bytes[16] << 24 | (unsigned) bytes[17] << 16 |
                         (unsigned) bytes[18] << 8 | bytes[19];
        unsigned height = (unsigned) bytes[20] << 24 | (unsigned) bytes[21] << 16 |
                          (unsigned) bytes[22] << 8 | bytes[23];
        if (memcmp(bytes, signature, 8) != 0 || width < 1 || width > 256 ||
            height < 1 || height > 256) {
            free(bytes);
            return error_at(error, 0, "Item icon must be a PNG no larger than 256x256.");
        }
        fprintf(output, "static const unsigned char scripted_item_icon_%u[] = {", i);
        for (long j = 0; j < size; ++j) {
            if ((j & 15) == 0) fputs("\n    ", output);
            fprintf(output, "%u,", bytes[j]);
        }
        fputs("\n};\n", output);
        free(bytes);
    }
    for (unsigned i = 0; i < program->count; ++i)
        for (unsigned callback = 0; callback < ITEM_CALLBACKS; ++callback) {
            const ItemCallback* cb = &program->items[i].callbacks[callback];
            if (cb->count == 0) continue;
            bool predicate = callback == ITEM_GROUND || callback == ITEM_HIT;
            fprintf(output,
                "static %s scripted_item_%u_%u(HSD_GObj* gobj) {\n"
                "    Item* item = gobj->user_data;\n",
                predicate ? "bool" : "void", i, callback);
            nativeItemStatements_Emit(output, cb, NULL, predicate);
            if (predicate) fputs("    return false;\n", output);
            fputs("}\n", output);
        }
    if (program->count != 0)
        fputs("static const NativeItemDefinition scripted_items[] = {\n",
              output);
    for (unsigned i = 0; i < program->count; ++i) {
        const ItemDeclaration* item = &program->items[i];
        fprintf(output, "    { \"%s\", %u, %u, %u, ", item->name,
                item->damage, item->angle, item->lifetime);
        for (unsigned callback = 0; callback < ITEM_CALLBACKS; ++callback)
            if (item->callbacks[callback].count)
                fprintf(output, "scripted_item_%u_%u, ", i, callback);
            else fputs("0, ", output);
        if (item->icon[0] != '\0' && asset_root != NULL)
            fprintf(output, "scripted_item_icon_%u, sizeof(scripted_item_icon_%u), %.4ff, ",
                    i, i, item->billboard_size);
        else
            fputs("0, 0, 0, ", output);
        fprintf(output, "%.4ff, %u, %u },\n", item->hitbox_size,
                item->knockback_growth, item->base_knockback);
    }
    if (program->count != 0) fputs("};\n", output);
    static const char* logic_fields[ARTICLE_LOGIC_CALLBACKS] = {
        "spawned", "destroyed", "picked_up", "dropped", "thrown",
        "dmg_dealt", "dmg_received", "entered_air", "reflected",
        "clanked", "absorbed", "shield_bounced", "hit_shield",
        "evt_unk", NULL, NULL, NULL, NULL
    };
    if (program->article_count != 0)
        fputs("static inline int32_t scripted_article_state_int(Item* item, unsigned offset) {\n"
              "    int32_t value; memcpy(&value, item->xDD4_itemVar.native_article.state + offset, 4); return value;\n"
              "}\n"
              "static inline float scripted_article_state_float(Item* item, unsigned offset) {\n"
              "    float value; memcpy(&value, item->xDD4_itemVar.native_article.state + offset, 4); return value;\n"
              "}\n"
              "static inline bool scripted_article_state_bool(Item* item, unsigned offset) {\n"
              "    return item->xDD4_itemVar.native_article.state[offset] != 0;\n"
              "}\n"
              "static inline void scripted_article_set_state_int(Item* item, unsigned offset, int32_t value) {\n"
              "    memcpy(item->xDD4_itemVar.native_article.state + offset, &value, 4);\n"
              "}\n"
              "static inline void scripted_article_set_state_float(Item* item, unsigned offset, float value) {\n"
              "    memcpy(item->xDD4_itemVar.native_article.state + offset, &value, 4);\n"
              "}\n"
              "static inline void scripted_article_set_state_bool(Item* item, unsigned offset, bool value) {\n"
              "    item->xDD4_itemVar.native_article.state[offset] = (uint8_t) value;\n"
              "}\n"
              "static inline Fighter* scripted_article_creator(Item* item) {\n"
              "    HSD_GObj* creator = engine->item_creator(item->entity);\n"
              "    return creator != NULL ? creator->user_data : NULL;\n"
              "}\n"
              "static inline Fighter* scripted_article_attached_owner(Item* item) {\n"
              "    HSD_GObj* owner = engine->item_creator(item->entity);\n"
              "    return owner != NULL && item->owner == owner ? owner->user_data : NULL;\n"
              "}\n"
              "static inline int scripted_article_owner_motion(Item* item) {\n"
              "    Fighter* owner = scripted_article_attached_owner(item);\n"
              "    return owner != NULL ? owner->motion_id : -1;\n"
              "}\n"
              "static inline int32_t scripted_article_owner_state_int(Item* item, unsigned offset) {\n"
              "    Fighter* owner = scripted_article_attached_owner(item);\n"
              "    int32_t value = 0;\n"
              "    if (owner != NULL) memcpy(&value, owner->native_module_state + script_state_base + offset, 4);\n"
              "    return value;\n"
              "}\n"
              "static inline float scripted_article_owner_state_float(Item* item, unsigned offset) {\n"
              "    Fighter* owner = scripted_article_attached_owner(item);\n"
              "    float value = 0;\n"
              "    if (owner != NULL) memcpy(&value, owner->native_module_state + script_state_base + offset, 4);\n"
              "    return value;\n"
              "}\n"
              "static inline bool scripted_article_owner_state_bool(Item* item, unsigned offset) {\n"
              "    Fighter* owner = scripted_article_attached_owner(item);\n"
              "    return owner != NULL && owner->native_module_state[script_state_base + offset] != 0;\n"
              "}\n"
              "static inline void scripted_article_set_owner_state_int(Item* item, unsigned offset, int32_t value) {\n"
              "    Fighter* owner = scripted_article_attached_owner(item);\n"
              "    if (owner != NULL) memcpy(owner->native_module_state + script_state_base + offset, &value, 4);\n"
              "}\n"
              "static inline void scripted_article_set_owner_state_float(Item* item, unsigned offset, float value) {\n"
              "    Fighter* owner = scripted_article_attached_owner(item);\n"
              "    if (owner != NULL) memcpy(owner->native_module_state + script_state_base + offset, &value, 4);\n"
              "}\n"
              "static inline void scripted_article_set_owner_state_bool(Item* item, unsigned offset, bool value) {\n"
              "    Fighter* owner = scripted_article_attached_owner(item);\n"
              "    if (owner != NULL) owner->native_module_state[script_state_base + offset] = (uint8_t) value;\n"
              "}\n"
              "static inline int scripted_article_owner_command(Item* item, unsigned slot) {\n"
              "    Fighter* owner = scripted_article_attached_owner(item);\n"
              "    return owner != NULL ? owner->cmd_vars[slot] : 0;\n"
              "}\n"
              "static inline Fighter* scripted_article_holder(Item* item) {\n"
              "    HSD_GObj* holder = item->owner;\n"
              "    return holder != NULL && holder->classifier == HSD_GOBJ_CLASS_FIGHTER ? holder->user_data : NULL;\n"
              "}\n", output);
    for (unsigned article_index = 0;
         article_index < program->article_count; ++article_index) {
        const ArticleDeclaration* article =
            &program->articles[article_index];
        if (!nativeArticleOperations_EmitModelTable(&article->model_roots,
                                                   article_index, false, output) ||
            !nativeArticleOperations_EmitModelTable(&article->model_animations,
                                                   article_index, true, output))
            return false;
        for (unsigned callback = ARTICLE_STAGE_CONTACT; callback < ARTICLE_HELD_STATUS; ++callback)
            if (article->callbacks[callback].count)
                fprintf(output, "static void scripted_article_%u_logic_%u(HSD_GObj*);\n", article_index, callback);
        if (article->callbacks[ARTICLE_ACCESSORY].count)
            fprintf(output, "static void scripted_article_%u_logic_%u(HSD_GObj*);\n", article_index, ARTICLE_ACCESSORY);
        if (article->attachment_declared)
            fprintf(output, "static const NativeFighterAttachmentSpawn scripted_article_%u_attachment = { .origin_at_owner = %s, .origin_at_center = %s, .flatten_z = %s, .initial_collision = %s, .register_as_held = %s, .previous_at_bone = %s };\n",
                    article_index, article->attachment.origin_at_owner ? "true" : "false",
                    article->attachment.origin_at_center ? "true" : "false",
                    article->attachment.flatten_z ? "true" : "false",
                    article->attachment.initial_collision ? "true" : "false",
                    article->attachment.register_as_held ? "true" : "false",
                    article->attachment.previous_at_bone ? "true" : "false");
        for (unsigned table = 0; table < article->table_count; ++table)
            nativeScriptItemTable_Emit(&article->tables[table], article_index, table, output);
        for (unsigned signal = 0; signal < article->signal_count; ++signal)
            fprintf(output, "static void scripted_article_%u_signal_%u(HSD_GObj*, float);\n", article_index, signal);
        nativeItemFunctions_Emit(article, output);
        for (unsigned signal = 0; signal < article->signal_count; ++signal) {
            const ItemCallback* body = &article->signals[signal].callback;
            fprintf(output,
                "static void scripted_article_%u_signal_%u(HSD_GObj* gobj, float signal_value) {\n"
                "    Item* item = gobj->user_data;\n", article_index, signal);
            nativeItemStatements_Emit(output, body, article->functions, false);
            fputs("}\n", output);
        }
        if (article->signal_count) {
            fprintf(output, "static const NativeFighterArticleSignal scripted_article_%u_signals[] = {\n", article_index);
            for (unsigned signal = 0; signal < article->signal_count; ++signal)
                fprintf(output, "    { UINT32_C(%u), scripted_article_%u_signal_%u },\n",
                        article->signals[signal].hash, article_index, signal);
            fputs("};\n", output);
        }
        if (article->projectile_declared) {
            bool launched = article->callbacks[ARTICLE_LAUNCHED].count != 0;
            if (launched)
                fprintf(output, "static void scripted_article_%u_logic_%u(HSD_GObj*);\n",
                        article_index, ARTICLE_LAUNCHED);
            fprintf(output,
                    "static void scripted_article_%u_values(const Article* source, float values[3]) {\n"
                    "    (void)source; values[0] = %s; values[1] = %s; values[2] = %s;\n}\n"
                    "static const NativeFighterProjectile scripted_article_%u_projectile = {\n"
                    "    .appearance_state = %d, .item_state = %u,\n"
                    "    .read_values = scripted_article_%u_values,\n"
                    "    .auto_lifetime = %s, .initial_velocity = %s, .initial_update = %s,\n",
                    article_index, article->projectile_values[0],
                    article->projectile_values[1], article->projectile_values[2],
                    article_index, article->projectile_appearance,
                    article->projectile_state, article_index,
                    article->projectile_auto_lifetime ? "true" : "false",
                    article->projectile_initial_velocity ? "true" : "false",
                    article->projectile_initial_update ? "true" : "false");
            if (launched)
                fprintf(output, "    .on_launched = scripted_article_%u_logic_%u,\n",
                        article_index, ARTICLE_LAUNCHED);
            fputs("};\n", output);
        }
        for (unsigned state = 0; state < article->state_count; ++state)
            for (unsigned callback = 0;
                 callback < ARTICLE_STATE_CALLBACKS; ++callback) {
                const ItemCallback* body =
                    &article->states[state].callbacks[callback];
                if (body->count == 0) continue;
                bool predicate = callback != ARTICLE_PHYSICS;
                fprintf(output,
                    "static %s scripted_article_%u_state_%u_%u(HSD_GObj* gobj) {\n"
                    "    Item* item = gobj->user_data;\n",
                    predicate ? "bool" : "void", article_index,
                    state, callback);
                nativeItemStatements_Emit(output, body, article->functions, predicate);
                if (predicate) fputs("    return false;\n", output);
                fputs("}\n", output);
            }
        for (unsigned callback = 0;
             callback < ARTICLE_LOGIC_CALLBACKS; ++callback) {
            const ItemCallback* body = &article->callbacks[callback];
            if (body->count == 0) continue;
            bool predicate = article_callback_is_predicate(callback);
            fprintf(output,
                "static %s scripted_article_%u_logic_%u(HSD_GObj* gobj%s) {\n"
                "    Item* item = gobj->user_data;\n",
                callback == ARTICLE_HELD_STATUS ? "int32_t" : predicate ? "bool" : "void", article_index,
                callback, callback == ARTICLE_INTERACTION ? ", HSD_GObj* other" :
                    callback == ARTICLE_HELD_REQUEST ? ", int32_t held_action, float held_frame_rate, float held_duration" : "");
            nativeItemStatements_Emit(output, body, article->functions, predicate);
            if (callback == ARTICLE_HELD_STATUS) fputs("    return -1;\n", output);
            else if (predicate) fputs("    return false;\n", output);
            fputs("}\n", output);
        }
        fprintf(output,
                "static ItemStateTable scripted_article_%u_states[%u] = {\n",
                article_index, article->state_count);
        for (unsigned state = 0; state < article->state_count; ++state) {
            fprintf(output, "    { %d, ",
                    article->states[state].animation);
            for (unsigned callback = 0;
                 callback < ARTICLE_STATE_CALLBACKS; ++callback)
                if (article->states[article->states[state].callback_state].callbacks[callback].count)
                    fprintf(output, "scripted_article_%u_state_%u_%u, ",
                            article_index, article->states[state].callback_state, callback);
                else fputs("0, ", output);
            fputs("},\n", output);
        }
        fputs("};\n", output);
        fprintf(output,
                "static ItemLogicTable scripted_article_%u_logic = {\n"
                "    .states = scripted_article_%u_states,\n",
                article_index, article_index);
        for (unsigned callback = 0;
             callback < ARTICLE_LOGIC_CALLBACKS; ++callback)
            if (logic_fields[callback] != NULL && article->callbacks[callback].count)
                fprintf(output,
                        "    .%s = scripted_article_%u_logic_%u,\n",
                        logic_fields[callback], article_index, callback);
        fputs("};\n", output);
    }
    for (unsigned i = 0; i < program->article_count; ++i) {
        char symbol[64]; snprintf(symbol, sizeof(symbol), "scripted_article_%u_state_fields", i);
        if (!nativeScriptState_EmitMetadata(output, &program->articles[i].local_state, symbol, true)) return false;
    }
    if (program->article_count != 0) {
        fputs("static const NativeFighterArticleLogic "
              "scripted_article_logics[] = {\n", output);
        for (unsigned i = 0; i < program->article_count; ++i) {
            fprintf(output, "    { %s, &scripted_article_%u_logic, ",
                    program->articles[i].kind, i);
            if (program->articles[i].projectile_declared)
                fprintf(output, "&scripted_article_%u_projectile", i);
            else fputs("NULL", output);
            fprintf(output, ", %u, %u, %u, ", program->articles[i].state_count,
                    program->articles[i].local_state.bytes, program->articles[i].local_state.count);
            if (program->articles[i].local_state.count)
                fprintf(output, "scripted_article_%u_state_fields", i);
            else fputs("NULL", output);
            fprintf(output, ", %u, ", program->articles[i].signal_count);
            if (program->articles[i].signal_count)
                fprintf(output, "scripted_article_%u_signals", i);
            else fputs("NULL", output);
            if (program->articles[i].attachment_declared)
                fprintf(output, ", &scripted_article_%u_attachment", i);
            else fputs(", NULL", output);
            fprintf(output, ", %s, ", program->articles[i].standard_render ? "true" : "false");
            if (program->articles[i].callbacks[ARTICLE_CREATED].count)
                fprintf(output, "scripted_article_%u_logic_%u", i, ARTICLE_CREATED);
            else fputs("NULL", output);
            fputs(", ", output);
            if (program->articles[i].callbacks[ARTICLE_OUT_OF_BOUNDS].count)
                fprintf(output, "scripted_article_%u_logic_%u", i, ARTICLE_OUT_OF_BOUNDS);
            else fputs("NULL", output);
            fprintf(output, ", %u, ", program->articles[i].model_roots.count);
            if (program->articles[i].model_roots.count)
                fprintf(output, "scripted_article_%u_model_roots", i);
            else fputs("NULL", output);
            fprintf(output, ", %u, ", program->articles[i].model_animations.count);
            if (program->articles[i].model_animations.count)
                fprintf(output, "scripted_article_%u_model_animations", i);
            else fputs("NULL", output);
            fprintf(output, ", %u, %d", program->articles[i].resources.capacity,
                program->articles[i].resources.declared ?
                    program->articles[i].resources.reserved_part : -1);
            for (unsigned callback = ARTICLE_HELD_STATUS; callback <= ARTICLE_HOLDER_NEUTRAL; ++callback) {
                fputs(", ", output);
                if (program->articles[i].callbacks[callback].count)
                    fprintf(output, "scripted_article_%u_logic_%u", i, callback);
                else fputs("NULL", output);
            }
            fputs(" },\n", output);
        }
        fputs("};\n", output);
    }
    return !ferror(output);
}

unsigned nativeFighterItemScript_Count(const NativeFighterItemProgram* program)
{
    return program->count;
}

unsigned nativeFighterItemScript_ArticleCount(
    const NativeFighterItemProgram* program)
{
    return program->article_count;
}

bool nativeFighterItemScript_HostChecks(const NativeFighterItemProgram* program,
                                        char* output, size_t capacity)
{
    if (capacity == 0) return false;
    output[0] = '\0';
    size_t used = 0;
    static const char* fields[] = {
        "target_call", "target_call_bool", "target_call_int", "target_call_float", "callback_source_alive",
        "target_held_id", "target_held_kind", "target_held_motion", "target_held_frame", "item_current_owner",
        "item_set_appearance", "item_reset_spin", "item_refresh_physics",
        "item_pause", "item_resume",
        "item_fall", "item_loop_sound",
        "item_gravity", "item_spawn_food", "item_land", "item_bounce",
        "item_change_state", "item_set_lifetime", "item_reflect",
        "item_stage_collision", "item_bounce_stage",
        "item_shield_bounce", "item_play_sound",
        "item_spawn_effect_on_bone", "item_activate_attached",
        "fighter_model_scale", "item_set_uniform_scale",
        "fighter_root_scale_y",
        "fighter_incapacitated",
        "item_set_child_visible",
        "item_interact",
        "item_joint_value", "item_set_joint_value",
        "item_dynamic_joint_value", "item_set_dynamic_joint_value",
        "item_set_animation_descriptor", "item_add_animation_descriptor", "item_read_attachment_matrix_rotation", "item_sweep_stage_flags",
        "item_wall_contact_flags", "item_ceiling_contact_flags", "item_align_to_floor", "item_reflect_from_wall",
        "item_spawn_async_joint_effect",
        "item_reset_default_ecb", "item_set_grounded", "item_set_airborne", "item_bounce_stage_to_ground",
        "set_collision_facing", "spawn_wind",
        "sin_float", "cos_float", "abs_float", "sqrt_float",
        "min_float", "max_float", "angle_atan2_float", "wrap_angle",
        "item_sweep_stage", "vector_mirror",
        "item_remove", "item_advance_animation", "item_scale_by_attribute",
        "item_set_instance_scale", "item_respond_stage_mask",
        "item_spawn_directional_effect", "destroy_effects",
        "item_clone_model", "item_destroy_model",
        "item_load_model", "item_set_joint_transform",
        "item_add_model_animation", "item_request_model_animation",
        "item_advance_model_animation", "item_remove_model_animation",
        "item_set_model_animation_rate", "item_read_joint_matrix_rotation",
        "item_joint_world_position", "spawn_wind",
        "set_item_diffuse_color", "target_controller_color", "set_item_layered",
        "stage_segment_intersection", "play_global_sound",
        "item_segment_sweep", "stage_line_exists", "stage_line_island_bounds", "set_collision_line",
        "stage_line_speed", "stage_line_normal", "item_set_animation_frame",
        "item_set_hitbox_damage", "item_spawn_effect", "item_spawn_joint_effect_at",
        "item_contact_fighter", "target_is_fighter", "target_guard_active",
        "item_creator", "item_current_owner", "item_reflector", "target_camera_target_position",
        "create_item_at_position", "item_hits_stage", "item_land_on_stage",
        "target_model_scale", "target_instance_scale_y", "target_motion", "target_airborne", "target_shield",
        "item_inherit_owner_scale", "item_use_manual_lifetime",
        "vector_normalize", "vector_angle", "vector_cross_normalized", "stage_internal_kind", "item_release_camera", "wrap_signed_angle",
        "item_disable_hitboxes", "item_disable_hitbox", "spawn_effect", "random_float", "random_int",
        "item_spawn_link", "item_unlink", "item_link_reciprocal", "item_link_value",
        "item_bone_value", "item_link_bone_value", "item_copy_link_collision",
        "item_signal_link", "item_change_state_flags", "item_link_id", "item_create_child",
        "article_ref_exists", "article_ref_bool", "article_ref_int", "article_ref_float",
        "article_ref_current_owner", "target_smash_charge_active",
        "article_ref_primary_translation",
        "target_position", "target_position_delta", "target_damage", "target_facing",
        "target_sleeping", "target_floor_line", "target_cpu_body_half_width",
        "item_set_debug_owner",
        "item_check_ground", "camera_quake", "rumble",
        "signal_article", "item_rumble", "item_stop_loop_sound",
        "article_resource_begin", "article_resource_create", "article_resource_load_model",
        "article_resource_set_draw", "article_resource_count", "article_resource_exists",
        "article_resource_bind_external_part", "article_resource_set_capture_anchor", "article_resource_release",
        "article_resource_read_joint", "article_resource_write_joint", "article_resource_set_world_matrix",
        "article_resource_read_float", "article_resource_write_float", "article_resource_read_int", "article_resource_write_int",
        "article_resource_sweep_stage", "article_resource_sweep_point", "article_resource_follow_floor", "article_resource_read_line_speed",
        "article_resource_read_attachment_point", "article_resource_read_attachment_matrix", "article_resource_read_world_matrix",
        "article_resource_add_animation", "article_resource_request_animation", "article_resource_advance_animation",
        "article_resource_remove_animation", "article_resource_set_animation_rate",
        "article_resource_spawn_effect_at_position", "article_resource_spawn_effect_facing_at_position",
        "item_show_model", "item_hide_model",
        "item_attach_to_owner", "bone_count", "item_release_attachment", "item_spawn_joint_effect",
        "item_stage_collision_land", "item_stage_response", "item_begin_expiry", "item_expire_tick",
        "item_stop_motion", "item_find_fighter", "item_find_target", "item_center_position",
        "vector_normalize_xy", "vector_rotate", "vector_angle_xy",
        "item_owner_overlap", "item_follow_ground", "item_bounce_to_ground", "item_slide_on_ground",
        "item_align_facing", "model_animation_rate", "model_advance_animation",
        "model_animation_active", "item_sweep_floor", "item_stage_contact_response", "spawn_async_effect",
    };
    for (unsigned field = 0; field < sizeof(fields) / sizeof(fields[0]);
         ++field) {
        char call[64];
        snprintf(call, sizeof(call), "engine->%s(", fields[field]);
        bool needed = false;
        bool lifetime = strcmp(fields[field], "callback_source_alive") == 0;
        for (unsigned item = 0; item < program->count && !needed; ++item)
            for (unsigned callback = 0; callback < ITEM_CALLBACKS && !needed;
                 ++callback)
                needed = lifetime ? nativeItemStatements_MayInvalidate(&program->items[item].callbacks[callback], NULL)
                    : callback_uses(&program->items[item].callbacks[callback], call);
        for (unsigned article = 0;
             article < program->article_count && !needed; ++article) {
            const ArticleDeclaration* declaration = &program->articles[article];
            for (unsigned callback = 0;
                 callback < ARTICLE_LOGIC_CALLBACKS && !needed; ++callback)
                needed = lifetime ? nativeItemStatements_MayInvalidate(&declaration->callbacks[callback], declaration->functions)
                    : callback_uses(&declaration->callbacks[callback], call);
            for (unsigned signal = 0; signal < declaration->signal_count && !needed; ++signal)
                needed = lifetime ? nativeItemStatements_MayInvalidate(&declaration->signals[signal].callback, declaration->functions)
                    : callback_uses(&declaration->signals[signal].callback, call);
            for (unsigned state = 0;
                 state < declaration->state_count && !needed; ++state)
                for (unsigned callback = 0;
                     callback < ARTICLE_STATE_CALLBACKS && !needed; ++callback)
                    needed = lifetime ? nativeItemStatements_MayInvalidate(&declaration->states[state].callbacks[callback], declaration->functions)
                        : callback_uses(&declaration->states[state].callbacks[callback], call);
            if (declaration->item_functions)
                for (unsigned helper = 0;
                     helper < declaration->item_functions->count && !needed; ++helper) {
                    const ItemCallback* body = &declaration->item_functions->functions[helper].body;
                    needed = lifetime ? nativeItemStatements_MayInvalidate(body, declaration->functions)
                        : callback_uses(body, call);
                }
        }
        if (!needed) continue;
        int written = snprintf(output + used, capacity - used,
                               " || host->%s == 0", fields[field]);
        if (written < 0 || (size_t) written >= capacity - used) return false;
        used += (size_t) written;
    }
    return true;
}

void nativeFighterItemScript_Free(NativeFighterItemProgram* program)
{
    if (!program) return;
    for (unsigned i = 0; i < program->article_count; ++i) free(program->articles[i].item_functions);
    free(program);
}

const NativeScriptArticleCatalog* nativeFighterItemScript_ArticleCatalog(
    const NativeFighterItemProgram* program)
{
    return &program->catalog;
}

unsigned nativeFighterItemScript_StateArrayTypes(const NativeFighterItemProgram* program)
{
    unsigned mask = 0;
    for (unsigned i = 0; i < program->article_count; ++i)
        mask |= nativeScriptState_ArrayTypes(&program->articles[i].local_state);
    return mask;
}
