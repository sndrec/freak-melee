#include <melee/ft/native_fighter_animation_editor.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
// Sampling uses a stack FObj; native pools remain unbound, so allocation asserts.
void OSReport(char* format,...){(void)format;}
void __assert(char* f,u32 l,char* m){fprintf(stderr,"%s:%u %s\n",f,l,m);abort();}
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static void put32(uint8_t* p,uint32_t x) { p[0]=x>>24;p[1]=x>>16;p[2]=x>>8;p[3]=x; }
static uint32_t get32(const uint8_t* p) { return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3]; }
static void putfloat(uint8_t* p,float f) { uint32_t x;memcpy(&x,&f,4);put32(p,x); }
static size_t clip(uint8_t* p,const char* name,float a,float b) {
    const unsigned data=96, reloc=3;size_t size=32+data+4*reloc+8+strlen(name)+1;
    memset(p,0,size);put32(p,size);put32(p+4,data);put32(p+8,reloc);put32(p+12,1);
    uint8_t* d=p+32;putfloat(d+8,20);put32(d+12,20);put32(d+16,24);d[20]=1;d[21]=255;
    d[25]=12;d[28]=HSD_A_J_TRAX;put32(d+32,40);
    d[40]=HSD_A_OP_LIN;memcpy(d+41,&a,4);d[45]=10;
    d[46]=HSD_A_OP_LIN;memcpy(d+47,&b,4);d[51]=0;
    put32(d+data,12);put32(d+data+4,16);put32(d+data+8,32);
    put32(d+data+12,0);put32(d+data+16,0);memcpy(d+data+20,name,strlen(name)+1);return size;
}
static NativeFighterProject project(void) {
    NativeFighterProject p={0};strcpy(p.fighter_root,"fighter");p.action_count=2;p.demo_count=4;
    p.animation_dat=malloc(2048);size_t a=clip(p.animation_dat,"action",1,3);
    size_t b=clip(p.animation_dat+a,"demo",2,4);size_t c=clip(p.animation_dat+a+b,"later",7,9);p.animation_size=a+b+c;
    const unsigned data=256;unsigned slots[]={12,20,32,56,80,104,128};
    p.fighter_size=32+data+sizeof(slots)+8+8;p.fighter_dat=calloc(1,p.fighter_size);
    uint8_t* f=p.fighter_dat;uint8_t* d=f+32;put32(f,p.fighter_size);put32(f+4,data);put32(f+8,7);put32(f+12,1);
    put32(d+12,32);put32(d+20,80);strcpy((char*)d+192,"action");strcpy((char*)d+200,"demo");strcpy((char*)d+208,"later");
    unsigned names[]={192,200,200,208,192};unsigned offsets[]={0,a,a,a+b,0};unsigned sizes[]={a,b,b,c,a};
    for(unsigned i=0;i<5;++i){unsigned r=i<2?32+i*24:80+(i-2)*24;put32(d+r,names[i]);put32(d+r+4,offsets[i]);put32(d+r+8,sizes[i]);memset(d+r+12,0xA0+i,12);}
    for(unsigned i=0;i<7;++i)put32(d+data+i*4,slots[i]);memcpy(d+data+sizeof(slots)+8,"fighter",8);return p;
}
static void close_project(NativeFighterProject* p){free(p->fighter_dat);free(p->animation_dat);}
int main(void) {
    NativeFighterProject p=project();NativeFighterAnimationClip demo={NativeFighterAnimation_Demo,0},action={NativeFighterAnimation_Action,0},alias={NativeFighterAnimation_Action,1},later={NativeFighterAnimation_Demo,1};
    NativeFighterAnimationInfo i,j;NativeFighterAnimationTrack tracks[4];NativeFighterAnimationKey keys[4];size_t n;float value;
    CHECK(nativeFighterAnimation_Info(&p,demo,&i)&&i.frames==20&&i.tracks==1);
    CHECK(nativeFighterAnimation_Tracks(&p,demo,tracks,4,&n)&&n==1&&tracks[0].keys==2);
    CHECK(nativeFighterAnimation_Keys(&p,demo,0,keys,4,&n)&&n==2&&keys[1].frame==10&&keys[1].value==4);
    CHECK(nativeFighterAnimation_Sample(&p,demo,0,5,&value)&&fabsf(value-3)<0.00001f);
    CHECK(nativeFighterAnimation_Snap(&p,demo,0,false,6,&value)&&value==6);
    CHECK(nativeFighterAnimation_SetKey(&p,demo,0,1,6));
    CHECK(nativeFighterAnimation_SetFrames(&p,demo,25));
    CHECK(nativeFighterAnimation_Sample(&p,demo,0,5,&value)&&fabsf(value-4)<0.00001f);
    CHECK(!nativeFighterAnimation_SetTangent(&p,demo,0,0,1));
    CHECK(!nativeFighterAnimation_MoveKey(&p,demo,0,0,3));
    uint8_t fighter[512];memcpy(fighter,p.fighter_dat,p.fighter_size);uint8_t action_bytes[512],later_bytes[512];
    CHECK(nativeFighterAnimation_Info(&p,action,&j));memcpy(action_bytes,p.animation_dat+j.clip_offset,j.clip_size);unsigned action_size=j.clip_size;
    CHECK(nativeFighterAnimation_Info(&p,later,&j));memcpy(later_bytes,p.animation_dat+j.clip_offset,j.clip_size);unsigned later_size=j.clip_size,old_later=j.clip_offset;
    CHECK(nativeFighterAnimation_InsertKey(&p,demo,0,5,5));
    CHECK(nativeFighterAnimation_Info(&p,demo,&i));unsigned growth=i.clip_size-get32(fighter+32+80+8);CHECK(growth>0);
    CHECK(nativeFighterAnimation_Info(&p,alias,&j)&&j.clip_offset==i.clip_offset&&j.clip_size==i.clip_size);
    CHECK(get32(p.fighter_dat+32+104+4)==old_later+growth);
    CHECK(nativeFighterAnimation_Info(&p,later,&j)&&j.clip_size==later_size&&!memcmp(later_bytes,p.animation_dat+j.clip_offset,later_size));
    CHECK(!memcmp(action_bytes,p.animation_dat,action_size));
    for(unsigned r=32;r<152;r+=24)CHECK(!memcmp(fighter+32+r+12,p.fighter_dat+32+r+12,12));
    CHECK(nativeFighterAnimation_MoveKey(&p,demo,0,1,6));
    CHECK(nativeFighterAnimation_DeleteKey(&p,demo,0,1));
    CHECK(nativeFighterAnimation_KeyChannel(&p,demo,0,HSD_A_J_TRAY,5,8,2));
    CHECK(nativeFighterAnimation_Tracks(&p,demo,tracks,4,&n)&&n==2);
    CHECK(nativeFighterAnimation_Sample(&p,demo,1,5,&value)&&fabsf(value-8)<0.00001f);
    NativeFighterAnimationClip bad[]={{NativeFighterAnimation_Demo,3},{NativeFighterAnimation_Demo,4},{(NativeFighterAnimationBank)9,0}};
    for(unsigned k=0;k<3;++k){NativeFighterAnimationInfo sentinel={99};CHECK(!nativeFighterAnimation_Info(&p,bad[k],&sentinel)&&sentinel.frames==99);}
    size_t old_size=p.animation_size;uint8_t* before=malloc(old_size);memcpy(before,p.animation_dat,old_size);memcpy(fighter,p.fighter_dat,p.fighter_size);
    put32(p.fighter_dat+32+20,255);uint8_t malformed[512];memcpy(malformed,p.fighter_dat,p.fighter_size);
    CHECK(!nativeFighterAnimation_InsertKey(&p,action,0,5,2));CHECK(p.animation_size==old_size&&!memcmp(before,p.animation_dat,old_size)&&!memcmp(malformed,p.fighter_dat,p.fighter_size));
    memcpy(p.fighter_dat,fighter,p.fighter_size);free(before);close_project(&p);
    p=project();
    // Growing an action also shifts both demo references, including a shared alias.
    CHECK(nativeFighterAnimation_Info(&p,demo,&i));unsigned prior=i.clip_offset;
    CHECK(nativeFighterAnimation_KeyChannel(&p,action,0,HSD_A_J_TRAY,4,6,1));
    CHECK(nativeFighterAnimation_Info(&p,demo,&i)&&i.clip_offset>prior);
    CHECK(nativeFighterAnimation_Info(&p,alias,&j)&&i.clip_offset==j.clip_offset);
    CHECK(nativeFighterAnimation_Info(&p,(NativeFighterAnimationClip){NativeFighterAnimation_Demo,2},&j)&&j.clip_offset==0);
    close_project(&p);
    p=project();CHECK(nativeFighterAnimation_Info(&p,demo,&i));
    // A float spline carries independently editable value and tangent payloads.
    uint8_t* stream=p.animation_dat+i.clip_offset+32+40;stream[0]=HSD_A_OP_SPL;
    float slope=0.25f;memcpy(stream+5,&slope,4);stream[9]=10;stream[10]=HSD_A_OP_SPL;
    float endvalue=4;memcpy(stream+11,&endvalue,4);memcpy(stream+15,&slope,4);stream[19]=0;
    p.animation_dat[i.clip_offset+32+25]=20;
    CHECK(nativeFighterAnimation_Keys(&p,demo,0,keys,4,&n)&&n==2&&keys[0].tangent_editable);
    CHECK(nativeFighterAnimation_SetTangent(&p,demo,0,0,0.5f));
    CHECK(nativeFighterAnimation_Keys(&p,demo,0,keys,4,&n)&&keys[0].tangent==0.5f);
    CHECK(nativeFighterAnimation_Sample(&p,demo,0,5,&value)&&isfinite(value));
    close_project(&p);
    printf("{\"checks\":%u,\"status\":\"PASS\"}\n",checks);return 0;
}
