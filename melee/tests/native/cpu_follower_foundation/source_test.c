#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <melee/ft/types.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <sysdolphin/baselib/gobj.h>
__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
static void* pages;static Fighter* fp;static HSD_GObj* object;static uint32_t generation;static bool live;static NativeFighterDefinition definitions[2];static unsigned selected,mutation,phase_at,count,checks;
#define CHECK(x) do{++checks;assert(x);}while(0)
static void protect(unsigned mode){unsigned long old;CHECK(VirtualProtect(pages,65536,mode,&old));}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){return &definitions[selected];}
bool nativeCallbackSource_Snapshot(HSD_GObj* source,uint32_t type,uint32_t* id){CHECK(source==object&&type==NativeCallbackSource_Fighter);if(!live)return false;*id=generation;return true;}
bool nativeCallbackSource_Alive(HSD_GObj* source,uint32_t id,uint32_t type){CHECK(source==object&&id==generation&&type==NativeCallbackSource_Fighter);return live&&fp->x8_spawnNum==generation;}
static void phase(unsigned i,Fighter* f){CHECK(f==fp);CHECK(i==++count);if(i!=phase_at)return;switch(mutation){case 1:live=false;protect(1);break;case 2:++fp->x8_spawnNum;break;case 3:fp->kind=Ft_Kind_Fox;break;case 4:fp->ft_data=(void*)2;break;case 5:object->user_data=(void*)3;break;case 6:selected=1;break;}}
static void phase_1(Fighter* f){phase(1,f);}static void phase_2(Fighter* f){phase(2,f);}static void phase_3(Fighter* f){phase(3,f);}static void phase_4(Fighter* f){phase(4,f);}static void phase_5(Fighter* f){phase(5,f);}
#include "dispatch_body.h"
#include "destination_bodies.h"
static void reset(unsigned at,unsigned mode){protect(4);memset(pages,0,65536);object=pages;fp=(Fighter*)((char*)pages+4096);object->user_data=fp;fp->gobj=object;fp->x8_spawnNum=generation=0x80000001;fp->kind=Ft_Kind_Mario;fp->ft_data=(void*)1;selected=0;live=true;phase_at=at;mutation=mode;count=0;fp->cpu.x7C=19;}
int main(void)
{
 pages=VirtualAlloc(NULL,65536,0x3000,4);CHECK(pages);
 for(unsigned at=0;at<=5;++at)for(unsigned mode=0;mode<=6;++mode){reset(at,mode);ftCo_800B3900(object);protect(4);CHECK(count==(!at||!mode?5:at));CHECK(fp->cpu.x7C==(!at||!mode?20:19));}
 for(int previous=-2;previous<8;++previous)for(int last=-2;last<8;++last)for(int behavior=-2;behavior<22;++behavior)for(int latch=0;latch<3;++latch){Fighter stock={0},candidate={0};stock.cpu.x18=candidate.cpu.x18=behavior;stock.cpu.x1C=candidate.cpu.x1C=previous;stock.cpu.x20=candidate.cpu.x20=last;stock.cpu.x60=candidate.cpu.x60=latch;stock.cpu.xFA_b2=candidate.cpu.xFA_b2=true;CHECK(stock_destination_update(&stock)==ftCo_CpuPrepareDestinationUpdate(&candidate));CHECK(!memcmp(&stock.cpu,&candidate.cpu,sizeof(stock.cpu)));}
 printf("PASS %u actual dispatcher/destination checks; five phase internals controlled\n",checks);return 0;
}
