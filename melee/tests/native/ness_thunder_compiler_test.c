#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_kinetics.h"
#include <melee/ft/forward.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char* read_file(const char* path, size_t* size)
{
    FILE* file = fopen(path, "rb"); assert(file);
    assert(!fseek(file, 0, SEEK_END)); long length = ftell(file); assert(length >= 0);
    assert(!fseek(file, 0, SEEK_SET));
    char* text = malloc((size_t)length + 1); assert(text);
    *size = fread(text, 1, (size_t)length, file); assert(*size == (size_t)length);
    text[*size] = 0; fclose(file); return text;
}

static bool compile(const char* source, const NativeFighterParameters* parameters,
                    FILE* output, bool diagnose)
{
    NativeFighterScriptError error = { 0 };
    bool result = nativeFighterScript_GenerateC(source, strlen(source),
        "ness_clone", NULL, parameters, NULL, output, &error);
    if (!result && diagnose) fprintf(stderr, "Thunder compiler line %u: %s\n", error.line, error.message);
    return result;
}

static void contracts(void)
{
    ScriptCodeBlock effect_context = { 0 };
    effect_context.model_effect = true;
    char output[1024];
    assert(!nativeScriptKinetics_Action(&effect_context,
        "position.x = 1", output, sizeof(output)));
    const char* positive =
        "state float cross_x\nstate float cross_y\nstate float cross_z\nstate int stage\n"
        "action Queries:\n    on damage:\n"
        "        state.cross_x = vectors.cross_normalized_x(1, 2, 3, 4, 5, 6)\n"
        "        state.cross_y = vectors.cross_normalized_y(1, 2, 3, 4, 5, 6)\n"
        "        state.cross_z = vectors.cross_normalized_z(1, 2, 3, 4, 5, 6)\n"
        "        state.stage = stage.internal_kind\n"
        "        let bool callback = callbacks.accessory_before_is(None)\n"
        "        let bool platform = floor.is_platform\n"
        "        state.cross_x = vectors.angle(1, 2, 3, 4, 5, 6)\n"
        "        physics.rotate_velocity_unit_axis(0, 0, 1, 0.5)\n"
        "        physics.mirror_velocity(0, 1, 0)\n"
        "        position.x = position.x + 1\n"
        "        position.y = position.y - 2\n"
        "        position.z = position.z * 3\n"
        "        state.freefall_blend(1, 0, 1, 1, 12, 0.2)\n"
        "        state.down_bound()\n"
        "article It_Kind_Ness_PKThunder states 1:\n    state int group\n"
        "    state 0 anim 0:\n        on physics:\n"
        "            velocity.x = vectors.cross_normalized_x(1, 2, 3, 4, 5, 6)\n"
        "            velocity.y = vectors.cross_normalized_y(1, 2, 3, 4, 5, 6)\n"
        "            velocity.z = vectors.cross_normalized_z(1, 2, 3, 4, 5, 6)\n"
        "            let int group = item.collision_group\n"
        "            item.state.group = group\n"
        "            if stage.internal_kind == 84:\n                item.release_camera()\n";
    FILE* file=tmpfile();assert(file);assert(compile(positive,NULL,file,true));
    fseek(file,0,SEEK_END);long length=ftell(file);rewind(file);
    char* code=malloc(length+1);assert(code&&fread(code,1,length,file)==(size_t)length);code[length]=0;
    assert(strstr(code,"host->stage_internal_kind == 0"));
    assert(strstr(code,"host->item_release_camera == 0"));
    assert(strstr(code,"host->vector_cross_normalized == 0"));
    assert(strstr(code,"item->xAC4_ignoreItemID"));
    assert(strstr(code,"cross_result.x")&&strstr(code,"cross_result.y")&&strstr(code,"cross_result.z"));
    assert(strstr(code,"host->vector_rotate_unit_axis == 0"));
    assert(strstr(code,"host->enter_freefall_blend == 0"));
    assert(strstr(code,"host->enter_down_bound == 0"));
    assert(strstr(code,"engine->callback_source_alive(gobj, callback_generation"));
    assert(strstr(code,"accessory2_cb == NULL"));
    assert(strstr(code,"fighter->cur_pos.x ="));
    assert(strstr(code,"fighter->cur_pos.y ="));
    assert(strstr(code,"fighter->cur_pos.z ="));
    assert(!strstr(code,"native_article.state" "_cross"));
    free(code);fclose(file);
    const char* invalid[]={
        "action Bad:\n    on damage:\n        let x = vectors.cross_normalized_q(1, 2, 3, 4, 5, 6)\n",
        "action Bad:\n    on damage:\n        let x = vectors.cross_normalized_x(1, 2, 3, 4, 5)\n",
        "article It_Kind_Ness_PKThunder states 1:\n    state 0 anim 0:\n        on physics:\n            item.collision_group = 0\n",
        "article It_Kind_Ness_PKThunder states 1:\n    state 0 anim 0:\n        on physics:\n            stage.internal_kind = 0\n",
        "article It_Kind_Ness_PKThunder states 1:\n    state 0 anim 0:\n        on physics:\n            velocity.x = vectors.cross_normalized_x(1,2,3,4,5,6,7)\n",
        "article It_Kind_Ness_PKThunder states 1:\n    state 0 anim 0:\n        on physics:\n            item.release_camera(1)\n",
        "action Bad:\n    on damage:\n        floor.is_platform = true\n",
        "action Bad:\n    on damage:\n        state.freefall_blend(1, 0, 1, 1, 12)\n",
        "action Bad:\n    on damage:\n        physics.rotate_velocity_unit_axis(0, 0, 1)\n",
        "action Bad:\n    on damage:\n        let x = vectors.angle(1, 2, 3, 4, 5)\n",
        "action Bad:\n    on damage:\n        position.q = 1\n",
        "action Bad:\n    on damage:\n        position.x = unknown_name\n",
        "action Bad:\n    on damage:\n        position.x = \"not a number\"\n",
        "action Bad:\n    on damage:\n        position.x = targets.call(0, \"event\")\n",
    };
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){file=tmpfile();assert(file);assert(!compile(invalid[i],NULL,file,false));fclose(file);}
}

int main(int argc, char** argv)
{
    contracts();
    if (argc == 5 && !strcmp(argv[1], "--import")) {
        size_t source_size,dat_size;char* source=read_file(argv[2],&source_size);char* dat=read_file(argv[3],&dat_size);
        char *owned,*text;size_t owned_size,text_size;unsigned line=0;
        if(!nativeFighterParameters_Import(dat,dat_size,"ftDataNess",Ft_Kind_Ness,source,source_size,&owned,&owned_size,&text,&text_size,&line)){
            fprintf(stderr,"Normal parameter import line %u failed\n",line);return 1;}
        char path[1024];const char* names[]={"behavior.fms","fighter_parameters.fmp"};const char* contents[]={owned,text};size_t sizes[]={owned_size,text_size};
        for(unsigned i=0;i<2;++i){snprintf(path,sizeof(path),"%s/%s",argv[4],names[i]);FILE* f=fopen(path,"wb");assert(f&&fwrite(contents[i],1,sizes[i],f)==sizes[i]);fclose(f);}
        NativeFighterParameters parameters;assert(nativeFighterParameters_Parse(text,text_size,&parameters,&line));
        snprintf(path,sizeof(path),"%s/behavior.generated.c",argv[4]);FILE* out=fopen(path,"wb");assert(out);
        bool valid=compile(owned,&parameters,out,true);fclose(out);free(owned);free(text);free(source);free(dat);if(!valid)return 1;
    } else if (argc == 4) {
        size_t source_size, parameter_size;
        char* source = read_file(argv[1], &source_size);
        char* text = read_file(argv[2], &parameter_size);
        NativeFighterParameters parameters; unsigned line = 0;
        assert(nativeFighterParameters_Parse(text, parameter_size, &parameters, &line));
        FILE* output = fopen(argv[3], "wb"); assert(output);
        bool valid = compile(source, &parameters, output, true);
        fclose(output); free(source); free(text); if (!valid) return 1;
    } else if (argc != 1) return 2;
    puts("PK Thunder generic stage/camera/cross compiler contracts PASS");
    return 0;
}
