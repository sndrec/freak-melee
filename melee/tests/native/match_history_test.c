#include <melee/gm/native_match_history.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static NativeMatchHistory history,unchanged;
static NativeMatchArena arena[2];
static _Alignas(64) uint8_t source[2][512];
static _Alignas(8) uint8_t backing[2048*NativeMatchHistory_Capacity];
static struct {uint64_t value;uint32_t count;} root;
static uint64_t entries[8];
int main(void)
{
    NativeMatchArena* owners[]={&arena[0],&arena[1]};
    CHECK(nativeMatchArena_Begin(owners[0],source[0],sizeof(source[0]),17));
    CHECK(nativeMatchArena_Begin(owners[1],source[1],sizeof(source[1]),31));
    CHECK(nativeMatchArena_AddRoot(owners[0],&root,sizeof(root)));
    CHECK(nativeMatchArena_AddCountedRoot(owners[0],entries,sizeof(*entries),8,&root.count));
    uint64_t* second=nativeMatchArena_Allocate(owners[1],sizeof(*second),8);CHECK(second);
    size_t slot_bytes=nativeMatchHistory_SlotBytes(owners,2);CHECK(slot_bytes>1024);
    memset(&history,0xA5,sizeof(history));unchanged=history;
    CHECK(!nativeMatchHistory_Bind(&history,owners,2,backing,sizeof(backing),0,17));
    CHECK(!nativeMatchHistory_Bind(&history,owners,2,backing,sizeof(backing),17,17));
    CHECK(!nativeMatchHistory_Bind(&history,owners,2,backing+1,sizeof(backing)-1,16,17));
    CHECK(!nativeMatchHistory_Bind(&history,owners,2,backing,sizeof(backing),16,0));
    CHECK(!nativeMatchHistory_Bind(&history,owners,2,&history,sizeof(history),16,17));
    CHECK(!nativeMatchHistory_Bind(&history,owners,2,(void*)(UINTPTR_MAX-7),16,2,17));
    CHECK(!nativeMatchHistory_Bind(&history,owners,2,source[0],sizeof(backing),16,17));
    CHECK(!memcmp(&history,&unchanged,sizeof(history)));
    CHECK(nativeMatchHistory_Bind(&history,owners,2,backing,sizeof(backing),16,17));
    for(uint64_t frame=100;frame<140;++frame) {
        root.value=frame*3;*second=frame*5;root.count=2;entries[1]=frame;
        CHECK(nativeMatchHistory_Save(&history,frame,17));
        CHECK(nativeMatchHistory_Find(&history,frame));
        if(frame>=116)CHECK(!nativeMatchHistory_Find(&history,frame-16));
    }
    CHECK(!nativeMatchHistory_Save(&history,141,17));
    CHECK(!nativeMatchHistory_Save(&history,140,18));
    CHECK(!nativeMatchHistory_Save(&history,UINT64_MAX,17));
    root.value=999;*second=998;
    CHECK(!nativeMatchHistory_Load(&history,123,17));
    CHECK(!nativeMatchHistory_Load(&history,124,18));
    CHECK(root.value==999&&*second==998);
    CHECK(nativeMatchHistory_Load(&history,124,17));
    CHECK(root.value==372&&*second==620&&entries[1]==124);
    for(uint64_t frame=125;frame<140;++frame)CHECK(!nativeMatchHistory_Find(&history,frame));
    for(uint64_t frame=125;frame<140;++frame) {
        root.value=frame*7;*second=frame*11;CHECK(nativeMatchHistory_Save(&history,frame,17));
    }
    CHECK(nativeMatchHistory_Load(&history,130,17)&&root.value==910&&*second==1430);
    CHECK(nativeMatchHistory_Load(&history,125,17)&&root.value==875&&*second==1375);
    // Copy history rewinds actual allocation cursor and a counted satellite.
    void* future=nativeMatchArena_Allocate(owners[0],24,16);CHECK(future);
    root.count=8;entries[7]=12345;
    CHECK(nativeMatchHistory_Load(&history,125,17)&&root.count==2);
    CHECK(nativeMatchArena_Allocate(owners[0],24,16)==future);
    nativeMatchHistory_Discard(&history);
    CHECK(!nativeMatchHistory_Find(&history,125)&&!history.has_frames);
    CHECK(nativeMatchHistory_Save(&history,400,17));
    // A retired secondary arena refuses before either span is copied.
    root.value=444;*second=555;arena[1].lifetime=32;
    CHECK(!nativeMatchHistory_Load(&history,400,17));
    CHECK(root.value==444&&*second==555&&!nativeMatchHistory_Find(&history,400));
    CHECK(nativeMatchHistory_Bind(&history,owners,2,backing,sizeof(backing),16,18));
    CHECK(nativeMatchHistory_Save(&history,0,18));
    ((NativeMatchArenaCursor*)source[1])->used=513;
    CHECK(!nativeMatchHistory_Save(&history,1,18)&&history.failed);
    CHECK(!nativeMatchHistory_Find(&history,0));
    CHECK(!nativeMatchHistory_Load(&history,0,18));
    printf("copy-history real-arena checks=%u slot_bytes=%zu\n",checks,slot_bytes);
    return 0;
}
