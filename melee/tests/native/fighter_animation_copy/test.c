#include <melee/ft/native_fighter_animation_copy.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/ftdat.h>
#include <melee/lb/lbanim.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, negatives;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"copy line%d: %s\n",__LINE__,#x); exit(1); } } while(0)
static uint32_t be32(const unsigned char* p)
{return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3];}
static void put32(unsigned char* p,uint32_t n)
{p[0]=(unsigned char)(n>>24);p[1]=(unsigned char)(n>>16);p[2]=(unsigned char)(n>>8);p[3]=(unsigned char)n;}
static void* read_file(const char* path,size_t* size)
{
    FILE* f=fopen(path,"rb");CHECK(f && !fseek(f,0,SEEK_END));
    long n=ftell(f);CHECK(n>0 && !fseek(f,0,SEEK_SET));
    void* p=malloc((size_t)n);CHECK(p && fread(p,1,(size_t)n,f)==(size_t)n);
    CHECK(!fclose(f));*size=(size_t)n;return p;
}
static void write_file(const char* path,const void* bytes,size_t size)
{FILE* f=fopen(path,"wb");CHECK(f && fwrite(bytes,1,size,f)==size);CHECK(!fclose(f));}
static void rejected(const NativeFighterAnimationCopyInput* d,const NativeFighterAnimationCopyInput* s)
{
    NativeFighterAnimationCopyOutput o={0};
    CHECK(!nativeFighterAnimationCopy_Materialize(d,s,&o));
    NativeFighterAnimationCopyOutput zero={0};CHECK(!memcmp(&o,&zero,sizeof(o)));++negatives;
}
static FtCharacterAssets* import(const NativeFighterAnimationCopyInput* input)
{
    FtCharacterAssets* a=calloc(1,sizeof(*a));CHECK(a);
    a->arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(a->arena);
    void* arena=a->arena;
    CHECK(ftCharacterAssets_Import(input->fighter_dat,input->fighter_size,&(FtCharacterAssetLayout){0},
        input->fighter_root,321,14,4,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,
        FT_CHARACTER_IMPORT_RECORD_CAPACITY,a));
    a->arena=arena;
    return a;
}
static void compare_tree(const FigaTree* a,const FigaTree* b)
{
    CHECK(a && b && a->type==b->type && a->flags==b->flags && a->frames==b->frames);
    unsigned track=0;
    for (unsigned node=0;;++node) {
        CHECK(a->nodes[node]==b->nodes[node]);if(a->nodes[node]==-1)break;
        for (unsigned i=0;i<(unsigned char)a->nodes[node];++i,++track) {
            const FigaTrack* x=a->tracks+track;const FigaTrack* y=b->tracks+track;
            CHECK(x->length==y->length && x->startframe==y->startframe && x->obj_type==y->obj_type &&
                x->frac_value==y->frac_value && x->frac_slope==y->frac_slope);
            CHECK(!memcmp(x->ad_head,y->ad_head,x->length));
        }
    }
}
int main(int argc,char** argv)
{
    CHECK(argc==12);
    NativeFighterAnimationCopyInput d={.fighter_root="ftDataNana",.action_count=321};
    NativeFighterAnimationCopyInput s={.fighter_root="ftDataPopo",.action_count=321};
    d.fighter_dat=read_file(argv[1],&d.fighter_size);d.animation_dat=read_file(argv[2],&d.animation_size);
    s.fighter_dat=read_file(argv[3],&s.fighter_size);s.animation_dat=read_file(argv[4],&s.animation_size);
    unsigned char* old=malloc(d.fighter_size);CHECK(old);memcpy(old,d.fighter_dat,d.fighter_size);
    NativeFighterAnimationCopyOutput o={0};CHECK(nativeFighterAnimationCopy_Materialize(&d,&s,&o));
    CHECK(o.filled_actions==260 && o.copied_blobs==199);
    CHECK(!memcmp(d.fighter_dat,old,d.fighter_size));
    CHECK(!memcmp(o.animation_dat,d.animation_dat,d.animation_size));
    HSD_Dat before,after;u32 root,table,changed_table;const char* external;
    CHECK(HSD_DatOpen(&before,d.fighter_dat,d.fighter_size)==HSD_DAT_OK);
    CHECK(HSD_DatOpen(&after,o.fighter_dat,o.fighter_size)==HSD_DAT_OK);
    CHECK(HSD_DatFindRoot(&before,d.fighter_root,&root));
    CHECK(HSD_DatResolve(&before,root+12,&table,&external)==HSD_DAT_REF_LOCAL);
    CHECK(HSD_DatResolve(&after,root+12,&changed_table,&external)==HSD_DAT_REF_LOCAL && table==changed_table);
    CHECK(before.root_count==after.root_count && before.extern_count==after.extern_count);
    CHECK(!memcmp(before.relocations,after.relocations,(size_t)before.relocation_count*4));
    size_t tail=d.fighter_size-(size_t)(before.roots-(const unsigned char*)d.fighter_dat);
    CHECK(!memcmp(before.roots,after.roots,tail));
    for (uint32_t i=0;i<before.data_size;++i) {
        bool payload=false;
        if(i>=table && i<table+321*24) {
            unsigned entry=(i-table)/24,member=(i-table)%24;
            payload=member<12 && !be32(before.data+table+entry*24+8) &&
                be32(after.data+table+entry*24+8);
        }
        if(!payload) CHECK(before.data[i]==after.data[i]);
    }
    NativeFighterAnimationCopyInput complete=d;
    complete.fighter_dat=o.fighter_dat;complete.fighter_size=o.fighter_size;
    complete.animation_dat=o.animation_dat;complete.animation_size=o.animation_size;
    NativeFighterAnimationCopyOutput twice={0};CHECK(nativeFighterAnimationCopy_Materialize(&complete,&s,&twice));
    CHECK(!twice.filled_actions && !twice.copied_blobs && twice.fighter_size==o.fighter_size &&
        twice.animation_size==o.animation_size && !memcmp(twice.fighter_dat,o.fighter_dat,o.fighter_size) &&
        !memcmp(twice.animation_dat,o.animation_dat,o.animation_size));nativeFighterAnimationCopy_Free(&twice);
    FtCharacterAssets* original=import(&d);FtCharacterAssets* primary=import(&s);FtCharacterAssets* owned=import(&complete);
    void* arenas[3];FtAnimationBankStats stats;
    FtCharacterAssets* characters[]={original,primary,owned};const NativeFighterAnimationCopyInput* inputs[]={&d,&s,&complete};
    for (unsigned i=0;i<3;++i) {
        arenas[i]=malloc(FT_ANIMATION_IMPORT_ARENA_SIZE);CHECK(arenas[i]);
        CHECK(ftAnimationBank_Import(inputs[i]->animation_dat, inputs[i]->animation_size,
                characters[i]->data.xC, 321,
                arenas[i], FT_ANIMATION_IMPORT_ARENA_SIZE, &stats));
    }
    unsigned present=0,holes=0;
    for (unsigned i=0;i<321;++i) {
        const struct Fighter_WaitAnimData* a=original->data.xC+i;
        const struct Fighter_WaitAnimData* b=owned->data.xC+i;
        const struct Fighter_WaitAnimData* expected=a->x8?a:primary->data.xC+i;
        CHECK(a->x10_animCurrFlags==b->x10_animCurrFlags);
        CHECK(!memcmp(original->data.x10+i,owned->data.x10+i,2));
        // The importer moves commands into arenas: serialized byte-by-byte
        // preservation above proves complete command graph/relocation identity.
        if (expected->x8) {compare_tree(expected->x14,b->x14);++present;}
        else {CHECK(!b->x8 && !b->x14);++holes;}
    }
    CHECK(present==268 && holes==53);
    NativeFighterAnimationCopyInput bad=d;
    bad.action_count=1025;rejected(&bad,&s);bad=d;bad.fighter_root="missing";rejected(&bad,&s);
    bad=d;--bad.fighter_size;rejected(&bad,&s);bad=s;bad.animation_size=be32(s.animation_dat)-1;rejected(&d,&bad);
    bad=s;bad.animation_size=(size_t)INT32_MAX+1;rejected(&d,&bad);
    put32(old+32+table+8,UINT32_MAX);bad=d;bad.fighter_dat=old;rejected(&bad,&s);memcpy(old,d.fighter_dat,d.fighter_size);
    if(before.relocation_count>1) {
        size_t at=32+before.data_size;memcpy(old+at+4,old+at,4);bad=d;bad.fighter_dat=old;rejected(&bad,&s);
        memcpy(old,d.fighter_dat,d.fighter_size);
    }
    size_t double_size=d.animation_size*2;unsigned char* duplicate=malloc(double_size);CHECK(duplicate);
    memcpy(duplicate,d.animation_dat,d.animation_size);memcpy(duplicate+d.animation_size,d.animation_dat,d.animation_size);
    bad=d;bad.animation_dat=duplicate;bad.animation_size=double_size;rejected(&bad,&s);free(duplicate);
    // Keep all destination-owned clips, and add a conflicting selected source
    // member. The conflict must reach name resolution, not an unrelated missing
    // destination-clip failure.
    HSD_Dat source_view;u32 source_root,source_table;
    CHECK(HSD_DatOpen(&source_view,s.fighter_dat,s.fighter_size)==HSD_DAT_OK &&
        HSD_DatFindRoot(&source_view,s.fighter_root,&source_root));
    CHECK(HSD_DatResolve(&source_view,source_root+12,&source_table,&external)==HSD_DAT_REF_LOCAL);
    unsigned selected=0;
    while(be32(before.data+table+selected*24+8) || !be32(source_view.data+source_table+selected*24+8))++selected;
    size_t source_at=be32(source_view.data+source_table+selected*24+4);
    size_t member_size=be32(source_view.data+source_table+selected*24+8);
    size_t append=(d.animation_size+31)&~(size_t)31;
    unsigned char* conflict=malloc(append+member_size);CHECK(conflict);
    memcpy(conflict,d.animation_dat,d.animation_size);memset(conflict+d.animation_size,255,append-d.animation_size);
    memcpy(conflict+append,(const unsigned char*)s.animation_dat+source_at,member_size);
    // Header reserved byte: distinct complete valid DAT with identical roots.
    conflict[append+20]^=1;
    bad=d;bad.animation_dat=conflict;bad.animation_size=append+member_size;
    rejected(&bad,&s);free(conflict);
    extern void test_capacity(void);
    test_capacity();
    extern void test_retarget(FtCharacterAssets*,FtCharacterAssets*,FtCharacterAssets*,char**);
    test_retarget(original,primary,owned,argv+5);
    write_file(argv[10],o.fighter_dat,o.fighter_size);write_file(argv[11],o.animation_dat,o.animation_size);
    for(unsigned i=0;i<3;++i){free(arenas[i]);free(characters[i]->arena);free(characters[i]);}
    nativeFighterAnimationCopy_Free(&o);CHECK(!o.fighter_dat && !o.animation_dat);
    free(old);free((void*)d.fighter_dat);free((void*)d.animation_dat);free((void*)s.fighter_dat);free((void*)s.animation_dat);
    printf("{\"checks\":%u,\"filled\":260,\"copied_blobs\":199,\"present\":%u,\"holes\":%u,\"negative\":%u}\n",checks,present,holes,negatives);
    return 0;
}
