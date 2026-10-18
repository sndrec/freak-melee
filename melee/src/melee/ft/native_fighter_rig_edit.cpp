#include "native_fighter_rig_edit.h"
#include "native_fighter_rig.h"
#include "native_fighter_rig_animation.h"
#include "native_fighter_rig_references.h"
#include "native_fighter_rest_matrices.h"
#include "native_fighter_model_references.h"
#include "native_fighter_metal_references.h"
#include "native_fighter_action_editor.h"
#include "native_asset_dat.h"
extern "C" {
#include "native_script_resource_metadata.h"
#include <sysdolphin/baselib/mtx.h>
}
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

namespace {
constexpr uint16_t absent = UINT16_MAX;
constexpr uint32_t missing = UINT32_MAX;
struct Plan {
    unsigned count = 0, old_count = 0;
    uint16_t order[MAX_FT_PARTS], parent[MAX_FT_PARTS];
    uint16_t old_part[MAX_FT_PARTS], new_part[MAX_FT_PARTS];
    uint16_t forward[MAX_FT_PARTS], backward[MAX_FT_PARTS];
    NativeSourceAssetFacts facts;
    NativeFighterRig rig;
};
struct Candidate {
    NativeFighterProject project;
    explicit Candidate(const NativeFighterProject& source):project(source)
    {project.fighter_dat=nullptr;project.animation_dat=nullptr;project.costume_dat=nullptr;project.asset_record=nullptr;}
    ~Candidate(){std::free(project.fighter_dat);std::free(project.animation_dat);std::free(project.costume_dat);std::free(project.asset_record);}
};
bool fail(char* text,size_t capacity,const char* message)
{if(text&&capacity)std::snprintf(text,capacity,"%s",message);return false;}
bool pointer(const HSD_Dat& dat,uint32_t slot,uint32_t* output,bool optional=true)
{
    u32 at;const char* name;auto kind=HSD_DatResolve(&dat,slot,&at,&name);
    if(optional&&kind==HSD_DAT_REF_NULL){*output=missing;return true;}
    if(kind!=HSD_DAT_REF_LOCAL)return false;*output=(uint32_t)at;return true;
}
bool copy_bytes(const void* source,size_t size,uint8_t** output)
{
    if(!source||!size||size>NATIVE_CONTENT_MAX_PACKAGE_BYTES)return false;
    *output=(uint8_t*)std::malloc(size);if(!*output)return false;
    std::memcpy(*output,source,size);return true;
}
bool reservations(const NativeFighterProject& project,const NativeFighterRig& rig,
                   bool used[MAX_FT_PARTS],char* error,size_t capacity)
{
    if(!project.script_size)return true;
    if(!project.script_text||project.script_size>NATIVE_CONTENT_MAX_PACKAGE_BYTES)
        return fail(error,capacity,"The fighter's script source is unavailable.");
    // The syntax tree has at most two nodes per source line. This authoring
    // allocation does not compile callbacks or require valid expressions.
    unsigned lines=2;for(size_t n=0;n<project.script_size;++n)lines+=project.script_text[n]=='\n';
    std::vector<NativeScriptSourceNode> nodes((size_t)lines*2);
    NativeScriptSourceTree tree={(const char*)project.script_text,(uint32_t)project.script_size,
                                nodes.data(),0,(uint32_t)nodes.size()};
    NativeScriptDiagnostic diagnostic={};
    if(!nativeScriptSource_Parse(&tree,&diagnostic))return fail(error,capacity,diagnostic.message);
    for(uint32_t n=1;n<tree.count;++n)
        if(tree.nodes[n].kind==NativeScriptSource_Article||tree.nodes[n].kind==NativeScriptSource_Item){
            NativeScriptResourceRecipe recipe;
            if(!nativeScriptResourceMetadata_Prepare(&tree,n,&project.facts,&rig,&recipe,&diagnostic))
                return fail(error,capacity,diagnostic.message);
            if(recipe.reserved_part>=0)used[recipe.reserved_part]=true;
        }
    return true;
}
bool hierarchy(const NativeFighterModelEditor& model,const NativeFighterRig& rig,
               const NativeFighterRigEdit& edit,Plan& plan,char* error,size_t capacity)
{
    const unsigned old=model.joint_count,body=rig.count;
    if(!old||old!=model.project->facts.joints||old>MAX_FT_PARTS||body>MAX_FT_PARTS)
        return fail(error,capacity,"The costume hierarchy disagrees with its owned rig.");
    bool hole[MAX_FT_PARTS]={};
    for(unsigned n=0;n<model.project->facts.omission_count;++n){
        unsigned at=model.project->facts.omissions[n].joint;
        if(at>=body||hole[at])return fail(error,capacity,"Reserved rig parts have invalid positions.");hole[at]=true;
    }
    for(unsigned p=0,n=0;p<body;++p)if(!hole[p]){
        if(n==old)return fail(error,capacity,"The physical rig has too many costume parts.");
        plan.old_part[n++]=(uint16_t)p;
    }
    for(unsigned n=0;n<old;++n)if(model.joints[n].flags&((1u<<12)|(1u<<17)))
        return fail(error,capacity,"Hierarchy editing requires ordinary Euler joints without instances.");
    auto ordinal=[&](int32_t id)->int{
        int p=nativeFighterRig_Resolve(&rig,id);
        for(unsigned n=0;n<old;++n)if(p==plan.old_part[n])return (int)n;return -1;
    };
    const int selected=ordinal(edit.bone_id),destination=ordinal(edit.parent_id);
    if(edit.kind!=NativeRigEdit_Remove&&destination<0)
        return fail(error,capacity,"Choose a parent bone in the costume hierarchy.");
    if(edit.kind!=NativeRigEdit_Add&&(selected<=0||!(rig.flags[plan.old_part[selected]]&NATIVE_RIG_EXTRA)))
        return fail(error,capacity,"Only authored extra bones can be moved or removed.");
    if(edit.kind==NativeRigEdit_Add){
        if(body==MAX_FT_PARTS||rig.next_id==INT32_MAX||!edit.name[0]||strnlen(edit.name,64)==64)
            return fail(error,capacity,"Adding a bone needs a unique name and a free native part slot.");
    }else if(edit.kind!=NativeRigEdit_Reparent&&edit.kind!=NativeRigEdit_Remove)
        return fail(error,capacity,"Unknown hierarchy edit.");
    if(edit.kind==NativeRigEdit_Reparent){
        for(int at=destination;at>=0;at=model.joints[at].parent)
            if(at==selected)return fail(error,capacity,"A bone cannot be parented to itself or its descendants.");
        if(model.joints[selected].parent==destination)return fail(error,capacity,"That bone already has this parent.");
    }
    std::vector<unsigned> children[MAX_FT_PARTS+1];
    const unsigned roots=MAX_FT_PARTS;
    for(unsigned n=0;n<old;++n){int p=model.joints[n].parent;
        if(p>=int(n)||p< -1)return fail(error,capacity,"The costume has invalid parent ordering.");
        children[p<0?roots:(unsigned)p].push_back(n);
    }
    if(edit.kind==NativeRigEdit_Add)children[destination].push_back(old);
    else {
        int parent=model.joints[selected].parent;
        if(parent<0)return fail(error,capacity,"A root bone cannot be moved or removed.");
        auto& siblings=children[parent];auto at=std::find(siblings.begin(),siblings.end(),(unsigned)selected);
        if(at==siblings.end())return false;
        if(edit.kind==NativeRigEdit_Reparent){siblings.erase(at);children[destination].push_back((unsigned)selected);}
        else {at=siblings.erase(at);siblings.insert(at,children[selected].begin(),children[selected].end());children[selected].clear();}
    }
    auto walk=[&](auto&& self,unsigned node,uint16_t parent,unsigned depth)->bool{
        if(plan.count==MAX_FT_PARTS||depth>=30)return false;
        const unsigned index=plan.count++;plan.order[index]=node==old?absent:(uint16_t)node;plan.parent[index]=parent;
        for(unsigned child:children[node])if(!self(self,child,(uint16_t)index,depth+1))return false;
        return true;
    };
    for(unsigned root:children[roots])if(!walk(walk,root,absent,0))
        return fail(error,capacity,"The edited hierarchy exceeds native part or depth limits.");
    plan.old_count=old;plan.facts=model.project->facts;plan.facts.joints=(uint16_t)plan.count;
    // Retain each omission immediately before its next retained body anchor.
    // The omission's descriptor identity/order (and therefore mask bit) stays.
    uint16_t anchor[NATIVE_SOURCE_OMISSIONS];
    for(unsigned n=0;n<plan.facts.omission_count;++n){
        anchor[n]=absent;
        for(unsigned i=0;i<old;++i)if(plan.old_part[i]>plan.facts.omissions[n].joint){
            bool retained=false;for(unsigned j=0;j<plan.count;++j)retained|=plan.order[j]==i;
            if(retained){anchor[n]=(uint16_t)i;break;}
        }
    }
    std::fill_n(plan.forward,MAX_FT_PARTS,absent);std::fill_n(plan.backward,MAX_FT_PARTS,absent);
    unsigned part=0;
    for(unsigned n=0;n<=plan.count;++n){
        for(unsigned h=0;h<plan.facts.omission_count;++h)
            if(anchor[h]==(n==plan.count?absent:plan.order[n]) &&
               (n==plan.count||plan.order[n]!=absent)){
                unsigned previous=model.project->facts.omissions[h].joint;
                plan.forward[previous]=(uint16_t)part;plan.backward[part]=(uint16_t)previous;
                plan.facts.omissions[h].joint=(uint8_t)part++;
            }
        if(n==plan.count)break;
        plan.new_part[n]=(uint16_t)part;
        if(plan.order[n]!=absent){unsigned previous=plan.old_part[plan.order[n]];
            plan.forward[previous]=(uint16_t)part;plan.backward[part]=(uint16_t)previous;}
        ++part;
    }
    if(part!=plan.count+plan.facts.omission_count||part>MAX_FT_PARTS)return false;
    for(unsigned n=0;n<plan.facts.bone_count;++n){
        unsigned p=plan.forward[plan.facts.bones[n].joint];
        if(p==absent)return fail(error,capacity,"A required gameplay bone cannot be removed.");plan.facts.bones[n].joint=(uint8_t)p;
    }
    for(unsigned n=0;n<plan.facts.omission_count;++n){
        unsigned p=plan.forward[plan.facts.omissions[n].parent];
        if(p==absent)return fail(error,capacity,"A reserved part's parent cannot be removed.");plan.facts.omissions[n].parent=(uint8_t)p;
    }
    return true;
}
bool close_matrix(const Mtx a,const Mtx b)
{
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
        if(!std::isfinite(a[r][c])||!std::isfinite(b[r][c])||
           std::fabs(a[r][c]-b[r][c])>0.0001f*(1.0f+std::fabs(b[r][c])))return false;
    return true;
}
void srt(const NativeModelJoint& bone,const Vec3* parent,Mtx output)
{
    Vec3 s={bone.scale[0],bone.scale[1],bone.scale[2]},r={bone.rotation[0],bone.rotation[1],bone.rotation[2]},
         t={bone.position[0],bone.position[1],bone.position[2]};
    HSD_MtxSRT(output,&s,&r,&t,const_cast<Vec3*>(parent));
}
void real(NativeAssetDat& dat,uint32_t at,float value)
{uint32_t bits;std::memcpy(&bits,&value,4);dat.word(at,bits);}
bool transforms(const NativeFighterModelEditor& before,const Plan& plan,NativeFighterModelEditor& after,
                char* error,size_t capacity)
{
    std::unique_ptr<Mtx[]> old(new Mtx[before.joint_count]),world(new Mtx[plan.count]);
    Vec3 accumulated[MAX_FT_PARTS];
    if(!nativeFighterModel_RestMatrices(&before,old.get(),nullptr))return fail(error,capacity,"Could not calculate the original rest transforms.");
    after.joint_count=(uint16_t)plan.count;
    for(unsigned n=0;n<plan.count;++n){
        auto& bone=after.joints[n];const unsigned from=plan.order[n],parent=plan.parent[n];
        if(from!=absent)bone=before.joints[from];else {bone={};bone.flags=1;bone.scale[0]=bone.scale[1]=bone.scale[2]=1.0f;}
        bone.parent=parent==absent?-1:(int16_t)parent;bone.depth=parent==absent?0:after.joints[parent].depth+1;
        const Vec3* compensation=parent==absent?nullptr:&accumulated[parent];
        Mtx local;
        if(from!=absent){
            Mtx desired,inverse;
            if(parent==absent)std::memcpy(desired,old[from],sizeof(desired));
            else {if(!C_MTXInverse(world[parent],inverse))return fail(error,capacity,"A parent rest transform cannot be inverted.");C_MTXConcat(inverse,old[from],desired);}
            srt(bone,compensation,local);
            if(!close_matrix(local,desired)){
                // HSD's parent-scale compensation gives L = P^-1 R P S.
                // P L P^-1 has ordinary rotation/scale columns.
                const float p[3]={compensation?compensation->x:1.0f,compensation?compensation->y:1.0f,compensation?compensation->z:1.0f};
                Mtx rotation;float scale[3];
                for(unsigned c=0;c<3;++c){float sum=0;
                    if(std::fabs(p[c])<0.000001f)return fail(error,capacity,"A parent has a zero rest scale.");
                    for(unsigned r=0;r<3;++r){rotation[r][c]=p[r]*desired[r][c]/p[c];sum+=rotation[r][c]*rotation[r][c];}
                    scale[c]=std::sqrt(sum);if(scale[c]<0.000001f)return fail(error,capacity,"The new rest transform has a zero scale.");
                }
                const float determinant=rotation[0][0]*(rotation[1][1]*rotation[2][2]-rotation[1][2]*rotation[2][1])-
                    rotation[0][1]*(rotation[1][0]*rotation[2][2]-rotation[1][2]*rotation[2][0])+
                    rotation[0][2]*(rotation[1][0]*rotation[2][1]-rotation[1][1]*rotation[2][0]);
                if(determinant<0)scale[0]=-scale[0];
                for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)rotation[r][c]/=scale[c];rotation[r][3]=0;}
                Vec3 angles;HSD_MtxGetRotation(rotation,&angles);
                const float a[3]={angles.x,angles.y,angles.z};
                for(unsigned axis=0;axis<3;++axis){bone.rotation[axis]=a[axis];bone.scale[axis]=scale[axis];bone.position[axis]=desired[axis][3];}
                srt(bone,compensation,local);
                if(!close_matrix(local,desired))return fail(error,capacity,"This parent change needs shear, which Melee joints cannot store.");
            }
        }else srt(bone,compensation,local);
        if(parent==absent)std::memcpy(world[n],local,sizeof(local));else C_MTXConcat(world[parent],local,world[n]);
        if(from!=absent&&!close_matrix(world[n],old[from]))return fail(error,capacity,"The edit could not preserve a bone's world rest transform.");
        const float p[3]={compensation?compensation->x:1.0f,compensation?compensation->y:1.0f,compensation?compensation->z:1.0f};
        accumulated[n]={p[0],p[1],p[2]};
        if(!(bone.flags&8)){accumulated[n].x*=bone.scale[0];accumulated[n].y*=bone.scale[1];accumulated[n].z*=bone.scale[2];}
    }
    return true;
}
// Ordinary joint forests use offsets 8/12; animation trees use 0/4. Original
// object offsets remain stable, preserving envelopes, constraints and labels.
bool tree_nodes(const HSD_Dat& dat,uint32_t root,unsigned child,unsigned next,unsigned bytes,
                std::vector<uint32_t>& nodes,std::vector<int>& parents)
{
    auto walk=[&](auto&& self,uint32_t at,int parent,unsigned depth)->bool{
        if(depth>=30)return false;
        for(;at!=missing;){
            if(nodes.size()==MAX_FT_PARTS||!HSD_DatBytes(&dat,at,bytes)||std::find(nodes.begin(),nodes.end(),at)!=nodes.end())return false;
            int index=(int)nodes.size();nodes.push_back(at);parents.push_back(parent);
            uint32_t sub,sibling;if(!pointer(dat,at+child,&sub)||!pointer(dat,at+next,&sibling))return false;
            if(sub!=missing&&!self(self,sub,index,depth+1))return false;at=sibling;
        }
        return true;
    };
    return walk(walk,root,-1,0);
}
void links(NativeAssetDat& dat,const std::vector<uint32_t>& nodes,const uint16_t* parents,
            unsigned child,unsigned next)
{
    for(unsigned n=0;n<nodes.size();++n){
        uint32_t first=missing,sibling=missing;
        for(unsigned j=n+1;j<nodes.size();++j){
            if(parents[j]==n&&first==missing)first=nodes[j];
            if(parents[j]==parents[n]){sibling=nodes[j];break;}
        }
        if(first==missing)dat.link(nodes[n]+child,0);else dat.link_local(nodes[n]+child,first);
        if(sibling==missing)dat.link(nodes[n]+next,0);else dat.link_local(nodes[n]+next,sibling);
    }
}
bool removed_references(const HSD_Dat& dat,const std::vector<uint32_t>& nodes,const Plan& plan,
                         unsigned child,unsigned next,char* error,size_t capacity)
{
    for(unsigned n=0;n<nodes.size();++n){
        bool retained=false;for(unsigned j=0;j<plan.count;++j)retained|=plan.order[j]==n;
        if(retained)continue;
        if(child==8){uint32_t mesh,constraint;
            if(!pointer(dat,nodes[n]+16,&mesh)||!pointer(dat,nodes[n]+60,&constraint))return false;
            if(mesh!=missing||constraint!=missing)return fail(error,capacity,"Detach this bone's meshes and constraints before removing it.");
        }
        for(u32 r=0;r<dat.relocation_count;++r){
            u32 slot,target;const char* name;if(!HSD_DatRelocation(&dat,r,&slot))return false;
            if(HSD_DatResolve(&dat,slot,&target,&name)!=HSD_DAT_REF_LOCAL||target!=nodes[n])continue;
            bool structural=false;for(uint32_t bone:nodes)structural|=slot==bone+child||slot==bone+next;
            if(!structural)return fail(error,capacity,"The removed bone is still referenced by a mesh, constraint or other asset.");
        }
    }
    return true;
}
bool joint_tree(NativeAssetDat& dat,uint32_t root,const Plan& plan,const NativeFighterModelEditor& after,
                 std::vector<uint32_t>& output,bool costume,char* error,size_t capacity)
{
    std::vector<uint32_t> old;std::vector<int> parents;
    if(!tree_nodes(dat.original,root,8,12,64,old,parents)||old.size()!=plan.old_count)
        return fail(error,capacity,"A parallel model skeleton disagrees with the costume hierarchy.");
    if(!removed_references(dat.original,old,plan,8,12,error,capacity))return false;
    for(unsigned n=0;n<old.size();++n){bool retained=false;
        for(unsigned j=0;j<plan.count;++j)retained|=plan.order[j]==n;
        if(!retained){dat.link(old[n]+8,0);dat.link(old[n]+12,0);}
    }
    std::unique_ptr<Mtx[]> world;
    if(costume){world.reset(new Mtx[plan.count]);if(!nativeFighterModel_RestMatrices(&after,world.get(),nullptr))return false;}
    for(unsigned n=0;n<plan.count;++n){
        unsigned from=plan.order[n];uint32_t at=from==absent?dat.allocate(64):old[from];
        if(at==missing)return false;output.push_back(at);
        if(from==absent){
            dat.word(at+4,1);
            if(costume){Mtx inverse;if(!C_MTXInverse(world[n],inverse))return false;
                uint32_t bind=dat.allocate(sizeof(Mtx));if(bind==missing)return false;dat.link_local(at+56,bind);
                for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)real(dat,bind+(r*4+c)*4,inverse[r][c]);
            }
        }
        if(costume||from==absent)for(unsigned axis=0;axis<3;++axis){
            real(dat,at+20+axis*4,after.joints[n].rotation[axis]);real(dat,at+32+axis*4,after.joints[n].scale[axis]);real(dat,at+44+axis*4,after.joints[n].position[axis]);
        }
    }
    links(dat,output,plan.parent,8,12);
    return true;
}
bool material_tree(NativeAssetDat& dat,uint32_t root,const NativeFighterModelEditor& before,
                    const Plan& plan,char* error,size_t capacity)
{
    uint32_t slots[MAX_FT_PARTS];std::fill_n(slots,MAX_FT_PARTS,missing);unsigned visited=0;
    auto walk=[&](auto&& self,uint32_t at,int parent,unsigned depth)->bool{
        if(depth>=30)return false;
        for(unsigned n=0;n<before.joint_count;++n)if(before.joints[n].parent==parent){
            if(at==missing)break;
            if(++visited>MAX_FT_PARTS||!HSD_DatBytes(&dat.original,at,12))return false;
            for(unsigned i=0;i<n;++i)if(slots[i]==at)return false;
            slots[n]=at;uint32_t child,sibling;
            if(!pointer(dat.original,at,&child)||!pointer(dat.original,at+4,&sibling)||!self(self,child,(int)n,depth+1))return false;
            at=sibling;
        }
        return at==missing;
    };
    if(!walk(walk,root,-1,0))return fail(error,capacity,"The material animation tree does not match the original rig.");
    std::vector<uint32_t> nodes;
    for(unsigned n=0;n<plan.count;++n){
        unsigned old=plan.order[n];uint32_t at=old==absent?missing:slots[old];
        if(at==missing)at=dat.allocate(12);if(at==missing)return false;nodes.push_back(at);
    }
    links(dat,nodes,plan.parent,0,4);return true;
}
bool partial_animations(NativeAssetDat& dat,const NativeFighterProject& source,
                         const Plan& plan,char* error,size_t capacity)
{
    HSD_Dat original;u32 root;uint32_t groups,end;
    if(HSD_DatOpen(&original,source.fighter_dat,source.fighter_size)!=HSD_DAT_OK||
       !HSD_DatFindRoot(&original,source.fighter_root,&root)||
       !pointer(original,root+28,&groups,false)||!pointer(original,root+32,&end,false)||
       end<groups||(end-groups)%4||(end-groups)/4>5)return false;
    struct Shared {uint32_t root,destination;unsigned begin,end;std::vector<uint16_t> order;};
    std::vector<Shared> shared;
    for(uint32_t slot=groups;slot<end;slot+=4){
        uint32_t group;if(!pointer(original,slot,&group))return false;if(group==missing)continue;
        u16 first;uint32_t variants;
        if(!HSD_DatReadU16(&original,group,&first)||!pointer(original,group+8,&variants,false))return false;
        unsigned start=plan.old_count;for(unsigned n=0;n<plan.old_count;++n)if(plan.old_part[n]==first)start=n;
        if(start==plan.old_count)return fail(error,capacity,"A partial animation starts at a reserved or missing body bone.");
        unsigned group_start=MAX_FT_PARTS;
        for(unsigned v=0;v<8;++v){
            uint32_t graph;u32 ignored;const char* external;
            auto kind=HSD_DatResolve(&original,variants+v*4,&ignored,&external);
            if(kind==HSD_DAT_REF_NULL||kind==HSD_DAT_REF_INVALID)break;
            if(kind==HSD_DAT_REF_EXTERNAL)continue;graph=(uint32_t)ignored;
            bool descriptor=true;for(unsigned k=0;k<4;++k)
                descriptor&=HSD_DatResolve(&original,graph+k*4,&ignored,&external)!=HSD_DAT_REF_INVALID;
            if(!descriptor)break; // Same packed-table boundary as the importer.
            std::vector<uint32_t> nodes;std::vector<int> parents;
            if(!tree_nodes(original,graph,0,4,20,nodes,parents)||nodes.empty()||nodes.size()>plan.old_count-start)
                return fail(error,capacity,"A partial animation exceeds the body bones it addresses.");
            unsigned begin=plan.count,last=0;
            for(unsigned n=0;n<plan.count;++n)if(plan.order[n]>=start&&plan.order[n]<start+nodes.size()){
                begin=std::min(begin,n);last=std::max(last,n);
            }
            if(begin==plan.count)return fail(error,capacity,"A partial animation cannot lose all of its addressed bones.");
            if(group_start!=MAX_FT_PARTS&&group_start!=plan.new_part[begin])
                return fail(error,capacity,"Partial animation variants require conflicting starting bones.");
            group_start=plan.new_part[begin];
            std::vector<uint16_t> order;
            for(unsigned n=begin;n<=last;++n){
                unsigned old=plan.order[n];order.push_back(old>=start&&old<start+nodes.size()?(uint16_t)(old-start):absent);
            }
            auto found=std::find_if(shared.begin(),shared.end(),[&](const Shared& row){return row.root==graph;});
            uint32_t destination;
            if(found!=shared.end()){
                if(found->begin!=begin||found->end!=last||found->order!=order)
                    return fail(error,capacity,"A shared partial animation requires conflicting bone maps.");
                destination=found->destination;
            }else{
                std::vector<uint32_t> output;
                for(uint16_t old:order){
                    uint32_t at=dat.allocate(20);if(at==missing)return false;output.push_back(at);
                    if(old!=absent){
                        std::memcpy(dat.data.data()+at,original.data+nodes[old],20);
                        for(unsigned field:{8u,12u}){uint32_t target;
                            if(!pointer(original,nodes[old]+field,&target))
                                return fail(error,capacity,"A partial animation has an unresolved external callback graph.");
                            if(target==missing)dat.link(at+field,0);else dat.link_local(at+field,target);
                        }
                    }
                }
                for(unsigned n=0;n<output.size();++n){
                    dat.link(output[n],0);if(n+1<output.size())dat.link_local(output[n]+4,output[n+1]);else dat.link(output[n]+4,0);
                }
                destination=output.front();shared.push_back({graph,destination,begin,last,std::move(order)});
            }
            dat.link_local(variants+v*4,destination);
        }
        if(group_start!=MAX_FT_PARTS)dat.half(group,(uint16_t)group_start);
    }
    return true;
}
} // namespace

extern "C" bool nativeFighterRig_Edit(NativeFighterProject* project,const NativeFighterRigEdit* edit,
    int32_t* result,char* error,size_t capacity) try
{
    if(!project||!edit)return fail(error,capacity,"Hierarchy editing needs a draft and an edit.");
    if(!nativeFighterProject_ValidateAssets(project,error,capacity))return false;
    auto before=std::make_unique<NativeFighterModelEditor>(),after=std::make_unique<NativeFighterModelEditor>();
    HSD_Dat fighter;NativeFighterRig source;Plan plan={};
    if(!nativeFighterModel_Open(before.get(),project)||
       HSD_DatOpen(&fighter,project->fighter_dat,project->fighter_size)!=HSD_DAT_OK||
       !nativeFighterRig_Read(&fighter,project->facts.joints+project->facts.omission_count,&source,error,capacity)||
       !hierarchy(*before,source,*edit,plan,error,capacity)||!transforms(*before,plan,*after,error,capacity))return false;
    Candidate candidate(*project);auto& next=candidate.project;
    if(!copy_bytes(project->fighter_dat,project->fighter_size,&next.fighter_dat))return fail(error,capacity,"Could not allocate the hierarchy transaction.");
    if(!nativeFighterAction_OwnHitboxFlags(&next,nullptr,error,capacity))return false;
    next.facts=plan.facts;
    const unsigned body=next.facts.joints+next.facts.omission_count;
    bool reserved[MAX_FT_PARTS]={};
    if(!reservations(*project,source,reserved,error,capacity))return false;
    for(unsigned n=source.count;n<MAX_FT_PARTS;++n)plan.forward[n]=(uint16_t)n;
    // Growing the body consumes one spare physical slot. Preserve an occupied
    // reservation's stable ID by relocating it; retire only an unused spare.
    if(body>source.count){
        const unsigned consumed=source.count;
        bool admitted=false;
        if(!reserved[consumed]){
            plan.forward[consumed]=absent;NativeFighterRigReferences probe={};
            admitted=nativeFighterRig_RemapReferences(&next,plan.forward,&probe,error,capacity);
            std::free(probe.fighter);
        }
        for(unsigned retire=MAX_FT_PARTS;!admitted&&retire-->body;){
            if(reserved[retire])continue;
            plan.forward[consumed]=(uint16_t)retire;
            plan.forward[retire]=absent;
            NativeFighterRigReferences probe={};
            if(nativeFighterRig_RemapReferences(&next,plan.forward,&probe,error,capacity)){std::free(probe.fighter);admitted=true;break;}
            plan.forward[retire]=(uint16_t)retire;
        }
        if(!admitted){if(error&&capacity&&error[0])return false;return fail(error,capacity,"No unused part slot is available without changing a live fighter reference.");}
    }
    NativeFighterRigReferences references={};
    if(!nativeFighterRig_RemapReferences(&next,plan.forward,&references,error,capacity))return false;
    std::free(next.fighter_dat);next.fighter_dat=references.fighter;next.fighter_size=references.fighter_size;
    plan.rig=source;plan.rig.count=(uint16_t)body;
    for(unsigned n=0;n<MAX_FT_PARTS;++n){plan.rig.ids[n]=missing;plan.rig.flags[n]=0;plan.rig.names[n][0]=0;}
    for(unsigned n=0;n<MAX_FT_PARTS;++n)if(plan.forward[n]!=absent){unsigned at=plan.forward[n];
        plan.rig.ids[at]=source.ids[n];plan.rig.flags[at]=source.flags[n];std::memcpy(plan.rig.names[at],source.names[n],64);
    }
    int32_t selected=edit->bone_id;
    if(edit->kind==NativeRigEdit_Remove)for(unsigned n=0;n<before->joint_count;++n)
        if(source.ids[plan.old_part[n]]==(uint32_t)edit->bone_id&&before->joints[n].parent>=0)
            selected=nativeFighterRig_Id(&source,plan.old_part[before->joints[n].parent]);
    if(edit->kind==NativeRigEdit_Add)for(unsigned n=0;n<plan.count;++n)if(plan.order[n]==absent){
        unsigned at=plan.new_part[n];selected=(int32_t)plan.rig.next_id++;
        plan.rig.ids[at]=(uint32_t)selected;plan.rig.flags[at]=NATIVE_RIG_EXTRA;std::memcpy(plan.rig.names[at],edit->name,64);
    }
    if(!nativeFighterRig_Index(&plan.rig,error,capacity))return false;
    // Animation mapping consumes the old facts, even though reference fields
    // and command storage have already been rewritten in this private copy.
    NativeFighterProject bank_source=next;bank_source.facts=project->facts;
    bank_source.animation_dat=project->animation_dat;bank_source.animation_size=project->animation_size;
    NativeFighterRigAnimation bank={};
    if(!nativeFighterRig_RemapAnimationBank(&bank_source,&plan.facts,plan.backward,body,&bank,error,capacity))return false;
    std::free(next.fighter_dat);next.fighter_dat=bank.fighter;next.fighter_size=bank.fighter_size;
    next.animation_dat=bank.animation;next.animation_size=bank.animation_size;next.facts=plan.facts;
    NativeAssetDat costume,metal;
    if(!costume.open(project->costume_dat,project->costume_size)||!metal.open(next.fighter_dat,next.fighter_size))return false;
    u32 costume_root,fighter_root;uint32_t metal_root;
    if(!HSD_DatFindRoot(&costume.original,project->facts.costume_joint,&costume_root)||
       !HSD_DatFindRoot(&metal.original,project->fighter_root,&fighter_root)||!pointer(metal.original,fighter_root+92,&metal_root))return false;
    std::vector<uint32_t> costume_nodes,metal_nodes,old_metal,retained_metal;
    if(!joint_tree(costume,(uint32_t)costume_root,plan,*after,costume_nodes,true,error,capacity))return false;
    for(unsigned n=0;n<plan.count;++n)after->joints[n].offset=costume_nodes[n];
    if(project->facts.costume_matanim[0]){u32 root;
        if(!HSD_DatFindRoot(&costume.original,project->facts.costume_matanim,&root)||
           !material_tree(costume,(uint32_t)root,*before,plan,error,capacity))return false;
    }
    if(metal_root!=missing){
        if(!joint_tree(metal,metal_root,plan,*after,metal_nodes,false,error,capacity))return false;
        std::vector<int> parents;
        if(!tree_nodes(metal.original,metal_root,8,12,64,old_metal,parents))return false;
        for(unsigned n=0;n<plan.count;++n)if(plan.order[n]!=absent)retained_metal.push_back(old_metal[plan.order[n]]);
    }
    // Partial action/pose graphs need their own interval maps. Never publish
    // an insertion through a graph whose positional correspondence is unknown.
    if(project->facts.pose_count)return fail(error,capacity,"This fighter has alternate pose graphs that still need hierarchy remapping.");
    if(!partial_animations(metal,*project,plan,error,capacity))return false;
    std::vector<uint8_t> bytes;
    if(!costume.finish(&bytes)||!copy_bytes(bytes.data(),bytes.size(),&next.costume_dat))return false;
    next.costume_size=bytes.size();bytes.clear();
    if(!metal.finish(&bytes))return false;
    std::free(next.fighter_dat);next.fighter_dat=nullptr;
    if(!copy_bytes(bytes.data(),bytes.size(),&next.fighter_dat))return false;next.fighter_size=bytes.size();
    if(!nativeFighterRig_Write(&next,&plan.rig,error,capacity)||!nativeFighterModel_Open(after.get(),&next)||
       !nativeFighterModel_RemapSelectors(before.get(),after.get(),&next))return fail(error,capacity,"Could not preserve the edited model's draw selectors.");
    if(metal_root!=missing){
        NativeAssetDat selectors;
        if(!selectors.open(next.fighter_dat,next.fighter_size)||
           !nativeFighterModel_RemapMetalSelectors(fighter,project->fighter_root,
               old_metal.data(),old_metal.size(),retained_metal.data(),retained_metal.size(),
               selectors,error,capacity)||!selectors.finish(&bytes))return false;
        std::free(next.fighter_dat);next.fighter_dat=nullptr;
        if(!copy_bytes(bytes.data(),bytes.size(),&next.fighter_dat))return false;
        next.fighter_size=bytes.size();
    }
    NativeScriptDiagnostic diagnostic={};std::vector<uint8_t> facts(NATIVE_SOURCE_FACT_BYTES);size_t size;
    if(!nativeSourceAssets_Write(&next.facts,facts.data(),facts.size(),&size,&diagnostic)||
       !copy_bytes(facts.data(),size,&next.asset_record))return fail(error,capacity,"Could not write the edited rig facts.");
    next.asset_record_size=size;
    if(!nativeFighterProject_ValidateAssets(&next,error,capacity))return false;
    std::free(project->fighter_dat);std::free(project->animation_dat);std::free(project->costume_dat);std::free(project->asset_record);
    project->fighter_dat=next.fighter_dat;project->fighter_size=next.fighter_size;next.fighter_dat=nullptr;
    project->animation_dat=next.animation_dat;project->animation_size=next.animation_size;next.animation_dat=nullptr;
    project->costume_dat=next.costume_dat;project->costume_size=next.costume_size;next.costume_dat=nullptr;
    project->asset_record=next.asset_record;project->asset_record_size=next.asset_record_size;next.asset_record=nullptr;
    project->facts=next.facts;if(result)*result=selected;if(error&&capacity)error[0]=0;return true;
}
catch(const std::bad_alloc&)
{return fail(error,capacity,"Could not allocate storage for the hierarchy edit. The draft is unchanged.");}
