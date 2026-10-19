#include <melee/ft/native_fighter_skin_mesh.h>
#include <melee/ft/native_asset_dat.h>
#include <melee/ft/native_fighter_costume_compact.h>
#include <melee/ft/native_mesh_vertex_ids.h>
extern "C" {
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/displayfunc.h>
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
    HSD_EnvelopeMatrices matrix_sets={lists,nullptr};
    if((p.flags&0x3000)==POBJ_ENVELOPE || (p.flags&0x3000)==POBJ_SEPARATE_ENVELOPES) {
        const bool separate = (word(model,polygon+12)>>16&0x3000)==POBJ_SEPARATE_ENVELOPES;
        const u32 metadata=word(model,polygon+20);
        const u32 table=separate?word(model,metadata):metadata;
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
        if(separate)p.u.matrices=&matrix_sets;else p.u.envelope_list=lists;
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
    NativeAssetDat dat;check(dat.open(project.costume_dat,project.costume_size));uint32_t polygon;
    check(nativeSkinMesh_Encode(dat,*before,skin,&polygon));dat.link(before->meshes[0].offset+12,polygon);
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

Fixture texture_fixture(unsigned owner,unsigned indexing,bool nbt,bool distinct_basis=false)
{
    auto f=fixture(owner,-1,indexing,nbt,owner==2,false);
    NativeAssetDat dat;check(dat.open(f.file.data(),f.file.size()));
    const u32 old=NativeAssetDat::read32(dat.data.data()+f.polygon+8);
    const u32 attrs=dat.allocate(9*24);
    std::memcpy(dat.data.data()+attrs,dat.data.data()+old,5*24);
    for(unsigned i=0;i<5;++i)
        if(NativeAssetDat::read32(dat.data.data()+attrs+i*24+4)>GX_DIRECT)
            dat.link_local(attrs+i*24+20,NativeAssetDat::read32(dat.data.data()+attrs+i*24+20));
    for(unsigned i=0;i<3;++i) {
        dat.word(attrs+(5+i)*24,i==2?8:i+1);dat.word(attrs+(5+i)*24+4,GX_DIRECT);
    }
    dat.word(attrs+8*24,GX_VA_NULL);dat.link(f.polygon+8,attrs);
    const u32 table=dat.allocate(4*4);dat.link(f.polygon+20,table);
    dat.half(f.polygon+12,POBJ_ENVELOPE);
    for(unsigned slot=0;slot<3;++slot) {
        const u32 entries=dat.allocate(3*8);dat.link(table+slot*4,entries);
        for(unsigned i=0;i<2;++i) {
            dat.link_local(entries+i*8,f.bones[slot+i]);
            const float value=distinct_basis&&slot==0 ? (i?0.0f:1.0f) : i?.75f:.25f;
            u32 bits;std::memcpy(&bits,&value,4);dat.word(entries+i*8+4,bits);
        }
    }
    std::vector<u8> bytes={GX_TRIANGLES,0,6};
    for(unsigned corner=0;corner<6;++corner) {
        bytes.push_back((u8)(corner%3*3));
        bytes.push_back((u8)(30+(corner+1)%3*3)); // Independent reflected normal.
        bytes.push_back((u8)(30+corner%3*3)); // Follows position weights.
        bytes.push_back(60); // Identity.
        for(unsigned attr:{9u,10u}) {
            (void)attr;if(indexing==GX_INDEX16)bytes.push_back(0);bytes.push_back((u8)(corner/2));
        }
        bytes.insert(bytes.end(),{(u8)(20+corner),40,60,255});
        if(indexing==GX_INDEX16)bytes.push_back(0);bytes.push_back((u8)(corner/2));
    }
    bytes.resize((bytes.size()+31)&~size_t(31));f.display=dat.allocate(bytes.size(),32);
    std::memcpy(dat.data.data()+f.display,bytes.data(),bytes.size());
    dat.link(f.polygon+16,f.display);dat.half(f.polygon+14,(u16)(bytes.size()/32));
    std::vector<u8> file;check(dat.finish(&file));f.file=std::move(file);return f;
}

// Independently read the saved selectors/envelopes, then apply the runtime's
// model-node transform to animated reflected normals. No editable texture tags.
std::vector<float> texture_vectors(const NativeFighterModelEditor& model,unsigned tick)
{
    HSD_JObj joints[4];Mtx binds[4];pose(joints,model,binds,tick);
    Mtx right_storage;MtxPtr right=_HSD_mkEnvelopeModelNodeMtx(&joints[model.meshes[0].joint],right_storage);
    std::vector<float> result;
    for(u32 polygon=word(model,model.meshes[0].offset+12);polygon;polygon=word(model,polygon+4)) {
        std::vector<NativeSkinAttribute> attrs;check(nativeSkinMesh_ReadAttributes(model.dat,word(model,polygon+8),&attrs));
        const bool separate=(word(model,polygon+12)>>16&0x3000)==POBJ_SEPARATE_ENVELOPES;
        const u32 metadata=word(model,polygon+20);
        const u32 table=separate?word(model,metadata+4):metadata;
        const u32 bases=separate?word(model,metadata+20):0;
        const unsigned constants=separate?(word(model,metadata+16)&0xffff):0;
        std::vector<std::array<float,12>> matrices;
        for(unsigned slot=0;word(model,table+slot*4);++slot) {
            Mtx matrix={};
            for(u32 entry=word(model,table+slot*4);;entry+=8) {
                u32 bone;const char* external;
                const auto kind=HSD_DatResolve(&model.dat,entry,&bone,&external);
                if(kind==HSD_DAT_REF_NULL)break;
                check(kind==HSD_DAT_REF_LOCAL);const int index=bone_index(model,bone);check(index>=0);
                float weight;check(HSD_DatReadF32(&model.dat,entry+4,&weight));
                const bool shortcut=!separate&&!right&&weight>=1.0f;
                Mtx effective;
                if(shortcut)C_MTXCopy(joints[index].mtx,effective);
                else C_MTXConcat(joints[index].mtx,binds[index],effective);
                for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)matrix[r][c]+=weight*effective[r][c];
                if(shortcut)break;
            }
            if(right)C_MTXConcat(matrix,right,matrix);
            std::array<float,12> stored;std::memcpy(stored.data(),matrix,sizeof(matrix));matrices.push_back(stored);
        }
        u32 cursor=word(model,polygon+16),end=cursor+(word(model,polygon+12)&0xffff)*32;
        while(cursor<end) {
            const u8 command=model.dat.data[cursor++];if(!command)continue;check(command==GX_TRIANGLES);
            const unsigned count=unsigned(model.dat.data[cursor])<<8|model.dat.data[cursor+1];cursor+=2;
            for(unsigned i=0;i<count;++i) {
                unsigned selectors[8]={},position_matrix=0;float normal[3]={};
                for(const auto& attr:attrs) {
                    const u8* bytes=model.dat.data+cursor;
                    if(attr.attr==GX_VA_PNMTXIDX)position_matrix=bytes[0];
                    if(attr.attr>=1&&attr.attr<=8)selectors[attr.attr-1]=bytes[0];
                    if(attr.attr==GX_VA_NRM||attr.attr==GX_VA_NBT) {
                        const unsigned index=attr.type==GX_INDEX8?bytes[0]:unsigned(bytes[0])<<8|bytes[1];
                        for(unsigned a=0;a<3;++a)check(HSD_DatReadF32(&model.dat,attr.array+index*attr.stride+a*4,&normal[a]));
                    }
                    cursor+=attr.display_bytes;
                }
                if (!separate) check(selectors[1]==30+position_matrix);
                else if (selectors[1]!=60) {
                    const unsigned follower=model.dat.data[metadata+8+(selectors[1]-30)/3];
                    if(follower!=255)check(follower==position_matrix/3);
                }
                for(unsigned channel:{0u,7u}) {
                    float value[3];
                    if(selectors[channel]==60)std::memcpy(value,normal,sizeof(value));
                    else {
                        check(selectors[channel]>=30&&selectors[channel]%3==0);
                        const unsigned matrix=(selectors[channel]-30)/3;check(matrix<matrices.size());
                        float input[3];std::memcpy(input,normal,sizeof(input));
                        if(bases) for(unsigned r=0;r<3;++r) {
                            input[r]=0;
                            for(unsigned c=0;c<3;++c) {
                                float coefficient;check(HSD_DatReadF32(&model.dat,bases+(matrix*9+r*3+c)*4,&coefficient));
                                input[r]+=coefficient*normal[c];
                            }
                        }
                        if(constants&(1U<<matrix))std::memcpy(value,input,sizeof(value));
                        else normal_matrix((float(*)[4])matrices[matrix].data(),input,value);
                    }
                    result.insert(result.end(),value,value+3);
                }
            }
        }
    }
    return result;
}

void textures(unsigned owner,unsigned indexing,bool nbt,bool distinct_basis=false)
{
    auto f=texture_fixture(owner,indexing,nbt,distinct_basis);
    NativeFighterProject project={};std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    project.costume_dat=(u8*)std::malloc(f.file.size());check(project.costume_dat);
    std::memcpy(project.costume_dat,f.file.data(),f.file.size());project.costume_size=f.file.size();
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    std::vector<std::vector<float>> expected;
    for(unsigned tick=0;tick<19;++tick)expected.push_back(texture_vectors(*model,tick));
    NativeSkinMesh skin;char error[256]={};check(nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
    check(distinct_basis?skin.parts[0].texture_bindings.size()>3:skin.parts[0].texture_bindings.size()==3);
    const auto fixed=skin.parts[0].texture_bindings[skin.parts[0].corners[0].textures[0]];
    // Equal weights must retain two distinct slots: only position is painted.
    skin.vertices[skin.parts[0].corners[0].vertex].skin=fixed;
    NativeAssetDat dat;check(dat.open(project.costume_dat,project.costume_size));uint32_t polygon;
    check(nativeSkinMesh_Encode(dat,*model,skin,&polygon));dat.link(model->meshes[0].offset+12,polygon);
    std::vector<u8> file;check(dat.finish(&file));
    NativeCostumeCompaction stats;check(nativeFighterCostume_Compact(&project,&file,&stats,error,sizeof(error))&&stats.supported);
    std::free(project.costume_dat);project.costume_dat=(u8*)std::malloc(file.size());check(project.costume_dat);
    std::memcpy(project.costume_dat,file.data(),file.size());project.costume_size=file.size();
    for(unsigned stroke=0;stroke<3;++stroke) {
        check(nativeFighterModel_Open(model.get(),&project));check(nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
        const auto& part=skin.parts[0];const auto& corner=part.corners[0];
        check(corner.textures[0]<part.texture_bindings.size());
        check(nativeSkinWeights_Equal(&part.texture_bindings[corner.textures[0]],&fixed));
        check(corner.textures[1]==NATIVE_SKIN_TEXTURE_FOLLOW);
        if(!distinct_basis)check(corner.textures[7]==NATIVE_SKIN_TEXTURE_IDENTITY);
        for(unsigned tick=0;tick<19;++tick) {
            const auto actual=texture_vectors(*model,tick);check(actual.size()==expected[tick].size());
            for(unsigned i=0;i<actual.size();++i)near(actual[i],expected[tick][i],&max_normal_error);
        }
        if(stroke==2)break;
        NativeSkinBrushPoint center;std::memcpy(center.position,skin.vertices[corner.vertex].position,sizeof(center.position));
        unsigned affected=0;check(nativeFighterSkin_Paint(&project,0,3,&center,1,.001f,.2f,false,&affected,error,sizeof(error)));
        check(affected==(distinct_basis?1u:2u));
    }
    if(owner==0&&indexing==GX_INDEX16&&!nbt&&!distinct_basis) {
        check(nativeFighterModel_Open(model.get(),&project));check(nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
        auto& part=skin.parts[0];part.texture_bindings.clear();part.texture_bases.clear();
        for(unsigned i=0;i<9;++i) {
            NativeSkinWeights binding={};binding.count=2;binding.joints[0]=0;binding.joints[1]=1;
            binding.weights[0]=.1f+.01f*i;binding.weights[1]=1-binding.weights[0];
            part.texture_bindings.push_back(binding);
            part.texture_bases.push_back(nativeSkinIdentityBasis);
        }
        for(unsigned i=0;i<skin.vertices.size();++i) {
            auto& vertex=skin.vertices[i];vertex.skin={};vertex.skin.count=2;
            vertex.skin.joints[0]=0;vertex.skin.joints[1]=3;
            vertex.skin.weights[0]=.2f+.1f*i;vertex.skin.weights[1]=1-vertex.skin.weights[0];
        }
        for(unsigned c=0;c<part.corners.size();++c) {
            part.corners[c].textures[0]=c%3*3;
            part.corners[c].textures[1]=c%3*3+1;
            part.corners[c].textures[7]=c%3*3+2;
        }
        NativeAssetDat bounded;check(bounded.open(project.costume_dat,project.costume_size));
        check(nativeSkinMesh_Encode(bounded,*model,skin,&polygon));bounded.link(model->meshes[0].offset+12,polygon);
        check(bounded.finish(&file));
        std::free(project.costume_dat);project.costume_dat=(u8*)std::malloc(file.size());check(project.costume_dat);
        std::memcpy(project.costume_dat,file.data(),file.size());project.costume_size=file.size();
        check(nativeFighterModel_Open(model.get(),&project));check(nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
        check(skin.parts[0].texture_bindings.size()==9);
        check((word(*model,polygon+12)>>16&0x3000)==POBJ_SEPARATE_ENVELOPES);
        const u32 metadata=word(*model,polygon+20),positions=word(*model,metadata),textures=word(*model,metadata+4);
        for(unsigned fault=0;fault<13;++fault) {
            NativeAssetDat bad;check(bad.open(file.data(),file.size()));
            const u32 commands=word(*model,polygon+16);
            if(fault<2)bad.data[metadata+8]=fault?0:99;
            else if(fault==2)bad.data[commands+3]=30;
            else if(fault==3)bad.data[commands+4]=57;
            else if(fault==4)bad.word(word(*model,positions)+4,0x3f800000);
            else if(fault==5)bad.word(word(*model,textures)+4,0x7fc00000);
            else if(fault==6)bad.link(metadata+4,0);
            else if(fault==7) {
                const u32 excessive=bad.allocate(12*4);
                for(unsigned i=0;i<11;++i)bad.link_local(excessive+i*4,word(*model,textures+(i%9)*4));
                bad.link(metadata+4,excessive);
            } else if(fault==8) bad.half(metadata+18,1);
            else if(fault==9) bad.half(metadata+18,1U<<9);
            else if(fault==12) bad.link(textures,bad.allocate(8));
            else {
                const u32 bases=bad.allocate(360);
                for(unsigned i=0;i<10;++i)for(unsigned j=0;j<9;++j) {
                    u32 bits;const float value=nativeSkinIdentityBasis[j];std::memcpy(&bits,&value,4);
                    bad.word(bases+(i*9+j)*4,bits);
                }
                bad.link(metadata+20,bases);
                if(fault==10)bad.word(bases,0x7fc00000);
                else {bad.link(textures,word(*model,positions));bad.data[metadata+8]=0;bad.word(bases,0x40000000);}
            }
            std::vector<u8> malformed;check(bad.finish(&malformed));const auto unchanged=malformed;
            HSD_Dat source;check(HSD_DatOpen(&source,malformed.data(),malformed.size())==HSD_DAT_OK);
            std::vector<u8> arena(1024*1024);HSD_DatImport importer;
            check(HSD_DatImportInit(&importer,&source,arena.data(),arena.size(),1024)==HSD_DAT_OK);
            check(!HSD_DatImportGraph(&importer,polygon,DAT_POBJ));
            check(importer.status!=HSD_DAT_OK&&malformed==unchanged);
        }
        const auto before_texture=texture_vectors(*model,7);
        NativeSkinBrushPoint center;std::memcpy(center.position,skin.vertices[0].position,sizeof(center.position));
        unsigned affected=99;
        check(nativeFighterSkin_Paint(&project,0,3,&center,1,.001f,.2f,false,&affected,error,sizeof(error)));
        check(affected==2);
        check(nativeFighterModel_Open(model.get(),&project));
        const auto after_texture=texture_vectors(*model,7);check(before_texture.size()==after_texture.size());
        for(unsigned i=0;i<after_texture.size();++i)near(after_texture[i],before_texture[i],&max_normal_error);
        // Native separate tables always use inverse-bind envelopes. A legacy
        // rigid shortcut inside this native representation remains malformed.
        file.assign(project.costume_dat,project.costume_dat+project.costume_size);
        NativeAssetDat different;check(different.open(file.data(),file.size()));
        const u32 head=word(*model,model->meshes[0].offset+12);
        const u32 table=word(*model,word(*model,head+20)),entries=word(*model,table);
        different.word(entries+4,0x3f800000);different.word(entries+12,0);
        std::vector<u8> different_file;check(different.finish(&different_file));file=std::move(different_file);
        std::free(project.costume_dat);project.costume_dat=(u8*)std::malloc(file.size());check(project.costume_dat);
        std::memcpy(project.costume_dat,file.data(),file.size());project.costume_size=file.size();
        check(nativeFighterModel_Open(model.get(),&project));
        check(!nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
    }
    std::free(project.costume_dat);++cases;
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
    uint32_t polygon;check(nativeSkinMesh_Encode(dat,*model,skin,&polygon));dat.link(model->meshes[0].offset+12,polygon);
    std::vector<u8> bytes;check(dat.finish(&bytes));
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
        for(unsigned owner:{0u,1u,2u})for(unsigned indexing:{2u,3u})for(bool nbt:{false,true})textures(owner,indexing,nbt);
        for(unsigned indexing:{2u,3u})for(bool nbt:{false,true})textures(0,indexing,nbt,true);
        large_mesh(false);large_mesh(true);
        std::printf("PASS: %u rigid skin cases, %u checks; max position=%g normal=%g\n",cases,checks,max_position_error,max_normal_error);
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
