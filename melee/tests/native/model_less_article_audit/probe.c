#include <melee/ft/ftcharacter_assets.h>
#include <melee/it/itarticle_data.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FtCharacterAssets assets;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"probe %u: %s (status%d offset0x%lX)\n",__LINE__,#x,assets.import.status,assets.import.error_offset);return 1;}}while(0)
int main(int argc,char** argv)
{
    CHECK(argc==2);FILE* file=fopen(argv[1],"rb");CHECK(file&&fseek(file,0,SEEK_END)==0);long n=ftell(file);CHECK(n>=32&&fseek(file,0,SEEK_SET)==0);
    void* bytes=malloc((size_t)n);void* arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(bytes&&arena);CHECK(fread(bytes,1,(size_t)n,file)==(size_t)n&&fclose(file)==0);
    const FtCharacterAssetLayout layout={.article_mask=3};
    CHECK(ftCharacterAssets_Import(bytes,(size_t)n,&layout,"ftDataZelda",311,14,1,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,&assets));
    u32 root,table;const char* external;CHECK(HSD_DatFindRoot(&assets.dat,"ftDataZelda",&root));CHECK(HSD_DatResolve(&assets.dat,root+0x48,&table,&external)==HSD_DAT_REF_LOCAL);
    for(unsigned slot=0;slot<2;++slot){u32 article_source,model_source,joint_source;
        CHECK(HSD_DatResolve(&assets.dat,table+slot*4,&article_source,&external)==HSD_DAT_REF_LOCAL);
        CHECK(HSD_DatResolve(&assets.dat,article_source+16,&model_source,&external)==HSD_DAT_REF_LOCAL);
        HSD_DatReferenceKind root_kind=HSD_DatResolve(&assets.dat,model_source,&joint_source,&external);CHECK(root_kind==HSD_DAT_REF_NULL);
        const Article* article=assets.fighter_items[slot];CHECK(article&&article->x0_common_attr&&article->x10_modelDesc&&article->xC_itemStates);
        CHECK(article->x10_modelDesc->x0_joint==NULL&&article->x10_modelDesc->x4_bone_count==0&&article->x14_dynamics==NULL);
        unsigned animations=0,materials=0,shapes=0,scripts=0;
        for(unsigned i=0;i<article->xC_itemStates->count;++i){const ItemStateDesc* state=&article->xC_itemStates->x0_itemStateDesc[i];animations+=state->x0_anim_joint!=NULL;materials+=state->x4_matanim_joint!=NULL;shapes+=state->x8_parameters!=NULL;scripts+=state->xC_script!=NULL;}
        printf("{\"slot\":%u,\"article_offset\":%lu,\"model_offset\":%lu,\"raw_root_kind\":%u,\"imported_root_null\":true,\"bone_count\":%lu,\"dynamics_null\":true,\"states\":%lu,\"animations\":%u,\"materials\":%u,\"shapes\":%u,\"scripts\":%u}\n",slot,article_source,model_source,(unsigned)root_kind,article->x10_modelDesc->x4_bone_count,article->xC_itemStates->count,animations,materials,shapes,scripts);
    }
    printf("{\"status\":\"PASS actual model-less Article import\",\"checks\":%u,\"arena_used\":%zu,\"import_records\":%lu}\n",checks,assets.import.arena_used,assets.import.record_count);free(arena);free(bytes);return 0;
}
