#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_fighter_family_project.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/fighter.h>
#include <melee/gm/native_rollback_pages.h>
#include <melee/lb/native_disc_archive.h>
#include <melee/pl/player.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static NativeFighterFamilyProject family;
static NativeFighterFamilyProject full_companion;
static NativeFighterCpuProfile expected_cpu[2];
static NativeContentId full_companion_id;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"family runtime %u: %s\n",__LINE__,#x);return 1;}}while(0)
static const char activation[]="\nstate int activation_count\naction FamilyLife:\n    on form_activated:\n        state.activation_count = state.activation_count + 1\n";

static int companion_publication(CharacterKind character,FighterKind first)
{
    const NativeSourcePackage* a=nativeFighterRuntime_SourcePackage(first);
    const NativeSourcePackage* b=nativeFighterRuntime_SourcePackage((FighterKind)(first+1));
    CHECK(a&&b&&a!=b&&a->leases==1&&b->leases==1);
    CHECK(a->input_memory!=b->input_memory&&a->compiler.arena!=b->compiler.arena&&
        a->compiler.bank.source_storage!=b->compiler.bank.source_storage);
    CHECK(a->assets.character.data.x0!=b->assets.character.data.x0&&a->assets.motions!=b->assets.motions);
    const NativeSourcePackage* members[2]={a,b};
    ftCo_AttackEntry** rows[FT_CPU_ATTACK_FAMILIES]={Fighter_804D64FC->x4,Fighter_804D64FC->x8,
        Fighter_804D64FC->xC,Fighter_804D64FC->x10,Fighter_804D64FC->x14,Fighter_804D64FC->x18,Fighter_804D64FC->x1C};
    for(unsigned m=0;m<2;++m){
        FighterKind kind=(FighterKind)(first+m);const NativeSourcePackage* p=members[m];
        const NativeScriptFighterProgram* program=nativeScriptFighter_Program(kind);
        CHECK(program&&program!=&p->program&&program->identity==p->program.identity&&program->bank==&p->compiler.bank);
        CHECK(p->owned_input.family==NativeSourcePackage_Single&&p->owned_input.source_bytes==full_companion.members[m].script_size);
        CHECK(!memcmp(p->owned_input.source,full_companion.members[m].script_text,p->owned_input.source_bytes));
        CHECK(program->hooks[NativeScriptFighter_FormActivated]==UINT32_MAX);
        CHECK(gFtDataList[kind]==&p->assets.character.data&&ftData_CharacterStateTables[kind]==p->assets.motions);
        CHECK(ftPartsTable[kind]==&p->assets.parts&&!nativeFighterRuntime_Definition(kind)&&!nativeScriptFighter_HasActors(kind));
        CHECK(!memcmp(&Fighter_804D64FC->x20[kind],&expected_cpu[m].distance_threshold,sizeof(float)));
        for(unsigned f=0;f<FT_CPU_ATTACK_FAMILIES;++f){
            CHECK(rows[f][kind]&&rows[f][first]!=rows[f][first+1]);
            CHECK(!memcmp(rows[f][kind],expected_cpu[m].attacks[f],sizeof(expected_cpu[m].attacks[f])));
        }
    }
    CHECK(Player_800325C8(character,false)==first&&Player_800325C8(character,true)==first+1);
    Player_SetPlayerCharacter(0,character);CHECK(!Player_HasAlternateMember(0));
    CHECK(!nativeFighterRuntime_AlternatePair(first,(FighterKind)(first+1))&&
        !nativeFighterRuntime_AlternatePair((FighterKind)(first+1),first));
    return 0;
}

static int full_companion_publication(const char* draft,const char* portrait_path,
                                      const char* cache,const NativeContentId* alternate)
{
    CHECK(nativeFighterFamilyProject_Open(draft,&full_companion));
    CHECK(full_companion.mode==NativeFighterFamily_SimultaneousCompanion&&full_companion.member_count==2);
    CHECK(full_companion.members[0].script_size>50000&&full_companion.members[1].script_size>50000);
    for(unsigned m=0;m<2;++m)CHECK(nativeFighterCpu_Parse((const char*)full_companion.members[m].cpu_text,
        full_companion.members[m].cpu_size,&expected_cpu[m]));
    FILE* f=fopen(portrait_path,"rb");CHECK(f&&fseek(f,0,SEEK_END)==0);long size=ftell(f);
    CHECK(size>0&&fseek(f,0,SEEK_SET)==0);uint8_t* portrait=malloc((size_t)size);CHECK(portrait);
    CHECK(fread(portrait,1,(size_t)size,f)==(size_t)size&&fclose(f)==0);
    uint8_t* packet=NULL;size_t bytes=0;
    CHECK(nativeFighterFamilyProject_Build(&full_companion,portrait,(size_t)size,&packet,&bytes,&full_companion_id));
    CHECK(nativeContentStore_Cache(cache,packet,bytes,NULL,&full_companion_id));
    NativeFighterPackage metadata;NativeFighterPackageData data;CHECK(nativeContentStore_ParseFighter(packet,bytes,&metadata,&data));
    const uint8_t* source=data.members[1].script_text;size_t source_bytes=data.members[1].script_size;
    static const char invalid_source[]="fighter:\n    behavior independent\naction Main:\n    on load:\n        missing_companion_operation()\n";
    data.members[1].script_text=(const uint8_t*)invalid_source;data.members[1].script_size=sizeof(invalid_source)-1;
    uint8_t* bad_packet=NULL;size_t bad_bytes=0;NativeContentId bad_source,bad_cpu;
    CHECK(nativeContentStore_BuildFighter(&metadata,&data,&bad_packet,&bad_bytes,&bad_source));
    CHECK(nativeContentStore_Cache(cache,bad_packet,bad_bytes,NULL,&bad_source));free(bad_packet);
    data.members[1].script_text=source;data.members[1].script_size=source_bytes;
    static const char invalid_cpu[]="invalid CPU profile\n";
    data.members[1].cpu_text=(const uint8_t*)invalid_cpu;data.members[1].cpu_size=sizeof(invalid_cpu)-1;
    CHECK(nativeContentStore_BuildFighter(&metadata,&data,&bad_packet,&bad_bytes,&bad_cpu));
    CHECK(nativeContentStore_Cache(cache,bad_packet,bad_bytes,NULL,&bad_cpu));free(bad_packet);free(packet);free(portrait);
    CHECK(nativeFighterRuntime_ValidateCached(&full_companion_id,cache));
    CHECK(!nativeFighterRuntime_ValidateCached(&bad_source,cache)&&!nativeFighterRuntime_ValidateCached(&bad_cpu,cache));
    CHECK(nativeFighterRuntime_StageCached(&full_companion_id,1,cache,false));
    CHECK(!nativeFighterRuntime_StageCached(&bad_source,1,cache,false)&&!nativeFighterRuntime_StageCached(&bad_cpu,1,cache,false));
    CHECK(!nativeScriptFighter_Program(Ft_Kind_CustomFirst)&&!nativeScriptFighter_Program(Ft_Kind_CustomFirst+1));
    CHECK(!nativeFighterRuntime_ActivateStaged(2)&&nativeFighterRuntime_ActivateStaged(1));
    CHECK(companion_publication(ChKind_CustomFirst,Ft_Kind_CustomFirst)==0);
    const NativeSourcePackage* secondary=nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst+1);
    CHECK(nativeSourcePackage_Pin((NativeSourcePackage*)secondary));CHECK(!nativeFighterRuntime_CanClear()&&!nativeFighterRuntime_Clear());
    CHECK(nativeSourcePackage_Unpin((NativeSourcePackage*)secondary));
    NativeContentId mixed[2]={full_companion_id,*alternate};CHECK(nativeFighterRuntime_StageCached(mixed,2,cache,false));
    SYSTEM_INFO system;GetSystemInfo(&system);uint8_t* tracked=VirtualAlloc(NULL,system.dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(tracked);
    NativeRollbackPagedRegion region={tracked,system.dwPageSize};CHECK(nativeRollbackPages_Init(&region,1,NULL,0,2,2));
    CHECK(nativeRollbackPages_BeginFrame(0));tracked[0]=1;CHECK(nativeRollbackPages_EndFrame());
    CHECK(!nativeFighterRuntime_CanClear()&&!nativeFighterRuntime_Clear()&&!nativeFighterRuntime_ActivateStaged(2));
    nativeRollbackPages_Shutdown();CHECK(VirtualFree(tracked,0,MEM_RELEASE));
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(!gFtDataList[Ft_Kind_CustomFirst]&&!gFtDataList[Ft_Kind_CustomFirst+1]&&
        !nativeScriptFighter_Program(Ft_Kind_CustomFirst)&&!nativeScriptFighter_Program(Ft_Kind_CustomFirst+1));
    CHECK(Player_800325C8(ChKind_CustomFirst,false)==-1&&Player_800325C8(ChKind_CustomFirst,true)==-1);
    CHECK(nativeFighterRuntime_ActivateStaged(2));CHECK(companion_publication(ChKind_CustomFirst,Ft_Kind_CustomFirst)==0);
    CHECK(nativeFighterRuntime_AlternatePair(Ft_Kind_CustomFirst+2,Ft_Kind_CustomFirst+3));
    Player_SetPlayerCharacter(1,ChKind_CustomFirst+1);CHECK(Player_HasAlternateMember(1));
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(!nativeFighterRuntime_Load(&full_companion_id,ChKind_CustomFirst,Ft_Kind_RuntimeCapacity-1,cache,false,false));
    CHECK(nativeFighterRuntime_Load(&full_companion_id,ChKind_CustomFirst+2,Ft_Kind_CustomFirst+4,cache,false,false));
    CHECK(companion_publication(ChKind_CustomFirst+2,Ft_Kind_CustomFirst+4)==0);
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    nativeFighterFamilyProject_Close(&full_companion);return 0;
}

int main(int argc,char** argv)
{
    CHECK(argc==7&&aurora_dvd_open(argv[1]));
    void* common_bytes=NULL;size_t common_size=0;
    CHECK(nativeDiscArchive_Read("PlCo.dat",&common_bytes,&common_size));
    void* common_arena=malloc(16*1024*1024);CHECK(common_arena);FtCommonAssets common;
    CHECK(ftCommonAssets_Import(common_bytes,common_size,common_arena,16*1024*1024,32768,&common));
    ftPartsTable=common.parts;Fighter_804D6540=common.omissions;Fighter_804D64FC=&common.cpu_data.descriptor;
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(nativeFighterFamilyProject_Open(argv[2],&family)&&family.member_count==2);
    uint8_t* original_source[2];size_t original_bytes[2];uint8_t* authored[2];
    for(unsigned m=0;m<2;++m) {
        original_source[m]=family.members[m].script_text;original_bytes[m]=family.members[m].script_size;
        authored[m]=malloc(original_bytes[m]+sizeof(activation));CHECK(authored[m]);
        memcpy(authored[m],original_source[m],original_bytes[m]);memcpy(authored[m]+original_bytes[m],activation,sizeof(activation));
        family.members[m].script_text=authored[m];family.members[m].script_size=original_bytes[m]+sizeof(activation)-1;
    }
    FILE* file=fopen(argv[3],"rb");CHECK(file&&fseek(file,0,SEEK_END)==0);long portrait_bytes=ftell(file);
    CHECK(portrait_bytes>0&&fseek(file,0,SEEK_SET)==0);uint8_t* portrait=malloc((size_t)portrait_bytes);
    CHECK(portrait&&fread(portrait,1,(size_t)portrait_bytes,file)==(size_t)portrait_bytes&&fclose(file)==0);
    uint8_t* packet=NULL;size_t bytes=0;NativeContentId good,missing_hook,bad,companion,single;
    family.mode=NativeFighterFamily_AlternateForm;
    CHECK(nativeFighterFamilyProject_Build(&family,portrait,(size_t)portrait_bytes,&packet,&bytes,&good));
    CHECK(nativeContentStore_Cache(argv[4],packet,bytes,NULL,&good));
    NativeFighterPackage metadata;NativeFighterPackageData data;
    CHECK(nativeContentStore_ParseFighter(packet,bytes,&metadata,&data));
    uint8_t* other=NULL;size_t other_bytes=0;
    const uint8_t* valid_second=data.members[1].script_text;size_t valid_second_bytes=data.members[1].script_size;
    data.members[1].script_text=original_source[1];data.members[1].script_size=original_bytes[1];
    CHECK(nativeContentStore_BuildFighter(&metadata,&data,&other,&other_bytes,&missing_hook));
    CHECK(nativeContentStore_Cache(argv[4],other,other_bytes,NULL,&missing_hook));free(other);
    static const char invalid[]="fighter:\n    behavior independent\naction Main:\n    on load:\n        unsupported_operation()\n";
    data.members[1].script_text=(const uint8_t*)invalid;data.members[1].script_size=sizeof(invalid)-1;
    CHECK(nativeContentStore_BuildFighter(&metadata,&data,&other,&other_bytes,&bad));
    CHECK(nativeContentStore_Cache(argv[4],other,other_bytes,NULL,&bad));free(other);
    data.members[1].script_text=valid_second;data.members[1].script_size=valid_second_bytes;
    metadata.mode=NativeFighterFamily_SimultaneousCompanion;
    data.members[0].script_text=original_source[0];data.members[0].script_size=original_bytes[0];
    data.members[1].script_text=original_source[1];data.members[1].script_size=original_bytes[1];
    CHECK(nativeContentStore_BuildFighter(&metadata,&data,&other,&other_bytes,&companion));
    CHECK(nativeContentStore_Cache(argv[4],other,other_bytes,NULL,&companion));free(other);
    metadata.mode=NativeFighterFamily_Single;metadata.member_count=1;memset(&metadata.members[1],0,sizeof(metadata.members[1]));
    data.members[1]=(NativeFighterMemberData){0};
    CHECK(nativeContentStore_BuildFighter(&metadata,&data,&other,&other_bytes,&single));
    CHECK(nativeContentStore_Cache(argv[4],other,other_bytes,NULL,&single));free(other);free(packet);
    CHECK(nativeFighterRuntime_ValidateCached(&good,argv[4]));
    CHECK(!nativeFighterRuntime_ValidateCached(&missing_hook,argv[4]));
    CHECK(!nativeFighterRuntime_ValidateCached(&bad,argv[4]));
    CHECK(nativeFighterRuntime_ValidateCached(&companion,argv[4]));
    CHECK(nativeFighterRuntime_StageCached(&good,1,argv[4],false));
    CHECK(!nativeFighterRuntime_StageCached(&bad,1,argv[4],false));
    CHECK(!nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst)&&!nativeFighterRuntime_SourcePackage((FighterKind)(Ft_Kind_CustomFirst+1)));
    CHECK(!nativeFighterRuntime_ActivateStaged(2));CHECK(nativeFighterRuntime_ActivateStaged(1));
    const NativeSourcePackage* a=nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst);
    const NativeSourcePackage* b=nativeFighterRuntime_SourcePackage((FighterKind)(Ft_Kind_CustomFirst+1));
    CHECK(a&&b&&a!=b&&a->leases==1&&b->leases==1&&a->compiler.arena!=b->compiler.arena);
    CHECK(a->input_memory!=b->input_memory&&a->assets.character.data.x0!=b->assets.character.data.x0);
    CHECK(a->program.hooks[NativeScriptFighter_FormActivated]!=UINT32_MAX&&b->program.hooks[NativeScriptFighter_FormActivated]!=UINT32_MAX);
    CHECK(nativeScriptFighter_Program(Ft_Kind_CustomFirst)->bank==&a->compiler.bank);
    CHECK(nativeScriptFighter_Program((FighterKind)(Ft_Kind_CustomFirst+1))->bank==&b->compiler.bank);
    CHECK(gFtDataList[Ft_Kind_CustomFirst]==&a->assets.character.data&&gFtDataList[Ft_Kind_CustomFirst+1]==&b->assets.character.data);
    CHECK(ftData_CharacterStateTables[Ft_Kind_CustomFirst]==a->assets.motions&&ftData_CharacterStateTables[Ft_Kind_CustomFirst+1]==b->assets.motions);
    CHECK(Player_800325C8(ChKind_CustomFirst,false)==Ft_Kind_CustomFirst&&Player_800325C8(ChKind_CustomFirst,true)==Ft_Kind_CustomFirst+1);
    Player_SetPlayerCharacter(0,ChKind_CustomFirst);CHECK(Player_HasAlternateMember(0));
    CHECK(nativeFighterRuntime_AlternatePair(Ft_Kind_CustomFirst,(FighterKind)(Ft_Kind_CustomFirst+1)));
    CHECK(nativeFighterRuntime_AlternatePair((FighterKind)(Ft_Kind_CustomFirst+1),Ft_Kind_CustomFirst));
    CHECK(!nativeFighterRuntime_AlternatePair(Ft_Kind_CustomFirst,Ft_Kind_CustomFirst));
    CHECK(!nativeFighterRuntime_AlternatePair(Ft_Kind_CustomFirst,(FighterKind)(Ft_Kind_CustomFirst+2)));
    CHECK(!nativeFighterRuntime_Load(&single,(CharacterKind)(ChKind_CustomFirst+1),(FighterKind)(Ft_Kind_CustomFirst+1),argv[4],false,false));
    CHECK(nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst)==a&&nativeFighterRuntime_SourcePackage((FighterKind)(Ft_Kind_CustomFirst+1))==b);
    CHECK(nativeSourcePackage_Pin((NativeSourcePackage*)b));CHECK(!nativeFighterRuntime_CanClear()&&!nativeFighterRuntime_Clear());
    CHECK(nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst)==a&&nativeFighterRuntime_SourcePackage((FighterKind)(Ft_Kind_CustomFirst+1))==b);
    CHECK(nativeSourcePackage_Unpin((NativeSourcePackage*)b));
    CHECK(nativeFighterRuntime_StageCached(&good,1,argv[4],false));
    CHECK(!nativeFighterRuntime_StageCached(&missing_hook,1,argv[4],false));
    SYSTEM_INFO system;GetSystemInfo(&system);uint8_t* tracked=VirtualAlloc(NULL,system.dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(tracked);
    NativeRollbackPagedRegion region={tracked,system.dwPageSize};CHECK(nativeRollbackPages_Init(&region,1,NULL,0,2,2));
    CHECK(nativeRollbackPages_BeginFrame(0));tracked[0]=1;CHECK(nativeRollbackPages_EndFrame());
    CHECK(!nativeFighterRuntime_CanClear()&&!nativeFighterRuntime_Clear()&&!nativeFighterRuntime_ActivateStaged(1));
    nativeRollbackPages_Shutdown();CHECK(VirtualFree(tracked,0,MEM_RELEASE));
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(!gFtDataList[Ft_Kind_CustomFirst]&&!gFtDataList[Ft_Kind_CustomFirst+1]);
    CHECK(!nativeFighterRuntime_AlternatePair(Ft_Kind_CustomFirst,(FighterKind)(Ft_Kind_CustomFirst+1)));
    CHECK(Player_800325C8(ChKind_CustomFirst,false)==-1&&Player_800325C8(ChKind_CustomFirst,true)==-1);
    CHECK(nativeFighterRuntime_ActivateStaged(1));CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(!nativeFighterRuntime_Load(&good,ChKind_CustomFirst,(FighterKind)(Ft_Kind_RuntimeCapacity-1),argv[4],false,false));
    CHECK(nativeFighterRuntime_Load(&good,(CharacterKind)(ChKind_CustomFirst+1),(FighterKind)(Ft_Kind_CustomFirst+2),argv[4],false,false));
    CHECK(nativeFighterRuntime_AlternatePair((FighterKind)(Ft_Kind_CustomFirst+2),(FighterKind)(Ft_Kind_CustomFirst+3)));
    CHECK(nativeFighterRuntime_SourcePackage((FighterKind)(Ft_Kind_CustomFirst+2))->leases==1&&nativeFighterRuntime_SourcePackage((FighterKind)(Ft_Kind_CustomFirst+3))->leases==1);
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(nativeFighterRuntime_StageCached(&companion,1,argv[4],false)&&nativeFighterRuntime_ActivateStaged(1));
    CHECK(!nativeFighterRuntime_AlternatePair(Ft_Kind_CustomFirst,Ft_Kind_CustomFirst+1));
    Player_SetPlayerCharacter(0,ChKind_CustomFirst);CHECK(!Player_HasAlternateMember(0));
    CHECK(nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst+1)->program.hooks[NativeScriptFighter_FormActivated]==UINT32_MAX);
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(full_companion_publication(argv[5],argv[6],argv[4],&good)==0);
    for(unsigned m=0;m<2;++m){family.members[m].script_text=original_source[m];family.members[m].script_size=original_bytes[m];free(authored[m]);}
    nativeFighterFamilyProject_Close(&family);free(portrait);
    ftPartsTable=NULL;Fighter_804D6540=NULL;Fighter_804D64FC=NULL;free(common_arena);_aligned_free(common_bytes);aurora_dvd_close();
    char full_id[65];nativeContentStore_IdHex(&full_companion_id,full_id);
    printf("{\"status\":\"PASS alternate and companion source family publication\",\"checks\":%u,\"actors_created\":false,\"full_companion_id\":\"%s\"}\n",checks,full_id);return 0;
}
