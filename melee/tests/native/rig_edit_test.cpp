#undef NDEBUG
#include <melee/ft/native_fighter_rig_edit.h>
#include <melee/ft/native_fighter_rig.h>
#include <melee/ft/native_fighter_rest_matrices.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_skin_edit.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <memory>
#include <string>

static unsigned checks,cases;
static char error[256];
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s [%s]\n",__LINE__,#x,error);std::exit(1);}}while(0)
struct Snapshot {
    std::vector<uint8_t> data[4];NativeSourceAssetFacts facts;
    explicit Snapshot(const NativeFighterProject& p):facts(p.facts){
        const uint8_t* bytes[]={p.fighter_dat,p.animation_dat,p.costume_dat,p.asset_record};
        const size_t size[]={p.fighter_size,p.animation_size,p.costume_size,p.asset_record_size};
        for(unsigned n=0;n<4;++n)data[n].assign(bytes[n],bytes[n]+size[n]);
    }
    void same(const NativeFighterProject& p)const{
        const uint8_t* bytes[]={p.fighter_dat,p.animation_dat,p.costume_dat,p.asset_record};
        const size_t size[]={p.fighter_size,p.animation_size,p.costume_size,p.asset_record_size};
        CHECK(!std::memcmp(&facts,&p.facts,sizeof(facts)));
        for(unsigned n=0;n<4;++n)CHECK(data[n].size()==size[n]&&!std::memcmp(data[n].data(),bytes[n],size[n]));
    }
    void restore(NativeFighterProject& p)const{
        uint8_t** bytes[]={&p.fighter_dat,&p.animation_dat,&p.costume_dat,&p.asset_record};
        size_t* size[]={&p.fighter_size,&p.animation_size,&p.costume_size,&p.asset_record_size};
        for(unsigned n=0;n<4;++n){auto* copy=(uint8_t*)std::malloc(data[n].size());CHECK(copy);
            std::memcpy(copy,data[n].data(),data[n].size());std::free(*bytes[n]);*bytes[n]=copy;*size[n]=data[n].size();}
        p.facts=facts;
    }
};
static NativeFighterRig rig(const NativeFighterProject& p){
    HSD_Dat dat;NativeFighterRig r;CHECK(HSD_DatOpen(&dat,p.fighter_dat,p.fighter_size)==HSD_DAT_OK);
    CHECK(nativeFighterRig_Read(&dat,p.facts.joints+p.facts.omission_count,&r,error,sizeof(error)));return r;
}
static void failed(NativeFighterProject& p,const NativeFighterRigEdit& edit){
    Snapshot before(p);int32_t selected=-123;
    CHECK(!nativeFighterRig_Edit(&p,&edit,&selected,error,sizeof(error))&&selected==-123&&error[0]);before.same(p);++cases;
}
static void same_rest(const NativeFighterModelEditor& a,const NativeFighterModelEditor& b){
    std::unique_ptr<Mtx[]> old(new Mtx[a.joint_count]),now(new Mtx[b.joint_count]);
    CHECK(nativeFighterModel_RestMatrices(&a,old.get(),nullptr)&&nativeFighterModel_RestMatrices(&b,now.get(),nullptr));
    for(unsigned n=0;n<a.joint_count;++n){unsigned found=b.joint_count;
        for(unsigned m=0;m<b.joint_count;++m)if(a.joints[n].offset==b.joints[m].offset)found=m;
        if(found==b.joint_count)continue;
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
            CHECK(std::fabs(old[n][r][c]-now[found][r][c])<0.0002f*(1+std::fabs(old[n][r][c])));
    }
}
static std::vector<uint8_t> read(const char* path){
    FILE* f=std::fopen(path,"rb");CHECK(f&&!std::fseek(f,0,SEEK_END));long n=std::ftell(f);
    CHECK(n>0&&!std::fseek(f,0,SEEK_SET));std::vector<uint8_t> data((size_t)n);
    CHECK(std::fread(data.data(),1,data.size(),f)==data.size()&&!std::fclose(f));return data;
}
int main(int argc,char** argv){
    NativeFighterProject p={};CHECK(argc==2||argc==5);
    if(argc==2)CHECK(nativeFighterProject_Open(argv[1],&p));
    else {
        auto facts=read(argv[1]);NativeScriptDiagnostic diagnostic={};
        CHECK(nativeSourceAssets_Read(facts.data(),facts.size(),&p.facts,&diagnostic));
        std::snprintf(p.fighter_root,sizeof(p.fighter_root),"%s",p.facts.fighter_root);
        p.action_count=p.facts.actions;p.demo_count=p.facts.demo_count;
        std::vector<uint8_t> input[4]={read(argv[2]),read(argv[3]),read(argv[4]),std::move(facts)};
        uint8_t** buffers[]={&p.fighter_dat,&p.animation_dat,&p.costume_dat,&p.asset_record};
        size_t* sizes[]={&p.fighter_size,&p.animation_size,&p.costume_size,&p.asset_record_size};
        for(unsigned n=0;n<4;++n){*buffers[n]=(uint8_t*)std::malloc(input[n].size());CHECK(*buffers[n]);
            *sizes[n]=input[n].size();std::memcpy(*buffers[n],input[n].data(),input[n].size());}
    }
    CHECK(nativeFighterProject_ValidateAssets(&p,error,sizeof(error)));
    auto before=std::make_unique<NativeFighterModelEditor>(),after=std::make_unique<NativeFighterModelEditor>();
    CHECK(nativeFighterModel_Open(before.get(),&p));const auto source=rig(p);const unsigned original_count=p.facts.joints;
    NativeFighterRigEdit edit={NativeRigEdit_Remove,0,0,{}};failed(p,edit);
    edit={NativeRigEdit_Add,0,0,{}};std::snprintf(edit.name,sizeof(edit.name),"WorkshopExtra");int32_t id=-1;
    const uint32_t reservation=source.ids[source.count];
    char declaration[128];std::snprintf(declaration,sizeof(declaration),"\narticle RigEditReservation:\n    resources capacity 2 reserve_part %u\n",reservation);
    std::string script((const char*)p.script_text,p.script_size);script+=declaration;
    uint8_t* original_script=p.script_text;size_t original_script_size=p.script_size;
    p.script_text=(uint8_t*)script.data();p.script_size=script.size();
    CHECK(nativeFighterRig_Edit(&p,&edit,&id,error,sizeof(error))&&id==(int32_t)source.next_id);++cases;
    p.script_text=original_script;p.script_size=original_script_size;
    CHECK(p.facts.joints==original_count+1&&nativeFighterModel_Open(after.get(),&p));same_rest(*before,*after);
    auto with_extra=rig(p);const int part=nativeFighterRig_Resolve(&with_extra,id);
    CHECK(part>=0&&part<with_extra.count&&(with_extra.flags[part]&NATIVE_RIG_EXTRA)&&!std::strcmp(with_extra.names[part],edit.name));
    CHECK(nativeFighterRig_Id(&with_extra,0)==0&&nativeFighterRig_Resolve(&with_extra,reservation)>=with_extra.count);
    CHECK(!std::strcmp(after->joints[after->joint_count-1].name,"WorkshopExtra"));
    // Use the newly authored inverse bind in the real weight painter, then
    // verify deletion refuses that mesh reference without changing any asset.
    Snapshot unpainted(p);bool painted=false;
    for(unsigned mesh=0;mesh<after->mesh_count&&!painted;++mesh){
        unsigned corners,count=nativeFighterSkin_Vertices(&p,mesh,nullptr,0,nullptr,0,&corners);
        if(!count)continue;
        std::vector<NativeSkinVertex> vertices(count);
        CHECK(nativeFighterSkin_Vertices(&p,mesh,vertices.data(),count,nullptr,0,&corners)==count);
        NativeSkinBrushPoint point={{vertices[0].position[0],vertices[0].position[1],vertices[0].position[2]}};unsigned affected=0;
        CHECK(nativeFighterSkin_Paint(&p,mesh,after->joint_count-1,&point,1,0.25f,0.5f,false,&affected,error,sizeof(error))&&affected);
        CHECK(nativeFighterProject_ValidateAssets(&p,error,sizeof(error)));
        NativeFighterRigEdit remove={NativeRigEdit_Remove,id,0,{}};failed(p,remove);painted=true;
    }
    CHECK(painted);unpainted.restore(p);CHECK(nativeFighterModel_Open(after.get(),&p));++cases;
    failed(p,edit); // Duplicate name cannot consume another ID or publish data.
    edit={NativeRigEdit_Reparent,id,id,{}};failed(p,edit);
    edit={NativeRigEdit_Add,0,id,{}};std::snprintf(edit.name,sizeof(edit.name),"WorkshopChild");int32_t child=-1;
    CHECK(nativeFighterRig_Edit(&p,&edit,&child,error,sizeof(error)));++cases;
    // Give the added child a nontrivial local transform; deleting its parent
    // must retain the child's world rest pose and stable ID.
    CHECK(nativeFighterModel_Open(after.get(),&p));
    unsigned child_index=after->joint_count-1;
    CHECK(nativeFighterModel_SetJoint(after.get(),child_index,2,0,2.0f));
    CHECK(nativeFighterModel_SetJoint(after.get(),child_index,2,1,3.0f));
    CHECK(nativeFighterModel_SetJoint(after.get(),child_index,0,2,0.35f));
    CHECK(nativeFighterModel_Open(before.get(),&p));
    edit={NativeRigEdit_Remove,id,0,{}};int32_t selected=-1;
    CHECK(nativeFighterRig_Edit(&p,&edit,&selected,error,sizeof(error))&&selected==0);++cases;
    CHECK(p.facts.joints==original_count+1&&nativeFighterModel_Open(after.get(),&p));same_rest(*before,*after);
    auto remaining=rig(p);CHECK(nativeFighterRig_Resolve(&remaining,id)<0&&nativeFighterRig_Resolve(&remaining,child)>=0);
    edit={NativeRigEdit_Reparent,child,1,{}};
    CHECK(nativeFighterModel_Open(before.get(),&p));
    CHECK(nativeFighterRig_Edit(&p,&edit,&selected,error,sizeof(error))&&selected==child);++cases;
    CHECK(nativeFighterModel_Open(after.get(),&p));same_rest(*before,*after);
    edit={NativeRigEdit_Remove,child,0,{}};CHECK(nativeFighterRig_Edit(&p,&edit,&selected,error,sizeof(error)));++cases;
    CHECK(p.facts.joints==original_count&&nativeFighterProject_ValidateAssets(&p,error,sizeof(error)));
    auto restored=rig(p);CHECK(nativeFighterRig_Resolve(&restored,id)<0&&nativeFighterRig_Resolve(&restored,child)<0&&restored.next_id==source.next_id+2);
    std::printf("PASS %u cases %u checks\n",cases,checks);nativeFighterProject_Close(&p);return 0;
}
