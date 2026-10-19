#include "class.h"

#include <string.h>

#include "debug.h"
#include "hash.h"
#include "memory.h"
#include "object.h" // IWYU pragma: keep
#ifdef MELEE_NATIVE
#include "native_hsd_reservation.h"
#include "native_class_snapshot.h"
#include "native_hsd_arena.h"
#include "native_objalloc_snapshot.h"
#endif
#include <dolphin/os.h>

void _hsdClassInfoInit(void);
HSD_ClassInfo hsdClass = { _hsdClassInfoInit };

static HSD_MemoryEntry** memory_list;
static s32 nb_memory_list;
static HSD_Hash* current_hash;

#ifdef MELEE_NATIVE
// These identify actual retained arena chunks, not copies of model payloads.
// Recording a newly acquired chunk is O(1); ordinary piece reuse is unchanged.
static NativeClassSnapshotBlock class_blocks[NATIVE_CLASS_SNAPSHOT_BLOCKS];
static uint32_t class_block_count;
static uint64_t class_epoch;

static void record_class_block(void* address, size_t bytes)
{
    if (address) {
        HSD_ASSERT(0, class_block_count < NATIVE_CLASS_SNAPSHOT_BLOCKS);
        class_blocks[class_block_count++] =
            (NativeClassSnapshotBlock){address, bytes};
    }
}
#endif

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
void ClassInfoInit(HSD_ClassInfo* info)
{
    if ((info->head.flags & 1) == 0) {
        (*info->head.info_init)();
    }
}
#ifdef MUST_MATCH
#pragma pop
#endif

void hsdInitClassInfo(HSD_ClassInfo* class_info, HSD_ClassInfo* parent_info,
                      char* base_class_library, char* type, s32 info_size,
                      s32 class_size)
{
#ifdef MELEE_NATIVE
    HSD_ASSERTREPORT(0,!nativeHsdArena.sealed,
        "Class %s must be initialized before admitting match history.\n",type);
#endif
    class_info->head.flags = 1;
    class_info->head.library_name = base_class_library;
    class_info->head.class_name = type;
    class_info->head.obj_size = (s16) class_size;
    class_info->head.info_size = (s16) info_size;
    class_info->head.parent = parent_info;
    class_info->head.child = NULL;
    class_info->head.next = NULL;
    class_info->head.nb_exist = 0;
    class_info->head.nb_peak = 0;

    if (parent_info != NULL) {
        if ((parent_info->head.flags & 1) == 0) {
            (*parent_info->head.info_init)();
        }
        HSD_ASSERT(94, class_info->head.obj_size >= parent_info->head.obj_size);
        HSD_ASSERT(95, class_info->head.info_size >= parent_info->head.info_size);
        memcpy(&class_info->alloc, &parent_info->alloc,
               parent_info->head.info_size - sizeof(HSD_ClassInfoHead));
        class_info->head.next = parent_info->head.child;
        parent_info->head.child = class_info;
    }
}

void OSReport_PrintSpaces(s32 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        OSReport(" ");
    }
}

#ifdef MUST_MATCH
#pragma push
#pragma force_active on
static char unused1[] = "entry %d <null>\n";
static char unused2[] = "entry %d - %d <null>\n";
static char unused3[] = "entry %d(%d)";
static char unused4[] = "  nb_alloc %d nb_free %d\n";
#pragma pop
#endif

HSD_MemoryEntry* GetMemoryEntry(s32 idx)
{
    HSD_ASSERT(171, idx >= 0);
#ifdef MELEE_NATIVE
    // Raw piece users also initialize this owner during construction. Normal
    // hsdNew already initializes the root class before allocating any payload.
    if(!nativeHsdArena.lifetime)ClassInfoInit(&hsdClass);
#endif

    if (idx >= nb_memory_list) {
        if (nb_memory_list ==
            0) { // In this case, it's uninitialized and allocs the array
            s32 new_nb;

            for (new_nb = 32; idx >= new_nb; new_nb *= 2) {
            }
#ifdef MELEE_NATIVE
            HSD_ASSERT(0, class_block_count < NATIVE_CLASS_SNAPSHOT_BLOCKS);
#endif
#ifdef MELEE_NATIVE
            if(!nativeHsdArena_Ensure(new_nb*sizeof(*memory_list),32))return NULL;
            memory_list = nativeMatchArena_Allocate(&nativeHsdArena,
                new_nb * sizeof(*memory_list),32);
#else
            memory_list = (HSD_MemoryEntry**) HSD_MemAlloc(new_nb * sizeof(*memory_list));
#endif
#ifdef MELEE_NATIVE
            record_class_block(memory_list, new_nb * sizeof(*memory_list));
#endif
            if (memory_list == NULL) {
                return NULL;
            }
            memset(memory_list, 0, new_nb * sizeof(*memory_list));
            nb_memory_list = new_nb;
        } else { // Resizes the array
            HSD_MemoryEntry** old_list;
            HSD_MemoryEntry** new_list;
            s32 old_nb, new_nb;

            new_nb = nb_memory_list * 2;
            while (idx >= new_nb) {
                new_nb *= 2;
            }

#ifdef MELEE_NATIVE
            HSD_ASSERT(0, class_block_count < NATIVE_CLASS_SNAPSHOT_BLOCKS);
#endif
#ifdef MELEE_NATIVE
            if(!nativeHsdArena_Ensure(sizeof(*memory_list)*new_nb,32))return NULL;
            new_list = nativeMatchArena_Allocate(&nativeHsdArena,sizeof(*memory_list)*new_nb,32);
#else
            new_list = HSD_MemAlloc(sizeof(*memory_list) * new_nb);
#endif
#ifdef MELEE_NATIVE
            record_class_block(new_list, sizeof(*memory_list) * new_nb);
#endif
            if (new_list == NULL) {
                return NULL;
            }

            memcpy(new_list, memory_list,
                   sizeof(*memory_list) * nb_memory_list);
            memset(&new_list[nb_memory_list], 0,
                   sizeof(*new_list) * (new_nb - nb_memory_list));

            old_list = memory_list;
            old_nb = OSRoundDown32B(nb_memory_list * sizeof(*memory_list));
            memory_list = new_list;
            nb_memory_list = new_nb;

            hsdFreeMemPiece(old_list, old_nb);
            memory_list[OSRoundUp32B(old_nb) / 32 - 1]->nb_alloc += 1;
        }
    }

    {
        ssize_t i;
        bool found;
        HSD_MemoryEntry* entry;
        if (memory_list[idx] == NULL) {
#ifdef MELEE_NATIVE
            HSD_ASSERT(0, class_block_count < NATIVE_CLASS_SNAPSHOT_BLOCKS);
#endif
#ifdef MELEE_NATIVE
            if(!nativeHsdArena_Ensure(sizeof(HSD_MemoryEntry),32))return NULL;
            entry = nativeMatchArena_Allocate(&nativeHsdArena,sizeof(HSD_MemoryEntry),32);
#else
            entry = HSD_MemAlloc(sizeof(HSD_MemoryEntry));
#endif
#ifdef MELEE_NATIVE
            record_class_block(entry, sizeof(HSD_MemoryEntry));
#endif
            if (entry == NULL) {
                return NULL;
            }
            memset(entry, 0, sizeof(HSD_MemoryEntry));
            entry->size = (idx + 1) * 32;
            memory_list[idx] = entry;

            found = false;
            for (i = idx - 1; i >= 0; --i) {
                if (memory_list[i] != NULL) {
                    found = true;
                    entry->next = memory_list[i]->next;
                    memory_list[i]->next = entry;
                    break;
                }
            }
            if (found == false) {
                for (i = idx + 1; i < nb_memory_list; i++) {
                    if (memory_list[i] != NULL) {
                        entry->next = memory_list[i];
                        break;
                    }
                }
            }
        }
        return memory_list[idx];
    }
}

void* hsdAllocMemPiece(s32 size)
{
#ifdef MELEE_NATIVE
    bool reserved;
    void* allocation = nativeHsdReservation_Allocate(NULL, (u32)size, &reserved);
    if (reserved) return allocation;
#endif
    HSD_FreeList* temp_r3_2;
    HSD_FreeList* temp_r4;
    HSD_FreeList* temp_r4_2;
    HSD_FreeList* temp_r5;
    HSD_MemoryEntry* temp_r3_3;
    HSD_MemoryEntry* temp_r3_4;
    HSD_MemoryEntry* var_r28;
    HSD_MemoryEntry* var_r30;
    s32 temp_r28;
    s32 temp_r29;
    void* temp_r3;

    temp_r29 = (size + 0x1F) / 32 - 1;
    temp_r3_3 = GetMemoryEntry((size + 0x1F) / 32 - 1);
    if (temp_r3_3 == NULL) {
        return NULL;
    }
    if ((temp_r3_2 = temp_r3_3->free_list)) {
        temp_r3_3->free_list = temp_r3_2->next;
        temp_r3_3->nb_free -= 1;
        return temp_r3_2;
    }
    var_r28 = temp_r3_3->next;
    while (var_r28 != NULL) {
        if (var_r28->free_list != NULL) {
            temp_r3_4 = GetMemoryEntry(
                (s32) (var_r28->size - temp_r3_3->size + 0x1F) / 32 - 1);
            if (temp_r3_4 == NULL) {
                return NULL;
            }
            temp_r5 = var_r28->free_list;
            var_r28->free_list = var_r28->free_list->next;
            var_r28->nb_free -= 1;
            var_r28->nb_alloc -= 1;
            temp_r4_2 = (void*) ((char*) temp_r5 + temp_r3_3->size);
            temp_r4_2->next = temp_r3_4->free_list;
            temp_r3_4->free_list = temp_r4_2;
            temp_r3_4->nb_alloc += 1;
            temp_r3_4->nb_free += 1;
            temp_r3_3->nb_alloc += 1;
            return temp_r5;
        }
        var_r28 = var_r28->next;
    }
    temp_r28 = (nb_memory_list - temp_r29) - 2;
    temp_r29 = temp_r28;
    if (temp_r29 >= 0) {
        var_r30 = GetMemoryEntry(temp_r29);
        if (var_r30 == NULL) {
            return NULL;
        }
    }
#ifdef MELEE_NATIVE
    HSD_ASSERT(0, class_block_count < NATIVE_CLASS_SNAPSHOT_BLOCKS);
#endif
#ifdef MELEE_NATIVE
    if(!nativeHsdArena_Ensure(nb_memory_list*32,32))return NULL;
    temp_r3 = nativeMatchArena_Allocate(&nativeHsdArena,nb_memory_list*32,32);
#else
    temp_r3 = HSD_MemAlloc(nb_memory_list * 32);
#endif
#ifdef MELEE_NATIVE
    record_class_block(temp_r3, nb_memory_list * 32);
#endif
    if (temp_r3 == NULL) {
        return NULL;
    }
    if (temp_r28 >= 0) {
        temp_r4 = (void*) ((char*) temp_r3 + temp_r3_3->size);
        temp_r4->next = var_r30->free_list;
        var_r30->free_list = temp_r4;
        var_r30->nb_alloc += 1;
        var_r30->nb_free += 1;
    }
    temp_r3_3->nb_alloc += 1;
    return temp_r3;
}

void hsdFreeMemPiece(void* mem, s32 size)
{
    HSD_MemoryEntry* entry;
    HSD_FreeList* piece = (HSD_FreeList*) mem;

    if (mem != NULL) {
#ifdef MELEE_NATIVE
        int reserved = nativeHsdReservation_Free(NULL, mem, (u32)size);
        HSD_ASSERT(0, reserved >= 0);
        if (reserved) return;
#endif
        entry = GetMemoryEntry((size + 31) / 32 - 1);
        piece->next = entry->free_list;
        entry->free_list = piece;
        entry->nb_free += 1;
    }
}

/// _hsdClassAlloc
HSD_Class* _hsdClassAlloc(HSD_ClassInfo* info)
{
    HSD_Class* mem_piece = hsdAllocMemPiece(info->head.obj_size);
    if (mem_piece != NULL) {
        info->head.nb_exist += 1;
        if (info->head.nb_exist > info->head.nb_peak) {
            info->head.nb_peak = info->head.nb_exist;
        }
    }
    return mem_piece;
}

int _hsdClassInit(HSD_Class* arg0)
{
    return 0;
}

void _hsdClassRelease(HSD_Class* cls) {}

void _hsdClassDestroy(HSD_Class* cls)
{
    HSD_ClassInfo* info = cls->class_info;
    info->head.nb_exist -= 1;
    hsdFreeMemPiece(cls, info->head.obj_size);
}

void _hsdClassAmnesia(HSD_ClassInfo* info)
{
#ifdef MELEE_NATIVE
    HSD_ASSERT(0, !nativeHsdReservation_Active());
    HSD_ASSERT(0, class_epoch != UINT64_MAX);
    ++class_epoch;
    if (info == &hsdClass) {
        class_block_count = 0;
        HSD_ASSERTREPORT(0,nativeHsdArena.lifetime || nativeHsdArena_BeginHeap(),"Cannot reserve class-object arena.\n");
        HSD_ASSERTREPORT(0,nativeHsdArena_ResetHistory(),"Cannot retire class history.\n");
    }
#endif
    info->head.nb_exist = 0;
    info->head.nb_peak = 0;
    if (info == &hsdClass) {
        nb_memory_list = 0;
        memory_list = NULL;
        current_hash = NULL;
    }
}

void _hsdClassInfoInit(void)
{
#ifdef MELEE_NATIVE
    if(!nativeHsdArena.lifetime)
        HSD_ASSERTREPORT(0,nativeHsdArena_BeginHeap(),"Cannot reserve class-object arena.\n");
#endif
    hsdInitClassInfo(&hsdClass, NULL, "sysdolphin_base_library", "hsd_class",
                     sizeof(HSD_ClassInfo), sizeof(HSD_Class));
    hsdClass.alloc = _hsdClassAlloc;
    hsdClass.init = _hsdClassInit;
    hsdClass.release = _hsdClassRelease;
    hsdClass.destroy = _hsdClassDestroy;
    hsdClass.amnesia = _hsdClassAmnesia;
}

void* hsdNew(HSD_ClassInfo* i)
{
    HSD_ClassInfo* info = i;
    HSD_ClassInfo* info2 = info;
    HSD_Class* cls;
    if (!(info2->head.flags & 1)) {
        info2->head.info_init();
    }
    cls = info->alloc(info2);
    if (cls == NULL) {
        return NULL;
    }
    ClassInfoInit(info);
    memset(cls, 0, info->head.obj_size);
    cls->class_info = info;
    if (info->init(cls) < 0) {
        info->destroy(cls);
        return NULL;
    }
    return cls;
}

static inline HSD_ClassInfo* HSD_GetClassInfo(HSD_Obj* object)
{
    return object->parent.class_info;
}

static inline HSD_ClassInfo* HSD_PushClassInfo(HSD_ClassInfo* class_info)
{
    HSD_ClassInfo* ret;
    return ret = class_info;
}

static inline bool hsdChangeClass_inline(HSD_Obj* object,
                                         HSD_ClassInfo* class_info)
{
    HSD_ClassInfo* var_r29;
    HSD_ClassInfo* var_r28;

    HSD_ASSERT(0x249, object);
    HSD_ASSERT(0x24A, class_info);
    var_r29 = HSD_GetClassInfo(object);
    !var_r29;
    var_r28 = HSD_PushClassInfo(class_info);
    if (!(var_r28->head.flags & 1)) {
        var_r28->head.info_init();
    }
    if (var_r29->head.obj_size != var_r28->head.obj_size) {
        return false;
    }
    while (var_r29->head.parent != NULL &&
           var_r29->head.parent->head.obj_size == var_r29->head.obj_size)
    {
        var_r29 = var_r29->head.parent;
    }
    while (var_r28->head.parent != NULL &&
           var_r28->head.parent->head.obj_size == var_r28->head.obj_size)
    {
        var_r28 = var_r28->head.parent;
    }
    if (var_r29 == var_r28) {
        var_r29->head.nb_exist--;
        class_info->head.nb_exist++;
        if (class_info->head.nb_exist > class_info->head.nb_peak) {
            class_info->head.nb_peak = class_info->head.nb_exist;
        }
        object->parent.class_info = class_info;
        return true;
    }
    return false;
}

bool hsdChangeClass(void* object, void* class_info)
{
    return hsdChangeClass_inline(object, class_info);
}

bool hsdIsDescendantOf(void* info, void* p)
{
    HSD_ClassInfo* var_r31;
    HSD_ClassInfo* cls = (HSD_ClassInfo*) p;

    if (info == NULL || p == NULL) {
        return false;
    }

    var_r31 =
#ifdef MUST_MATCH
        var_r31 =
#endif
            info;

    if (!(HSD_CLASS_INFO(info)->head.flags & 1)) {
        var_r31->head.info_init();
    }
    if (!(cls->head.flags & 1)) {
        cls->head.info_init();
    }
    while (var_r31 != NULL) {
        if (var_r31 == cls) {
            return true;
        }
        var_r31 = var_r31->head.parent;
    }
    return false;
}

bool hsdObjIsDescendantOf(HSD_Obj* o, HSD_ClassInfo* p)
{
    HSD_ClassInfo* info;

    if (o == NULL || p == NULL) {
        return false;
    }
    info = o->parent.class_info;
    if (!(p->head.flags & 1)) {
        p->head.info_init();
    }
    while (info != NULL) {
        if (info == p) {
            return true;
        }
        info = info->head.parent;
    }
    return false;
}

void class_set_flags(HSD_ClassInfo* class_info, s32 set, s32 reset)
{
    class_info->head.flags = (class_info->head.flags & ~reset) | set;
}

void ForgetClassLibraryReal(HSD_ClassInfo* class_info)
{
    HSD_ClassInfo* cur = class_info->head.child;
    HSD_ClassInfo* next;
    while (cur != NULL) {
        next = cur->head.next;
        cur->head.next = NULL;
        ForgetClassLibraryReal(cur);
        cur = next;
    }
    class_info->amnesia(class_info);
    class_info->head.child = NULL;
    class_info->head.parent = NULL;
    class_set_flags(class_info, 0, 1);
}

void ForgetClassLibraryChild(const char* library_name,
                             HSD_ClassInfo* class_info)
{
    HSD_ClassInfo** cur = &class_info->head.child;
    while (*cur != NULL) {
        if (strcmp(library_name, (*cur)->head.library_name) == 0) {
            ForgetClassLibraryReal(*cur);
            *cur = (*cur)->head.next;
        } else {
            cur = &(*cur)->head.next;
        }
    }
}

void hsdForgetClassLibrary(const char* library_name)
{
    if (library_name == NULL) {
        library_name = "sysdolphin_base_library";
    }
    if (!(hsdClass.head.flags & 1)) {
        return;
    }
#ifdef MELEE_NATIVE
    // Retire frozen definitions before child amnesia callbacks can initialize
    // or remove their allocator registrations. Retained backing stays live.
    if(nativeHsdArena.lifetime)
        HSD_ASSERTREPORT(0,nativeHsdArena_ResetHistory(),"Cannot retire class history.\n");
#endif
    if (strcmp(library_name, hsdClass.head.library_name) == 0) {
        current_hash = NULL;
        ForgetClassLibraryReal(&hsdClass);
    } else {
        ForgetClassLibraryChild(library_name, &hsdClass);
    }
}

HSD_ClassInfo* hsdSearchClassInfo(const char* class_name)
{
    if (current_hash != NULL) {
        return (HSD_ClassInfo*) HSD_HashSearch(current_hash,
                                               (void*) class_name, 0);
    }
    return NULL;
}

#ifdef MUST_MATCH
#pragma push
#pragma force_active on
static char unused5[] = "info_hash";
#pragma pop
#endif

void DumpClassStat(HSD_ClassInfo* info, s32 level)
{
    OSReport_PrintSpaces(level);
    OSReport("<class %s>\n", info->head.class_name);
    OSReport_PrintSpaces(level);
    OSReport("    info %d object %d nb_exist %d nb_peak %d\n",
             info->head.info_size, info->head.obj_size, info->head.nb_exist,
             info->head.nb_peak);
}

void hsdDumpClassStat(HSD_ClassInfo* info, bool recursive, s32 level)
{
    if (info == NULL) {
        hsdDumpClassStat(&hsdClass, true, level);
    } else if (info->head.flags & 1) {
        DumpClassStat(info, level);
        if (recursive) {
            level += 2;
            info = info->head.child;
            while (info != NULL) {
                hsdDumpClassStat(info, true, level);
                info = info->head.next;
            }
        }
    }
}

#ifdef MELEE_NATIVE
static bool class_overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return an > UINTPTR_MAX - x || bn > UINTPTR_MAX - y ||
           (x < y + bn && y < x + an);
}

static bool collect_classes(HSD_ClassInfo** list, uint32_t* count)
{
    *count = 0;
    if (!(hsdClass.head.flags & 1)) return true;
    HSD_ClassInfo* stack[NATIVE_CLASS_SNAPSHOT_TYPES];
    uint32_t pending = 1;
    stack[0] = &hsdClass;
    while (pending) {
        HSD_ClassInfo* info = stack[--pending];
        if (*count == NATIVE_CLASS_SNAPSHOT_TYPES || !(info->head.flags & 1) ||
            info->head.obj_size < (s16)sizeof(HSD_Class) ||
            info->head.info_size < (s16)sizeof(HSD_ClassInfo) ||
            info->head.nb_peak < info->head.nb_exist) return false;
        for (uint32_t i = 0; i < *count; ++i)
            if (list[i] == info) return false;
        list[(*count)++] = info;
        if (info->head.next) {
            if (pending == NATIVE_CLASS_SNAPSHOT_TYPES) return false;
            stack[pending++] = info->head.next;
        }
        if (info->head.child) {
            if (pending == NATIVE_CLASS_SNAPSHOT_TYPES ||
                info->head.child->head.parent != info) return false;
            stack[pending++] = info->head.child;
        }
    }
    return true;
}

static bool class_buffer(const void* data, size_t bytes,
                         HSD_ClassInfo** types, uint32_t count)
{
    if (!data || !bytes ||
        class_overlap(data,bytes,&nativeHsdArena,sizeof(nativeHsdArena)) ||
        class_overlap(data,bytes,nativeHsdArena.memory,nativeHsdArena.capacity) ||
        class_overlap(data, bytes, class_blocks, sizeof(class_blocks)) ||
        class_overlap(data, bytes, &class_block_count, sizeof(class_block_count)) ||
        class_overlap(data, bytes, &class_epoch, sizeof(class_epoch)) ||
        class_overlap(data, bytes, &memory_list, sizeof(memory_list)) ||
        class_overlap(data, bytes, &nb_memory_list, sizeof(nb_memory_list)) ||
        class_overlap(data, bytes, &current_hash, sizeof(current_hash))) return false;
    // Ordinary class blocks are constructed only within nativeHsdArena. Its
    // complete range exclusion above covers every block, including future ones.
    for (uint32_t i = 0; i < count; ++i)
        if (class_overlap(data, bytes, types[i], (size_t)types[i]->head.info_size)) return false;
    return true;
}

bool nativeHsdArena_AdmitHistory(void)
{
    HSD_ClassInfo* types[NATIVE_CLASS_SNAPSHOT_TYPES];uint32_t count;
    if(nativeHsdReservation_Active() || current_hash || class_block_count>NATIVE_CLASS_SNAPSHOT_BLOCKS || !collect_classes(types,&count) ||
       !nativeHsdArena.lifetime || count+5>NATIVE_MATCH_ARENA_ROOTS)return false;
    size_t used=nativeMatchArena_Used(&nativeHsdArena);
    uintptr_t begin=(uintptr_t)nativeHsdArena.memory;
    for(uint32_t i=0;i<class_block_count;++i){
        uintptr_t at=(uintptr_t)class_blocks[i].address;
        if(at<begin || at-begin>used || class_blocks[i].bytes>used-(at-begin))return false;
    }
    if(!nativeHsdArena_ResetHistory())return false;
#define ROOT(value) if(!nativeMatchArena_AddRoot(&nativeHsdArena,&(value),sizeof(value)))return false
    ROOT(memory_list);ROOT(nb_memory_list);ROOT(current_hash);ROOT(class_block_count);
#undef ROOT
    if(!nativeMatchArena_AddCountedRoot(&nativeHsdArena,class_blocks,sizeof(*class_blocks),
        NATIVE_CLASS_SNAPSHOT_BLOCKS,&class_block_count))return false;
    for(uint32_t i=0;i<count;++i)
        if(!nativeMatchArena_AddRoot(&nativeHsdArena,&types[i]->head,sizeof(types[i]->head)))return false;
    return nativeObjAlloc_AdmitArenaHistory();
}

NativeClassReference nativeClass_Reference(const void* address,size_t bytes)
{
    NativeClassReference invalid={0};
    if (!address || !bytes || bytes > UINTPTR_MAX-(uintptr_t)address ||
        nativeHsdReservation_Active() || current_hash) return invalid;
    uintptr_t at = (uintptr_t)address;
    for (uint32_t i = 0; i < class_block_count; ++i) {
        uintptr_t base = (uintptr_t)class_blocks[i].address;
        size_t n = class_blocks[i].bytes;
        if (at >= base && at-base <= n && bytes <= n-(at-base)) {
            return (NativeClassReference){class_epoch, i, 0, at-base, bytes};
        }
    }
    return invalid;
}

bool nativeClass_TypeKnown(const HSD_ClassInfo* address, size_t minimum_object_bytes)
{
    HSD_ClassInfo* types[NATIVE_CLASS_SNAPSHOT_TYPES];
    uint32_t count;
    if (!address || !minimum_object_bytes || nativeHsdReservation_Active() || current_hash ||
        !collect_classes(types, &count)) return false;
    for (uint32_t i = 0; i < count; ++i) if (types[i] == address) {
        return types[i]->head.obj_size > 0 && (size_t)types[i]->head.obj_size >= minimum_object_bytes;
    }
    return false;
}

void* nativeClass_Resolve(NativeClassReference ref)
{
    if (ref.epoch != class_epoch || ref.block >= class_block_count || ref.reserved ||
        !ref.bytes || nativeHsdReservation_Active() || current_hash) return NULL;
    NativeClassSnapshotBlock block = class_blocks[ref.block];
    if (ref.offset > block.bytes || ref.bytes > block.bytes-ref.offset ||
        block.bytes > UINTPTR_MAX-(uintptr_t)block.address) return NULL;
    return (u8*)block.address+ref.offset;
}

static size_t class_snapshot_bytes(uint32_t types)
{
    size_t bytes = sizeof(NativeClassSnapshotHeader) +
                   types * sizeof(NativeClassSnapshotType);
    for (uint32_t i = 0; i < class_block_count; ++i) {
        size_t n = class_blocks[i].bytes;
        if (!class_blocks[i].address || !n ||
            n > SIZE_MAX - sizeof(NativeClassSnapshotBlock) ||
            n + sizeof(NativeClassSnapshotBlock) > SIZE_MAX - bytes) return 0;
        bytes += sizeof(NativeClassSnapshotBlock) + n;
    }
    return bytes;
}

size_t nativeClass_SnapshotBytes(void)
{
    HSD_ClassInfo* types[NATIVE_CLASS_SNAPSHOT_TYPES];
    uint32_t count;
    if (nativeHsdReservation_Active() || current_hash ||
        !collect_classes(types, &count)) return 0;
    return class_snapshot_bytes(count);
}

bool nativeClass_Save(void* data, size_t capacity, size_t* written)
{
    HSD_ClassInfo* types[NATIVE_CLASS_SNAPSHOT_TYPES];
    uint32_t count;
    if (nativeHsdReservation_Active() || current_hash || !written ||
        !collect_classes(types, &count)) return false;
    size_t bytes = class_snapshot_bytes(count);
    if (!bytes || capacity < bytes || !class_buffer(data, bytes, types, count) ||
        !class_buffer(written, sizeof(*written), types, count) ||
        class_overlap(data, bytes, written, sizeof(*written))) return false;
    if (nb_memory_list < 0 || (nb_memory_list == 0) != (memory_list == NULL) ||
        (nb_memory_list && (nb_memory_list < 32 || (nb_memory_list & (nb_memory_list - 1))))) return false;
    bool table_owned = nb_memory_list == 0;
    for (uint32_t i = 0; i < class_block_count; ++i)
        if (class_blocks[i].address == memory_list &&
            class_blocks[i].bytes == (size_t)nb_memory_list * sizeof(*memory_list)) table_owned = true;
    if (!table_owned) return false;
    NativeClassSnapshotHeader header = {0};
    header.epoch = class_epoch;
    header.memory_list = memory_list;
    header.entries = nb_memory_list;
    header.blocks = class_block_count;
    header.types = count;
    header.arena_used=nativeMatchArena_Used(&nativeHsdArena);
    if(header.arena_used<64 || header.arena_used>nativeHsdArena.capacity)return false;
    unsigned char* cursor = data;
    memcpy(cursor, &header, sizeof(header)); cursor += sizeof(header);
    for (uint32_t i = 0; i < count; ++i) {
        NativeClassSnapshotType record = {0};
        record.info = types[i]; record.state = types[i]->head;
        memcpy(cursor, &record, sizeof(record)); cursor += sizeof(record);
    }
    for (uint32_t i = 0; i < class_block_count; ++i) {
        uintptr_t at=(uintptr_t)class_blocks[i].address,base=(uintptr_t)nativeHsdArena.memory;
        if(at<base || at-base>header.arena_used ||
           class_blocks[i].bytes>header.arena_used-(at-base))return false;
        memcpy(cursor, &class_blocks[i], sizeof(class_blocks[i]));
        cursor += sizeof(class_blocks[i]);
        memcpy(cursor, class_blocks[i].address, class_blocks[i].bytes);
        cursor += class_blocks[i].bytes;
    }
    *written = bytes;
    return true;
}

static bool same_class_definition(const HSD_ClassInfoHead* a,
                                   const HSD_ClassInfoHead* b)
{
    return a->info_init == b->info_init && a->flags == b->flags &&
           a->library_name == b->library_name && a->class_name == b->class_name &&
           a->obj_size == b->obj_size && a->info_size == b->info_size &&
           a->parent == b->parent && a->next == b->next && a->child == b->child &&
           a->nb_peak >= a->nb_exist;
}

bool nativeClass_ValidateSnapshot(const void* data, size_t bytes)
{
    HSD_ClassInfo* types[NATIVE_CLASS_SNAPSHOT_TYPES];
    uint32_t count;
    if (nativeHsdReservation_Active() || current_hash ||
        bytes < sizeof(NativeClassSnapshotHeader) ||
        !collect_classes(types, &count) || !class_buffer(data, bytes, types, count)) return false;
    NativeClassSnapshotHeader header;
    const unsigned char* cursor = data;
    memcpy(&header, cursor, sizeof(header));
    cursor += sizeof(header); bytes -= sizeof(header);
    if (header.epoch != class_epoch || header.blocks > class_block_count ||
        header.arena_used<64 || header.arena_used>nativeHsdArena.capacity ||
        header.types != count || header.entries < 0 ||
        (header.entries == 0) != (header.memory_list == NULL) ||
        (header.entries && (header.entries < 32 || (header.entries & (header.entries - 1)))) ||
        bytes < count * sizeof(NativeClassSnapshotType)) return false;
    for (uint32_t i = 0; i < count; ++i) {
        NativeClassSnapshotType record;
        memcpy(&record, cursor, sizeof(record));
        cursor += sizeof(record); bytes -= sizeof(record);
        if (record.info != types[i] || !same_class_definition(&record.state, &types[i]->head)) return false;
    }
    bool table_owned = header.entries == 0;
    for (uint32_t i = 0; i < header.blocks; ++i) {
        NativeClassSnapshotBlock record;
        if (bytes < sizeof(record)) return false;
        memcpy(&record, cursor, sizeof(record));
        cursor += sizeof(record); bytes -= sizeof(record);
        if (record.address != class_blocks[i].address ||
            record.bytes != class_blocks[i].bytes || bytes < record.bytes) return false;
        uintptr_t at=(uintptr_t)record.address,base=(uintptr_t)nativeHsdArena.memory;
        if(at<base || at-base>header.arena_used || record.bytes>header.arena_used-(at-base))return false;
        if (record.address == header.memory_list &&
            record.bytes == (size_t)header.entries * sizeof(*memory_list)) table_owned = true;
        cursor += record.bytes; bytes -= record.bytes;
    }
    return bytes == 0 && table_owned;
}

bool nativeClass_Load(const void* data, size_t bytes)
{
    if (!nativeClass_ValidateSnapshot(data, bytes)) return false;
    NativeClassSnapshotHeader header;
    const unsigned char* cursor = data;
    memcpy(&header, cursor, sizeof(header)); cursor += sizeof(header);
    for (uint32_t i = 0; i < header.types; ++i) {
        NativeClassSnapshotType record;
        memcpy(&record, cursor, sizeof(record)); cursor += sizeof(record);
        record.info->head.nb_exist = record.state.nb_exist;
        record.info->head.nb_peak = record.state.nb_peak;
    }
    for (uint32_t i = 0; i < header.blocks; ++i) {
        NativeClassSnapshotBlock record;
        memcpy(&record, cursor, sizeof(record)); cursor += sizeof(record);
        memcpy(record.address, cursor, record.bytes); cursor += record.bytes;
    }
    class_block_count = header.blocks;
    memory_list = header.memory_list;
    nb_memory_list = header.entries;
    ((NativeMatchArenaCursor*)nativeHsdArena.memory)->used=header.arena_used;
    return true;
}
NativeClassSnapshotView nativeClass_ViewSnapshot(const void* data,size_t bytes,
                                                 const uint8_t** index,uint32_t capacity)
{
    NativeClassSnapshotView invalid={0};
    if(!index || !capacity || capacity>NATIVE_CLASS_SNAPSHOT_BLOCKS ||
       !nativeClass_ValidateSnapshot(data,bytes))return invalid;
    NativeClassSnapshotHeader header;memcpy(&header,data,sizeof(header));
    HSD_ClassInfo* types[NATIVE_CLASS_SNAPSHOT_TYPES];uint32_t count;
    size_t index_bytes=(size_t)capacity*sizeof(*index);
    if(header.blocks>capacity || !collect_classes(types,&count) ||
       !class_buffer(index,index_bytes,types,count) || class_overlap(index,index_bytes,data,bytes))return invalid;
    const uint8_t* definitions=(const uint8_t*)data+sizeof(header);
    const uint8_t* cursor=definitions+(size_t)header.types*sizeof(NativeClassSnapshotType);
    // All validation and alias checks precede the first caller-index write.
    for(uint32_t i=0;i<header.blocks;++i) {
        NativeClassSnapshotBlock block;memcpy(&block,cursor,sizeof(block));
        cursor+=sizeof(block);index[i]=cursor;cursor+=block.bytes;
    }
    return (NativeClassSnapshotView){header,definitions,index};
}
const void* nativeClass_ReadSnapshot(NativeClassSnapshotView view,NativeClassReference reference)
{
    if(!view.types || !view.payloads || view.header.epoch!=class_epoch ||
       reference.epoch!=view.header.epoch || reference.block>=view.header.blocks ||
       reference.block>=class_block_count || reference.reserved || !reference.bytes ||
       nativeHsdReservation_Active() || current_hash)return NULL;
    const uint8_t* payload=view.payloads[reference.block];
    NativeClassSnapshotBlock block;memcpy(&block,payload-sizeof(block),sizeof(block));
    const NativeClassSnapshotBlock* retained=&class_blocks[reference.block];
    if(block.address!=retained->address || block.bytes!=retained->bytes ||
       reference.offset>block.bytes || reference.bytes>block.bytes-reference.offset)return NULL;
    return payload+reference.offset;
}
void* nativeClass_ResolveSnapshot(NativeClassSnapshotView view,NativeClassReference reference)
{
    const uint8_t* payload=nativeClass_ReadSnapshot(view,reference);
    if(!payload)return NULL;
    NativeClassSnapshotBlock block;
    memcpy(&block,view.payloads[reference.block]-sizeof(block),sizeof(block));
    return (uint8_t*)block.address+reference.offset;
}
bool nativeClass_SnapshotTypeKnown(NativeClassSnapshotView view,const HSD_ClassInfo* type,size_t minimum_bytes)
{
    if(!view.types || !view.payloads || view.header.epoch!=class_epoch || !type || !minimum_bytes ||
       nativeHsdReservation_Active() || current_hash)return false;
    for(uint32_t i=0;i<view.header.types;++i) {
        NativeClassSnapshotType record;
        memcpy(&record,view.types+(size_t)i*sizeof(record),sizeof(record));
        if(record.info==type)return record.state.obj_size>0 && (size_t)record.state.obj_size>=minimum_bytes;
    }
    return false;
}

bool nativeClass_SnapshotObjectType(NativeClassSnapshotView view,NativeClassReference reference,
                                    const HSD_ClassInfo* type)
{
    if(reference.bytes<sizeof(HSD_Class) || !nativeClass_SnapshotTypeKnown(view,type,reference.bytes))return false;
    const void* payload=nativeClass_ReadSnapshot(view,reference);
    if(!payload)return false;
    HSD_Class object;memcpy(&object,payload,sizeof(object));
    return object.class_info==type;
}

#endif
