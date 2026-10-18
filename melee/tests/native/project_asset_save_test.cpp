#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_animation_editor.h>
#include <melee/ft/native_fighter_model_editor.h>
#include <melee/ft/native_fighter_rig.h>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::abort(); } } while(0)
using Bytes = std::vector<uint8_t>;
using Snapshot = std::array<Bytes,8>;
static const char* names[] = {"fighter.dat","animation.dat","costume.dat","assets.fma"};
static std::string filename(const std::string& path,unsigned i)
{ return path + '/' + names[i%4] + (i<4?"":".rig.previous"); }
static Bytes read(const std::string& path)
{
    std::ifstream file(path,std::ios::binary);CHECK(file.good());
    return Bytes(std::istreambuf_iterator<char>(file),{});
}
static void write(const std::string& path,const Bytes& bytes)
{
    std::ofstream file(path,std::ios::binary|std::ios::trunc);CHECK(file.good());
    file.write((const char*)bytes.data(),bytes.size());file.close();CHECK(file.good());
}
static Snapshot snapshot(const std::string& path)
{
    Snapshot s;
    for(unsigned i=0;i<8;++i)s[i]=read(filename(path,i));
    return s;
}
static void clean(const std::string& path)
{
    for(unsigned i=0;i<8;++i)for(const char* suffix:{".rig.stage",".rig.rollback"})
        CHECK(GetFileAttributesA((filename(path,i)+suffix).c_str())==INVALID_FILE_ATTRIBUTES &&
              GetLastError()==ERROR_FILE_NOT_FOUND);
}
static void encode_facts(NativeFighterProject& p)
{
    uint8_t* data=(uint8_t*)std::malloc(NATIVE_SOURCE_FACT_BYTES);CHECK(data);
    size_t size=0;NativeScriptDiagnostic d={};
    CHECK(nativeSourceAssets_Write(&p.facts,data,NATIVE_SOURCE_FACT_BYTES,&size,&d));
    std::free(p.asset_record);p.asset_record=data;p.asset_record_size=size;
}
int main(int argc,char** argv)
{
    CHECK(argc==2);
    const std::string path=argv[1];
    NativeFighterProject p;
    CHECK(nativeFighterProject_Open(path.c_str(),&p));
    std::array<Bytes,4> initial;
    for(unsigned i=0;i<4;++i)initial[i]=read(filename(path,i));
    const Bytes note={0,1,2,3,255};
    write(path+"/asset-save-unrelated.txt",note);

    // Real simultaneous changes in all four assets: clip creation changes the
    // fighter descriptors and animation bank, rest data changes the costume,
    // and an owned gameplay option changes the facts record.
    CHECK(nativeFighterAnimation_CreateRest(&p,13,"AssetSaveCheck",30));
    auto* model=new NativeFighterModelEditor;
    CHECK(nativeFighterModel_Open(model,&p));CHECK(model->joint_count>6);
    CHECK(nativeFighterModel_SetJoint(model,6,2,0,model->joints[6].position[0]+0.125f));
    delete model;
    HSD_Dat fighter_dat;
    NativeFighterRig rig;
    char rig_error[192];
    CHECK(HSD_DatOpen(&fighter_dat,p.fighter_dat,p.fighter_size)==HSD_DAT_OK);
    CHECK(nativeFighterRig_Read(&fighter_dat,p.facts.joints+p.facts.omission_count,&rig,rig_error,sizeof(rig_error)));
    CHECK(nativeFighterRig_Write(&p,&rig,rig_error,sizeof(rig_error)));
    p.facts.ignore_surface_friction=!p.facts.ignore_surface_friction;
    encode_facts(p);
    char error[256]={};
    CHECK(nativeFighterProject_SaveAssets(path.c_str(),&p,error,sizeof(error)));
    const Snapshot saved=snapshot(path);
    for(unsigned i=0;i<4;++i){CHECK(saved[i]!=initial[i]);CHECK(saved[i+4]==initial[i]);}
    CHECK(read(path+"/asset-save-unrelated.txt")==note);clean(path);
    NativeFighterProject reopened;
    CHECK(nativeFighterProject_Open(path.c_str(),&reopened));
    CHECK(reopened.facts.ignore_surface_friction==p.facts.ignore_surface_friction);
    CHECK(nativeFighterProject_SaveAssets(path.c_str(),&reopened,error,sizeof(error)));
    CHECK(snapshot(path)==saved); // A no-op save retains the predecessor.
    nativeFighterProject_Close(&reopened);

    // A second valid candidate remains in memory throughout the failure matrix.
    p.fighter_dat[31]^=0x11;p.animation_dat[31]^=0x22;p.costume_dat[31]^=0x44;
    p.facts.ignore_surface_friction=!p.facts.ignore_surface_friction;
    encode_facts(p);
    for(unsigned i=0;i<8;++i){
        const auto name=filename(path,i);
        HANDLE lock=CreateFileA(name.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        CHECK(lock!=INVALID_HANDLE_VALUE);
        CHECK(!nativeFighterProject_SaveAssets(path.c_str(),&p,error,sizeof(error)) && *error);
        CHECK(CloseHandle(lock));
        CHECK(snapshot(path)==saved);clean(path);
    }
    for(const char* suffix:{".rig.stage",".rig.rollback"}){
        const auto name=filename(path,2)+suffix;
        write(name,note);
        CHECK(!nativeFighterProject_SaveAssets(path.c_str(),&p,error,sizeof(error)));
        CHECK(read(name)==note);CHECK(snapshot(path)==saved);
        CHECK(DeleteFileA(name.c_str()));clean(path);
    }
    p.facts.ignore_surface_friction=!p.facts.ignore_surface_friction;
    CHECK(!nativeFighterProject_SaveAssets(path.c_str(),&p,error,sizeof(error)));
    CHECK(snapshot(path)==saved); // In-memory facts disagree with serialized facts.
    p.facts.ignore_surface_friction=!p.facts.ignore_surface_friction;
    --p.animation_size;
    CHECK(!nativeFighterProject_SaveAssets(path.c_str(),&p,error,sizeof(error)));
    CHECK(snapshot(path)==saved);
    ++p.animation_size;
    char title[64];std::memcpy(title,p.title,sizeof(title));
    std::snprintf(p.title,sizeof(p.title),"Different draft");
    CHECK(!nativeFighterProject_SaveAssets(path.c_str(),&p,error,sizeof(error)));
    CHECK(snapshot(path)==saved);std::memcpy(p.title,title,sizeof(title));

    CHECK(nativeFighterProject_UndoLastAssetSave(path.c_str(),error,sizeof(error)));
    for(unsigned i=0;i<4;++i){CHECK(read(filename(path,i))==initial[i]);CHECK(read(filename(path,i+4))==saved[i]);}
    CHECK(nativeFighterProject_UndoLastAssetSave(path.c_str(),error,sizeof(error)));
    CHECK(snapshot(path)==saved);clean(path);
    CHECK(read(path+"/asset-save-unrelated.txt")==note);
    nativeFighterProject_Close(&p);
    std::printf("PASS %u multi-asset save, rollback, validation and undo checks\n",checks);
}
