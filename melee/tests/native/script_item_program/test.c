#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_execution.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { Source=65536,Nodes=4096,Expressions=8192,Functions=128,Fields=128,Instructions=16384 };
typedef struct Buffers {
    char source[Source];NativeScriptSourceNode nodes[Nodes];NativeScriptIRNode expressions[Expressions];uint32_t arguments[Expressions];
    NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[Fields];NativeScriptValueDeclaration fields[Fields];
    NativeScriptLocal locals[128];NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];NativeScriptExecutionFunction executions[Functions];
    NativeScriptCompileFunctionScratch graph[Functions];NativeScriptStorageField storage[Fields];NativeScriptStorageOwner owners[128];NativeScriptParameterSlot slots[Fields];
} Buffers;
static Buffers buffers[2];static NativeScriptCompileBank banks[2];
static NativeScriptFighterProgram prepared,before,accepted_copy;
static NativeScriptItemAsset assets[8];static NativeScriptItemMetadataContext metadata;
static Article articles[4];static ItemAttr attributes;static HSD_Joint joint;
static ItemModelDesc model={.x0_joint=&joint};static ItemStateDesc animation_states[32];static ItemStateArray animations={animation_states,32};
static uint8_t* dats[2];static uint8_t original_dat[2][128];static uint8_t png[8]={137,80,78,71,13,10,26,10};
static NativeScriptParameterSupply supplies[Fields];static unsigned checks,cases,imported_scalars,aliases_checked;
static bool fighter_live,item_live;static NativeScriptDiagnostic diagnostic;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"line %u %s: %s (%u:%u)\n",(unsigned)__LINE__,#v,diagnostic.message,diagnostic.source.line,diagnostic.source.column);exit(1);}}while(0)
bool nativeScriptFighter_HasActors(FighterKind kind){return kind==Ft_Kind_Mario&&fighter_live;}
bool nativeScriptItem_HasActors(FighterKind kind){return kind==Ft_Kind_Mario&&item_live;}
static bool provider(void* context,const NativeScriptProgramIndex* index,const NativeScriptParameterSupply** out,uint32_t* count,NativeScriptDiagnostic* d)
{if(!nativeScriptItemMetadata_ImportParameters(index,context,supplies,Fields,count,d))return false;*out=supplies;return true;}
static void be32(uint8_t* bytes,uint32_t value){bytes[0]=(uint8_t)(value>>24);bytes[1]=(uint8_t)(value>>16);bytes[2]=(uint8_t)(value>>8);bytes[3]=(uint8_t)value;}
static void dat_setup(void)
{
    static HSD_Dat archives[2];
    for(unsigned n=0;n<2;++n){dats[n]=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(dats[n]);
        uint8_t* bytes=dats[n];be32(bytes,128);be32(bytes+4,80);be32(bytes+8,2);
        be32(bytes+32,32);be32(bytes+36,64);be32(bytes+112,0);be32(bytes+116,4);
        be32(bytes+64,n?UINT32_C(0x3f000000):UINT32_C(0x3fa00000));be32(bytes+68,(uint32_t)-123456);
        bytes[72]=1;bytes[73]=239;memcpy(original_dat[n],bytes,128);DWORD old;
        CHECK(VirtualProtect(bytes,4096,PAGE_READONLY,&old));
        CHECK(HSD_DatOpen(&archives[n],bytes,128)==HSD_DAT_OK);
    }
    for(unsigned n=0;n<4;++n)articles[n]=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model,.xC_itemStates=&animations,.native_special_attribute_source=32};
    assets[0]=(NativeScriptItemAsset){.declaration="SharedA",.kind=It_Kind_Mario_Fire,.article=&articles[0],.dat=&archives[0],.attribute_bytes=32};
    assets[1]=(NativeScriptItemAsset){.declaration="SharedB",.kind=It_Kind_Mario_Fire,.article=&articles[1],.dat=&archives[1],.attribute_bytes=32};
    assets[2]=(NativeScriptItemAsset){.declaration="Snack",.kind=It_Kind_NativeHoldable,.article=&articles[2],.icon_path="icons/snack.png",.icon_png=png,.icon_bytes=sizeof(png)};
    assets[3]=(NativeScriptItemAsset){.declaration="Pear",.kind=It_Kind_NativeHoldable,.article=&articles[3]};
    metadata=(NativeScriptItemMetadataContext){.assets=assets,.asset_count=4};
}
static void setup(unsigned i)
{
    Buffers* b=&buffers[i];banks[i]=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=Source,.tree={.nodes=b->nodes,.capacity=Nodes},
        .expressions={.nodes=b->expressions,.node_capacity=Expressions,.arguments=b->arguments,.argument_capacity=Expressions},
        .declarations={.functions=b->functions,.function_capacity=Functions,.parameters=b->parameters,.parameter_capacity=Fields,.values=b->fields,.value_capacity=Fields},
        .locals=b->locals,.local_capacity=128,.instructions=b->instructions,.instruction_capacity=Instructions,.function_code=b->code,.function_code_capacity=Functions,
        .execution_functions=b->executions,.execution_function_capacity=Functions,.graph=b->graph,.graph_capacity=Functions,
        .storage_layout={.fields=b->storage,.field_capacity=Fields,.owners=b->owners,.owner_capacity=128},.parameter_slots=b->slots,.parameter_slot_capacity=Fields};
}
static bool compile(const char* source,unsigned bank)
{
    setup(bank);diagnostic=(NativeScriptDiagnostic){0};const NativeScriptCompileBank* accepted=NULL;
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata,
        .parameter_provider=provider,.parameter_context=&metadata};
    return nativeScriptCompile_Build(&banks[bank],&request,&accepted,&diagnostic);
}
static bool prepare(unsigned bank)
{return nativeScriptFighter_PrepareProgram(Ft_Kind_Mario,&banks[bank],NULL,&metadata,&prepared,&diagnostic);}
static void unchanged(void)
{const NativeScriptFighterProgram* p=nativeScriptFighter_Program(Ft_Kind_Mario);CHECK(p&&!memcmp(p,&accepted_copy,sizeof(*p)));}
static void reject(const char* source)
{
    before=prepared;bool ok=compile(source,1)&&prepare(1);CHECK(!ok);CHECK(!memcmp(&before,&prepared,sizeof(prepared)));unchanged();++cases;
}
static uint32_t hash(const char* name){uint32_t value=UINT32_C(2166136261);for(;*name;++name){value^=(uint8_t)*name;value*=UINT32_C(16777619);}return value;}
static void pointers(const NativeScriptFighterProgram* p)
{
    for(unsigned i=0;i<p->items.count;++i){const NativeScriptItemProgram* item=&p->items.programs[i];
        CHECK(item->states>=p->items.states&&item->states+item->state_count<=p->items.states+p->items.state_count);
        if(item->signals)CHECK(item->signals>=p->items.signals&&item->signals+item->signal_count<=p->items.signals+p->items.signal_count);
        if(item->recipe){CHECK(item->recipe_index<p->items.recipe_count);
            CHECK(item->recipe==&p->items.recipes[item->recipe_index]&&item->recipe->name==p->items.recipe_names[item->recipe_index]);
            CHECK(!item->recipe->on_tick&&!item->recipe->on_thrown&&!item->recipe->on_hit&&!item->recipe->on_ground_collision);}
        if(item->projectile)CHECK(item->projectile==&p->items.projectiles[i]);
        if(item->attachment)CHECK(item->attachment==&p->items.attachments[i]);
        CHECK(item->bank==p->bank&&item->identity);
    }
}
// The execution-owner record builder is compiled from exact current source;
// runtime execution/receivers are outside this preparation-only fixture.
#include "build_functions.inc"
int main(void)
{
    dat_setup();static char source[Source];
    strcpy(source,"fighter:\n    behavior independent\narticle SharedA states 32:\n"
        "    attribute float acceleration 0\n    attribute int signed_value 4\n    attribute bool enabled 8\n    attribute byte category 9\n"
        "    parameter float duration = 35\n"
        "    projectile appearance 2 state 0 speed acceleration angle 0 lifetime duration auto_lifetime off initial_velocity on initial_update off\n"
        "    attachment origin center previous bone plane on initial_collision off held on\n"
        "    state int count\n    on spawned:\n            item.state.count = 1\n"
        "    on signal Zebra:\n            pass\n    on signal Alpha:\n            pass\n"
        "    state 0 anim 0:\n        on physics:\n            pass\n");
    for(unsigned i=1;i<32;++i){char line[96];snprintf(line,sizeof(line),"    state %u anim %u use %u:\n",i,i,i-1);strcat(source,line);}
    strcat(source,"article SharedB states 1:\n    attribute float acceleration 0\n    state 0 anim 0:\n"
        "item Snack damage 4 angle 45 lifetime 90:\n    icon icons/snack.png\n    billboard_size 12.5\n    hitbox_size 3.25\n    knockback_growth 83\n    base_knockback 21\n    on tick:\n        pass\n"
        "item Pear damage 7 angle 22 lifetime 125:\n    on hit:\n        return false\n");
    CHECK(compile(source,0));CHECK(prepare(0));pointers(&prepared);CHECK(prepared.items.count==4&&prepared.items.state_count==41&&prepared.items.signal_count==2);
    const NativeScriptItemProgram* a=&prepared.items.programs[0],*b=&prepared.items.programs[1],*snack=&prepared.items.programs[2],*pear=&prepared.items.programs[3];
    CHECK(a->kind==b->kind&&a->owner!=b->owner&&a->identity!=b->identity&&a->article==assets[0].article&&b->article==assets[1].article);
    for(unsigned i=0;i<32;++i){CHECK(a->states[i].animation_id==(int32_t)i&&a->states[i].functions[NativeScriptItemPhase_Physics]==a->states[0].functions[NativeScriptItemPhase_Physics]);++aliases_checked;}
    CHECK(a->signals[0].signal_id<a->signals[1].signal_id&&a->signals[0].signal_id==(hash("Zebra")<hash("Alpha")?hash("Zebra"):hash("Alpha")));
    CHECK(a->projectile&&a->projectile->speed.value==1.25f&&a->projectile->speed.parameter_semantic!=UINT32_MAX&&a->projectile->angle.value==0&&a->projectile->angle.parameter_semantic==UINT32_MAX&&a->projectile->lifetime.value==35&&
        a->projectile->appearance_state==2&&a->projectile->item_state==0&&!a->projectile->auto_lifetime&&a->projectile->initial_velocity&&!a->projectile->initial_update);
    CHECK(a->attachment&&a->attachment->origin_at_center&&!a->attachment->origin_at_owner&&a->attachment->previous_at_bone&&a->attachment->flatten_z&&!a->attachment->initial_collision&&a->attachment->register_as_held);++cases;
    CHECK(snack->recipe_index==0&&pear->recipe_index==1&&snack->recipe->damage==4&&pear->recipe->damage==7&&snack->recipe->billboard_size==12.5f&&snack->recipe->hitbox_size==3.25f&&snack->recipe->billboard_png==png&&snack->recipe->billboard_png_size==8);
    for(unsigned i=0;i<banks[0].declarations.value_count;++i){const NativeScriptValueDeclaration* field=&banks[0].declarations.values[i];if(!field->imported)continue;
        CHECK(banks[0].parameter_slots[i].resolved);NativeScriptValue value=banks[0].parameter_slots[i].value;
        if(field->type==NativeScriptType_F32)CHECK(value.as.f32==(field->owner==a->owner?1.25f:.5f));
        else if(field->type==NativeScriptType_I32)CHECK(value.as.i64==-123456);else CHECK(value.as.u64==(field->type==NativeScriptType_Bool?1:239));++imported_scalars;
    }
    CHECK(imported_scalars==5);
    // The real common Item constructor accepts a model descriptor whose root
    // is NULL and creates a runtime joint anchor. Test both ordinary Articles
    // and workshop Items without inventing a mesh in their immutable assets.
    model.x0_joint=NULL;
    CHECK(compile(source,1));CHECK(prepare(1));pointers(&prepared);
    CHECK(prepared.items.count==4);
    for(unsigned i=0;i<prepared.items.count;++i) {
        CHECK(prepared.items.programs[i].article->x10_modelDesc==&model);
        CHECK(!prepared.items.programs[i].article->x10_modelDesc->x0_joint);
    }
    ++cases;model.x0_joint=&joint;CHECK(prepare(0));
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptFighter_PublishProgram(Ft_Kind_Mario,&prepared);
    const NativeScriptFighterProgram* accepted=nativeScriptFighter_Program(Ft_Kind_Mario);CHECK(accepted&&accepted!=&prepared);pointers(accepted);accepted_copy=*accepted;
    CHECK(nativeScriptItem_Program(Ft_Kind_Mario,a->owner)==&accepted->items.programs[0]);
    memset(&prepared,0xA5,sizeof(prepared));pointers(accepted);CHECK(!memcmp(accepted,&accepted_copy,sizeof(*accepted)));++cases;
    fighter_live=true;CHECK(!nativeScriptFighter_CanPublish(Ft_Kind_Mario));fighter_live=false;item_live=true;CHECK(!nativeScriptFighter_CanPublish(Ft_Kind_Mario));item_live=false;
    nativeScriptRuntime_VM()->depth=1;CHECK(!nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptRuntime_VM()->depth=0;
    nativeScriptRuntime_VM()->register_top=1;CHECK(!nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptRuntime_VM()->register_top=0;
    nativeScriptRuntime_Payloads()->depth=1;CHECK(!nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptRuntime_Payloads()->depth=0;CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));unchanged();++cases;
    nativeScriptRuntime_Receivers()->depth=1;CHECK(!nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptRuntime_Receivers()->depth=0;
    nativeScriptRuntime_Receivers()->top=1;CHECK(!nativeScriptFighter_CanPublish(Ft_Kind_Mario));nativeScriptRuntime_Receivers()->top=0;CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Mario));++cases;
    static char cycle32[4096],duplicate32[4096];strcpy(cycle32,"fighter:\n    behavior independent\narticle SharedA states 32:\n");strcpy(duplicate32,cycle32);
    for(unsigned i=0;i<32;++i){char line[96];snprintf(line,sizeof(line),"    state %u anim %u use %u:\n",i,i,i?i-1:31);strcat(cycle32,line);
        snprintf(line,sizeof(line),"    state %u anim %u:\n",i,i);strcat(duplicate32,line);}
    strcat(duplicate32,"    state 31 anim 31:\n");reject(cycle32);reject(duplicate32);
    reject("fighter:\n    behavior independent\narticle SharedA states 2:\n    state 0 anim 0 use 1:\n    state 1 anim 1 use 0:\n");
    reject("fighter:\n    behavior independent\narticle SharedA states 2:\n    state 0 anim 0:\n    state 0 anim 1:\n");
    reject("fighter:\n    behavior independent\narticle SharedA states 2:\n    state 0 anim 0:\n");
    reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    state 0 anim 32:\n");
    reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    state 0 anim 0 use 0:\n");
    reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    on spawned:\n            pass\n    on spawned:\n            pass\n    state 0 anim -1:\n");
    reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    on signal Same:\n            pass\n    on signal Same:\n            pass\n    state 0 anim -1:\n");
    reject("fighter:\n    behavior independent\nitem Snack damage 4 damage 5 angle 45 lifetime 90:\n");
    reject("fighter:\n    behavior independent\nitem Snack damage 4 angle 45 lifetime 90:\n    icon absent.png\n");
    reject("fighter:\n    behavior independent\nitem Snack damage 4 angle 45 lifetime 90:\n    hitbox_size -1\n");
    reject("fighter:\n    behavior independent\nitem Snack damage 4 angle 45 lifetime 90:\n    state 0 anim -1:\n");
    const char* invalid_projectiles[]={
        "projectile appearance -1 state 0 speed absent angle 0 lifetime 10",
        "projectile appearance -1 state 0 speed 0 angle 0 lifetime 0",
        "projectile appearance 32 state 0 speed 0 angle 0 lifetime 10",
        "projectile appearance -1 state 1 speed 0 angle 0 lifetime 10",
        "projectile appearance -1 state 0 speed 0 speed 1 angle 0 lifetime 10",
        "projectile appearance -1 state 0 speed nan angle 0 lifetime 10",
        "projectile appearance -1 state 0 speed 0 angle 0 lifetime 10 auto_lifetime maybe",
        "attachment origin center origin bone",
        "attachment origin absent",
        "attachment origin bone previous absent"
    };
    for(unsigned i=0;i<sizeof(invalid_projectiles)/sizeof(invalid_projectiles[0]);++i){char text[1024];snprintf(text,sizeof(text),"fighter:\n    behavior independent\narticle SharedA states 1:\n    %s\n    state 0 anim -1:\n",invalid_projectiles[i]);reject(text);}
    reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    parameter int integer_speed = 1\n    projectile appearance -1 state 0 speed integer_speed angle 0 lifetime 10\n    state 0 anim -1:\n");
    reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    projectile appearance -1 state 0 speed 0 angle 0 lifetime 10\n    projectile appearance -1 state 0 speed 0 angle 0 lifetime 10\n    state 0 anim -1:\n");
    const char* imports="fighter:\n    behavior independent\narticle SharedA states 1:\n    attribute float value 2\n    state 0 anim -1:\n";reject(imports);
    NativeScriptItemAsset saved=assets[0];assets[0].article=NULL;reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    state 0 anim -1:\n");assets[0]=saved;
    articles[0].x10_modelDesc=NULL;reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    state 0 anim -1:\n");articles[0].x10_modelDesc=&model;
    articles[0].x0_common_attr=NULL;reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    state 0 anim -1:\n");articles[0].x0_common_attr=&attributes;
    assets[4]=assets[0];metadata.asset_count=5;reject("fighter:\n    behavior independent\narticle SharedA states 1:\n    state 0 anim -1:\n");metadata.asset_count=4;
    assets[2].kind=It_Kind_Mario_Fire;reject("fighter:\n    behavior independent\nitem Snack damage 4 angle 45 lifetime 90:\n");assets[2].kind=It_Kind_NativeHoldable;
    // Import failure must publish neither an early validated prefix nor count.
    NativeScriptParameterSupply saved_supplies[Fields];memcpy(saved_supplies,supplies,sizeof(supplies));uint32_t count=777;
    CHECK(!nativeScriptItemMetadata_ImportParameters(&banks[0].declarations,&metadata,supplies,1,&count,&diagnostic));
    CHECK(count==777&&!memcmp(saved_supplies,supplies,sizeof(supplies)));++cases;
    unsigned imported=UINT32_MAX;for(unsigned i=0;i<banks[0].declarations.value_count;++i)if(banks[0].declarations.values[i].imported){imported=i;break;}CHECK(imported!=UINT32_MAX);
    NativeScriptValueDeclaration saved_field=banks[0].declarations.values[imported];banks[0].declarations.values[imported].import_offset=32;
    CHECK(!nativeScriptItemMetadata_ImportParameters(&banks[0].declarations,&metadata,supplies,Fields,&count,&diagnostic));
    CHECK(count==777&&!memcmp(saved_supplies,supplies,sizeof(supplies)));banks[0].declarations.values[imported]=saved_field;++cases;
    DWORD old;CHECK(VirtualProtect(dats[0],4096,PAGE_READWRITE,&old));dats[0][72]=2;
    CHECK(!nativeScriptItemMetadata_ImportParameters(&banks[0].declarations,&metadata,supplies,Fields,&count,&diagnostic));CHECK(count==777&&!memcmp(saved_supplies,supplies,sizeof(supplies)));dats[0][72]=1;
    be32(dats[0]+64,UINT32_C(0x7f800000));CHECK(!nativeScriptItemMetadata_ImportParameters(&banks[0].declarations,&metadata,supplies,Fields,&count,&diagnostic));CHECK(count==777&&!memcmp(saved_supplies,supplies,sizeof(supplies)));be32(dats[0]+64,UINT32_C(0x3fa00000));
    CHECK(VirtualProtect(dats[0],4096,PAGE_READONLY,&old));++cases;
    static NativeScriptItemPrograms direct;bool direct_asset_bypass=false;
    static NativeScriptItemPrograms direct_before;memset(&direct,0x5A,sizeof(direct));direct_before=direct;
    const char* duplicate_fields[]={"icon icons/snack.png","billboard_size 4","hitbox_size 3","knockback_growth 80","base_knockback 20"};
    bool duplicate_fields_accepted=false;
    for(unsigned key=0;key<5;++key) {
        char repeated[512];snprintf(repeated,sizeof(repeated),"fighter:\n    behavior independent\nitem Snack damage 4 angle 45 lifetime 90:\n    %s\n    %s\n",duplicate_fields[key],duplicate_fields[key]);
        CHECK(compile(repeated,1));CHECK(!prepare(1));unchanged();
        CHECK(!nativeScriptItemPrograms_Prepare(&banks[1],Ft_Kind_Mario,accepted->identity,&metadata,&direct,&diagnostic));
        CHECK(!memcmp(&direct,&direct_before,sizeof(direct)));++cases;
    }
    uint32_t original_count=animations.count;animations.count=1;
    direct_asset_bypass=nativeScriptItemPrograms_Prepare(&banks[0],Ft_Kind_Mario,accepted->identity,&metadata,&direct,&diagnostic);animations.count=original_count;
    CHECK(!direct_asset_bypass);CHECK(!memcmp(&direct,&direct_before,sizeof(direct)));++cases;
    CHECK(!memcmp(dats[0],original_dat[0],128)&&!memcmp(dats[1],original_dat[1],128));
    for(unsigned i=0;i<2;++i)CHECK(VirtualFree(dats[i],0,MEM_RELEASE));
    printf("{\"cases\":%u,\"checks\":%u,\"imported_scalars\":%u,\"alias_states\":%u,\"published_entities\":%u,\"duplicate_recipe_fields_accepted\":%s,\"direct_prepare_asset_bounds_bypassed\":%s,\"status\":\"PASS\"}\n",cases,checks,imported_scalars,aliases_checked,accepted->items.count,duplicate_fields_accepted?"true":"false",direct_asset_bypass?"true":"false");return 0;
}
