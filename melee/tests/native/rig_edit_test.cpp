#undef NDEBUG
#include <melee/ft/native_fighter_rig_edit.h>
#include <melee/ft/native_fighter_rig.h>
#include <melee/ft/native_fighter_rest_matrices.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_fighter_skin_edit.h>
#include <melee/ft/native_fighter_action_editor.h>
#include <melee/ft/native_asset_dat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <memory>
#include <string>

static unsigned checks,cases;
static char error[256];
extern "C" bool rig_test_prepare_source(const NativeFighterProject*,char*,size_t);
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
static uint32_t pointer(const HSD_Dat& dat,uint32_t slot){
    u32 value=0;const char* name;auto kind=HSD_DatResolve(&dat,slot,&value,&name);
    CHECK(kind==HSD_DAT_REF_LOCAL||kind==HSD_DAT_REF_NULL);
    return kind==HSD_DAT_REF_NULL?UINT32_MAX:(uint32_t)value;
}
static uint32_t word(const HSD_Dat& dat,uint32_t slot){u32 value;CHECK(HSD_DatReadU32(&dat,slot,&value));return value;}
static HSD_Dat archive(const NativeFighterProject& p){
    HSD_Dat dat;CHECK(HSD_DatOpen(&dat,p.fighter_dat,p.fighter_size)==HSD_DAT_OK);return dat;
}
static std::vector<uint32_t> metal_joints(const NativeFighterProject& p){
    auto dat=archive(p);u32 root;CHECK(HSD_DatFindRoot(&dat,p.fighter_root,&root));
    std::vector<uint32_t> result;
    auto walk=[&](auto&& self,uint32_t node,unsigned depth)->void{
        CHECK(depth<30);
        for(;node!=UINT32_MAX;node=pointer(dat,node+12)){
            CHECK(result.size()<MAX_FT_PARTS&&std::find(result.begin(),result.end(),node)==result.end());
            result.push_back(node);self(self,pointer(dat,node+8),depth+1);
        }
    };walk(walk,pointer(dat,root+92),0);return result;
}
static std::vector<uint32_t> metal_draws(const NativeFighterProject& p,const std::vector<uint32_t>& joints){
    auto dat=archive(p);std::vector<uint32_t> result;
    for(uint32_t joint:joints){
        if(word(dat,joint+4)&((1u<<5)|(1u<<14)))continue;
        for(uint32_t mesh=pointer(dat,joint+16);mesh!=UINT32_MAX;mesh=pointer(dat,mesh+4)){
            CHECK(result.size()<32);result.push_back(mesh);
        }
    }return result;
}
static std::vector<uint32_t> metal_selected(const NativeFighterProject& p,const std::vector<uint32_t>& draws){
    auto dat=archive(p);u32 root;CHECK(HSD_DatFindRoot(&dat,p.fighter_root,&root));
    uint32_t model=pointer(dat,root+8);if(model==UINT32_MAX)model=0;
    unsigned groups=word(dat,model);uint32_t rows=pointer(dat,model+4),end=HSD_DatNextTarget(&dat,rows);
    CHECK(groups<=11&&end>=rows&&(end-rows)%16==0);
    std::vector<uint32_t> selected;
    for(uint32_t row=rows;row<end;row+=16){
        uint32_t lookup=pointer(dat,row+8);selected.push_back(lookup==UINT32_MAX?UINT32_MAX:0);
        if(lookup==UINT32_MAX)continue;
        for(unsigned g=0;g<groups;++g){
            unsigned variants=word(dat,lookup+g*8);uint32_t list=pointer(dat,lookup+g*8+4);
            CHECK(variants<=256);selected.push_back(variants);
            for(unsigned v=0;v<variants;++v){
                unsigned count=word(dat,list+v*8);uint32_t ids=pointer(dat,list+v*8+4);
                CHECK(count<=256);selected.push_back(count);
                const uint8_t* bytes=HSD_DatBytes(&dat,ids,count);CHECK(!count||bytes);
                for(unsigned i=0;i<count;++i){CHECK(bytes[i]<draws.size());selected.push_back(draws[bytes[i]]);}
            }
        }
    }return selected;
}
static void metal_reparent(NativeFighterProject& p,int32_t id,const NativeFighterModelEditor& model){
    auto nodes=metal_joints(p);if(nodes.empty())return;
    CHECK(nodes.size()==model.joint_count);
    const auto dat=archive(p);int last=-1,destination=-1;
    for(unsigned n=0;n<nodes.size();++n)
        if(!(word(dat,nodes[n]+4)&((1u<<5)|(1u<<14)))&&pointer(dat,nodes[n]+16)!=UINT32_MAX)last=(int)n;
    // Put the last draw chain on the authored extra bone, keeping its current
    // ordinal. Then move it before a different chain to exercise real reordering.
    for(int n=1;n<last&&destination<0;++n){
        if(model.joints[n+1].parent==n)continue;
        for(int j=n+1;j<last;++j)
            if(!(word(dat,nodes[j]+4)&((1u<<5)|(1u<<14)))&&pointer(dat,nodes[j]+16)!=UINT32_MAX){destination=n;break;}
    }
    if(destination<0)return; // Some models have only one indivisible draw chain.
    Snapshot original(p);const auto draws=metal_draws(p,nodes);
    NativeAssetDat edit;CHECK(edit.open(p.fighter_dat,p.fighter_size));
    edit.link_local(nodes.back()+16,pointer(dat,nodes[last]+16));edit.link(nodes[last]+16,0);
    std::vector<uint8_t> bytes;CHECK(edit.finish(&bytes));
    auto* replacement=(uint8_t*)std::malloc(bytes.size());CHECK(replacement);
    std::memcpy(replacement,bytes.data(),bytes.size());std::free(p.fighter_dat);p.fighter_dat=replacement;p.fighter_size=bytes.size();
    CHECK(nativeFighterProject_ValidateAssets(&p,error,sizeof(error)));
    CHECK(metal_draws(p,metal_joints(p))==draws);const auto selected=metal_selected(p,draws);
    auto owned=rig(p);unsigned body=0,ordinal=0;
    for(;body<owned.count;++body){bool omitted=false;
        for(unsigned h=0;h<p.facts.omission_count;++h)omitted|=p.facts.omissions[h].joint==body;
        if(!omitted){if(ordinal==(unsigned)destination)break;++ordinal;}
    }
    CHECK(body<owned.count);
    NativeFighterRigEdit reparent={NativeRigEdit_Reparent,id,(int32_t)owned.ids[body],{}};
    int32_t result=-1;CHECK(nativeFighterRig_Edit(&p,&reparent,&result,error,sizeof(error))&&result==id);
    const auto reordered=metal_draws(p,metal_joints(p));CHECK(reordered!=draws);
    CHECK(metal_selected(p,reordered)==selected);++cases;
    NativeFighterRigEdit remove={NativeRigEdit_Remove,id,0,{}};failed(p,remove);
    original.restore(p);
}
int main(int argc,char** argv){
    NativeFighterProject p={};CHECK(argc==2||argc==5||argc==6);
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
    if(argc==6){
        CHECK(!std::strcmp(argv[5],"--normalize-intervals"));unsigned owned=0,converted=0;
        CHECK(nativeFighterAction_OwnHitboxFlags(&p,&owned,error,sizeof(error)));
        nativeFighterAction_ConvertAllHitboxIntervals(&p,&converted);
        bool admitted=nativeFighterProject_ValidateAssets(&p,error,sizeof(error));
        if(!admitted){
            std::string path=std::string(argv[2])+".normalized";FILE* file=std::fopen(path.c_str(),"wb");
            CHECK(file&&std::fwrite(p.fighter_dat,1,p.fighter_size,file)==p.fighter_size&&!std::fclose(file));
        }
        CHECK(admitted);++cases;
    }
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
    metal_reparent(p,id,*after);CHECK(nativeFighterModel_Open(after.get(),&p));
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
    if(p.script_size){CHECK(rig_test_prepare_source(&p,error,sizeof(error)));++cases;}
    edit={NativeRigEdit_Remove,child,0,{}};CHECK(nativeFighterRig_Edit(&p,&edit,&selected,error,sizeof(error)));++cases;
    CHECK(p.facts.joints==original_count&&nativeFighterProject_ValidateAssets(&p,error,sizeof(error)));
    auto restored=rig(p);CHECK(nativeFighterRig_Resolve(&restored,id)<0&&nativeFighterRig_Resolve(&restored,child)<0&&restored.next_id==source.next_id+2);
    std::printf("PASS %u cases %u checks\n",cases,checks);nativeFighterProject_Close(&p);return 0;
}
