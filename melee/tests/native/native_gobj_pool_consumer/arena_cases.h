// Actual GObj/procedure/scheduler state, restored without packed records or
// topology walks. Test bookkeeping/history is external to mutable game state.
static uint8_t arena_saved[64*1024*1024];
static NativeMatchArenaFrame arena_frame;
static size_t arena_copy_bytes;
static void contiguous_scheduler_history(void)
{
    initialize();
    NativeMatchArena* arena=&nativeMatchSceneArena;
    CHECK(nativeMatchArena_BindFrame(arena,&arena_frame,arena_saved,sizeof(arena_saved)));
    CHECK(!nativeMatchArena_AddRoot(arena,&scheduler_mask,sizeof(scheduler_mask)));
    HSD_GObj* a=GObj_Create(11,1,0),*b=GObj_Create(22,1,1);
    HSD_GObjProc* ap=HSD_GObj_SetupProc(a,tick,0);
    HSD_GObjProc* bp=HSD_GObj_SetupProc(b,tick,0);
    GObj_SetupGXLinkMax(a,draw,0);
    CHECK(a&&b&&ap&&bp);
    NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,a,&ref));
    HSD_GObj_804085F0[0]=19;
    CHECK(nativeMatchArena_Save(&arena_frame));arena_copy_bytes=arena_frame.bytes;
    for(unsigned i=0;i<arena->root_count;++i)arena_copy_bytes+=arena_frame.root_bytes[i];
    size_t saved_used=nativeMatchArena_Used(arena);
    CHECK(arena_copy_bytes<400000);
    uint8_t* future=nativeMatchArena_Allocate(arena,137,32);
    CHECK(future&&((uintptr_t)future%32)==0);memset(future,0xA5,137);
    HSD_GObjFree(a);HSD_GObj* next=GObj_Create(33,1,0);
    CHECK(next==a&&HSD_GObj_SetupProc(next,tick,0)==ap);
    HSD_GObj_804085F0[0]=23;
    CHECK(nativeMatchArena_Load(&arena_frame));
    CHECK(nativeMatchArena_Used(arena)==saved_used&&HSD_GObj_804085F0[0]==19);
    CHECK(nativeMatchPool_Resolve(ref)==a&&a->classifier==11&&a->next==b&&a->proc==ap);
    CHECK(HSD_GObjPLinkHead[1]==a&&HSD_GObj_ProcList[1]==bp);
    event_count=0;HSD_GObj_80390CFC();
    CHECK(event_count==2&&events[0]==11&&events[1]==22);
    // No tail clearing on load: subsequent allocations initialize reclaimed
    // bytes and consume exactly the address/alignment of the fresh reference.
    CHECK(nativeMatchArena_Allocate(arena,137,32)==future);
    for(unsigned i=0;i<137;++i)CHECK(future[i]==0);
    HSD_GObjFree(a);next=GObj_Create(33,1,0);
    CHECK(next==a&&HSD_GObj_SetupProc(next,tick,0)==ap);
    CHECK(nativeMatchArena_Load(&arena_frame));
    CHECK(nativeMatchPool_Resolve(ref)==a&&nativeGObjPool.used==2);
    CHECK(!nativeMatchArena_Allocate(arena,arena->capacity,32));
    CHECK(nativeMatchArena_Used(arena)==saved_used);
    CHECK(!nativeMatchArena_Allocate(arena,1,3));
    CHECK(nativeGObjPool_RetireScene());
    CHECK(!nativeMatchArena_Load(&arena_frame)&&!nativeMatchArena_Save(&arena_frame));
    initialize();CHECK(!nativeMatchArena_Load(&arena_frame));
    ++cases;
}

// The same history restores real typed payload windows and their allocator
// metadata. A slot reached only in the discarded future must be reinitialized.
static void contiguous_actor_history(void)
{
    initialize();
    NativeMatchPool* pools[]={&nativeFighterPool,&nativeGroundPool,&nativeItemPool,&nativeItemLinkPool};
    NativeMatchPoolReference initial[4],future[4];
    void *first[4],*next[4];
    for(unsigned i=0;i<4;++i){
        first[i]=nativeMatchPool_Allocate(pools[i],&initial[i]);CHECK(first[i]);
        memset(first[i],0x10+i,pools[i]->object_size);
        CHECK(pools[i]->initialized_count==1);
    }
    CHECK(nativeMatchArena_BindFrame(&nativeMatchSceneArena,&arena_frame,arena_saved,sizeof(arena_saved)));
    CHECK(nativeMatchArena_Save(&arena_frame));
    size_t copied=arena_frame.bytes;
    for(unsigned i=0;i<nativeMatchSceneArena.root_count;++i)copied+=arena_frame.root_bytes[i];
    CHECK(copied<nativeMatchArena_Used(&nativeMatchSceneArena)+1024*1024);
    for(unsigned i=0;i<4;++i){
        next[i]=nativeMatchPool_Allocate(pools[i],&future[i]);CHECK(next[i]);
        memset(next[i],0xFE,pools[i]->object_size);memset(first[i],0xFF,pools[i]->object_size);
        CHECK(pools[i]->initialized_count==2);CHECK(nativeMatchPool_Free(pools[i],initial[i]));
    }
    for(unsigned round=0;round<2;++round){
        CHECK(nativeMatchArena_Load(&arena_frame));
        for(unsigned i=0;i<4;++i){
            CHECK(pools[i]->initialized_count==1&&pools[i]->used==1);
            CHECK(nativeMatchPool_Resolve(initial[i])==first[i]&&!nativeMatchPool_Resolve(future[i]));
            for(size_t byte=0;byte<pools[i]->object_size;++byte)CHECK(((uint8_t*)first[i])[byte]==0x10+i);
            NativeMatchPoolReference replay;
            CHECK(nativeMatchPool_Allocate(pools[i],&replay)==next[i]);
            CHECK(replay.slot==future[i].slot&&replay.generation==future[i].generation);
            for(size_t byte=0;byte<pools[i]->object_size;++byte)CHECK(((uint8_t*)next[i])[byte]==0);
            memset(next[i],0xFE,pools[i]->object_size);
        }
    }
    ++cases;
}
