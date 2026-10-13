#include <melee/gm/native_match_arena.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static _Alignas(64) uint8_t backing[4096];
static uint8_t history[8192], other_history[8192];
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"%u: %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct Node { struct Node* next; uint32_t value; } Node;
int main(void)
{
    NativeMatchArena arena={0},old;
    NativeMatchArenaFrame frame={0},other={0};
    uint32_t roots[NATIVE_MATCH_ARENA_ROOTS+8]={0};
    CHECK(!nativeMatchArena_Begin(NULL,backing,sizeof(backing),1));
    CHECK(!nativeMatchArena_Begin(&arena,backing+1,sizeof(backing)-1,1));
    CHECK(!nativeMatchArena_Begin(&arena,backing,sizeof(backing),0));
    CHECK(!nativeMatchArena_Begin((NativeMatchArena*)backing,backing,sizeof(backing),1));
    CHECK(nativeMatchArena_Begin(&arena,backing,sizeof(backing),1));
    CHECK(!nativeMatchArena_AddRoot(&arena,backing,8));
    CHECK(!nativeMatchArena_AddRoot(&arena,&arena,8));
    CHECK(nativeMatchArena_AddRoot(&arena,roots,sizeof(roots[0])));
    CHECK(!nativeMatchArena_AddRoot(&arena,roots,8));
    old=arena;CHECK(!nativeMatchArena_AddRoot(&arena,(void*)(UINTPTR_MAX-3),8));
    CHECK(!memcmp(&old,&arena,sizeof(arena)));
    CHECK(!nativeMatchArena_BindFrame(&arena,&frame,history,sizeof(backing)-1));
    CHECK(!nativeMatchArena_BindFrame(&arena,&frame,backing,sizeof(backing)));
    CHECK(!nativeMatchArena_BindFrame(&arena,(NativeMatchArenaFrame*)backing,history,sizeof(history)));
    CHECK(!nativeMatchArena_BindFrame(&arena,&frame,&frame,sizeof(history)));
    CHECK(nativeMatchArena_BindFrame(&arena,&frame,history,sizeof(history)));
    CHECK(nativeMatchArena_BindFrame(&arena,&other,other_history,sizeof(other_history)));
    CHECK(!nativeMatchArena_AddRoot(&arena,&roots[1],sizeof(roots[1])));
    CHECK(!nativeMatchArena_Load(&frame));
    Node* a=nativeMatchArena_Allocate(&arena,sizeof(*a),_Alignof(Node));
    Node* b=nativeMatchArena_Allocate(&arena,sizeof(*b),_Alignof(Node));
    CHECK(a&&b);a->next=b;a->value=10;b->next=a;b->value=20;roots[0]=31;
    CHECK(nativeMatchArena_Save(&frame));
    size_t early=nativeMatchArena_Used(&arena);
    Node* c=nativeMatchArena_Allocate(&arena,sizeof(*c),64);
    CHECK(c&&((uintptr_t)c%64)==0);a->next=c;a->value=100;c->next=b;c->value=30;roots[0]=41;
    CHECK(nativeMatchArena_Save(&other));
    size_t later=nativeMatchArena_Used(&arena);
    CHECK(later>early&&frame.bytes==early&&other.bytes==later);
    CHECK(nativeMatchArena_Load(&frame));
    CHECK(a->next==b&&b->next==a&&a->value==10&&b->value==20&&roots[0]==31);
    CHECK(nativeMatchArena_Used(&arena)==early);
    CHECK(nativeMatchArena_Allocate(&arena,sizeof(*c),64)==c);
    CHECK(c->next==NULL&&c->value==0);
    CHECK(nativeMatchArena_Load(&other));
    CHECK(a->next==c&&a->value==100&&c->next==b&&c->value==30&&roots[0]==41);
    size_t before=nativeMatchArena_Used(&arena);
    CHECK(!nativeMatchArena_Allocate(&arena,sizeof(backing),16));
    CHECK(!nativeMatchArena_Allocate(&arena,1,(size_t)1 << (sizeof(size_t)*8-1)));
    CHECK(!nativeMatchArena_Allocate(&arena,1,3));
    CHECK(nativeMatchArena_Used(&arena)==before);
    // Scene changes invalidate private history regardless of address reuse.
    CHECK(nativeMatchArena_Begin(&arena,backing,sizeof(backing),2));
    CHECK(!nativeMatchArena_Load(&frame)&&!nativeMatchArena_Save(&frame));
    CHECK(nativeMatchArena_Used(&arena)==64);
    for(unsigned i=0;i<NATIVE_MATCH_ARENA_ROOTS;++i)
        CHECK(nativeMatchArena_AddRoot(&arena,&roots[i],sizeof(roots[i])));
    CHECK(!nativeMatchArena_AddRoot(&arena,&roots[NATIVE_MATCH_ARENA_ROOTS],sizeof(roots[0])));
    CHECK(nativeMatchArena_Begin(&arena,backing,sizeof(backing),3));
    uint32_t entries[16]={0},entry_count=0;
    CHECK(!nativeMatchArena_AddCountedRoot(&arena,entries,sizeof(*entries),16,&entry_count));
    CHECK(nativeMatchArena_AddRoot(&arena,&entry_count,sizeof(entry_count)));
    CHECK(!nativeMatchArena_AddCountedRoot(&arena,entries,SIZE_MAX,16,&entry_count));
    CHECK(nativeMatchArena_AddCountedRoot(&arena,entries,sizeof(*entries),16,&entry_count));
    CHECK(nativeMatchArena_BindFrame(&arena,&frame,history,sizeof(history)));
    entries[0]=11;entries[1]=22;entry_count=2;CHECK(nativeMatchArena_Save(&frame));
    CHECK(frame.root_bytes[1]==2*sizeof(*entries));
    entries[2]=33;entry_count=3;CHECK(nativeMatchArena_Save(&other)==false); // Old admission.
    CHECK(nativeMatchArena_BindFrame(&arena,&other,other_history,sizeof(other_history)));
    CHECK(nativeMatchArena_Save(&other));
    entries[0]=99;entries[2]=77;entry_count=4;
    CHECK(nativeMatchArena_Load(&frame)&&entry_count==2&&entries[0]==11&&entries[1]==22);
    CHECK(nativeMatchArena_Load(&other)&&entry_count==3&&entries[2]==33);
    void* wide=nativeMatchArena_Allocate(&arena,7,256);
    CHECK(wide&&((uintptr_t)wide%256)==0);
    printf("{\"checks\":%u,\"copy_only_history\":true}\n",checks);
    return 0;
}
