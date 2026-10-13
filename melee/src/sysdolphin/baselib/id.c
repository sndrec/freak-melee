#include <melee/gm/native_match_control_history.h>
#include "id.h"

#include <string.h>

#include "debug.h"
#ifdef MELEE_NATIVE
#include "native_hsd_reservation.h"
#include "native_id_snapshot.h"
#include "native_objalloc_snapshot.h"
static uint64_t id_epoch;
static void retire_id_roots(void)
{
    HSD_ASSERT(0, !nativeHsdReservation_Active());
    HSD_ASSERT(0, id_epoch != UINT64_MAX);
    ++id_epoch;
}
#endif

HSD_ObjAllocData hsd_iddata;

HSD_IDTable default_table;

HSD_ObjAllocData* HSD_IDGetAllocData(void)
{
    return &hsd_iddata;
}

void HSD_IDInitAllocData(void)
{
    HSD_ObjAllocInit(HSD_IDGetAllocData(), sizeof(IDEntry), 4);
}

void HSD_IDSetup(void)
{
#ifdef MELEE_NATIVE
    retire_id_roots();
#endif
    memset(&default_table, 0, sizeof(HSD_IDTable));
}

static inline u32 hash(uintptr_t id)
{
    return id % 0x65;
}

static inline IDEntry* IDEntryAlloc(void)
{
    IDEntry* entry;

    entry = HSD_ObjAlloc(HSD_IDGetAllocData());
    HSD_ASSERT(67, entry);
    memset(entry, 0, sizeof(IDEntry));

    return entry;
}

void HSD_IDInsertToTable(HSD_IDTable* table, uintptr_t id, void* data)
{
    IDEntry* entry;

    if (table == NULL) {
        table = &default_table;
    }

    entry = table->table[hash(id)];
    while (entry != NULL) {
        if (entry->id == id) {
            break;
        }
        entry = entry->next;
    }

    if (entry != NULL) {
        entry->id = id;
        entry->data = data;
    } else {
        entry = IDEntryAlloc();
        entry->id = id;
        entry->data = data;
        entry->next = table->table[hash(id)];
        table->table[hash(id)] = entry;
    }
}

static inline void IDEntryFree(IDEntry* entry)
{
    HSD_ObjFree(HSD_IDGetAllocData(), entry);
}

void HSD_IDRemoveByIDFromTable(HSD_IDTable* table, uintptr_t id)
{
    IDEntry* entry;
    IDEntry* prev;

    if (table == NULL) {
        table = &default_table;
    }

    prev = NULL;
    for (entry = table->table[hash(id)]; entry != NULL; entry = entry->next) {
        if (entry->id == id) {
            if (prev != NULL) {
                prev->next = entry->next;
            } else {
                table->table[hash(id)] = entry->next;
            }
            IDEntryFree(entry);
            return;
        }
        prev = entry;
    }
}

void* HSD_IDGetDataFromTable(HSD_IDTable* table, uintptr_t id, s32* success)
{
    IDEntry* entry;

    if (table == NULL) {
        table = &default_table;
    }

    entry = table->table[hash(id)];
    while (entry != NULL) {
        if (entry->id == id) {
            if (success != NULL) {
                *success = 1;
            }
            return entry->data;
        }
        entry = entry->next;
    }

    if (success != NULL) {
        *success = 0;
    }
    return NULL;
}

void _HSD_IDForgetMemory(void* low, void* high)
{
#ifdef MELEE_NATIVE
    retire_id_roots();
#endif
    memset(&default_table, 0, sizeof(HSD_IDTable));
}

#ifdef MELEE_NATIVE
static bool id_overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
static bool id_buffer(const void* data,size_t bytes)
{
    if(!data || !bytes || id_overlap(data,bytes,&default_table,sizeof(default_table)) ||
       id_overlap(data,bytes,&hsd_iddata,sizeof(hsd_iddata)) ||
       id_overlap(data,bytes,&id_epoch,sizeof(id_epoch)))return false;
    // Consult actual immutable backing catalogs, not live or candidate ID
    // links. Either graph may be between companion restores at this point.
    return nativeObjAlloc_BufferIndependent(data,bytes) &&
        nativeHsdReservation_BufferIndependent(data,bytes);
}
bool nativeID_Save(void* data,size_t capacity,size_t* written)
{
    if(!id_epoch || nativeHsdReservation_Active() || capacity<sizeof(NativeIdSnapshot) ||
       !id_buffer(data,sizeof(NativeIdSnapshot)) || !id_buffer(written,sizeof(*written)) ||
       id_overlap(data,sizeof(NativeIdSnapshot),written,sizeof(*written)))return false;
    NativeIdSnapshot state={0};state.epoch=id_epoch;state.roots=default_table;
    memcpy(data,&state,sizeof(state));*written=sizeof(state);return true;
}
bool nativeID_ValidateSnapshot(const void* data,size_t bytes)
{
    if(!id_epoch || nativeHsdReservation_Active() || bytes!=sizeof(NativeIdSnapshot) ||
       !id_buffer(data,bytes))return false;
    NativeIdSnapshot state;memcpy(&state,data,sizeof(state));
    if(state.epoch!=id_epoch)return false;
    for(unsigned i=0;i<101;++i)
        if((uintptr_t)state.roots.table[i] & (_Alignof(IDEntry)-1))return false;
    return true;
}
bool nativeID_Load(const void* data,size_t bytes)
{
    if(!nativeID_ValidateSnapshot(data,bytes))return false;
    NativeIdSnapshot state;memcpy(&state,data,sizeof(state));
    default_table=state.roots;return true;
}
#endif

#ifdef MELEE_NATIVE
bool nativeID_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(default_table);
#undef ROOT
    return true;
}
#endif
