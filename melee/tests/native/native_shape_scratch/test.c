#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/debug.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <setjmp.h>
#include <dolphin/gx.h>

static unsigned checks, pairs, allocations;
static jmp_buf refusal;
static bool expect_refusal;
static unsigned refusals;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"shape %u: %s\n",__LINE__,#x); exit(1); } } while(0)
void __assert(char* file,u32 line,char* condition)
{ if(expect_refusal){++refusals;longjmp(refusal,1);} fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,condition); exit(2); }
void HSD_Panic(char* file,u32 line,char* condition)
{ __assert(file,line,condition); }
void* HSD_MemAlloc(ssize_t bytes)
{ ++allocations; CHECK(bytes>0); void* result=malloc((size_t)bytes); CHECK(result); return result; }

static float output_vertices[2][3],output_normals[6][3];
static unsigned output_normal_count;
static float emitted_vertices[2][3],emitted_normals[6][3];
static unsigned emitted_vertex_count,emitted_normal_count;
void GXBegin(GXPrimitive primitive,GXVtxFmt format,u16 count)
{ CHECK(primitive==GX_LINES&&format==GX_VTXFMT0&&count==2);emitted_vertex_count=emitted_normal_count=0; }
void GXPosition3f32(f32 x,f32 y,f32 z)
{ CHECK(emitted_vertex_count<2);memcpy(emitted_vertices[emitted_vertex_count++],(float[3]){x,y,z},3*sizeof(float)); }
void GXNormal3f32(f32 x,f32 y,f32 z)
{ CHECK(emitted_normal_count<6);memcpy(emitted_normals[emitted_normal_count++],(float[3]){x,y,z},3*sizeof(float)); }
void GXTexCoord1u8(u8 x){(void)x;CHECK(false);}
void GXTexCoord1x8(u8 x){(void)x;CHECK(false);}
void GXTexCoord1x16(u16 x){(void)x;CHECK(false);}
void GXColor1u16(u16 x){(void)x;CHECK(false);}
void GXColor1x8(u8 x){(void)x;CHECK(false);}
void GXColor1x16(u16 x){(void)x;CHECK(false);}
void GXColor3u8(u8 x,u8 y,u8 z){(void)x;(void)y;(void)z;CHECK(false);}
void GXColor4u8(u8 x,u8 y,u8 z,u8 w){(void)x;(void)y;(void)z;(void)w;CHECK(false);}
void OSReport(char* format,...){(void)format;CHECK(false);}
#include "emission.h"
static void(*emit)(HSD_PObj*,float(*)[3],float(*)[3]);
static void capture(HSD_PObj* polygon,float (*vertices)[3],float (*normals)[3])
{
    HSD_ShapeSet* shape=polygon->u.shape_set;
    CHECK(shape->nb_vertex_index<=2&&shape->nb_normal_index<=2);
    output_normal_count=shape->nb_normal_index*(shape->normal_desc&&shape->normal_desc->attr==GX_VA_NBT?3:1);
    memcpy(output_vertices,vertices,shape->nb_vertex_index*sizeof(*vertices));
    memcpy(output_normals,normals,output_normal_count*sizeof(*normals));
    emit(polygon,vertices,normals);
}
#include "helpers.h"
#define pobj_min(x,y) ((x)<(y)?(x):(y))
#include "prior.h"
#include "fixed.h"

static void pair(HSD_PObj* polygon)
{
    emit=prior_emission;prior_draw(polygon);
    float vertices[2][3],normals[6][3];
    memcpy(vertices,output_vertices,sizeof(vertices));memcpy(normals,output_normals,sizeof(normals));
    unsigned normal_count=output_normal_count;
    CHECK(emitted_vertex_count==2&&emitted_normal_count==normal_count);
    CHECK(!memcmp(emitted_vertices,vertices,sizeof(vertices)));
    CHECK(!memcmp(emitted_normals,normals,normal_count*sizeof(normals[0])));
    // Changing all fixed scratch demonstrates that the consumed values are
    // regenerated; no old-frame scratch participates in the result.
    for(unsigned i=0;i<HSD_DEFAULT_MAX_SHAPE_VERTICES;++i)
        for(unsigned j=0;j<3;++j)fixed_vertices[i][j]=-12345.0f;
    for(unsigned i=0;i<HSD_DEFAULT_MAX_SHAPE_NORMALS;++i)
        for(unsigned j=0;j<3;++j)fixed_normals[i][j]=98765.0f;
    unsigned before=allocations;emit=fixed_emission;fixed_draw(polygon);CHECK(allocations==before);
    CHECK(output_normal_count==normal_count);
    CHECK(!memcmp(vertices,output_vertices,sizeof(vertices)));
    CHECK(!memcmp(normals,output_normals,normal_count*sizeof(normals[0])));
    CHECK(emitted_vertex_count==2&&emitted_normal_count==normal_count);
    CHECK(!memcmp(emitted_vertices,vertices,sizeof(vertices)));
    CHECK(!memcmp(emitted_normals,normals,normal_count*sizeof(normals[0])));
    ++pairs;
}

int main(void)
{
    float vertices[6][3],normals[6][9];
    u8 vertices_be[sizeof(vertices)],normals_be[sizeof(normals)];
    for(unsigned i=0;i<6;++i){
        for(unsigned j=0;j<3;++j)vertices[i][j]=(float)((int)i*7+(int)j-13)*.125f;
        for(unsigned j=0;j<9;++j)normals[i][j]=(float)((int)i*11-(int)j+3)*.0625f;
    }
    for(unsigned i=0;i<sizeof(vertices)/4;++i){u32 bits;memcpy(&bits,(u8*)vertices+i*4,4);for(unsigned j=0;j<4;++j)vertices_be[i*4+j]=(u8)(bits>>(24-j*8));}
    for(unsigned i=0;i<sizeof(normals)/4;++i){u32 bits;memcpy(&bits,(u8*)normals+i*4,4);for(unsigned j=0;j<4;++j)normals_be[i*4+j]=(u8)(bits>>(24-j*8));}
    u8 index8[3][2]={{0,1},{2,3},{4,5}};
    u8 index16[3][4]={{0,0,0,1},{0,2,0,3},{0,4,0,5}};
    u8* indices[3];float weights[2]={.25f,.75f};
    HSD_VtxDescList vertex={.attr=GX_VA_POS,.comp_cnt=GX_POS_XYZ,.comp_type=GX_F32,.stride=sizeof(vertices[0])};
    HSD_VtxDescList normal={.comp_cnt=GX_NRM_XYZ,.comp_type=GX_F32,.stride=sizeof(normals[0])};
    HSD_ShapeSet shape={.nb_shape=2,.nb_vertex_index=2,.vertex_desc=&vertex,.vertex_idx_list=indices,.nb_normal_index=2,.normal_desc=&normal,.normal_idx_list=indices};
    HSD_PObj polygon={.flags=POBJ_SHAPEANIM,.u.shape_set=&shape};
    u8 display[32]={0};HSD_VtxDescList attributes[3]={0};
    polygon.display=display;polygon.n_display=1;polygon.verts=attributes;
    const float blends[]={-1,0,.125f,.5f,.999f,1,1.75f,2,9};
    for(unsigned little=0;little<2;++little)for(unsigned indexed16=0;indexed16<2;++indexed16)for(unsigned nbt=0;nbt<2;++nbt){
        vertex.vertex=little?(void*)vertices:(void*)vertices_be;vertex.vertex_little_endian=!!little;
        normal.vertex=little?(void*)normals:(void*)normals_be;normal.vertex_little_endian=!!little;
        vertex.attr_type=normal.attr_type=indexed16?GX_INDEX16:GX_INDEX8;
        normal.attr=nbt?GX_VA_NBT:GX_VA_NRM;
        attributes[0]=vertex;attributes[1]=normal;attributes[2].attr=GX_VA_NULL;
        memset(display,0,sizeof(display));display[0]=GX_LINES;display[2]=2;
        unsigned at=3;
        for(unsigned i=0;i<2;++i)for(unsigned j=0;j<2;++j){if(indexed16)display[at++]=0;display[at++]=(u8)i;}
        for(unsigned i=0;i<3;++i)indices[i]=indexed16?index16[i]:index8[i];
        shape.flags=SHAPESET_AVERAGE;
        for(unsigned i=0;i<sizeof(blends)/sizeof(blends[0]);++i){shape.blend.bl=blends[i];pair(&polygon);}
        shape.flags=SHAPESET_ADDITIVE;shape.blend.bp=weights;
        for(unsigned i=0;i<4;++i){weights[0]=(float)i*.5f-.25f;weights[1]=(float)(3-i)*.25f;pair(&polygon);}
        // A vertex-only draw must not consume old normal scratch.
        shape.normal_desc=NULL;shape.nb_normal_index=0;attributes[1].attr=GX_VA_NULL;
        memset(display,0,sizeof(display));display[0]=GX_LINES;display[2]=2;at=3;
        for(unsigned i=0;i<2;++i){if(indexed16)display[at++]=0;display[at++]=(u8)i;}
        pair(&polygon);
        shape.normal_desc=&normal;shape.nb_normal_index=2;
    }
    // The old capacity failures remain failures, including signed negatives.
    for(unsigned n=0;n<5;++n){
        shape.nb_vertex_index=n==0?-1:n==1?2001:2;
        shape.nb_normal_index=n==2?-1:n==3?2001:667;normal.attr=n==4?GX_VA_NBT:GX_VA_NRM;
        for(unsigned old=0;old<2;++old){
            expect_refusal=true;
            if(!setjmp(refusal)){if(old)prior_draw(&polygon);else fixed_draw(&polygon);CHECK(false);}
            expect_refusal=false;
        }
    }
    // A display list may not consume an old scratch element beyond this draw.
    shape.nb_vertex_index=shape.nb_normal_index=2;normal.attr=GX_VA_NRM;
    attributes[0]=vertex;attributes[1].attr=GX_VA_NULL;
    memset(display,0,sizeof(display));display[0]=GX_LINES;display[2]=2;display[3]=0;display[4]=2;
    expect_refusal=true;emit=fixed_emission;
    if(!setjmp(refusal)){fixed_draw(&polygon);CHECK(false);}expect_refusal=false;
    for(unsigned nbt=0;nbt<2;++nbt){
        attributes[1]=normal;attributes[1].attr=nbt?GX_VA_NBT:GX_VA_NRM;
        attributes[2].attr=GX_VA_NULL;
        memset(display,0,sizeof(display));display[0]=GX_LINES;display[2]=2;
        display[3]=0;display[4]=0;display[5]=0;display[6]=2;
        expect_refusal=true;
        if(!setjmp(refusal)){fixed_emission(&polygon,fixed_vertices,fixed_normals);CHECK(false);}expect_refusal=false;
    }
    CHECK(allocations==2);free(vertex_buffer);free(normal_buffer);
    printf("{\"checks\":%u,\"kernel_pairs\":%u,\"capacity_and_index_refusals\":%u,\"prior_lazy_allocations\":%u,\"fixed_lazy_allocations\":0,\"scratch_bytes\":%zu}\n",checks,pairs,refusals,allocations,sizeof(fixed_vertices)+sizeof(fixed_normals));
    return 0;
}
