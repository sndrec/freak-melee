#include "native_fighter_script_colors.h"
#include <stdio.h>
#include <string.h>

bool nativeScriptColors_ParseVisibility(char* source, size_t size,
    uint16_t* mask, NativeFighterScriptError* error)
{
    *mask = 0;
    bool code_seen = false;
    unsigned line = 0;
    for (size_t start = 0; start < size;) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        size_t length = end - start;
        ++line;
        while (length && (source[start + length - 1] == '\r' ||
            source[start + length - 1] == ' ' || source[start + length - 1] == '\t')) --length;
        const char* text = source + start;
        if (length >= 17 && !memcmp(text, "visibility_asset ", 17)) {
            char declaration[64]; unsigned slot; int consumed = 0;
            if (length >= sizeof(declaration))
                return nativeScriptCode_Fail(error, line, "Visibility asset declaration is too long.");
            memcpy(declaration, text, length); declaration[length] = 0;
            if (code_seen || sscanf(declaration, "visibility_asset %u%n", &slot, &consumed) != 1 ||
                declaration[consumed] || slot >= 16)
                return nativeScriptCode_Fail(error, line,
                    "Expected `visibility_asset SLOT` (0..15) before code blocks.");
            uint16_t bit = (uint16_t)(1u << slot);
            if (*mask & bit)
                return nativeScriptCode_Fail(error, line, "Visibility asset slot is already declared.");
            *mask |= bit;
            memset(source + start, ' ', end - start);
        } else {
            code_seen |= (length >= 5 && !memcmp(text,"move ",5)) ||
                (length >= 5 && !memcmp(text,"func ",5)) ||
                (length >= 12 && !memcmp(text,"export func ",12)) ||
                (length >= 7 && !memcmp(text,"action ",7)) ||
                (length >= 8 && !memcmp(text,"article ",8)) ||
                (length >= 5 && !memcmp(text,"item ",5)) ||
                (length >= 7 && !memcmp(text,"effect ",7));
        }
        start = end < size ? end + 1 : end;
    }
    return true;
}

void nativeScriptColors_EmitVisibility(uint16_t mask, FILE* output)
{
    if (mask) fprintf(output,"    generated.asset_layout.visibility_mask |= %uu;\n",mask);
}

#define RGBA_READ(field) "((int32_t)(((uint32_t)" field ".r<<24)|((uint32_t)" field ".g<<16)|((uint32_t)" field ".b<<8)|(uint32_t)" field ".a))"

const char* nativeScriptColors_Property(const char* name, bool article)
{
    if (article) {
        if (!strcmp(name,"item.outline_color")) return RGBA_READ("item->xBC8.x0_unk");
        if (!strcmp(name,"item.controller_tint")) return RGBA_READ("item->xBC4");
        if (!strcmp(name,"item.controller_tint_enabled")) return "item->x5C8";
        if (!strcmp(name,"item.material_opacity")) return "item->x5C9";
        if (!strcmp(name,"item.layered")) return "item->xDCF_flag.b3";
    } else {
        if (!strcmp(name,"visuals.outline_color")) return RGBA_READ("fighter->x610_color_rgba[1]");
        if (!strcmp(name,"visuals.preserve_secondary_material")) return "fighter->x2224_b0";
        if (!strcmp(name,"model.instance_scale_z")) return "fighter->x34_scale.z";
    }
    return NULL;
}

static int method(const char* name)
{
    if (!strcmp(name,"color.rgba")) return 1;
    if (!strcmp(name,"targets.controller_color_present")) return 2;
    if (!strcmp(name,"targets.controller_color")) return 3;
    if (!strcmp(name,"visuals.install_outline_visibility")) return 4;
    return 0;
}

static bool emit_expression(int kind, char values[4][384], char* output, size_t capacity)
{
    int n;
    if (kind == 1)
        n=snprintf(output,capacity,
            "({ uint32_t color_r=(uint32_t)(%s); uint32_t color_g=(uint32_t)(%s); "
            "uint32_t color_b=(uint32_t)(%s); uint32_t color_a=(uint32_t)(%s); "
            "(int32_t)(((color_r&255u)<<24)|((color_g&255u)<<16)|((color_b&255u)<<8)|(color_a&255u)); })",
            values[0],values[1],values[2],values[3]);
    else if (kind == 4)
        n=snprintf(output,capacity,"engine->install_outline_visibility(gobj,(int32_t)(%s))",values[0]);
    else
        n=snprintf(output,capacity,
            "({ uint32_t color_target=(uint32_t)(%s); GXColor target_color={0}; "
            "bool color_present=engine->target_controller_color(color_target,&target_color); %s; })",
            values[0],kind==2 ? "color_present" : RGBA_READ("target_color"));
    return n>=0 && (size_t)n<capacity;
}

bool nativeScriptColors_FighterExpression(const ScriptCodeBlock* block,
    const char* name, const char** source, char* output, size_t capacity)
{
    int kind=method(name);
    if (!kind || ((kind!=1) && (!block || block->model_effect || block->item_event))) return false;
    char args[4][192], values[4][384]; const char* after;
    unsigned count=kind==1?4:1;
    if (!nativeScriptCode_ParseArguments(*source,args,count,&after)) return false;
    for (unsigned i=0;i<count;++i)
        if (nativeScriptCode_ExpressionType(block,args[i])!=ScriptType_Int ||
            !nativeScriptCode_Expression(block,args[i],values[i],sizeof(values[i]))) return false;
    if (!emit_expression(kind,values,output,capacity)) return false;
    *source=after;
    return true;
}

bool nativeScriptColors_ItemExpression(const ArticleDeclaration* article,
    const NativeFighterParameters* parameters, const char* name,
    const char** source, char* output, size_t capacity)
{
    int kind=method(name);
    if (!article || !kind || kind==4) return false;
    char args[4][192], values[4][384]; const char* after;
    unsigned count=kind==1?4:1;
    if (!nativeScriptCode_ParseArguments(*source,args,count,&after)) return false;
    for (unsigned i=0;i<count;++i)
        if (nativeFighterItemScript_ExpressionType(args[i],article,parameters)!=ScriptType_Int ||
            !nativeFighterItemScript_Expression(args[i],values[i],sizeof(values[i]),article,parameters)) return false;
    if (!emit_expression(kind,values,output,capacity)) return false;
    *source=after;
    return true;
}

static int emit_color_write(const char* field, const char* value, char* output, size_t capacity)
{
    int n=snprintf(output,capacity,
        "{ uint32_t packed_color=(uint32_t)(%s); %s=(GXColor){ "
        "(uint8_t)(packed_color>>24),(uint8_t)(packed_color>>16),(uint8_t)(packed_color>>8),(uint8_t)packed_color }; }",
        value,field);
    return n>=0 && (size_t)n<capacity?1:-1;
}

static int emit_diffuse(bool article, const char* value, char* output, size_t capacity)
{
    int n=snprintf(output,capacity,
        "{ uint32_t packed_color=(uint32_t)(%s); GXColor diffuse={ "
        "(uint8_t)(packed_color>>24),(uint8_t)(packed_color>>16),(uint8_t)(packed_color>>8),(uint8_t)packed_color }; "
        "engine->%s(gobj,&diffuse); }",value,
        article?"set_item_diffuse_color":"set_fighter_diffuse_color");
    return n>=0 && (size_t)n<capacity?1:-1;
}

static bool assignment(const char* source, const char* name, const char** value)
{
    size_t n=strlen(name);
    if (strncmp(source,name,n) || strncmp(source+n," = ",3)) return false;
    *value=source+n+3;
    return true;
}

int nativeScriptColors_FighterAction(const ScriptCodeBlock* block,
    const char* source, char* output, size_t capacity)
{
    const char* value=NULL; const char* target=NULL;
    const char* names[]={"visuals.outline_color","visuals.preserve_secondary_material","model.instance_scale_z"};
    for (unsigned i=0;i<3;++i) if (assignment(source,names[i],&value)) {target=names[i];break;}
    bool diffuse=!strncmp(source,"visuals.set_diffuse_color(",26);
    bool install=!strncmp(source,"visuals.install_outline_visibility(",35);
    if (!target && !diffuse && !install) return 0;
    if (!block || block->model_effect || block->item_event) return -1;
    char translated[640];
    if (install) {
        const char* after=strchr(source,'(');
        if (!nativeScriptColors_FighterExpression(block,"visuals.install_outline_visibility",&after,translated,sizeof(translated)) || *after) return -1;
        int n=snprintf(output,capacity,"%s;",translated);
        return n>=0 && (size_t)n<capacity?1:-1;
    }
    if (diffuse) {
        char arg[1][192];const char* after;
        if (!nativeScriptCode_ParseArguments(strchr(source,'('),arg,1,&after) || *after ||
            nativeScriptCode_ExpressionType(block,arg[0])!=ScriptType_Int ||
            !nativeScriptCode_Expression(block,arg[0],translated,sizeof(translated))) return -1;
        return emit_diffuse(false,translated,output,capacity);
    }
    ScriptExpressionType type=nativeScriptCode_ExpressionType(block,value);
    if (!strcmp(target,names[0]) ? type!=ScriptType_Int :
        !strcmp(target,names[1]) ? type!=ScriptType_Bool :
        type!=ScriptType_Int && type!=ScriptType_Float) return -1;
    if (!nativeScriptCode_Expression(block,value,translated,sizeof(translated))) return -1;
    if (!strcmp(target,names[0])) return emit_color_write("fighter->x610_color_rgba[1]",translated,output,capacity);
    int n=!strcmp(target,names[1])
        ? snprintf(output,capacity,"fighter->x2224_b0=(%s);",translated)
        : snprintf(output,capacity,"{ float instance_width=(%s); if(isfinite(instance_width) && instance_width!=0) fighter->x34_scale.z=instance_width; }",translated);
    return n>=0 && (size_t)n<capacity?1:-1;
}

int nativeScriptColors_ItemAction(const ArticleDeclaration* article,
    const NativeFighterParameters* parameters,const char* source,char* output,size_t capacity)
{
    const char* names[]={"item.outline_color","item.controller_tint","item.controller_tint_enabled","item.material_opacity","item.layered"};
    const char* value=NULL;unsigned index=5;
    for(unsigned i=0;i<5;++i) if(assignment(source,names[i],&value)){index=i;break;}
    bool diffuse=!strncmp(source,"item.set_diffuse_color(",23);
    if(index==5 && !diffuse)return 0;
    if(!article)return -1;
    char translated[640];
    if(diffuse){
        char arg[1][192];const char* after;
        if(!nativeScriptCode_ParseArguments(strchr(source,'('),arg,1,&after)||*after||
            nativeFighterItemScript_ExpressionType(arg[0],article,parameters)!=ScriptType_Int||
            !nativeFighterItemScript_Expression(arg[0],translated,sizeof(translated),article,parameters))return -1;
        return emit_diffuse(true,translated,output,capacity);
    }
    ScriptExpressionType type=nativeFighterItemScript_ExpressionType(value,article,parameters);
    if((index==2||index==4)?type!=ScriptType_Bool:type!=ScriptType_Int)return -1;
    if(!nativeFighterItemScript_Expression(value,translated,sizeof(translated),article,parameters))return -1;
    if(index<2)return emit_color_write(index?"item->xBC4":"item->xBC8.x0_unk",translated,output,capacity);
    int n=index==2?snprintf(output,capacity,"item->x5C8=(%s);",translated):
        index==3?snprintf(output,capacity,"{ int32_t opacity=(%s); if((uint32_t)opacity<=255) item->x5C9=(uint8_t)opacity; }",translated):
        snprintf(output,capacity,"engine->set_item_layered(gobj,(%s));",translated);
    return n>=0 && (size_t)n<capacity?1:-1;
}
