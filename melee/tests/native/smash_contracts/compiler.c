#include "../../../tools/native_fighter_script_smash.h"
#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(value) do { ++checks; assert(value); } while (0)

int main(void)
{
    const char* properties[] = { "smash.charge_mode", "smash.charge_frames",
        "smash.charge_limit", "smash.saved_animation_rate", "smash.charge_color",
        "smash.shake_index", "smash.shake_period", "smash.shake_table_count" };
    ScriptCodeBlock block = { 0 };
    char line[192], output[2048];
    for (unsigned i=0;i<8;++i) {
        CHECK(nativeScriptSmash_Property(properties[i])!=NULL);
        CHECK(nativeScriptSmash_Type(properties[i])==(i>=1&&i<=3?ScriptType_Float:ScriptType_Int));
        snprintf(line,sizeof(line),"%s = 1",properties[i]);
        CHECK(nativeScriptSmash_Action(&block,line,output,sizeof(output))==(i<7));
        CHECK(!nativeScriptSmash_Action(&block,line,output,1));
        block.model_effect=true;
        CHECK(!nativeScriptSmash_Action(&block,line,output,sizeof(output)));
        block.model_effect=false;
        block.item_event=true;
        CHECK(nativeScriptSmash_Action(&block,line,output,sizeof(output))==(i<7));
        block.item_event=false;
        snprintf(line,sizeof(line),"%s = 1.5",properties[i]);
        CHECK(nativeScriptSmash_Action(&block,line,output,sizeof(output))==(i>=1&&i<=3));
    }
    const char* names[]={"Smash.none","Smash.precharge","Smash.charging","Smash.release","Smash.manual_charge"};
    for(unsigned i=0;i<5;++i){
        CHECK(nativeScriptSmash_Type(names[i])==ScriptType_Int);
        CHECK(atoi(nativeScriptSmash_Property(names[i]))==(int)i);
    }
    CHECK(nativeScriptSmash_Type("smash.shake_period()")==ScriptType_Invalid);
    CHECK(nativeScriptSmash_Property("smash.shake_table_count.extra")==NULL);
    const char* bad[]={"smash.charge_mode = true","smash.shake_index = 1.0",
        "smash.shake_period = false","smash.shake_table_count = 1",
        "Smash.none = 1","smash.charge_limit = missing","smash.charge_mode() = 1"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(bad[0]);++i)
        CHECK(!nativeScriptSmash_Action(&block,bad[i],output,sizeof(output)));
    static const char source[]="action Charge:\n    on load:\n"
        "        let int count = smash.shake_table_count\n"
        "        smash.shake_period = count\n"
        "        smash.charge_mode = Smash.manual_charge\n"
        "        let float rate = smash.saved_animation_rate\n";
    FILE* file=tmpfile();CHECK(file!=NULL);NativeFighterScriptError error={0};
    bool ok=nativeFighterScript_GenerateC(source,sizeof(source)-1,"smash_contract",NULL,NULL, NULL,file,&error);
    if(!ok)fprintf(stderr,"line%u %s\n",error.line,error.message);CHECK(ok);
    char generated[65536];rewind(file);size_t n=fread(generated,1,sizeof(generated)-1,file);generated[n]=0;CHECK(feof(file));fclose(file);
    CHECK(strstr(generated,"host->common_attribute == 0")!=NULL);
    CHECK(strstr(generated,"smash_value <= smash_count")!=NULL);
    puts("Common charge compiler contract checks passed");
    printf("Checks: %u\n",checks);return 0;
}
