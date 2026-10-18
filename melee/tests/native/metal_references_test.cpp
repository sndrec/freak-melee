#undef NDEBUG
#include <melee/ft/native_fighter_metal_references.h>
#include <melee/ft/native_asset_dat.h>
#include <array>
#include <cstdio>
#include <cstdlib>

static unsigned checks;
static char error[256];
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s [%s]\n",__LINE__,#x,error); std::exit(1); } } while (0)
static uint32_t ptr(const HSD_Dat& dat, uint32_t slot) {
    u32 target; const char* external;
    CHECK(HSD_DatResolve(&dat,slot,&target,&external)==HSD_DAT_REF_LOCAL);
    return target;
}
static uint32_t word(const HSD_Dat& dat, uint32_t slot) {
    u32 value; CHECK(HSD_DatReadU32(&dat,slot,&value)); return value;
}
struct Fixture {
    std::vector<uint8_t> bytes;
    std::array<uint32_t,3> joints;
    uint32_t visibility, lookup, list, ids;
    Fixture() {
        // Model record at offset zero, with the retail unrelocated root link.
        bytes.resize(32+256+8+8,0);
        NativeAssetDat::be32(bytes.data(),(uint32_t)bytes.size());
        NativeAssetDat::be32(bytes.data()+4,256);
        NativeAssetDat::be32(bytes.data()+12,1);
        NativeAssetDat::be32(bytes.data()+32+256,64);
        std::memcpy(bytes.data()+32+256+8,"Fighter",8);
        NativeAssetDat dat; CHECK(dat.open(bytes.data(),bytes.size()));
        for (auto& joint:joints) { joint=dat.allocate(64); dat.word(joint+4,1); }
        uint32_t a=dat.allocate(16), b=dat.allocate(16), c=dat.allocate(16);
        dat.link_local(a+4,b);
        // B occurs on two joints: descriptor identity alone is insufficient.
        dat.link_local(joints[0]+16,a); dat.link_local(joints[1]+16,b);
        dat.link_local(joints[2]+16,c);
        visibility=dat.allocate(32); lookup=dat.allocate(16);
        list=dat.allocate(16); ids=dat.allocate(3);
        dat.data[ids]=0; dat.data[ids+1]=2; dat.data[ids+2]=3;
        dat.word(0,2); dat.link_local(4,visibility);
        dat.word(lookup,2); dat.link_local(lookup+4,list);
        dat.word(lookup+8,1); dat.link_local(lookup+12,list);
        dat.word(list,3); dat.link_local(list+4,ids);
        // An empty second variant tests null index lists.
        for (unsigned row=0;row<2;++row)
            for (unsigned layer=0;layer<4;++layer)
                dat.link_local(visibility+row*16+layer*4,lookup);
        CHECK(dat.finish(&bytes));
    }
};
static std::vector<uint8_t> remap(const Fixture& f, const std::array<uint32_t,3>& order,
                                 bool costume_changed) {
    HSD_Dat source; CHECK(HSD_DatOpen(&source,f.bytes.data(),f.bytes.size())==HSD_DAT_OK);
    NativeAssetDat candidate; CHECK(candidate.open(f.bytes.data(),f.bytes.size()));
    if (costume_changed) {
        candidate.data[f.ids]=3; candidate.data[f.ids+1]=1; candidate.data[f.ids+2]=0;
    }
    CHECK(nativeFighterModel_RemapMetalSelectors(source,"Fighter",f.joints.data(),3,
          order.data(),3,candidate,error,sizeof(error)));
    CHECK(!error[0]);
    std::vector<uint8_t> output; CHECK(candidate.finish(&output)); return output;
}
int main() {
    const Fixture f;
    // Every permutation, both independently and with shared costume selectors
    // already modified, must select the same (joint, DObj) instances.
    std::array<unsigned,3> order={0,1,2}; unsigned cases=0;
    do {
        std::array<uint32_t,3> joints={f.joints[order[0]],f.joints[order[1]],f.joints[order[2]]};
        for (bool costume_changed:{false,true}) {
            auto bytes=remap(f,joints,costume_changed); HSD_Dat result;
            CHECK(HSD_DatOpen(&result,bytes.data(),bytes.size())==HSD_DAT_OK);
            unsigned map[4], index=0;
            for (unsigned bone:order) {
                if (!bone) { map[0]=index++; map[1]=index++; }
                else map[bone+1]=index++;
            }
            uint32_t metal=ptr(result,f.visibility+8);
            CHECK(ptr(result,f.visibility+24)==metal); // Row alias retained.
            uint32_t variants=ptr(result,metal+4), ids=ptr(result,variants+4);
            CHECK(word(result,metal)==2 && word(result,metal+8)==1);
            CHECK(word(result,variants)==3 && word(result,variants+8)==0);
            CHECK(result.data[ids]==map[0] && result.data[ids+1]==map[2] && result.data[ids+2]==map[3]);
            CHECK(ptr(result,ptr(result,metal+12)+4)==ids); // Index alias retained.
            for (unsigned row=0;row<2;++row) for (unsigned layer:{0u,1u,3u}) {
                CHECK(ptr(result,f.visibility+row*16+layer*4)==f.lookup);
                CHECK(result.data[f.ids]==(costume_changed?3:0));
                CHECK(result.data[f.ids+1]==(costume_changed?1:2));
                CHECK(result.data[f.ids+2]==(costume_changed?0:3));
            }
            if (order==std::array<unsigned,3>{0,1,2} && !costume_changed)
                CHECK(bytes==f.bytes); // Identity edit appends nothing.
            ++cases;
        }
    } while (std::next_permutation(order.begin(),order.end()));
    // A required mesh disappears, and invalid source ordinals are rejected.
    HSD_Dat source; CHECK(HSD_DatOpen(&source,f.bytes.data(),f.bytes.size())==HSD_DAT_OK);
    NativeAssetDat candidate; CHECK(candidate.open(f.bytes.data(),f.bytes.size()));
    CHECK(!nativeFighterModel_RemapMetalSelectors(source,"Fighter",f.joints.data(),3,
        f.joints.data(),2,candidate,error,sizeof(error)) && error[0]);
    auto malformed=f.bytes; malformed[32+f.ids]=4;
    CHECK(HSD_DatOpen(&source,malformed.data(),malformed.size())==HSD_DAT_OK);
    NativeAssetDat bad; CHECK(bad.open(malformed.data(),malformed.size()));
    CHECK(!nativeFighterModel_RemapMetalSelectors(source,"Fighter",f.joints.data(),3,
        f.joints.data(),3,bad,error,sizeof(error)) && error[0]);
    std::printf("PASS %u permutations/alias cases %u checks\n",cases,checks);
}
