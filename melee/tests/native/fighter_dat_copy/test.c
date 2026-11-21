#include "native_fighter_dat_copy.h"
#include <melee/ft/ftcharacter_assets.h>
#include <melee/it/itCharItems.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, rejects;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const void* p){const unsigned char*b=p;return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static void put(void* p,uint32_t x){unsigned char*b=p;b[0]=x>>24;b[1]=x>>16;b[2]=x>>8;b[3]=x;}
static uint32_t resolve(const HSD_Dat* d,uint32_t slot,HSD_DatReferenceKind kind,const char* symbol){u32 target;const char*name;CHECK(HSD_DatResolve(d,slot,&target,&name)==kind);CHECK(symbol?!strcmp(symbol,name):!name);return target;}
static void* file(const char*dir,const char*name,size_t*n){char path[1024];snprintf(path,sizeof(path),"%s/%s",dir,name);FILE*f=fopen(path,"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long size=ftell(f);CHECK(size>0);rewind(f);void*p=malloc(size);CHECK(p);CHECK(fread(p,1,size,f)==(size_t)size);fclose(f);*n=size;return p;}

// Small complete serialized archives; no alternate importer or pointer resolver.
static size_t synthetic(unsigned char*b,int source){
    memset(b,0,512);const unsigned data=source?68:64;
    const uint32_t rel[]={0,4,8};const char*names=source?"source\0shared\0new\0":"destination\0shared\0old\0";
    size_t strings=source?18:23,at=32+data;
    put(b+4,data);put(b+8,3);put(b+12,1);put(b+16,2);
    for(unsigned i=20;i<32;++i)b[i]=(unsigned char)(0x70+i);
    put(b+32,source?0:16);put(b+36,source?12:20);put(b+40,source?12:24);
    put(b+44,0x12345678);put(b+48,20);put(b+52,UINT32_MAX);put(b+56,UINT32_MAX);
    for(unsigned i=28;i<data;++i)b[32+i]=(unsigned char)(i*7);
    for(unsigned i=0;i<3;++i)put(b+at+i*4,rel[i]);at+=12;
    put(b+at,28);put(b+at+4,0);at+=8;
    put(b+at,16);put(b+at+4,source?7:12);put(b+at+8,24);put(b+at+12,source?14:19);at+=16;
    memcpy(b+at,names,strings);at+=strings;put(b,(uint32_t)at);return at;
}

static void rejection(const unsigned char*d,size_t ds,uint32_t dp,const unsigned char*s,size_t ss,uint32_t sp,NativeFighterDatCopyResult expected){
    unsigned char output[2048],before[2048];memset(output,0xA5,sizeof(output));memcpy(before,output,sizeof(output));
    NativeFighterDatCopyStats st;size_t required=99;
    CHECK(!nativeFighterDatCopy_PointerField(d,ds,dp,s,ss,sp,output,sizeof(output),&required,&st));CHECK(st.result==expected);CHECK(!memcmp(output,before,sizeof(output)));++rejects;
}

static void synthetic_tests(void){
    unsigned char d[512],s[512],bad[512],output[2048],old_d[512],old_s[512];size_t ds=synthetic(d,0),ss=synthetic(s,1),required;
    memcpy(old_d,d,sizeof(d));memcpy(old_s,s,sizeof(s));NativeFighterDatCopyStats st;
    CHECK(nativeFighterDatCopy_PointerField(d,ds,0,s,ss,0,NULL,0,&required,&st));CHECK(st.result==NativeFighterDatCopy_Ok&&st.source_base==64&&st.copied_data_bytes==68&&st.merged_external_count==1&&st.external_count==3);
    CHECK(nativeFighterDatCopy_PointerField(d,ds,0,s,ss,0,output,sizeof(output),NULL,&st));CHECK(st.output_bytes==required);HSD_Dat result;CHECK(HSD_DatOpen(&result,output,required)==HSD_DAT_OK);
    CHECK(resolve(&result,0,HSD_DAT_REF_LOCAL,NULL)==64); // source relocated zero is LOCAL, not NULL.
    CHECK(resolve(&result,64,HSD_DAT_REF_LOCAL,NULL)==64);
    CHECK(resolve(&result,68,HSD_DAT_REF_LOCAL,NULL)==76&&resolve(&result,72,HSD_DAT_REF_LOCAL,NULL)==76); // aliases/interior target.
    CHECK(resolve(&result,16,HSD_DAT_REF_EXTERNAL,"shared")==20);CHECK(resolve(&result,80,HSD_DAT_REF_EXTERNAL,"shared")==84);
    CHECK(word(result.data+84)==16&&word(result.data+20)==UINT32_MAX);
    resolve(&result,24,HSD_DAT_REF_EXTERNAL,"old");resolve(&result,88,HSD_DAT_REF_EXTERNAL,"new");
    HSD_Dat original;CHECK(HSD_DatOpen(&original,d,ds)==HSD_DAT_OK);CHECK(!memcmp(result.roots,original.roots,8));CHECK(!memcmp(result.strings,original.strings,original.strings_size));
    u32 root;CHECK(HSD_DatFindRoot(&result,"destination",&root)&&root==28);CHECK(!HSD_DatFindRoot(&result,"source",&root));CHECK(!memcmp(output+20,d+20,12));CHECK(!memcmp(result.data+4,d+36,60));
    CHECK(!memcmp(d,old_d,sizeof(d))&&!memcmp(s,old_s,sizeof(s)));
    memset(output,0xCC,sizeof(output));CHECK(!nativeFighterDatCopy_PointerField(d,ds,0,s,ss,0,output,required-1,NULL,&st));CHECK(st.result==NativeFighterDatCopy_Capacity);for(unsigned i=0;i<sizeof(output);++i)CHECK(output[i]==0xCC);
    CHECK(!nativeFighterDatCopy_PointerField(d,ds,0,s,ss,0,d,sizeof(d),NULL,&st));CHECK(st.result==NativeFighterDatCopy_Overlap&& !memcmp(d,old_d,sizeof(d)));
    CHECK(!nativeFighterDatCopy_PointerField(d,ds,0,s,ss,0,s,sizeof(s),NULL,&st));CHECK(st.result==NativeFighterDatCopy_Overlap&& !memcmp(s,old_s,sizeof(s)));
    rejection(d,ds,1,s,ss,0,NativeFighterDatCopy_Slot);rejection(d,ds,12,s,ss,0,NativeFighterDatCopy_Slot);rejection(d,ds,16,s,ss,0,NativeFighterDatCopy_Slot);rejection(d,ds,0,s,ss,16,NativeFighterDatCopy_Slot);
    memcpy(bad,s,ss);put(bad+32,0);put(bad+32+68,4); // duplicate reloc slot0 replaced with4; local zero now unrelocated.
    rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_References);
    memcpy(bad,s,ss);put(bad+32+68+12+8+8,20); // external chains overlap slot20.
    rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_References);
    memcpy(bad,s,ss);put(bad+32+68+12+8+12,7); // duplicate external name.
    rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_References);
    memcpy(bad,s,ss);put(bad+32+68,16); // reloc overlaps external.
    rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_References);
    memcpy(bad,s,ss);put(bad+32+20,16);rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_Input); // cycle.
    memcpy(bad,s,ss);put(bad+4,500);rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_Input);
    memcpy(bad,s,ss);put(bad+32+68+12+8+4,1000);rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_Input);
    memcpy(bad,s,ss);put(bad+32,1000);rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_Input);
    memcpy(bad,s,ss);put(bad+32,0);put(bad+32+68,12);put(bad+44,0); // source0 unrelocated NULL; distinct reloc12 is valid.
    rejection(d,ds,0,bad,ss,0,NativeFighterDatCopy_Slot);
    rejection(d,ds-1,0,s,ss,0,NativeFighterDatCopy_Input);rejection(d,ds,0,NULL,0,0,NativeFighterDatCopy_Input);
    // Destination local-zero is an existing relocation and can be replaced.
    memcpy(bad,d,ds);put(bad+32,0);CHECK(nativeFighterDatCopy_PointerField(bad,ds,0,s,ss,0,output,sizeof(output),&required,&st));CHECK(HSD_DatOpen(&result,output,required)==HSD_DAT_OK);CHECK(resolve(&result,0,HSD_DAT_REF_LOCAL,NULL)==64);
    CHECK(!nativeFighterDatCopy_PointerField(d,ds,0,s,ss,0,(void*)(UINTPTR_MAX-8),16,NULL,&st)&&st.result==NativeFighterDatCopy_Overlap);
    // Empty same-name source chain preserves existing destination chain.
    memcpy(bad,s,ss);put(bad+32+68+12+8,UINT32_MAX);CHECK(nativeFighterDatCopy_PointerField(d,ds,0,bad,ss,0,output,sizeof(output),&required,&st));CHECK(HSD_DatOpen(&result,output,required)==HSD_DAT_OK);resolve(&result,16,HSD_DAT_REF_EXTERNAL,"shared");
    // Both inputs may be the same immutable archive; appended slots stay distinct.
    CHECK(nativeFighterDatCopy_PointerField(s,ss,0,s,ss,0,output,sizeof(output),&required,&st));CHECK(st.source_base==96&&st.merged_external_count==2);CHECK(HSD_DatOpen(&result,output,required)==HSD_DAT_OK);resolve(&result,112,HSD_DAT_REF_EXTERNAL,"shared");resolve(&result,16,HSD_DAT_REF_EXTERNAL,"shared");
    // Unsorted relocation input is rebuilt sorted without changing its data.
    memcpy(bad,s,ss);put(bad+32+68,8);put(bad+32+68+8,0);CHECK(nativeFighterDatCopy_PointerField(d,ds,0,bad,ss,0,output,sizeof(output),&required,&st));CHECK(HSD_DatOpen(&result,output,required)==HSD_DAT_OK);
    for (u32 i=1;i<result.relocation_count;++i) { u32 prior,slot;CHECK(HSD_DatRelocation(&result,i-1,&prior)&&HSD_DatRelocation(&result,i,&slot)&&prior<slot); }
    // Alignment padding is zero and non-word-sized source data is preserved.
    memcpy(bad,d,ds);put(bad+4,63);memmove(bad+95,bad+96,ds-96);put(bad,(uint32_t)ds-1);
    CHECK(nativeFighterDatCopy_PointerField(bad,ds-1,0,s,ss,0,output,sizeof(output),&required,&st));CHECK(st.source_base==64&&output[32+63]==0);
    // Checked package cap before any huge input dereference.
    rejection(d,ds,0,s,64u*1024u*1024u+1,0,NativeFighterDatCopy_Input);
    size_t huge_data=64u*1024u*1024u-48,huge_size=32+huge_data+4;
    unsigned char*huge=calloc(huge_size,1);CHECK(huge);put(huge,(uint32_t)huge_size);put(huge+4,(uint32_t)huge_data);put(huge+8,1);put(huge+32+huge_data,0);
    rejection(d,ds,0,huge,huge_size,0,NativeFighterDatCopy_Capacity);free(huge);
}

static FtCharacterAssets* imported(const void*bytes,size_t size,const char*root){
    FtCharacterAssets*a=calloc(1,sizeof(*a));CHECK(a);void*arena=malloc(FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(arena);
    FtCharacterAssetLayout layout={.article_mask=7,.item_format=FtCharacterItems_ClimberString};
    CHECK(ftCharacterAssets_Import(bytes,size,&layout,root,321,14,4,arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,a));return a;
}
static void graph_identity(const FtCharacterAssets*a,const void* x,const FtCharacterAssets*b,const void*y,uint32_t base){
    CHECK((x==NULL)==(y==NULL));if(!x)return;
    const HSD_DatImportRecord*one=NULL,*two=NULL;
    for(u32 i=0;i<a->import.record_count;++i)if(a->import.records[i].object==x)one=&a->import.records[i];
    for(u32 i=0;i<b->import.record_count;++i)if(b->import.records[i].object==y)two=&b->import.records[i];
    CHECK(one&&two);CHECK(one->offset+base==two->offset&&one->type==two->type&&one->count==two->count);
}

static void retail_tests(const char*directory){
    size_t ds,ss;unsigned char*d=file(directory,"PlNn.dat",&ds),*s=file(directory,"PlPp.dat",&ss);
    HSD_Dat od,os;CHECK(HSD_DatOpen(&od,d,ds)==HSD_DAT_OK&&HSD_DatOpen(&os,s,ss)==HSD_DAT_OK);u32 dr,sr;CHECK(HSD_DatFindRoot(&od,"ftDataNana",&dr)&&HSD_DatFindRoot(&os,"ftDataPopo",&sr));
    size_t required;NativeFighterDatCopyStats st;CHECK(nativeFighterDatCopy_PointerField(d,ds,dr+0x48,s,ss,sr+0x48,NULL,0,&required,&st));unsigned char*out=malloc(required);CHECK(out);CHECK(nativeFighterDatCopy_PointerField(d,ds,dr+0x48,s,ss,sr+0x48,out,required,NULL,&st));
    CHECK(st.copied_data_bytes==101664&&st.merged_external_count==1&&st.external_count==4);
    HSD_Dat result;CHECK(HSD_DatOpen(&result,out,required)==HSD_DAT_OK);CHECK(!memcmp(result.roots,od.roots,od.root_count*8));CHECK(!memcmp(result.strings,od.strings,od.strings_size));CHECK(result.root_count==od.root_count);
    CHECK(!HSD_DatFindRoot(&result,"ftDataPopo",&sr)); // No source root publication.
    for(u32 i=0;i<od.data_size;++i)if(i<dr+0x48||i>=dr+0x4C)CHECK(result.data[i]==od.data[i]);
    // Entire source byte closure, not guessed graph extents. Every copied local
    // pointer (zero included) and external slot has independently decoded meaning.
    unsigned char*pointer=calloc(os.data_size,1);CHECK(pointer);
    for(u32 i=0;i<os.relocation_count;++i){u32 slot,target;const char*name;CHECK(HSD_DatRelocation(&os,i,&slot));CHECK(HSD_DatResolve(&os,slot,&target,&name)==HSD_DAT_REF_LOCAL);CHECK(resolve(&result,st.source_base+slot,HSD_DAT_REF_LOCAL,NULL)==st.source_base+target);memset(pointer+slot,1,4);}
    for(u32 i=0;i<os.extern_count;++i){u32 slot;const char*name;CHECK(HSD_DatSymbol(&os,true,i,&slot,&name));while(slot!=UINT32_MAX){resolve(&result,st.source_base+slot,HSD_DAT_REF_EXTERNAL,name);memset(pointer+slot,1,4);slot=word(os.data+slot);}}
    for(u32 i=0;i<os.data_size;++i)if(!pointer[i])CHECK(result.data[st.source_base+i]==os.data[i]);free(pointer);
    FtCharacterAssets*primary=imported(s,ss,"ftDataPopo"),*original=imported(d,ds,"ftDataNana"),*owned=imported(out,required,"ftDataNana");
    printf("Unresolved import visits: primary=%lu original=%lu owned=%lu\n",primary->import.unresolved_external_count,original->import.unresolved_external_count,owned->import.unresolved_external_count);CHECK(primary->import.unresolved_external_count==1&&original->import.unresolved_external_count==6&&owned->import.unresolved_external_count==2);
    // Complete frozen importer visits the unchanged Nana part-animation table's
    // following external slot 0x6CD8. This is not the new article table: both
    // original and owned archives preserve its exact external name and bytes.
    resolve(&od,0x6CD8,HSD_DAT_REF_EXTERNAL,"ItmIceclimberIceShot_TopN_joint");
    resolve(&result,0x6CD8,HSD_DAT_REF_EXTERNAL,"ItmIceclimberIceShot_TopN_joint");
    for(unsigned group=0;group<5;++group){CHECK((original->part_animation_groups[group]==NULL)==(owned->part_animation_groups[group]==NULL));if(original->part_animation_groups[group]){struct ftData_x1C*x=original->part_animation_groups[group],*y=owned->part_animation_groups[group];CHECK(x->x0==y->x0&&x->x2==y->x2);}}
    CHECK(!memcmp(&original->common_attributes,&owned->common_attributes,sizeof(original->common_attributes)));CHECK(!memcmp(original->extended_attributes,owned->extended_attributes,sizeof(original->extended_attributes)));
    CHECK(original->action_stats.entry_count==owned->action_stats.entry_count&&owned->article_stats.article_count==3);CHECK(original->model_data.x0.costume_count==owned->model_data.x0.costume_count);
    for(unsigned i=0;i<3;++i){Article*a=primary->fighter_items[i],*b=owned->fighter_items[i];CHECK(a&&b);CHECK(!memcmp(a->x0_common_attr,b->x0_common_attr,sizeof(ItemAttr)));
        CHECK(a->x10_modelDesc&&b->x10_modelDesc);CHECK(a->x10_modelDesc->x4_bone_count==b->x10_modelDesc->x4_bone_count&&a->x10_modelDesc->x8_bone_attach_id==b->x10_modelDesc->x8_bone_attach_id&&a->x10_modelDesc->xC_bit_field==b->x10_modelDesc->xC_bit_field);
        graph_identity(primary,a->x10_modelDesc->x0_joint,owned,b->x10_modelDesc->x0_joint,st.source_base);
        CHECK((a->xC_itemStates==NULL)==(b->xC_itemStates==NULL));if(a->xC_itemStates){CHECK(a->xC_itemStates->count==b->xC_itemStates->count);for(u32 j=0;j<a->xC_itemStates->count;++j){ItemStateDesc*x=&a->xC_itemStates->x0_itemStateDesc[j],*y=&b->xC_itemStates->x0_itemStateDesc[j];graph_identity(primary,x->x0_anim_joint,owned,y->x0_anim_joint,st.source_base);graph_identity(primary,x->x4_matanim_joint,owned,y->x4_matanim_joint,st.source_base);graph_identity(primary,x->x8_parameters,owned,y->x8_parameters,st.source_base);CHECK((x->xC_script==NULL)==(y->xC_script==NULL));}}
        CHECK((a->x14_dynamics==NULL)==(b->x14_dynamics==NULL));if(a->x14_dynamics){CHECK(a->x14_dynamics->flexible_bone_count==b->x14_dynamics->flexible_bone_count&&a->x14_dynamics->collision_bone_count==b->x14_dynamics->collision_bone_count);}
        CHECK(b->native_special_attribute_source==st.source_base+a->native_special_attribute_source);CHECK(!memcmp(a->x4_specialAttributes,b->x4_specialAttributes,i==0?0x34:i==1?0x14:0x24));
    }
    itClimbersStringAttributes*a=((Article*)primary->fighter_items[2])->x4_specialAttributes,*b=((Article*)owned->fighter_items[2])->x4_specialAttributes,*old=((Article*)original->fighter_items[2])->x4_specialAttributes;
    CHECK(!old->x24_joint&&!old->x28_joint);CHECK(b->x0_count==40&&b->x24_joint&&b->x28_joint&&b->x24_joint==b->x28_joint);graph_identity(primary,a->x24_joint,owned,b->x24_joint,st.source_base);
    CHECK(!((Article*)owned->fighter_items[2])->x10_modelDesc->x0_joint); // Intentional placeholder remains NULL.
    // Every imported appended descriptor has the source import's type/count.
    unsigned copied_records=0;for(u32 i=0;i<owned->import.record_count;++i){HSD_DatImportRecord*r=&owned->import.records[i];if(r->offset<st.source_base)continue;bool found=false;for(u32 j=0;j<primary->import.record_count;++j){HSD_DatImportRecord*q=&primary->import.records[j];if(q->offset+st.source_base==r->offset&&q->type==r->type&&q->count==r->count){found=true;break;}}CHECK(found);++copied_records;}CHECK(copied_records>0);
    char path[1024];snprintf(path,sizeof(path),"%s/owned-nana.dat",directory);FILE*f=fopen(path,"wb");CHECK(f);CHECK(fwrite(out,1,required,f)==required);fclose(f);
    printf("Retail entire three-article transplant: %u copied descriptors, %zu bytes, external visits 6 -> 2 (retained Nana animation placeholder + primary model placeholder)\n",copied_records,required);
    free(primary->arena);free(original->arena);free(owned->arena);free(primary);free(original);free(owned);free(d);free(s);free(out);
}
int main(int argc,char**argv){CHECK(argc==2);synthetic_tests();retail_tests(argv[1]);printf("PASS DAT pointer transplant: %u checks, %u transactional rejects\n",checks,rejects);return 0;}
