#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/ftdemo.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/pl/player.h>
#include <melee/pl/types.h>
#include <melee/lb/lbanim.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, creations;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"owned results line%d: %s\n",__LINE__,#x); exit(1); } } while (0)
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
ftData_UnkCountStruct ftData_UnkIntPairs[Ft_Kind_RuntimeCapacity];
ftData_UnkCountStruct ftData_Table_Unk0[Ft_Kind_RuntimeCapacity];
struct UnkCostumeList CostumeListsForeachCharacter[Ft_Kind_RuntimeCapacity];
StaticPlayer player_slots[Gm_Player_NumMax];
#include "mapping.inc"
static ft_8045993C_t ft_8045993C[6];
static ftMapping ftMapping_list[ChKind_RuntimeCapacity];
static NativeFighterDefinition definitions[2];
static bool registered = true;
static plAllocInfo2 calls[2];
static HSD_GObj objects[2];
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return registered && (kind == 40 || kind == 41) ? definitions + (kind-40) : NULL; }
static void Player_CheckSlot(int slot) { CHECK((unsigned)slot < Gm_Player_NumMax); }
Fighter_GObj* ftDemo_CreateFighter(plAllocInfo2* info)
{ CHECK(creations < 2); calls[creations]=*info; return &objects[creations++]; }
#include "bodies.inc"

static void put32(unsigned char* p, unsigned v)
{ p[0]=(unsigned char)(v>>24);p[1]=(unsigned char)(v>>16);p[2]=(unsigned char)(v>>8);p[3]=(unsigned char)v; }
static unsigned char* read_bytes(const char* path, size_t* size)
{ FILE* f=fopen(path,"rb");CHECK(f && !fseek(f,0,SEEK_END));*size=(size_t)ftell(f);CHECK(*size && !fseek(f,0,SEEK_SET));unsigned char* b=malloc(*size);CHECK(b && fread(b,1,*size,f)==*size && !fclose(f));return b; }
static void import_matrix(const char* path)
{
    size_t size;unsigned char* bytes=read_bytes(path,&size);void* arena=malloc(FT_ANIMATION_IMPORT_ARENA_SIZE);CHECK(arena);
    struct Fighter_WaitAnimData action[1]={{.x0="Action",.x4=-1,.x8=32}};
    struct Fighter_WaitAnimData demos[3]={{.x0="Win1",.x4=-1,.x8=32},{.x0="Win1Wait",.x4=-1,.x8=32},{.x0=NULL,.x8=0}};
    FtAnimationBankTable tables[2]={{action,1,false},{demos,3,true}};FtAnimationBankStats stats;
    CHECK(ftAnimationBank_Import(bytes,size,tables,2,arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&stats));
    CHECK(stats.clip_count==3 && demos[2].x14==NULL);
    CHECK(action[0].x14 && demos[0].x14 && demos[1].x14 && action[0].x14!=demos[0].x14 && demos[0].x14!=demos[1].x14);
    CHECK(((FigaTree*)action[0].x14)->frames==10 && ((FigaTree*)demos[0].x14)->frames==20 && ((FigaTree*)demos[1].x14)->frames==30);
    FigaTree saved=*(FigaTree*)action[0].x14;CHECK((unsigned char*)demos[0].x14>(unsigned char*)action[0].x14);
    ((FigaTree*)demos[0].x14)->frames=77;CHECK(!memcmp(action[0].x14,&saved,sizeof(saved)));
    // Existing edited projects have demo descriptors without their symbols.
    demos[0].x0="AbsentWin";demos[1].x0="AbsentWait";
    CHECK(ftAnimationBank_Import(bytes,size,tables,2,arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&stats));
    CHECK(stats.clip_count==1 && action[0].x14 && !demos[0].x14 && !demos[1].x14);
    tables[1].allow_missing=false;CHECK(!ftAnimationBank_Import(bytes,size,tables,2,arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&stats));
    CHECK(stats.status==HSD_DAT_SYMBOL && stats.failed_table==1 && stats.failed_index==0);
    tables[1].allow_missing=true;demos[0].x0="Win1";demos[1].x0="Win1Wait";
    // A present malformed optional clip MUST fail, never disappear silently.
    unsigned offset=(unsigned)demos[0].x4;put32(bytes+offset+32+8,0x7fc00000);
    CHECK(!ftAnimationBank_Import(bytes,size,tables,2,arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&stats));
    CHECK(stats.status==HSD_DAT_DESCRIPTOR && stats.failed_table==1 && stats.failed_index==0);
    tables[1].entries=NULL;CHECK(!ftAnimationBank_Import(bytes,size,tables,2,arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&stats));
    CHECK(stats.status==HSD_DAT_DESCRIPTOR && stats.failed_table==1);
    tables[1].count=0;CHECK(ftAnimationBank_Import(bytes,size,tables,2,arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&stats));
    CHECK(stats.clip_count==1);
    action[0].x0="MissingAction";CHECK(!ftAnimationBank_Import(bytes,size,tables,2,arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&stats));
    CHECK(stats.failed_table==0 && stats.failed_index==0);
    CHECK(!ftAnimationBank_Import(bytes,size,tables,2,arena,1,&stats) && stats.status==HSD_DAT_CAPACITY);
    free(arena);free(bytes);
}
static void result_matrix(void)
{
    ftData data={0};struct Fighter_WaitAnimData demos[14]={0};FigaTree tree={0};data.x14=demos;gFtDataList[40]=&data;ftData_UnkIntPairs[40].count=14;
    const unsigned entry[5]={0,2,5,0,9};
    for(int variant=-1;variant<=5;++variant)for(unsigned mask=0;mask<1024;++mask){
        for(unsigned i=0;i<14;++i)demos[i].x14=(i<10 && (mask&(1u<<i)))?&tree:NULL;
        bool want=false;if(variant>=0 && variant<5 && variant!=3){unsigned e=entry[variant],l=variant==4?e:e+1;want=(mask&(1u<<e))&&(mask&(1u<<l));}
        CHECK(ftDemo_HasResultClips(40,variant)==want);
    }
    CHECK(!ftDemo_HasResultClips(-1,0) && !ftDemo_HasResultClips(Ft_Kind_RuntimeCapacity,0));
    data.x14=NULL;CHECK(!ftDemo_HasResultClips(40,0));data.x14=demos;
    for(unsigned i=0;i<14;++i)demos[i].x14=&tree;
    for(unsigned count=0;count<14;++count){ftData_UnkIntPairs[40].count=count;CHECK(ftDemo_HasResultClips(40,2)==(count>6));}
    gFtDataList[40]=NULL;CHECK(!ftDemo_HasResultClips(40,4));
    for(unsigned alternate=0;alternate<2;++alternate)for(int selected=Ft_Kind_None;selected<=42;++selected)for(unsigned variant=0;variant<5;++variant){
        ftMapping map={(s8)40,(s8)41,alternate};ftMapping_list[40]=map;player_slots[0]=(StaticPlayer){0};player_slots[0].player_character=40;
        creations=0;registered=true;Player_CreateDemoFighters(0,variant,selected);
        bool accepted=!alternate || selected==Ft_Kind_None || selected==40 || selected==41;
        CHECK(creations==(accepted?(alternate?1:2):0));CHECK(!memcmp(&map,&ftMapping_list[40],sizeof(map)));
        if(accepted){CHECK(calls[0].internal_id==(alternate && selected!=Ft_Kind_None?selected:40));CHECK(calls[0].unk8==variant && !calls[0].has_transformation);CHECK(player_slots[0].player_entity[0]==objects);if(!alternate){CHECK(calls[1].internal_id==41 && calls[1].has_transformation && calls[1].unk8==variant);CHECK(player_slots[0].player_entity[1]==objects+1);}}
    }
    registered=false;creations=0;ftMapping_list[40]=(ftMapping){40,41,true};Player_CreateDemoFighters(0,0,41);CHECK(creations==0);
    // Single custom and ordinary stock callers pass None and keep normal order.
    for(unsigned character=0;character<=40;character+=40){ftMapping_list[character]=(ftMapping){6,-1,false};player_slots[0].player_character=character;creations=0;Player_CreateDemoFighters(0,4,Ft_Kind_None);CHECK(creations==1 && calls[0].internal_id==6 && calls[0].unk8==4);}
}
static void reset_matrix(void)
{
    ftData data={0};struct Fighter_WaitAnimData demos[14]={0};FigaTree tree={0};data.x14=demos;
    for(unsigned i=0;i<14;++i)demos[i].x14=&tree;
    UnkCostumeStruct costumes[Ft_Kind_RuntimeCapacity]={0};
    const UnkCostumeStruct sample={.joint=(void*)(uintptr_t)0x1000,.x4=(void*)(uintptr_t)0x2000,.pad_x8=7,.x14_archive=(void*)(uintptr_t)0x3000,.native_assets=(void*)(uintptr_t)0x4000};
    for(unsigned iteration=0;iteration<2;++iteration){
        registered=true;
        for(unsigned i=0;i<Ft_Kind_RuntimeCapacity;++i){
            gFtDataList[i]=&data;costumes[i]=sample;
            CostumeListsForeachCharacter[i]=(struct UnkCostumeList){costumes+i,1};
            ftData_Table_Unk0[i]=(ftData_UnkCountStruct){.data=(void*)(uintptr_t)0x5000,.count=10};
            ftData_UnkIntPairs[i]=(ftData_UnkCountStruct){.data=(void*)(uintptr_t)0x6000,.count=14};
        }
        for(unsigned i=0;i<6;++i){ft_8045993C[i].pad_x0=1;ft_8045993C[i].x6_b0=1;ft_8045993C[i].x6_b1_b2=3;}
        CHECK(ftDemo_HasResultClips(40,0));
        if(iteration==0)ft_800852B0_before();else ft_800852B0();
        CHECK(ftDemo_HasResultClips(40,0)==(iteration!=0));
        for(unsigned i=0;i<Ft_Kind_RuntimeCapacity;++i){
            bool retain=iteration!=0 && (i==40 || i==41);
            CHECK((gFtDataList[i]!=NULL)==retain);
            CHECK((costumes[i].joint!=NULL)==retain && (costumes[i].x4!=NULL)==retain && (costumes[i].native_assets!=NULL)==retain);
            CHECK((costumes[i].x14_archive!=NULL)==retain && costumes[i].pad_x8==(retain?7:0));
            CHECK((ftData_Table_Unk0[i].data!=NULL)==retain && (ftData_UnkIntPairs[i].data!=NULL)==retain);
            CHECK(ftData_Table_Unk0[i].count==10 && ftData_UnkIntPairs[i].count==14);
        }
        for(unsigned i=0;i<6;++i)CHECK(ft_8045993C[i].pad_x0==0 && !ft_8045993C[i].x6_b0 && !ft_8045993C[i].x6_b1_b2);
    }
    // Retired package slots are no longer exempt from scene reset.
    registered=false;ft_800852B0();CHECK(!gFtDataList[40] && !gFtDataList[41] && !costumes[40].joint && !costumes[41].native_assets);
}
int main(int argc,char** argv)
{CHECK(argc==2);import_matrix(argv[1]);result_matrix();reset_matrix();printf("PASS %u checks: actual multi-table DAT decoder and extracted result/creation/reset bodies\n",checks);return 0;}
