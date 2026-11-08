#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_fighter_family_project.h>
#include <melee/ft/native_script_fighter.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static NativeFighterFamilyProject family,opened;
static NativeFighterProject base;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"source family %u: %s\n",__LINE__,#x);return 1;}}while(0)
static const char partner_script[] =
    "fighter:\n    behavior independent\nstate int marker\naction Main:\n"
    "    on load:\n        state.marker = 17\n";

static bool same_bytes(const void* a,size_t an,const void* b,size_t bn)
{return an==bn&&(!an||memcmp(a,b,an)==0);}

int main(int argc,char** argv)
{
    CHECK(argc==4&&aurora_dvd_open(argv[1]));
    CHECK(nativeFighterProject_Open(argv[2],&base));
    char portrait_path[512];CHECK(snprintf(portrait_path,sizeof(portrait_path),"%s/portrait.png",argv[2])<sizeof(portrait_path));
    FILE* file=fopen(portrait_path,"rb");CHECK(file&&fseek(file,0,SEEK_END)==0);long portrait_bytes=ftell(file);
    CHECK(portrait_bytes>0&&fseek(file,0,SEEK_SET)==0);
    uint8_t* portrait=malloc((size_t)portrait_bytes);CHECK(portrait&&fread(portrait,1,(size_t)portrait_bytes,file)==(size_t)portrait_bytes&&fclose(file)==0);
    CHECK(CreateDirectoryA(argv[3],NULL));
    for(unsigned mode=NativeFighterFamily_Single;mode<=NativeFighterFamily_SimultaneousCompanion;++mode) {
        family=(NativeFighterFamilyProject){.mode=(NativeFighterFamilyMode)mode,.member_count=mode?2:1};
        family.members[0]=base;strcpy(family.keys[0],"primary");
        strcpy(family.members[0].tile_name, "MY FIGHTER");
        if(mode){family.members[1]=base;family.members[1].script_text=(uint8_t*)partner_script;
            family.members[1].script_size=sizeof(partner_script)-1;family.members[1].parameter_text=NULL;
            family.members[1].parameter_size=0;strcpy(family.keys[1],"secondary");}
        uint8_t* packet=NULL;size_t bytes=0;NativeContentId id;
        CHECK(nativeFighterFamilyProject_Build(&family,portrait,(size_t)portrait_bytes,&packet,&bytes,&id));
        CHECK(!nativeScriptFighter_Program(Ft_Kind_CustomFirst)&&!nativeScriptFighter_Program((FighterKind)(Ft_Kind_CustomFirst+1)));
        NativeFighterPackage metadata;NativeFighterPackageData spans;
        CHECK(nativeContentStore_ParseFighter(packet,bytes,&metadata,&spans));
        CHECK(metadata.mode==family.mode&&metadata.member_count==family.member_count);
        CHECK(!strcmp(metadata.tile_name, "MY FIGHTER"));
        CHECK(!strcmp(metadata.members[0].key,"primary"));
        if(mode)CHECK(!strcmp(metadata.members[1].key,"secondary")&&spans.members[1].script_size==sizeof(partner_script)-1);
        char destination[512];CHECK(snprintf(destination,sizeof(destination),"%s/mode-%u",argv[3],mode)<sizeof(destination));
        CHECK(nativeFighterFamilyProject_CreateFromPackage(destination,metadata.title,packet,bytes));
        CHECK(nativeFighterFamilyProject_Open(destination,&opened));
        CHECK(opened.mode==family.mode&&opened.member_count==family.member_count);
        CHECK(!strcmp(opened.members[0].tile_name, "MY FIGHTER"));
        CHECK(!strcmp(opened.keys[0],"primary"));
        for(unsigned member=0;member<family.member_count;++member) {
            const NativeFighterProject* a=&family.members[member];const NativeFighterProject* b=&opened.members[member];
            CHECK(same_bytes(a->fighter_dat,a->fighter_size,b->fighter_dat,b->fighter_size));
            CHECK(same_bytes(a->animation_dat,a->animation_size,b->animation_dat,b->animation_size));
            CHECK(same_bytes(a->costume_dat,a->costume_size,b->costume_dat,b->costume_size));
            CHECK(same_bytes(a->asset_record,a->asset_record_size,b->asset_record,b->asset_record_size));
            CHECK(same_bytes(a->script_text,a->script_size,b->script_text,b->script_size));
            CHECK(same_bytes(a->parameter_text,a->parameter_size,b->parameter_text,b->parameter_size));
            CHECK(same_bytes(a->resources,a->resource_size,b->resources,b->resource_size));
        }
        if(mode)CHECK(opened.members[0].fighter_dat!=opened.members[1].fighter_dat&&
            opened.members[0].animation_dat!=opened.members[1].animation_dat&&opened.members[0].costume_dat!=opened.members[1].costume_dat);
        uint8_t* roundtrip=NULL;size_t roundtrip_bytes=0;NativeContentId rebuilt;
        CHECK(nativeFighterFamilyProject_Build(&opened,portrait,(size_t)portrait_bytes,&roundtrip,&roundtrip_bytes,&rebuilt));
        CHECK(same_bytes(packet,bytes,roundtrip,roundtrip_bytes)&&!memcmp(&id,&rebuilt,sizeof(id)));
        NativeFighterProjectInfo named;
        CHECK(nativeFighterProject_Rename(destination, "Renamed fighter"));
        CHECK(nativeFighterProject_ReadInfo(destination, &named) &&
            !strcmp(named.title, "Renamed fighter") && !strcmp(named.tile_name, "MY FIGHTER"));
        free(roundtrip);nativeFighterFamilyProject_Close(&opened);free(packet);
        if(mode) {
            family.members[1].script_text=(uint8_t*)"fighter:\n    behavior independent\nfunc Invalid():\n    not_a_real_operation()\n";
            family.members[1].script_size=strlen((const char*)family.members[1].script_text);
            packet=(uint8_t*)(uintptr_t)0x1234;bytes=123;rebuilt=id;
            CHECK(!nativeFighterFamilyProject_Build(&family,portrait,(size_t)portrait_bytes,&packet,&bytes,&rebuilt));
            CHECK(packet==(uint8_t*)(uintptr_t)0x1234&&bytes==123&&!memcmp(&id,&rebuilt,sizeof(id)));
            CHECK(!nativeScriptFighter_Program(Ft_Kind_CustomFirst)&&!nativeScriptFighter_Program((FighterKind)(Ft_Kind_CustomFirst+1)));
        }
    }
    const struct {NativeFighterFamilyMode mode;FighterKind a,b;} stock[]={
        {NativeFighterFamily_AlternateForm,Ft_Kind_Zelda,Ft_Kind_Seak},
        {NativeFighterFamily_SimultaneousCompanion,Ft_Kind_Popo,Ft_Kind_Nana}};
    for(unsigned i=0;i<2;++i) {
        char destination[512];CHECK(snprintf(destination,sizeof(destination),"%s/stock-%u",argv[3],i)<sizeof(destination));
        CHECK(nativeFighterFamilyProject_CreateStockFromDisc(destination,"Owned pair",stock[i].mode,stock[i].a,stock[i].b));
        CHECK(nativeFighterFamilyProject_Open(destination,&opened));
        CHECK(opened.mode==stock[i].mode&&opened.member_count==2);
        CHECK(opened.members[0].action_count&&opened.members[1].action_count);
        CHECK(opened.members[0].facts.joints&&opened.members[1].facts.joints);
        CHECK(opened.members[0].fighter_dat!=opened.members[1].fighter_dat);
        CHECK(opened.members[0].animation_dat!=opened.members[1].animation_dat);
        CHECK(!opened.members[0].script_size&&!opened.members[1].script_size);
        nativeFighterFamilyProject_Close(&opened);
    }
    nativeFighterProject_Close(&base);free(portrait);aurora_dvd_close();
    printf("{\"status\":\"PASS actual source family project\",\"checks\":%u,\"modes\":3,\"stock_pairs\":2,\"game\":false}\n",checks);
    return 0;
}
