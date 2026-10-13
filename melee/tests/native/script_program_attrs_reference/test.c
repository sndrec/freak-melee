#include "native_fighter_item_program.h"
#include "native_fighter_script_code.h"
#include "native_fighter_script_model_effect.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,cases;
static NativeFighterParameters parameters;
static ArticleDeclaration article;
static ScriptStateTable state;
static ScriptCodeBlock block;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"program parameter oracle %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static void expression(const char* name,ScriptExpressionType type,const char* expected)
{
 char explicit_name[128],legacy_name[128],explicit_output[512],legacy_output[512];
 snprintf(explicit_name,sizeof(explicit_name),"program.attrs.%s",name);snprintf(legacy_name,sizeof(legacy_name),"attrs.%s",name);
 CHECK(nativeFighterItemScript_ExpressionType(explicit_name,&article,&parameters)==type);
 CHECK(nativeFighterItemScript_Expression(explicit_name,explicit_output,sizeof(explicit_output),&article,&parameters));
 CHECK(nativeFighterItemScript_Expression(legacy_name,legacy_output,sizeof(legacy_output),&article,&parameters));
 CHECK(!strcmp(explicit_output,legacy_output)&&!strcmp(explicit_output,expected));
 CHECK(!strstr(explicit_output,"creator")&&!strstr(explicit_output,"recipient")&&!strstr(explicit_output,"engine->"));
 CHECK(nativeScriptCode_ExpressionType(&block,explicit_name)==type);
 CHECK(nativeScriptCode_Expression(&block,explicit_name,explicit_output,sizeof(explicit_output)));
 CHECK(!strcmp(explicit_output,expected));
 CHECK(nativeScriptModelEffect_AllowedName(&block,explicit_name));
 ++cases;
}
static void generated(void)
{
 const char* source="action Probe:\n    on load:\n        velocity.x = program.attrs.tuning\narticle It_Kind_Link_Bomb states 1:\n    on created:\n            item.life = program.attrs.tuning\n            item.command0 = program.attrs.part\n    state 0 anim 0:\n        on animation:\n            return program.attrs.enabled\n";
 FILE* output=tmpfile();CHECK(output);NativeFighterScriptError error={0};bool accepted=nativeFighterScript_GenerateC(source,strlen(source),"program_attrs_probe",NULL,&parameters,NULL,output,&error);
 if(!accepted)fprintf(stderr,"oracle %u: %s\n",error.line,error.message);CHECK(accepted);
 rewind(output);static char text[262144];size_t count=fread(text,1,sizeof(text)-1,output);CHECK(feof(output));text[count]=0;fclose(output);
 CHECK(strstr(text,"static union { float f; int32_t i; uint32_t bits; } script_parameters[3]"));
 CHECK(strstr(text,"item->xD44_lifeTimer = script_parameters[0].f")||strstr(text,"item->xD44_lifeTimer=script_parameters[0].f"));
 CHECK(strstr(text,"script_parameters[1].i")&&strstr(text,"script_parameters[2].i"));
 CHECK(!strstr(text,"program.attrs."));++cases;
}
static void put32(unsigned char* p,uint32_t value)
{for(unsigned i=0;i<4;++i)p[i]=(unsigned char)(value>>(24-8*i));}
static void imported(void)
{
 unsigned char dat[120]={0};put32(dat,120);put32(dat+4,64);put32(dat+8,1);put32(dat+12,1);put32(dat+36,32);put32(dat+64,UINT32_C(0x3fa00000));put32(dat+96,4);memcpy(dat+108,"ftDataMario",12);
 const char* text="attribute float tuning 0\nparameter int part = -17\nparameter bool enabled = true\naction Probe:\n    on load:\n        velocity.x = program.attrs.tuning\narticle CustomProbe states 1:\n    parameter float local = 2\n    state 0 anim 0:\n        on physics:\n            velocity.x = program.attrs.tuning\n            item.life = item.attr.local\n";
 char *owned=NULL,*values=NULL;size_t owned_size=0,values_size=0;unsigned error_line=0;
 CHECK(nativeFighterParameters_Import(dat,sizeof(dat),"ftDataMario",0,text,strlen(text),&owned,&owned_size,&values,&values_size,&error_line));
 CHECK(owned&&values&&owned_size==strlen(owned)&&values_size==strlen(values));
 CHECK(strstr(owned,"velocity.x = program.attrs.tuning\n"));CHECK(!strstr(owned,"attribute float tuning"));
 CHECK(strstr(owned,"parameter float tuning = 1.25\n"));
 CHECK(strstr(owned,"parameter int part = -17\n"));
 CHECK(strstr(owned,"parameter bool enabled = true\n"));
 CHECK(strstr(owned,"    parameter float local = 2\n"));
 CHECK(strstr(owned,"item.life = item.params.local\n"));CHECK(!strstr(owned,"item.attr.local"));
 NativeFighterParameters checked;CHECK(nativeFighterParameters_Parse(values,values_size,&checked,NULL));CHECK(checked.count==4);
 CHECK(!strcmp(checked.fields[0].name,"tuning")&&checked.fields[0].bits==UINT32_C(0x3fa00000));
 CHECK(!strcmp(checked.fields[3].name,"CustomProbe.local")&&checked.fields[3].bits==UINT32_C(0x40000000));
 free(owned);free(values);++cases;
}
int main(void)
{
 const char* values="float tuning = 1.25\nint part = -17\nbool enabled = true\n";CHECK(nativeFighterParameters_Parse(values,strlen(values),&parameters,NULL));
 strcpy(article.kind,"It_Kind_Link_Bomb");article.attribute_count=1;strcpy(article.attributes[0].name,"tuning");article.attributes[0].offset=12;article.attributes[0].integer=true;
 state.parameters=&parameters;state.attribute_count=1;strcpy(state.attributes[0].name,"raw_only");state.attributes[0].offset=12;state.attributes[0].is_int=true;block.state=&state;
 expression("tuning",ScriptType_Float,"script_parameters[0].f");expression("part",ScriptType_Int,"script_parameters[1].i");expression("enabled",ScriptType_Bool,"script_parameters[2].i");
 char long_name[128],qualified[160],translated[512];memset(long_name,'x',127);long_name[127]=0;strcpy(parameters.fields[3].name,long_name);parameters.fields[3].type=NativeFighterParameter_Int;parameters.count=4;snprintf(qualified,sizeof(qualified),"program.attrs.%s",long_name);
 CHECK(nativeFighterItemScript_ExpressionType(qualified,&article,&parameters)==ScriptType_Int);CHECK(nativeFighterItemScript_Expression(qualified,translated,sizeof(translated),&article,&parameters));CHECK(!strcmp(translated,"script_parameters[3].i"));CHECK(nativeScriptCode_Expression(&block,qualified,translated,sizeof(translated)));CHECK(!strcmp(translated,"script_parameters[3].i"));parameters.count=3;++cases;
 CHECK(nativeFighterItemScript_ExpressionType("item.attr.tuning",&article,&parameters)==ScriptType_Int);
 ScriptScalar scalar={0};CHECK(nativeScriptCode_ParseScalar("attrs.raw_only",&state,&scalar)&&scalar.declared_offset_plus_one==13);
 const char* invalid[]={"program.attrs.missing","program.attrs.raw_only","program.attrs.tuning.extra","program.attrs.tuning()"};
 for(unsigned i=0;i<4;++i){char output[512];CHECK(nativeFighterItemScript_ExpressionType(invalid[i],&article,&parameters)==ScriptType_Invalid);CHECK(!nativeFighterItemScript_Expression(invalid[i],output,sizeof(output),&article,&parameters));CHECK(nativeScriptCode_ExpressionType(&block,invalid[i])==ScriptType_Invalid);CHECK(!nativeScriptCode_Expression(&block,invalid[i],output,sizeof(output)));++cases;}
 scalar=(ScriptScalar){0};CHECK(!nativeScriptCode_ParseScalar("program.attrs.raw_only",&state,&scalar));CHECK(!nativeScriptModelEffect_AllowedName(&block,"program.attrs.raw_only"));
 char output[128];CHECK(nativeFighterItemScript_ExpressionType("program.attrs.tuning",&article,NULL)==ScriptType_Invalid);CHECK(!nativeFighterItemScript_Expression("program.attrs.tuning",output,sizeof(output),&article,NULL));++cases;
 generated();imported();printf("{\"checks\":%u,\"cases\":%u}\n",checks,cases);return 0;
}
