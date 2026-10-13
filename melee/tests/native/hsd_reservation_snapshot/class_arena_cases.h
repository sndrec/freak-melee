#include <sysdolphin/baselib/native_hsd_arena.h>
static uint8_t class_arena_saved[NATIVE_HSD_ARENA_BYTES+3*1024*1024];
static NativeMatchArenaFrame class_arena_frame;
static size_t class_arena_prefix;
static void test_class_arena_history(void)
{
    HSD_ObjSetHeap(0,NULL);HSD_ObjAllocInit(&satellite,24,16);
    HSD_Class* a=hsdNew(&test_class);CHECK(a);
    memset((uint8_t*)a+sizeof(*a),0x52,96-sizeof(*a));
    NativeClassReference ref=nativeClass_Reference(a,96);CHECK(ref.bytes==96);
    CHECK(nativeHsdArena_AdmitHistory());
    CHECK(nativeMatchArena_BindFrame(&nativeHsdArena,&class_arena_frame,class_arena_saved,sizeof(class_arena_saved)));
    CHECK(nativeMatchArena_Save(&class_arena_frame));
    class_arena_prefix=class_arena_frame.bytes;
    u32 exist=test_class.head.nb_exist,peak=test_class.head.nb_peak;
    u32 object_used=satellite.used,object_free=satellite.free;
    long os_bytes=OSCheckHeap(HSD_GetHeap());
    void* added=hsdAllocMemPiece(16384);CHECK(added);
    CHECK(OSCheckHeap(HSD_GetHeap())==os_bytes); // No ordinary OS heap acquisition.
    HSD_Class* b=hsdNew(&test_class);CHECK(b);hsdDelete(a);
    CHECK(HSD_ObjAllocAddFree(&satellite,4)==4);
    void* object=HSD_ObjAlloc(&satellite);CHECK(object);
    NativeObjAllocReference object_ref=nativeObjAlloc_Reference(&satellite,object,satellite.size);
    CHECK(object_ref.epoch);memset(object,0x63,satellite.size);
    CHECK(nativeMatchArena_Load(&class_arena_frame));
    CHECK(a->class_info==&test_class&&((uint8_t*)a)[95]==0x52);
    CHECK(nativeClass_Resolve(ref)==a&&test_class.head.nb_exist==exist&&test_class.head.nb_peak==peak);
    CHECK(nativeMatchArena_Used(&nativeHsdArena)==class_arena_prefix);
    CHECK(satellite.used==object_used&&satellite.free==object_free&&!nativeObjAlloc_Resolve(object_ref));
    CHECK(hsdAllocMemPiece(16384)==added&&hsdNew(&test_class)==b);
    CHECK(HSD_ObjAllocAddFree(&satellite,4)==4&&HSD_ObjAlloc(&satellite)==object);
    CHECK(nativeObjAlloc_Resolve(object_ref)==object);
    CHECK(nativeMatchArena_Load(&class_arena_frame));
    // A scene history transition can preserve live heap storage yet retire all
    // prior private frames. A heap reset also uses a distinct generation.
    CHECK(nativeHsdArena_AdmitHistory());
    CHECK(!nativeMatchArena_Load(&class_arena_frame));
    CHECK(nativeClass_Resolve(ref)==a);
    void* survivor=HSD_ObjAlloc(&satellite);CHECK(survivor);
    NativeObjAllocReference survivor_ref=nativeObjAlloc_Reference(&satellite,survivor,satellite.size);
    CHECK(survivor_ref.epoch);
    size_t retained=nativeMatchArena_Used(&nativeHsdArena);
    hsdForgetClassLibrary(NULL);
    CHECK(!nativeMatchArena_Load(&class_arena_frame)&&!nativeClass_Resolve(ref));
    CHECK(nativeMatchArena_Used(&nativeHsdArena)==retained&&nativeObjAlloc_Resolve(survivor_ref)==survivor);
    _HSD_ObjAllocForgetMemory(NULL,NULL);CHECK(nativeHsdArena_BeginHeap());
    CHECK(nativeMatchArena_Used(&nativeHsdArena)==64&&!nativeObjAlloc_Resolve(survivor_ref));
}
