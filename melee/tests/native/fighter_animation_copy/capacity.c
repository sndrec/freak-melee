// Structural limits are independent of gameplay and clip decoding. Retail
// decoded/retargeted payload checks live in test.c and retarget.c.
#include <melee/ft/native_fighter_animation_copy.h>
#include <sysdolphin/baselib/dat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,cases;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"capacity line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void be32(unsigned char* p,uint32_t n){p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n;}
static void clip(unsigned char* bytes,const char* name,unsigned value)
{
    memset(bytes,0,64);be32(bytes,64);be32(bytes+4,4);be32(bytes+12,1);
    bytes[32]=(unsigned char)value;memcpy(bytes+44,name,strlen(name)+1);
}
static NativeFighterAnimationCopyInput fighter(unsigned count,bool populated)
{
    // Fixed nine-byte names (including NUL), with all padding initialized.
    size_t data=16+count*24+(populated?count*9:0);data=(data+3)&~(size_t)3;
    unsigned relocs=1+(populated?count:0);size_t size=32+data+relocs*4+8+8;
    unsigned char* dat=calloc(1,size);CHECK(dat);
    be32(dat,size);be32(dat+4,data);be32(dat+8,relocs);be32(dat+12,1);be32(dat+32+12,16);
    unsigned char* relocation=dat+32+data;be32(relocation,12);
    for(unsigned i=0;i<count;++i){
        unsigned char* entry=dat+32+16+i*24;be32(entry+16,0x12345678u+i);be32(entry+20,0x87654321u-i);
        if(populated){
            unsigned at=16+count*24+i*9;char name[9];snprintf(name,sizeof(name),"clip%04u",i);
            memcpy(dat+32+at,name,9);be32(entry,at);be32(entry+4,i*64);be32(entry+8,64);be32(relocation+4+i*4,16+i*24);
        }
    }
    memcpy(relocation+relocs*4+8,"fighter",8);
    unsigned char* bank=populated?malloc(count*64):NULL;CHECK(!populated || bank);
    if(populated)for(unsigned i=0;i<count;++i){char name[9];snprintf(name,sizeof(name),"clip%04u",i);clip(bank+i*64,name,i);}
    return (NativeFighterAnimationCopyInput){dat,size,"fighter",count,bank,populated?count*64:0};
}
static void destroy(NativeFighterAnimationCopyInput* x){free((void*)x->fighter_dat);free((void*)x->animation_dat);}
static void reject(const NativeFighterAnimationCopyInput* d,const NativeFighterAnimationCopyInput* s)
{
    NativeFighterAnimationCopyOutput o={0},empty={0};CHECK(!nativeFighterAnimationCopy_Materialize(d,s,&o));CHECK(!memcmp(&o,&empty,sizeof(o)));++cases;
}
void test_capacity(void)
{
    // Appending owned clip names can turn a small, unordered relocation table
    // into a large one. Preserve every original pointer in the materialized DAT.
    NativeFighterAnimationCopyInput empty = fighter(160, false);
    NativeFighterAnimationCopyInput donor = fighter(160, true);
    enum { EXTRA = 64, TABLE = 32 + 16 + 160 * 24 };
    unsigned char* grown = realloc((void*)empty.fighter_dat, empty.fighter_size + EXTRA * 4);
    CHECK(grown);
    memmove(grown + TABLE + (EXTRA + 1) * 4, grown + TABLE + 4, 16);
    empty.fighter_dat = grown;
    empty.fighter_size += EXTRA * 4;
    be32(grown, (uint32_t)empty.fighter_size);
    be32(grown + 8, EXTRA + 1);
    for (unsigned i = 0; i < EXTRA; ++i) {
        unsigned action = i % 2 ? EXTRA - 1 - i / 2 : i / 2;
        be32(grown + TABLE + (i + 1) * 4, 16 + 24 * action + 12);
    }
    NativeFighterAnimationCopyOutput expanded = {0};
    CHECK(nativeFighterAnimationCopy_Materialize(&empty, &donor, &expanded));
    HSD_Dat combined;
    CHECK(HSD_DatOpen(&combined, expanded.fighter_dat, expanded.fighter_size) == HSD_DAT_OK);
    for (unsigned i = 0; i < EXTRA; ++i) {
        u32 target;
        const char* symbol;
        CHECK(HSD_DatResolve(&combined, 16 + 24 * i + 12, &target, &symbol) ==
              HSD_DAT_REF_LOCAL && !target);
    }
    ++cases;
    nativeFighterAnimationCopy_Free(&expanded);
    destroy(&empty);
    destroy(&donor);

    NativeFighterAnimationCopyInput d=fighter(1024,false),s=fighter(1024,true);
    NativeFighterAnimationCopyOutput o={0};CHECK(nativeFighterAnimationCopy_Materialize(&d,&s,&o));
    CHECK(o.filled_actions==1024 && o.copied_blobs==1024);++cases;
    HSD_Dat dat;u32 root,table;const char* external;
    CHECK(HSD_DatOpen(&dat,o.fighter_dat,o.fighter_size)==HSD_DAT_OK && HSD_DatFindRoot(&dat,"fighter",&root));
    CHECK(HSD_DatResolve(&dat,root+12,&table,&external)==HSD_DAT_REF_LOCAL);
    for(unsigned i=0;i<1024;++i){
        u32 name,offset,size,flags;CHECK(HSD_DatResolve(&dat,table+i*24,&name,&external)==HSD_DAT_REF_LOCAL);
        char expected[9];snprintf(expected,sizeof(expected),"clip%04u",i);
        CHECK(!strcmp((const char*)dat.data+name,expected));
        CHECK(HSD_DatReadU32(&dat,table+i*24+4,&offset) && HSD_DatReadU32(&dat,table+i*24+8,&size) && size==64);
        CHECK(!memcmp((const unsigned char*)o.animation_dat+offset,(const unsigned char*)s.animation_dat+i*64,64));
        CHECK(HSD_DatReadU32(&dat,table+i*24+16,&flags) && flags==0x12345678u+i);
    }
    // A pre-populated output is rejected without losing the caller's allocation.
    NativeFighterAnimationCopyOutput original=o;CHECK(!nativeFighterAnimationCopy_Materialize(&d,&s,&o));CHECK(!memcmp(&o,&original,sizeof(o)));++cases;
    nativeFighterAnimationCopy_Free(&o);
    NativeFighterAnimationCopyInput bad=d;bad.action_count=1025;reject(&bad,&s);
    bad=s;bad.action_count=1025;reject(&d,&bad);
    unsigned char extra[64];clip(extra,"extra",0);bad=d;bad.animation_dat=extra;bad.animation_size=64;reject(&bad,&s);
    // Invalid relocation target and unresolved action name are independent
    // structural failures, and must not publish either output allocation.
    unsigned char* mutable=malloc(s.fighter_size);CHECK(mutable);memcpy(mutable,s.fighter_dat,s.fighter_size);
    be32(mutable+32+16,0xffffffff);bad=s;bad.fighter_dat=mutable;reject(&d,&bad);
    memcpy(mutable,s.fighter_dat,s.fighter_size);be32(mutable+32+12,4);reject(&d,&bad);free(mutable);
    destroy(&d);destroy(&s);
    // Two independently named roots in one physical member are copied once,
    // and an identical complete destination member is reused without copies.
    d=fighter(2,false);s=fighter(2,true);
    unsigned char aliases[80]={0};be32(aliases,80);be32(aliases+4,4);be32(aliases+12,2);
    be32(aliases+48,9);memcpy(aliases+52,"clip0000",9);memcpy(aliases+61,"clip0001",9);
    free((void*)s.animation_dat);s.animation_dat=aliases;s.animation_size=80;
    unsigned char* actions=(unsigned char*)s.fighter_dat+32+16;
    for(unsigned i=0;i<2;++i){be32(actions+i*24+4,0);be32(actions+i*24+8,80);}
    CHECK(nativeFighterAnimationCopy_Materialize(&d,&s,&o));CHECK(o.filled_actions==2 && o.copied_blobs==1 && o.animation_size==80);++cases;
    CHECK(!memcmp(o.animation_dat,aliases,80));nativeFighterAnimationCopy_Free(&o);
    d.animation_dat=aliases;d.animation_size=80;
    CHECK(nativeFighterAnimationCopy_Materialize(&d,&s,&o));CHECK(o.filled_actions==2 && !o.copied_blobs && o.animation_size==80);++cases;
    CHECK(!memcmp(o.animation_dat,aliases,80));nativeFighterAnimationCopy_Free(&o);
    free((void*)d.fighter_dat);free((void*)s.fighter_dat);
    fprintf(stderr,"capacity %u cases / %u checks\n",cases,checks);
}
