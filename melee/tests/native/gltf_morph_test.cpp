#include <melee/ft/native_fighter_gltf_import.h>
#include <melee/ft/native_fighter_gltf_skin.h>
#include <melee/ft/native_fighter_skin_mesh.h>
#include <melee/ft/native_fighter_mesh_source.h>
#include <melee/ft/native_fighter_core_asset_state.h>
#include <melee/ft/native_fighter_authoring.h>
#include <melee/ft/native_asset_dat.h>
extern "C" {
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/shape.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/displayfunc.h>
#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <sysdolphin/baselib/mtx.h>
}
#include <array>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace {
unsigned checks;char error[512];
void check(bool value,unsigned line=__builtin_LINE())
{++checks;if(!value)throw std::runtime_error("glTF morph check at "+std::to_string(line)+": "+error);}
void near(double a,double b,unsigned line=__builtin_LINE())
{
    ++checks;
    if(!std::isfinite(a)||!std::isfinite(b)||std::abs(a-b)>=.00004)
        throw std::runtime_error("glTF morph values at "+std::to_string(line)+": "+std::to_string(a)+" != "+std::to_string(b));
}
unsigned ordinal(const NativeFighterModelEditor& model,uint32_t offset)
{for(unsigned i=0;i<model.mesh_count;++i)if(model.meshes[i].offset==offset)return i;check(false);return 0;}
struct Source {
    cgltf_data* data=nullptr;const cgltf_node* node=nullptr;
    Source(const std::filesystem::path& path) {
        cgltf_options options={};check(cgltf_parse_file(&options,path.string().c_str(),&data)==cgltf_result_success);
        check(cgltf_load_buffers(&options,data,path.string().c_str())==cgltf_result_success);
        for(cgltf_size i=0;i<data->nodes_count;++i)if(data->nodes[i].mesh==&data->meshes[0]){node=&data->nodes[i];break;}
        check(node);
    }
    ~Source(){cgltf_free(data);}
};
const cgltf_accessor* attribute(const cgltf_attribute* attrs,size_t count,cgltf_attribute_type type)
{for(size_t i=0;i<count;++i)if(attrs[i].type==type)return attrs[i].data;return nullptr;}

void append_normals(const NativeFighterModelEditor& model,const NativeSkinMesh& skin,
                    const FtCostumeAssets& imported,HSD_JObj* owner,const float (*evaluated)[3],std::vector<float>& output)
{
    std::vector<HSD_JObj*> bones(model.joint_count);
    for(unsigned i=0;i<imported.import.record_count;++i) {
        const auto& record=imported.import.records[i];if(record.type!=DAT_JOINT)continue;
        for(unsigned b=0;b<model.joint_count;++b)if(model.joints[b].offset==record.offset)
            bones[b]=(HSD_JObj*)HSD_IDGetData((uintptr_t)record.object,nullptr);
    }
    struct NormalMatrix {float values[3][3];};std::vector<NormalMatrix> matrices(skin.vertices.size());
    Mtx right_storage;MtxPtr right=_HSD_mkEnvelopeModelNodeMtx(owner,right_storage);
    for(unsigned v=0;v<skin.vertices.size();++v) {
        Mtx palette={};const auto& weights=skin.vertices[v].skin;
        for(unsigned i=0;i<weights.count;++i) {
            auto* joint=bones[weights.joints[i]];check(joint);HSD_JObjSetupMatrix(joint);Mtx matrix;
            if(joint->envelopemtx)C_MTXConcat(joint->mtx,joint->envelopemtx,matrix);else C_MTXCopy(joint->mtx,matrix);
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)palette[r][c]+=matrix[r][c]*weights.weights[i];
        }
        if(right){Mtx matrix;C_MTXConcat(palette,right,matrix);C_MTXCopy(matrix,palette);}
        Mtx inverse;check(C_MTXInverse(palette,inverse));
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)matrices[v].values[r][c]=inverse[c][r];
    }
    for(const auto& part:skin.parts)for(unsigned triangle:part.triangles) {
        const auto& corner=part.corners[triangle];const float* value=evaluated?evaluated[corner.normal_index]:corner.normal;
        float normal[3]={};for(unsigned r=0;r<3;++r)for(unsigned c=0;c<3;++c)
            normal[r]+=matrices[corner.vertex].values[r][c]*value[c];
        const float length=std::sqrt(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);check(std::isfinite(length)&&length>0);
        for(float component:normal)output.push_back(component/length);
    }
}

std::vector<float> runtime(NativeFighterProject& project,uint32_t mesh_offset,const Source& source,bool oracle)
{
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    const unsigned mesh=ordinal(*model,mesh_offset);const auto* metadata=nativeFighterMeshSource_Get(&project,mesh_offset);check(metadata);
    FtCostumeAssets imported={};check(ftCostumeAssets_Load(project.costume_dat,project.costume_size,model->root_name,nullptr,&imported));
    HSD_DObjDesc* desc=nullptr;HSD_Joint* owner_desc=nullptr;
    for(unsigned i=0;i<imported.import.record_count;++i) {
        const auto& r=imported.import.records[i];
        if(r.type==DAT_DOBJ&&r.offset==mesh_offset)desc=(HSD_DObjDesc*)r.object;
        if(r.type==DAT_JOINT) {
            auto* joint=(HSD_Joint*)r.object;
            if(!(joint->flags&(JOBJ_PTCL|JOBJ_SPLINE)))joint->u.dobjdesc=nullptr;
            if(r.offset==model->joints[model->meshes[mesh].joint].offset)owner_desc=joint;
        }
    }
    check(desc&&desc->pobjdesc&&owner_desc);HSD_DObjDesc only={};only.pobjdesc=desc->pobjdesc;owner_desc->u.dobjdesc=&only;
    HSD_JObj* root=HSD_JObjLoadJoint(imported.joint);check(root);
    HSD_JObj* owner=(HSD_JObj*)HSD_IDGetData((uintptr_t)owner_desc,nullptr);check(owner&&owner->u.dobj);
    NativeSkinMesh normal_skin;check(nativeSkinMesh_Read(*model,mesh,&normal_skin,error,sizeof(error)));
    if(!source.data->meshes[0].primitives[0].targets_count) {
        const auto* metadata=nativeFighterMeshSource_Get(&project,mesh_offset);check(metadata);
        nativeModelView_Begin(1,true);nativeModelView_Mesh(0,owner,owner->u.dobj,false,false,nullptr);
        std::vector<Vec3> corners(metadata->corners);check(nativeModelView_CornerPositions(0,corners.data(),(unsigned)corners.size()));
        std::vector<float> result;for(auto& point:corners)result.insert(result.end(),{point.x,point.y,point.z});
        append_normals(*model,normal_skin,imported,owner,nullptr,result);
        nativeModelView_Clear();HSD_JObjRemoveAll(root);std::free(imported.arena);return result;
    }
    HSD_ShapeSet* shape=owner->u.dobj->pobj->u.shape_set;check(shape&&shape->nb_shape==source.data->meshes[0].primitives[0].targets_count);
    check(shape->flags&SHAPESET_SIGNED_WEIGHTS);
    const bool named=source.data->meshes[0].target_names_count!=0;check(named==(shape->target_names!=nullptr));
    const auto& primitive=source.data->meshes[0].primitives[0];
    const float* defaults=source.node->weights_count?source.node->weights:source.data->meshes[0].weights;
    std::vector<unsigned> mapping;
    for(unsigned i=0;i<shape->nb_shape;++i) {
        unsigned found=i;
        if(named){found=0;while(found<shape->nb_shape&&std::strcmp(shape->target_names[i],source.data->meshes[0].target_names[found]))++found;}
        check(found<shape->nb_shape);mapping.push_back(found);near(shape->blend.bp[i],defaults?defaults[found]:0);
    }
    std::vector<float> result;
    for(unsigned pose=0;pose<4;++pose) {
        if(pose)for(unsigned t=0;t<shape->nb_shape;++t)shape->blend.bp[t]=pose==1?0:pose==2?-.4f+.65f*t:1.25f-.1f*t;
        std::vector<std::array<float,3>> positions_storage(shape->nb_vertex_index), normals_storage(size_t(shape->nb_normal_index)*3);
        auto* positions=reinterpret_cast<float(*)[3]>(positions_storage.data());
        auto* normals=reinterpret_cast<float(*)[3]>(normals_storage.data());
        check(HSD_ShapeSetEvaluate(shape,positions,normals));
        if(oracle) {
            const auto* pos=attribute(primitive.attributes,primitive.attributes_count,cgltf_attribute_type_position);
            const auto* nrm=attribute(primitive.attributes,primitive.attributes_count,cgltf_attribute_type_normal);check(pos&&nrm);
            Mtx matrix,inverse;for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)matrix[r][c]=metadata->transform[r+c*4];
            check(C_MTXInverse(matrix,inverse));
            for(unsigned v=0;v<(unsigned)shape->nb_vertex_index;++v) {
                float base[3],normal[3];check(cgltf_accessor_read_float(pos,v,base,3)&&cgltf_accessor_read_float(nrm,v,normal,3));
                double p[3]={base[0],base[1],base[2]},n[3]={normal[0],normal[1],normal[2]},normal_length=0;
                for(unsigned r=0;r<3;++r) {
                    double value=0;for(unsigned k=0;k<3;++k)value+=inverse[k][r]*normal[k];normal_length+=value*value;
                }
                normal_length=std::sqrt(normal_length);
                for(unsigned t=0;t<shape->nb_shape;++t) {
                    const auto& target=primitive.targets[mapping[t]];
                    for(auto type:{cgltf_attribute_type_position,cgltf_attribute_type_normal}) {
                        const auto* a=attribute(target.attributes,target.attributes_count,type);if(!a)continue;
                        float value[3];check(cgltf_accessor_read_float(a,v,value,3));
                        for(unsigned c=0;c<3;++c)(type==cgltf_attribute_type_position?p[c]:n[c])+=shape->blend.bp[t]*double(value[c]);
                    }
                }
                for(unsigned r=0;r<3;++r) {
                    double point=0,direction=0;
                    for(unsigned k=0;k<3;++k){point+=matrix[r][k]*p[k]*metadata->scale;direction+=inverse[k][r]*n[k]/normal_length;}
                    point+=matrix[r][3]*(metadata->uses_bind_shape?1:metadata->scale);
                    near(positions[v][r],point);near(normals[v][r],direction);
                }
            }
        }
        nativeModelView_Begin(1,true);nativeModelView_Mesh(0,owner,owner->u.dobj,false,false,nullptr);
        std::vector<Vec3> corners(metadata->corners);check(nativeModelView_CornerPositions(0,corners.data(),(unsigned)corners.size()));
        for(auto& point:corners)result.insert(result.end(),{point.x,point.y,point.z});
        append_normals(*model,normal_skin,imported,owner,normals,result);
    }
    check(nativeHsdArena_AdmitHistory()&&nativeAnimationPool_AdmitHistory(&nativeHsdArena));
    std::vector<u8> storage(nativeHsdArena.capacity+nativeHsdArena.root_bytes);NativeMatchArenaFrame frame;
    check(nativeMatchArena_BindFrame(&nativeHsdArena,&frame,storage.data(),storage.size())&&nativeMatchArena_Save(&frame));
    const float weight=shape->blend.bp[0];const auto* names=shape->target_names;auto* batches=shape->skin;
    shape->blend.bp[0]=123;shape->target_names=nullptr;shape->skin=nullptr;
    check(nativeMatchArena_Load(&frame));near(shape->blend.bp[0],weight);
    check(shape->target_names==names&&shape->skin==batches);check(nativeHsdArena_ResetHistory());
    nativeModelView_Clear();HSD_JObjRemoveAll(root);std::free(imported.arena);return result;
}

void run(const std::filesystem::path& base,const std::filesystem::path& fixtures,NativeGltfBindingMode binding,bool large=false)
{
    const std::string prefix=large?"large-":"";
    const auto folder=fixtures/(prefix+"case-"+std::to_string(binding));std::filesystem::create_directory(folder);
    std::filesystem::copy(base,folder,std::filesystem::copy_options::recursive);
    NativeFighterProject project={};check(nativeFighterProject_Open(folder.string().c_str(),&project));
    const auto original=fixtures/(large?(binding==NATIVE_GLTF_BIND_SOURCE?"large-skin.glb":"large.glb"):
        (binding==NATIVE_GLTF_BIND_SOURCE?"skin.glb":"original.glb"));Source source(original);
    NativeGltfImportRequest request={0,0,UINT_MAX,binding,1.25f,false};
    check(nativeFighterGltf_ApplyBatch(&project,original.string().c_str(),&request,1,error,sizeof(error)));
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    uint32_t offset=0;for(unsigned i=0;i<model->mesh_count;++i) {
        const auto* s=nativeFighterMeshSource_Get(&project,(uint32_t)model->meshes[i].offset);
        if(s&&!std::strcmp(s->identity,"mesh:11:Expressions"))offset=s->mesh;
    }
    check(offset);runtime(project,offset,source,true);
    NativeFighterCoreAssetState before,painted;check(nativeFighterCoreAssets_Capture(project,before));
    NativeSkinMesh skin;unsigned mesh=ordinal(*model,offset);check(nativeSkinMesh_Read(*model,mesh,&skin,error,sizeof(error)));
    unsigned bone=0;for(unsigned i=0;i<model->joint_count;++i)
        if((model->joints[i].flags&JOBJ_SKELETON)&&nativeSkinWeights_Get(&skin.vertices[0].skin,(u16)i)<.75f){bone=i;break;}
    check(bone);NativeSkinBrushPoint center;std::memcpy(center.position,skin.vertices[0].position,sizeof(center.position));unsigned affected;
    check(nativeFighterSkin_Paint(&project,mesh,bone,&center,1,.01f,.17f,false,&affected,error,sizeof(error))&&affected==1);
    check(nativeFighterCoreAssets_Capture(project,painted));runtime(project,offset,source,true);
    check(nativeFighterCoreAssets_Restore(project,painted,before,error,sizeof(error))&&nativeFighterCoreAssets_Matches(project,before));
    check(nativeFighterCoreAssets_Restore(project,before,painted,error,sizeof(error))&&nativeFighterCoreAssets_Matches(project,painted));
    const auto prior=runtime(project,offset,source,false);std::vector<u8> old(project.costume_dat,project.costume_dat+project.costume_size);
    check(nativeFighterModel_Open(model.get(),&project));
    check(nativeFighterModel_SetJoint(model.get(),bone,0,2,.2f));
    check(nativeFighterModel_CompensateRest(model.get(),old.data(),old.size(),bone));
    const auto after=runtime(project,offset,source,false);check(prior.size()==after.size());
    for(unsigned i=0;i<prior.size();++i)near(prior[i],after[i]);
    check(nativeFighterProject_SaveAssets(folder.string().c_str(),&project,error,sizeof(error)));
    nativeFighterProject_Close(&project);check(nativeFighterProject_Open(folder.string().c_str(),&project));
    runtime(project,offset,source,true);
    check(nativeFighterModel_Open(model.get(),&project));mesh=ordinal(*model,offset);
    NativeSkinMesh saved;check(nativeSkinMesh_Read(*model,mesh,&saved,error,sizeof(error)));
    const auto update=fixtures/(prefix+(binding==NATIVE_GLTF_BIND_SOURCE?"skin-reordered.glb":"reordered.glb"));Source updated(update);
    request={0,model->meshes[mesh].joint,mesh,NATIVE_GLTF_BIND_KEEP,1.25f,false};
    check(nativeFighterGltf_ApplyBatch(&project,update.string().c_str(),&request,1,error,sizeof(error)));
    check(nativeFighterModel_Open(model.get(),&project));mesh=ordinal(*model,offset);NativeSkinMesh current;
    check(nativeSkinMesh_Read(*model,mesh,&current,error,sizeof(error)));
    check(current.parts[0].morph.target_names==std::vector<std::string>({"Wide","Lean","Blink"}));
    for(unsigned i=0;i<saved.vertices.size();++i)check(nativeSkinWeights_Equal(&saved.vertices[i].skin,&current.vertices[i].skin));
    runtime(project,offset,updated,true);
    NativeFighterCoreAssetState stable;check(nativeFighterCoreAssets_Capture(project,stable));
    const char* rejected[][2]={{"missing.glb","missing morph target"},{"bad-count.glb","Could not parse"},
        {"nan.glb","finite vectors"},{"tangent.glb","supports position and normal"}};
    if(!large)for(const auto& item:rejected) {
        check(!nativeFighterGltf_ApplyBatch(&project,(fixtures/item[0]).string().c_str(),&request,1,error,sizeof(error)));
        check(std::strstr(error,item[1])!=nullptr);
        check(nativeFighterCoreAssets_Matches(project,stable));
    }
    nativeFighterProject_Close(&project);
}

void defaults_case(const std::filesystem::path& base,const std::filesystem::path& fixtures,const char* name)
{
    const auto folder=fixtures/(std::string("case-")+name);std::filesystem::create_directory(folder);
    std::filesystem::copy(base,folder,std::filesystem::copy_options::recursive);
    NativeFighterProject project={};check(nativeFighterProject_Open(folder.string().c_str(),&project));
    const auto file=fixtures/(std::string(name)+".glb");Source source(file);
    NativeGltfImportRequest request={0,0,UINT_MAX,NATIVE_GLTF_BIND_RIGID,1.25f,false};
    check(nativeFighterGltf_ApplyBatch(&project,file.string().c_str(),&request,1,error,sizeof(error)));
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    uint32_t offset=0;for(unsigned i=0;i<model->mesh_count;++i) {
        const auto* meta=nativeFighterMeshSource_Get(&project,(uint32_t)model->meshes[i].offset);
        if(meta&&!std::strcmp(meta->identity,"mesh:11:Expressions"))offset=meta->mesh;
    }
    check(offset);runtime(project,offset,source,true);
    check(nativeFighterProject_SaveAssets(folder.string().c_str(),&project,error,sizeof(error)));
    nativeFighterProject_Close(&project);check(nativeFighterProject_Open(folder.string().c_str(),&project));
    runtime(project,offset,source,true);nativeFighterProject_Close(&project);
}

std::vector<u8> authoring(const NativeFighterProject& project)
{
    u8* bytes=nullptr;size_t size=0;check(nativeFighterAuthoring_Encode(&project,&bytes,&size));
    std::vector<u8> result(bytes,bytes+size);std::free(bytes);return result;
}

void attachment_rest_case(const std::filesystem::path& base,const std::filesystem::path& fixtures,
                          unsigned attachment,bool morph,bool painted,bool explicit_root=false)
{
    const auto folder=fixtures/("attachment-rest-"+std::to_string(attachment)+"-"+std::to_string(morph)+"-"+std::to_string(painted)+(explicit_root?"-bound":""));
    std::filesystem::create_directory(folder);
    std::filesystem::copy(base,folder,std::filesystem::copy_options::recursive);
    NativeFighterProject project={};check(nativeFighterProject_Open(folder.string().c_str(),&project));
    const auto path=fixtures/(morph?"original.glb":"ordinary.glb");Source source(path);
    NativeGltfImportRequest request={0,attachment,UINT_MAX,NATIVE_GLTF_BIND_RIGID,1.25f,false};
    check(nativeFighterGltf_ApplyBatch(&project,path.string().c_str(),&request,1,error,sizeof(error)));
    auto model=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(model.get(),&project));
    if(explicit_root) {
        NativeAssetDat dat;check(dat.open(project.costume_dat,project.costume_size));
        check(nativeSkinMesh_EnsureInverseBind(dat,*model,0));std::vector<u8> bytes;check(dat.finish(&bytes));
        auto* copy=(u8*)std::malloc(bytes.size());check(copy);std::memcpy(copy,bytes.data(),bytes.size());
        std::free(project.costume_dat);project.costume_dat=copy;project.costume_size=bytes.size();check(nativeFighterModel_Open(model.get(),&project));
    }
    uint32_t offset=0;for(unsigned i=0;i<model->mesh_count;++i) {
        const auto* meta=nativeFighterMeshSource_Get(&project,(uint32_t)model->meshes[i].offset);
        if(meta&&!std::strcmp(meta->identity,"mesh:11:Expressions"))offset=meta->mesh;
    }
    check(offset);unsigned mesh=ordinal(*model,offset);const unsigned owner=model->meshes[mesh].joint;check(owner==attachment);
    NativeSkinMesh skin;check(nativeSkinMesh_Read(*model,mesh,&skin,error,sizeof(error)));
    NativeSkinBrushPoint center;std::memcpy(center.position,skin.vertices[0].position,sizeof(center.position));unsigned affected;
    if(painted)check(nativeFighterSkin_Paint(&project,mesh,2,&center,1,.01f,.25f,false,&affected,error,sizeof(error))&&affected==1);
    const auto before=runtime(project,offset,source,false);
    NativeFighterCoreAssetState before_rest,after_rest;check(nativeFighterCoreAssets_Capture(project,before_rest));
    const auto before_names=authoring(project);
    for(unsigned channel=0;channel<3;++channel) {
        check(nativeFighterModel_Open(model.get(),&project));std::vector<u8> old(project.costume_dat,project.costume_dat+project.costume_size);
        const float values[]={.65f,1.2f,.75f};
        check(nativeFighterModel_SetJoint(model.get(),owner,channel,channel==0?2:0,values[channel]));
        check(nativeFighterModel_CompensateRest(model.get(),old.data(),old.size(),owner));
    }
    const auto edited=runtime(project,offset,source,false);check(before.size()==edited.size());
    for(unsigned i=0;i<before.size();++i)near(before[i],edited[i]);
    check(nativeFighterCoreAssets_Capture(project,after_rest));const auto after_names=authoring(project);
    const bool correction=owner!=0 || (!morph&&!painted);
    check((before_names!=after_names)==correction);
    check(nativeFighterMeshSource_Get(&project,offset)->has_rest_adjustment==correction);
    // Model history restores both costume bytes and the complete authoring facts.
    check(nativeFighterCoreAssets_Restore(project,after_rest,before_rest,error,sizeof(error)));
    check(nativeFighterAuthoring_Decode(before_names.data(),before_names.size(),&project));
    check(authoring(project)==before_names);const auto undone=runtime(project,offset,source,false);
    for(unsigned i=0;i<before.size();++i)near(before[i],undone[i]);
    check(nativeFighterCoreAssets_Restore(project,before_rest,after_rest,error,sizeof(error)));
    check(nativeFighterAuthoring_Decode(after_names.data(),after_names.size(),&project));check(authoring(project)==after_names);
    check(nativeFighterProject_SaveAssets(folder.string().c_str(),&project,error,sizeof(error)));
    nativeFighterProject_Close(&project);check(nativeFighterProject_Open(folder.string().c_str(),&project));
    check(authoring(project)==after_names);
    check(nativeFighterModel_Open(model.get(),&project));mesh=ordinal(*model,offset);
    request={0,owner,mesh,NATIVE_GLTF_BIND_KEEP,1.25f,false};
    for(unsigned repeat=0;repeat<2;++repeat) {
        check(nativeFighterGltf_ApplyBatch(&project,path.string().c_str(),&request,1,error,sizeof(error)));
        const auto reimported=runtime(project,offset,source,false);check(edited.size()==reimported.size());
        for(unsigned i=0;i<edited.size();++i)near(edited[i],reimported[i]);
    }
    // Compare a new import scale with the same painted fighter before its
    // rest edits. Unit scaling must not scale the later rest correction.
    NativeFighterCoreAssetState current_state,scaled_state;check(nativeFighterCoreAssets_Capture(project,current_state));
    const auto current_names=authoring(project);
    check(nativeFighterCoreAssets_Restore(project,current_state,before_rest,error,sizeof(error)));
    check(nativeFighterAuthoring_Decode(before_names.data(),before_names.size(),&project));
    request.scale=2.5f;check(nativeFighterGltf_ApplyBatch(&project,path.string().c_str(),&request,1,error,sizeof(error)));
    const auto scaled_before=runtime(project,offset,source,false);check(nativeFighterCoreAssets_Capture(project,scaled_state));
    check(nativeFighterCoreAssets_Restore(project,scaled_state,current_state,error,sizeof(error)));
    check(nativeFighterAuthoring_Decode(current_names.data(),current_names.size(),&project));
    check(nativeFighterGltf_ApplyBatch(&project,path.string().c_str(),&request,1,error,sizeof(error)));
    const auto scaled_after=runtime(project,offset,source,false);check(scaled_before.size()==scaled_after.size());
    for(unsigned i=0;i<scaled_before.size();++i)near(scaled_before[i],scaled_after[i]);
    if(owner==0 && !morph && !painted) {
        // A first stroke after a root rest edit changes rigid coordinates into
        // bind coordinates. Its source correction must change with the asset.
        check(nativeFighterModel_Open(model.get(),&project));mesh=ordinal(*model,offset);
        check(nativeSkinMesh_Read(*model,mesh,&skin,error,sizeof(error)));
        std::memcpy(center.position,skin.vertices[0].position,sizeof(center.position));
        NativeFighterCoreAssetState before_paint,after_paint;check(nativeFighterCoreAssets_Capture(project,before_paint));
        const auto prior_names=authoring(project);
        check(nativeFighterSkin_Paint(&project,mesh,2,&center,1,.01f,.25f,false,&affected,error,sizeof(error))&&affected==1);
        const auto painted_world=runtime(project,offset,source,false);
        const auto painted_names=authoring(project);
        check((painted_names!=prior_names)==explicit_root);check(nativeFighterCoreAssets_Capture(project,after_paint));
        check(nativeFighterCoreAssets_Restore(project,after_paint,before_paint,error,sizeof(error)));
        check(nativeFighterAuthoring_Decode(prior_names.data(),prior_names.size(),&project));
        check(nativeFighterCoreAssets_Restore(project,before_paint,after_paint,error,sizeof(error)));
        check(nativeFighterAuthoring_Decode(painted_names.data(),painted_names.size(),&project));
        check(nativeFighterGltf_ApplyBatch(&project,path.string().c_str(),&request,1,error,sizeof(error)));
        const auto imported_world=runtime(project,offset,source,false);check(painted_world.size()==imported_world.size());
        for(unsigned i=0;i<painted_world.size();++i)near(painted_world[i],imported_world[i]);
    }
    // Invalid rest facts reject as a whole and retain the current authoring data.
    const auto names=authoring(project);const std::string encoded(names.begin(),names.end());
    const std::string marker="mesh-rest\t"+std::to_string(offset)+"\t";
    const size_t start=encoded.find(marker),end=encoded.find('\n',start);
    check((start!=std::string::npos)==nativeFighterMeshSource_Get(&project,offset)->has_rest_adjustment);
    const char* invalid[]={"nan,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1","0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1",
        "1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0","1,0,0,0,0,1"};
    if(start!=std::string::npos)for(const char* value:invalid) {
        const auto bad=encoded.substr(0,start)+marker+value+encoded.substr(end);
        check(!nativeFighterAuthoring_Decode((const u8*)bad.data(),bad.size(),&project)&&authoring(project)==names);
    }
    nativeFighterProject_Close(&project);
}
}
int main(int argc,char** argv)
{
    try {check(argc==3);HSD_ObjInit();for(unsigned owner:{0u,1u,4u,7u})for(bool morph:{false,true})for(bool painted:{false,true})
        attachment_rest_case(std::filesystem::absolute(argv[1]),std::filesystem::absolute(argv[2]),owner,morph,painted);
        attachment_rest_case(std::filesystem::absolute(argv[1]),std::filesystem::absolute(argv[2]),0,false,false,true);
        for(bool large:{false,true})
        for(auto binding:{NATIVE_GLTF_BIND_RIGID,NATIVE_GLTF_BIND_AUTOMATIC,NATIVE_GLTF_BIND_SOURCE})
            run(std::filesystem::absolute(argv[1]),std::filesystem::absolute(argv[2]),binding,large);
        for(const char* name:{"mesh-default","zero","anonymous","mirrored"})
            defaults_case(std::filesystem::absolute(argv[1]),std::filesystem::absolute(argv[2]),name);
        std::printf("PASS: %u GLB morph import, signed/default weights, rest, history and reimport checks\n",checks);return 0;
    }catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
}
