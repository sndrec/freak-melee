#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_expression.h>
#include <stdio.h>
#include <string.h>

static char source[1048576];
static NativeScriptExpressionToken tokens[131072];
static uint32_t uses[2048];
static unsigned unknown;

static bool prefix(const char* name,const char* expected) {return !strncmp(name,expected,strlen(expected));}
static bool candidate(const char* name)
{
    const char* prefixes[]={"FtPart_","ftCo_MS_","Ft_MF_","ftCo_MF_","ftCommon_GroundAirColl_MF",
        "It_Kind_","It_PKind_","It_Common_","ITEM_","Ft_Kind_","Collide_"};
    for (unsigned i=0;i<sizeof(prefixes)/sizeof(*prefixes);++i) if (prefix(name,prefixes[i])) return true;
    NativeScriptConstant constant;
    return nativeScriptConstants_Lookup(name,(uint32_t)strlen(name),&constant);
}

static void record(const char* name)
{
    for (uint32_t i=0;i<nativeScriptConstants_Count();++i) {
        NativeScriptConstant value;nativeScriptConstants_At(i,&value);
        if (!strcmp(value.name,name)) {++uses[i];return;}
    }
    if (unknown++) putchar(',');printf("\"%s\"",name);
}

int main(int argc,char** argv)
{
    if (argc!=2 || nativeScriptConstants_Count()>sizeof(uses)/sizeof(*uses)) return 2;
    FILE* input=fopen(argv[1],"rb");if (!input) return 2;
    uint32_t bytes=(uint32_t)fread(source,1,sizeof(source),input);
    bool complete=feof(input);fclose(input);if (!complete) return 2;
    NativeScriptDiagnostic diagnostic;uint32_t count;
    if (!nativeScriptExpression_Tokenize(source,bytes,(NativeScriptSourceSpan){.length=bytes,.line=1,.column=1},
        tokens,131072,&count,&diagnostic)) {fprintf(stderr,"line %u: %s\n",diagnostic.source.line,diagnostic.message);return 1;}
    printf("{\"unresolved\":[");
    for (uint32_t i=0;i+1<count;++i) {
        NativeScriptExpressionToken token=tokens[i];
        if (token.kind!=NativeScriptToken_Name || (i && tokens[i-1].kind==NativeScriptToken_Dot) || token.text_length>=96) continue;
        char name[192];memcpy(name,source+token.text_offset,token.text_length);name[token.text_length]=0;
        NativeScriptIRNode node={.kind=NativeScriptIR_Name,.text_offset=token.text_offset,.text_length=token.text_length,
            .source=token.source,.a=NATIVE_SCRIPT_NODE_NONE};
        NativeScriptIRArena arena={.source=source,.source_bytes=bytes};
        NativeScriptConstant scalar;NativeScriptIRResolution space;
        int found=nativeScriptConstants_LookupNode(&arena,&node,&scalar,&space,&diagnostic);
        if (found==2) {
            if (i+2<count && tokens[i+1].kind==NativeScriptToken_Dot && tokens[i+2].kind==NativeScriptToken_Name &&
                tokens[i+2].text_length<96) {
                uint32_t length=token.text_length;name[length++]='.';
                memcpy(name+length,source+tokens[i+2].text_offset,tokens[i+2].text_length);
                name[length+tokens[i+2].text_length]=0;record(name);
            }
        } else if (candidate(name)) record(name);
    }
    printf("],\"uses\":{");bool comma=false;uint32_t total=0;
    for (uint32_t i=0;i<nativeScriptConstants_Count();++i) if (uses[i]) {
        NativeScriptConstant value;nativeScriptConstants_At(i,&value);
        if (comma) putchar(',');comma=true;printf("\"%s\":%u",value.name,uses[i]);total+=uses[i];
    }
    printf("},\"lexical_uses\":%u,\"whole_program_acceptance\":false}\n",total);return 0;
}
