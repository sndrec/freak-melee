#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_holdable.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include "boundary.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=4096,F=64,V=128};
static char source[65536];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[N*2];static uint32_t args[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[16];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptItemPrograms prepared;static NativeScriptItemAsset bindings[9];
static Article article;static ItemAttr attrs;static ItemModelDesc model;static HSD_Joint descriptor;static ItemStateDesc animation;static ItemStateArray animations={&animation,1};
static uint8_t png[8][4096];static Item item;static HSD_JObj joint;static unsigned generation,checks,cases;static bool publication=true;
static NativeScriptDiagnostic diagnostic;
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){(void)kind;return NULL;}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){(void)kind;return NULL;}
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"Rook Item line %u: %s (%s / %s)\n",__LINE__,#x,diagnostic.message,nativeScriptRuntime_VM()->diagnostic.message);exit(1);}}while(0)
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{if(!publication)return NULL;for(unsigned i=0;i<prepared.count;++i)if(prepared.programs[i].author_kind==author&&prepared.programs[i].owner==owner)return &prepared.programs[i];return NULL;}
void rook_animation(HSD_GObj*);void rook_hit(HSD_GObj*);void rook_collision(HSD_GObj*);
static bool near(float a,float b){return fabsf(a-b)<0.00002f;}
static void prepare(const char* fms,const char* icons)
{
    FILE* file=fopen(fms,"rb");CHECK(file);char input[65536];size_t bytes=fread(input,1,sizeof(input),file);fclose(file);CHECK(bytes<sizeof(input));
    static const char* names[]={"ChuftyCookie","MuscleMilk","FortuneCandy","SportsDrink","MindBone","GummyShark","BirdSeed","PHYSICS_OBJECT","CrimeShard"};static char icon_names[8][32];
    model.x0_joint=&descriptor;article=(Article){.x0_common_attr=&attrs,.x10_modelDesc=&model,.xC_itemStates=&animations};attrs.x10_fall_speed=.25f;attrs.x14_fall_speed_max=3.5f;attrs.x60_scale=1;
    for(unsigned i=0;i<9;++i){bindings[i]=(NativeScriptItemAsset){.declaration=names[i],.kind=It_Kind_NativeHoldable,.article=&article,.blueprint_path="items/base.fib"};
        if(i<8){char path[1024];snprintf(path,sizeof(path),"%s/snack%u.png",icons,i);file=fopen(path,"rb");CHECK(file);size_t count=fread(png[i],1,sizeof(png[i]),file);fclose(file);CHECK(count&&count<sizeof(png[i]));snprintf(icon_names[i],32,"items/snack%u.png",i);bindings[i].icon_path=icon_names[i];bindings[i].icon_png=png[i];bindings[i].icon_bytes=(uint32_t)count;}}
    bank=(NativeScriptCompileBank){.source_storage=source,.source_capacity=sizeof(source),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N*2,.arguments=args,.argument_capacity=N*2},
        .declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,
        .execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=16},.parameter_slots=slots,.parameter_slot_capacity=V};
    NativeScriptItemMetadataContext metadata={.assets=bindings,.asset_count=9};NativeScriptCompileRequest request={.source=input,.source_bytes=(uint32_t)bytes,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptItemPrograms_Prepare(&bank,Ft_Kind_CustomFirst,88,&metadata,&prepared,&diagnostic));CHECK(prepared.count==9&&prepared.recipe_count==9);
    for(unsigned i=0;i<9;++i){const NativeScriptItemProgram* p=&prepared.programs[i];CHECK(p->recipe_index==i&&nativeScriptItem_ProgramValid(p)&&p->state_count==4);CHECK(!p->recipe->on_tick&&!p->recipe->on_thrown&&!p->recipe->on_hit&&!p->recipe->on_ground_collision);
        CHECK(p->recipe->billboard_png==(i<8?png[i]:NULL));for(unsigned phase=0;phase<3;++phase)for(unsigned state=0;state<4;++state)CHECK(p->states[state].functions[phase]==UINT32_MAX);}
    ++cases;
}
static HSD_GObj* create(unsigned recipe)
{
    scheduler_spec=&prepared.programs[recipe];HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);memset(&item,0,sizeof(item));memset(&joint,0,sizeof(joint));
    item=(Item){.entity=object,.kind=It_Kind_NativeHoldable,.x1C=++generation,.msid=-1,.native_article_author_kind=Ft_Kind_CustomFirst,.native_script_owner=scheduler_spec->owner,.xC4_article_data=&article,.xCC_item_attr=&attrs,.scl=1.25f,.spin_spd=2.5f,.facing_dir=-1,.ground_or_air=GA_Air,.x5D0_animFrameSpeed=1};
    item.xDD4_itemVar.native_holdable.owner_kind=Ft_Kind_CustomFirst;item.xDD4_itemVar.native_holdable.definition_index=recipe;object->hsd_obj=&joint;GObj_InitUserData(object,6,scheduler_userdata_remove,&item);CHECK(nativeScriptItem_Attach(object,scheduler_spec));Item_80268E5C(object,1,0);return object;
}
static void dispose(HSD_GObj* object)
{Item_8026A8EC(object);for(HSD_GObj* next=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];next;next=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM])Item_8026A8EC(next);CHECK(nativeScriptRuntime_Idle());}
static void corpus(void)
{
    static const float thrown_x[]={6.5f,7.5f,4.5f,-1.8f,7,-3,-3,-2,5.5f},thrown_y[]={3,4,.6f,.1f,3.5f,.5f,0,2,2.3f};
    static const unsigned damage[]={3,4,4,5,12,5,6,6,1},lifetime[]={180,160,90,110,140,110,140,140,100};NativeScriptValue result;
    for(unsigned recipe=0;recipe<9;++recipe){HSD_GObj* object=create(recipe);item.xDD4_itemVar.native_holdable.age=17;
        CHECK(nativeHoldable_PickedUp(object,&result)==NativeScriptVM_Ok&&item.msid==0);
        for(unsigned frame=0;frame<32;++frame)CHECK(nativeHoldable_Animation(object,&result)==NativeScriptVM_Ok&&!result.as.u64&&item.xDD4_itemVar.native_holdable.age==17);
        item.x40_vel=(Vec3){10,2,0};CHECK(nativeHoldable_Thrown(object,&result)==NativeScriptVM_Ok&&item.msid==2&&item.xDD4_itemVar.native_holdable.age==0);
        if(!near(item.x40_vel.x,thrown_x[recipe])||!near(item.x40_vel.y,thrown_y[recipe]))fprintf(stderr,"recipe %u throw (%g,%g) facing %g init %g\n",recipe,item.x40_vel.x,item.x40_vel.y,item.facing_dir,item.init_facing_dir);
        CHECK(near(item.x40_vel.x,thrown_x[recipe])&&near(item.x40_vel.y,thrown_y[recipe]));CHECK(item.x5D4_hitboxes[0].hit.damage==damage[recipe]&&item.x5D4_hitboxes[0].hit.state==HitCapsule_Enabled);++cases;
        float previous_y=item.x40_vel.y;CHECK(nativeHoldable_Physics(object,&result)==NativeScriptVM_Ok);
        bool custom=recipe==2||recipe==3||recipe==5||recipe==7;CHECK(near(item.x40_vel.y,previous_y-(custom?0:.25f)));
        CHECK(nativeHoldable_Animation(object,&result)==NativeScriptVM_Ok&&!result.as.u64&&item.xDD4_itemVar.native_holdable.age==1);
        if(recipe==2)CHECK(near(item.x40_vel.y,.35f));
        if(recipe==5)CHECK(near(item.x40_vel.x,-2.97f));
        if(recipe==7)CHECK(near(item.x40_vel.x,(float)(-2+sin(.47)*.24))&&near(item.x40_vel.y,(float)(2+cos(.41)*.1)));
        for(unsigned age=2;age<=31;++age)CHECK(nativeHoldable_Animation(object,&result)==NativeScriptVM_Ok&&!result.as.u64);
        if(recipe==3)CHECK(near(item.x40_vel.x,1.6f));
        if(recipe==5)CHECK(near(item.x40_vel.y,.5f+19*.18f));
        item.xDD4_itemVar.native_holdable.age=(u16)(lifetime[recipe]-1);CHECK(nativeHoldable_Animation(object,&result)==NativeScriptVM_Ok&&result.as.u64);dispose(object);++cases;
        object=create(recipe);item.xDD4_itemVar.native_holdable.age=27;CHECK(nativeHoldable_Dropped(object,&result)==NativeScriptVM_Ok&&item.msid==3&&item.xDD4_itemVar.native_holdable.age==0);dispose(object);++cases;
        object=create(recipe);Item_80268E5C(object,2,0);holdable_contact=0;unsigned queries=holdable_collision_queries;
        CHECK(nativeHoldable_Collision(object,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Bool&&!result.as.u64&&item.msid==2&&holdable_collision_queries==queries+1);
        holdable_contact=1;unsigned side_frees=scheduler_frees;rook_collision(object);
        if(recipe==1)CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok&&item.msid==2&&scheduler_frees==side_frees);
        else CHECK(!nativeScriptItem_IsAttached(object)&&scheduler_frees>side_frees);
        if(recipe==1)dispose(object);++cases;
        object=create(recipe);Item_80268E5C(object,2,0);holdable_contact=14;unsigned responses=holdable_collision_response,frees=scheduler_frees;rook_collision(object);
        if(recipe==1){CHECK(!nativeScriptItem_IsAttached(object)&&scheduler_frees>frees);HSD_GObj* food=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];CHECK(food&&((Item*)food->user_data)->kind==It_Kind_Foods);Item_8026A8EC(food);}
        else{CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok&&item.msid==1&&holdable_collision_response==responses+1);dispose(object);}++cases;
        object=create(recipe);Item_80268E5C(object,2,0);frees=scheduler_frees;rook_hit(object);
        if(recipe==1){CHECK(!nativeScriptItem_IsAttached(object)&&scheduler_frees>frees);HSD_GObj* food=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];CHECK(food&&((Item*)food->user_data)->kind==It_Kind_Foods);Item_8026A8EC(food);}
        else{CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok&&item.msid==1);dispose(object);}++cases;
        object=create(recipe);item.xDD4_itemVar.native_holdable.age=(u16)(lifetime[recipe]-1);rook_animation(object);CHECK(!nativeScriptItem_IsAttached(object));++cases;
    }
    HSD_GObj* object=create(0);NativeScriptValue unchanged={.type=NativeScriptType_U64,.as.u64=123};publication=false;u16 age=item.xDD4_itemVar.native_holdable.age;
    CHECK(!nativeScriptItem_Spec(object));CHECK(nativeHoldable_Animation(object,&unchanged)==NativeScriptVM_SourceRetired&&item.xDD4_itemVar.native_holdable.age==age&&unchanged.as.u64==123);CHECK(nativeHoldable_Thrown(object,&unchanged)==NativeScriptVM_SourceRetired);publication=true;dispose(object);++cases;
}
int main(int argc,char** argv)
{CHECK(argc==3);prepare(argv[1],argv[2]);HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();corpus();printf("{\"status\":\"PASS exact Rook Item corpus\",\"checks\":%u,\"cases\":%u,\"recipes\":9,\"native_frees\":%u}\n",checks,cases,scheduler_frees);return 0;}
