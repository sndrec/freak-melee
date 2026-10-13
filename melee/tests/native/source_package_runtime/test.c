/* Real source package/cache/runtime publication. No mocked import or binder. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcommon_assets.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/lb/native_disc_archive.h>
#include <melee/gm/native_rollback_pages.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr,"source runtime %u: %s\n",__LINE__,#x);return 1; \
} } while(0)

int main(int argc,char** argv)
{
    CHECK(argc==7);
    CHECK(aurora_dvd_open(argv[3]));
    // Same real common importer as scene bootstrap; retain its actual graph.
    // This console creates no renderer/audio heap and no game actors.
    void* common_bytes=NULL;size_t common_size=0;
    CHECK(nativeDiscArchive_Read("PlCo.dat",&common_bytes,&common_size));
    void* common_arena=malloc(16*1024*1024);CHECK(common_arena);
    FtCommonAssets common;
    CHECK(ftCommonAssets_Import(common_bytes,common_size,common_arena,
                               16*1024*1024,32768,&common));
    ftPartsTable=common.parts;Fighter_804D6540=common.omissions;
    Fighter_804D64FC=&common.cpu_data.descriptor;
    NativeContentId id,missing={{0}};
    CHECK(nativeContentStore_IdFromHex(argv[2],&id));
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(nativeFighterRuntime_ValidateCached(&id,argv[1]));
    CHECK(!nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst));
    CHECK(nativeFighterRuntime_StageCached(&id,1,argv[1],false));
    CHECK(!nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst));
    CHECK(nativeFighterRuntime_ActivateStaged(1));
    const NativeSourcePackage* accepted=nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst);
    CHECK(accepted&&accepted->ready&&accepted->target==Ft_Kind_CustomFirst);
    CHECK(nativeFighterRuntime_Definition(Ft_Kind_CustomFirst)==NULL);
    CHECK(nativeScriptFighter_Program(Ft_Kind_CustomFirst)->bank==&accepted->compiler.bank);
    CHECK(nativeScriptFighter_Program(Ft_Kind_CustomFirst)->identity==accepted->program.identity);
    CHECK(gFtDataList[Ft_Kind_CustomFirst]==&accepted->assets.character.data);
    CHECK(ftData_CharacterStateTables[Ft_Kind_CustomFirst]==accepted->assets.motions);
    CHECK(ftData_803C2468[Ft_Kind_CustomFirst]==&accepted->assets.demos);
    unsigned functions=(unsigned)strtoul(argv[4],NULL,10);
    unsigned items=(unsigned)strtoul(argv[5],NULL,10);
    unsigned effects=(unsigned)strtoul(argv[6],NULL,10);
    if(accepted->program.items.count!=items||accepted->program.effects.count!=effects)
        fprintf(stderr,"Prepared package: %u functions, %u Items, %u Effects; expected %u/%u/%u\n",
            accepted->compiler.bank.declarations.function_count,accepted->program.items.count,
            accepted->program.effects.count,functions,items,effects);
    CHECK(accepted->compiler.bank.declarations.function_count==functions);
    CHECK(accepted->program.items.count==items&&accepted->program.effects.count==effects);
    CHECK(!nativeFighterRuntime_StageCached(&missing,1,argv[1],false));
    CHECK(nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst)==accepted);
    CHECK(nativeScriptFighter_Program(Ft_Kind_CustomFirst)->bank==&accepted->compiler.bank);
    CHECK(nativeSourcePackage_Pin((NativeSourcePackage*)accepted));
    CHECK(!nativeFighterRuntime_CanClear()&&!nativeFighterRuntime_Clear());
    CHECK(nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst)==accepted);
    CHECK(gFtDataList[Ft_Kind_CustomFirst]==&accepted->assets.character.data);
    CHECK(nativeSourcePackage_Unpin((NativeSourcePackage*)accepted));

    SYSTEM_INFO info;GetSystemInfo(&info);
    uint8_t* tracked=VirtualAlloc(NULL,info.dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    CHECK(tracked);
    NativeRollbackPagedRegion region={tracked,info.dwPageSize};
    CHECK(nativeRollbackPages_Init(&region,1,NULL,0,2,2));
    CHECK(nativeRollbackPages_IsInitialized());
    CHECK(nativeRollbackPages_BeginFrame(0));tracked[0]=1;
    CHECK(nativeRollbackPages_EndFrame());
    CHECK(!nativeFighterRuntime_CanClear()&&!nativeFighterRuntime_Clear());
    CHECK(nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst)==accepted);
    CHECK(nativeScriptFighter_Program(Ft_Kind_CustomFirst)->bank==&accepted->compiler.bank);
    nativeRollbackPages_Shutdown();
    CHECK(!nativeRollbackPages_IsInitialized());
    CHECK(VirtualFree(tracked,0,MEM_RELEASE));
    CHECK(nativeFighterRuntime_CanClear()&&nativeFighterRuntime_Clear());
    CHECK(!nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst));
    CHECK(!gFtDataList[Ft_Kind_CustomFirst]);
    CHECK(!ftData_803C2468[Ft_Kind_CustomFirst]);
    ftPartsTable=NULL;Fighter_804D6540=NULL;Fighter_804D64FC=NULL;
    free(common_arena);_aligned_free(common_bytes);
    aurora_dvd_close();
    printf("{\"status\":\"PASS source runtime\",\"checks\":%u,\"real_publication\":true,\"actors_created\":false}\n",checks);
    return 0;
}
