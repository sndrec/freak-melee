#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <melee/ft/types.h>
#include <melee/ft/native_cpu_history.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/inlines.h>
#include <sysdolphin/baselib/gobj.h>
#include "history_stock.h"
static Fighter current,reference,input;static HSD_GObj receiver,source;static bool receiver_live,source_live;static unsigned checks,pairs;
#define CHECK(x) do{++checks;assert(x);}while(0)
bool nativeCallbackSource_Snapshot(HSD_GObj* object,uint32_t type,uint32_t* generation){CHECK(type==NativeCallbackSource_Fighter);if(object!=&receiver||!receiver_live)return false;*generation=current.x8_spawnNum;return true;}
HSD_GObj* ftLib_8008741C(u32 id){return source_live&&id==(uint32_t)input.x8_spawnNum?&source:NULL;}
static void reset(unsigned write,unsigned read)
{
 memset(&current,0xa5,sizeof(current));memset(&reference,0xa5,sizeof(reference));memset(&input,0,sizeof(input));
 receiver=(HSD_GObj){.user_data=&current};source=(HSD_GObj){.user_data=&input};current.gobj=&receiver;current.x8_spawnNum=7;input.gobj=&source;input.x8_spawnNum=(s32)0x80000001;
 current.cpu.x444=current.cpu.xFC+write;current.cpu.x448=current.cpu.xFC+read;reference.cpu.x444=reference.cpu.xFC+write;reference.cpu.x448=reference.cpu.xFC+read;receiver_live=source_live=true;
 input.cur_pos=(Vec3){3,-4,5};input.facing_dir=-1;input.input.held_buttons[0]=0xffffffff;
}
int main(void)
{
 const float axes[]={0,.001f,.25f,.5f,.999f,1};const float triggers[]={-.5f,0,.01f,.5f,.999f,1,1.25f,127,255,255.75f};
 for(unsigned w=0;w<30;++w)for(unsigned r=0;r<30;++r)for(unsigned a=0;a<6;++a)for(unsigned t=0;t<10;++t){reset(w,r);input.input.lstick[0]=(Vec2){axes[a],axes[5-a]};input.input.cstick[0]=(Vec2){axes[(a+1)%6],axes[(a+2)%6]};input.input.triggers[0]=triggers[t];for(unsigned step=0;step<3;++step){ftCo_800B0918(&input,&reference);CHECK(nativeCpuHistory_Record(&receiver,(u32)input.x8_spawnNum));CHECK(!memcmp(current.cpu.xFC,reference.cpu.xFC,sizeof(current.cpu.xFC)));CHECK(current.cpu.x444-current.cpu.xFC==reference.cpu.x444-reference.cpu.xFC);CHECK(current.cpu.x448-current.cpu.xFC==reference.cpu.x448-reference.cpu.xFC);Vec3 position={0};float facing=0;CHECK(nativeCpuHistory_ReadPose(&receiver,&position,&facing));CHECK(!memcmp(&position,&current.cpu.x448->cur_pos,sizeof(position))&&facing==current.cpu.x448->facing_dir);CHECK(nativeCpuHistory_Apply(&receiver));CHECK(!memcmp(&current.cpu.lstick,&current.cpu.x448->lstick,sizeof(S8Vec2))&&!memcmp(&current.cpu.cstick,&current.cpu.x448->cstick,sizeof(S8Vec2))&&current.cpu.buttons==current.cpu.x448->x0&&current.cpu.ltrigger==current.cpu.x448->x4&&current.cpu.rtrigger==current.cpu.x448->x5);++pairs;}}
 const float signed_axes[]={-1,-.999f,-.5f,-.01f,0,.01f,.5f,.999f,1};
 for(unsigned a=0;a<9;++a){reset(29,29);input.input.lstick[0]=(Vec2){signed_axes[a],signed_axes[a]};input.input.cstick[0]=input.input.lstick[0];CHECK(nativeCpuHistory_Record(&receiver,(u32)input.x8_spawnNum));int expected=(int)(signed_axes[a]*(signed_axes[a]<0?128:127));CHECK(current.cpu.xFC[0].lstick.x==expected&&current.cpu.xFC[0].cstick.y==expected&&current.cpu.x444==current.cpu.xFC&&current.cpu.x448==current.cpu.xFC);}
 for(unsigned mode=0;mode<13;++mode){reset(5,0);struct CpuFighter before=current.cpu;switch(mode){case 0:input.input.lstick[0].x=NAN;break;case 1:input.input.cstick[0].y=INFINITY;break;case 2:input.input.lstick[0].x=1.01f;break;case 3:input.input.lstick[0].y=-1.01f;break;case 4:input.input.triggers[0]=-1;break;case 5:input.input.triggers[0]=256;break;case 6:input.cur_pos.z=NAN;break;case 7:input.facing_dir=INFINITY;break;case 8:source_live=false;break;case 9:receiver_live=false;break;case 10:source.user_data=NULL;break;case 11:current.cpu.x444=(void*)((char*)current.cpu.xFC+1);before=current.cpu;break;case 12:current.cpu.x448=current.cpu.xFC+30;before=current.cpu;break;}CHECK(!nativeCpuHistory_Record(&receiver,(u32)input.x8_spawnNum));CHECK(!memcmp(&before,&current.cpu,sizeof(before)));}
 const u32 ids[]={0,1,0x1000001,0x80000001,0xffffffff};for(unsigned i=0;i<5;++i){reset(5,0);input.x8_spawnNum=(s32)ids[i];CHECK(nativeCpuHistory_Record(&receiver,ids[i])==(ids[i]!=0));}
 reset(5,0);Vec3 point={101,102,103};float face=104;current.cpu.x448=(void*)((char*)current.cpu.xFC+1);struct CpuFighter before=current.cpu;CHECK(!nativeCpuHistory_ReadPose(&receiver,&point,&face)&&point.x==101&&point.y==102&&point.z==103&&face==104);CHECK(!nativeCpuHistory_Apply(&receiver)&&!memcmp(&before,&current.cpu,sizeof(before)));CHECK(!nativeCpuHistory_ReadPose(&receiver,NULL,&face)&&!nativeCpuHistory_ReadPose(&receiver,&point,NULL));
 for(unsigned component=0;component<4;++component)for(unsigned special=0;special<3;++special){reset(5,0);current.cpu.x448->cur_pos=(Vec3){1,2,3};current.cpu.x448->facing_dir=1;float invalid=special==0?NAN:special==1?INFINITY:-INFINITY;if(component==3)current.cpu.x448->facing_dir=invalid;else ((float*)&current.cpu.x448->cur_pos)[component]=invalid;Vec3 unchanged={101,102,103};float unchanged_facing=104;CHECK(!nativeCpuHistory_ReadPose(&receiver,&unchanged,&unchanged_facing));CHECK(unchanged.x==101&&unchanged.y==102&&unchanged.z==103&&unchanged_facing==104);}
 printf("PASS %u defined original history pairs / %u checks; signed-negative authored cases separate\n",pairs,checks);return 0;
}
