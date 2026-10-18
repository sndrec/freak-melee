#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/fighter.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/native_fighter_pose.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/ft/ftanim.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
// Actual complete resolvers refer to their real private slot fields. Other
// package loading/scene registration is controlled; no alternate resolver.
#define NATIVE_FIGHTER_ACTOR_CAPACITY (4 * NATIVE_FIGHTER_FAMILY_MEMBERS)
typedef struct NativeFighterSlot {
    bool active; const NativeSourcePackage* source; UnkCostumeStruct costume_entry;
} NativeFighterSlot;
static NativeSourcePackage package;
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return package.ready&&kind==package.target?&package:NULL;}

static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g) {(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g) {(void)o;(void)g;return NULL;}
static NativeFighterSlot slot_storage[NATIVE_FIGHTER_ACTOR_CAPACITY], *slots=slot_storage;
static const UnkCostumeStruct* selected_cosmetic;
const UnkCostumeStruct* nativeCostumeRuntime_ForFighter(const Fighter* fighter)
{ (void)fighter;return selected_cosmetic; }
#include "runtime.inc"

#include <melee/ft/ftmaterial.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/dobj.h>
#include "rig.inc"
static unsigned checks;
#define CHECK(c) do { ++checks; if(!(c)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#c);exit(1);} } while(0)
static NativeFighterPoolSlot fighter_slot;static FighterBone* parts;static HSD_DObj** dobjs;
NativeFighterPoolSlot* nativeFighterPool_Slot(Fighter* actor){return actor==&fighter_slot.actor?&fighter_slot:NULL;}
HSD_ObjAllocData fighter_parts_alloc_data,fighter_dobj_list_alloc_data;
HSD_MObjInfo ftMObj;
void* HSD_ObjAlloc(HSD_ObjAllocData* data) {
    if(data==&fighter_parts_alloc_data)return parts;
    CHECK(data==&fighter_dobj_list_alloc_data);return dobjs;
}
bool hsdChangeClass(void* object,void* info) {(void)object;(void)info;CHECK(false);return false;}
u32 HSD_JObjGetFlags(HSD_JObj* j) {return j->flags;}
HSD_DObj* HSD_JObjGetDObj(HSD_JObj* j) {return j->u.dobj;}
void OSReport(char* format,...) {(void)format;CHECK(false);}
void __assert(char* file,u32 line,char* text) {(void)file;(void)line;(void)text;abort();}
void HSD_Panic(char* file,u32 line,char* text) {(void)file;(void)line;(void)text;abort();}
#include "retail_setup.inc"

static void* read_file(const char* directory,const char* name,size_t* size) {
    char path[1024];snprintf(path,sizeof(path),"%s/%s",directory,name);
    FILE* f=fopen(path,"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long length=ftell(f);CHECK(length>0);rewind(f);
    void* bytes=malloc((size_t)length);CHECK(bytes);CHECK(fread(bytes,1,(size_t)length,f)==(size_t)length);fclose(f);*size=(size_t)length;return bytes;
}
// HSD allocation/material objects are controlled. Descriptor preorder, parent/
// sibling edges, flags and SRT are copied verbatim. The real shared part setup
// creates the runtime map and flags; instances are not descended, as in HSD.
static HSD_JObj live_nodes[MAX_FT_PARTS],alternate_nodes[MAX_FT_PARTS];
static HSD_JObj* materialize(HSD_Joint* descriptor,HSD_JObj* pool,unsigned* used,HSD_JObj* parent) {
    HSD_JObj* first=NULL,*previous=NULL;
    for(;descriptor;descriptor=descriptor->next) {
        CHECK(*used<MAX_FT_PARTS);HSD_JObj* j=&pool[(*used)++];*j=(HSD_JObj){0};
        j->flags=descriptor->flags;j->parent=parent;j->scale=descriptor->scale;j->translate=descriptor->position;
        j->rotate.x=descriptor->rotation.x;j->rotate.y=descriptor->rotation.y;j->rotate.z=descriptor->rotation.z;
        if(previous)previous->next=j;else first=j;previous=j;
        if(!(descriptor->flags&JOBJ_INSTANCE))j->child=materialize(descriptor->child,pool,used,j);
    }
    return first;
}
static bool validate(HSD_GObj* g,NativeFighterPoseOperation operation,int destination,int start,HSD_Joint* graph,FigaTree* clip) {
    return nativeFighterPose_Validate(g,operation,destination,start,graph,clip);
}
int main(int argc,char** argv) {
    CHECK(argc==2);size_t char_size,anim_size;
    void* char_bytes=read_file(argv[1],"PlSk.dat",&char_size);
    void* anim_bytes=read_file(argv[1],"PlSkAJ.dat",&anim_size);
    NativeFighterSlot* slot=&slots[0];package.assets.character.arena=calloc(1,FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(package.assets.character.arena);
    FtCharacterAssetLayout layout={.article_mask=0xf,.joint_graph_mask=0x30,.item_format=FtCharacterItems_Chain};
    CHECK(ftCharacterAssets_Import(char_bytes,char_size,&layout,"ftDataSeak",317,14,5,
        package.assets.character.arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,&package.assets.character));
    package.assets.animation_arena=calloc(1,FT_ANIMATION_IMPORT_ARENA_SIZE);CHECK(package.assets.animation_arena);
    FtAnimationBankStats stats;CHECK(ftAnimationBank_Import(anim_bytes, anim_size,
                (const FtAnimationBankTable[]){{ package.assets.character.data.xC, 317, false }}, 1,
                package.assets.animation_arena, FT_ANIMATION_IMPORT_ARENA_SIZE, &stats));
    package.owned_input.assets.animation=anim_bytes;package.owned_input.assets.animation_bytes=anim_size;
    NativeSourcePoseGraph graphs[]={{.name=1,.slot=4,.path="0"},{.name=2,.slot=5,.path="0"}};
    slot->active=true;package.target=Ft_Kind_CustomFirst;slot->source=&package;package.ready=package.assets.ready=true;
    package.assets.facts.animation_tag=Ft_Kind_CustomFirst;package.assets.facts.actions=317;package.assets.facts.pose_count=2;package.assets.facts.layout=layout;
    memcpy(package.assets.facts.poses,graphs,sizeof(graphs));
    package.assets.parts=(FighterPartsTable){.part_to_joint=semantic_rig,.parts_num=58};
    ftPartsTable=calloc(Ft_Kind_RuntimeCapacity,sizeof(*ftPartsTable));Fighter_804D6540=calloc(Ft_Kind_RuntimeCapacity,sizeof(*Fighter_804D6540));CHECK(ftPartsTable&&Fighter_804D6540);
    ftPartsTable[package.target]=&package.assets.parts;gFtDataList[package.target]=&package.assets.character.data;
    Fighter* f=&fighter_slot.actor;parts=fighter_slot.parts;dobjs=fighter_slot.primary_dobjs;
    HSD_GObj g={.classifier=HSD_GOBJ_CLASS_FIGHTER,.user_data=f};heads[HSD_GOBJ_PLINK_FIGHTER]=&g;
    f->kind=package.target;f->x8_spawnNum=UINT32_C(0xfedcba98);f->ft_data=&package.assets.character.data;f->x24=package.assets.character.data.xC;f->x58C=317;
    static const char* colors[]={"Nr","Re","Bu","Gr","Wh"};
    unsigned combinations=0;
    for(unsigned color=0;color<5;++color) {
        char filename[32],symbol[80];size_t costume_size;
        snprintf(filename,sizeof(filename),"PlSk%s.dat",colors[color]);
        snprintf(symbol,sizeof(symbol),"PlySeak5K%s_Share_joint",color?colors[color]:"");
        void* bytes=read_file(argv[1],filename,&costume_size);void* arena=calloc(1,FT_COSTUME_IMPORT_ARENA_SIZE);CHECK(arena);
        CHECK(ftCostumeAssets_Import(bytes,costume_size,symbol,NULL,arena,FT_COSTUME_IMPORT_ARENA_SIZE,
            FT_COSTUME_IMPORT_RECORD_CAPACITY,&package.assets.costume));CHECK(package.assets.costume.joint_count==58);
        slot->costume_entry=(UnkCostumeStruct){.joint=package.assets.costume.joint,.native_assets=&package.assets.costume};
        selected_cosmetic=color?&slot->costume_entry:NULL;f->x108_costume_joint=package.assets.costume.joint;
        unsigned current_count=0,alternate_count=0;
        g.hsd_obj=materialize(package.assets.costume.joint,live_nodes,&current_count,NULL);
        f->x8AC_animSkeleton=materialize(package.assets.costume.joint,alternate_nodes,&alternate_count,NULL);
        CHECK(current_count==58&&alternate_count==58);
        ftParts_80074E58(f);ftParts_SetupParts(&g);ftParts_8007462C(&g);
        CHECK(parts[1].flags_b4&&parts[semantic_rig[53]].flags_b4);
        CHECK(f->dobj_list.count==0);
        NativeFighterPoseAssets context;CHECK(nativeFighterRuntime_PoseAssets(&g,&context));
        for(int destination=0;destination<2;++destination) {
            for(int index=0;index<2;++index) {
                HSD_Joint* graph=nativeFighterRuntime_PoseGraph(&g,index);CHECK(graph);
                CHECK(graph==package.assets.character.joint_graphs[index+4]->child);
                CHECK(validate(&g,NativeFighterPose_CopyGraph,destination,1,graph,NULL));
                CHECK(validate(&g,NativeFighterPose_BlendGraph,destination,1,graph,NULL));++combinations;
            }
            CHECK(validate(&g,NativeFighterPose_CopyCostume,destination,1,NULL,NULL));
            for(int index=305;index<=308;index+=3) {
                FigaTree* clip=nativeFighterRuntime_PoseClip(&g,index);CHECK(clip);
                CHECK(validate(&g,NativeFighterPose_LoadAnimation,destination,0,NULL,clip));++combinations;
            }
            CHECK(validate(&g,NativeFighterPose_RequestFrame,destination,0,NULL,NULL));
            CHECK(validate(&g,NativeFighterPose_Evaluate,destination,0,NULL,NULL));
        }
        CHECK(validate(&g,NativeFighterPose_BlendToLive,0,1,NULL,NULL));
        CHECK(validate(&g,NativeFighterPose_CopyToLive,0,1,NULL,NULL));
        printf("retail %s: 58 physical/runtime, zero omissions, graphs4/5 and clips305/308 accepted\n",colors[color]);
        free(bytes);free(arena);
    }
    printf("PASS %u retail asset/runtime checks, %u graph/clip/destination combinations; actual import and part setup, controlled HSD materialization\n",checks,combinations);
    return 0;
}
