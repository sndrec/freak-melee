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
    bool active; FighterKind fighter_kind; const NativeFighterDefinition* definition;
    FtCharacterAssets character; FtCostumeAssets costume; FighterPartsTable parts;
    UnkCostumeStruct costume_entry; const uint8_t* animation_bytes; size_t animation_size;
    void* animation_arena;
} NativeFighterSlot;
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
static FighterBone parts[MAX_FT_PARTS];static HSD_DObj* dobjs[124];
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
    NativeFighterSlot* slot=&slots[0];slot->character.arena=calloc(1,FT_CHARACTER_IMPORT_ARENA_SIZE);CHECK(slot->character.arena);
    FtCharacterAssetLayout layout={.article_mask=0xf,.joint_graph_mask=0x30,.item_format=FtCharacterItems_Chain};
    CHECK(ftCharacterAssets_Import(char_bytes,char_size,&layout,"ftDataSeak",317,14,5,
        slot->character.arena,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,&slot->character));
    slot->animation_arena=calloc(1,FT_ANIMATION_IMPORT_ARENA_SIZE);CHECK(slot->animation_arena);
    FtAnimationBankStats stats;CHECK(ftAnimationBank_Import(anim_bytes, anim_size,
                (const FtAnimationBankTable[]){{ slot->character.data.xC, 317, false }}, 1,
                slot->animation_arena, FT_ANIMATION_IMPORT_ARENA_SIZE, &stats));
    slot->animation_bytes=anim_bytes;slot->animation_size=anim_size;
    NativeFighterPoseGraph graphs[]={{1,4,"0"},{2,5,"0"}};
    NativeFighterDefinition definition={.action_count=317,.pose_graph_count=2,.pose_graphs=graphs,.asset_layout=layout};
    slot->active=true;slot->fighter_kind=Ft_Kind_CustomFirst;slot->definition=&definition;
    slot->parts=(FighterPartsTable){.part_to_joint=semantic_rig,.parts_num=58};
    ftPartsTable=calloc(Ft_Kind_RuntimeCapacity,sizeof(*ftPartsTable));Fighter_804D6540=calloc(Ft_Kind_RuntimeCapacity,sizeof(*Fighter_804D6540));CHECK(ftPartsTable&&Fighter_804D6540);
    ftPartsTable[slot->fighter_kind]=&slot->parts;gFtDataList[slot->fighter_kind]=&slot->character.data;
    Fighter* f=calloc(1,sizeof(*f));CHECK(f);
    HSD_GObj g={.classifier=HSD_GOBJ_CLASS_FIGHTER,.user_data=f};heads[HSD_GOBJ_PLINK_FIGHTER]=&g;
    f->kind=slot->fighter_kind;f->x8_spawnNum=UINT32_C(0xfedcba98);f->ft_data=&slot->character.data;f->x24=slot->character.data.xC;f->x58C=317;
    static const char* colors[]={"Nr","Re","Bu","Gr","Wh"};
    unsigned combinations=0;
    for(unsigned color=0;color<5;++color) {
        char filename[32],symbol[80];size_t costume_size;
        snprintf(filename,sizeof(filename),"PlSk%s.dat",colors[color]);
        snprintf(symbol,sizeof(symbol),"PlySeak5K%s_Share_joint",color?colors[color]:"");
        void* bytes=read_file(argv[1],filename,&costume_size);void* arena=calloc(1,FT_COSTUME_IMPORT_ARENA_SIZE);CHECK(arena);
        CHECK(ftCostumeAssets_Import(bytes,costume_size,symbol,NULL,arena,FT_COSTUME_IMPORT_ARENA_SIZE,
            FT_COSTUME_IMPORT_RECORD_CAPACITY,&slot->costume));CHECK(slot->costume.joint_count==58);
        slot->costume_entry=(UnkCostumeStruct){.joint=slot->costume.joint,.native_assets=&slot->costume};
        selected_cosmetic=color?&slot->costume_entry:NULL;f->x108_costume_joint=slot->costume.joint;
        unsigned current_count=0,alternate_count=0;
        g.hsd_obj=materialize(slot->costume.joint,live_nodes,&current_count,NULL);
        f->x8AC_animSkeleton=materialize(slot->costume.joint,alternate_nodes,&alternate_count,NULL);
        CHECK(current_count==58&&alternate_count==58);
        ftParts_80074E58(f);ftParts_SetupParts(&g);ftParts_8007462C(&g);
        CHECK(parts[1].flags_b4&&parts[semantic_rig[53]].flags_b4);
        CHECK(f->dobj_list.count==0);
        NativeFighterPoseAssets context;CHECK(nativeFighterRuntime_PoseAssets(&g,&context));
        for(int destination=0;destination<2;++destination) {
            for(int index=0;index<2;++index) {
                HSD_Joint* graph=nativeFighterRuntime_PoseGraph(&g,index);CHECK(graph);
                CHECK(graph==slot->character.joint_graphs[index+4]->child);
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
