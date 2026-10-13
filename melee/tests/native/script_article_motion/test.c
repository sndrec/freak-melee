#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_article_motion.h>
#include <melee/ft/native_script_binding_common_properties.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/itgroundcoll.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage motion_package;
#define bank motion_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static Fighter fighters[64],alternate_fighter;static Item items[128],alternate_item;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned motion_mode,motion_path,motion_calls,read_probe_count,read_mutation;bool collision_result;
HSD_GObj* motion_source,*motion_owner,*motion_replacement;HSD_GObj*(*motion_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons,reflector_pairs,reflected_entries,reflector_rejections;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"Article motion %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source=
        "fighter:\n    behavior independent\narticle Probe states 1:\n"
        "    state bool match\n    state int owner_id\n    state int contact_id\n"
        "    state bool reflection_disabled\n    state bool detaches\n"
        "    state int reflector_id\n    state int reflected_owner_id\n    state int reflected_target_id\n"
        "    state bool reflected_fault\n    state int zero\n"
        "    on signal Observations:\n        item.state.contact_id = item.contact_fighter\n"
        "        item.state.reflection_disabled = item.reflection_owner_transfer_disabled\n"
        "        item.state.detaches = false\n        if attachment_owner.exists:\n"
        "            item.state.detaches = attachment_owner.fighter.action_detaches_items\n"
        "    on signal ReflectorRead:\n        item.state.reflector_id = item.reflector\n"
        "    on signal ReflectedFault:\n        item.state.reflected_fault = true\n"
        "    on reflected:\n        item.state.reflector_id = item.reflector\n"
        "        item.state.reflected_owner_id = item.current_owner\n"
        "        if item.reflection_owner_transfer_disabled:\n            item.state.reflected_target_id = item.reflector\n"
        "        else:\n            item.state.reflected_target_id = item.current_owner\n"
        "        if item.state.reflected_fault:\n            item.state.reflected_target_id = 1 / item.state.zero\n"
        "        return false\n"
        "    on signal Check:\n        item.state.match = item.attachment_active\n        item.state.owner_id = item.current_owner\n"
        "    on signal Debug:\n        item.inherit_owner_debug_display()\n"
        "    on signal CollisionTrue:\n        item.stage_collision(true)\n"
        "    on signal CollisionFalse:\n        item.stage_collision(false)\n"
        "    on signal CollisionDefault:\n        item.stage_collision()\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Zelda_DinFire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,NULL,&metadata,&motion_package.program,&diagnostic));
    motion_package.address=&motion_package;motion_package.target=Ft_Kind_CustomFirst;motion_package.ready=motion_package.assets.ready=true;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&motion_package.program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(void)
{
    CHECK(fighter_count<64);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=Ft_Kind_Link,.behavior_kind=Ft_Kind_Link,.x8_spawnNum=20+fighter_count};GObj_InitUserData(o,4,fighter_remove,f);return o;
}
static HSD_GObj* item(void)
{
    CHECK(item_count<128);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count++];*i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100+item_count,.xC4_article_data=&article};
    GObj_InitUserData(o,6,item_remove,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static void signal(HSD_GObj* o,const char* name){NativeScriptValue out;CHECK(nativeScriptItem_RunSignal(o,hash(name),0,&out)==NativeScriptVM_Ok);}
static bool match(HSD_GObj* o)
{NativeScriptReceiverView v;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&v));for(unsigned n=0;n<bank.declarations.value_count;++n)if(values[n].owner==v.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,"match")){NativeScriptValue out;CHECK(nativeScriptStorage_Read(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&out,&diagnostic));return out.as.u64!=0;}CHECK(false);return false;}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Item,.generation=((Item*)o->user_data)->x1C,.actor_id=((Item*)o->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};}
static bool invoke(HSD_GObj* o,uint32_t id,const NativeScriptValue* a,uint32_t n,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(o);return nativeScriptBindingArticleMotion_Invoke(&c,id,a,n,out,&diagnostic);}
static bool read(HSD_GObj* o,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(o);return nativeScriptBindingArticleMotion_Read(&c,NativeScriptBinding_ItemAttachmentActive,NULL,0,out,&diagnostic);}
static uint32_t owner_field(HSD_GObj* o)
{
    NativeScriptReceiverView v;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&v));
    for(unsigned n=0;n<bank.declarations.value_count;++n)
        if(values[n].owner==v.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,"owner_id")) {
            NativeScriptValue out;CHECK(nativeScriptStorage_Read(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&out,&diagnostic));
            CHECK(out.type==NativeScriptType_I32);return (uint32_t)out.as.i64;
        }
    CHECK(false);return 0;
}
static NativeScriptValue observation_field(HSD_GObj* o,const char* name)
{
    NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&view));
    for(unsigned n=0;n<bank.declarations.value_count;++n)
        if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
            NativeScriptValue out;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&out,&diagnostic));return out;
        }
    CHECK(false);return (NativeScriptValue){0};
}
static HSD_GObj* reuse_owner(void);
static void observation_checks(HSD_GObj* o,HSD_GObj* target,HSD_GObj* nonfighter)
{
    Item* data=o->user_data;Fighter* f=target->user_data;uint32_t generation=(uint32_t)f->x8_spawnNum;
    CHECK(nativeScriptItem_SetAttachmentOwner(o,target));
    for(unsigned present=0;present<3;++present)for(unsigned bit=0;bit<2;++bit){
        data->xCF4_fighterGObjUnk=present==0?NULL:present==1?nonfighter:target;
        data->xDCC_flag.b1=bit;f->x2070.x2071_b6=bit;signal(o,"Observations");
        NativeScriptValue contact=observation_field(o,"contact_id"),reflection=observation_field(o,"reflection_disabled"),detaches=observation_field(o,"detaches");
        CHECK(contact.type==NativeScriptType_I32&&(uint32_t)contact.as.i64==(present==2?generation:0));
        CHECK(reflection.type==NativeScriptType_Bool&&reflection.as.u64==bit);
        CHECK(detaches.type==NativeScriptType_Bool&&detaches.as.u64==bit);
    }
    data->xCF4_fighterGObjUnk=target;f->x8_spawnNum=(int32_t)UINT32_C(0xfffffff0);signal(o,"Observations");
    CHECK((uint32_t)observation_field(o,"contact_id").as.i64==UINT32_C(0xfffffff0));
    NativeScriptRuntimeContext high_context=context(o);NativeScriptValue high;
    CHECK(nativeScriptBindingArticleMotion_Read(&high_context,NativeScriptBinding_ItemContactFighter,NULL,0,&high,&diagnostic)&&high.type==NativeScriptType_U32&&high.as.u64==UINT32_C(0xfffffff0));
    f->x8_spawnNum=(int32_t)generation;
    HSD_GObj* stale=fighter();data->xCF4_fighterGObjUnk=stale;HSD_GObjFree(stale);signal(o,"Observations");CHECK(observation_field(o,"contact_id").as.u64==0);
    CHECK(nativeScriptItem_ClearAttachmentOwner(o));signal(o,"Observations");CHECK(!observation_field(o,"detaches").as.u64);CHECK(nativeScriptItem_SetAttachmentOwner(o,target));
    data->xCF4_fighterGObjUnk=target;
    NativeScriptRuntimeContext c=context(o);NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel;
    CHECK(!nativeScriptBindingArticleMotion_Read(&c,NativeScriptBinding_ItemContactFighter,&sentinel,1,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));
    c.scene_epoch=0;CHECK(!nativeScriptBindingArticleMotion_Read(&c,NativeScriptBinding_ItemReflectionOwnerTransferDisabled,NULL,0,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));
    c=(NativeScriptRuntimeContext){.source=target,.source_class=NativeCallbackSource_Fighter,.generation=generation,.scene_epoch=nativeGObjPool.lifetime};
    CHECK(nativeScriptBindingCommonProperties_Read(&c,NativeScriptBinding_FighterActionDetachesItems,&out,&diagnostic)&&out.type==NativeScriptType_Bool&&out.as.u64==f->x2070.x2071_b6);
    c.generation++;out=sentinel;CHECK(!nativeScriptBindingCommonProperties_Read(&c,NativeScriptBinding_FighterActionDetachesItems,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));
    c.generation=generation;c.scene_epoch++;CHECK(!nativeScriptBindingCommonProperties_Read(&c,NativeScriptBinding_FighterActionDetachesItems,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));
    for(unsigned mode=7;mode<=14;++mode){if(mode==10||mode==11)continue;
        motion_source=item();motion_owner=fighter();((Item*)motion_source->user_data)->xCF4_fighterGObjUnk=motion_owner;
        read_probe_count=0;read_mutation=mode;motion_reuse=mode==12?reuse_owner:NULL;out=sentinel;c=context(motion_source);
        CHECK(!nativeScriptBindingArticleMotion_Read(&c,NativeScriptBinding_ItemContactFighter,NULL,0,&out,&diagnostic)&&read_mutation==0&&!memcmp(&out,&sentinel,sizeof(out)));
        HSD_GObjFree(motion_source);if(mode==12)HSD_GObjFree(motion_replacement);else if(mode!=7)HSD_GObjFree(motion_owner);motion_source=NULL;motion_reuse=NULL;
    }
    HSD_GObj* bad=item();for(unsigned slot=0;slot<NATIVE_SCRIPT_ITEM_ACTORS;++slot)if(nativeScriptItemActors[slot].active&&nativeScriptItemActors[slot].object==bad)nativeScriptItemActors[slot].faulted=true;
    c=context(bad);out=sentinel;CHECK(!nativeScriptBindingArticleMotion_Read(&c,NativeScriptBinding_ItemContactFighter,NULL,0,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));
    CHECK(!nativeScriptBindingArticleMotion_Read(&c,NativeScriptBinding_ItemReflectionOwnerTransferDisabled,NULL,0,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));HSD_GObjFree(bad);
}
static void current_owner_checks(HSD_GObj* o,HSD_GObj* owner,HSD_GObj* nonfighter)
{
    Item* data=o->user_data;Fighter* f=owner->user_data;uint32_t saved=(uint32_t)f->x8_spawnNum;
    data->owner=NULL;signal(o,"Check");CHECK(owner_field(o)==0);
    data->owner=nonfighter;signal(o,"Check");CHECK(owner_field(o)==0);
    data->owner=owner;signal(o,"Check");CHECK(owner_field(o)==saved);
    f->x8_spawnNum=(int32_t)UINT32_C(0xfffffff0);signal(o,"Check");CHECK(owner_field(o)==UINT32_C(0xfffffff0));
    NativeScriptRuntimeContext c=context(o);NativeScriptValue out;CHECK(nativeScriptBindingArticleMotion_Read(&c,NativeScriptBinding_ItemCurrentOwner,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_U32&&out.as.u64==UINT32_C(0xfffffff0));
    f->x8_spawnNum=(int32_t)saved;
    HSD_GObj* retired=fighter();data->owner=retired;HSD_GObjFree(retired);signal(o,"Check");CHECK(owner_field(o)==0);
    data->owner=owner;
}
static HSD_GObj* reuse_item(void){motion_replacement=item();return motion_replacement;}
static HSD_GObj* reuse_owner(void){motion_replacement=fighter();return motion_replacement;}
static void mutate(unsigned mode)
{
    if(mode==1){NativeScriptValue out;CHECK(nativeScriptItem_RunHook(motion_source,NativeScriptItemHook_Destroyed,NULL,0,&out)==NativeScriptVM_Ok);}
    if(mode==2)HSD_GObjFree(motion_source);
    if(mode==3){HSD_GObjFree(motion_source);if(motion_reuse)motion_reuse();}
    if(mode==4){alternate_item=*(Item*)motion_source->user_data;motion_source->user_data=&alternate_item;}
    if(mode==5)((Item*)motion_source->user_data)->x1C++;
    if(mode==6){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==motion_source)nativeScriptItemActors[i].faulted=true;}
    if(mode==7)HSD_GObjFree(motion_owner);
    if(mode==8)((Fighter*)motion_owner->user_data)->kind=Ft_Kind_Zelda;
    if(mode==9){alternate_fighter=*(Fighter*)motion_owner->user_data;motion_owner->user_data=&alternate_fighter;}
    if(mode==10)((Item*)motion_source->user_data)->owner=NULL;
    if(mode==11)CHECK(nativeScriptItem_ClearAttachmentOwner(motion_source));
    if(mode==12){HSD_GObjFree(motion_owner);if(motion_reuse)motion_reuse();}
    if(mode==13)((Fighter*)motion_owner->user_data)->x8_spawnNum++;
    if(mode==14)motion_owner->classifier=HSD_GOBJ_CLASS_ITEM;
    if(mode==15)((Item*)motion_source->user_data)->xC64_reflectGObj=NULL;
}
void motion_native_completed(HSD_GObj* source)
{if(source==motion_source&&motion_mode){unsigned mode=motion_mode;motion_mode=0;mutate(mode);}}
void motion_read_probe(NativeScriptRuntimeContext* c)
{if(c->source!=motion_source)return;++read_probe_count;if(read_mutation&&read_probe_count==3){unsigned mode=read_mutation;read_mutation=0;mutate(mode);}}
static void reflector_pair(HSD_GObj* source)
{
    uint32_t expected=nativeFighterTarget_Reflector(source);NativeScriptValue out;NativeScriptRuntimeContext c=context(source);
    CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemReflector,NULL,0,&out,&diagnostic));
    CHECK(out.type==NativeScriptType_U32&&out.as.u64==expected);
    signal(source,"ReflectorRead");NativeScriptValue stored=observation_field(source,"reflector_id");
    CHECK(stored.type==NativeScriptType_I32&&(uint32_t)stored.as.i64==expected);++reflector_pairs;
}
static void reflector_checks(HSD_GObj* source,HSD_GObj* target,HSD_GObj* nonfighter)
{
    Item* data=source->user_data;Fighter* fp=target->user_data;uint32_t generation=(uint32_t)fp->x8_spawnNum;
    data->xC64_reflectGObj=NULL;reflector_pair(source);
    data->xC64_reflectGObj=nonfighter;reflector_pair(source);
    data->xC64_reflectGObj=target;reflector_pair(source);
    fp->x8_spawnNum=(int32_t)UINT32_C(0xfffffff0);reflector_pair(source);fp->x8_spawnNum=(int32_t)generation;
    HSD_GObj* retired=fighter();data->xC64_reflectGObj=retired;HSD_GObjFree(retired);reflector_pair(source);
    // This raw native field records a pointer, not a historical generation.
    HSD_GObj* replacement=fighter();CHECK(replacement==retired);reflector_pair(source);HSD_GObjFree(replacement);
    HSD_GObj* owned=fighter();Fighter* own=owned->user_data;own->kind=own->behavior_kind=motion_package.target;
    CHECK(nativeScriptFighter_Attach(owned));data->xC64_reflectGObj=owned;reflector_pair(source);HSD_GObjFree(owned);
    data->xC64_reflectGObj=target;
    NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel;NativeScriptRuntimeContext c=context(source);
    CHECK(!nativeScriptBindings_Read(&c,NativeScriptBinding_ItemReflector,&sentinel,1,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));++reflector_rejections;
    c.generation++;CHECK(!nativeScriptBindings_Read(&c,NativeScriptBinding_ItemReflector,NULL,0,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));++reflector_rejections;
    c=context(source);c.scene_epoch=0;CHECK(!nativeScriptBindings_Read(&c,NativeScriptBinding_ItemReflector,NULL,0,&out,&diagnostic)&&!memcmp(&out,&sentinel,sizeof(out)));++reflector_rejections;
    for(unsigned mode=1;mode<=15;++mode){if(mode==4||mode==10||mode==11)continue;
        motion_source=item();motion_owner=fighter();((Item*)motion_source->user_data)->xC64_reflectGObj=motion_owner;
        read_probe_count=0;read_mutation=mode;motion_reuse=mode==3?reuse_item:mode==12?reuse_owner:NULL;out=sentinel;c=context(motion_source);
        bool accepted=nativeScriptBindings_Read(&c,NativeScriptBinding_ItemReflector,NULL,0,&out,&diagnostic);
        if(accepted||read_mutation||memcmp(&out,&sentinel,sizeof(out)))fprintf(stderr,"reflector mode %u accepted=%d probe=%u pending=%u\n",mode,accepted,read_probe_count,read_mutation);
        CHECK(!accepted&&read_mutation==0&&!memcmp(&out,&sentinel,sizeof(out)));++reflector_rejections;
        if(mode==3){CHECK(motion_replacement==motion_source);HSD_GObjFree(motion_replacement);}else if(mode!=2)HSD_GObjFree(motion_source);
        if(mode==12){CHECK(motion_replacement==motion_owner);HSD_GObjFree(motion_replacement);}else if(mode!=7)HSD_GObjFree(motion_owner);
        motion_source=NULL;motion_reuse=NULL;
    }
}
static void reflected_checks(HSD_GObj* source,HSD_GObj* owner,HSD_GObj* nonfighter)
{
    HSD_GObj* reflector=fighter();((Fighter*)reflector->user_data)->team=7;((Fighter*)owner->user_data)->team=3;
    Item* data=source->user_data;uint32_t reflector_id=nativeFighterTarget_Id(reflector),owner_id=nativeFighterTarget_Id(owner);
    for(unsigned keep=0;keep<2;++keep){
        data->owner=owner;data->xC64_reflectGObj=reflector;data->x20_team_id=3;data->xDCC_flag.b1=keep;data->xDCC_flag.b0=true;
        data->xC74=17;data->xC78=(Vec2){4,5};data->xC80=(S32Vec2){6,7};data->xC88=31;data->xC8C=53;
        CHECK(!motion_reflected_entry(source));++reflected_entries;
        CHECK(data->owner==(keep?owner:reflector)&&data->x20_team_id==(keep?3:7));
        CHECK(data->xD90.x2070_int==data->xC74&&!memcmp(&data->xD94,&data->xC78,sizeof(data->xC78))&&!memcmp(&data->xD9C,&data->xC80,sizeof(data->xC80))&&data->xDA4_word==data->xC88&&data->xDA8_short==data->xC8C);
        CHECK((uint32_t)observation_field(source,"reflector_id").as.i64==reflector_id);
        CHECK((uint32_t)observation_field(source,"reflected_owner_id").as.i64==(keep?owner_id:reflector_id));
        CHECK((uint32_t)observation_field(source,"reflected_target_id").as.i64==reflector_id);
    }
    data->owner=owner;data->xC64_reflectGObj=nonfighter;data->xDCC_flag.b1=true;data->xDCC_flag.b0=true;
    CHECK(!motion_reflected_entry(source));++reflected_entries;
    CHECK(data->owner==owner&&(uint32_t)observation_field(source,"reflected_owner_id").as.i64==owner_id&&observation_field(source,"reflector_id").as.i64==0&&observation_field(source,"reflected_target_id").as.i64==0);
    HSD_GObj* faulted=item();Item* bad=faulted->user_data;bad->owner=owner;bad->xC64_reflectGObj=reflector;bad->xDCC_flag.b0=false;
    bad->x5D4_hitboxes[0].hit.state=HitCapsule_Enabled;bad->x5D4_hitboxes[0].hit.damage=3;bad->xC6C=1;
    signal(faulted,"ReflectedFault");CHECK(motion_reflected_entry(faulted));++reflected_entries;
    CHECK(bad->owner==reflector&&nativeScriptItem_Status(faulted)!=NativeScriptVM_Ok);
    HSD_GObjFree(faulted);HSD_GObjFree(reflector);data->owner=owner;data->xC64_reflectGObj=NULL;
}
int main(void)
{
    prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
    HSD_GObj* o=item(),*r=item(),*owner=fighter(),*nonfighter=item();Item* data=o->user_data,*reference=r->user_data;Fighter* fp=owner->user_data;
    current_owner_checks(o,owner,nonfighter);
    observation_checks(o,owner,nonfighter);
    reflector_checks(o,owner,nonfighter);
    reflected_checks(o,owner,nonfighter);
    data->owner=owner;CHECK(nativeScriptItem_SetAttachmentOwner(o,owner));CHECK(nativeScriptItem_Creator(o)==NULL);signal(o,"Check");CHECK(match(o));CHECK(nativeScriptItem_ClearAttachmentOwner(o));signal(o,"Check");CHECK(!match(o));CHECK(nativeScriptItem_SetAttachmentOwner(o,owner));data->owner=NULL;signal(o,"Check");CHECK(!match(o));
    for(unsigned n=0;n<256;++n)for(unsigned kind=0;kind<3;++kind)for(unsigned mode=0;mode<4;++mode){db_ShowItemCollisionBubbles=mode;fp->x21FC_flag.u8=(u8)((n*37+mode*53)&255);HSD_GObj* relation=kind==0?NULL:kind==1?owner:nonfighter;data->owner=reference->owner=relation;data->xDAA_byte=reference->xDAA_byte=(u8)n;
        NativeScriptValue out;CHECK(invoke(o,NativeScriptBinding_ItemInheritOwnerDebugDisplay,NULL,0,&out)&&out.type==NativeScriptType_Void);db_80225DD8(r,relation);CHECK(data->xDAA_byte==reference->xDAA_byte);++comparisons;
    }
    data->owner=owner;db_ShowItemCollisionBubbles=1;fp->x21FC_flag.u8=2;data->xDAA_byte=0xF1;signal(o,"Debug");CHECK(data->xDAA_byte==0xF2);
    for(unsigned branch=0;branch<3;++branch)for(unsigned result=0;result<2;++result){NativeScriptValue arg={.type=NativeScriptType_Bool,.as.u64=branch!=1},out;collision_result=result;data->pos=reference->pos=(Vec3){-1,-2,-3};data->xC30=reference->xC30=-9;motion_calls=0;
        uint32_t id=branch==2?NativeScriptBinding_ItemStageCollision:NativeScriptBinding_ItemStageCollisionMode;CHECK(invoke(o,id,branch==2?NULL:&arg,branch==2?0:1,&out));unsigned call_order=motion_calls,path=motion_path;motion_calls=0;if(branch==1)it_8026DA08(r);else it_8026D9A0(r);
        CHECK(call_order==motion_calls&&call_order==(branch==1?13u:12u)&&path==motion_path&&memcmp(&data->pos,&reference->pos,sizeof(Vec3))==0&&data->xC30==reference->xC30&&out.type==NativeScriptType_Void);++comparisons;
    }
    motion_calls=0;collision_result=true;signal(o,"CollisionFalse");CHECK(motion_calls==13&&data->xC30==23);motion_calls=0;signal(o,"CollisionTrue");CHECK(motion_calls==12&&data->xC30==17);motion_calls=0;signal(o,"CollisionDefault");CHECK(motion_calls==12);
    // Stale initial relations are false for a pure compare, but a native debug
    // operation must not inspect an unregistered object's classifier.
    HSD_GObj* stale=fighter();data->owner=stale;CHECK(nativeScriptItem_SetAttachmentOwner(o,stale));HSD_GObjFree(stale);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(read(o,&out)&&!out.as.u64);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};u8 byte=data->xDAA_byte;CHECK(!invoke(o,NativeScriptBinding_ItemInheritOwnerDebugDisplay,NULL,0,&out)&&out.as.u64==99&&data->xDAA_byte==byte);
    // Collision has no owner classifier access; dangling raw owner does not
    // block its otherwise healthy map operation.
    CHECK(invoke(o,NativeScriptBinding_ItemStageCollision,NULL,0,&out));data->owner=owner;CHECK(nativeScriptItem_SetAttachmentOwner(o,owner));
    NativeScriptValue bad={.type=NativeScriptType_I32,.as.i64=1};out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(o,NativeScriptBinding_ItemStageCollisionMode,&bad,1,&out)&&out.as.u64==99);CHECK(!invoke(o,NativeScriptBinding_ItemStageCollision,&bad,1,&out)&&out.as.u64==99);
    NativeScriptRuntimeContext invalid=context(o);invalid.scene_epoch=0;motion_calls=0;
    CHECK(!nativeScriptBindingArticleMotion_Invoke(&invalid,NativeScriptBinding_ItemStageCollision,NULL,0,&out,&diagnostic)&&out.as.u64==99&&motion_calls==0);
    invalid=context(o);invalid.source_class=NativeCallbackSource_Fighter;CHECK(!nativeScriptBindingArticleMotion_Read(&invalid,NativeScriptBinding_ItemAttachmentActive,NULL,0,&out,&diagnostic)&&out.as.u64==99);
    uint32_t saved_owner=data->native_script_owner;data->native_script_owner=UINT32_MAX;CHECK(!invoke(o,NativeScriptBinding_ItemInheritOwnerDebugDisplay,NULL,0,&out)&&out.as.u64==99);data->native_script_owner=saved_owner;
    HSD_GObj* faulted=item();for(unsigned slot=0;slot<NATIVE_SCRIPT_ITEM_ACTORS;++slot)if(nativeScriptItemActors[slot].active&&nativeScriptItemActors[slot].object==faulted)nativeScriptItemActors[slot].faulted=true;
    CHECK(!invoke(faulted,NativeScriptBinding_ItemStageCollision,NULL,0,&out)&&out.as.u64==99&&motion_calls==0);CHECK(!read(faulted,&out)&&out.as.u64==99);HSD_GObjFree(faulted);
    for(unsigned mode=1;mode<=12;++mode){if(mode==11)continue;motion_source=item();motion_owner=fighter();((Item*)motion_source->user_data)->owner=motion_owner;motion_mode=mode;motion_reuse=mode==3?reuse_item:mode==12?reuse_owner:NULL;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!invoke(motion_source,NativeScriptBinding_ItemInheritOwnerDebugDisplay,NULL,0,&out)&&motion_mode==0&&out.type==NativeScriptType_U64&&out.as.u64==99);
        if(mode==3){CHECK(motion_replacement==motion_source);HSD_GObjFree(motion_replacement);}else if(mode!=2)HSD_GObjFree(motion_source);if(mode==12){CHECK(motion_replacement==motion_owner);HSD_GObjFree(motion_replacement);}else if(mode!=7)HSD_GObjFree(motion_owner);motion_source=NULL;motion_reuse=NULL;
    }
    for(unsigned mode=7;mode<=12;++mode){motion_source=item();motion_owner=fighter();((Item*)motion_source->user_data)->owner=motion_owner;CHECK(nativeScriptItem_SetAttachmentOwner(motion_source,motion_owner));read_probe_count=0;read_mutation=mode;motion_reuse=mode==12?reuse_owner:NULL;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!read(motion_source,&out)&&read_mutation==0&&out.type==NativeScriptType_U64&&out.as.u64==99);HSD_GObjFree(motion_source);if(mode==12)HSD_GObjFree(motion_replacement);else if(mode!=7)HSD_GObjFree(motion_owner);motion_source=NULL;motion_reuse=NULL;
    }
    HSD_GObjFree(o);HSD_GObjFree(r);HSD_GObjFree(owner);HSD_GObjFree(nonfighter);CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS Article motion\",\"checks\":%u,\"native_comparisons\":%u,\"reflector_native_pairs\":%u,\"reflected_entry_cases\":%u,\"reflector_rejections\":%u}\n",checks,comparisons,reflector_pairs,reflected_entries,reflector_rejections);return 0;
}
