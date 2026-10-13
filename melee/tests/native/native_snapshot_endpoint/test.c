#include <melee/gm/native_snapshot_endpoint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t first[16],second[32],exact[4],buffer[1024];
static bool tracking,build_ok=true;
static unsigned checks;
static NativeMatchPool pool;
static _Alignas(16) uint8_t payload[64];
static NativeMatchPoolSlot slots[4];
static uint32_t free_order[4];
static bool include_pool;
#define CHECK(c) do {++checks;if(!(c)){fprintf(stderr,"line %u: %s\n",__LINE__,#c);exit(1);}}while(0)
bool nativeRollbackPages_IsInitialized(void) {return tracking;}
bool nativeRollbackRegions_Build(NativeRollbackRegionSet* regions)
{
    if(!build_ok)return false;
    *regions=(NativeRollbackRegionSet){.count=2,.exact_count=1,
        .regions={{first,sizeof(first)},{second,sizeof(second)}},
        .exact_regions={{exact,sizeof(exact)}}};
    if(include_pool) {
        regions->regions[regions->count++]=(NativeRollbackPagedRegion){payload,sizeof(payload)};
        regions->regions[regions->count++]=(NativeRollbackPagedRegion){slots,sizeof(slots)};
        regions->regions[regions->count++]=(NativeRollbackPagedRegion){free_order,sizeof(free_order)};
        regions->regions[regions->count++]=(NativeRollbackPagedRegion){&pool,sizeof(pool)};
    }
    return true;
}
static void pool_coverage(void)
{
    CHECK(nativeMatchPool_Bind(&pool,payload,sizeof(payload),slots,free_order,
                               4,3,16,42));
    memset(payload,0xC4,sizeof(payload));
    NativeMatchPoolReference reference;
    uint8_t* live=nativeMatchPool_Allocate(&pool,&reference);
    CHECK(live==payload && live[0]==0 && live[2]==0 && live[3]==0xC4);
    include_pool=true;
    NativeSnapshotEndpoint endpoint={0};
    CHECK(nativeSnapshotEndpoint_Save(&endpoint,buffer,sizeof(buffer)));
    payload[0]=1;payload[3]^=1;payload[16]^=1;
    ((uint8_t*)&free_order[3])[0]^=1;
    slots[1].generation=1;pool.peak=2;
    CHECK(nativeMatchPool_Validate(&pool));
    NativeMatchPool before=pool;uint8_t payload_before[64];
    NativeMatchPoolSlot slots_before[4];uint32_t order_before[4];
    memcpy(payload_before,payload,sizeof(payload));memcpy(slots_before,slots,sizeof(slots));
    memcpy(order_before,free_order,sizeof(free_order));
    const NativeSnapshotEndpointPool owners[]={{"fixture",&pool}};
    FILE* report=tmpfile();CHECK(report);
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,owners,1,report)==6);
    rewind(report);char text[4096]={0};CHECK(fread(text,1,sizeof(text)-1,report)>0);
    CHECK(strstr(text,"owner=fixture state=3 free_payload=1 stride_padding=1 free_tail=1"));
    CHECK(strstr(text,"total=6 pool_state=3 inactive_pool=3 unclassified=0"));
    CHECK(!memcmp(&pool,&before,sizeof(pool)) && !memcmp(payload,payload_before,sizeof(payload)) &&
          !memcmp(slots,slots_before,sizeof(slots)) && !memcmp(free_order,order_before,sizeof(free_order)));
    fclose(report);
    // Omitted ownership remains fully visible, never treated as an ignored byte.
    report=tmpfile();CHECK(report);
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,NULL,0,report)==6);
    rewind(report);memset(text,0,sizeof(text));CHECK(fread(text,1,sizeof(text)-1,report)>0);
    CHECK(strstr(text,"total=6 pool_state=0 inactive_pool=0 unclassified=6"));fclose(report);
    report=tmpfile();CHECK(report);
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,NULL,1,report)==SIZE_MAX);
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,owners,NATIVE_SNAPSHOT_ENDPOINT_POOLS+1,report)==SIZE_MAX);
    NativeSnapshotEndpointPool bad={NULL,&pool};
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,&bad,1,report)==SIZE_MAX);
    bad=(NativeSnapshotEndpointPool){"bad",NULL};
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,&bad,1,report)==SIZE_MAX);
    NativeSnapshotEndpointPool duplicate[]={{"one",&pool},{"two",&pool}};
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,duplicate,2,report)==SIZE_MAX);
    NativeMatchPool alias=pool;NativeSnapshotEndpointPool shared[]={{"one",&pool},{"alias",&alias}};
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,shared,2,report)==SIZE_MAX);
    pool.peak=0;CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,owners,1,report)==SIZE_MAX);pool.peak=2;
    slots[0].generation=0;CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,owners,1,report)==SIZE_MAX);slots[0].generation=1;
    pool.stride=SIZE_MAX;CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,owners,1,report)==SIZE_MAX);pool.stride=16;
    CHECK(ftell(report)==0);fclose(report);
    nativeSnapshotEndpoint_Restore(&endpoint);CHECK(nativeSnapshotEndpoint_Matches(&endpoint));
    CHECK(pool.peak==1 && payload[0]==0 && payload[3]==0xC4 && payload[16]==0xC4);
    include_pool=false;
}
int main(void)
{
    memset(first,1,sizeof(first));memset(second,2,sizeof(second));memset(exact,3,sizeof(exact));
    NativeSnapshotEndpoint endpoint={0};
    CHECK(nativeSnapshotEndpoint_Save(&endpoint,buffer,sizeof(buffer)));
    CHECK(endpoint.bytes==52 && nativeSnapshotEndpoint_Matches(&endpoint));
    memset(first,4,sizeof(first));memset(second,5,sizeof(second));memset(exact,6,sizeof(exact));
    CHECK(!nativeSnapshotEndpoint_Matches(&endpoint));
    FILE* report=tmpfile();CHECK(report);
    CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,NULL,0,report)==52);
    fclose(report);
    nativeSnapshotEndpoint_Restore(&endpoint);CHECK(nativeSnapshotEndpoint_Matches(&endpoint));
    report=tmpfile();CHECK(report);CHECK(nativeSnapshotEndpoint_ReportDifferences(&endpoint,NULL,0,report)==0);fclose(report);
    CHECK(first[15]==1 && second[31]==2 && exact[3]==3);
    uint8_t saved[sizeof(buffer)];memcpy(saved,buffer,sizeof(saved));
    NativeSnapshotEndpoint before=endpoint;
    CHECK(!nativeSnapshotEndpoint_Save(&endpoint,buffer,51));
    CHECK(!memcmp(&endpoint,&before,sizeof(before)) && !memcmp(saved,buffer,sizeof(saved)));
    CHECK(!nativeSnapshotEndpoint_Save(&endpoint,first,sizeof(first)));
    CHECK(first[0]==1 && nativeSnapshotEndpoint_Matches(&endpoint));
    CHECK(!nativeSnapshotEndpoint_Save(&endpoint,&endpoint,sizeof(endpoint)));
    tracking=true;CHECK(!nativeSnapshotEndpoint_Save(&endpoint,buffer,sizeof(buffer)));tracking=false;
    build_ok=false;CHECK(!nativeSnapshotEndpoint_Save(&endpoint,buffer,sizeof(buffer)));build_ok=true;
    CHECK(!memcmp(&endpoint,&before,sizeof(before)) && !memcmp(saved,buffer,sizeof(saved)));
    CHECK(!nativeSnapshotEndpoint_Save(NULL,buffer,sizeof(buffer)));
    CHECK(!nativeSnapshotEndpoint_Matches(NULL));
    pool_coverage();
    printf("{\"checks\":%u,\"bytes\":52,\"controlled_regions\":3}\n",checks);return 0;
}
