// Actual runtime/heap algorithms; only their private transition input tables
// are staged here. Immutable package/controller state stays outside rollback.
#pragma clang section data = ".data"
#include <melee/ft/native_fighter_runtime.c>
#pragma clang section data = ".mstate$m"
#include <melee/lb/lbheap.c>
#pragma clang section data = ".data"
#include <melee/it/native_article_resource_internal.h>
#include <melee/it/native_article_resource_scene.h>
#include <melee/it/native_article_model_scene.h>
#include <melee/it/native_article_resource_model.h>
#include <melee/it/native_article_resource_animation.h>
#include <melee/it/native_article_model_pool.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/itarticle_data.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/kinds/ftSamus/types.h>
#include <melee/ft/native_source_motion_policy.h>
#include <melee/gm/native_rollback_regions.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/mtx.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/fobj.h>
#include <melee/lb/lb_00B0.h>
#include <stdio.h>
#include <melee/ef/efsync.h>
#include <melee/mp/mplib.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/native_class_snapshot.h>
#include <sysdolphin/baselib/native_id_snapshot.h>
#include <sysdolphin/baselib/native_gobj_snapshot.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <dolphin/os/OSHeapSnapshot.h>
#include <placeholder.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static unsigned checks,cases,graphs,sweeps,frames,restores,line_comparisons,line_refusals,line_vm_cases,constructor_cleanup_cases;
static NativeSourcePackage candidate;
static NativeFighterProject project;
static NativeScriptDiagnostic diagnostic;
static Item* data;
static Fighter* fighter_data;
static Fighter_GObj* fighter_object;
static Item_GObj* objects[2];
static NativeRollbackRegionSet regions;
static Item_GObj* swept_owner;
static bool release_during_sweep,animation_enabled,animated_graphs,samus_graphs;
static bool samus_flight_geometry;
static uint32_t samus_contact_flags;
static const char* samus_source_path;
static char source_buffer[262144];
_Static_assert(offsetof(ftSs_DatAttrs,tether_hang_frames)==0xD0,"Samus hang duration must retain its retail offset");
_Static_assert(sizeof(ftSs_DatAttrs)==0xD4,"Samus special attributes contain scalar values, not a host pointer");
static unsigned animation_pairs,animation_refusals;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"resources %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)

// Geometry/effect controls are explicit external leaves in the copied resource
// TU. Native collision initialization, resource dispatch/order and lifetimes
// are unchanged. This callback intentionally revokes collection while in use.
bool fixtureSweepStage(CollData* collision)
{
    ++sweeps;collision->env_flags=0x1234;collision->floor.index=17;
    if(samus_flight_geometry){
        CHECK(!release_during_sweep);
        collision->cur_pos.x+=0.125f;collision->cur_pos.y-=0.25f;
        collision->env_flags=samus_contact_flags;collision->floor.index=7;
        collision->left_facing_wall.index=5;collision->right_facing_wall.index=6;
        collision->floor.normal=(Vec3){0,1,0};
        return (samus_contact_flags&Collide_FloorMask)!=0;
    }
    if(release_during_sweep){
        ResourceCollection* set=nativeArticleResource_Collection(swept_owner);CHECK(set);
        CHECK(set->active_operations==1);ItemLink* payload=set->nodes[0].link;
        nativeArticleResource_Release(swept_owner);CHECK(set->releasing&&set->nodes[0].link==payload);
        CHECK(nativeMatchPool_Resolve(set->nodes[0].payload)==payload);
        CHECK(!nativeArticleResource_Exists(swept_owner,0));
    }
    return true;
}
bool fixtureSweepPoint(CollData* collision){++sweeps;collision->left_facing_wall.index=23;return false;}
bool fixtureFollowFloor(CollData* collision){++sweeps;collision->cur_pos.y+=2;return true;}
#include "samus_sweep_actual.h"
static MapCollData map_geometry={.vert_count=2,.line_count=8};
static MapLine map_lines[8];
static CollLine line_geometry[8];
static CollVtx vertex_geometry[2];
static MapCollData* fixtureMap=&map_geometry;
static CollLine* fixtureLines=line_geometry;
static CollVtx* fixtureVertices=vertex_geometry;
static int fixtureDebugLevel;
#include "line_speed_actual.h"
#include "samus_line_actual.h"
typedef struct GrappleSegment {float x0,y0,x1,y1;} GrappleSegment;
static GrappleSegment grapple_segments[2];
static unsigned grapple_segment_count;
static bool grapple_segment_reference;
bool fixtureSegmentBlocked(Vec3* position,int* line,u32* flags,Vec3* normal,
    int skip,int only,float x0,float y0,float x1,float y1)
{
    CHECK(!position&&!line&&!flags&&!normal&&skip==-1&&only==-1);
    CHECK(grapple_segment_count<2);
    GrappleSegment* segment=&grapple_segments[grapple_segment_count++];
    if(grapple_segment_reference){
        CHECK(fabsf(segment->x0-x0)<0.001f&&fabsf(segment->y0-y0)<0.001f);
        CHECK(fabsf(segment->x1-x1)<0.001f&&fabsf(segment->y1-y1)<0.001f);
    }else *segment=(GrappleSegment){x0,y0,x1,y1};
    return false;
}
EF_SpawnResult fixtureSpawnEffect(int effect,HSD_GObj* object,...){(void)effect;(void)object;abort();}

static void destroy_item(void* opaque)
{Item* item=opaque;nativeArticleResource_CancelGeneration(item->entity,(uint32_t)item->x1C);nativeScriptItem_Detach(item->entity);}
static void destroy_fighter(void* opaque)
{Fighter* fighter=opaque;nativeScriptFighter_Detach(fighter->gobj);nativeArticleResource_CancelCreator(fighter->gobj,(uint32_t)fighter->x8_spawnNum);NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeFighterPool,fighter,&ref));CHECK(nativeMatchPool_Free(&nativeFighterPool,ref));}
static void stage_candidate(const char* path)
{
    CHECK(nativeFighterProject_Open(path,&project));
    candidate.address=&candidate;candidate.target=Ft_Kind_CustomFirst;
    candidate.owned_input.assets=(NativeSourceAssetInput){project.fighter_dat,project.fighter_size,
        project.animation_dat,project.animation_size,project.costume_dat,project.costume_size};
    CHECK(nativeSourceAssets_Import(&project.facts,&candidate.owned_input.assets,
        Ft_Kind_CustomFirst,&candidate.assets,&diagnostic));
    if(samus_graphs)CHECK(candidate.assets.facts.bone_count==41&&
        candidate.assets.facts.joints==60&&!candidate.assets.facts.omission_count);
    CHECK(nativeSourceBank_Allocate(&candidate.compiler,&diagnostic));
    const char* animation_source=animated_graphs?
        "    model_animation Flight joint 76 material None shape None\n"
        "    state int animation_op\n    state float animation_frame\n    state float animation_rate\n"
        "    on signal Animate:\n"
        "        if item.state.animation_op == 0:\n            item.state.delivered = item.resource(0).joint(\"\").add_animation(Flight)\n"
        "        elif item.state.animation_op == 1:\n            item.state.delivered = item.resource(0).joint(\"\").request_animation(item.state.animation_frame)\n"
        "        elif item.state.animation_op == 2:\n            item.state.delivered = item.resource(0).joint(\"\").set_animation_rate(item.state.animation_rate)\n"
        "        elif item.state.animation_op == 3:\n            item.state.delivered = item.resource(0).joint(\"\").advance_animation()\n"
        "        else:\n            item.state.delivered = item.resource(0).joint(\"\").remove_animation()\n":"";
    snprintf(source_buffer,sizeof(source_buffer),"fighter:\n    behavior independent\n"
        "state int scheduler_ticks\nfunc scheduler_pulse():\n    state.scheduler_ticks = state.scheduler_ticks + 1\n"
        "article %s states 1:\n    resources capacity 3 reserve_part %u\n    model_root ResourceModel %u\n    state int visits\n    state bool delivered\n"
        "    state int line\n    state float speed_x\n    state float speed_y\n    state float speed_z\n"
        "%s"
        "    on signal Construct:\n        item.state.delivered = item.resources.begin(2)\n"
        "        if item.state.delivered:\n            item.state.delivered = item.resource(0).create(0, 1, 0)\n"
        "            item.state.delivered = item.resource(1).create(1, 2, 0)\n"
        "            item.state.delivered = item.resource(0).load_model(ResourceModel)\n"
        "    on signal Speed:\n        item.state.delivered = item.resource(0).read_line_speed(item.state.line, item.state.speed_x, item.state.speed_y, item.state.speed_z)\n"
        "    state 0 anim -1:\n        on animation:\n            item.state.visits = item.state.visits + 1\n            return false\n",animated_graphs?"It_Kind_Link_Boomerang":"It_Kind_Link_Arrow",
        (unsigned)candidate.assets.parts.parts_num,animated_graphs?68u:36u,animation_source);
    if(samus_graphs){
        FILE* file=fopen(samus_source_path,"rb");CHECK(file);
        size_t size=fread(source_buffer,1,sizeof(source_buffer)-1,file);
        CHECK(!ferror(file)&&feof(file));fclose(file);source_buffer[size]=0;
    }
    const char* source=source_buffer;
    NativeScriptSourceTree* tree=&candidate.compiler.bank.tree;
    tree->source=source;tree->source_bytes=(uint32_t)strlen(source);
    CHECK(nativeScriptSource_Parse(tree,&diagnostic));
    CHECK(nativeSourceMotionPolicy_Prepare(tree,ftData_MotionStateList,&candidate.assets,&diagnostic));
    CHECK(nativeSourceEntityAssets_Prepare(&candidate.entities,&candidate.assets,
        &candidate.resources,tree,&diagnostic));
    unsigned row_index=UINT32_MAX;
    for(unsigned i=0;i<candidate.entities.count;++i)
        if(!strcmp(candidate.entities.rows[i].declaration,samus_graphs?"It_Kind_Samus_GBeam":animated_graphs?"It_Kind_Link_Boomerang":"It_Kind_Link_Arrow"))row_index=i;
    CHECK(row_index!=UINT32_MAX);
    candidate.metadata=(NativeScriptItemMetadataContext){.motions=&candidate.assets.facts.moves,
        .assets=candidate.entities.rows,.asset_count=candidate.entities.count,
        .source_facts=&candidate.assets.facts,.source_assets=&candidate.assets};
    candidate.parameters.items=&candidate.metadata;
    candidate.parameters.fighter=(NativeScriptMetadataDat){project.fighter_dat,
        project.fighter_size,candidate.assets.facts.fighter_root};
    CHECK(nativeSourceParameters_Parse(&candidate.parameters,&diagnostic));
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),
        .metadata=nativeScriptItemMetadata_Validate,.metadata_context=&candidate.metadata,
        .parameter_provider=nativeSourceParameters_Provide,.parameter_context=&candidate.parameters,
        .motion_layout=&candidate.assets.facts.moves,.pose_graphs=candidate.assets.facts.poses,
        .pose_graph_count=candidate.assets.facts.pose_count,
        .pose_graph_mask=candidate.assets.facts.layout.joint_graph_mask};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&candidate.compiler.bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,
        &candidate.assets.facts.moves,&candidate.metadata,&candidate.program,&diagnostic));
    CHECK(nativeSourceItemVisuals_Prepare(&candidate.visuals,&candidate.program.items,&diagnostic));
    candidate.input_memory=malloc(1);CHECK(candidate.input_memory);
    candidate.ready=true;
    CHECK(nativeSourcePackage_Pin(&candidate));
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&candidate.program);
    slots[0]=(NativeFighterSlot){.source=&candidate,.fighter_kind=Ft_Kind_CustomFirst,.active=true};
    // Same-kind duplicate slot rows retain the actual getter's first-match
    // behavior; this is not a distinct-kind package deduplication proof.
    slots[1]=slots[0];
    static FighterPartsTable* parts_tables[Ft_Kind_RuntimeCapacity];
    ftPartsTable=parts_tables;ftPartsTable[Ft_Kind_CustomFirst]=&candidate.assets.parts;
    gFtDataList[Ft_Kind_CustomFirst]=&candidate.assets.character.data;
    ftData_CharacterStateTables[Ft_Kind_CustomFirst]=candidate.assets.motions;
    CHECK(nativeFighterRuntime_SourcePackage(Ft_Kind_CustomFirst)==&candidate);
}

static unsigned rig_parts(HSD_JObj* joint,unsigned index,unsigned depth)
{
    CHECK(depth<64);
    for(;joint;joint=joint->next){
        CHECK(index<candidate.assets.parts.parts_num);
        fighter_data->parts[index++].joint=joint;
        if(joint->child&&!(joint->flags&JOBJ_INSTANCE))index=rig_parts(joint->child,index,depth+1);
    }
    return index;
}
static void make_fighter(void)
{
    fighter_object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(fighter_object);
    NativeMatchPoolReference fighter_reference;NativeFighterPoolSlot* slot=nativeMatchPool_Allocate(&nativeFighterPool,&fighter_reference);CHECK(slot);fighter_data=&slot->actor;
    *fighter_data=(Fighter){.gobj=fighter_object,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,
        .x8_spawnNum=200,.ft_data=&candidate.assets.character.data,
        .dat_attrs_backup=slot->attributes,.x34_scale={1,1,1},
        .dobj_list={.data=slot->primary_dobjs},.x203C={.data=slot->secondary_dobjs}};
    fighter_data->parts=slot->parts;
    memset(fighter_data->parts,0,sizeof(FighterBone)*MAX_FT_PARTS);
    // Controlled transition rig: actual HSD constructors and owned parts table;
    // no claim that this small graph is the complete imported Fighter costume.
    static HSD_Joint child={.scale={1,1,1}};
    static HSD_Joint parent={.child=&child,.scale={1,1,1}};
    HSD_JObj* rig=HSD_JObjLoadJoint(samus_graphs?candidate.assets.costume.joint:&parent);CHECK(rig&&rig->child);
    HSD_GObjObject_80390A70(fighter_object,HSD_GObj_JObjKind,rig);
    if(samus_graphs)CHECK(rig_parts(rig,0,0)==candidate.assets.costume.joint_count);
    else {fighter_data->parts[0].joint=rig;fighter_data->parts[1].joint=rig->child;}
    GObj_InitUserData(fighter_object,4,destroy_fighter,fighter_data);
    CHECK(nativeScriptFighter_Attach(fighter_object));
}
static Item_GObj* make_item(unsigned at)
{
    const NativeScriptItemProgram* p=NULL;
    for(unsigned n=0;n<candidate.program.items.count;++n)
        if(!samus_graphs||candidate.program.items.programs[n].kind==It_Kind_Samus_GBeam){
            p=nativeScriptItem_Program(Ft_Kind_CustomFirst,candidate.program.items.programs[n].owner);break;}
    CHECK(p);
    Item_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
    data[at]=(Item){.entity=object,.kind=p->kind,.x1C=100+at,.msid=-1,.owner=fighter_object,
        .native_article_author_kind=p->author_kind,.native_script_owner=p->owner,
        .xC4_article_data=(Article*)p->article,.xCC_item_attr=p->article->x0_common_attr};
    GObj_InitUserData(object,6,destroy_item,&data[at]);nativeItem_CaptureCreator(object,fighter_object);
    CHECK(nativeScriptItem_Attach(object,p));CHECK(nativeScriptItem_AttachmentOwner(object)==fighter_object);
    if(samus_graphs){
        CHECK(p->article&&p->article->x10_modelDesc&&p->article->x10_modelDesc->x0_joint);
        HSD_JObj* model=HSD_JObjLoadJoint(p->article->x10_modelDesc->x0_joint);CHECK(model);
        HSD_GObjObject_80390A70(object,HSD_GObj_JObjKind,model);
        return object;
    }
    CHECK(nativeScriptItem_SelectState(object,0));
    // Controlled native Item entry: the production state-change path publishes
    // msid before selecting the matching interpreted state.
    data[at].msid=0;
    NativeScriptValue result={0};CHECK(nativeScriptItem_RunPhase(object,NativeScriptItemPhase_Animation,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_Bool&&!result.as.u64);return object;
}
static void collection(Item_GObj* owner,unsigned count)
{
    NativeItemLifetime source;CHECK(nativeItemLifetime_Capture(owner,&source));CHECK(source.source_owned);
    CHECK(nativeArticleResourceScene_Ready());CHECK(nativeArticleResourceModel_OwnerReady(&source));
    Fighter_GObj* attachment=nativeScriptItem_AttachmentOwner(owner);CHECK(attachment==fighter_object);
    NativeFighterLifetime fighter;CHECK(nativeFighterLifetime_Capture(fighter_data,&fighter));
    CHECK(nativeArticleResource_Begin(owner,count));
    for(unsigned i=0;i<count;++i){NativeArticleResourceInit init={.attachment_part=i%2,.radius=1+i,
        .collision_category=i,.position={i,2,3}};CHECK(nativeArticleResource_CreateNode(owner,i,&init));}
    CHECK(nativeArticleResource_Count(owner)==count);
}
static void load(Item_GObj* owner,unsigned index)
{CHECK(nativeArticleResource_LoadModel(owner,index,0));++graphs;ResourceCapture view;CHECK(nativeArticleResource_Capture(owner,index,&view));CHECK(view.model&&HSD_IDGetData(view.model->id,NULL)==view.model);}
static bool captured(const void* pointer,size_t bytes)
{
    for(uint32_t i=0;i<regions.count;++i){uintptr_t p=(uintptr_t)pointer,b=(uintptr_t)regions.regions[i].memory;
        if(p>=b&&p-b<=regions.regions[i].size&&bytes<=regions.regions[i].size-(p-b))return true;}return false;
}
static void line_geometry_reset(void)
{
    fixtureMap=&map_geometry;fixtureLines=line_geometry;fixtureVertices=vertex_geometry;
    map_geometry=(MapCollData){.line_count=8,.vert_count=2};
    memset(line_geometry,0,sizeof(line_geometry));memset(map_lines,0,sizeof(map_lines));
    line_geometry[5]=(CollLine){.x0=&map_lines[5],.flags=LINE_FLAG_ENABLED};
    map_lines[5].v1_idx=1;
    vertex_geometry[0]=(CollVtx){.pos={5,2}};
    vertex_geometry[1]=(CollVtx){.x10=10,.pos={15,2}};
}
static NativeScriptValue state(Item_GObj* object,const char* name,const NativeScriptValue* write)
{
    Item* item=object->user_data;NativeScriptReceiverView view;
    CHECK(nativeScriptItem_View(object,(uint32_t)item->x1C,&view));
    for(uint32_t n=0;n<view.program->value_count;++n)
        if(view.program->values[n].owner==view.storage->owner&&
           nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
            uint32_t semantic=NATIVE_SCRIPT_STATE_SEMANTIC|n;
            if(write)CHECK(nativeScriptStorage_Write(view.storage,semantic,NULL,0,write,&diagnostic));
            NativeScriptValue output;CHECK(nativeScriptStorage_Read(view.storage,semantic,NULL,0,&output,&diagnostic));return output;
        }
    CHECK(false);return (NativeScriptValue){0};
}
static uint32_t signal_id(Item_GObj* owner,const char* name)
{
    const NativeScriptItemProgram* program=nativeScriptItem_Spec(owner);CHECK(program);
    NativeScriptReceiverView view;CHECK(nativeScriptItem_View(owner,(uint32_t)((Item*)owner->user_data)->x1C,&view));
    for(uint32_t n=0;n<program->signal_count;++n){
        uint32_t function=program->signals[n].function;CHECK(function<view.program->function_count);
        NativeScriptEventResolution event;
        CHECK(nativeScriptEvents_Resolve(view.program->tree,
            &view.program->tree->nodes[view.program->functions[function].source_node],&event,&diagnostic));
        if(nativeScriptSource_SpanEquals(view.program->tree,event.signal_name,name))return program->signals[n].signal_id;
    }
    CHECK(false);return UINT32_MAX;
}
static void line_vm(Item_GObj* owner,int line,bool expected_result,const Vec3* expected)
{
    const char* fields[]={"speed_x","speed_y","speed_z"};
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=line};state(owner,"line",&value);
    for(unsigned n=0;n<3;++n){value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=91+(float)n};state(owner,fields[n],&value);}
    const NativeScriptItemProgram* program=nativeScriptItem_Spec(owner);CHECK(program&&program->signal_count==(animated_graphs?3u:2u));
    nativeScriptItem_BeginTick(owner);value=(NativeScriptValue){0};
    CHECK(nativeScriptItem_RunSignal(owner,signal_id(owner,"Speed"),0,&value)==NativeScriptVM_Ok);
    value=state(owner,"delivered",NULL);CHECK(value.type==NativeScriptType_Bool&&!!value.as.u64==expected_result);
    const float xyz[]={expected->x,expected->y,expected->z};
    for(unsigned n=0;n<3;++n){value=state(owner,fields[n],NULL);CHECK(value.type==NativeScriptType_F32&&value.as.f32==xyz[n]);}
    ++line_vm_cases;
}
static void line_refuse(Item_GObj* owner,int line)
{
    Vec3 output={91,92,93},before=output;
    CHECK(!nativeArticleResource_ReadLineSpeed(owner,0,line,&output));CHECK(!memcmp(&output,&before,sizeof(output)));
    ResourceCollection* set=nativeArticleResource_Collection(owner);CHECK(set&&set->active_operations==0);
    Vec3 zero={0};line_vm(owner,line,false,&zero);++line_refusals;
}
static void line_speed(void)
{
    Item_GObj* owner=objects[0];collection(owner,2);line_geometry_reset();
    ssize_t heap=OSCheckHeap(HSD_GetHeap());
    for(unsigned n=0;n<256;++n){
        // Includes coincident previous vertices and points before/after the
        // segment; compare the native remapping result without approximation.
        vertex_geometry[0]=(CollVtx){.x10=-2,.x14=3,.pos={-.5f+(float)n*.125f,4}};
        vertex_geometry[1]=(CollVtx){.x10=n%4?-2+8: -2,.x14=n%4?7:3,.pos={7,8-(float)n*.0625f}};
        Vec3 point={-5+(float)(n%23),-3+(float)(n%17),7},expected,output={91,92,93};
        CHECK(nativeArticleResource_WriteFloat(owner,0,NativeArticleResource_PositionX,point.x));
        CHECK(nativeArticleResource_WriteFloat(owner,0,NativeArticleResource_PositionY,point.y));
        CHECK(nativeArticleResource_WriteFloat(owner,0,NativeArticleResource_PositionZ,point.z));
        CHECK(fixtureLineSpeed(5,&point,&expected));
        CHECK(nativeArticleResource_ReadLineSpeed(owner,0,5,&output));CHECK(!memcmp(&expected,&output,sizeof(output)));
        if(n%32==0)line_vm(owner,5,true,&expected);++line_comparisons;
    }
    const int invalid[]={-2,INT32_MIN,8,INT32_MAX};
    for(unsigned n=0;n<sizeof(invalid)/sizeof(invalid[0]);++n)line_refuse(owner,invalid[n]);
    fixtureMap=NULL;fixtureLines=NULL;fixtureVertices=NULL;
    Vec3 output={91,92,93},point={0},before=output;
    CHECK(!fixtureLineSpeed(-1,&point,&output)&&!memcmp(&output,&before,sizeof(output)));
    line_refuse(owner,-1);line_refuse(owner,5);
    line_geometry_reset();fixtureLines=NULL;line_refuse(owner,5);
    line_geometry_reset();line_geometry[5].x0=NULL;fixtureVertices=NULL;
    line_geometry[5].flags=0;line_refuse(owner,5);
    line_geometry[5].flags=LINE_FLAG_ENABLED|LINE_FLAG_HIDDEN;line_refuse(owner,5);
    line_geometry_reset();line_geometry[5].x0=NULL;line_refuse(owner,5);
    line_geometry_reset();fixtureVertices=NULL;line_refuse(owner,5);
    line_geometry_reset();map_lines[5].v0_idx=2;line_refuse(owner,5);
    line_geometry_reset();map_lines[5].v1_idx=2;line_refuse(owner,5);
    line_geometry_reset();map_geometry.vert_count=-1;line_refuse(owner,5);
    line_geometry_reset();map_geometry.line_count=0;line_refuse(owner,5);
    line_geometry_reset();map_geometry.line_count=-1;line_refuse(owner,5);
    CHECK(OSCheckHeap(HSD_GetHeap())==heap);nativeArticleResource_Release(owner);++cases;
}
static void basics(void)
{
    Item_GObj* owner=objects[0];CHECK(!nativeArticleResource_Begin(NULL,2));CHECK(!nativeArticleResource_Begin(owner,1));
    CHECK(!nativeArticleResource_Begin(owner,4));collection(owner,3);
    CHECK(!nativeArticleResource_Begin(owner,2));ResourceCapture old;CHECK(nativeArticleResource_Capture(owner,0,&old));
    CHECK(!nativeArticleResource_CreateNode(owner,3,&(NativeArticleResourceInit){0}));
    old.gobj->classifier=HSD_GOBJ_CLASS_ITEM;CHECK(!nativeArticleResource_Current(&old));old.gobj->classifier=HSD_GOBJ_CLASS_ITEMLINK;
    void* original=old.gobj->user_data;old.gobj->user_data=NULL;CHECK(!nativeArticleResource_Current(&old));old.gobj->user_data=original;
    original=owner->user_data;owner->user_data=NULL;CHECK(!nativeArticleResource_Current(&old));owner->user_data=original;
    ++fighter_data->x8_spawnNum;CHECK(!nativeArticleResource_AttachmentCurrent(old.collection));--fighter_data->x8_spawnNum;
    CHECK(nativeArticleResource_Current(&old)&&nativeArticleResource_AttachmentCurrent(old.collection));
    ssize_t before=OSCheckHeap(HSD_GetHeap());
    CHECK(nativeArticleResource_WriteFloat(owner,0,NativeArticleResource_VelocityX,7));
    CHECK(nativeArticleResource_ReadFloat(owner,0,NativeArticleResource_VelocityX)==7);
    CHECK(nativeArticleResource_WriteInt(owner,0,NativeArticleResource_SurfaceLine,11));
    CHECK(nativeArticleResource_ReadInt(owner,0,NativeArticleResource_SurfaceLine)==11);
    CHECK(nativeArticleResource_SweepStage(owner,0));CHECK(nativeArticleResource_ReadInt(owner,0,NativeArticleResource_FloorLine)==17);
    CHECK(!nativeArticleResource_SweepPoint(owner,0));CHECK(nativeArticleResource_FollowFloor(owner,0));
    Vec3 velocity={99,99,99};CHECK(!nativeArticleResource_ReadLineSpeed(owner,0,-1,&velocity));CHECK(velocity.x==99);
    CHECK(nativeArticleResource_ReadLineSpeed(owner,0,5,&velocity)&&velocity.x==5&&velocity.y==2&&velocity.z==0);
    load(owner,0);CHECK(OSCheckHeap(HSD_GetHeap())==before);CHECK(!nativeArticleResource_Current(&old));
    if(animation_enabled){
    ResourceCapture animation;CHECK(nativeArticleResource_Capture(owner,0,&animation));
    HSD_JObj snapshot=*animation.model;uint32_t used=animation.graph.pool->used;
    CHECK(!nativeArticleResource_AddAnimation(owner,0,NULL,0,999));
    CHECK(!memcmp(&snapshot,animation.model,sizeof(snapshot))&&used==animation.graph.pool->used);
    CHECK(nativeArticleResource_RequestAnimation(owner,0,NULL,0,4));
    CHECK(nativeArticleResource_SetAnimationRate(owner,0,NULL,0,.5f));
    CHECK(nativeArticleResource_AdvanceAnimation(owner,0,NULL,0));
    CHECK(nativeArticleResource_RemoveAnimation(owner,0,NULL,0));
    CHECK(OSCheckHeap(HSD_GetHeap())==before);
    }
    ResourceCapture model;CHECK(nativeArticleResource_Capture(owner,0,&model));
    unsigned part=(unsigned)candidate.assets.parts.parts_num;CHECK(nativeArticleResource_BindExternalPart(owner,0,part));
    CHECK(fighter_data->parts[part].joint==model.model);
    fighter_data->motion_id=ftCo_MS_CatchWait;CHECK(nativeArticleResource_SetCaptureAnchor(owner,0));
    fighter_data->x914[0].state=HitCapsule_Enabled;fighter_data->x914[0].jobj=model.model;
    CHECK(nativeScriptItem_ClearAttachmentOwner(owner));CHECK(!nativeArticleResource_AttachmentCurrent(model.collection));
    Mtx unchanged={{9}};CHECK(!nativeArticleResource_ReadAttachmentMatrix(owner,0,unchanged)&&unchanged[0][0]==9);
    CHECK(nativeArticleResource_WriteFloat(owner,0,NativeArticleResource_PositionX,12));
    nativeArticleResource_Release(owner);CHECK(!nativeArticleResource_Current(&model));
    CHECK(!fighter_data->parts[part].joint&&!fighter_data->x914[0].jobj&&fighter_data->x914[0].state==HitCapsule_Disabled);
    CHECK(fighter_data->mv.co.capturedamage.x18!=model.model);CHECK(OSCheckHeap(HSD_GetHeap())==before);++cases;
    CHECK(nativeScriptItem_SetAttachmentOwner(owner,fighter_object));collection(owner,2);
    ResourceCapture reused;CHECK(nativeArticleResource_Capture(owner,0,&reused));
    CHECK(reused.generation>model.generation&&!nativeArticleResource_Current(&model));
    CHECK(reused.payload.generation>model.payload.generation);nativeArticleResource_Release(owner);++cases;
}
static void retire_cases(void)
{
    Item_GObj* owner=objects[0];collection(owner,2);load(owner,0);
    ResourceCapture view;CHECK(nativeArticleResource_Capture(owner,0,&view));
    HSD_GObjFree(view.gobj);CHECK(!nativeMatchPool_Resolve(view.object)&&!nativeMatchPool_Resolve(view.graph));
    CHECK(!nativeArticleResource_Current(&view)&&!nativeArticleResource_Collection(owner));++cases;
    collection(owner,2);load(owner,0);CHECK(nativeArticleResource_Capture(owner,0,&view));
    // Explicitly sever the endpoint's userdata/graph ownership before a real
    // physical GObj free. The node retains its original graph token; this is
    // an orphan-state injection, not the normal native destructor sequence.
    view.gobj->user_data=NULL;view.gobj->user_data_kind=HSD_GOBJ_USER_DATA_NONE;
    view.gobj->user_data_remove_func=NULL;view.gobj->hsd_obj=NULL;view.gobj->obj_kind=HSD_GOBJ_OBJ_NONE;
    HSD_GObjFree(view.gobj);CHECK(!nativeMatchPool_Resolve(view.object));CHECK(nativeMatchPool_Resolve(view.graph)==view.model);
    CHECK(!nativeArticleResource_Current(&view));CHECK(nativeArticleResourceModel_Remove(view.collection,view.node));
    CHECK(!nativeMatchPool_Resolve(view.graph));nativeArticleResource_Release(owner);CHECK(!nativeArticleResource_Collection(owner));++cases;
    collection(owner,2);load(owner,0);CHECK(nativeArticleResource_Capture(owner,0,&view));
    swept_owner=owner;release_during_sweep=true;
    CHECK(!nativeArticleResource_SweepStage(owner,0));release_during_sweep=false;
    CHECK(!nativeArticleResource_Current(&view)&&!nativeMatchPool_Resolve(view.payload));++cases;
    collection(owner,2);load(owner,0);CHECK(nativeArticleResource_Capture(owner,0,&view));
    unsigned part=(unsigned)candidate.assets.parts.parts_num;CHECK(nativeArticleResource_BindExternalPart(owner,0,part));
    nativeScriptFighter_Detach(fighter_object);CHECK(!nativeArticleResource_AttachmentCurrent(view.collection));
    CHECK(nativeArticleResource_AttachmentPhysical(view.collection));
    nativeArticleResource_Release(owner);CHECK(!fighter_data->parts[part].joint);
    CHECK(nativeScriptFighter_Attach(fighter_object));++cases;
}
static void constructing_attachment_cases(void)
{
    for(unsigned deferred=0;deferred<2;++deferred){
        Item_GObj* owner=objects[0];collection(owner,2);load(owner,0);
        ResourceCapture view;CHECK(nativeArticleResource_Capture(owner,0,&view));
        unsigned part=(unsigned)candidate.assets.parts.parts_num;CHECK(nativeArticleResource_BindExternalPart(owner,0,part));
        fighter_data->motion_id=ftCo_MS_CatchWait;CHECK(nativeArticleResource_SetCaptureAnchor(owner,0));
        fighter_data->x914[0]=(HitCapsule){.state=HitCapsule_Enabled,.jobj=view.model};
        fighter_data->x1064_thrownHitbox=(HitCapsule){.state=HitCapsule_Enabled,.jobj=view.model};
        ResourceCapture active;if(deferred)CHECK(nativeArticleResource_BeginOperation(owner,0,&active));
        NativeMatchPoolReference object_ref,owner_ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,fighter_object,&object_ref));CHECK(nativeMatchPool_Reference(&nativeFighterPool,fighter_data,&owner_ref));
        uint32_t original=(uint32_t)fighter_data->x8_spawnNum;fighter_data->x8_spawnNum=original+1;nativeScriptFighter_NewLife(fighter_object,original+1);
        CHECK(!nativeArticleResource_AttachmentPhysical(view.collection));
        nativeArticleResource_CancelCreator(fighter_object,original+1);CHECK(nativeMatchPool_Resolve(view.graph)==view.model);
        fighter_data->native_constructing=true;CHECK(!nativeArticleResource_AttachmentPhysical(view.collection));
        fighter_object->native_userdata_removing=true;CHECK(nativeArticleResource_AttachmentPhysical(view.collection));
        CHECK(!nativeArticleResource_AttachmentCurrent(view.collection));
        NativeMatchPoolReference invalid=object_ref;invalid.generation++;nativeArticleResource_CancelConstructingAttachment(fighter_object,invalid,owner_ref);CHECK(fighter_data->parts[part].joint==view.model);
        invalid=owner_ref;invalid.generation++;nativeArticleResource_CancelConstructingAttachment(fighter_object,object_ref,invalid);CHECK(fighter_data->parts[part].joint==view.model);
        nativeArticleResource_CancelConstructingAttachment(fighter_object,object_ref,owner_ref);
        CHECK(!fighter_data->parts[part].joint&&!fighter_data->x914[0].jobj&&!fighter_data->x1064_thrownHitbox.jobj&&fighter_data->mv.co.capturedamage.x18!=view.model);
        CHECK(fighter_data->x914[0].state==HitCapsule_Disabled&&fighter_data->x1064_thrownHitbox.state==HitCapsule_Disabled);
        if(deferred){CHECK(view.collection->releasing&&nativeMatchPool_Resolve(view.graph)==view.model);CHECK(!nativeArticleResource_EndOperation(&active));}
        CHECK(!nativeMatchPool_Resolve(view.graph)&&!nativeMatchPool_Resolve(view.payload));
        fighter_object->native_userdata_removing=false;fighter_data->native_constructing=false;fighter_data->x8_spawnNum=original;nativeScriptFighter_NewLife(fighter_object,original);
        ++constructor_cleanup_cases;++cases;
    }
}
typedef struct Observation {
    uint64_t hash;
    NativeMatchPoolReference payload[2],object[2],graph[2];
    HSD_JObj* root[2];
    uint32_t generation[2],id[2],used[2],free_count[2];
    float x;uint32_t visits;
} Observation;
static void observe(Observation* o)
{
    memset(o,0,sizeof(*o));for(unsigned i=0;i<2;++i){
        ResourceCapture capture;CHECK(nativeArticleResource_Capture(objects[0],i,&capture));
        o->payload[i]=capture.payload;o->object[i]=capture.object;o->graph[i]=capture.graph;
        o->root[i]=capture.model;o->generation[i]=capture.generation;o->id[i]=capture.model->id;
        o->used[i]=capture.payload.pool->used;o->free_count[i]=capture.payload.pool->free_count;
    }
    o->x=nativeArticleResource_ReadFloat(objects[0],0,NativeArticleResource_PositionX);
    o->visits=(uint32_t)state(objects[0],"visits",NULL).as.i64;
    o->hash=nativeRollbackRegions_Hash(&regions);
}
static void mutate_history(void)
{
    CHECK(nativeArticleResource_WriteFloat(objects[0],0,NativeArticleResource_PositionX,20));
    load(objects[0],0);
    Mtx matrix={{1,0,0,7},{0,1,0,8},{0,0,1,9}},observed;
    CHECK(nativeArticleResource_SetWorldMatrix(objects[0],0,matrix));
    CHECK(nativeArticleResource_ReadWorldMatrix(objects[0],0,observed));
    CHECK(!memcmp(matrix,observed,sizeof(matrix)));
    NativeScriptValue result={0};
    CHECK(nativeScriptItem_RunPhase(objects[0],NativeScriptItemPhase_Animation,&result)==NativeScriptVM_Ok);
}
static void same_animation_controllers(HSD_AObj* actual,HSD_AObj* expected)
{
    CHECK(!!actual==!!expected);
    if(actual){
        HSD_AObj a=*actual,b=*expected;a.fobj=b.fobj=NULL;a.hsd_obj=b.hsd_obj=NULL;
        CHECK(!memcmp(&a,&b,sizeof(a)));
        HSD_FObj* x=actual->fobj;HSD_FObj* y=expected->fobj;
        for(;x&&y;x=x->next,y=y->next){HSD_FObj a=*x,b=*y;a.next=b.next=NULL;CHECK(!memcmp(&a,&b,sizeof(a)));}
        CHECK(!x&&!y);
    }
}
static void same_animation_graph(HSD_JObj* actual,HSD_JObj* expected,unsigned depth)
{
    CHECK(depth<64);
    for(;actual&&expected;actual=actual->next,expected=expected->next){
        CHECK(actual->flags==expected->flags);
        CHECK(!memcmp(&actual->translate,&expected->translate,sizeof(actual->translate)));
        if(memcmp(&actual->rotate,&expected->rotate,sizeof(actual->rotate)))
            fprintf(stderr,"animation rotation mismatch depth=%u actual=(%.9g,%.9g,%.9g) native=(%.9g,%.9g,%.9g) flags=0x%X\n",
                depth,actual->rotate.x,actual->rotate.y,actual->rotate.z,expected->rotate.x,expected->rotate.y,expected->rotate.z,(unsigned)actual->flags);
        CHECK(!memcmp(&actual->rotate,&expected->rotate,sizeof(actual->rotate)));
        CHECK(!memcmp(&actual->scale,&expected->scale,sizeof(actual->scale)));
        same_animation_controllers(actual->aobj,expected->aobj);
        CHECK(!!actual->child==!!expected->child);
        if(actual->child&&!(actual->flags&JOBJ_INSTANCE))same_animation_graph(actual->child,expected->child,depth+1);
    }
    CHECK(!actual&&!expected);
}
static void animation_operation(Item_GObj* owner,int op,float frame,float rate,HSD_JObj* expected)
{
    NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=op};state(owner,"animation_op",&value);
    value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=frame};state(owner,"animation_frame",&value);
    value.as.f32=rate;state(owner,"animation_rate",&value);
    const NativeScriptItemProgram* p=nativeScriptItem_Spec(owner);CHECK(p&&p->model_animation_count==1);
    nativeScriptItem_BeginTick(owner);CHECK(nativeScriptItem_RunSignal(owner,signal_id(owner,"Animate"),0,&value)==NativeScriptVM_Ok);
    value=state(owner,"delivered",NULL);CHECK(value.type==NativeScriptType_Bool&&value.as.u64);
    switch(op){
    case 0:HSD_JObjAddAnimAll(expected,(HSD_AnimJoint*)p->model_animations[0].anim,NULL,NULL);break;
    case 1:HSD_JObjReqAnimAll(expected,frame);break;
    case 2:lb_8000BA0C(expected,rate);break;
    case 3:HSD_JObjAnimAll(expected);break;
    default:HSD_JObjRemoveAnimAll(expected);break;
    }
    ResourceCapture view;CHECK(nativeArticleResource_Capture(owner,0,&view));
    same_animation_graph(view.model,expected,0);++animation_pairs;
}
static void animations(void)
{
    if(!animated_graphs)return;
    Item_GObj* owner=objects[0];collection(owner,2);load(owner,0);load(owner,1);
    ResourceCapture actual,reference;CHECK(nativeArticleResource_Capture(owner,0,&actual));CHECK(nativeArticleResource_Capture(owner,1,&reference));
    ssize_t heap=OSCheckHeap(HSD_GetHeap());
    animation_operation(owner,0,0,1,reference.model);
    CHECK(actual.model->aobj||actual.model->child->aobj);
    animation_operation(owner,1,2.25f,1,reference.model);
    animation_operation(owner,2,0,.75f,reference.model);
    const NativeScriptItemProgram* p=nativeScriptItem_Spec(owner);
    const NativeScriptItemResourceSelector* add=NULL;
    for(unsigned n=0;n<p->resource_selector_count;++n){const NativeScriptItemResourceSelector* r=&p->resource_selectors[n];
        if((r->semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK)==NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION&&
           r->operation==NativeScriptItemResource_AddAnimation-NativeScriptItemResource_Resources)add=r;}
    CHECK(add);
    NativeScriptRuntimeContext context={.source=owner,.source_class=NativeCallbackSource_Item,
        .generation=(uint32_t)((Item*)owner->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};
    NativeScriptValue selectors[]={{.type=NativeScriptType_I32,.as.i64=0},
        {.type=NativeScriptType_Text,.as.text={add->path_offset,add->path_length}}};
    NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=add->animation},output={.type=NativeScriptType_I32,.as.i64=91},unchanged=output;
    for(unsigned mode=0;mode<3;++mode){
        NativeScriptValue selected[2]={selectors[0],selectors[1]},argument=input;uint32_t semantic=add->semantic;
        if(mode==0)selected[1].as.text.offset++;
        if(mode==1)argument.as.i64++;
        if(mode==2)semantic^=0x80000;
        CHECK(!nativeScriptBindingItemResources_Invoke(&context,semantic,selected,2,&argument,1,&output,&diagnostic));
        CHECK(!memcmp(&output,&unchanged,sizeof(output)));same_animation_graph(actual.model,reference.model,0);
        CHECK(actual.collection->active_operations==0);++animation_refusals;
    }
    for(unsigned n=0;n<24;++n)animation_operation(owner,3,0,1,reference.model);
    // Replacing an active bundle and fractional/backwards requests preserve
    // the stock HSD controller/track values, not just a visible sampled pose.
    animation_operation(owner,0,0,1,reference.model);
    animation_operation(owner,1,-.5f,1,reference.model);
    animation_operation(owner,2,0,-.25f,reference.model);
    for(unsigned n=0;n<8;++n)animation_operation(owner,3,0,1,reference.model);
    animation_operation(owner,4,0,1,reference.model);
    CHECK(OSCheckHeap(HSD_GetHeap())==heap);
    CHECK(actual.collection->active_operations==0);
    nativeArticleResource_Release(owner);++cases;
}
#include <melee/it/native_item_link_pool.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/native_objalloc_snapshot.h>
static unsigned reserved_graph_checks;
static size_t reserved_graph_bytes;
static HSD_Material* reserved_graph_material(HSD_JObj* node,unsigned depth)
{
    CHECK(depth<64);
    for(;node;node=node->next){
        if(union_type_dobj(node))for(HSD_DObj* d=node->u.dobj;d;d=d->next)
            if(d->mobj&&d->mobj->mat)return d->mobj->mat;
        HSD_Material* result=reserved_graph_material(node->child,depth+1);
        if(result)return result;
    }
    return NULL;
}
static void explicit_reserved_graph_records(void)
{
    unsigned start=checks;
    collection(objects[0],2);load(objects[0],0);load(objects[0],1);
    ResourceCapture nodes[2];HSD_JObj expected[2],child[2];Vec3 scale[2];HSD_Material materials[2];
    HSD_Material* material_ptrs[2];
    for(unsigned i=0;i<2;++i){
        CHECK(nativeArticleResource_Capture(objects[0],i,&nodes[i]));
        CHECK(nodes[i].model&&nativeMatchPool_Resolve(nodes[i].graph)==nodes[i].model);
        expected[i]=*nodes[i].model;
        if(nodes[i].model->child)child[i]=*nodes[i].model->child;
        if(nodes[i].model->scl)scale[i]=*nodes[i].model->scl;
        material_ptrs[i]=reserved_graph_material(nodes[i].model,0);CHECK(material_ptrs[i]);
        materials[i]=*material_ptrs[i];
    }
    HSD_JObj* ordinary=HSD_JObjAlloc();CHECK(ordinary);
    ordinary->translate=(Vec3){13,29,47};ordinary->rotate.y=.75f;
    HSD_JObj ordinary_expected=*ordinary;
    ordinary->id=(uintptr_t)ordinary;
    HSD_IDInsertToTable(NULL,ordinary->id,ordinary);ordinary_expected=*ordinary;
    uintptr_t extra_id=ordinary->id+1;
    Vec3* ordinary_vec=HSD_VecAlloc();MtxPtr ordinary_mtx=HSD_MtxAlloc();CHECK(ordinary_vec&&ordinary_mtx);
    *ordinary_vec=(Vec3){19,31,53};Mtx matrix_expected;
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col)ordinary_mtx[row][col]=(float)(row*4+col+1);
    memcpy(matrix_expected,ordinary_mtx,sizeof(matrix_expected));
    size_t class_capacity=nativeClass_SnapshotBytes(),class_written=0;CHECK(class_capacity);
    uint8_t* classes=malloc(class_capacity),*class_after=malloc(class_capacity);CHECK(classes&&class_after);
    CHECK(nativeClass_Save(classes,class_capacity,&class_written)&&class_written==class_capacity);
    NativeIdSnapshot ids;size_t ids_written;
    CHECK(nativeID_Save(&ids,sizeof(ids),&ids_written));
    u32 heap_capacity=OSHeapSnapshotBytes(HSD_GetHeap()),heap_written;CHECK(heap_capacity);
    uint8_t* heap_saved=malloc(heap_capacity),*heap_after=malloc(heap_capacity);CHECK(heap_saved&&heap_after);
    CHECK(OSSaveHeapSnapshot(HSD_GetHeap(),heap_saved,heap_capacity,&heap_written)&&heap_written==heap_capacity);
    size_t bytes=nativeHsdReservation_SnapshotBytes(),written=0,again=0;CHECK(bytes);
    size_t descriptor_capacity=nativeObjAlloc_SnapshotBytes(),descriptor_written=0;CHECK(descriptor_capacity);
    uint8_t* descriptors=malloc(descriptor_capacity),*descriptor_after=malloc(descriptor_capacity);CHECK(descriptors&&descriptor_after);
    CHECK(nativeObjAlloc_Save(descriptors,descriptor_capacity,&descriptor_written)&&descriptor_written==descriptor_capacity);
    uint8_t* saved=malloc(bytes),*after=malloc(bytes);CHECK(saved&&after);
    CHECK(nativeHsdReservation_Save(saved,bytes,&written)&&written==bytes);
    for(unsigned i=0;i<2;++i){
        nodes[i].model->translate.x+=37;nodes[i].model->rotate.y+=1.25f;
        nodes[i].model->mtx[0][0]=93;nodes[i].model->flags^=JOBJ_MTX_DIRTY;
        if(nodes[i].model->child){nodes[i].model->child->translate.z=-44;nodes[i].model->child->scale.y=17;}
        if(nodes[i].model->scl)nodes[i].model->scl->x=-31;
        material_ptrs[i]->alpha=.125f;material_ptrs[i]->diffuse.r=19;
    }
    ordinary->translate.y=-101;ordinary->rotate.y=-2.5f;ordinary->mtx[1][2]=71;
    ordinary_vec->y=-111;ordinary_mtx[2][1]=-123;
    HSD_IDRemoveByIDFromTable(NULL,ordinary->id);
    HSD_IDInsertToTable(NULL,extra_id,ordinary);
    HSD_IDInsertToTable(NULL,extra_id+1,NULL);
    HSD_JObj changed=*nodes[1].model;
    CHECK(!nativeHsdReservation_Load(saved,bytes-1));
    CHECK(!memcmp(nodes[1].model,&changed,sizeof(changed)));
    CHECK(nativeHsdReservation_ValidateSnapshot(saved,bytes)&&nativeObjAlloc_ValidateSnapshot(descriptors,descriptor_written));
    CHECK(nativeClass_ValidateSnapshot(classes,class_written));
    CHECK(nativeID_ValidateSnapshot(&ids,sizeof(ids))&&OSValidateHeapSnapshot(HSD_GetHeap(),heap_saved,heap_written));
    CHECK(OSLoadHeapSnapshot(HSD_GetHeap(),heap_saved,heap_written));
    CHECK(nativeID_Load(&ids,sizeof(ids)));
    CHECK(nativeHsdReservation_Load(saved,bytes)&&nativeObjAlloc_Load(descriptors,descriptor_written));
    CHECK(nativeClass_Load(classes,class_written));
    CHECK(!memcmp(ordinary,&ordinary_expected,sizeof(ordinary_expected)));
    CHECK(ordinary_vec->x==19&&ordinary_vec->y==31&&ordinary_vec->z==53);
    CHECK(!memcmp(ordinary_mtx,matrix_expected,sizeof(matrix_expected)));
    s32 found;CHECK(HSD_IDGetData(ordinary->id,&found)==ordinary&&found);
    CHECK(HSD_IDGetData(extra_id,&found)==NULL&&!found);
    CHECK(HSD_IDGetData(extra_id+1,&found)==NULL&&!found);
    for(unsigned i=0;i<2;++i){
        CHECK(!memcmp(nodes[i].model,&expected[i],sizeof(expected[i])));
        if(nodes[i].model->child)CHECK(!memcmp(nodes[i].model->child,&child[i],sizeof(child[i])));
        if(nodes[i].model->scl)CHECK(!memcmp(nodes[i].model->scl,&scale[i],sizeof(scale[i])));
        CHECK(!memcmp(material_ptrs[i],&materials[i],sizeof(materials[i])));
        ResourceCapture restored;CHECK(nativeArticleResource_Capture(objects[0],i,&restored));
        CHECK(restored.model==nodes[i].model&&nativeMatchPool_Resolve(restored.graph)==nodes[i].model);
    }
    CHECK(nativeHsdReservation_Save(after,bytes,&again)&&again==bytes&&!memcmp(saved,after,bytes));
    CHECK(nativeObjAlloc_Save(descriptor_after,descriptor_capacity,&again)&&again==descriptor_written&&!memcmp(descriptors,descriptor_after,again));
    CHECK(nativeClass_Save(class_after,class_capacity,&again)&&again==class_written&&!memcmp(classes,class_after,again));
    NativeIdSnapshot ids_after;CHECK(nativeID_Save(&ids_after,sizeof(ids_after),&again)&&again==ids_written&&!memcmp(&ids,&ids_after,again));
    u32 heap_again;CHECK(OSSaveHeapSnapshot(HSD_GetHeap(),heap_after,heap_capacity,&heap_again)&&heap_again==heap_written&&!memcmp(heap_saved,heap_after,heap_again));
    free(heap_after);free(heap_saved);
    free(class_after);free(classes);HSD_JObjRemoveAll(ordinary);
    HSD_VecFree(ordinary_vec);HSD_MtxFree(ordinary_mtx);
    free(descriptor_after);free(descriptors);
    free(after);free(saved);nativeArticleResource_Release(objects[0]);++cases;
    reserved_graph_bytes=bytes;reserved_graph_checks=checks-start;
}
static void explicit_link_records(void)
{
    collection(objects[0],2);load(objects[0],0);load(objects[0],1);
    ResourceCapture view;CHECK(nativeArticleResource_Capture(objects[0],0,&view));
    CHECK(view.payload.pool==&nativeItemLinkPool&&nativeMatchPool_Resolve(view.payload)==view.link);
    ItemLink expected=*view.link;
    size_t capacity=nativeMatchSnapshotPool_Bytes(&nativeItemLinkPool),bytes=0,after_bytes=0;
    uint8_t* saved=malloc(capacity),*after=malloc(capacity);CHECK(saved&&after);
    CHECK(nativeItemLinkPool_Save(saved,capacity,&bytes)&&bytes==capacity);
    view.link->pos=(Vec3){31,-17,2};view.link->vel.x=42;
    view.link->coll_data.contact.y=-19;view.link->x2C_b7=!view.link->x2C_b7;
    CHECK(nativeItemLinkPool_ValidateSnapshot(saved,bytes)&&nativeItemLinkPool_Load(saved,bytes));
    CHECK(!memcmp(view.link,&expected,sizeof(expected)));
    ResourceCapture restored;CHECK(nativeArticleResource_Capture(objects[0],0,&restored));
    CHECK(restored.link==view.link&&nativeMatchPool_Resolve(restored.payload)==view.link);
    CHECK(!nativeItemLinkPool_Load(saved,bytes-1));
    CHECK(nativeItemLinkPool_Save(after,capacity,&after_bytes)&&after_bytes==bytes&&!memcmp(saved,after,bytes));
    free(after);free(saved);nativeArticleResource_Release(objects[0]);++cases;
}
static void history(void)
{
    collection(objects[0],2);load(objects[0],0);load(objects[0],1);
    CHECK(nativeRollbackRegions_Build(&regions));
    ResourceCapture view;CHECK(nativeArticleResource_Capture(objects[0],0,&view));
    CHECK(captured(nativeArticleResourceCollections,sizeof(nativeArticleResourceCollections)));
    CHECK(captured(view.link,sizeof(*view.link))&&captured(view.payload.pool,sizeof(*view.payload.pool)));
    CHECK(captured(view.model,sizeof(*view.model))&&captured(view.graph.pool,sizeof(*view.graph.pool)));
    CHECK(captured(data,sizeof(*data)*2)&&captured(fighter_data,sizeof(*fighter_data)));
    CHECK(!captured(&candidate,sizeof(candidate))&&!captured(&checks,sizeof(checks)));
    static Observation baseline,changed,replayed;observe(&baseline);

    // Distinct fixed arenas may be adjacent in virtual address space. Region
    // discovery must preserve reservation boundaries for Windows protection.
    for(unsigned i=0;i<regions.count;++i) {
        MEMORY_BASIC_INFORMATION first,last;
        CHECK(VirtualQuery(regions.regions[i].memory,&first,sizeof(first)));
        CHECK(VirtualQuery((uint8_t*)regions.regions[i].memory+regions.regions[i].size-1,&last,sizeof(last)));
        CHECK(first.AllocationBase==last.AllocationBase);
    }
    CHECK(nativeRollbackPages_Init(regions.regions,regions.count,regions.exact_regions,regions.exact_count,3,512));
    CHECK(!nativeArticleResourceScene_Retire()&&!nativeArticleResourceModel_Retire());
    if(animation_enabled)CHECK(!nativeArticleResourceAnimation_Retire());

    bool begun=nativeRollbackPages_BeginFrame(0);
    if(!begun) {
        fprintf(stderr,"page-history begin refused: error=%lu regions=%u\n",GetLastError(),regions.count);
        for(unsigned i=0;i<regions.count;++i) {
            MEMORY_BASIC_INFORMATION first,last;
            CHECK(VirtualQuery(regions.regions[i].memory,&first,sizeof(first)));
            CHECK(VirtualQuery((uint8_t*)regions.regions[i].memory+regions.regions[i].size-1,&last,sizeof(last)));
            fprintf(stderr,"page-region %u bytes=%zu first-allocation=%p last-allocation=%p\n",
                i,regions.regions[i].size,first.AllocationBase,last.AllocationBase);
        }
    }
    CHECK(begun);
    mutate_history();
    CHECK(nativeRollbackPages_EndFrame());++frames;observe(&changed);
    CHECK(nativeRollbackPages_ToFrame(0,1));++restores;observe(&replayed);CHECK(!memcmp(&baseline,&replayed,sizeof(baseline)));
    CHECK(nativeRollbackPages_BeginFrame(0));mutate_history();
    CHECK(nativeRollbackPages_EndFrame());++frames;observe(&replayed);CHECK(!memcmp(&changed,&replayed,sizeof(changed)));
    nativeRollbackPages_Shutdown();nativeArticleResource_Release(objects[0]);++cases;
}

#include "palette_cases.h"
#include "samus_constructor_cases.h"
#include "samus_factory_cases.h"
#include "scheduler_cases.h"
#include "graph_buffer_cases.h"
#include "resource_snapshot_cases.h"

int main(int argc,char** argv)
{
    CHECK(argc==2||(argc==3&&(!strcmp(argv[2],"core")||!strcmp(argv[2],"animated")))||((argc==4||argc==5)&&!strcmp(argv[2],"samus")));
    samus_graphs=argc>=4;samus_source_path=samus_graphs?argv[3]:NULL;
    animated_graphs=argc==3&&!strcmp(argv[2],"animated");animation_enabled=argc==2||animated_graphs||samus_graphs;
    enum{HEAP_BYTES=64*1024*1024};unsigned char* heap_bytes=VirtualAlloc(NULL,HEAP_BYTES,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(heap_bytes);
    void* start=OSInitAlloc(heap_bytes,heap_bytes+HEAP_BYTES,1);OSHeapHandle heap=OSCreateHeap(start,heap_bytes+HEAP_BYTES);CHECK(heap>=0);
    HSD_SetHeap(heap);OSSetCurrentHeap(heap);HSD_ObjSetHeap(HEAP_BYTES,NULL);HSD_ObjInit();HSD_IDSetup();
    ClassInfoInit(HSD_CLASS_INFO(&hsdJObj));ClassInfoInit(HSD_CLASS_INFO(&hsdDObj));
    ClassInfoInit(HSD_CLASS_INFO(&hsdMObj));ClassInfoInit(HSD_CLASS_INFO(&hsdPObj));ClassInfoInit(HSD_CLASS_INFO(&hsdTObj));
    CHECK(HSD_JObjUsesStandardMethods()&&HSD_DObjUsesStandardMethods()&&HSD_MObjUsesStandardMethods()&&HSD_PObjUsesStandardMethods()&&HSD_TObjUsesStandardMethods());
    lbHeap_80431FA0.heap_array[0]=(struct Heap){.id=heap,.type=LbHeapType_Hsd,.status=LbHeapStatus_Create,.start=(uintptr_t)heap_bytes,.size=HEAP_BYTES};
    data=lbHeap_80015BD0(0,sizeof(*data)*2);CHECK(data);memset(data,0,sizeof(*data)*2);
    HSD_GObjLibInitDataType config={.p_link_max=63,.gx_link_max=63,.gproc_pri_max=24};HSD_GObj_80391304(&config);
    ssize_t empty=OSCheckHeap(heap);CHECK(nativeArticleResourceScene_Prepare());CHECK(OSCheckHeap(heap)==empty);CHECK(nativeArticleResourceScene_Retire());++cases;
    stage_candidate(argv[1]);
    if(samus_graphs)CHECK(nativeArticleModelScene_Prepare());
    CHECK(nativeArticleResourceScene_Prepare());CHECK(nativeArticleResourceModel_Prepare());
    if(animation_enabled){
        bool prepared=nativeArticleResourceAnimation_Prepare();
        if(!prepared&&samus_graphs)samus_animation_audit();
        CHECK(prepared);
    }
    CHECK(candidate.leases==(animation_enabled?3u:2u)+(samus_graphs?1u:0u));
    make_fighter();objects[0]=make_item(0);objects[1]=make_item(1);
    if(samus_graphs){samus_constructor_cases();if(argc==5)samus_factory_cases(argv[4]);goto cleanup;}
    line_geometry_reset();basics();line_speed();
    const NativeScriptItemProgram* p=nativeScriptItem_Spec(objects[1]);CHECK(p&&p->signal_count==(animated_graphs?3u:2u));
    NativeScriptValue signal_result={0};CHECK(nativeScriptItem_RunSignal(objects[1],signal_id(objects[1],"Construct"),0,&signal_result)==NativeScriptVM_Ok);
    CHECK(nativeArticleResource_Exists(objects[1],1));ResourceCapture vm_node;CHECK(nativeArticleResource_Capture(objects[1],0,&vm_node)&&vm_node.model);++graphs;
    nativeArticleResource_Release(objects[1]);++cases;
    animations();retire_cases();constructing_attachment_cases();explicit_link_records();explicit_reserved_graph_records();explicit_scheduler_scripts();graph_buffer_snapshots();graph_buffer_reserved();resource_registry_snapshot();history();
cleanup:
    HSD_GObjFree(objects[0]);HSD_GObjFree(objects[1]);
    HSD_GObjFree(fighter_object);
    if(samus_graphs)CHECK(nativeArticleModelScene_Retire()&&candidate.leases==3);
    CHECK(nativeArticleResourceScene_Retire());if(animation_enabled)CHECK(nativeArticleResourceAnimation_Retire()&&candidate.leases==2);CHECK(nativeArticleResourceModel_Retire()&&candidate.leases==1);
    CHECK(nativeSourcePackage_Unpin(&candidate));slots[0].active=slots[1].active=false;
    NativeScriptFighterProgram* empty_program=calloc(1,sizeof(*empty_program));CHECK(empty_program);CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,empty_program);free(empty_program);CHECK(nativeSourcePackage_Release(&candidate));
    nativeFighterProject_Close(&project);CHECK(nativeGObjPool_RetireScene());lbHeap_80015CA8(0,data);OSDestroyHeap(heap);CHECK(VirtualFree(heap_bytes,0,MEM_RELEASE));
    if(samus_graphs){printf("{\"status\":\"PASS Samus resource constructor\",\"checks\":%u,\"constructor_pairs\":%u,\"palette_census_cases\":%u,\"palette_comparisons\":%u,\"palette_refusals\":%u}\n",checks,samus_constructor_pairs,palette_census_cases,palette_comparisons,palette_refusals);return 0;}
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"graphs\":%u,\"sweeps\":%u,\"frames\":%u,\"restores\":%u,\"regions\":%u,\"line_comparisons\":%u,\"line_refusals\":%u,\"line_vm_cases\":%u,\"constructor_cleanup_cases\":%u,\"animation_pairs\":%u,\"animation_refusals\":%u,\"animation_prepared\":%s,\"reserved_graph_checks\":%u,\"reserved_graph_bytes\":%zu,\"scheduler_script_checks\":%u,\"scheduler_bytes\":%zu}\n",checks,cases,graphs,sweeps,frames,restores,regions.count,line_comparisons,line_refusals,line_vm_cases,constructor_cleanup_cases,animation_pairs,animation_refusals,animation_enabled?"true":"false",reserved_graph_checks,reserved_graph_bytes,scheduler_script_checks,scheduler_component_bytes);
    printf("{\"graph_buffer_checks\":%u,\"graph_buffer_class_bytes\":%zu,\"graph_buffer_reserved_checks\":%u}\n",graph_buffer_checks,graph_buffer_class_bytes,graph_buffer_reserved_checks);
    printf("{\"resource_registry_checks\":%u,\"resource_registry_refusals\":%u,\"resource_registry_bytes\":%zu}\n",resource_registry_checks,resource_registry_refusals,resource_registry_bytes);
    return 0;
}
