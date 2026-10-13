#include "native_fighter_demo_copy.h"
#include <melee/ft/native_fighter_animation_copy.h>
#include <melee/ft/native_fighter_dat_copy.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, clips, rejects;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"demo line%d: %s\n",__LINE__,#x); exit(1); } } while (0)
typedef struct Case { const char *code, *root; unsigned actions, demos; const char *result, *result_root, *intro_root, *ending_root, *wait, *wait_root, *extra0, *extra1; } Case;
#include "cases.inc"
static const char* directory;
static uint32_t be32(const unsigned char* p)
{ return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static void put32(unsigned char* p, uint32_t v)
{ p[0]=(unsigned char)(v>>24); p[1]=(unsigned char)(v>>16); p[2]=(unsigned char)(v>>8); p[3]=(unsigned char)v; }
static void* read_file(const char* filename, size_t* size)
{
    char path[1024]; CHECK(snprintf(path,sizeof(path),"%s/%s",directory,filename)>0);
    FILE* f=fopen(path,"rb"); CHECK(f && !fseek(f,0,SEEK_END));
    long n=ftell(f); CHECK(n>0 && !fseek(f,0,SEEK_SET));
    void* bytes=malloc((size_t)n); CHECK(bytes && fread(bytes,1,(size_t)n,f)==(size_t)n);
    CHECK(!fclose(f)); *size=(size_t)n; return bytes;
}
static u32 table(const void* bytes,size_t size,const char* name,unsigned field)
{
    HSD_Dat d; u32 root, target; const char* ext;
    CHECK(HSD_DatOpen(&d,bytes,size)==HSD_DAT_OK && HSD_DatFindRoot(&d,name,&root));
    CHECK(HSD_DatResolve(&d,root+field,&target,&ext)==HSD_DAT_REF_LOCAL); return target;
}
static void rejected(const NativeFighterDemoCopyInput* input,const NativeFighterDemoCopySource* sources,unsigned count)
{
    NativeFighterDemoCopyOutput o={0}; CHECK(!nativeFighterDemoCopy_Materialize(input,sources,count,&o));
    CHECK(!o.fighter_dat && !o.animation_dat && !o.fighter_size && !o.animation_size && !o.copied_clips); ++rejects;
}
static void one(const Case* c)
{
    char name[64]; size_t fs,bs;
    snprintf(name,sizeof(name),"Pl%s.dat",c->code); void* fighter=read_file(name,&fs);
    snprintf(name,sizeof(name),"Pl%sAJ.dat",c->code); void* bank=read_file(name,&bs);
    NativeFighterAnimationCopyOutput actions={0}; void* article_copy=NULL;
    if (!strcmp(c->code,"Nn")) {
        size_t pf,pb,required; void* primary=read_file("PlPp.dat",&pf); void* primary_bank=read_file("PlPpAJ.dat",&pb);
        HSD_Dat d,s; u32 dr,sr; CHECK(HSD_DatOpen(&d,fighter,fs)==HSD_DAT_OK && HSD_DatOpen(&s,primary,pf)==HSD_DAT_OK);
        CHECK(HSD_DatFindRoot(&d,c->root,&dr) && HSD_DatFindRoot(&s,"ftDataPopo",&sr));
        CHECK(nativeFighterDatCopy_PointerField(fighter,fs,dr+0x48,primary,pf,sr+0x48,NULL,0,&required,NULL));
        article_copy=malloc(required); CHECK(article_copy);
        CHECK(nativeFighterDatCopy_PointerField(fighter,fs,dr+0x48,primary,pf,sr+0x48,article_copy,required,NULL,NULL));
        NativeFighterAnimationCopyInput target={article_copy,required,c->root,c->actions,bank,bs};
        NativeFighterAnimationCopyInput source={primary,pf,"ftDataPopo",321,primary_bank,pb};
        CHECK(nativeFighterAnimationCopy_Materialize(&target,&source,&actions));
        free(primary); free(primary_bank);
    }
    NativeFighterDemoCopyInput input={actions.fighter_dat?actions.fighter_dat:fighter,actions.fighter_dat?actions.fighter_size:fs,
        c->root,c->demos,actions.animation_dat?actions.animation_dat:bank,actions.animation_dat?actions.animation_size:bs};
    const char* files[6]={c->result,"IrAls.dat","GmRegEnd.dat",c->wait,"Vi0102.dat","Vi1101.dat"};
    const char* roots[6]={c->result_root,c->intro_root,c->ending_root,c->wait_root,c->extra0,c->extra1};
    const unsigned first[6]={0,10,12,13,14,15},counts[6]={10,2,1,1,1,1};
    unsigned n=c->demos==16?6:4; void* archives[6]={0}; size_t sizes[6]={0};
    NativeFighterDemoCopySource sources[6]={0};
    for (unsigned i=0;i<n;++i) { archives[i]=read_file(files[i],sizes+i); sources[i]=(NativeFighterDemoCopySource){archives[i],sizes[i],roots[i],first[i],counts[i]}; }
    NativeFighterDemoCopyOutput o={0}; CHECK(nativeFighterDemoCopy_Materialize(&input,sources,n,&o));
    u32 at=table(input.fighter_dat,input.fighter_size,c->root,20);
    CHECK(o.fighter_size==input.fighter_size && o.animation_size>input.animation_size);
    CHECK(!memcmp(o.animation_dat,input.animation_dat,input.animation_size));
    const unsigned char* old=input.fighter_dat; const unsigned char* revised=o.fighter_dat;
    for (size_t i=0;i<input.fighter_size;++i) {
        bool changed=false;
        for (unsigned j=0;j<c->demos;++j) if (be32(old+32+at+j*24+8))
            changed |= i>=32+at+j*24+4 && i<32+at+j*24+12;
        if (!changed) CHECK(old[i]==revised[i]);
    }
    struct Fighter_WaitAnimData* decoded=calloc(c->demos,sizeof(*decoded)); CHECK(decoded);
    unsigned expected=0;
    for (unsigned i=0;i<c->demos;++i) {
        unsigned record=32+at+i*24,size=be32(old+record+8); if (!size) { CHECK(!memcmp(old+record,revised+record,24)); continue; }
        unsigned s=i<10?0:i<12?1:i==12?2:i==13?3:i-10;
        HSD_Dat a; u32 base; CHECK(HSD_DatOpen(&a,archives[s],sizes[s])==HSD_DAT_OK && HSD_DatFindRoot(&a,roots[s],&base));
        const void* original=HSD_DatBytes(&a,base+be32(old+record+4),size); CHECK(original);
        unsigned offset=be32(revised+record+4); CHECK((offset&31)==0 && offset>=input.animation_size);
        CHECK(size==be32(revised+record+8) && !memcmp((const unsigned char*)o.animation_dat+offset,original,size));
        HSD_Dat d; CHECK(HSD_DatOpen(&d,o.fighter_dat,o.fighter_size)==HSD_DAT_OK);
        const char* symbol=(const char*)d.data+be32(old+record);
        decoded[i].x0=(char*)symbol; decoded[i].x4=(s32)offset; decoded[i].x8=(s32)size;
        ++expected; ++clips;
    }
    CHECK(o.copied_clips==expected);
    // Actual common decoder, not a substitute tree algorithm; exact clip bytes
    // above also pin the source animation streams, timing, SRT and track flags.
    void* arena=malloc(FT_ANIMATION_IMPORT_ARENA_SIZE); CHECK(arena); FtAnimationBankStats stats;
    const FtAnimationBankTable bank_table={decoded,c->demos,false};
    CHECK(ftAnimationBank_Import(o.animation_dat,o.animation_size,&bank_table,1,arena,FT_ANIMATION_IMPORT_ARENA_SIZE,&stats));
    CHECK(stats.clip_count==expected);
    for (unsigned i=0;i<c->demos;++i) CHECK((decoded[i].x14!=NULL)==(be32(old+32+at+i*24+8)!=0));
    NativeFighterDemoCopySource bad[6]; memcpy(bad,sources,sizeof(bad));
    bad[0].root_name="missing"; rejected(&input,bad,n);
    memcpy(bad,sources,sizeof(bad)); bad[1].first=9; rejected(&input,bad,n);
    rejected(&input,sources,n-1);
    NativeFighterDemoCopyInput invalid=input; invalid.demo_count=129; rejected(&invalid,sources,n);
    invalid=input; invalid.animation_size=64u*1024*1024; rejected(&invalid,sources,n);
    unsigned char* damaged=malloc(input.fighter_size); CHECK(damaged); memcpy(damaged,input.fighter_dat,input.fighter_size);
    invalid=input; invalid.fighter_dat=damaged;
    put32(damaged+32+at+4,0xffffffffu); rejected(&invalid,sources,n);
    memcpy(damaged,input.fighter_dat,input.fighter_size); put32(damaged+32+at+8,31); rejected(&invalid,sources,n);
    NativeFighterDemoCopyOutput occupied={0}; occupied.copied_clips=7;
    CHECK(!nativeFighterDemoCopy_Materialize(&input,sources,n,&occupied) && occupied.copied_clips==7); ++rejects;
    printf("%s: %u copied demos, action bank preserved\n",c->code,expected);
    free(damaged); free(arena); free(decoded); nativeFighterDemoCopy_Free(&o);
    for (unsigned i=0;i<n;++i) free(archives[i]);
    nativeFighterAnimationCopy_Free(&actions); free(article_copy); free(fighter); free(bank);
}
int main(int argc,char** argv)
{
    CHECK(argc==2); directory=argv[1];
    for (unsigned i=0;i<sizeof(cases)/sizeof(*cases);++i) one(cases+i);
    printf("PASS owned demos: %u fighters / %u clips / %u checks / %u rejection cases\n",(unsigned)(sizeof(cases)/sizeof(*cases)),clips,checks,rejects);
    return 0;
}
