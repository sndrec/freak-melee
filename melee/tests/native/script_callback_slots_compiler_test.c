#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_functions.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)

static void direct_emitter(void)
{
    static const struct { const char* name; const char* field; } bindings[] = {
        { "animation", "anim_cb" }, { "interrupt", "input_cb" },
        { "physics", "phys_cb" }, { "collision", "coll_cb" },
        { "accessory", "accessory4_cb" }, { "accessory_before", "accessory2_cb" },
        { "accessory_hitlag", "accessory3_cb" }, { "facing_changed", "x21F8" },
        { "pre_hitlag", "pre_hitlag_cb" }, { "post_hitlag", "post_hitlag_cb" },
        { "damage", "take_dmg_cb" }, { "death1", "death1_cb" },
        { "damage_secondary", "take_dmg_2_cb" },
        { "death2", "death2_cb" }, { "death3", "death3_cb" },
        { "state_changed", "x21EC" },
    };
    static const struct { const char* name; const char* field; } queries[] = {
        { "callbacks.accessory_before_is", "accessory2_cb" },
        { "callbacks.accessory_hitlag_is", "accessory3_cb" },
        { "callbacks.death1_is", "death1_cb" },
        { "callbacks.damage_secondary_is", "take_dmg_2_cb" },
    };
    ScriptFunctionProgram* functions=calloc(1,sizeof(*functions));
    CHECK(functions!=NULL); functions->count=1;
    strcpy(functions->functions[0].name,"update");
    ScriptCodeBlock caller={0}; caller.functions=functions;
    char line[192], output[192], expected[192];
    for(unsigned i=0;i<sizeof(bindings)/sizeof(bindings[0]);++i) {
        for(unsigned empty=0;empty<2;++empty) {
            snprintf(line,sizeof(line),"callbacks.%s( %s )",bindings[i].name,empty?"None":"update");
            snprintf(expected,sizeof(expected),"fighter->%s = %s;",bindings[i].field,empty?"NULL":"script_fn_update");
            CHECK(nativeScriptFunctions_BindCallback(&caller,line,output,sizeof(output)));
            CHECK(strcmp(output,expected)==0); CHECK(!nativeScriptDependencies_Any(caller.function_calls));
        }
    }
    for(unsigned i=0;i<sizeof(queries)/sizeof(queries[0]);++i) {
        for(unsigned empty=0;empty<2;++empty) {
            const char* source=empty?"( None ) and false":"( update ) or true";
            CHECK(nativeScriptFunctions_CallbackExpression(&caller,queries[i].name,&source,output,sizeof(output)));
            snprintf(expected,sizeof(expected),"(fighter->%s == %s)",queries[i].field,empty?"NULL":"script_fn_update");
            CHECK(strcmp(output,expected)==0);
            CHECK(strcmp(source,empty?" and false":" or true")==0);
            CHECK(!nativeScriptDependencies_Any(caller.function_calls));
        }
    }
    const char* bad_args[]={"()","(missing)","(\"update\")","(update())",
                            "(update, None)","(1)","(update", "(None,)","(None]"};
    for(unsigned i=0;i<sizeof(bad_args)/sizeof(bad_args[0]);++i) {
        snprintf(line,sizeof(line),"callbacks.accessory_before%s",bad_args[i]);
        CHECK(!nativeScriptFunctions_BindCallback(&caller,line,output,sizeof(output)));
        const char* source=bad_args[i];
        CHECK(!nativeScriptFunctions_CallbackExpression(&caller,"callbacks.accessory_before_is",&source,output,sizeof(output)));
        CHECK(source==bad_args[i]);
    }
    for(unsigned invalid=0;invalid<4;++invalid) {
        functions->functions[0].argument_count=invalid==0?1:0;
        functions->functions[0].disabled=invalid==1;
        functions->functions[0].model_effect_only=invalid==2;
        caller.model_effect=invalid==3;
        CHECK(!nativeScriptFunctions_BindCallback(&caller,"callbacks.accessory_hitlag(update)",output,sizeof(output)));
        const char* source="(update)";
        CHECK(!nativeScriptFunctions_CallbackExpression(&caller,"callbacks.accessory_hitlag_is",&source,output,sizeof(output)));
    }
    caller.model_effect=false; caller.functions=NULL;
    CHECK(nativeScriptFunctions_BindCallback(&caller,"callbacks.accessory_before(None)",output,sizeof(output)));
    const char* source="(None)";
    CHECK(nativeScriptFunctions_CallbackExpression(&caller,"callbacks.death1_is",&source,output,sizeof(output)));
    CHECK(!nativeScriptFunctions_BindCallback(&caller,"callbacks.accessory_before(None) trailing",output,sizeof(output)));
    CHECK(!nativeScriptFunctions_BindCallback(NULL,"callbacks.accessory_before(None)",output,sizeof(output)));
    source="(None)";
    CHECK(!nativeScriptFunctions_CallbackExpression(NULL,"callbacks.death1_is",&source,output,sizeof(output)));
    CHECK(!nativeScriptFunctions_CallbackExpression(&caller,"callbacks.accessory_is",&source,output,sizeof(output)));
    source="(None)"; const char* before=source;
    CHECK(!nativeScriptFunctions_CallbackExpression(&caller,"callbacks.death1_is",&source,output,4));
    CHECK(source==before); CHECK(!nativeScriptDependencies_Any(caller.function_calls));
    free(functions);
}

static void complete_generation(void)
{
    static const char source[]=
        "state bool vacant\n"
        "func update():\n    physics.ground_friction()\n"
        "func hitlag():\n    state.vacant = false\n"
        "func cleanup():\n    callbacks.death1(None)\n"
        "action CallbackSlots:\n    on load:\n"
        "        callbacks.accessory_before(update)\n"
        "        callbacks.accessory_hitlag(hitlag)\n"
        "        callbacks.accessory(update)\n"
        "        callbacks.pre_hitlag(hitlag)\n"
        "        callbacks.post_hitlag(update)\n"
        "        callbacks.death1(cleanup)\n"
        "        let bool unused = callbacks.accessory_before_is(None)\n"
        "        state.vacant = callbacks.accessory_hitlag_is(hitlag) and callbacks.death1_is(cleanup)\n"
        "        if callbacks.accessory_before_is(update): callbacks.accessory_before(None)\n";
    FILE* file=tmpfile(); CHECK(file!=NULL);
    NativeFighterScriptError error={0};
    bool valid=nativeFighterScript_GenerateC(source,sizeof(source)-1,"callback_slots",NULL,NULL, NULL,file,&error);
    if(!valid) fprintf(stderr,"Callback slot at%u: %s\n",error.line,error.message);
    CHECK(valid);
    char output[65536]; rewind(file);
    size_t count=fread(output,1,sizeof(output)-1,file); output[count]=0; CHECK(feof(file)); fclose(file);
    CHECK(strstr(output,"fighter->accessory2_cb = script_fn_update;")!=NULL);
    CHECK(strstr(output,"fighter->accessory3_cb = script_fn_hitlag;")!=NULL);
    CHECK(strstr(output,"fighter->accessory4_cb = script_fn_update;")!=NULL);
    CHECK(strstr(output,"fighter->pre_hitlag_cb = script_fn_hitlag;")!=NULL);
    CHECK(strstr(output,"fighter->post_hitlag_cb = script_fn_update;")!=NULL);
    CHECK(strstr(output,"fighter->death1_cb == script_fn_cleanup")!=NULL);
    CHECK(strstr(output,"fighter->accessory2_cb == NULL")!=NULL);
    CHECK(strstr(output,"fighter->accessory3_cb == script_fn_hitlag")!=NULL);
    CHECK(strstr(output,"host->ground_friction == 0")!=NULL);
}

int main(void)
{
    direct_emitter(); complete_generation();
    printf("Callback slot compiler checks: %u\n",checks);
    return 0;
}
