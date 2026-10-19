#include <melee/ft/native_asset_dat.h>
#include <melee/ft/native_fighter_costume_compact.h>
#include <melee/ft/native_fighter_model_editor.h>
#include <melee/ft/native_fighter_skin_mesh.h>
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
Fixture fixture(unsigned owner,unsigned normals,bool additive,bool textures=false)
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
    f.attrs=d.allocate(((normals?6:5)+(textures?3:0))*24);
    const unsigned attributes[]={GX_VA_PNMTXIDX,GX_VA_POS,normals==2?(unsigned)GX_VA_NBT:(unsigned)GX_VA_NRM,GX_VA_CLR0,GX_VA_TEX0};
    unsigned number=0;
    if(textures)for(unsigned channel:{0u,1u,7u}) {
        const u32 at=f.attrs+number++*24;d.word(at,GX_VA_TEX0MTXIDX+channel);d.word(at+4,GX_DIRECT);
    }
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
            bytes.push_back(skin?(c%3)*3:0);
            if(textures)bytes.insert(bytes.end(),{(u8)(skin?30+(c+1)%3*3:30),(u8)(skin?30+c%3*3:30),GX_IDENTITY});
            bytes.push_back(0);bytes.push_back((u8)vertices[c]);
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
            std::vector<std::array<float,3>> storage(shape->nb_vertex_index);auto* points=reinterpret_cast<float(*)[3]>(storage.data());
            check(HSD_ShapeSetEvaluate(shape,points,nullptr));
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

std::vector<float> surface(const std::vector<u8>& file,const Fixture& f,unsigned pose=0)
{
    Imported imported;check(imported.open(file));HSD_JObj* root=HSD_JObjLoadJoint(imported.root);check(root);
    HSD_JObj* owner=(HSD_JObj*)HSD_IDGetData((uintptr_t)imported.joint(f.bones[f.owner]),nullptr);check(owner);
    HSD_DObj* mesh=owner->u.dobj;HSD_ShapeSet* shape=mesh->pobj->u.shape_set;
    if(pose)for(unsigned i=0;i<4;++i) {
        HSD_JObj* bone=(HSD_JObj*)HSD_IDGetData((uintptr_t)imported.joint(f.bones[i]),nullptr);check(bone);
        C_MTXIdentity(bone->mtx);bone->flags&=~JOBJ_MTX_DIRTY;
        bone->mtx[0][0]=1+.03f*i*pose;bone->mtx[1][1]=1+.02f*i*pose;
        bone->mtx[0][1]=.025f*i*pose;bone->mtx[0][3]=float(i)+.2f*pose;
        bone->mtx[1][3]=.5f*i-.1f*pose;bone->mtx[2][3]=.15f*i*pose;
    }
    std::vector<float> result;
    for(float blend:{-.25f,0.f,.6f,1.25f,2.f,3.f}) {
        if(f.additive){shape->blend.bp[0]=blend;shape->blend.bp[1]=1.3f-blend;}else shape->blend.bl=blend;
        nativeModelView_Begin(1,true);nativeModelView_Mesh(0,owner,mesh,false,false,nullptr);
        Vec3 points[6];check(nativeModelView_CornerPositions(0,points,6));
        for(auto& point:points)result.insert(result.end(),{point.x,point.y,point.z});
        std::vector<std::array<float,3>> local_storage(shape->nb_vertex_index), normals_storage(size_t(shape->nb_normal_index)*3);
        auto* local=reinterpret_cast<float(*)[3]>(local_storage.data());
        auto* normals=reinterpret_cast<float(*)[3]>(normals_storage.data());
        check(HSD_ShapeSetEvaluate(shape,local,normals));
        if(f.normals) {
            Mtx right_storage;MtxPtr right=_HSD_mkEnvelopeModelNodeMtx(owner,right_storage);
            unsigned corners=0;
            for(HSD_PObj* batch=shape->skin?shape->skin:mesh->pobj;batch;batch=shape->skin?batch->next:nullptr) {
                Mtx matrices[10]={};unsigned slots=0;
                if(shape->skin)for(HSD_SList* slot=pobj_type(batch)==POBJ_SEPARATE_ENVELOPES?
                    batch->u.matrices->positions:batch->u.envelope_list;slot;slot=slot->next,++slots) {
                    for(HSD_Envelope* env=(HSD_Envelope*)slot->data;env;env=env->next) {
                        HSD_JObjSetupMatrix(env->jobj);Mtx part;C_MTXConcat(env->jobj->mtx,env->jobj->envelopemtx,part);
                        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)matrices[slots][r][c]+=part[r][c]*env->weight;
                    }
                    if(right){Mtx tmp;C_MTXConcat(matrices[slots],right,tmp);C_MTXCopy(tmp,matrices[slots]);}
                } else {slots=1;C_MTXCopy(owner->mtx,matrices[0]);}
                const u8* cursor=batch->display;const u8* end=cursor+size_t(batch->n_display)*32;
                while(cursor<end) {
                    if(*cursor++==GX_NOP)continue;
                    const unsigned count=unsigned(cursor[0])<<8|cursor[1];cursor+=2;
                    for(unsigned i=0;i<count;++i,++corners) {
                        unsigned matrix=0,normal=0;
                        for(unsigned hardware=0;hardware<=GX_VA_TEX7;++hardware)
                            for(auto* a=batch->verts;a->attr!=GX_VA_NULL;++a) {
                                if((a->attr==GX_VA_NBT?GX_VA_NRM:a->attr)!=hardware||a->attr_type==GX_NONE)continue;
                                if(hardware==GX_VA_PNMTXIDX)matrix=cursor[0]/3;
                                if(hardware==GX_VA_NRM)normal=a->attr_type==GX_INDEX8?cursor[0]:unsigned(cursor[0])<<8|cursor[1];
                                cursor+=a->attr_type==GX_INDEX8?1:a->attr_type==GX_INDEX16?2:hardware<=GX_VA_TEX7MTXIDX?1:4;
                            }
                        check(matrix<slots&&normal<(unsigned)shape->nb_normal_index);
                        Mtx inverse;check(C_MTXInverse(matrices[matrix],inverse));
                        const unsigned vectors=f.normals==2?3:1;
                        for(unsigned n=0;n<vectors;++n)for(unsigned r=0;r<3;++r) {
                            float value=0;for(unsigned c=0;c<3;++c)value+=inverse[c][r]*normals[normal*vectors+n][c];
                            result.push_back(value);
                        }
                    }
                }
            }
            check(corners==6);
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

Fixture authoring_fixture(unsigned owner,unsigned normals,bool additive,bool weighted,
                           unsigned format,bool shared=false)
{
    auto f=fixture(owner,normals,additive);NativeAssetDat edit;check(edit.open(f.file.data(),f.file.size()));
    if(!weighted) {
        edit.half(f.shape,additive?SHAPESET_ADDITIVE:SHAPESET_AVERAGE);edit.link(f.shape+28,0);
        const u32 bind=NativeAssetDat::read32(edit.data.data()+f.bones[0]+56);
        edit.word(bind+12,0xbf400000);edit.word(bind+28,0x3e800000);
    }
    for(unsigned attr=0;attr<(normals?2u:1u);++attr) {
        const u32 descriptor=NativeAssetDat::read32(edit.data.data()+f.shape+8+attr*12);
        const u32 array=NativeAssetDat::read32(edit.data.data()+descriptor+20);
        const unsigned components=attr&&normals==2?9:3,width=format<=1?1:format<=3?2:4;
        const unsigned fraction=format==4?0:format<=1?3:8;
        const u32 converted=edit.allocate(18*components*width);check(converted!=UINT32_MAX);
        edit.word(descriptor+12,format);edit.data[descriptor+16]=(u8)fraction;
        edit.half(descriptor+18,(u16)(components*width));edit.link(descriptor+20,converted);
        for(unsigned i=0;i<18*components;++i) {
            u32 bits=NativeAssetDat::read32(edit.data.data()+array+i*4);float value;std::memcpy(&value,&bits,4);
            if(format==4)edit.word(converted+i*4,bits);
            else {
                const long low=format==0||format==2?0:format==1?-128:-32768;
                const long high=format==0?255:format==1?127:format==2?65535:32767;
                const long integer=std::clamp(std::lround(std::ldexp(value,(int)fraction)),low,high);
                if(width==1)edit.data[converted+i]=(u8)integer;
                else edit.half(converted+i*2,(u16)integer);
            }
        }
        // Distinct target identities survive coincident base positions.
        if(!attr&&!shared)std::memcpy(edit.data.data()+converted+components*width,
                                     edit.data.data()+converted,components*width);
    }
    if(shared) {
        check(normals&&!weighted);
        const unsigned stride=11;
        edit.half(f.display[0]+3+stride+3,0); // Second corner shares the first normal.
    }
    std::vector<u8> result;check(edit.finish(&result));f.file=std::move(result);return f;
}

std::vector<u8> rewritten(const Fixture& f,NativeSkinMesh* output=nullptr)
{
    NativeFighterProject project={};std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    project.costume_dat=(u8*)f.file.data();project.costume_size=f.file.size();
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    NativeSkinMesh skin;char error[256]={};check(nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
    check(skin.vertices.size()==6&&skin.parts.size()==1&&skin.corner_vertices.size()==6);
    for(unsigned i=0;i<6;++i)check(skin.corner_vertices[i]==vertices[i]);
    NativeAssetDat edit;check(edit.open(f.file.data(),f.file.size()));
    uint32_t polygon;check(nativeSkinMesh_Encode(edit,*model,skin,&polygon));edit.link(model->meshes[0].offset+12,polygon);
    std::vector<u8> result;check(edit.finish(&result));
    check(nativeFighterCostume_Compact(&project,&result,nullptr,error,sizeof(error)));
    if(output)*output=std::move(skin);return result;
}

void authoring(unsigned owner,unsigned normals,bool additive,bool weighted,unsigned format)
{
    const auto f=authoring_fixture(owner,normals,additive,weighted,format);
    const auto rewritten_file=rewritten(f);
    for(unsigned pose:{0u,3u,8u}) {
        const auto before=surface(f.file,f,pose),after=surface(rewritten_file,f,pose);check(before.size()==after.size());
        for(unsigned i=0;i<before.size();++i)near(before[i],after[i]);
    }
    ++cases;
}

void painting(unsigned owner,unsigned normals,bool additive)
{
    auto f=authoring_fixture(owner,normals,additive,false,GX_F32,true);
    NativeSkinMesh before;rewritten(f,&before);
    NativeFighterProject project={};std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    project.costume_dat=(u8*)std::malloc(f.file.size());check(project.costume_dat);
    project.costume_size=f.file.size();std::memcpy(project.costume_dat,f.file.data(),f.file.size());
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    unsigned bones[2]={};for(unsigned i=0;i<model->joint_count;++i)for(unsigned b=0;b<2;++b)
        if(model->joints[i].offset==f.bones[b+1])bones[b]=i;
    NativeSkinBrushPoint center;std::memcpy(center.position,before.vertices[1].position,sizeof(center.position));
    char error[256]={};unsigned affected=0;
    check(nativeFighterSkin_Paint(&project,0,bones[1],&center,1,.01f,.35f,false,&affected,error,sizeof(error))&&affected==1);
    NativeSkinMesh after;check(nativeFighterModel_Open(model.get(),&project));
    check(nativeSkinMesh_Read(*model,0,&after,error,sizeof(error)));
    check(after.parts.size()==1&&after.vertices.size()==6&&after.parts[0].morph.normal_count==7);
    check(after.parts[0].morph.positions==before.parts[0].morph.positions);
    for(unsigned v=0;v<6;++v)if(v!=1)check(nativeSkinWeights_Equal(&before.vertices[v].skin,&after.vertices[v].skin));
    near(nativeSkinWeights_Get(&after.vertices[1].skin,(u16)bones[1]),.35f);
    const unsigned components=normals==2?9:3;
    for(unsigned target=0;target<3;++target)for(unsigned n=0;n<7;++n)for(unsigned c=0;c<components;++c)
        near(after.parts[0].morph.normals[(target*7+n)*components+c],
             before.parts[0].morph.normals[(target*6+(n==6?0:n))*components+c]);
    const size_t size=project.costume_size;
    for(unsigned stroke=0;stroke<16;++stroke) {
        check(nativeFighterSkin_Paint(&project,0,bones[stroke%2],&center,1,.01f,.09f,false,&affected,error,sizeof(error))&&affected==1);
        check(project.costume_size<=size+1024);
    }
    f.file.assign(project.costume_dat,project.costume_dat+project.costume_size);
    const auto noop=rewritten(f);
    for(unsigned pose:{0u,5u}) {
        const auto a=surface(f.file,f,pose),b=surface(noop,f,pose);check(a.size()==b.size());
        for(unsigned i=0;i<a.size();++i)near(a[i],b[i]);
    }
    std::free(project.costume_dat);++cases;
}

void multiple_controllers(unsigned normals,bool additive)
{
    const auto f=fixture(0,normals,additive);NativeSkinMesh original;rewritten(f,&original);
    NativeSkinMesh skin;NativeSkinPart part=original.parts[0];
    part.corners.clear();part.triangles.clear();part.morph.position_count=30;part.morph.vertices.clear();
    part.morph.positions.resize(3*30*3);
    for(unsigned i=0;i<30;++i) {
        auto vertex=original.vertices[i%6];vertex.skin={};vertex.skin.count=2;
        vertex.skin.joints[0]=0;vertex.skin.joints[1]=1;
        vertex.skin.weights[1]=.05f+.03f*i;vertex.skin.weights[0]=1-vertex.skin.weights[1];
        for(unsigned target=0;target<3;++target)for(unsigned axis=0;axis<3;++axis)
            part.morph.positions[(target*30+i)*3+axis]=original.parts[0].morph.positions[(target*6+i%6)*3+axis]+float(i)*.125f;
        std::memcpy(vertex.position,part.morph.positions.data()+i*3,sizeof(vertex.position));
        skin.vertices.push_back(vertex);part.morph.vertices.push_back(i);
        auto corner=original.parts[0].corners[i%6];corner.vertex=i;
        part.corners.push_back(corner);part.triangles.push_back(i);
    }
    skin.parts.push_back(part);skin.parts.push_back(part);
    NativeFighterProject project={};std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    project.costume_dat=(u8*)f.file.data();project.costume_size=f.file.size();
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    NativeAssetDat edit;check(edit.open(f.file.data(),f.file.size()));uint32_t polygon;
    check(nativeSkinMesh_Encode(edit,*model,skin,&polygon));edit.link(model->meshes[0].offset+12,polygon);
    std::vector<u8> file;check(edit.finish(&file));char error[256]={};
    check(nativeFighterCostume_Compact(&project,&file,nullptr,error,sizeof(error)));
    Imported imported;check(imported.open(file));HSD_JObj* root=HSD_JObjLoadJoint(imported.root);check(root&&root->u.dobj);
    HSD_PObj* first=root->u.dobj->pobj;check(first&&first->next&&!first->next->next);
    HSD_AObjDesc animation[2]={};animation[0].end_frame=20;animation[1].end_frame=40;
    HSD_ShapeAnim clips[2]={};clips[0].next=&clips[1];clips[0].aobjdesc=&animation[0];clips[1].aobjdesc=&animation[1];
    HSD_PObjAddAnimAll(first,clips);check(nativeAObjPool.used==2);
    unsigned controller=0;
    for(HSD_PObj* parent=first;parent;parent=parent->next,++controller) {
        check(pobj_type(parent)==POBJ_SHAPEANIM&&parent->u.shape_set->aobj);
        auto* shape=parent->u.shape_set;check(shape->nb_vertex_index==30&&shape->nb_normal_index==30);
        unsigned batches=0;for(HSD_PObj* batch=shape->skin;batch;batch=batch->next)++batches;
        check(batches==4); // Ten-slot limit partitions the draw without changing either animation index.
        if(additive){shape->blend.bp[0]=.2f+.3f*controller;shape->blend.bp[1]=.65f-.1f*controller;}
        else shape->blend.bl=.25f+1.2f*controller;
    }
    nativeModelView_Begin(1,true);nativeModelView_Mesh(0,root,root->u.dobj,false,false,nullptr);
    Vec3 actual[60];check(nativeModelView_CornerPositions(0,actual,60));
    for(unsigned p=0;p<2;++p)for(unsigned i=0;i<30;++i)for(unsigned axis=0;axis<3;++axis) {
        auto value=[&](unsigned target){return part.morph.positions[(target*30+i)*3+axis];};
        float expected;
        if(additive)expected=value(0)+value(1)*(.2f+.3f*p)+value(2)*(.65f-.1f*p);
        else {const float t=.25f+1.2f*p;const unsigned target=(unsigned)t;
            expected=value(target)+(value(target+1)-value(target))*(t-target);}
        if(axis==0)expected-=skin.vertices[i].skin.weights[1];
        if(axis==1)expected-=.5f*skin.vertices[i].skin.weights[1];
        near(((float*)&actual[p*30+i])[axis],expected);
    }
    nativeModelView_Clear();HSD_JObjRemoveAll(root);check(nativeAObjPool.used==0);++cases;
}

void texture_attachments(unsigned owner,unsigned normals,bool additive)
{
    auto f=fixture(owner,normals,additive,true);
    const auto expected=surface(f.file,f);
    NativeSkinMesh original;auto file=rewritten(f,&original);
    const auto actual=surface(file,f);check(actual.size()==expected.size());
    for(unsigned i=0;i<actual.size();++i)near(actual[i],expected[i]);
    check(original.parts[0].texture_bindings.size()==3);
    NativeSkinWeights fixed[6];
    for(unsigned c=0;c<6;++c)fixed[c]=original.parts[0].texture_bindings[original.parts[0].corners[c].textures[0]];
    NativeFighterProject project={};std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    for(unsigned stroke=0;stroke<3;++stroke) {
        project.costume_dat=file.data();project.costume_size=file.size();
        auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
        NativeSkinMesh skin;char error[256]={};check(nativeSkinMesh_Read(*model,0,&skin,error,sizeof(error)));
        auto& part=skin.parts[0];check(part.corners.size()==6);
        for(unsigned c=0;c<6;++c) {
            const auto& corner=part.corners[c];check(corner.textures[0]<part.texture_bindings.size());
            check(nativeSkinWeights_Equal(&part.texture_bindings[corner.textures[0]],&fixed[c]));
            check(corner.textures[1]==NATIVE_SKIN_TEXTURE_FOLLOW&&corner.textures[7]==NATIVE_SKIN_TEXTURE_IDENTITY);
        }
        if(stroke==2)break;
        auto& position=skin.vertices[part.corners[0].vertex].skin;
        if(!stroke)position=fixed[0];else check(nativeSkinWeights_Set(&position,3,.2f));
        NativeAssetDat edit;check(edit.open(file.data(),file.size()));uint32_t polygon;
        check(nativeSkinMesh_Encode(edit,*model,skin,&polygon));edit.link(model->meshes[0].offset+12,polygon);
        std::vector<u8> next;check(edit.finish(&next));
        check(nativeFighterCostume_Compact(&project,&next,nullptr,error,sizeof(error)));file=std::move(next);
    }
    {
        Imported imported;check(imported.open(file));
        HSD_JObj* root=HSD_JObjLoadJoint(imported.root);check(root);
        auto* actor=(HSD_JObj*)HSD_IDGetData((uintptr_t)imported.joint(f.bones[owner]),nullptr);
        check(actor&&actor->u.dobj&&actor->u.dobj->pobj->u.shape_set->skin);
        auto* batch=actor->u.dobj->pobj->u.shape_set->skin;
        check(pobj_type(batch)==POBJ_SEPARATE_ENVELOPES);
        HSD_JObjSetCurrent(actor);nativeRenderGeometryEnabled=false;
        Mtx identity;C_MTXIdentity(identity);
        for(auto* part=batch;part;part=part->next)
            HSD_POBJ_METHOD(part)->setup_mtx(part,identity,identity,0);
        nativeRenderGeometryEnabled=true;HSD_JObjSetCurrent(nullptr);
        auto* texture_list=batch->u.matrices->textures;
        auto* position=(HSD_Envelope*)batch->u.matrices->positions->data;
        auto* texture=(HSD_Envelope*)texture_list->data;
        const float position_weight=position->weight,texture_weight=texture->weight;
        std::vector<u8> snapshot(nativeHsdArena.capacity+nativeHsdArena.root_bytes);NativeMatchArenaFrame frame;
        check(nativeMatchArena_BindFrame(&nativeHsdArena,&frame,snapshot.data(),snapshot.size())&&nativeMatchArena_Save(&frame));
        position->weight=.123f;texture->weight=.234f;batch->u.matrices->textures=nullptr;
        check(nativeMatchArena_Load(&frame));near(position->weight,position_weight);near(texture->weight,texture_weight);
        check(batch->u.matrices->textures==texture_list);
        check(nativeHsdArena_ResetHistory());HSD_JObjRemoveAll(root);
    }
    if(owner==0&&normals==1&&!additive)for(unsigned fault=0;fault<5;++fault) {
        NativeAssetDat edit;check(edit.open(f.file.data(),f.file.size()));
        const u32 offset=f.display[fault?1:0]+3+(fault==4?3:1);
        edit.data[offset]=fault==2?57:fault==3?60:fault==4?30:31;
        std::vector<u8> malformed;check(edit.finish(&malformed));const auto before=malformed;
        Imported imported;check(!imported.open(malformed));check(imported.loader.status!=HSD_DAT_OK&&malformed==before);
    }
    ++cases;
}

void full_index_domain(unsigned normals,bool additive)
{
    const auto f=fixture(1,normals,additive);NativeSkinMesh original;rewritten(f,&original);
    NativeSkinMesh skin;NativeSkinPart part=original.parts[0];
    const unsigned count=HSD_MAX_SHAPE_VERTICES,components=normals==2?9:3;
    part.corners.clear();part.triangles.clear();part.morph.position_count=count;
    part.morph.normal_count=count;part.morph.vertices.clear();
    part.morph.positions.resize(size_t(3)*count*3);part.morph.normals.resize(size_t(3)*count*components);
    for(unsigned i=0;i<count;++i) {
        NativeSkinVertex vertex={};vertex.position[0]=float(i)/1024;vertex.position[1]=float(i%31);
        vertex.skin.count=1;vertex.skin.joints[0]=1;vertex.skin.weights[0]=1;
        skin.vertices.push_back(vertex);part.morph.vertices.push_back(i);
        for(unsigned target=0;target<3;++target) {
            for(unsigned axis=0;axis<3;++axis)
                part.morph.positions[(size_t(target)*count+i)*3+axis]=(additive&&target?0:vertex.position[axis])+
                    (target?float(target)*10+float(i)/65536+float(axis)*.125f:0);
            for(unsigned c=0;c<components;++c)
                part.morph.normals[(size_t(target)*count+i)*components+c]=float(target)*10+float(i)/65536+float(c)*.125f;
        }
    }
    // Two GX primitives/private batches, including logical index 65535.
    for(unsigned i=0;i<count+2;++i) {
        auto corner=original.parts[0].corners[i%6];corner.vertex=corner.normal_index=i%count;
        part.corners.push_back(corner);part.triangles.push_back(i);
    }
    skin.parts.push_back(std::move(part));
    NativeFighterProject project={};std::snprintf(project.facts.costume_joint,sizeof(project.facts.costume_joint),"rig");
    project.costume_dat=(u8*)f.file.data();project.costume_size=f.file.size();
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    NativeAssetDat edit;check(edit.open(f.file.data(),f.file.size()));uint32_t polygon;
    check(nativeSkinMesh_Encode(edit,*model,skin,&polygon));edit.link(model->meshes[0].offset+12,polygon);
    std::vector<u8> file;check(edit.finish(&file));
    for(unsigned fault=0;fault<2;++fault) {
        NativeAssetDat bad;check(bad.open(file.data(),file.size()));
        const u32 shape=NativeAssetDat::read32(bad.data.data()+polygon+20);
        if(fault==0) {
            const u32 table=NativeAssetDat::read32(bad.data.data()+shape+12);
            const u32 indices=NativeAssetDat::read32(bad.data.data()+table+8);
            bad.word(indices+(count-1)*4,UINT32_MAX);
        } else {
            const u32 descriptor=NativeAssetDat::read32(bad.data.data()+shape+8);
            bad.word(descriptor+4,GX_INDEX8);
        }
        std::vector<u8> malformed;check(bad.finish(&malformed));const auto before=malformed;
        Imported rejected;rejected.arena.resize(4*1024*1024);check(!rejected.open(malformed));
        check(rejected.loader.status!=HSD_DAT_OK&&malformed==before);
    }
    project.costume_dat=file.data();project.costume_size=file.size();check(nativeFighterModel_Open(model.get(),&project));
    NativeSkinMesh decoded;char error[256]={};check(nativeSkinMesh_Read(*model,0,&decoded,error,sizeof(error)));
    check(decoded.parts[0].morph.positions==skin.parts[0].morph.positions);
    check(decoded.parts[0].morph.normals==skin.parts[0].morph.normals);
    Imported imported;imported.arena.resize(4*1024*1024);check(imported.open(file));
    HSD_JObj* root=HSD_JObjLoadJoint(imported.root);check(root);
    auto* owner=(HSD_JObj*)HSD_IDGetData((uintptr_t)imported.joint(f.bones[1]),nullptr);check(owner&&owner->u.dobj);
    auto* shape=owner->u.dobj->pobj->u.shape_set;
    check(shape->nb_vertex_index==count&&shape->nb_normal_index==count&&shape->skin&&shape->skin->next);
    check(shape->flags&SHAPESET_INDEX32);
    check(NativeAssetDat::read32(shape->vertex_idx_list[2]+(count-1)*4)>UINT16_MAX);
    check(NativeAssetDat::read32(shape->normal_idx_list[2]+(count-1)*4)>UINT16_MAX);
    if(additive){shape->blend.bp[0]=.3f;shape->blend.bp[1]=.7f;}else shape->blend.bl=1.5f;
    std::vector<std::array<float,3>> positions(count),lighting(size_t(count)*(normals==2?3:1));
    check(HSD_ShapeSetEvaluate(shape,reinterpret_cast<float(*)[3]>(positions.data()),reinterpret_cast<float(*)[3]>(lighting.data())));
    auto blended=[&](bool normal,unsigned i,unsigned c) {
        const unsigned width=normal?components:3;const auto& source=normal?skin.parts[0].morph.normals:skin.parts[0].morph.positions;
        const float a=source[i*width+c],b=source[(size_t(count)+i)*width+c],d=source[(size_t(count)*2+i)*width+c];
        return additive?a+b*.3f+d*.7f:(b+d)*.5f;
    };
    for(unsigned i=0;i<count;++i)for(unsigned axis=0;axis<3;++axis)near(positions[i][axis],blended(false,i,axis));
    for(unsigned i=0;i<count;++i)for(unsigned c=0;c<components;++c)
        near(lighting[size_t(i)*(normals==2?3:1)+c/3][c%3],blended(true,i,c));
    nativeModelView_Begin(1,true);nativeModelView_Mesh(0,owner,owner->u.dobj,false,false,nullptr);
    std::vector<Vec3> corners(count+2);check(nativeModelView_CornerPositions(0,corners.data(),(unsigned)corners.size()));
    for(unsigned i=0;i<corners.size();++i)for(unsigned axis=0;axis<3;++axis)
        near(((float*)&corners[i])[axis],blended(false,i%count,axis));
    nativeModelView_Clear();HSD_JObjRemoveAll(root);++cases;
    // Rest compensation must read and rewrite physical indices above 65535.
    project.costume_dat=(u8*)std::malloc(file.size());check(project.costume_dat);
    std::memcpy(project.costume_dat,file.data(),file.size());
    check(nativeFighterModel_Open(model.get(),&project));check(nativeFighterModel_SetJoint(model.get(),1,0,2,.35f));
    check(nativeFighterModel_CompensateRest(model.get(),file.data(),file.size(),1));
    std::vector<u8> rested(project.costume_dat,project.costume_dat+project.costume_size);
    Imported restored;restored.arena.resize(4*1024*1024);check(restored.open(rested));
    root=HSD_JObjLoadJoint(restored.root);check(root);
    owner=(HSD_JObj*)HSD_IDGetData((uintptr_t)restored.joint(f.bones[1]),nullptr);check(owner&&owner->u.dobj);
    shape=owner->u.dobj->pobj->u.shape_set;check(shape->flags&SHAPESET_INDEX32);
    if(additive){shape->blend.bp[0]=.3f;shape->blend.bp[1]=.7f;}else shape->blend.bl=1.5f;
    nativeModelView_Begin(1,true);nativeModelView_Mesh(0,owner,owner->u.dobj,false,false,nullptr);
    check(nativeModelView_CornerPositions(0,corners.data(),(unsigned)corners.size()));
    for(unsigned i=0;i<corners.size();++i)for(unsigned axis=0;axis<3;++axis)
        near(((float*)&corners[i])[axis],blended(false,i%count,axis));
    check(HSD_ShapeSetEvaluate(shape,reinterpret_cast<float(*)[3]>(positions.data()),reinterpret_cast<float(*)[3]>(lighting.data())));
    for(unsigned i=0;i<count;++i)for(unsigned v=0;v<(normals==2?3u:1u);++v) {
        Vec3 expected={blended(true,i,v*3),blended(true,i,v*3+1),blended(true,i,v*3+2)},actual;
        C_MTXMultVecSR(owner->mtx,(Vec3*)lighting[size_t(i)*(normals==2?3:1)+v].data(),&actual);
        near(actual.x,expected.x);near(actual.y,expected.y);near(actual.z,expected.z);
    }
    nativeModelView_Clear();HSD_JObjRemoveAll(root);std::free(project.costume_dat);++cases;
}
}
int main()
{
    try {
        HSD_ObjInit();
        malformed();
        for(unsigned owner:{0u,1u,3u})for(unsigned normals:{0u,1u,2u})for(bool additive:{false,true})valid(owner,normals,additive);
        for(unsigned owner:{0u,1u,3u})for(unsigned normals:{0u,1u,2u})for(bool additive:{false,true})rest(owner,normals,additive);
        for(unsigned owner:{0u,1u,3u})for(unsigned normals:{0u,1u,2u})for(bool additive:{false,true})
            for(bool weighted:{false,true})for(unsigned format=0;format<5;++format)authoring(owner,normals,additive,weighted,format);
        for(unsigned owner:{0u,1u,3u})for(unsigned normals:{1u,2u})for(bool additive:{false,true})painting(owner,normals,additive);
        for(unsigned normals:{1u,2u})for(bool additive:{false,true})multiple_controllers(normals,additive);
        for(unsigned owner:{0u,1u,3u})for(unsigned normals:{1u,2u})for(bool additive:{false,true})texture_attachments(owner,normals,additive);
        for(unsigned normals:{1u,2u})for(bool additive:{false,true})full_index_domain(normals,additive);
        std::printf("PASS: %u morph runtime/authoring cases, %u construction, pose, paint, compaction and snapshot checks\n",cases,checks);
        return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
