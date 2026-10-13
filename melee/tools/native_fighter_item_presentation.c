#include "native_fighter_item_presentation.h"
#include "native_fighter_item_program.h"
#include "native_fighter_script_signal.h"
#include <string.h>

int nativeItemPresentation_Action(const char* source, const ArticleDeclaration* article,
                                  const NativeFighterParameters* parameters,
                                  char* output, size_t capacity)
{
    if (strcmp(source, "item.release_camera()") == 0)
        return snprintf(output, capacity, "engine->item_release_camera(gobj);") < capacity ? 1 : -1;
    if (strcmp(source, "item.stop_loop_sound()") == 0)
        return snprintf(output, capacity, "engine->item_stop_loop_sound(item);") < capacity ? 1 : -1;
    bool quake = strncmp(source, "camera.quake(", 13) == 0;
    bool rumble = strncmp(source, "item.rumble(", 12) == 0;
    bool signal = strncmp(source, "articles.signal(", 16) == 0;
    if (!quake && !rumble && !signal) return 0;
    unsigned count = quake ? 4 : rumble ? 2 : 4;
    char args[4][192], values[4][384]; const char* after;
    if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, count, &after) || *after)
        return -1;
    uint32_t hash = 0;
    const NativeScriptArticleDeclaration* declaration = NULL;
    for (unsigned i = 0; i < count; ++i) {
        if(signal && i==1) {
            declaration=nativeScriptArticleRefs_Declaration(article?article->catalog:NULL,args[i]);
            if(!declaration)return -1;
            continue;
        }
        if (signal && i == 2) {
            size_t length = strlen(args[i]);
            if (length < 3 || args[i][0] != '"' || args[i][length - 1] != '"') return -1;
            args[i][length - 1] = 0;
            if (!nativeScriptCode_Identifier(args[i] + 1, length - 2, 48)) return -1;
            hash = nativeScriptSignal_Hash(args[i] + 1);
            continue;
        }
        if ((i == 0 || rumble) &&
            nativeFighterItemScript_ExpressionType(args[i], article, parameters) != ScriptType_Int)
            return -1;
        if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters))
            return -1;
    }
    int length = quake ? snprintf(output, capacity,
        "engine->camera_quake((CmQuakeKind)(%s), &(Vec3){ %s, %s, %s });",
        values[0], values[1], values[2], values[3]) : rumble ? snprintf(output, capacity,
        "engine->item_rumble(gobj, (s32)(%s), (s32)(%s));", values[0], values[1]) :
        snprintf(output, capacity,
        "{ uint32_t signal_handle=(uint32_t)(%s); float signal_value=(float)(%s); "
        "if (engine->article_ref_exists(gobj,signal_handle,%s)) "
        "engine->signal_article(gobj,signal_handle,UINT32_C(%u),signal_value); }",
        values[0], values[3], declaration?declaration->kind:"", hash);
    return length >= 0 && (size_t)length < capacity ? 1 : -1;
}
