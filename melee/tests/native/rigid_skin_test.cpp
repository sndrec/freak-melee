#include <melee/ft/native_fighter_skin_mesh.h>
#include <melee/ft/native_asset_dat.h>
#include <melee/ft/native_fighter_costume_compact.h>
#include <melee/ft/native_mesh_vertex_ids.h>
extern "C" {
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/list.h>
#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/pobj.h>
}
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>

namespace {
unsigned checks,cases;
float max_position_error,max_normal_error;
void check(bool ok,unsigned line=__builtin_LINE())
{++checks;if(!ok)throw std::runtime_error("rigid skin check at line "+std::to_string(line));}
void near(float a,float b,float* maximum)
{
    const float difference=std::fabs(a-b);*maximum=std::max(*maximum,difference);
    check(std::isfinite(a)&&std::isfinite(b)&&difference<.00005f);
}
struct Dat {
    std::vector<u8> data;
    std::vector<u32> links;
    u32 allocate(unsigned size){u32 at=((u32)data.size()+3)&~3u;data.resize(at+size);return at;}
    void word(u32 at,u32 value){NativeAssetDat::be32(data.data()+at,value);}
    void half(u32 at,u16 value){NativeAssetDat::be16(data.data()+at,value);}
    void real(u32 at,float value){u32 bits;std::memcpy(&bits,&value,4);word(at,bits);}
    void link(u32 at,u32 value){word(at,value);links.push_back(at);}
    std::vector<u8> finish(u32 root) {
        std::sort(links.begin(),links.end());
        std::vector<u8> result(32+data.size()+links.size()*4+12);
        NativeAssetDat::be32(result.data(),(u32)result.size());
        NativeAssetDat::be32(result.data()+4,(u32)data.size());
        NativeAssetDat::be32(result.data()+8,(u32)links.size());NativeAssetDat::be32(result.data()+12,1);
        std::memcpy(result.data()+32,data.data(),data.size());
        size_t tail=32+data.size();
        for(u32 at:links){NativeAssetDat::be32(result.data()+tail,at);tail+=4;}
        NativeAssetDat::be32(result.data()+tail,root);std::memcpy(result.data()+tail+8,"rig",4);
        return result;
    }
};
constexpr float positions[3][3]={{1,2,-3},{-2,.75f,1},{.5f,-1,2}};
constexpr float normals[3][9]={{.5f,.25f,-.75f, .2f,.4f,.1f, .1f,-.3f,.2f},
    {-.25f,.5f,.75f, .3f,.2f,-.1f, -.2f,.4f,.1f},
    {.75f,-.5f,.25f, -.2f,.1f,.3f, .4f,.1f,-.1f}};
struct Fixture {std::vector<u8> file;u32 bones[4],mesh,polygon,display;unsigned owner,secondary,components;};
Fixture fixture(unsigned owner,int secondary,unsigned indexing,bool nbt,bool helper,bool no_root_bind)
{
    Dat d;Fixture f={};f.owner=owner;f.secondary=secondary<0?owner:(unsigned)secondary;f.components=nbt?9:3;
    for(auto& bone:f.bones)bone=d.allocate(64);
    d.link(f.bones[0]+8,f.bones[1]);d.link(f.bones[1]+8,f.bones[2]);d.link(f.bones[1]+12,f.bones[3]);
    for(unsigned i=0;i<4;++i) {
        d.word(f.bones[i]+4,8|(i==0?JOBJ_SKELETON_ROOT:i==2&&helper?0:JOBJ_SKELETON));
        for(unsigned a=0;a<3;++a)d.real(f.bones[i]+32+a*4,1);
        if(i==0&&no_root_bind)continue;
        const u32 bind=d.allocate(48);d.link(f.bones[i]+56,bind);
        for(unsigned a=0;a<3;++a)d.real(bind+(a*4+a)*4,1+.1f*(i+a));
        d.real(bind+4,.15f*(i+1));d.real(bind+12,-1.25f*(i+1));
        d.real(bind+28,.5f*i);d.real(bind+44,-.25f*i);
    }
    f.mesh=d.allocate(16);f.polygon=d.allocate(24);d.link(f.bones[owner]+16,f.mesh);d.link(f.mesh+12,f.polygon);
    if(secondary>=0)d.link(f.polygon+20,f.bones[secondary]);
    const u32 vertices=d.allocate(36),vectors=d.allocate(3*f.components*4),uv=d.allocate(24);
    for(unsigned i=0;i<3;++i) {
        for(unsigned a=0;a<3;++a)d.real(vertices+(i*3+a)*4,positions[i][a]);
        for(unsigned a=0;a<f.components;++a)d.real(vectors+(i*f.components+a)*4,normals[i][a]);
        d.real(uv+i*8,.25f*i);d.real(uv+i*8+4,.125f*i);
    }
    const u32 attrs=d.allocate(6*24);
    const unsigned order[]={13,11,10,9,0};
    for(unsigned i=0;i<5;++i) {
        const unsigned attr=order[i],at=attrs+i*24;
        d.word(at,attr);d.word(at+4,attr==0||attr==11?1:indexing);
        d.word(at+8,attr==9||attr==11||attr==13?1:nbt?1:0);
        d.word(at+12,attr==11?5:4);
        if(attr==9||attr==10||attr==13) {
            d.half(at+18,(u16)(attr==9?12:attr==10?f.components*4:8));
            d.link(at+20,attr==9?vertices:attr==10?vectors:uv);
        }
    }
    d.word(attrs+5*24,255);
    std::vector<u8> commands={0x90,0,6};
    for(unsigned corner=0;corner<6;++corner) {
        commands.push_back((u8)(secondary>=0&&corner%2?3:0));
        for(unsigned attr:{9u,10u}) {
            (void)attr;if(indexing==3)commands.push_back(0);commands.push_back((u8)(corner/2));
        }
        commands.insert(commands.end(),{(u8)(20+corner),40,60,255});
        if(indexing==3)commands.push_back(0);commands.push_back((u8)(corner/2));
    }
    commands.resize((commands.size()+31)&~size_t(31));f.display=d.allocate((unsigned)commands.size());
    std::memcpy(d.data.data()+f.display,commands.data(),commands.size());
    d.link(f.polygon+8,attrs);d.half(f.polygon+14,(u16)(commands.size()/32));d.link(f.polygon+16,f.display);
    f.file=d.finish(f.bones[0]);return f;
}
unsigned word(const NativeFighterModelEditor& model,unsigned at)
{u32 result;check(HSD_DatReadU32(&model.dat,at,&result));return result;}
int bone_index(const NativeFighterModelEditor& model,u32 offset)
{for(unsigned i=0;i<model.joint_count;++i)if(model.joints[i].offset==offset)return i;return -1;}
void pose(HSD_JObj joints[4],const NativeFighterModelEditor& model,Mtx binds[4],unsigned tick)
{
    for(unsigned i=0;i<4;++i) {
        joints[i]={};joints[i].flags=model.joints[i].flags;
        const int parent=model.joints[i].parent;joints[i].parent=parent<0?nullptr:&joints[parent];
        const float angle=.07f*(tick+2*i),c=std::cos(angle),s=std::sin(angle);
        C_MTXIdentity(joints[i].mtx);
        joints[i].mtx[0][0]=c*(1+.1f*i);joints[i].mtx[0][1]=-s*(.75f+.02f*tick);
        joints[i].mtx[1][0]=s*(1+.1f*i);joints[i].mtx[1][1]=c*(.75f+.02f*tick);
        joints[i].mtx[2][2]=1.1f+.03f*i;
        joints[i].mtx[0][3]=2*i+.25f*tick;joints[i].mtx[1][3]=1-i*.5f;joints[i].mtx[2][3]=.3f*i;
        check(nativeSkinMesh_InverseBind(model,i,binds[i]));joints[i].envelopemtx=binds[i];
    }
}
// Uses the actual runtime corner query, including HSD's model-node matrix.
std::array<Vec3,6> runtime_positions(const NativeFighterModelEditor& model,unsigned tick)
{
    HSD_JObj joints[4];Mtx binds[4];pose(joints,model,binds,tick);
    const auto& selected=model.meshes[0];const u32 polygon=word(model,selected.offset+12);
    std::vector<u8> arena(1024*1024);HSD_DatImport importer;
    check(HSD_DatImportInit(&importer,&model.dat,arena.data(),arena.size(),1024)==HSD_DAT_OK);
    auto* descriptor=(HSD_PObjDesc*)HSD_DatImportGraph(&importer,polygon,DAT_POBJ);
    check(descriptor);
    HSD_PObj p={};p.verts=descriptor->verts;p.flags=descriptor->flags;
    p.n_display=descriptor->n_display;p.display=descriptor->display;
    HSD_Envelope envelopes[10][17]={};HSD_SList lists[10]={};
    if((p.flags&0x3000)==POBJ_ENVELOPE) {
        const u32 table=word(model,polygon+20);
        for(unsigned slot=0;slot<10&&word(model,table+slot*4);++slot) {
            u32 at=word(model,table+slot*4);lists[slot].data=&envelopes[slot][0];
            if(slot)lists[slot-1].next=&lists[slot];
            for(unsigned i=0;i<17;++i,at+=8) {
                u32 target;const char* external;
                const auto kind=HSD_DatResolve(&model.dat,at,&target,&external);
                if(kind==HSD_DAT_REF_NULL)break;
                check(kind==HSD_DAT_REF_LOCAL);
                const int bone=bone_index(model,target);check(bone>=0);
                envelopes[slot][i].jobj=&joints[bone];check(HSD_DatReadF32(&model.dat,at+4,&envelopes[slot][i].weight));
                if(i)envelopes[slot][i-1].next=&envelopes[slot][i];
            }
        }
        p.u.envelope_list=lists;
    } else {
        u32 other;const char* external;
        const auto reference=HSD_DatResolve(&model.dat,polygon+20,&other,&external);
        if(reference==HSD_DAT_REF_LOCAL){const int bone=bone_index(model,other);check(bone>=0);p.u.jobj=&joints[bone];}
        else check(reference==HSD_DAT_REF_NULL);
    }
    HSD_DObj mesh={};mesh.pobj=&p;std::array<Vec3,6> points;
    nativeModelView_Begin(1,true);nativeModelView_Mesh(0,&joints[selected.joint],&mesh,false,false,nullptr);
    check(nativeModelView_CornerPositions(0,points.data(),(unsigned)points.size()));nativeModelView_Clear();
    return points;
}
void normal_matrix(Mtx matrix,const float* normal,float* output)
{
    Mtx inv;check(C_MTXInverse(matrix,inv));
    for(unsigned r=0;r<3;++r){output[r]=0;for(unsigned c=0;c<3;++c)output[r]+=inv[c][r]*normal[c];}
}
void compare_normals(const NativeFighterModelEditor& model,const NativeSkinMesh& skin,
                     const Fixture& source,unsigned tick)
{
    HSD_JObj joints[4];Mtx binds[4];pose(joints,model,binds,tick);
    Mtx right;C_MTXIdentity(right);
    if(!(joints[source.owner].flags&JOBJ_SKELETON_ROOT)) {
        int ancestor=source.owner;while(!(joints[ancestor].flags&3))ancestor=model.joints[ancestor].parent;
        Mtx base,inv;
        if((unsigned)ancestor==source.owner)check(C_MTXInverse(binds[ancestor],right));
        else {
            if(joints[ancestor].flags&JOBJ_SKELETON_ROOT)C_MTXCopy(joints[ancestor].mtx,base);
            else C_MTXConcat(joints[ancestor].mtx,binds[ancestor],base);
            check(C_MTXInverse(base,inv));C_MTXConcat(inv,joints[source.owner].mtx,right);
        }
    }
    const auto& part=skin.parts[0];check(part.corners.size()==6);
    for(unsigned corner=0;corner<6;++corner) {
        const auto& c=part.corners[corner];const auto& weights=skin.vertices[c.vertex].skin;
        Mtx blended={};
        for(unsigned i=0;i<weights.count;++i) {
            Mtx effective;C_MTXConcat(joints[weights.joints[i]].mtx,binds[weights.joints[i]],effective);
            for(unsigned r=0;r<3;++r)for(unsigned col=0;col<4;++col)blended[r][col]+=weights.weights[i]*effective[r][col];
        }
        Mtx effective;C_MTXConcat(blended,right,effective);
        for(unsigned n=0;n<source.components;n+=3) {
            float actual[3],expected[3];normal_matrix(effective,c.normal+n,actual);
            normal_matrix(joints[corner%2?source.secondary:source.owner].mtx,normals[corner/2]+n,expected);
            for(unsigned a=0;a<3;++a)near(actual[a],expected[a],&max_normal_error);
        }
    }
}
void run(unsigned owner,int secondary,unsigned indexing,bool nbt,bool helper,bool no_root_bind)
{
    Fixture f=fixture(owner,secondary,indexing,nbt,helper,no_root_bind);
    NativeFighterProject project={};project.costume_dat=(u8*)std::malloc(f.file.size());check(project.costume_dat);
    std::memcpy(project.costume_dat,f.file.data(),f.file.size());project.costume_size=f.file.size();
    std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    auto before=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(before.get(),&project));
    NativeSkinMesh skin;char error[256]={};check(nativeSkinMesh_Read(*before,0,&skin,error,sizeof(error)));
    NativeAssetDat dat;check(dat.open(project.costume_dat,project.costume_size));check(nativeSkinMesh_Write(dat,*before,0,skin));
    std::vector<u8> rewritten;check(dat.finish(&rewritten));
    NativeFighterProject view=project;view.costume_dat=rewritten.data();view.costume_size=rewritten.size();
    auto after=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(after.get(),&view));
    NativeSkinMesh reloaded;check(nativeSkinMesh_Read(*after,0,&reloaded,error,sizeof(error)));
    for(unsigned tick=0;tick<19;++tick) {
        const auto a=runtime_positions(*before,tick),b=runtime_positions(*after,tick);
        for(unsigned i=0;i<6;++i)for(unsigned axis=0;axis<3;++axis)
            near((&a[i].x)[axis],(&b[i].x)[axis],&max_position_error);
        compare_normals(*after,reloaded,f,tick);
    }
    NativeSkinBrushPoint center;std::memcpy(center.position,skin.vertices[0].position,sizeof(center.position));
    unsigned affected;check(nativeFighterSkin_Paint(&project,0,3,&center,1,.001f,.25f,false,&affected,error,sizeof(error)));
    check(affected>0);check(nativeFighterModel_Open(after.get(),&project));
    NativeSkinMesh painted;check(nativeSkinMesh_Read(*after,0,&painted,error,sizeof(error)));
    check(std::fabs(nativeSkinWeights_Get(&painted.vertices[0].skin,3)-.25f)<.000001f);
    std::free(project.costume_dat);++cases;
}
void rejection()
{
    Fixture f=fixture(2,3,3,false,true,false);
    NativeFighterProject project={};project.costume_dat=f.file.data();project.costume_size=f.file.size();
    std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    NativeSkinMesh skin;char error[256];const auto original=f.file;const auto* pointer=project.costume_dat;
    check(!nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
    const NativeSkinBrushPoint center={{1,2,3}};unsigned affected=99;
    check(!nativeFighterSkin_Paint(&project,0,3,&center,1,1,.25f,false,&affected,error,sizeof(error)));
    check(!affected && project.costume_dat==pointer && f.file==original);
}

void large_mesh(bool helper)
{
    Fixture f=fixture(helper?2:0,-1,3,false,helper,false);
    NativeFighterProject project={};
    std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    auto publish=[&](const std::vector<u8>& bytes) {
        auto* owned=(u8*)std::malloc(bytes.size());check(owned);
        std::memcpy(owned,bytes.data(),bytes.size());std::free(project.costume_dat);
        project.costume_dat=owned;project.costume_size=bytes.size();
    };
    publish(f.file);
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    NativeSkinMesh skin;char error[256]={};check(nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
    const NativeSkinVertex prototype=skin.vertices[0];
    const NativeSkinCorner corner=skin.parts[0].corners[0];
    constexpr unsigned vertices=70002;
    skin.vertices.resize(vertices,prototype);skin.parts.resize(1);
    auto& part=skin.parts[0];part.corners.clear();part.triangles.clear();skin.corner_vertices.clear();
    for(unsigned i=0;i<vertices;++i) {
        skin.vertices[i].position[0]=i*.01f;skin.vertices[i].position[1]=(i%5)*.01f;skin.vertices[i].position[2]=0;
        auto current=corner;current.vertex=i;part.corners.push_back(current);part.triangles.push_back(i);
    }
    // Coincident independent vertices remain distinct; shared vertices across
    // sections remain one vertex. The final triangle spans far-apart indices.
    std::memcpy(skin.vertices[vertices-1].position,skin.vertices[vertices-2].position,sizeof(prototype.position));
    for(unsigned id:{0u,65536u,vertices-1}) {
        auto current=corner;current.vertex=id;part.triangles.push_back((unsigned)part.corners.size());part.corners.push_back(current);
    }
    NativeAssetDat dat;check(dat.open(project.costume_dat,project.costume_size));
    check(nativeSkinMesh_Write(dat,*model,0,skin));std::vector<u8> bytes;check(dat.finish(&bytes));
    if(helper) {
        check(NativeAssetDat::read32(bytes.data()+12)==2);
        const size_t roots=32+NativeAssetDat::read32(bytes.data()+4)+size_t(NativeAssetDat::read32(bytes.data()+8))*4;
        for(unsigned i=0;i<8;++i)std::swap(bytes[roots+i],bytes[roots+8+i]);
    }
    NativeCostumeCompaction stats;
    check(nativeFighterCostume_Compact(&project,&bytes,&stats,error,sizeof(error)));check(stats.supported);
    publish(bytes);check(nativeFighterModel_Open(model.get(),&project));
    NativeSkinMesh reloaded;check(nativeSkinMesh_Read(*model,0,&reloaded,error,sizeof(error)));
    check(reloaded.vertices.size()==vertices && reloaded.parts.size()==2 && reloaded.corner_vertices.size()==vertices+3);
    for(unsigned i=0;i<vertices;++i) {
        check(!std::memcmp(reloaded.vertices[i].position,skin.vertices[i].position,sizeof(prototype.position)));
        check(nativeSkinWeights_Equal(&reloaded.vertices[i].skin,&prototype.skin));
    }
    check(reloaded.corner_vertices[vertices]==0 && reloaded.corner_vertices[vertices+1]==65536 &&
        reloaded.corner_vertices[vertices+2]==vertices-1);
    const size_t initial_size=project.costume_size;
    std::vector<NativeMeshVertexIdRow> rows;check(nativeMeshVertexIds_Read(model->dat,&rows));check(rows.size()==2);
    if(helper) {
        const std::vector<u8> previous(project.costume_dat,project.costume_dat+project.costume_size);
        const u32 old_polygon=rows[0].polygon;
        check(nativeFighterModel_SetJoint(model.get(),2,2,0,2.5f));
        check(nativeFighterModel_CompensateRest(model.get(),previous.data(),previous.size(),2));
        check(nativeMeshVertexIds_Read(model->dat,&rows));check(rows.size()==2 && rows[0].polygon!=old_polygon);
        check(nativeSkinMesh_Read(*model,0,&reloaded,error,sizeof(error)));
        check(reloaded.vertices.size()==vertices && reloaded.corner_vertices[vertices]==0);
    }
    NativeSkinBrushPoint center;std::memcpy(center.position,reloaded.vertices[0].position,sizeof(center.position));
    for(unsigned stroke=0;stroke<4;++stroke) {
        unsigned affected=0;
        check(nativeFighterSkin_Paint(&project,0,3,&center,1,.001f,.1f,false,&affected,error,sizeof(error)));
        check(affected==1 && nativeFighterModel_Open(model.get(),&project));
        check(nativeSkinMesh_Read(*model,0,&reloaded,error,sizeof(error)));
        check(reloaded.vertices.size()==vertices && reloaded.corner_vertices[vertices]==0 &&
            reloaded.corner_vertices[vertices+1]==65536);
        check(std::fabs(nativeSkinWeights_Get(&reloaded.vertices[0].skin,3)-.1f*(stroke+1))<.000001f);
        check(nativeMeshVertexIds_Read(model->dat,&rows));check(rows.size()==2);
        check(project.costume_size<initial_size+8192);
    }
    // Corrupt the identity root's table reference. Neither reading nor
    // compaction may publish a partial repair of malformed authoring data.
    bytes.assign(project.costume_dat,project.costume_dat+project.costume_size);
    u32 header;check(HSD_DatFindRoot(&model->dat,NATIVE_MESH_VERTEX_IDS_ROOT,&header));
    NativeAssetDat::be32(bytes.data()+32+header+4,(uint32_t)model->dat.data_size);
    HSD_Dat invalid;check(HSD_DatOpen(&invalid,bytes.data(),bytes.size())==HSD_DAT_OK);
    check(!nativeMeshVertexIds_Read(invalid,&rows));const auto corrupt=bytes;
    check(!nativeFighterCostume_Compact(&project,&bytes,nullptr,error,sizeof(error)) && bytes==corrupt);
    // Retiring a draw section also retires its ID array. A subsequently small
    // rewrite can use ordinary shared position indices and drop the IDs.
    NativeAssetDat removed;check(removed.open(project.costume_dat,project.costume_size));
    const u32 head=word(*model,model->meshes[0].offset+12);
    removed.link(model->meshes[0].offset+12,word(*model,head+4));check(removed.finish(&bytes));
    check(nativeFighterCostume_Compact(&project,&bytes,&stats,error,sizeof(error)) && stats.supported);
    publish(bytes);check(nativeFighterModel_Open(model.get(),&project));
    check(nativeMeshVertexIds_Read(model->dat,&rows));check(rows.size()==1);
    check(nativeSkinMesh_Read(*model,0,&reloaded,error,sizeof(error)) && reloaded.vertices.size()==4471);
    std::memcpy(center.position,reloaded.vertices[0].position,sizeof(center.position));unsigned affected=0;
    check(nativeFighterSkin_Paint(&project,0,3,&center,1,.001f,.1f,false,&affected,error,sizeof(error)));
    check(affected==1 && nativeFighterModel_Open(model.get(),&project));
    check(nativeMeshVertexIds_Read(model->dat,&rows));check(rows.empty());
    check(nativeSkinMesh_Read(*model,0,&reloaded,error,sizeof(error)) && reloaded.vertices.size()==4471);
    check(project.costume_size<initial_size/4);
    std::printf("large mesh helper=%u vertices=%u corners=%zu bytes=%zu\n",helper,vertices,reloaded.corner_vertices.size(),project.costume_size);
    std::free(project.costume_dat);++cases;
}
}
int main()
{
    try {
        for(unsigned indexing:{2u,3u})for(bool nbt:{false,true}) {
            for(unsigned owner:{0u,1u})for(int secondary:{-1,0,1,2,3})
                run(owner,secondary,indexing,nbt,false,false);
            run(0,2,indexing,nbt,false,true);
            run(2,2,indexing,nbt,true,false);
            run(2,-1,indexing,nbt,true,false);
        }
        rejection();
        large_mesh(false);large_mesh(true);
        std::printf("PASS: %u rigid skin cases, %u checks; max position=%g normal=%g\n",cases,checks,max_position_error,max_normal_error);
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
