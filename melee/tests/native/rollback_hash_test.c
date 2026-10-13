#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/gm/native_rollback_hash.h>

typedef struct HashFixture {
    uintptr_t self;
    uintptr_t other;
    uintptr_t image;
    uintptr_t asset;
    uint64_t value;
} HashFixture;

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static uint64_t digest_pair(HashFixture* first, HashFixture* second,
                            uintptr_t* exact, const void* asset)
{
    NativeRollbackRegionSet regions = { 0 };
    regions.regions[0] = (NativeRollbackPagedRegion){
        .memory = first,
        .size = sizeof(*first),
    };
    regions.regions[1] = (NativeRollbackPagedRegion){
        .memory = second,
        .size = sizeof(*second),
    };
    regions.count = 2;
    regions.exact_regions[0] = (NativeRollbackExactRegion){
        .memory = exact,
        .size = sizeof(*exact),
    };
    regions.exact_count = 1;
    regions.identity_regions[0] = (NativeAssetIdentityRegion){
        .memory = asset,
        .size = 64,
        .identity = UINT64_C(0xF1A5E7A55E7),
    };
    regions.identity_count = 1;
    return nativeRollbackHash_AddressNormalized(&regions);
}

int main(void)
{
    HashFixture* first =
        VirtualAlloc(NULL, sizeof(HashFixture) * 2, MEM_RESERVE | MEM_COMMIT,
                     PAGE_READWRITE);
    HashFixture* second =
        VirtualAlloc(NULL, sizeof(HashFixture) * 2, MEM_RESERVE | MEM_COMMIT,
                     PAGE_READWRITE);
    require(first != NULL && second != NULL, "allocate hash fixtures");
    void* first_asset =
        VirtualAlloc(NULL, 64, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    void* second_asset =
        VirtualAlloc(NULL, 64, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    require(first_asset != NULL && second_asset != NULL,
            "allocate asset identity fixtures");

    memset(first_asset, 0x6D, 64);
    memset(second_asset, 0x6D, 64);
    uint64_t first_content = nativeAssetIdentity_Hash(first_asset, 64);
    uint64_t second_content = nativeAssetIdentity_Hash(second_asset, 64);
    require(first_content == second_content,
            "equal asset bytes have equal content identities");
    uint64_t asset_identity = nativeAssetIdentity_Derive(
        first_content, UINT64_C(0x5445535441535345));
    require(nativeAssetIdentity_Register(first_asset, 64, asset_identity),
            "register first immutable asset region");
    require(nativeAssetIdentity_Register(first_asset, 64, asset_identity),
            "repeat identical asset registration");
    require(!nativeAssetIdentity_Register((uint8_t*) first_asset + 32, 32,
                                          asset_identity),
            "reject overlapping asset registration");
    require(nativeAssetIdentity_Register(second_asset, 64, asset_identity),
            "register equal asset at a different address");
    NativeAssetIdentityRegion registered[2];
    require(nativeAssetIdentity_GetRegions(registered, 2) == 2,
            "enumerate immutable asset regions");

    uintptr_t exact_first = (uintptr_t) &first[1];
    first[0] = (HashFixture){
        .self = (uintptr_t) &first[0],
        .other = (uintptr_t) &first[1],
        .image = (uintptr_t) &main,
        .asset = (uintptr_t) first_asset + 24,
        .value = UINT64_C(0x123456789ABCDEF0),
    };
    first[1] = (HashFixture){
        .self = (uintptr_t) &first[1],
        .other = (uintptr_t) &exact_first,
        .image = (uintptr_t) &main,
        .asset = (uintptr_t) first_asset + 40,
        .value = 17,
    };

    uintptr_t exact_second = (uintptr_t) &second[1];
    second[0] = first[0];
    second[0].self = (uintptr_t) &second[0];
    second[0].other = (uintptr_t) &second[1];
    second[0].asset = (uintptr_t) second_asset + 24;
    second[1] = first[1];
    second[1].self = (uintptr_t) &second[1];
    second[1].other = (uintptr_t) &exact_second;
    second[1].asset = (uintptr_t) second_asset + 40;

    uint64_t first_hash =
        digest_pair(&first[0], &first[1], &exact_first, first_asset);
    uint64_t second_hash =
        digest_pair(&second[0], &second[1], &exact_second, second_asset);
    require(first_hash == second_hash,
            "equivalent pointer graphs have one address-normalized hash");

    second[1].value += 1;
    require(first_hash != digest_pair(&second[0], &second[1], &exact_second,
                                      second_asset),
            "non-pointer state changes the address-normalized hash");

    require(VirtualFree(first, 0, MEM_RELEASE), "release first fixture");
    require(VirtualFree(second, 0, MEM_RELEASE), "release second fixture");
    require(VirtualFree(first_asset, 0, MEM_RELEASE),
            "release first asset fixture");
    require(VirtualFree(second_asset, 0, MEM_RELEASE),
            "release second asset fixture");
    return 0;
}
