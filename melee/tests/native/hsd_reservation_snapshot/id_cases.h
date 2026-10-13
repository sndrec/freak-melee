#include <sysdolphin/baselib/native_id_snapshot.h>

static unsigned id_checks,id_rejections;
static unsigned char id_objects[32768],id_heap[32768];
static NativeIdSnapshot id_saved,id_bad,id_before,id_after;

static void reject_id(size_t bytes)
{
    size_t before,after;
    CHECK(nativeID_Save(&id_before,sizeof(id_before),&before));
    CHECK(!nativeID_ValidateSnapshot(&id_bad,bytes)&&!nativeID_Load(&id_bad,bytes));
    CHECK(nativeID_Save(&id_after,sizeof(id_after),&after));
    CHECK(before==after&&!memcmp(&id_before,&id_after,before));++id_rejections;
}
static void test_id_roots(void)
{
    unsigned first=checks;size_t id_bytes,obj_bytes;u32 heap_bytes;
    _HSD_ObjAllocForgetMemory(NULL,NULL);HSD_ObjSetHeap(0,NULL);HSD_IDInitAllocData();HSD_IDSetup();
    uintptr_t a=(uintptr_t)UINT64_C(0x1234567800000020),b=a+101,c=b+101;
    int first_value=17,second_value=29;
    HSD_IDInsertToTable(NULL,a,&first_value);HSD_IDInsertToTable(NULL,b,&second_value);
    HSD_IDInsertToTable(NULL,c,NULL);
    CHECK(nativeID_Save(&id_saved,sizeof(id_saved),&id_bytes));
    CHECK(nativeObjAlloc_Save(id_objects,sizeof(id_objects),&obj_bytes));
    CHECK(OSSaveHeapSnapshot(HSD_GetHeap(),id_heap,sizeof(id_heap),&heap_bytes));
    IDEntry* entry=id_saved.roots.table[a%101];CHECK(entry);
    CHECK(!nativeID_Save(entry,sizeof(id_saved),&id_bytes));
    CHECK(!nativeID_Save(&id_bad,sizeof(id_bad),(size_t*)entry));
    CHECK(!nativeID_Save(&id_bad,sizeof(id_bad),(size_t*)&id_bad));
    HSD_IDRemoveByIDFromTable(NULL,a);HSD_IDRemoveByIDFromTable(NULL,b);
    HSD_IDInsertToTable(NULL,a+1,NULL);HSD_IDInsertToTable(NULL,b+2,&first_value);
    HSD_IDInsertToTable(NULL,c+3,&second_value); // Grow after two free-slot reuses.
    id_bad=id_saved;reject_id(sizeof(id_bad)-1);
    id_bad=id_saved;++id_bad.epoch;reject_id(sizeof(id_bad));
    id_bad=id_saved;id_bad.roots.table[0]=(IDEntry*)1;reject_id(sizeof(id_bad));
    CHECK(OSValidateHeapSnapshot(HSD_GetHeap(),id_heap,heap_bytes));
    CHECK(nativeObjAlloc_ValidateSnapshot(id_objects,obj_bytes)&&nativeID_ValidateSnapshot(&id_saved,sizeof(id_saved)));
    CHECK(OSLoadHeapSnapshot(HSD_GetHeap(),id_heap,heap_bytes));
    // Root load can run before or after entry-byte restore. It must not walk a
    // graph between companion restores, even when freed slots were reused.
    CHECK(nativeID_Load(&id_saved,sizeof(id_saved)));
    CHECK(nativeObjAlloc_Load(id_objects,obj_bytes));
    s32 found;CHECK(HSD_IDGetData(a,&found)==&first_value&&found);
    CHECK(HSD_IDGetData(b,&found)==&second_value&&found);
    CHECK(HSD_IDGetData(c,&found)==NULL&&found);
    CHECK(HSD_IDGetData(a+1,&found)==NULL&&!found);
    CHECK(nativeID_Save(&id_after,sizeof(id_after),&id_bytes));
    CHECK(!memcmp(&id_saved,&id_after,sizeof(id_saved)));
    _HSD_IDForgetMemory(NULL,NULL);CHECK(!nativeID_Load(&id_saved,sizeof(id_saved)));
    HSD_IDSetup();CHECK(!nativeID_Load(&id_saved,sizeof(id_saved)));
    id_checks=checks-first;
}
