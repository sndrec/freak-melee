#include "../../../tools/native_fighter_script.h"
#include "../../../tools/native_fighter_script_colors.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static bool generate(const char* text) {
    NativeFighterScriptError error={0}; FILE* out=tmpfile(); CHECK(out);
    bool ok=nativeFighterScript_GenerateC(text,strlen(text),"presentation",NULL,NULL, NULL,out,&error);
    fclose(out); return ok;
}
int main(void) {
    const char* bad[]={"visibility_asset 16\naction P:\n    on load:\n        velocity.x = 1\n",
        "visibility_asset -1\naction P:\n    on load:\n        velocity.x = 1\n",
        "visibility_asset 10\nvisibility_asset 10\naction P:\n    on load:\n        velocity.x = 1\n",
        "action P:\n    on load:\n        velocity.x = 1\nvisibility_asset 10\n"};
    for(unsigned i=0;i<4;++i) CHECK(!generate(bad[i]));
    const char* lines[]={"visuals.outline_color = 1.5", "visuals.preserve_secondary_material = 1",
        "visuals.set_diffuse_color(1.5)", "let int c = color.rgba(1,2,3,4.5)",
        "let int c = targets.controller_color(1.5)", "let bool c = visuals.install_outline_visibility(1.5)",
        "visuals.outline_color = color.rgba(1,2,3)", "visuals.outline_color = color.rgba(1,2,3,4,5)"};
    char source[512];
    for(unsigned i=0;i<sizeof(lines)/sizeof(*lines);++i) {
        snprintf(source,sizeof(source),"action P:\n    on load:\n        %s\n",lines[i]);
        CHECK(!generate(source));
    }
    ScriptCodeBlock block={0}; char output[4096];
    block.model_effect=true;
    CHECK(nativeScriptColors_FighterAction(&block,"visuals.outline_color = 1",output,sizeof(output))<0);
    block.model_effect=false;block.item_event=true;
    CHECK(nativeScriptColors_FighterAction(&block,"visuals.outline_color = 1",output,sizeof(output))<0);
    block.item_event=false;
    CHECK(nativeScriptColors_FighterAction(&block,"visuals.outline_color = 1",output,1)<0);
    CHECK(nativeScriptColors_FighterAction(&block,"visuals.install_outline_visibility(1)",output,1)<0);
    puts("Presentation controls reject wrong types, duplicate slots and invalid contexts.");
    printf("Checks: %u\n",checks);return 0;
}
