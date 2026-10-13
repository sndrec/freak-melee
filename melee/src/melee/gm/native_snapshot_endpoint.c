#include "native_snapshot_endpoint.h"
#include <string.h>

static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
bool nativeSnapshotEndpoint_Save(NativeSnapshotEndpoint* endpoint,void* data,size_t capacity)
{
    if(!endpoint || !data || nativeRollbackPages_IsInitialized())return false;
    NativeRollbackRegionSet regions;
    if(!nativeRollbackRegions_Build(&regions))return false;
    size_t bytes=0;
    for(unsigned i=0;i<regions.count;++i) {
        NativeRollbackPagedRegion r=regions.regions[i];
        if(r.size>SIZE_MAX-bytes || overlap(data,capacity,r.memory,r.size) ||
           overlap(endpoint,sizeof(*endpoint),r.memory,r.size))return false;
        bytes+=r.size;
    }
    for(unsigned i=0;i<regions.exact_count;++i) {
        NativeRollbackExactRegion r=regions.exact_regions[i];
        if(r.size>SIZE_MAX-bytes || overlap(data,capacity,r.memory,r.size) ||
           overlap(endpoint,sizeof(*endpoint),r.memory,r.size))return false;
        bytes+=r.size;
    }
    if(bytes>capacity || overlap(data,capacity,endpoint,sizeof(*endpoint)))return false;
    uint8_t* cursor=data;
    for(unsigned i=0;i<regions.count;++i) {
        memcpy(cursor,regions.regions[i].memory,regions.regions[i].size);cursor+=regions.regions[i].size;
    }
    for(unsigned i=0;i<regions.exact_count;++i) {
        memcpy(cursor,regions.exact_regions[i].memory,regions.exact_regions[i].size);cursor+=regions.exact_regions[i].size;
    }
    *endpoint=(NativeSnapshotEndpoint){regions,data,bytes,true};return true;
}
void nativeSnapshotEndpoint_Restore(const NativeSnapshotEndpoint* endpoint)
{
    if(!endpoint || !endpoint->saved)return;
    const uint8_t* cursor=endpoint->data;
    for(unsigned i=0;i<endpoint->regions.count;++i) {
        NativeRollbackPagedRegion r=endpoint->regions.regions[i];memcpy(r.memory,cursor,r.size);cursor+=r.size;
    }
    for(unsigned i=0;i<endpoint->regions.exact_count;++i) {
        NativeRollbackExactRegion r=endpoint->regions.exact_regions[i];memcpy(r.memory,cursor,r.size);cursor+=r.size;
    }
}
bool nativeSnapshotEndpoint_Matches(const NativeSnapshotEndpoint* endpoint)
{
    if(!endpoint || !endpoint->saved)return false;
    const uint8_t* cursor=endpoint->data;
    for(unsigned i=0;i<endpoint->regions.count;++i) {
        NativeRollbackPagedRegion r=endpoint->regions.regions[i];
        if(memcmp(r.memory,cursor,r.size))return false;cursor+=r.size;
    }
    for(unsigned i=0;i<endpoint->regions.exact_count;++i) {
        NativeRollbackExactRegion r=endpoint->regions.exact_regions[i];
        if(memcmp(r.memory,cursor,r.size))return false;cursor+=r.size;
    }
    return true;
}
enum { PoolState, FreePayload, StridePadding, FreeTail, StorageKinds };
typedef struct PoolSpan {
    const uint8_t* data;
    size_t bytes;
    unsigned owner, storage;
} PoolSpan;
typedef struct PoolCoverage {
    PoolSpan spans[NATIVE_SNAPSHOT_ENDPOINT_POOLS*4];
    unsigned span_count;
    const NativeSnapshotEndpointPool* pools;
    size_t counts[NATIVE_SNAPSHOT_ENDPOINT_POOLS][StorageKinds];
    size_t unclassified;
} PoolCoverage;

static bool prepare_coverage(PoolCoverage* coverage,
    const NativeSnapshotEndpointPool* pools,size_t count)
{
    if(count>NATIVE_SNAPSHOT_ENDPOINT_POOLS || (count && !pools))return false;
    memset(coverage,0,sizeof(*coverage));coverage->pools=pools;
    for(unsigned owner=0;owner<count;++owner) {
        const NativeMatchPool* pool=pools[owner].pool;
        if(!pools[owner].name || !*pools[owner].name || !pool ||
           !pool->capacity || pool->capacity>NATIVE_MATCH_POOL_MAX_SLOTS ||
           !pool->stride || pool->stride>SIZE_MAX/pool->capacity ||
           !pool->payload || !pool->slots || !pool->free_order)return false;
        PoolSpan spans[]={
            {(const uint8_t*)pool,sizeof(*pool),owner,PoolState},
            {(const uint8_t*)pool->slots,sizeof(*pool->slots)*pool->capacity,owner,PoolState},
            {(const uint8_t*)pool->free_order,sizeof(*pool->free_order)*pool->capacity,owner,FreeTail},
            {pool->payload,pool->stride*pool->capacity,owner,FreePayload}};
        for(unsigned i=0;i<4;++i) {
            if(spans[i].bytes>UINTPTR_MAX-(uintptr_t)spans[i].data)return false;
            for(unsigned j=0;j<coverage->span_count;++j)
                if(overlap(spans[i].data,spans[i].bytes,
                           coverage->spans[j].data,coverage->spans[j].bytes))return false;
            coverage->spans[coverage->span_count++]=spans[i];
        }
        if(!nativeMatchPool_Validate(pool))return false;
    }
    return true;
}
static void classify_byte(PoolCoverage* coverage,const uint8_t* address)
{
    uintptr_t at=(uintptr_t)address;
    for(unsigned i=0;i<coverage->span_count;++i) {
        const PoolSpan* span=&coverage->spans[i];
        uintptr_t base=(uintptr_t)span->data;
        if(at<base || at-base>=span->bytes)continue;
        const NativeMatchPool* pool=coverage->pools[span->owner].pool;
        size_t offset=at-base;unsigned storage=span->storage;
        if(storage==FreePayload) {
            size_t slot=offset/pool->stride;
            if(offset%pool->stride>=pool->object_size)storage=StridePadding;
            else if(pool->slots[slot].occupied)storage=PoolState;
        } else if(storage==FreeTail && offset<pool->free_count*sizeof(uint32_t))
            storage=PoolState;
        ++coverage->counts[span->owner][storage];return;
    }
    ++coverage->unclassified;
}
static size_t report_region(const uint8_t* expected,const uint8_t* actual,size_t bytes,
                            const char* kind,unsigned region,PoolCoverage* coverage,FILE* output)
{
    size_t differences=0,ranges=0;
    for(size_t i=0;i<bytes;) {
        if(expected[i]==actual[i]) {++i;continue;}
        size_t first=i;
        while(i<bytes && expected[i]!=actual[i]) {
            classify_byte(coverage,actual+i);++differences;++i;
        }
        if(ranges<4096) {
            fprintf(output,"[snapshot-census] rewind-raw-range kind=%s region=%u address=%p offset=%zu bytes=%zu old=",
                    kind,region,(const void*)(actual+first),first,i-first);
            for(size_t k=first;k<i && k<first+8;++k)fprintf(output,"%02x",expected[k]);
            fprintf(output," current=");
            for(size_t k=first;k<i && k<first+8;++k)fprintf(output,"%02x",actual[k]);
            fprintf(output,"\n");
        }
        ++ranges;
    }
    fprintf(output,"[snapshot-census] rewind-raw-region kind=%s region=%u address=%p bytes=%zu differing_bytes=%zu ranges=%zu\n",
            kind,region,(const void*)actual,bytes,differences,ranges);
    return differences;
}
size_t nativeSnapshotEndpoint_ReportDifferences(const NativeSnapshotEndpoint* endpoint,
    const NativeSnapshotEndpointPool* pools,size_t pool_count,FILE* output)
{
    PoolCoverage coverage;
    if(!endpoint || !endpoint->saved || !output ||
       !prepare_coverage(&coverage,pools,pool_count))return SIZE_MAX;
    const uint8_t* cursor=endpoint->data;size_t total=0;
    for(unsigned i=0;i<endpoint->regions.count;++i) {
        NativeRollbackPagedRegion r=endpoint->regions.regions[i];
        total+=report_region(cursor,r.memory,r.size,"paged",i,&coverage,output);cursor+=r.size;
    }
    for(unsigned i=0;i<endpoint->regions.exact_count;++i) {
        NativeRollbackExactRegion r=endpoint->regions.exact_regions[i];
        total+=report_region(cursor,r.memory,r.size,"exact",i,&coverage,output);cursor+=r.size;
    }
    size_t state=0,inactive=0;
    for(unsigned i=0;i<pool_count;++i) {
        const size_t* count=coverage.counts[i];
        state+=count[PoolState];inactive+=count[FreePayload]+count[StridePadding]+count[FreeTail];
        fprintf(output,"[snapshot-census] rewind-pool-coverage owner=%s state=%zu free_payload=%zu stride_padding=%zu free_tail=%zu\n",
                pools[i].name,count[PoolState],count[FreePayload],count[StridePadding],count[FreeTail]);
    }
    fprintf(output,"[snapshot-census] rewind-coverage-summary total=%zu pool_state=%zu inactive_pool=%zu unclassified=%zu\n",
            total,state,inactive,coverage.unclassified);
    return total;
}
