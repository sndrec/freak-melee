#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftparts.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static unsigned rt_checks, rt_pairs, rt_samples;
#define REQUIRE(x) do { ++rt_checks; if(!(x)){fprintf(stderr,"retarget line%d: %s\n",__LINE__,#x);exit(1);} } while(0)
static HSD_FObj fobjects[4096];static unsigned fcount;
static HSD_AObj aobjects[256];static unsigned acount;
static FighterBone bones[2][140];static unsigned bcount;
static HSD_DObj* dummy_dobjs[2][124];static unsigned dcount;
HSD_ObjAllocData fighter_parts_alloc_data,fighter_dobj_list_alloc_data;
void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{
    if(data==&fighter_parts_alloc_data){REQUIRE(bcount<2);return bones[bcount++];}
    if(data==&fighter_dobj_list_alloc_data){REQUIRE(dcount<2);return dummy_dobjs[dcount++];}
    REQUIRE(data==HSD_FObjGetAllocData() && fcount<4096);return fobjects+fcount++;
}
void HSD_ObjFree(HSD_ObjAllocData* data,void* object){(void)data;(void)object;}
void HSD_ObjAllocInit(HSD_ObjAllocData* data,size_t size,u32 align){(void)data;(void)size;(void)align;}
HSD_AObj* HSD_AObjAlloc(void){REQUIRE(acount<256);HSD_AObj* a=aobjects+acount++;memset(a,0,sizeof(*a));return a;}
void HSD_AObjRemove(HSD_AObj* a){(void)a;}
void HSD_AObjSetFlags(HSD_AObj* a,u32 flags){a->flags=flags;}
void HSD_AObjSetRewindFrame(HSD_AObj* a,float frame){a->rewind_frame=frame;}
void HSD_AObjSetEndFrame(HSD_AObj* a,float frame){a->end_frame=frame;}
void HSD_AObjSetFObj(HSD_AObj* a,HSD_FObj* f){a->fobj=f;}
void HSD_JObjSetFlags(HSD_JObj* j,u32 flags){j->flags|=flags;}
void HSD_JObjClearFlags(HSD_JObj* j,u32 flags){j->flags&=~flags;}
void OSReport(char* format,...){(void)format;}
void __assert(char* file,u32 line,char* message){fprintf(stderr,"assert %s:%lu %s\n",file,line,message);exit(1);}

#include "retarget_bodies.h"

static void* read_file(const char* path,size_t* length)
{
    FILE* f=fopen(path,"rb");REQUIRE(f && !fseek(f,0,SEEK_END));long n=ftell(f);
    REQUIRE(n>0 && !fseek(f,0,SEEK_SET));void* bytes=malloc(n);REQUIRE(bytes && fread(bytes,1,n,f)==(size_t)n);
    REQUIRE(!fclose(f));*length=n;return bytes;
}
static FighterPartsTable maps[2];static u8 joint_to_part[2][256],part_to_joint[2][54];
static struct Fighter_804D6540_t omitted[2];static struct Fighter_804D6540_x0_t omissions[2][32];
static FighterPartsTable* map_ptrs[Ft_Kind_RuntimeCapacity];
static struct Fighter_804D6540_t* omission_ptrs[Ft_Kind_RuntimeCapacity];
static void common(const char* path)
{
    size_t length;void* bytes=read_file(path,&length);HSD_Dat dat;u32 root,table,skip,at;const char* ext;
    REQUIRE(HSD_DatOpen(&dat,bytes,length)==HSD_DAT_OK && HSD_DatFindRoot(&dat,"ftLoadCommonData",&root));
    REQUIRE(HSD_DatResolve(&dat,root+16,&table,&ext)==HSD_DAT_REF_LOCAL);
    REQUIRE(HSD_DatResolve(&dat,root+20,&skip,&ext)==HSD_DAT_REF_LOCAL);
    unsigned kinds[]={Ft_Kind_Nana,Ft_Kind_None};
    for(unsigned i=0;i<2;++i){
        REQUIRE(HSD_DatResolve(&dat,table+kinds[i]*4,&at,&ext)==HSD_DAT_REF_LOCAL);
        REQUIRE(ftData_DecodeParts(&dat,at,maps+i,joint_to_part[i],256,part_to_joint[i])==HSD_DAT_OK);
        HSD_DatReferenceKind ref=HSD_DatResolve(&dat,skip+kinds[i]*4,&at,&ext);
        REQUIRE(ref==HSD_DAT_REF_NULL || ref==HSD_DAT_REF_LOCAL);
        if(ref==HSD_DAT_REF_LOCAL)REQUIRE(ftData_DecodePartOmissions(&dat,at,omitted+i,omissions[i],32,maps[i].parts_num)==HSD_DAT_OK);
        map_ptrs[kinds[i]]=maps+i;omission_ptrs[kinds[i]]=omitted+i;
    }
    map_ptrs[Ft_Kind_CustomFirst]=maps;omission_ptrs[Ft_Kind_CustomFirst]=omitted;
    ftPartsTable=map_ptrs;Fighter_804D6540=omission_ptrs;free(bytes);
}
static HSD_JObj joints[2][140][2];static unsigned physical;
static void skeleton(Fighter* fp,HSD_Joint* desc,unsigned depth,unsigned side)
{
    for(;desc;desc=desc->next){
        while(physical<maps[0].parts_num && ftParts_8007506C(fp->kind,physical))++physical;
        REQUIRE(physical<maps[0].parts_num);unsigned i=physical++;
        FighterBone* b=fp->parts+i;b->flags_b1=true;b->xC=depth;
        for(unsigned blend=0;blend<2;++blend){
            HSD_JObj* j=&joints[side][i][blend];memset(j,0,sizeof(*j));j->flags=desc->flags;
            j->rotate.x=desc->rotation.x;j->rotate.y=desc->rotation.y;j->rotate.z=desc->rotation.z;
            j->scale=desc->scale;j->translate=desc->position;
            if(blend)b->x4_jobj2=j;else b->joint=j;
        }
        if(!(desc->flags&JOBJ_INSTANCE))skeleton(fp,desc->child,depth+1,side);
    }
}
// Observation boundary: the actual FObj decoder/interpolator supplies scalar
// SRT channels. Matrix/IK/render evaluation is outside this import contract.
static void sample(void* object,enum_t type,HSD_ObjData* value)
{
    HSD_JObj* j=object;
    switch(type){
    case 1:j->rotate.x=value->fv;break;case 2:j->rotate.y=value->fv;break;case 3:j->rotate.z=value->fv;break;
    case 5:j->translate.x=value->fv;break;case 6:j->translate.y=value->fv;break;case 7:j->translate.z=value->fv;break;
    case 8:j->scale.x=value->fv;break;case 9:j->scale.y=value->fv;break;case 10:j->scale.z=value->fv;break;
    default:break;
    }
}
static void pair(FigaTree* reference,FigaTree* actual,s32 flags,HSD_Joint* costume,unsigned part,bool partial,bool blend)
{
    static Fighter fp[2];memset(fp,0,sizeof(fp));memset(bones,0,sizeof(bones));bcount=dcount=fcount=acount=0;
    for(unsigned side=0;side<2;++side){
        fp[side].kind=side?Ft_Kind_CustomFirst:Ft_Kind_Nana;
        ftParts_80074E58(fp+side);physical=0;skeleton(fp+side,costume,0,side);
        REQUIRE(physical==maps[0].parts_num);
        fp[side].x594_s32=flags;fp[side].x590=side?actual:reference;
        if(fp[side].x597_bits==Ft_Kind_Nana && side)fp[side].x597_bits=Ft_Kind_CustomFirst;
        if(partial)ftAnim_8006F954(fp+side,part,blend,fp[side].x590);else ftAnim_8006FE08(fp+side,blend);
    }
    for(unsigned i=0;i<maps[0].parts_num;++i){
        HSD_AObj* a=joints[0][i][blend].aobj;HSD_AObj* b=joints[1][i][blend].aobj;
        REQUIRE((a==NULL)==(b==NULL));if(!a)continue;
        REQUIRE(a->flags==b->flags && a->end_frame==b->end_frame);
        HSD_FObj* x=a->fobj;HSD_FObj* y=b->fobj;
        while(x && y){REQUIRE(x->obj_type==y->obj_type && x->length==y->length && x->startframe==y->startframe &&
            x->frac_value==y->frac_value && x->frac_slope==y->frac_slope && !memcmp(x->ad_head,y->ad_head,x->length));x=x->next;y=y->next;}
        REQUIRE(!x && !y);
        const float frames[]={0,0.25f,1,5.5f,17,31.75f};
        for(unsigned n=0;n<6;++n){
            HSD_FObjReqAnimAll(a->fobj,frames[n]);HSD_FObjReqAnimAll(b->fobj,frames[n]);
            HSD_FObjInterpretAnimAll(a->fobj,&joints[0][i][blend],sample,0);
            HSD_FObjInterpretAnimAll(b->fobj,&joints[1][i][blend],sample,0);
            REQUIRE(!memcmp(&joints[0][i][blend].rotate,&joints[1][i][blend].rotate,sizeof(Quaternion)));
            REQUIRE(!memcmp(&joints[0][i][blend].scale,&joints[1][i][blend].scale,sizeof(Vec3)));
            REQUIRE(!memcmp(&joints[0][i][blend].translate,&joints[1][i][blend].translate,sizeof(Vec3)));++rt_samples;
        }
    }
    ++rt_pairs;
}
void test_retarget(FtCharacterAssets* original,FtCharacterAssets* primary,FtCharacterAssets* owned,char** paths)
{
    common(paths[0]);const char* names[]={"PlyNana5K_Share_joint","PlyNana5KYe_Share_joint","PlyNana5KAq_Share_joint","PlyNana5KWh_Share_joint"};
    for(unsigned costume=0;costume<4;++costume){
        size_t length;void* bytes=read_file(paths[costume+1],&length);void* arena=malloc(FT_COSTUME_IMPORT_ARENA_SIZE);REQUIRE(arena);
        FtCostumeAssets assets;REQUIRE(ftCostumeAssets_Import(bytes,length,names[costume],NULL,arena,FT_COSTUME_IMPORT_ARENA_SIZE,FT_COSTUME_IMPORT_RECORD_CAPACITY,&assets));
        for(unsigned i=0;i<321;++i){
            const struct Fighter_WaitAnimData* raw=original->data.xC+i;
            FigaTree* reference=raw->x8?raw->x14:primary->data.xC[i].x14;FigaTree* actual=owned->data.xC[i].x14;
            if(!actual)continue;
            for(unsigned blend=0;blend<2;++blend){
                pair(reference,actual,raw->x10_animCurrFlags,assets.joint,0,false,blend);
                const unsigned starts[]={0,1,4,14};
                for(unsigned n=0;n<4;++n)pair(reference,actual,raw->x10_animCurrFlags,assets.joint,starts[n],true,blend);
            }
        }
        free(arena);free(bytes);
    }
    fprintf(stderr,"retarget %u pairs / %u scalar pose samples / %u checks\n",rt_pairs,rt_samples,rt_checks);
}
