#include <melee/ft/native_asset_dat.h>
#include <melee/ft/native_fighter_costume_compact.h>
#include <melee/ft/native_fighter_model_editor.h>
extern "C" {
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/native_render_phase.h>
#include <sysdolphin/baselib/shape.h>
}
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>

namespace {
unsigned checks,cases;
void check(bool ok,unsigned line=__builtin_LINE())
{++checks;if(!ok)throw std::runtime_error("weighted shape check at "+std::to_string(line));}
void near(float a,float b){check(std::isfinite(a)&&std::isfinite(b)&&std::fabs(a-b)<.00002f);}
struct Dat {
    std::vector<u8> data;std::vector<u32> links;
    u32 allocate(unsigned n){u32 at=((u32)data.size()+3)&~3u;data.resize(at+n);return at;}
    void word(u32 at,u32 v){NativeAssetDat::be32(data.data()+at,v);}
    void half(u32 at,u16 v){NativeAssetDat::be16(data.data()+at,v);}
    void real(u32 at,float v){u32 bits;std::memcpy(&bits,&v,4);word(at,bits);}
    void link(u32 at,u32 v){word(at,v);links.push_back(at);}
    std::vector<u8> finish(u32 root){
        std::sort(links.begin(),links.end());
        std::vector<u8> file(32+data.size()+links.size()*4+12);
        NativeAssetDat::be32(file.data(),(u32)file.size());NativeAssetDat::be32(file.data()+4,(u32)data.size());
        NativeAssetDat::be32(file.data()+8,(u32)links.size());NativeAssetDat::be32(file.data()+12,1);
        std::memcpy(file.data()+32,data.data(),data.size());size_t tail=32+data.size();
        for(u32 slot:links){NativeAssetDat::be32(file.data()+tail,slot);tail+=4;}
        NativeAssetDat::be32(file.data()+tail,root);std::memcpy(file.data()+tail+8,"rig",4);return file;
    }
};
struct Fixture {
    std::vector<u8> file;
    u32 bones[4],shape,parent,batches[2],display[3],tables[2],weights[2][3],attrs;
    unsigned owner,normals;bool additive;
};
constexpr unsigned vertices[]={0,1,2,0,4,5};
constexpr unsigned joints[3][2]={{0,1},{0,2},{1,2}};
constexpr float weights[3][2]={{.7f,.3f},{.4f,.6f},{.5f,.5f}};
float value(bool additive,unsigned target,unsigned vertex,unsigned axis)
{
    const float base[6][3]={{-3,0,1},{0,3,0},{-2,2,2},{3,0,-1},{3,3,-1},{1,2,1}};
    const float delta=(float(vertex+1)*.125f)*(axis==1?-.5f:1.f)*target;
    return additive&&target?delta:base[vertex][axis]+delta;
}
Fixture fixture(unsigned owner,unsigned normals,bool additive)
{
    Dat d;Fixture f={};f.owner=owner;f.normals=normals;f.additive=additive;
    for(u32& bone:f.bones)bone=d.allocate(64);
    d.link(f.bones[0]+8,f.bones[1]);d.link(f.bones[1]+12,f.bones[2]);d.link(f.bones[1]+8,f.bones[3]);
    for(unsigned i=0;i<4;++i) {
        d.word(f.bones[i]+4,JOBJ_LIGHTING|(i==0?JOBJ_SKELETON_ROOT:i==3?0:JOBJ_SKELETON));
        for(unsigned a=0;a<3;++a)d.real(f.bones[i]+32+a*4,1);
        const u32 bind=d.allocate(48);d.link(f.bones[i]+56,bind);
        for(unsigned a=0;a<3;++a)d.real(bind+(a*4+a)*4,1);
        d.real(bind+12,-float(i));d.real(bind+28,-.5f*i);
    }
    const u32 mesh=d.allocate(16);d.link(f.bones[owner]+16,mesh);
    f.parent=d.allocate(24);d.link(mesh+12,f.parent);
    f.shape=d.allocate(32);d.half(f.shape,(additive?SHAPESET_ADDITIVE:SHAPESET_AVERAGE)|SHAPESET_SKIN);
    d.half(f.shape+2,additive?2:3);d.word(f.shape+4,6);d.word(f.shape+16,normals?6:0);
    u32 arrays[2]={};
    for(unsigned attr=0;attr<(normals?2u:1u);++attr) {
        const unsigned components=attr&&normals==2?9:3;
        const u32 desc=d.allocate(48),table=d.allocate(12);arrays[attr]=d.allocate(3*6*components*4);
        d.link(f.shape+8+attr*12,desc);d.link(f.shape+12+attr*12,table);
        d.word(desc,attr?(normals==2?GX_VA_NBT:GX_VA_NRM):GX_VA_POS);d.word(desc+4,GX_INDEX16);
        d.word(desc+8,attr?GX_NRM_XYZ:GX_POS_XYZ);d.word(desc+12,GX_F32);d.half(desc+18,components*4);
        d.link(desc+20,arrays[attr]);d.word(desc+24,GX_VA_NULL);
        for(unsigned target=0;target<3;++target) {
            const u32 indices=d.allocate(12);d.link(table+target*4,indices);
            for(unsigned v=0;v<6;++v) {
                d.half(indices+v*2,target*6+v);
                for(unsigned c=0;c<components;++c)d.real(arrays[attr]+((target*6+v)*components+c)*4,
                    attr?(additive&&target?.025f*target:float(c%3==2)+.1f*v):value(additive,target,v,c));
            }
        }
    }
    const u32 uv=d.allocate(48);for(unsigned i=0;i<12;++i)d.real(uv+i*4,.05f*i);
    f.attrs=d.allocate((normals?6:5)*24);
    const unsigned attributes[]={GX_VA_PNMTXIDX,GX_VA_POS,normals==2?(unsigned)GX_VA_NBT:(unsigned)GX_VA_NRM,GX_VA_CLR0,GX_VA_TEX0};
    unsigned number=0;
    for(unsigned attr:attributes) {
        if(!normals&&attr==GX_VA_NRM)continue;
        const u32 at=f.attrs+number++*24;d.word(at,attr);
        d.word(at+4,attr==GX_VA_PNMTXIDX||attr==GX_VA_CLR0?GX_DIRECT:GX_INDEX16);
        d.word(at+8,attr==GX_VA_POS||attr==GX_VA_CLR0||attr==GX_VA_TEX0?1:0);
        d.word(at+12,attr==GX_VA_CLR0?GX_RGBA8:GX_F32);
        if(attr==GX_VA_POS||attr==GX_VA_NRM||attr==GX_VA_NBT||attr==GX_VA_TEX0) {
            const unsigned components=attr==GX_VA_TEX0?2:attr==GX_VA_NBT?9:3;
            d.half(at+18,components*4);d.link(at+20,attr==GX_VA_TEX0?uv:attr==GX_VA_POS?arrays[0]:arrays[1]);
        }
    }
    d.word(f.attrs+number*24,GX_VA_NULL);
    auto display=[&](unsigned start,unsigned count,bool skin) {
        std::vector<u8> bytes={GX_TRIANGLES,0,(u8)count};
        for(unsigned c=start;c<start+count;++c) {
            bytes.push_back(skin?(c%3)*3:0);bytes.push_back(0);bytes.push_back((u8)vertices[c]);
            if(normals){bytes.push_back(0);bytes.push_back((u8)c);}
            bytes.insert(bytes.end(),{(u8)(100+c),50,70,255,0,(u8)vertices[c]});
        }
        bytes.resize((bytes.size()+31)&~size_t(31));u32 at=d.allocate((unsigned)bytes.size());
        std::memcpy(d.data.data()+at,bytes.data(),bytes.size());return std::pair(at,(u16)(bytes.size()/32));
    };
    auto authored=display(0,6,false);f.display[0]=authored.first;
    d.link(f.parent+8,f.attrs);d.half(f.parent+12,POBJ_SHAPEANIM);d.half(f.parent+14,authored.second);
    d.link(f.parent+16,authored.first);d.link(f.parent+20,f.shape);
    for(unsigned batch=0;batch<2;++batch) {
        f.batches[batch]=d.allocate(24);
        if(batch)d.link(f.batches[batch-1]+4,f.batches[batch]);else d.link(f.shape+28,f.batches[batch]);
        auto commands=display(batch*3,3,true);f.display[batch+1]=commands.first;
        d.link(f.batches[batch]+8,f.attrs);d.half(f.batches[batch]+12,POBJ_ENVELOPE);
        d.half(f.batches[batch]+14,commands.second);d.link(f.batches[batch]+16,commands.first);
        f.tables[batch]=d.allocate(12*4);d.link(f.batches[batch]+20,f.tables[batch]);
        for(unsigned slot=0;slot<3;++slot) {
            const u32 entries=d.allocate(3*8);f.weights[batch][slot]=entries;d.link(f.tables[batch]+slot*4,entries);
            for(unsigned b=0;b<2;++b){d.link(entries+b*8,f.bones[joints[slot][b]]);d.real(entries+b*8+4,weights[slot][b]);}
        }
    }
    f.file=d.finish(f.bones[0]);return f;
}

struct Imported {
    HSD_Dat dat={};std::vector<u8> arena=std::vector<u8>(512*1024);HSD_DatImport loader={};HSD_Joint* root=nullptr;
    bool open(const std::vector<u8>& file) {
        const auto status=HSD_DatOpen(&dat,file.data(),file.size());
        if(status!=HSD_DAT_OK){loader.status=status;return false;}
        u32 root_at;check(HSD_DatFindRoot(&dat,"rig",&root_at));
        check(HSD_DatImportInit(&loader,&dat,arena.data(),arena.size(),512)==HSD_DAT_OK);
        root=(HSD_Joint*)HSD_DatImportGraph(&loader,root_at,DAT_JOINT);return root!=nullptr;
    }
    HSD_Joint* joint(u32 offset) {
        for(unsigned i=0;i<loader.record_count;++i)if(loader.records[i].type==DAT_JOINT&&loader.records[i].offset==offset)
            return (HSD_Joint*)loader.records[i].object;
        check(false);return nullptr;
    }
};
void valid(unsigned owner,unsigned normals,bool additive)
{
    auto f=fixture(owner,normals,additive);const auto original=f.file;
    NativeFighterProject project={};std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    NativeCostumeCompaction stats;char error[256]={};
    check(nativeFighterCostume_Compact(&project,&f.file,&stats,error,sizeof(error))&&stats.supported);
    check(f.file.size()<=original.size());Imported imported;check(imported.open(f.file));
    HSD_JObj* root=HSD_JObjLoadJoint(imported.root);check(root);
    HSD_JObj* bones[4];for(unsigned i=0;i<4;++i){bones[i]=(HSD_JObj*)HSD_IDGetData((uintptr_t)imported.joint(f.bones[i]),nullptr);check(bones[i]);}
    HSD_DObj* mesh=bones[owner]->u.dobj;check(mesh&&mesh->pobj);
    HSD_PObj* parent=mesh->pobj;HSD_ShapeSet* shape=parent->u.shape_set;
    check(parent->next==nullptr&&shape->skin&&shape->skin->next&&shape->skin->next->next==nullptr);
    HSD_AObjDesc animation={};animation.end_frame=20;HSD_ShapeAnim clip={};clip.aobjdesc=&animation;
    HSD_PObjAddAnimAll(parent,&clip);check(shape->aobj&&nativeAObjPool.used==1);
    NativeMatchPoolReference controller;check(nativeMatchPool_Reference(&nativeAObjPool,shape->aobj,&controller));
    for(unsigned pose=0;pose<9;++pose) {
        for(unsigned i=0;i<4;++i) {
            C_MTXIdentity(bones[i]->mtx);bones[i]->flags&=~JOBJ_MTX_DIRTY;
            bones[i]->mtx[0][0]=1+.03f*i*pose;bones[i]->mtx[1][1]=1+.02f*i*pose;
            bones[i]->mtx[0][1]=.025f*i*pose;bones[i]->mtx[0][3]=float(i)+.2f*pose;
            bones[i]->mtx[1][3]=.5f*i-.1f*pose;bones[i]->mtx[2][3]=.15f*i*pose;
        }
        for(float blend:{-.5f,0.f,.25f,1.f,1.75f,3.f}) {
            if(additive){shape->blend.bp[0]=blend;shape->blend.bp[1]=1.3f-blend;}else shape->blend.bl=blend;
            float points[HSD_DEFAULT_MAX_SHAPE_VERTICES][3];check(HSD_ShapeSetEvaluate(shape,points,nullptr));
            nativeModelView_Begin(1,true);nativeModelView_Mesh(0,bones[owner],mesh,false,false,bones[1]);
            Vec3 actual[6];check(nativeModelView_CornerPositions(0,actual,6));
            Mtx right;C_MTXIdentity(right);
            if(owner==1)check(C_MTXInverse(bones[1]->envelopemtx,right));
            else if(owner==3){Mtx base,inv;C_MTXConcat(bones[1]->mtx,bones[1]->envelopemtx,base);
                check(C_MTXInverse(base,inv));C_MTXConcat(inv,bones[3]->mtx,right);}
            for(unsigned c=0;c<6;++c) {
                Vec3 expected={};
                for(unsigned b=0;b<2;++b) {
                    HSD_JObj* joint=bones[joints[c%3][b]];Mtx skin,final;
                    C_MTXConcat(joint->mtx,joint->envelopemtx,skin);C_MTXConcat(skin,right,final);
                    Vec3 position;C_MTXMultVec(final,(Vec3*)points[vertices[c]],&position);
                    expected.x+=position.x*weights[c%3][b];expected.y+=position.y*weights[c%3][b];expected.z+=position.z*weights[c%3][b];
                }
                near(actual[c].x,expected.x);near(actual[c].y,expected.y);near(actual[c].z,expected.z);
            }
            HSD_JObjSetCurrent(bones[owner]);nativeRenderGeometryEnabled=false;
            HSD_POBJ_METHOD(parent)->setup_mtx(parent,right,right,0);
            nativeRenderGeometryEnabled=true;HSD_JObjSetCurrent(nullptr);
        }
    }
    check(nativeHsdArena_AdmitHistory()&&nativeAnimationPool_AdmitHistory(&nativeHsdArena));
    std::vector<u8> snapshot(nativeHsdArena.capacity+nativeHsdArena.root_bytes);NativeMatchArenaFrame frame;
    check(nativeMatchArena_BindFrame(&nativeHsdArena,&frame,snapshot.data(),snapshot.size())&&nativeMatchArena_Save(&frame));
    const float blend=additive?shape->blend.bp[0]:shape->blend.bl;
    const float animation_frame=shape->aobj->curr_frame;
    HSD_PObj* second=shape->skin->next;auto* env=(HSD_Envelope*)shape->skin->u.envelope_list->data;
    const float weight=env->weight;
    if(additive)shape->blend.bp[0]=17;else shape->blend.bl=17;
    env->weight=.123f;shape->aobj->curr_frame=19;shape->skin->next=nullptr;
    check(nativeMatchArena_Load(&frame));near(additive?shape->blend.bp[0]:shape->blend.bl,blend);
    near(env->weight,weight);near(shape->aobj->curr_frame,animation_frame);check(shape->skin->next==second);
    check(nativeHsdArena_ResetHistory());nativeModelView_Clear();HSD_JObjRemoveAll(root);
    check(nativeAObjPool.used==0&&nativeFObjPool.used==0&&!nativeMatchPool_Resolve(controller));++cases;
}

void malformed()
{
    const auto f=fixture(0,1,false);
    for(unsigned fault=0;fault<17;++fault) {
        NativeAssetDat edit;check(edit.open(f.file.data(),f.file.size()));
        const unsigned stride=11;
        switch(fault) {
        case 0:break; // Corrupt the finalized pointer below, bypassing the writer's own rejection.
        case 1:edit.link(f.batches[0]+4,f.batches[0]);break;
        case 2:edit.link(f.shape+28,f.parent);break;
        case 3:edit.data[f.display[1]+3]=1;break;
        case 4:edit.data[f.display[1]+3]=30;break;
        case 5:edit.data[f.display[2]+3]=3;break;
        case 6:edit.half(f.display[1]+4,1);break;
        case 7:edit.half(f.display[0]+3+stride+3,0);edit.half(f.display[1]+3+stride+3,0);break;
        case 8:edit.word(f.weights[0][0]+4,0x7fc00000);break;
        case 9:edit.word(f.weights[0][0]+4,0xbf800000);break;
        case 10:edit.word(f.weights[0][0]+4,0);break;
        case 11:edit.word(f.shape+4,1);break;
        case 12:edit.data[f.display[1]]=GX_QUADS;break;
        case 13:edit.half(f.display[1]+3+9,5);break;
        case 14:for(unsigned i=3;i<11;++i)edit.link(f.tables[0]+i*4,f.weights[0][0]);break;
        case 15:edit.word(f.weights[0][0]+4,0x3f800000);edit.word(f.weights[0][0]+12,0);break;
        case 16:edit.link(f.bones[1]+56,0);break;
        }
        std::vector<u8> bytes;check(edit.finish(&bytes));
        if(fault==0)NativeAssetDat::be32(bytes.data()+32+f.shape+28,UINT32_MAX);
        const auto original=bytes;
        Imported imported;check(!imported.open(bytes));check(imported.loader.status!=HSD_DAT_OK&&bytes==original);
    }
}

std::vector<float> surface(const std::vector<u8>& file,const Fixture& f)
{
    Imported imported;check(imported.open(file));HSD_JObj* root=HSD_JObjLoadJoint(imported.root);check(root);
    HSD_JObj* owner=(HSD_JObj*)HSD_IDGetData((uintptr_t)imported.joint(f.bones[f.owner]),nullptr);check(owner);
    HSD_DObj* mesh=owner->u.dobj;HSD_ShapeSet* shape=mesh->pobj->u.shape_set;
    std::vector<float> result;
    for(float blend:{-.25f,0.f,.6f,1.25f,2.f,3.f}) {
        if(f.additive){shape->blend.bp[0]=blend;shape->blend.bp[1]=1.3f-blend;}else shape->blend.bl=blend;
        nativeModelView_Begin(1,true);nativeModelView_Mesh(0,owner,mesh,false,false,nullptr);
        Vec3 points[6];check(nativeModelView_CornerPositions(0,points,6));
        for(auto& point:points)result.insert(result.end(),{point.x,point.y,point.z});
        float local[HSD_DEFAULT_MAX_SHAPE_VERTICES][3],normals[HSD_DEFAULT_MAX_SHAPE_NORMALS][3];
        check(HSD_ShapeSetEvaluate(shape,local,normals));
        if(f.normals) {
            Mtx right_storage;MtxPtr right=_HSD_mkEnvelopeModelNodeMtx(owner,right_storage);
            unsigned corner=0;
            for(HSD_PObj* batch=shape->skin;batch;batch=batch->next) {
                for(HSD_SList* slot=batch->u.envelope_list;slot;slot=slot->next,++corner) {
                    Mtx matrix={};
                    for(HSD_Envelope* env=(HSD_Envelope*)slot->data;env;env=env->next) {
                        HSD_JObjSetupMatrix(env->jobj);Mtx part;C_MTXConcat(env->jobj->mtx,env->jobj->envelopemtx,part);
                        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)matrix[r][c]+=part[r][c]*env->weight;
                    }
                    if(right){Mtx tmp;C_MTXConcat(matrix,right,tmp);C_MTXCopy(tmp,matrix);}
                    Mtx inverse;check(C_MTXInverse(matrix,inverse));
                    const unsigned vectors=f.normals==2?3:1;
                    for(unsigned n=0;n<vectors;++n)for(unsigned r=0;r<3;++r) {
                        float value=0;for(unsigned c=0;c<3;++c)value+=inverse[c][r]*normals[corner*vectors+n][c];
                        result.push_back(value);
                    }
                }
            }
        }
    }
    nativeModelView_Clear();HSD_JObjRemoveAll(root);return result;
}

void rest(unsigned owner,unsigned normals,bool additive)
{
    const auto f=fixture(owner,normals,additive);const auto expected=surface(f.file,f);
    for(unsigned bone=0;bone<4;++bone)for(unsigned channel=0;channel<3;++channel) {
        NativeFighterProject project={};std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
        project.costume_dat=(u8*)std::malloc(f.file.size());check(project.costume_dat);
        project.costume_size=f.file.size();std::memcpy(project.costume_dat,f.file.data(),f.file.size());
        auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
        unsigned joint=0;while(joint<model->joint_count&&model->joints[joint].offset!=f.bones[bone])++joint;
        check(joint<model->joint_count);
        check(nativeFighterModel_SetJoint(model.get(),joint,channel,channel==1?0:2,channel==0?.5f:channel==1?1.7f:3.f));
        check(nativeFighterModel_CompensateRest(model.get(),f.file.data(),f.file.size(),joint));
        std::vector<u8> bytes(project.costume_dat,project.costume_dat+project.costume_size);
        const auto actual=surface(bytes,f);check(actual.size()==expected.size());
        for(unsigned i=0;i<actual.size();++i)near(actual[i],expected[i]);
        std::free(project.costume_dat);++cases;
    }
}
}
int main()
{
    try {
        HSD_ObjInit();
        malformed();
        for(unsigned owner:{0u,1u,3u})for(unsigned normals:{0u,1u,2u})for(bool additive:{false,true})valid(owner,normals,additive);
        for(unsigned owner:{0u,1u,3u})for(unsigned normals:{0u,1u,2u})for(bool additive:{false,true})rest(owner,normals,additive);
        std::printf("PASS: %u weighted morph cases, %u construction, pose, compaction and snapshot checks\n",cases,checks);
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
