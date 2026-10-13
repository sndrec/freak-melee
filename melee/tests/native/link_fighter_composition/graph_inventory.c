// Actual owned DAT import and rig decoding; no game or model installation.
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/dat_import.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define REQUIRE(x) do {if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static void* load(const char* path,size_t* length)
{FILE* f=fopen(path,"rb");if(!f)return NULL;fseek(f,0,SEEK_END);*length=ftell(f);rewind(f);void* p=malloc(*length);if(!p||fread(p,1,*length,f)!=*length){fclose(f);free(p);return NULL;}fclose(f);return p;}
static unsigned nodes(HSD_Joint* joint)
{unsigned result=0;for(;joint;joint=joint->next){if(++result>256)return 10000;result+=nodes(joint->child);}return result;}
int main(int argc,char** argv)
{
 REQUIRE(argc==5);bool young=!strcmp(argv[1],"young_link");FighterKind kind=young?Ft_Kind_CLink:Ft_Kind_Link;
 size_t length;void* bytes=load(argv[2],&length);REQUIRE(bytes);void* arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);REQUIRE(arena);FtCharacterAssets assets;
 REQUIRE(ftCharacterAssets_Import(bytes,length,&ftCharacterStockLayouts[kind],young?"ftDataClink":"ftDataLink",314,14,5,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,&assets));
 u32 root,table,source;const char* external;REQUIRE(HSD_DatFindRoot(&assets.dat,young?"ftDataClink":"ftDataLink",&root));REQUIRE(HSD_DatResolve(&assets.dat,root+72,&table,&external)==HSD_DAT_REF_LOCAL);
 unsigned span=(HSD_DatNextTarget(&assets.dat,table)-table)/4;
 REQUIRE(assets.data.x48_items==assets.fighter_items && assets.fighter_items[6]);
 HSD_Joint* graph=assets.fighter_items[6];unsigned dcount=0;for(HSD_DObjDesc* d=graph->u.dobjdesc;d;d=d->next)++dcount;
 printf("{\"family\":\"%s\",\"table_source\":%u,\"span_to_next_target_words\":%u,\"runtime_slot_capacity\":%u,\"present_slots\":[",argv[1],(unsigned)table,span,FT_CHARACTER_ITEM_CAPACITY);
 bool comma=false;for(unsigned i=0;i<FT_CHARACTER_ITEM_CAPACITY;++i)if(assets.fighter_items[i]){printf("%s%u",comma?",":"",i);comma=true;}
 REQUIRE(HSD_DatResolve(&assets.dat,table+24,&source,&external)==HSD_DAT_REF_LOCAL);
 printf("],\"graph6_source\":%u,\"graph6_nodes\":%u,\"graph6_root_flags\":%u,\"graph6_root_draw_objects\":%u,",(unsigned)source,nodes(graph),(unsigned)graph->flags,dcount);
 free(bytes);free(arena);
 bytes=load(argv[3],&length);REQUIRE(bytes);HSD_Dat costume;REQUIRE(HSD_DatOpen(&costume,bytes,length)==HSD_DAT_OK);REQUIRE(HSD_DatFindRoot(&costume,young?"PlyClink5K_Share_joint":"PlyLink5K_Share_joint",&root));
 arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);HSD_DatImport imported;REQUIRE(HSD_DatImportInit(&imported,&costume,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,4096)==HSD_DAT_OK);HSD_Joint* costume_graph=HSD_DatImportGraph(&imported,root,DAT_JOINT);REQUIRE(costume_graph);printf("\"physical_joints\":%u,",nodes(costume_graph));free(bytes);free(arena);
 bytes=load(argv[4],&length);REQUIRE(bytes);HSD_Dat common;REQUIRE(HSD_DatOpen(&common,bytes,length)==HSD_DAT_OK&&HSD_DatFindRoot(&common,"ftLoadCommonData",&root));
 u8 to_part[256],to_joint[FT_COMMON_PART_COUNT];FighterPartsTable parts;REQUIRE(HSD_DatResolve(&common,root+16,&table,&external)==HSD_DAT_REF_LOCAL&&HSD_DatResolve(&common,table+kind*4,&source,&external)==HSD_DAT_REF_LOCAL);REQUIRE(ftData_DecodeParts(&common,source,&parts,to_part,256,to_joint)==HSD_DAT_OK);
 struct Fighter_804D6540_t omitted;struct Fighter_804D6540_x0_t records[32];REQUIRE(HSD_DatResolve(&common,root+20,&table,&external)==HSD_DAT_REF_LOCAL&&HSD_DatResolve(&common,table+kind*4,&source,&external)==HSD_DAT_REF_LOCAL);REQUIRE(ftData_DecodePartOmissions(&common,source,&omitted,records,32,parts.parts_num)==HSD_DAT_OK);
 printf("\"runtime_parts\":%u,\"semantic_part_to_joint\":[",(unsigned)parts.parts_num);for(unsigned i=0;i<FT_COMMON_PART_COUNT;++i)printf("%s%u",i?",":"",to_joint[i]);printf("],\"omissions\":[");
 for(unsigned i=0;i<omitted.x4;++i)printf("%s{\"part\":%u,\"parent\":%u,\"mode\":%u,\"source_ordinal\":%u}",i?",":"",records[i].x0,records[i].x1,records[i].x2,records[i].x3);
 puts("]}");free(bytes);return 0;
}
