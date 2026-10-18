#undef NDEBUG
#include <melee/ft/native_fighter_rig.h>
#include <melee/ft/native_asset_dat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) {std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort();} } while(0)
using Bytes=std::vector<uint8_t>;
static Bytes fixture()
{
    Bytes b(32+64+16+15,0);
    NativeAssetDat::be32(b.data(),(uint32_t)b.size());
    NativeAssetDat::be32(b.data()+4,64);
    NativeAssetDat::be32(b.data()+12,1);NativeAssetDat::be32(b.data()+16,1);
    NativeAssetDat::be32(b.data()+32+16,UINT32_MAX);
    NativeAssetDat::be32(b.data()+96+8,16);NativeAssetDat::be32(b.data()+96+12,8);
    std::memcpy(b.data()+112,"fighter\0shared\0",15);
    return b;
}
static void open(const NativeFighterProject& p,HSD_Dat& d)
{CHECK(HSD_DatOpen(&d,p.fighter_dat,p.fighter_size)==HSD_DAT_OK);}
static void symbols(const HSD_Dat& d,const Bytes& original)
{
    u32 at;const char* name;
    CHECK(HSD_DatSymbol(&d,false,0,&at,&name)&&at==0&&!std::strcmp(name,"fighter"));
    CHECK(HSD_DatSymbol(&d,true,0,&at,&name)&&at==16&&!std::strcmp(name,"shared"));
    CHECK(HSD_DatResolve(&d,16,&at,&name)==HSD_DAT_REF_EXTERNAL&&!std::strcmp(name,"shared"));
    CHECK(!std::memcmp(d.data,original.data()+32,64));
}
int main()
{
    auto source=fixture();HSD_Dat dat;NativeFighterRig rig;
    CHECK(HSD_DatOpen(&dat,source.data(),source.size())==HSD_DAT_OK);
    char error[256];
    CHECK(nativeFighterRig_Read(&dat,140,&rig,error,sizeof(error))&&!rig.authored);
    for(unsigned i=0;i<140;++i){CHECK(nativeFighterRig_Resolve(&rig,i)==(int)i);CHECK(nativeFighterRig_Id(&rig,i)==(int)i);}
    CHECK(nativeFighterRig_Resolve(&rig,-1)==-1);
    CHECK(nativeFighterRig_Resolve(&rig,INT64_MAX)==-1);

    rig.next_id=1000000;
    for(unsigned i=0;i<140;++i){
        rig.ids[i]=1000+i*512; // Force one maximal live collision chain.
        rig.flags[i]=(i&1)?NATIVE_RIG_EXTRA:0;
        std::snprintf(rig.names[i],64,"Bone %u",i);
    }
    CHECK(nativeFighterRig_Index(&rig,error,sizeof(error))&&rig.authored);
    for(unsigned i=0;i<140;++i){
        CHECK(nativeFighterRig_Resolve(&rig,1000+i*512)==(int)i);
        CHECK(nativeFighterRig_Id(&rig,i)==(int32_t)(1000+i*512));
        CHECK(nativeFighterRig_Find(&rig,rig.names[i])==(int32_t)(1000+i*512));
    }
    CHECK(nativeFighterRig_Resolve(&rig,1)==-1);
    CHECK(nativeFighterRig_Find(&rig,"removed bone")==-1);
    auto invalid=rig;invalid.ids[4]=invalid.ids[3];
    const auto before=invalid;
    CHECK(!nativeFighterRig_Index(&invalid,error,sizeof(error))&&!std::memcmp(&before,&invalid,sizeof(invalid)));
    invalid=rig;std::strcpy(invalid.names[4],invalid.names[3]);
    CHECK(!nativeFighterRig_Index(&invalid,error,sizeof(error)));
    invalid=rig;invalid.flags[0]=2;CHECK(!nativeFighterRig_Index(&invalid,error,sizeof(error)));
    invalid=rig;invalid.names[0][0]='\n';CHECK(!nativeFighterRig_Index(&invalid,error,sizeof(error)));

    NativeFighterProject project={};project.fighter_size=source.size();
    project.fighter_dat=(uint8_t*)std::malloc(source.size());CHECK(project.fighter_dat);
    std::memcpy(project.fighter_dat,source.data(),source.size());
    CHECK(nativeFighterRig_Write(&project,&rig,error,sizeof(error)));
    open(project,dat);symbols(dat,source);
    NativeFighterRig read;
    CHECK(nativeFighterRig_Read(&dat,140,&read,error,sizeof(error)));
    CHECK(!std::memcmp(&rig,&read,sizeof(rig)));
    const size_t bounded=project.fighter_size;
    for(unsigned edit=0;edit<128;++edit){
        // Remove the last ID and insert a different ID in the middle of the
        // traversal. Repeated saves reuse the owned metadata allocation.
        uint32_t removed=rig.ids[139];
        for(unsigned i=139;i>27;--i){rig.ids[i]=rig.ids[i-1];rig.flags[i]=rig.flags[i-1];std::memcpy(rig.names[i],rig.names[i-1],64);}
        rig.ids[27]=rig.next_id++;rig.flags[27]=NATIVE_RIG_EXTRA;
        std::snprintf(rig.names[27],64,"Inserted %u",edit);
        CHECK(nativeFighterRig_Index(&rig,error,sizeof(error)));
        CHECK(nativeFighterRig_Resolve(&rig,removed)==-1);
        CHECK(nativeFighterRig_Write(&project,&rig,error,sizeof(error)));
        CHECK(project.fighter_size==bounded);open(project,dat);symbols(dat,source);
        CHECK(nativeFighterRig_Read(&dat,140,&read,error,sizeof(error))&&!std::memcmp(&rig,&read,sizeof(rig)));
    }
    u32 root,table;const char* external;
    CHECK(HSD_DatFindRoot(&dat,NATIVE_RIG_ROOT,&root));
    CHECK(HSD_DatResolve(&dat,root+16,&table,&external)==HSD_DAT_REF_LOCAL);
    auto retained=read;
    NativeAssetDat::be32(project.fighter_dat+32+table,rig.ids[1]);
    open(project,dat);
    CHECK(!nativeFighterRig_Read(&dat,140,&read,error,sizeof(error))&&!std::memcmp(&retained,&read,sizeof(read)));
    std::free(project.fighter_dat);
    std::printf("PASS %u stable bone ID, metadata, root and bounded rewrite checks\n",checks);
}
