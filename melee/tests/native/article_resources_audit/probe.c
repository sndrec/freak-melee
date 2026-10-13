#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/it/itarticle_data.h>
#include <melee/it/itCharItems.h>
#include <stdio.h>
#include <stdlib.h>

static FtCharacterAssets assets;
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line%u %s import=%u offset=%lu\n",__LINE__,#x,assets.import.status,assets.import.error_offset); return 1; } } while(0)
static uint32_t hash(const char* s)
{uint32_t h=UINT32_C(2166136261);for(;*s;++s){h^=(unsigned char)*s;h*=UINT32_C(16777619);}return h;}
int main(int argc,char** argv)
{
    CHECK(argc==2);FILE* f=fopen(argv[1],"rb");CHECK(f&&fseek(f,0,SEEK_END)==0);
    long bytes=ftell(f);CHECK(bytes>32&&fseek(f,0,SEEK_SET)==0);
    void* dat=malloc((size_t)bytes);void* arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(dat&&arena);
    CHECK(fread(dat,1,(size_t)bytes,f)==(size_t)bytes&&fclose(f)==0);
    const FtCharacterAssetLayout layout={.article_mask=15,.item_format=FtCharacterItems_Chain};
    CHECK(ftCharacterAssets_Import(dat,(size_t)bytes,&layout,"ftDataSeak",317,14,1,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,&assets));
    Article* article=assets.fighter_items[3];CHECK(article&&article->x4_specialAttributes&&article->native_model_root_count==0);
    const itSeakChain_Attrs* attrs=article->x4_specialAttributes;
    printf("observed count=%ld link=%p tip=%p\n",attrs->x0,(void*)attrs->x64_joint,(void*)attrs->x68_joint);
    CHECK(attrs->x0>=2&&attrs->x0<=30&&attrs->x64_joint&&attrs->x68_joint);
    u32 link,tip;const char* external;u32 source=article->native_special_attribute_source;
    CHECK(HSD_DatResolve(&assets.dat,source+100,&link,&external)==HSD_DAT_REF_LOCAL);
    CHECK(HSD_DatResolve(&assets.dat,source+104,&tip,&external)==HSD_DAT_REF_LOCAL);
    const NativeFighterArticleModelRoot roots[]={{hash("Link"),100},{hash("Tip"),104}};
    CHECK(itArticle_ImportModelRoots(&assets.import,article,roots,2));
    CHECK(article->native_model_root_count==2&&article->native_model_roots[0].root==attrs->x64_joint&&article->native_model_roots[1].root==attrs->x68_joint);
    printf("{\"status\":\"PASS owned Chain roots\",\"checks\":%u,\"link_count\":%ld,\"special_attribute_source\":%lu,\"link_target\":%lu,\"tip_target\":%lu,\"roots_match_owned_graphs\":true,\"arena_used\":%zu,\"records\":%lu}\n",checks,attrs->x0,source,link,tip,assets.import.arena_used,assets.import.record_count);
    free(arena);free(dat);return 0;
}
