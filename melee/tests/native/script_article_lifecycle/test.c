#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_article_lifecycle.h>
#include <melee/ft/native_script_binding_fighter_items.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage lifecycle_package;
#define bank lifecycle_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;
static Fighter fighters[64];static Item items[128];static FighterBone parts[4];static HSD_JObj joints[4],roots[128];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned lifecycle_mode,lifecycle_pickups,lifecycle_tails,lifecycle_calls;HSD_GObj* lifecycle_creator;HSD_GObj*(*lifecycle_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons;static uint32_t next_handle;static HSD_GObj* replacement;
static uint8_t saved[131072];static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"lifecycle %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    const char* source="fighter:\n    behavior independent\nstate int handle\nstate bool bound\naction Hooks:\n    on tick:\n        state.bound = articles.bind_auxiliary(state.handle)\narticle Probe states 1:\n    state int pickups\n    on picked_up:\n        item.state.pickups = item.state.pickups + 1\n        position.x = 7\n    on signal Attach:\n        item.attach_to_owner(1, true)\n    on signal DefaultAttach:\n        item.attach_to_owner(2)\n    on signal ClearCurrent:\n        item.clear_current_owner()\n    on signal ClearAttachment:\n        item.clear_attachment_owner()\n    on signal Detach:\n        item.detach_owner()\n    on signal Unbind:\n        item.owner_bound = false\n    state 0 anim -1:\n";
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Seak_Chain,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
    NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&lifecycle_package.program,&diagnostic));
    lifecycle_package.address=&lifecycle_package;lifecycle_package.target=Ft_Kind_CustomFirst;lifecycle_package.ready=lifecycle_package.assets.ready=true;
    lifecycle_package.assets.facts.moves=layout;lifecycle_package.assets.facts.joints=4;lifecycle_package.assets.parts.parts_num=4;
    tables[Ft_Kind_CustomFirst]=&lifecycle_package.assets.parts;gFtDataList[Ft_Kind_CustomFirst]=&lifecycle_package.assets.character.data;
    ftData_CharacterStateTables[Ft_Kind_CustomFirst]=lifecycle_package.assets.motions;
    for(unsigned i=0;i<4;++i)parts[i].joint=&joints[i];
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&lifecycle_package.program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned)
{
    CHECK(fighter_count<64);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,
        .x8_spawnNum=19+fighter_count,.ft_data=&lifecycle_package.assets.character.data,.parts=parts,.player_id=2,.team=3};
    GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(HSD_GObj* creator)
{
    CHECK(item_count<128);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count];HSD_JObj* r=&roots[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,
        .x1C=next_handle?next_handle:100+item_count,.owner=creator,.xC4_article_data=&article,.sfx_unk2=12};
    o->hsd_obj=r;GObj_InitUserData(o,6,item_remove,i);nativeItem_CaptureCreator(o,creator);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static HSD_GObj* reuse(void){replacement=item(lifecycle_creator);return replacement;}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    uint32_t kind=o->classifier==HSD_GOBJ_CLASS_FIGHTER?NativeCallbackSource_Fighter:NativeCallbackSource_Item;
    uint32_t gen=kind==NativeCallbackSource_Fighter?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(kind==NativeCallbackSource_Fighter?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    for(unsigned n=0;n<view.program->value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));
        CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static void signal(HSD_GObj* o,const char* name)
{NativeScriptValue v;CHECK(nativeScriptItem_RunSignal(o,hash(name),0,&v)==NativeScriptVM_Ok);}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};
}
static bool invoke(HSD_GObj* o,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(o);return semantic==NativeScriptBinding_ArticlesBindAuxiliary?nativeScriptBindingFighterItems_Invoke(&c,semantic,args,count,out,&diagnostic):nativeScriptBindingArticleLifecycle_Invoke(&c,semantic,args,count,out,&diagnostic);}
void lifecycle_mutation(HSD_GObj* object)
{
    unsigned mode=lifecycle_mode;if(mode!=9)lifecycle_mode=0;
    if(mode==1){((Item*)object->user_data)->owner=NULL;CHECK(nativeScriptItem_ClearAttachmentOwner(object));}
    if(mode==2||mode==7){NativeScriptValue ignored;CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_Destroyed,NULL,0,&ignored)==NativeScriptVM_Ok);HSD_GObjFree(object);if(mode==7&&lifecycle_reuse)lifecycle_reuse();}
    if(mode==3)HSD_GObjFree(lifecycle_creator);
    if(mode==4){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].object==object&&nativeScriptItemActors[i].active)nativeScriptItemActors[i].faulted=true;}
    if(mode==5){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].object==object&&nativeScriptItemActors[i].active)nativeScriptItemActors[i].storage.owner=0;}
    if(mode==6)((Item*)object->user_data)->x1C++;
}
int main(void)
{
    prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
    lifecycle_creator=fighter(true);HSD_GObj* o=item(lifecycle_creator);Fighter* f=lifecycle_creator->user_data;Item* i=o->user_data;
    signal(o,"Attach");CHECK(i->owner==lifecycle_creator&&i->xDC4==1&&i->x20_team_id==3&&i->xDC8_word.flags.x13&&f->x1984_heldItemSpec==o);
    CHECK(field(o,"pickups",NULL).as.i64==1&&lifecycle_pickups==1&&i->sfx_unk2==-1);
    f->x1984_heldItemSpec=NULL;signal(o,"DefaultAttach");CHECK(i->xDC4==2&&f->x1984_heldItemSpec==NULL&&field(o,"pickups",NULL).as.i64==2);
    f->x1984_heldItemSpec=o;signal(o,"ClearAttachment");CHECK(!nativeScriptItem_AttachmentOwner(o)&&nativeScriptItem_Creator(o)==lifecycle_creator&&i->owner==lifecycle_creator&&i->xDC8_word.flags.x13&&f->x1984_heldItemSpec==o);
    unsigned before=lifecycle_calls;signal(o,"Attach");CHECK(before==lifecycle_calls&&field(o,"pickups",NULL).as.i64==2);
    CHECK(nativeScriptItem_SetAttachmentOwner(o,lifecycle_creator));signal(o,"ClearCurrent");CHECK(!i->owner&&nativeScriptItem_AttachmentOwner(o)==lifecycle_creator&&nativeScriptItem_Creator(o)==lifecycle_creator);
    i->owner=lifecycle_creator;signal(o,"Detach");CHECK(!i->owner&&!nativeScriptItem_AttachmentOwner(o)&&nativeScriptItem_Creator(o)==lifecycle_creator&&i->xDC8_word.flags.x13&&f->x1984_heldItemSpec==o);
    signal(o,"Unbind");CHECK(!i->xDC8_word.flags.x13);
    // Reflection changes damage owner; original-creator auxiliary binding stays valid.
    HSD_GObj* foreign=fighter(false);i->owner=foreign;NativeScriptValue handle={.type=NativeScriptType_I32,.as.i64=i->x1C};field(lifecycle_creator,"handle",&handle);
    NativeScriptValue out;CHECK(nativeScriptFighter_Run(lifecycle_creator,NativeScriptFighter_Tick,NULL,0, NULL,&out)==NativeScriptVM_Ok);CHECK(field(lifecycle_creator,"bound",NULL).as.u64==1&&f->x1984_heldItemSpec==o);
    HSD_GObj* other=item(foreign);handle.as.i64=((Item*)other->user_data)->x1C;CHECK(invoke(lifecycle_creator,NativeScriptBinding_ArticlesBindAuxiliary,&handle,1,&out)&&!out.as.u64&&f->x1984_heldItemSpec==o);
    for(unsigned h=0;h<2;++h){handle.as.i64=h?-1:123456;CHECK(invoke(lifecycle_creator,NativeScriptBinding_ArticlesBindAuxiliary,&handle,1,&out)&&!out.as.u64);}
    handle=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000001)};out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(lifecycle_creator,NativeScriptBinding_ArticlesBindAuxiliary,&handle,1,&out)&&out.type==NativeScriptType_U64&&out.as.u64==99);
    next_handle=UINT32_C(0x80000005);HSD_GObj* wide=item(lifecycle_creator);next_handle=0;handle=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-2147483643};CHECK(invoke(lifecycle_creator,NativeScriptBinding_ArticlesBindAuxiliary,&handle,1,&out)&&out.as.u64);
    ((Item*)wide->user_data)->native_removal_in_progress=true;CHECK(invoke(lifecycle_creator,NativeScriptBinding_ArticlesBindAuxiliary,&handle,1,&out)&&out.as.u64);((Item*)wide->user_data)->native_removal_in_progress=false;
    // Exact raw joint semantics: no alias mapping, no truncation, OOB no-op.
    CHECK(nativeScriptItem_SetAttachmentOwner(o,lifecycle_creator));i->owner=lifecycle_creator;
    for(unsigned x=0;x<3;++x){NativeScriptValue raw={.type=NativeScriptType_I32,.as.i64=x==0?-1:x==1?4:INT32_MAX};before=lifecycle_calls;CHECK(invoke(o,NativeScriptBinding_ItemAttachToOwner,&raw,1,&out)&&before==lifecycle_calls);}
    NativeScriptValue malformed={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!invoke(o,NativeScriptBinding_ItemAttachToOwner,&malformed,1,&out)&&out.as.u64==99);
    malformed=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};CHECK(!invoke(o,NativeScriptBinding_ItemAttachToOwner,&malformed,1,&out)&&out.as.u64==99);
    NativeScriptValue wrong_flag[]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_I32,.as.i64=1}};
    CHECK(!invoke(o,NativeScriptBinding_ItemAttachToOwnerAux,wrong_flag,2,&out)&&out.as.u64==99);
    CHECK(!invoke(o,NativeScriptBinding_ItemAttachToOwner,wrong_flag,2,&out)&&out.as.u64==99);
    malformed=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=1};HSD_JObj* oldroot=o->hsd_obj;o->hsd_obj=NULL;before=lifecycle_calls;
    CHECK(!invoke(o,NativeScriptBinding_ItemAttachToOwner,&malformed,1,&out)&&out.as.u64==99&&before==lifecycle_calls);o->hsd_obj=oldroot;
    HSD_JObj* oldjoint=parts[1].joint;parts[1].joint=NULL;CHECK(!invoke(o,NativeScriptBinding_ItemAttachToOwner,&malformed,1,&out)&&out.as.u64==99&&before==lifecycle_calls);parts[1].joint=oldjoint;
    // Compare native pickup fields with direct Item_8026AB54 across pending
    // owner/normal owner and reset flag inputs; unrelated leaves are controlled.
    for(unsigned n=0;n<16;++n){HSD_GObj* scripted=item(lifecycle_creator),*reference=item(lifecycle_creator);Item* s=scripted->user_data,*r=reference->user_data;
        s->xDC8_word.flags.x0=r->xDC8_word.flags.x0=(n>>2)&1;s->xDD0_flag.b6=r->xDD0_flag.b6=(n>>3)&1;
        s->xD40=r->xD40=3.25f;s->xD50_landNum=r->xD50_landNum=37;s->xDD0_flag.b0=r->xDD0_flag.b0=1;s->xDAA_flag.b3=r->xDAA_flag.b3=1;
        NativeScriptValue raw={.type=NativeScriptType_I32,.as.i64=n&3};CHECK(invoke(scripted,NativeScriptBinding_ItemAttachToOwner,&raw,1,&out));Item_8026AB54(reference,lifecycle_creator,(Fighter_Part)(n&3));
        CHECK(s->owner==r->owner&&s->x51C==r->x51C&&s->xDC4==r->xDC4&&s->x20_team_id==r->x20_team_id&&s->xD50_landNum==r->xD50_landNum);
        CHECK(s->xDC8_word.flags.xF==r->xDC8_word.flags.xF&&s->xDC8_word.flags.x13==r->xDC8_word.flags.x13&&s->xDD0_flag.b6==r->xDD0_flag.b6&&s->xD40==r->xD40);
        CHECK(s->xDD0_flag.b0==r->xDD0_flag.b0&&s->xDAA_flag.b3==r->xDAA_flag.b3&&s->sfx_unk2==r->sfx_unk2&&s->pos.x==r->pos.x);
        CHECK(field(scripted,"pickups",NULL).as.i64==field(reference,"pickups",NULL).as.i64);++comparisons;HSD_GObjFree(scripted);HSD_GObjFree(reference);
    }
    // Relation snapshots restore revocation without inventing new provenance.
    size_t written;CHECK(nativeScriptItem_Save(saved,sizeof(saved),&written));CHECK(nativeScriptItem_ClearAttachmentOwner(o));CHECK(nativeScriptItem_Load(saved,written));CHECK(nativeScriptItem_AttachmentOwner(o)==lifecycle_creator&&nativeScriptItem_Creator(o)==lifecycle_creator);
    HSD_GObj* faulted=item(lifecycle_creator);for(unsigned a=0;a<NATIVE_SCRIPT_ITEM_ACTORS;++a)if(nativeScriptItemActors[a].object==faulted&&nativeScriptItemActors[a].active)nativeScriptItemActors[a].faulted=true;
    handle=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=((Item*)faulted->user_data)->x1C};out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!invoke(lifecycle_creator,NativeScriptBinding_ArticlesBindAuxiliary,&handle,1,&out)&&out.type==NativeScriptType_U64&&out.as.u64==99);
    handle.type=NativeScriptType_I32;handle.as.i64=((Item*)faulted->user_data)->x1C;field(lifecycle_creator,"handle",&handle);
    CHECK(nativeScriptFighter_Run(lifecycle_creator,NativeScriptFighter_Tick,NULL,0, NULL,&out)==NativeScriptVM_Error&&out.as.u64==99);CHECK(nativeScriptFighter_Status(lifecycle_creator)==NativeScriptVM_Error);
    CHECK(!invoke(lifecycle_creator,NativeScriptBinding_ArticlesBindAuxiliary,&handle,1,&out)&&out.as.u64==99);
    HSD_GObjFree(faulted);HSD_GObjFree(lifecycle_creator);lifecycle_creator=fighter(true);
    // Known callback boundary: ordinary clears are healthy; retirement/faults stop publication.
    for(unsigned mode=1;mode<=9;++mode){if(mode==8)continue;HSD_GObj* t=item(lifecycle_creator);lifecycle_mode=mode;lifecycle_reuse=mode==7?reuse:NULL;
        NativeScriptValue args[]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_Bool,.as.u64=1}};out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        bool ok=invoke(t,NativeScriptBinding_ItemAttachToOwnerAux,args,2,&out);
        if(mode==1){CHECK(ok&&out.type==NativeScriptType_Void&&((Fighter*)lifecycle_creator->user_data)->x1984_heldItemSpec==t&&!((Item*)t->user_data)->owner&&!nativeScriptItem_AttachmentOwner(t));HSD_GObjFree(t);}
        else {CHECK(!ok&&out.type==NativeScriptType_U64&&out.as.u64==99);if(mode==3){HSD_GObjFree(t);lifecycle_creator=fighter(true);}else if(mode==7){CHECK(replacement==t&&nativeScriptItem_Status(replacement)==NativeScriptVM_Ok);HSD_GObjFree(replacement);}else if(mode!=2)HSD_GObjFree(t);}
        lifecycle_mode=0;lifecycle_reuse=NULL;
    }
    // A reused Fighter pool slot/native spawn ID does not reactivate a saved relation.
    HSD_GObj* stale=item(lifecycle_creator);HSD_GObj* old=lifecycle_creator;uint32_t oldgen=((Fighter*)old->user_data)->x8_spawnNum;HSD_GObjFree(old);lifecycle_creator=fighter(true);CHECK(lifecycle_creator==old);((Fighter*)lifecycle_creator->user_data)->x8_spawnNum=oldgen;
    CHECK(nativeScriptItem_Creator(stale)==NULL&&nativeScriptItem_AttachmentOwner(stale)==NULL);CHECK(nativeScriptItem_Save(saved,sizeof(saved),&written));CHECK(nativeScriptItem_Load(saved,written));CHECK(nativeScriptItem_Creator(stale)==NULL);
    HSD_GObjFree(stale);HSD_GObjFree(o);HSD_GObjFree(other);HSD_GObjFree(wide);HSD_GObjFree(foreign);HSD_GObjFree(lifecycle_creator);
    CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS source Article lifecycle\",\"checks\":%u,\"native_comparisons\":%u,\"pickups\":%u,\"native_leaves\":%u}\n",checks,comparisons,lifecycle_pickups,lifecycle_calls);return 0;
}
