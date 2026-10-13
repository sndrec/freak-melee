static void test_scene_history_renewal(void)
{
    CHECK(!nativeMatchSceneArena.lifetime && nativeMatchScene_Begin());
    NativeMatchArena* arena=&nativeMatchSceneArena;
    uint64_t* payload=nativeMatchArena_Allocate(arena,sizeof(*payload),_Alignof(uint64_t));
    CHECK(payload);*payload=7;
    uint32_t control=3;
    CHECK(nativeMatchArena_AddRoot(arena,&control,sizeof(control)));
    size_t used=nativeMatchArena_Used(arena),bytes=arena->capacity+arena->root_bytes;
    void* backing=malloc(bytes);CHECK(backing);
    NativeMatchArenaFrame old,next;
    CHECK(nativeMatchArena_BindFrame(arena,&old,backing,bytes) && nativeMatchArena_Save(&old));
    uint64_t epoch=arena->lifetime;
    *payload=9;control=11;
    CHECK(nativeMatchScene_ResetHistory());
    CHECK(arena->lifetime>epoch && !arena->sealed && !arena->root_count && !arena->root_bytes);
    CHECK(nativeMatchArena_Used(arena)==used && *payload==9 && control==11);
    CHECK(!nativeMatchArena_Load(&old) && *payload==9 && control==11);
    CHECK(nativeMatchArena_AddRoot(arena,&control,sizeof(control)));
    CHECK(nativeMatchArena_BindFrame(arena,&next,backing,bytes) && nativeMatchArena_Save(&next));
    *payload=13;control=17;
    CHECK(!nativeMatchArena_Load(&old) && *payload==13 && control==17);
    CHECK(nativeMatchArena_Load(&next) && *payload==9 && control==11);
    epoch=arena->lifetime;
    nativeMatchScene_Retire();
    CHECK(nativeMatchScene_Begin() && arena->lifetime>epoch);
    CHECK(!nativeMatchArena_Load(&next));
    nativeMatchScene_Retire();free(backing);
}
