#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/class.h>
#include <melee/lb/lbrefract.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <sysdolphin/baselib/objalloc.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned calls;
static HSD_PObjInfo ordered;
static s32 ordered_load(HSD_PObj* polygon, HSD_PObjDesc* desc)
{
    if ((uintptr_t)desc->display != 10000-calls) exit(1);
    if (polygon->next && polygon->next->display != (void*)((uintptr_t)desc->display+1)) exit(1);
    ++calls;
    return hsdPObj.load(polygon,desc);
}
static void ordered_init(void)
{
    hsdInitClassInfo((HSD_ClassInfo*)&ordered,(HSD_ClassInfo*)&hsdPObj,
        "test","ordered_polygon",sizeof(ordered),sizeof(HSD_PObj));
    ordered.load=ordered_load;
}

int main(void)
{
    HSD_Obj object={0};
    for(unsigned i=0;i<250000;++i) {ref_INC(&object);iref_INC(&object);}
    if(ref_CNT(&object)!=250000 || iref_CNT(&object)!=250000)return 1;
    for(unsigned i=0;i<250000;++i) {
        if(ref_DEC(&object) || iref_DEC(&object)!=(i==249999))return 1;
    }
    if(!ref_DEC(&object) || ref_CNT(&object)!=-1 || !ref_DEC(&object))return 1;
    enum { count = 10000 };
    HSD_PObjDesc* descriptors = calloc(count, sizeof(*descriptors));
    if (!descriptors) return 1;
    ClassInfoInit((HSD_ClassInfo*)&lbRefractPObjInfo);
    ordered.parent.head.info_init=ordered_init;
    ClassInfoInit((HSD_ClassInfo*)&ordered);
    HSD_PObjInfo* classes[]={&hsdPObj,&lbRefractPObjInfo,&ordered};
    for (unsigned pass = 0; pass < 3; ++pass) {
        HSD_PObjSetDefaultClass(classes[pass]);
        for (unsigned i = 0; i < count; ++i) {
            descriptors[i].next = i+1 < count ? &descriptors[i+1] : NULL;
            descriptors[i].flags = POBJ_SKIN;
            descriptors[i].display = (void*)(uintptr_t)(i+1);
        }
        HSD_PObj* head = HSD_PObjLoadDesc(descriptors);
        unsigned seen = 0;
        for (HSD_PObj* polygon = head; polygon; polygon = polygon->next) {
            if (seen == count || polygon->flags != POBJ_SKIN ||
                polygon->display != descriptors[seen].display ||
                HSD_POBJ_METHOD(polygon) != classes[pass]) return 1;
            ++seen;
        }
        if (seen != count) return 1;
        HSD_PObjRemoveAll(head);
    }
    HSD_PObjSetDefaultClass(NULL);
    free(descriptors);
    if (calls != count) return 1;
    enum { large_count=40000 };
    HSD_PObjDesc* large=calloc(large_count,sizeof(*large));
    if(!large || nativeHsdArena.capacity!=NATIVE_HSD_ARENA_BYTES)return 1;
    for(unsigned i=0;i<large_count;++i) {
        large[i].next=i+1<large_count?large+i+1:NULL;
        large[i].flags=POBJ_SKIN;large[i].display=(void*)(uintptr_t)(i+1);
    }
    void* base=nativeHsdArena.memory;
    HSD_PObj* first=HSD_PObjLoadDesc(large);
    HSD_PObj* second=HSD_PObjLoadDesc(large);
    if(nativeHsdArena.memory!=base || nativeHsdArena.capacity<=NATIVE_HSD_ARENA_BYTES ||
       nativeHsdArena.capacity>NATIVE_HSD_ARENA_MAX_BYTES ||
       nativeMatchArena_Used(&nativeHsdArena)<=NATIVE_HSD_ARENA_BYTES)return 1;
    for(unsigned list=0;list<2;++list) {
        unsigned seen=0;
        for(HSD_PObj* p=list?second:first;p;p=p->next)
            if(seen==large_count || p->display!=large[seen++].display)return 1;
        if(seen!=large_count)return 1;
    }
    HSD_ListInitAllocData();
    HSD_SList* lists=NULL;
    for(unsigned i=0;i<250000;++i) {
        HSD_SList* node=HSD_SListAlloc();
        node->data=(void*)(uintptr_t)(i+1);node->next=lists;lists=node;
    }
    if(nativeHsdArena.memory!=base || nativeHsdArena.capacity<=8*1024*1024)return 1;
    if(!nativeHsdArena_AdmitHistory())return 1;
    size_t capacity=nativeHsdArena.capacity,used=nativeMatchArena_Used(&nativeHsdArena);
    NativeMatchArenaFrame frame;
    size_t bytes=capacity+nativeHsdArena.root_bytes;
    void* memory=malloc(bytes);
    if(!memory || !nativeMatchArena_BindFrame(&nativeHsdArena,&frame,memory,bytes) ||
       !nativeMatchArena_Save(&frame) || frame.bytes!=used || frame.total_bytes>=bytes)return 1;
    if(nativeHsdArena_Ensure(capacity,32) || nativeHsdArena.capacity!=capacity)return 1;
    first->flags=POBJ_ENVELOPE;
    lists->data=(void*)(uintptr_t)17;
    if(!nativeMatchArena_Load(&frame) || first->flags!=POBJ_SKIN || nativeHsdArena.memory!=base ||
       lists->data!=(void*)(uintptr_t)250000)return 1;
    while(lists)lists=HSD_SListRemove(lists);
    HSD_PObjRemoveAll(first);HSD_PObjRemoveAll(second);
    free(memory);free(large);
    printf("arena used=%zu capacity=%zu reserved=%u; history restores the initialized prefix\n",used,capacity,NATIVE_HSD_ARENA_MAX_BYTES);
    _HSD_ObjAllocForgetMemory(NULL,NULL);hsdForgetClassLibrary(NULL);
    if(!nativeHsdArena_BeginHeap() || nativeHsdArena.memory!=base ||
       nativeHsdArena.capacity!=NATIVE_HSD_ARENA_BYTES || nativeMatchArena_Used(&nativeHsdArena)!=64)return 1;
    puts("PASS: three 10000-polygon chains retain class, reverse callback order, initialized tails and payload through construction/removal");
    return 0;
}
