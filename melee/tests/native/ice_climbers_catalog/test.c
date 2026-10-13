#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/kinds/ftPopo/ftpopo_clone.h>
#include <melee/ft/kinds/ftNana/ftnana_clone.h>
#include <melee/ft/kinds/ftPopo/forward.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/it/itCharItems.h>
#include <sysdolphin/baselib/jobj.h>

static unsigned checks;
#define CHECK(c) do { ++checks; if(!(c)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#c);exit(1);} } while(0)
#include "stock_tables.inc"
#include "expected.inc"

static void* read_file(const char* dir,const char* name,size_t* size)
{
    char path[1024];snprintf(path,sizeof(path),"%s/%s",dir,name);
    FILE* f=fopen(path,"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long length=ftell(f);
    CHECK(length>0);rewind(f);void* bytes=malloc((size_t)length);CHECK(bytes);
    CHECK(fread(bytes,1,(size_t)length,f)==(size_t)length);fclose(f);*size=(size_t)length;return bytes;
}

static void metadata(const NativeFighterDefinition* d,const unsigned char* map,int nana)
{
    CHECK(d->source_kind==(nana?Ft_Kind_Nana:Ft_Kind_Popo));
    CHECK(d->animation_source_kind==d->source_kind);
    CHECK(d->action_count==321&&d->demo_count==14&&d->joint_count==49);
    CHECK(d->sound_banks==(UINT64_C(1)<<13)&&d->sound_mute_channels==0x44&&d->effect_bank==14);
    CHECK(d->bone_count==40&&d->bones&&d->omission_count==0);
    for(unsigned role=0;role<54;++role){
        unsigned found=0;
        for(unsigned i=0;i<d->bone_count;++i)if(d->bones[i].part==role){
            CHECK(d->bones[i].joint==map[role]);++found;
        }
        CHECK(found==(map[role]!=255));
    }
    CHECK(d->asset_layout.article_mask==7&&d->asset_layout.joint_graph_mask==0&&
          d->asset_layout.visibility_mask==0&&d->asset_layout.byte_range_count==0&&
          d->asset_layout.item_format==FtCharacterItems_ClimberString);
    CHECK(ftCharacterAssetLayout_IsValid(&d->asset_layout));
    CHECK(d->article_asset_count==3);
    const ItemKind kinds[]={It_Kind_IceClimber_Ice,It_Kind_IceClimber_Blizzard,It_Kind_IceClimber_GumStrings};
    for(unsigned i=0;i<3;++i){CHECK(d->article_assets[i].kind==kinds[i]);
        CHECK(d->article_assets[i].slot==i&&d->article_assets[i].kind_attribute_offset==-1);}
    CHECK(d->motion_state_first==341&&d->motion_state_count==26);
    CHECK(d->motion_states==(nana?ftNn_Init_MotionStateTable:ftPp_Init_MotionStateTable));
    unsigned phases=0;
    for(unsigned i=0;i<26;++i){MotionState* m=&d->motion_states[i];
        phases+=(m->anim_cb!=NULL)+(m->input_cb!=NULL)+(m->phys_cb!=NULL)+(m->coll_cb!=NULL);
        if(i==22||i==25)CHECK(!m->anim_cb&&!m->input_cb&&!m->phys_cb&&!m->coll_cb);
    }
    CHECK(phases==96);CHECK(d->special_n&&d->special_air_n&&d->special_lw&&d->special_air_lw);
    CHECK((d->special_s!=NULL)==!nana&&(d->special_air_s!=NULL)==!nana);
    CHECK((d->special_hi!=NULL)==!nana&&(d->special_air_hi!=NULL)==!nana);
    const int ground[]={341,nana?0:343,nana?0:347,357};
    const int air[]={342,nana?0:345,nana?0:352,358};
    for(unsigned i=0;i<4;++i){CHECK(d->special_ground_state[i]==ground[i]);CHECK(d->special_air_state[i]==air[i]);}
    CHECK(!strcmp(d->demo_motion_names->vi_wait_filename,"ftDemoViWaitMotionFilePopo"));
}

static void imports(const char* directory,const NativeFighterDefinition* d,
                    const char* fighter_name,const char* bank_name,
                    const char* costumes[4][3],unsigned expected_clips)
{
    size_t size;void* bytes=read_file(directory,fighter_name,&size);
    void* arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(arena);FtCharacterAssets assets;
    CHECK(ftCharacterAssets_Import(bytes,size,&d->asset_layout,d->fighter_root,d->action_count,
          d->demo_count,4,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,&assets));
    CHECK(assets.model_data.x0.costume_count>=4&&assets.model_data.x8.costume_count>=4);
    CHECK(assets.article_stats.article_count==3&&assets.action_stats.entry_count>0);
    printf("%s unresolved occurrences=%lu\n",d->code_key,assets.import.unresolved_external_count);
    for(unsigned i=0;i<3;++i){Article* article=assets.fighter_items[d->article_assets[i].slot];
        CHECK(article&&article->x4_specialAttributes&&article->x10_modelDesc);
    }
    itClimbersStringAttributes* rope=((Article*)assets.fighter_items[2])->x4_specialAttributes;
    CHECK(rope->x0_count==40);
    if(d->source_kind==Ft_Kind_Nana){
        // Retail secondary article graphs are external, not local offsets.
        // This raw copied-base import must retain that incomplete ownership.
        CHECK(!rope->x24_joint&&!rope->x28_joint);
        // Generic article import and typed String import visit the two String
        // references independently: four symbols, six unresolved occurrences.
        CHECK(assets.import.unresolved_external_count==6);
        u32 root,items,article,special,target;const char* external;
        CHECK(HSD_DatFindRoot(&assets.dat,d->fighter_root,&root));
        CHECK(HSD_DatReadU32(&assets.dat,root+0x48,&items));
        CHECK(HSD_DatReadU32(&assets.dat,items+8,&article));
        CHECK(HSD_DatReadU32(&assets.dat,article+4,&special));
        CHECK(HSD_DatResolve(&assets.dat,special+0x24,&target,&external)==HSD_DAT_REF_EXTERNAL);
        CHECK(!strcmp(external,"ItmIceclimberGum_TopN_joint"));
        CHECK(HSD_DatResolve(&assets.dat,special+0x28,&target,&external)==HSD_DAT_REF_EXTERNAL);
        CHECK(!strcmp(external,"ItmIceclimberGum_TopN_joint"));
    }else CHECK(rope->x24_joint&&rope->x28_joint&&assets.import.unresolved_external_count==1);
    size_t bank_size;void* bank=read_file(directory,bank_name,&bank_size);
    void* animation_arena=malloc(FT_ANIMATION_IMPORT_ARENA_SIZE);CHECK(animation_arena);
    FtAnimationBankStats stats;
    CHECK(ftAnimationBank_Import(bank, bank_size,
                (const FtAnimationBankTable[]){{ assets.data.xC, d->action_count, false }}, 1,
                animation_arena, FT_ANIMATION_IMPORT_ARENA_SIZE, &stats));
    CHECK(stats.clip_count==expected_clips);
    unsigned present=0;for(unsigned i=0;i<321;++i)present+=assets.data.xC[i].x14!=NULL;
    CHECK(present==expected_clips);
    if(d->source_kind==Ft_Kind_Nana){CHECK(present==8);CHECK(!assets.data.xC[0].x14);}
    else CHECK(assets.data.xC[0].x14!=NULL);
    for(unsigned i=0;i<4;++i){size_t costume_size;void* raw=read_file(directory,costumes[i][0],&costume_size);
        void* costume_arena=malloc(FT_COSTUME_IMPORT_ARENA_SIZE);CHECK(costume_arena);FtCostumeAssets costume;
        CHECK(ftCostumeAssets_Import(raw,costume_size,costumes[i][1],costumes[i][2],costume_arena,
              FT_COSTUME_IMPORT_ARENA_SIZE,FT_COSTUME_IMPORT_RECORD_CAPACITY,&costume));
        CHECK(costume.joint_count==49&&costume.joint&&costume.matanim_joint);
        if(i==0)CHECK(!strcmp(d->costume_joint,costumes[i][1])&&!strcmp(d->costume_matanim,costumes[i][2]));
        free(costume_arena);free(raw);
    }
    printf("%s clips=%lu arena=%zu items=%lu\n",d->code_key,stats.clip_count,stats.arena_used,assets.article_stats.article_count);
    free(animation_arena);free(bank);free(arena);free(bytes);
}

int main(int argc,char** argv)
{
    CHECK(argc==2);
    metadata(&ftPopoClone_Definition,expected_popo_map,0);
    metadata(&ftNanaClone_Definition,expected_nana_map,1);
    imports(argv[1],&ftPopoClone_Definition,"PlPp.dat","PlPpAJ.dat",expected_popo_costumes,expected_popo_clips);
    imports(argv[1],&ftNanaClone_Definition,"PlNn.dat","PlNnAJ.dat",expected_nana_costumes,expected_nana_clips);
    printf("PASS 2 copied bases, 8 retail costumes, 6 article slots, %u checks\n",checks);return 0;
}
