#include <sysdolphin/baselib/texp.h>
#include <sysdolphin/baselib/texpdag.h>
#include <sysdolphin/baselib/class.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int Reference_HSD_TExpCompile(HSD_TExp*,HSD_TExpTevDesc**,HSD_TExp**);
extern void Reference_HSD_TExpFreeTevDesc(HSD_TExpTevDesc*);
extern HSD_TExp* Reference_HSD_TExpFreeList(HSD_TExp*,HSD_TExpType,s32);
static unsigned checks, pairs, allocations, frees, live, packing;
static int fail_after=-1;
#define CHECK(c)do{++checks;if(!(c)){fprintf(stderr,"line %u: %s\n",__LINE__,#c);exit(1);}}while(0)
void __assert(char* file,u32 line,char* condition)
{fprintf(stderr,"unexpected assertion %s:%lu %s\n",file,line,condition);exit(2);}
void HSD_Panic(char* file,u32 line,char* condition){__assert(file,line,condition);}
void* hsdAllocMemPiece(s32 size)
{
    if(fail_after==0)return NULL;
    if(fail_after>0)--fail_after;
    void* p=calloc(1,(size_t)size);CHECK(p!=NULL);++allocations;++live;return p;
}
void hsdFreeMemPiece(void* p,s32 size)
{(void)size;CHECK(p!=NULL&&live);--live;++frees;free(p);}
static unsigned char constants[256][4];
static HSD_TExp* graph(HSD_TExp** list,unsigned stages,unsigned seed,bool dense)
{
    HSD_TExp* previous=NULL;
    for(unsigned stage=0;stage<stages;++stage){
        HSD_TExp* tev=HSD_TExpTev(list);
        HSD_TExpColorOp(tev,GX_TEV_SUB,GX_TB_ZERO,GX_CS_SCALE_1,1);
        HSD_TExpAlphaOp(tev,GX_TEV_ADD,GX_TB_ZERO,GX_CS_SCALE_1,1);
        HSD_TExp* color=HSD_TExpCnst(constants[(seed+stage*4)&255],HSD_TE_RGB,HSD_TE_U8,list);
        HSD_TExp* a=dense?HSD_TExpCnst(constants[(seed+stage*4+1)&255],HSD_TE_RGB,HSD_TE_U8,list):HSD_TEXP_TEX;
        HSD_TExp* b=dense?HSD_TExpCnst(constants[(seed+stage*4+2)&255],HSD_TE_RGB,HSD_TE_U8,list):HSD_TEXP_RAS;
        HSD_TExpColorIn(tev,HSD_TE_RGB,a,HSD_TE_RGB,b,HSD_TE_RGB,color,
            previous?HSD_TE_RGB:HSD_TE_0,previous);
        HSD_TExpAlphaIn(tev,HSD_TE_A,HSD_TEXP_TEX,HSD_TE_A,HSD_TEXP_RAS,
            HSD_TE_1,NULL,HSD_TE_0,NULL);
        previous=tev;
    }
    return previous;
}
static void compare(const HSD_TExpTevDesc* a,const HSD_TExpTevDesc* b)
{
    while(a&&b){
        CHECK(a->desc.flags==b->desc.flags&&a->desc.stage==b->desc.stage&&a->desc.color==b->desc.color&&a->tobj==b->tobj);
        CHECK(!memcmp(&a->desc.u,&b->desc.u,sizeof(a->desc.u)));
        a=(const HSD_TExpTevDesc*)a->desc.next;b=(const HSD_TExpTevDesc*)b->desc.next;
    }
    CHECK(!a&&!b);
}
static void healthy(void)
{
    for(unsigned seed=0;seed<128;++seed){
        HSD_TExp *a=NULL,*b=NULL,*c=NULL;
        unsigned stages=1+seed%4;
        HSD_TExp *ra=graph(&a,stages,seed,false),*rb=graph(&b,stages,seed,false),*rc=graph(&c,stages,seed,false);
        HSD_TExpTevDesc *da=NULL,*db=NULL,*dc=NULL;
        int baseline=Reference_HSD_TExpCompile(ra,&da,&a);
        CHECK(HSD_TExpCompile(rb,&db,&b,HSD_TExpCompileMode_Abort)==baseline);
        CHECK(HSD_TExpCompile(rc,&dc,&c,HSD_TExpCompileMode_Checked)==baseline);
        compare(da,db);compare(da,dc);
        const HSD_TExp *ka=a,*kb=b,*kc=c;
        while(ka&&kb&&kc){
            CHECK(ka->cnst.reg==kb->cnst.reg&&ka->cnst.reg==kc->cnst.reg&&ka->cnst.idx==kb->cnst.idx&&ka->cnst.idx==kc->cnst.idx);
            CHECK(ka->cnst.val==kb->cnst.val&&ka->cnst.val==kc->cnst.val&&ka->cnst.ref==kb->cnst.ref&&ka->cnst.ref==kc->cnst.ref);
            ka=ka->comm.next;kb=kb->comm.next;kc=kc->comm.next;
        }
        CHECK(!ka&&!kb&&!kc);
        Reference_HSD_TExpFreeTevDesc(da);Reference_HSD_TExpFreeList(a,HSD_TE_ALL,1);
        HSD_TExpFreeTevDesc(db);HSD_TExpFreeList(b,HSD_TE_ALL,1);
        HSD_TExpFreeTevDesc(dc);HSD_TExpFreeList(c,HSD_TE_ALL,1);CHECK(!live);++pairs;
    }
}
static void failures(void)
{
    HSD_TExpTevDesc marker={0};HSD_TExpTevDesc* output=&marker;
    HSD_TExp* list=NULL;HSD_TExp* root=graph(&list,1,0,false);
    root->tev.c_in[3]=(HSD_TEArg){.type=HSD_TE_TEV,.sel=HSD_TE_RGB,.exp=root};
    CHECK(HSD_TExpCompile(root,&output,&list,HSD_TExpCompileMode_Checked)==HSD_TExpCompile_InvalidDag);
    CHECK(output==&marker);root->tev.c_in[3]=(HSD_TEArg){.type=HSD_TE_ZERO,.sel=HSD_TE_0};
    HSD_TExpFreeList(list,HSD_TE_ALL,1);CHECK(!live);
    list=NULL;root=graph(&list,1,0,false);HSD_TExp* tail=list;while(tail->comm.next)tail=tail->comm.next;
    tail->comm.next=list;
    CHECK(HSD_TExpCompile(root,&output,&list,HSD_TExpCompileMode_Checked)==HSD_TExpCompile_InvalidDag);
    CHECK(output==&marker);tail->comm.next=NULL;HSD_TExpFreeList(list,HSD_TE_ALL,1);CHECK(!live);
    list=NULL;for(unsigned i=0;i<=HSD_TEXP_CHECKED_GRAPH_LIMIT;++i)root=HSD_TExpTev(&list);
    CHECK(HSD_TExpCompile(root,&output,&list,HSD_TExpCompileMode_Checked)==HSD_TExpCompile_StageLimit);
    CHECK(output==&marker);HSD_TExpFreeList(list,HSD_TE_ALL,1);CHECK(!live);
    list=NULL;root=graph(&list,33,0,false);HSD_TExpDag dag[32];
    CHECK(HSD_TExpMakeDag(root,dag)==HSD_TExpCompile_StageLimit);
    HSD_TExpFreeList(list,HSD_TE_ALL,1);CHECK(!live);
    for(unsigned after=0;after<3;++after){
        list=NULL;root=graph(&list,4,0,false);fail_after=(int)after;
        CHECK(HSD_TExpCompile(root,&output,&list,HSD_TExpCompileMode_Checked)==HSD_TExpCompile_Allocation);
        fail_after=-1;CHECK(output==&marker);HSD_TExpFreeList(list,HSD_TE_ALL,1);CHECK(!live);
    }
    for(unsigned stages=2;stages<=12;++stages){
        list=NULL;root=graph(&list,stages,0,true);
        int status=HSD_TExpCompile(root,&output,&list,HSD_TExpCompileMode_Checked);
        if(status<0){CHECK(status==HSD_TExpCompile_RegisterPacking);CHECK(output==&marker);++packing;}
        else {HSD_TExpFreeTevDesc(output);output=&marker;}
        HSD_TExpFreeList(list,HSD_TE_ALL,1);CHECK(!live);
    }
    CHECK(packing>0);
}
int main(void)
{
    healthy();failures();CHECK(allocations==frees);
    printf("{\"checks\":%u,\"native_shader_pairs\":%u,\"packing_failures\":%u,\"allocations\":%u,\"frees\":%u}\n",checks,pairs,packing,allocations,frees);
    return 0;
}
