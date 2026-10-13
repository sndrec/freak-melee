#include <melee/ft/native_fighter_pose_editor.h>
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

static float sample(NativeFighterProject* p,NativeFighterAnimationClip c,unsigned joint,unsigned channel,float frame) {
    NativeFighterAnimationTrack tracks[8];size_t n;float value=-999;
    CHECK(nativeFighterAnimation_Tracks(p,c,tracks,8,&n));
    for(unsigned i=0;i<n;++i)if(tracks[i].joint==joint&&tracks[i].channel==channel){CHECK(nativeFighterAnimation_Sample(p,c,i,frame,&value));return value;}
    CHECK(0);return value;
}
static void cross_bank(NativeFighterAnimationClip source,NativeFighterAnimationClip destination) {
    NativeFighterProject p=project();NativeFighterAnimationClip clips[]={{NativeFighterAnimation_Action,0},{NativeFighterAnimation_Demo,0},{NativeFighterAnimation_Demo,1}};
    for(unsigned i=0;i<3;++i){CHECK(nativeFighterAnimation_KeyChannel(&p,clips[i],0,HSD_A_J_ROTX,10,2*(10+i),0));CHECK(nativeFighterAnimation_KeyChannel(&p,clips[i],1,HSD_A_J_TRAY,10,2*(20+i),0));}
    NativeFighterPose pose;
    CHECK(nativeFighterPose_Capture(&p,source,5,&pose));
    CHECK(pose.source_clip.bank==source.bank&&pose.source_clip.index==source.index&&pose.source_frame==5&&pose.root_joint==0&&pose.sampled_channels==3);
    float root_before=sample(&p,destination,0,HSD_A_J_TRAX,5);
    unsigned written=99,skipped=99,failed=99;
    CHECK(nativeFighterPose_Paste(&p,&pose,destination,5,NULL,false,false,&written,&skipped,&failed));
    CHECK(written==2&&skipped==0&&failed==UINT32_MAX);
    CHECK(sample(&p,destination,0,HSD_A_J_TRAX,5)==root_before);
    CHECK(sample(&p,destination,0,HSD_A_J_ROTX,5)==pose.values[0][HSD_A_J_ROTX]);
    CHECK(sample(&p,destination,1,HSD_A_J_TRAY,5)==pose.values[1][HSD_A_J_TRAY]);
    // Selected paste updates only the selected joint; source snapshot is independent of future source edits.
    unsigned char selected[NATIVE_FIGHTER_POSE_JOINTS]={0};selected[1]=1;pose.values[1][HSD_A_J_TRAY]=77;pose.values[0][HSD_A_J_ROTX]=88;
    float rotation_before=sample(&p,destination,0,HSD_A_J_ROTX,6);
    CHECK(nativeFighterPose_Paste(&p,&pose,destination,6,selected,true,true,&written,&skipped,&failed));CHECK(written==1&&skipped==0);
    CHECK(sample(&p,destination,1,HSD_A_J_TRAY,6)==77);CHECK(sample(&p,destination,0,HSD_A_J_ROTX,6)==rotation_before);
    // The first channel succeeds on the private working bank before a later NaN channel fails.
    size_t animation_size=p.animation_size;uint8_t* animation=malloc(animation_size);uint8_t* fighter=malloc(p.fighter_size);memcpy(animation,p.animation_dat,animation_size);memcpy(fighter,p.fighter_dat,p.fighter_size);
    pose.values[0][HSD_A_J_ROTX]=NAN;written=123;skipped=456;
    CHECK(!nativeFighterPose_Paste(&p,&pose,destination,7,NULL,false,true,&written,&skipped,&failed));
    CHECK(written==123&&skipped==456&&failed==1&&animation_size==p.animation_size&&!memcmp(animation,p.animation_dat,animation_size)&&!memcmp(fighter,p.fighter_dat,p.fighter_size));
    pose.values[0][HSD_A_J_ROTX]=8;pose.present[2][HSD_A_J_ROTX]=1;pose.values[2][HSD_A_J_ROTX]=4;
    CHECK(nativeFighterPose_Paste(&p,&pose,destination,7,NULL,false,true,&written,&skipped,&failed));CHECK(written==3&&skipped==1);
    // The exact paired buffers supplied to structural undo restore a readable prior bank and descriptors.
    CHECK(p.animation_size>animation_size);free(p.animation_dat);p.animation_dat=animation;p.animation_size=animation_size;memcpy(p.fighter_dat,fighter,p.fighter_size);free(fighter);
    CHECK(sample(&p,destination,1,HSD_A_J_TRAY,6)==77);
    NativeFighterPose untouched=pose;CHECK(!nativeFighterPose_Capture(&p,(NativeFighterAnimationClip){NativeFighterAnimation_Demo,3},0,&pose));CHECK(!memcmp(&pose,&untouched,sizeof(pose)));
    CHECK(!nativeFighterPose_Paste(&p,&pose,(NativeFighterAnimationClip){NativeFighterAnimation_Demo,3},0,NULL,false,true,&written,&skipped,&failed));
    close_project(&p);
}
int main(void) {
    cross_bank((NativeFighterAnimationClip){NativeFighterAnimation_Demo,0},(NativeFighterAnimationClip){NativeFighterAnimation_Action,0});
    cross_bank((NativeFighterAnimationClip){NativeFighterAnimation_Action,0},(NativeFighterAnimationClip){NativeFighterAnimation_Demo,0});
    cross_bank((NativeFighterAnimationClip){NativeFighterAnimation_Demo,0},(NativeFighterAnimationClip){NativeFighterAnimation_Demo,1});
    printf("{\"checks\":%u,\"bank_pairs\":3,\"status\":\"PASS\"}\n",checks);return 0;
}
