// Audits tool-produced payloads only. No materializer or DAT-copy unit linked.
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/it/itCharItems.h>
#include <melee/lb/lbanim.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,graphs,tracks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"asset line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void* read(const char* dir,const char* name,size_t* size)
{
    char path[2048];snprintf(path,sizeof(path),"%s/%s",dir,name);FILE* f=fopen(path,"rb");CHECK(f && !fseek(f,0,SEEK_END));
    long n=ftell(f);CHECK(n>0 && !fseek(f,0,SEEK_SET));void* bytes=malloc(n);CHECK(bytes && fread(bytes,1,n,f)==(size_t)n);CHECK(!fclose(f));*size=n;return bytes;
}
static FtCharacterAssets* imported(const void* bytes,size_t size,const char* name)
{
    FtCharacterAssets* a=calloc(1,sizeof(*a));void* arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(a && arena);
    FtCharacterAssetLayout layout={.article_mask=7,.item_format=FtCharacterItems_ClimberString};
    CHECK(ftCharacterAssets_Import(bytes,size,&layout,name,321,14,4,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,a));
    a->arena=arena;return a;
}
static void graph(FtCharacterAssets* p,const void* x,FtCharacterAssets* n,const void* y,u32 base)
{
    CHECK((x==NULL)==(y==NULL));if(!x)return;
    HSD_DatImportRecord *a=NULL,*b=NULL;
    for(u32 i=0;i<p->import.record_count;++i)if(p->import.records[i].object==x)a=p->import.records+i;
    for(u32 i=0;i<n->import.record_count;++i)if(n->import.records[i].object==y)b=n->import.records+i;
    CHECK(a && b && a->offset+base==b->offset && a->type==b->type && a->count==b->count);++graphs;
}
static void tree(FigaTree* x,FigaTree* y)
{
    CHECK(x && y && x->type==y->type && x->flags==y->flags && x->frames==y->frames);unsigned t=0;
    for(unsigned n=0;;++n){CHECK(x->nodes[n]==y->nodes[n]);if(x->nodes[n]==-1)break;
        for(unsigned j=0;j<(u8)x->nodes[n];++j,++t){FigaTrack*a=x->tracks+t,*b=y->tracks+t;
            CHECK(a->length==b->length && a->startframe==b->startframe && a->obj_type==b->obj_type && a->frac_value==b->frac_value && a->frac_slope==b->frac_slope);
            CHECK(!memcmp(a->ad_head,b->ad_head,a->length));++tracks;}}
}
int main(int argc,char**argv)
{
    CHECK(argc==4);size_t ps,ns,os,bs[3];void* p=read(argv[1],"PlPp.dat",&ps),*n=read(argv[1],"PlNn.dat",&ns),*o=read(argv[3],"fighter.dat",&os);
    size_t produced_size;void* produced=read(argv[2],"fighter.dat",&produced_size);CHECK(produced_size==ps && !memcmp(produced,p,ps));free(p);p=produced;
    void* banks[]={read(argv[1],"PlPpAJ.dat",bs),read(argv[1],"PlNnAJ.dat",bs+1),read(argv[3],"animation.dat",bs+2)};
    produced=read(argv[2],"animation.dat",&produced_size);CHECK(produced_size==bs[0] && !memcmp(produced,banks[0],bs[0]));free(banks[0]);banks[0]=produced;
    FtCharacterAssets* assets[]={imported(p,ps,"ftDataPopo"),imported(n,ns,"ftDataNana"),imported(o,os,"ftDataNana")};
    u32 root,pr,table,source;const char*ext;
    CHECK(HSD_DatFindRoot(&assets[0]->dat,"ftDataPopo",&pr) && HSD_DatFindRoot(&assets[2]->dat,"ftDataNana",&root));
    CHECK(HSD_DatResolve(&assets[0]->dat,pr+72,&source,&ext)==HSD_DAT_REF_LOCAL && HSD_DatResolve(&assets[2]->dat,root+72,&table,&ext)==HSD_DAT_REF_LOCAL);
    u32 base=table-source;
    CHECK(assets[2]->article_stats.article_count==3 && assets[0]->article_stats.article_count==3);
    CHECK(assets[2]->import.unresolved_external_count==2); // retained animation/model placeholders, not article graph donor lookup.
    CHECK(!memcmp(&assets[1]->common_attributes,&assets[2]->common_attributes,sizeof(ftCo_DatAttrs)));
    CHECK(!memcmp(assets[1]->extended_attributes,assets[2]->extended_attributes,sizeof(assets[1]->extended_attributes)));
    for(unsigned i=0;i<3;++i){Article*a=assets[0]->fighter_items[i],*b=assets[2]->fighter_items[i];CHECK(a && b);
        CHECK(!memcmp(a->x0_common_attr,b->x0_common_attr,sizeof(ItemAttr)));
        CHECK(a->x10_modelDesc && b->x10_modelDesc);
        CHECK(a->x10_modelDesc->x4_bone_count==b->x10_modelDesc->x4_bone_count && a->x10_modelDesc->x8_bone_attach_id==b->x10_modelDesc->x8_bone_attach_id && a->x10_modelDesc->xC_bit_field==b->x10_modelDesc->xC_bit_field);
        graph(assets[0],a->x10_modelDesc->x0_joint,assets[2],b->x10_modelDesc->x0_joint,base);
        CHECK((a->xC_itemStates==NULL)==(b->xC_itemStates==NULL));
        if(a->xC_itemStates){CHECK(a->xC_itemStates->count==b->xC_itemStates->count);
            for(u32 j=0;j<a->xC_itemStates->count;++j){ItemStateDesc*x=a->xC_itemStates->x0_itemStateDesc+j,*y=b->xC_itemStates->x0_itemStateDesc+j;
                graph(assets[0],x->x0_anim_joint,assets[2],y->x0_anim_joint,base);graph(assets[0],x->x4_matanim_joint,assets[2],y->x4_matanim_joint,base);graph(assets[0],x->x8_parameters,assets[2],y->x8_parameters,base);
                CHECK((x->xC_script==NULL)==(y->xC_script==NULL));}}
        CHECK((a->x14_dynamics==NULL)==(b->x14_dynamics==NULL));if(a->x14_dynamics)CHECK(a->x14_dynamics->flexible_bone_count==b->x14_dynamics->flexible_bone_count && a->x14_dynamics->collision_bone_count==b->x14_dynamics->collision_bone_count);
        CHECK(b->native_special_attribute_source==a->native_special_attribute_source+base);
        CHECK(!memcmp(a->x4_specialAttributes,b->x4_specialAttributes,i==0?0x34:i==1?0x14:0x24));
    }
    itClimbersStringAttributes* strings=((Article*)assets[2]->fighter_items[2])->x4_specialAttributes;
    CHECK(strings->x0_count==40 && strings->x24_joint && strings->x24_joint==strings->x28_joint);
    graph(assets[0],((itClimbersStringAttributes*)((Article*)assets[0]->fighter_items[2])->x4_specialAttributes)->x24_joint,assets[2],strings->x24_joint,base);
    // Every graph decoded from the transplanted region is a real primary
    // importer graph with the same offset/type/count, not a guessed extent.
    unsigned copied=0;
    for(u32 i=0;i<assets[2]->import.record_count;++i){HSD_DatImportRecord* r=assets[2]->import.records+i;if(r->offset<base)continue;bool found=false;
        for(u32 j=0;j<assets[0]->import.record_count;++j){HSD_DatImportRecord* q=assets[0]->import.records+j;if(q->offset+base==r->offset && q->type==r->type && q->count==r->count){found=true;break;}}
        CHECK(found);++copied;}CHECK(copied>0);
    void* arenas[3];FtAnimationBankStats stats;
    for(unsigned i=0;i<3;++i){arenas[i]=malloc(FT_ANIMATION_IMPORT_ARENA_SIZE);CHECK(arenas[i]);CHECK(ftAnimationBank_Import(banks[i], bs[i],
                (const FtAnimationBankTable[]){{ assets[i]->data.xC, 321, false }}, 1,
                arenas[i], FT_ANIMATION_IMPORT_ARENA_SIZE, &stats));}
    unsigned populated=0;
    for(unsigned i=0;i<321;++i){struct Fighter_WaitAnimData* x=assets[1]->data.xC+i,*y=assets[2]->data.xC+i;
        CHECK(x->x10_animCurrFlags==y->x10_animCurrFlags);CHECK(!memcmp(assets[1]->data.x10+i,assets[2]->data.x10+i,2));
        FigaTree* expected=x->x8?x->x14:assets[0]->data.xC[i].x14;
        if(expected){tree(expected,y->x14);++populated;}else CHECK(!y->x14 && !y->x8);}
    CHECK(populated==268);
    for(unsigned i=0;i<3;++i){free(arenas[i]);free(assets[i]->arena);free(assets[i]);free(banks[i]);}free(p);free(n);free(o);
    printf("{\"checks\":%u,\"article_graphs\":%u,\"copied_import_records\":%u,\"decoded_tracks\":%u,\"populated_actions\":%u,\"articles\":3,\"unresolved_import_visits\":2}\n",checks,graphs,copied,tracks,populated);
}
