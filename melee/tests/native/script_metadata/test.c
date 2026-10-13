#include <melee/ft/native_script_metadata.h>
#include <melee/ft/native_script_expression.h>
#include <sysdolphin/baselib/dat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NativeScriptSourceNode syntax[8192];static NativeScriptIRNode expressions[16384];static uint32_t arguments[16384];
static NativeScriptFunctionDeclaration functions[2048];static NativeScriptParameterDeclaration parameters[4096];
static NativeScriptValueDeclaration fields[2048];static NativeScriptParameterSupply supplies[2048];
static NativeScriptSourceTree tree;static NativeScriptIRArena arena;static NativeScriptProgramIndex index;
static NativeScriptDiagnostic diagnostic;static unsigned checks;
#define CHECK(value)do{++checks;if(!(value)){fprintf(stderr,"line%u %s: %s\n",__LINE__,#value,diagnostic.message);exit(1);}}while(0)

static void be32(uint8_t* data,uint32_t value)
{data[0]=(uint8_t)(value>>24);data[1]=(uint8_t)(value>>16);data[2]=(uint8_t)(value>>8);data[3]=(uint8_t)value;}

static void parse(const char* source,uint32_t bytes)
{
    tree=(NativeScriptSourceTree){source,bytes,syntax,0,8192};arena=(NativeScriptIRArena){source,bytes,expressions,0,16384,arguments,0,16384};
    index=(NativeScriptProgramIndex){.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=2048,
        .parameters=parameters,.parameter_capacity=4096,.values=fields,.value_capacity=2048};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));CHECK(nativeScriptProgram_Index(&index,&diagnostic));
}

static void synthetic(void)
{
    const char* source="attribute float acceleration 0\nattribute int signed_value 4\nattribute bool enabled 8\nattribute byte category 9\nattribute float end_value 92\nparameter float explicit_default = 3\narticle It_Kind_Mario_Fire states 1:\n    attribute float entity_value 0\n";
    parse(source,(uint32_t)strlen(source));
    uint8_t bytes[184]={0};be32(bytes,184);be32(bytes+4,128);be32(bytes+8,1);be32(bytes+12,1);
    be32(bytes+32+4,32);be32(bytes+160,4);memcpy(bytes+172,"ftDataProbe",12);
    be32(bytes+64,UINT32_C(0x40200000));be32(bytes+68,(uint32_t)-123456);bytes[72]=1;bytes[73]=239;be32(bytes+156,UINT32_C(0x40800000));
    NativeScriptMetadataDat dat={bytes,sizeof(bytes),"ftDataProbe"};
    uint32_t count=99;NativeScriptMetadataImport report={0};
    CHECK(nativeScriptMetadata_ImportFighterParameters(&index,&dat,supplies,2048,&count,&report,&diagnostic));
    CHECK(count==5&&report.fighter_imports==5&&report.unresolved_entity_imports==1&&report.attribute_bytes==96);
    CHECK(supplies[0].value.type==NativeScriptType_F32&&supplies[0].value.as.f32==2.5f);
    CHECK(supplies[1].value.type==NativeScriptType_I32&&supplies[1].value.as.i64==-123456);
    CHECK(supplies[2].value.type==NativeScriptType_Bool&&supplies[2].value.as.u64==1);
    CHECK(supplies[3].value.type==NativeScriptType_U32&&supplies[3].value.as.u64==239);
    CHECK(supplies[4].value.type==NativeScriptType_F32&&supplies[4].value.as.f32==4);
    CHECK(fields[5].default_value.as.f32==3);for(uint32_t i=0;i<count;++i)CHECK(supplies[i].owner==0);
    NativeScriptParameterSupply saved[8];memcpy(saved,supplies,sizeof(saved));NativeScriptMetadataImport saved_report=report;
#define FAILURE() do{count=777;report=saved_report;CHECK(!nativeScriptMetadata_ImportFighterParameters(&index,&dat,supplies,2048,&count,&report,&diagnostic));CHECK(count==777&&memcmp(&report,&saved_report,sizeof(report))==0&&memcmp(supplies,saved,sizeof(saved))==0);}while(0)
    bytes[72]=2;FAILURE();bytes[72]=1;
    be32(bytes+64,UINT32_C(0x7f800000));FAILURE();be32(bytes+64,UINT32_C(0x40200000));
    fields[4].import_offset=96;FAILURE();fields[4].import_offset=92;
    fields[0].import_offset=2;FAILURE();fields[0].import_offset=0;
    const char* root=dat.fighter_root;dat.fighter_root="missing";FAILURE();dat.fighter_root=root;
    size_t size=dat.size;dat.size=100;FAILURE();dat.size=size;
    uint32_t original=fields[0].count;fields[0].count=2;FAILURE();fields[0].count=original;
    count=777;CHECK(!nativeScriptMetadata_ImportFighterParameters(&index,&dat,supplies,1,&count,&report,&diagnostic));CHECK(count==777&&memcmp(supplies,saved,sizeof(saved))==0);
    be32(bytes+36,0);FAILURE();be32(bytes+36,32);
#undef FAILURE
    CHECK(nativeScriptMetadata_ImportFighterParameters(&index,&dat,supplies,2048,&count,&report,&diagnostic));
}

static unsigned real(const char* dat_path,const char* script_path,const char* root)
{
    FILE* input=fopen(dat_path,"rb");CHECK(input!=NULL);CHECK(fseek(input,0,SEEK_END)==0);long length=ftell(input);CHECK(length>0);rewind(input);
    uint8_t* bytes=malloc((size_t)length);CHECK(bytes!=NULL);CHECK(fread(bytes,1,(size_t)length,input)==(size_t)length);fclose(input);
    input=fopen(script_path,"rb");CHECK(input!=NULL);CHECK(fseek(input,0,SEEK_END)==0);long script_size=ftell(input);CHECK(script_size>0);rewind(input);
    char* source=malloc((size_t)script_size);CHECK(source!=NULL);CHECK(fread(source,1,(size_t)script_size,input)==(size_t)script_size);fclose(input);
    parse(source,(uint32_t)script_size);NativeScriptMetadataDat dat={bytes,(size_t)length,root};uint32_t count=0;NativeScriptMetadataImport report;
    CHECK(nativeScriptMetadata_ImportFighterParameters(&index,&dat,supplies,2048,&count,&report,&diagnostic));
    HSD_Dat view;u32 root_offset,attribute;const char* external;
    CHECK(HSD_DatOpen(&view,bytes,(size_t)length)==HSD_DAT_OK);CHECK(HSD_DatFindRoot(&view,root,&root_offset));
    CHECK(HSD_DatResolve(&view,root_offset+4,&attribute,&external)==HSD_DAT_REF_LOCAL);
    for(uint32_t i=0;i<count;++i){
        uint32_t id=supplies[i].semantic&UINT32_C(0x0fffffff);const NativeScriptValueDeclaration* field=&fields[id];CHECK(field->owner==0&&field->imported);
        if(field->type==NativeScriptType_Bool||field->type==NativeScriptType_U32){const u8* value=HSD_DatBytes(&view,attribute+field->import_offset,1);CHECK(value!=NULL);CHECK(supplies[i].value.as.u64==*value);}
        else{u32 value;CHECK(HSD_DatReadU32(&view,attribute+field->import_offset,&value));uint32_t actual;
            if(field->type==NativeScriptType_F32)memcpy(&actual,&supplies[i].value.as.f32,4);else actual=(uint32_t)supplies[i].value.as.i64;
            CHECK(actual==value);
        }
    }
    printf("{\"root_imports\":%u,\"entity_imports_unresolved\":%u,\"actual_attribute_bytes\":%u,\"checks\":%u}",count,report.unresolved_entity_imports,report.attribute_bytes,checks);
    free(source);free(bytes);return count;
}

int main(int argc,char** argv)
{
    CHECK(argc==1||argc==4);synthetic();
    if(argc==1){printf("{\"result\":\"pass\",\"checks\":%u,\"synthetic_only\":true}\n",checks);return 0;}
    unsigned count=real(argv[1],argv[2],argv[3]);CHECK(count==31);putchar('\n');return 0;
}
