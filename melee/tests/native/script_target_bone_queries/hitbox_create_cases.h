#include <melee/ft/native_script_binding_hitbox_create.h>
#include <melee/ft/ftcoll.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/it/native_article_resource_internal.h>
void hitbox_native_reference(Fighter_GObj*,itSamusGrapple_HitboxData*);
extern const itSamusGrapple_Hitbox it_803B8660;
static unsigned hitbox_create_comparisons;
static void resource_fixture_remove(void* payload) { (void)payload; }

static void hitbox_compile_negatives(void)
{
    NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
    const char* bodies[]={"hitbox.create(0, 0)","hitbox.create(0.5, Capsule)","hitbox.create(0, int(Capsule))",
        "hitbox.create(0, Capsule + 1)","hitbox.create(0)","hitbox.create(0, Capsule, 1)",
        "let int value = Capsule"};
    char source[1024];
    for(unsigned i=0;i<sizeof(bodies)/sizeof(*bodies);++i){
        snprintf(source,sizeof(source),"fighter:\n    behavior independent\nhitbox_definition Capsule:\n    bone 2\n    damage 0\n    radius 3\nfunc Bad():\n    %s\n",bodies[i]);
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source)};const NativeScriptCompileBank* accepted=NULL;
        bool rejected=!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&!accepted;
        if(!rejected)fprintf(stderr,"unexpected accepted hitbox call: %s\n",bodies[i]);
        CHECK(rejected);++negative_count;
    }
    const char* fields[]={"bone 140","bone -1","bone 1.5","damage -1","damage 4294967297",
        "radius -1","radius 1e100","angle -1","element 32","sound_severity 8","sound_kind 32",
        "unknown_field 1","bone 2\n    bone 2","damage 0\n    damage 0","radius 3\n    radius 3"};
    for(unsigned i=0;i<sizeof(fields)/sizeof(*fields);++i){
        bool single=strchr(fields[i],'\n')==NULL;
        const char* bone=single&&!strncmp(fields[i],"bone ",5)?"":"    bone 2\n";
        const char* damage=single&&!strncmp(fields[i],"damage ",7)?"":"    damage 0\n";
        const char* radius=single&&!strncmp(fields[i],"radius ",7)?"":"    radius 3\n";
        snprintf(source,sizeof(source),"fighter:\n    behavior independent\nhitbox_definition Capsule:\n%s%s%s    %s\n",bone,damage,radius,fields[i]);
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source)};const NativeScriptCompileBank* accepted=NULL;
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&!accepted);++negative_count;
    }
    const char* malformed[]={
        "hitbox_definition Capsule:\n    damage 0\n    radius 3\n",
        "hitbox_definition Capsule:\n    bone 2\n    damage 0\n",
        "hitbox_definition Capsule:\n    bone 2\n    radius 3\n",
        "hitbox_definition Capsule:\n    bone 2\n    damage 0\n    radius 3\nhitbox_definition Capsule:\n    bone 1\n    damage 1\n    radius 1\n",
        "hitbox_definition Capsule:\n    bone 2\n    damage 0\n    radius 3\nfunc Capsule():\n    pass\n",
        "hitbox_definition hitbox:\n    bone 2\n    damage 0\n    radius 3\n",
        "hitbox_definition Capsule\n",
        "action Something:\n    hitbox_definition Capsule:\n        bone 2\n        damage 0\n        radius 3\n"};
    for(unsigned i=0;i<sizeof(malformed)/sizeof(*malformed);++i){
        snprintf(source,sizeof(source),"fighter:\n    behavior independent\n%s",malformed[i]);
        NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source)};const NativeScriptCompileBank* accepted=NULL;
        CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&!accepted);++negative_count;
    }
    nativeSourceBank_Release(&scratch);
}

static NativeScriptValue create_vm(HSD_GObj* object,bool endpoint)
{
    const NativeScriptCompileBank* bank=nativeScriptFighter_Program(((Fighter*)object->user_data)->kind)->bank;
    uint32_t function=0;
    for(;function<bank->declarations.function_count;++function)
        if(nativeScriptSource_SpanEquals(&bank->tree,bank->declarations.functions[function].name,endpoint?"CreateEndpoint":"CreateOwned"))break;
    CHECK(function<bank->declarations.function_count);NativeScriptValue result;
    CHECK(nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function,NULL,0,&result,&diagnostic)==NativeScriptVM_Ok);
    ++vm_calls;return result;
}
static void create_compare(HSD_GObj* object,bool endpoint)
{
    static Fighter before,expected;Fighter* f=object->user_data;f->x34_scale.y=1;
    for(unsigned mode=0;mode<8;++mode)for(unsigned slot=0;slot<4;++slot)for(unsigned vm=0;vm<2;++vm){
        for(unsigned i=0;i<4;++i){memset(&f->x914[i],0x65,sizeof(HitCapsule));
            f->x914[i].state=i==slot?(HitCapsuleState)(mode%4):HitCapsule_Unk3;
            f->x914[i].x4=mode&4?0:7;}
        f->x2219_b3=false;before=expected=*f;
        HSD_GObj reference={.user_data=&expected};itSamusGrapple_HitboxData descriptor={.create_hitbox=it_803B8660};
        descriptor.create_hitbox.create_hitbox_0.id=slot;
        descriptor.create_hitbox.create_hitbox_0.bone=endpoint?139:2;
        descriptor.create_hitbox.create_hitbox_0.damage=endpoint?0:12;
        hitbox_native_reference(&reference,&descriptor);
        NativeScriptValue args[]={integer(slot),{.type=NativeScriptType_HitboxDefinition,.as.u64=endpoint?1:0}},result;
        if(vm){field(object,"part",args);result=create_vm(object,endpoint);}
        else {NativeScriptRuntimeContext c=context(object);CHECK(nativeScriptBindingHitboxCreate_Invoke(&c,args,2,&result,&diagnostic));}
        CHECK(result.type==NativeScriptType_Bool&&result.as.u64);
        CHECK(!memcmp(f->x914,expected.x914,sizeof(f->x914))&&f->x2219_b3==expected.x2219_b3);
        CHECK(f->parts==before.parts);++hitbox_create_comparisons;++comparisons;++cases;
    }
}
static void hitbox_create_cases(HSD_GObj* object,HSD_GObj* owner)
{
    hitbox_compile_negatives();create_compare(object,false);
    Fighter* f=object->user_data;NativeScriptRuntimeContext c=context(object);
    NativeScriptValue args[]={integer(0),{.type=NativeScriptType_HitboxDefinition,.as.u64=1}},result;
    HitCapsule before[4];memcpy(before,f->x914,sizeof(before));
    CHECK(nativeScriptBindingHitboxCreate_Invoke(&c,args,2,&result,&diagnostic)&&!result.as.u64);
    CHECK(!memcmp(before,f->x914,sizeof(before)));
    const NativeScriptValue invalid[]={integer(-1),integer(4),floating(0),{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)},
        {.type=NativeScriptType_Bool,.as.u64=2}};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){args[0]=invalid[i];result=integer(99);
        CHECK(!nativeScriptBindingHitboxCreate_Invoke(&c,args,2,&result,&diagnostic));
        CHECK(result.type==NativeScriptType_I32&&result.as.i64==99&&!memcmp(before,f->x914,sizeof(before)));++negative_count;}
    args[0]=integer(0);args[1]=integer(1);result=integer(99);
    CHECK(!nativeScriptBindingHitboxCreate_Invoke(&c,args,2,&result,&diagnostic)&&result.as.i64==99);++negative_count;

    // Construct the graph at a fixture transition; retained owner/attachment,
    // node/GObj/payload/graph captures and borrowing use the actual engine code.
    static NativeMatchPool payload_pool,graph_pool;
    static ItemLink links[2];static HSD_JObj graphs[2];static HSD_Joint descriptor;
    static NativeMatchPoolSlot payload_slots[2],graph_slots[2];static uint32_t payload_order[2],graph_order[2];
    CHECK(nativeMatchPool_Bind(&payload_pool,links,sizeof(links),payload_slots,payload_order,2,sizeof(ItemLink),_Alignof(ItemLink),nativeGObjPool.lifetime));
    CHECK(nativeMatchPool_Bind(&graph_pool,graphs,sizeof(graphs),graph_slots,graph_order,2,sizeof(HSD_JObj),_Alignof(HSD_JObj),nativeGObjPool.lifetime));
    CHECK(nativeScriptItem_SetAttachmentOwner(owner,object));
    ResourceCollection* set=&nativeArticleResourceCollections[0];
    CHECK(nativeItemLifetime_Capture(owner,&set->owner));CHECK(nativeFighterLifetime_Capture(f,&set->attachment));
    set->parts=f->parts;set->parts_table=ftPartsTable[f->kind];set->rig=object->hsd_obj;set->rig_kind=object->obj_kind;
    set->generation=1;set->count=set->created=2;set->part_count=4;set->external_part=139;
    HSD_GObj* nodes[2];
    for(unsigned i=0;i<2;++i){ResourceNode* node=&set->nodes[i];
        node->link=nativeMatchPool_Allocate(&payload_pool,&node->payload);node->model=nativeMatchPool_Allocate(&graph_pool,&node->graph);
        CHECK(node->link&&node->model);nodes[i]=GObj_Create(HSD_GOBJ_CLASS_ITEMLINK,10,0);CHECK(nodes[i]);
        GObj_InitUserData(nodes[i],6,resource_fixture_remove,node->link);nodes[i]->hsd_obj=node->model;nodes[i]->obj_kind=HSD_GObj_JObjKind;
        node->link->gobj=nodes[i];node->descriptor=&descriptor;node->generation=1;
        CHECK(nativeMatchPool_Reference(&nativeGObjPool,nodes[i],&node->object));}
    CHECK(nativeArticleResource_BindExternalPart(owner,1,139));
    HSD_JObj* resolved=NULL;CHECK(nativeArticleResource_ResolveBoundPart(object,139,&resolved)&&resolved==set->nodes[1].model);
    create_compare(object,true);
    // Replaced slot, author generation, graph generation and revoked binding
    // must not let a stale capsule retain an endpoint from another lifetime.
    for(unsigned mode=0;mode<4;++mode){ResourceNode saved=set->nodes[1];HSD_JObj* joint=f->parts[139].joint;uint32_t generation=set->owner.generation;
        if(mode==0)f->parts[139].joint=&replacement_joint;
        if(mode==1)++set->owner.generation;
        if(mode==2)++set->nodes[1].graph.generation;
        if(mode==3)set->nodes[1].bound_part=false;
        resolved=&replacement_joint;CHECK(!nativeArticleResource_ResolveBoundPart(object,139,&resolved)&&resolved==&replacement_joint);
        memcpy(before,f->x914,sizeof(before));args[1]=(NativeScriptValue){.type=NativeScriptType_HitboxDefinition,.as.u64=1};
        CHECK(nativeScriptBindingHitboxCreate_Invoke(&c,args,2,&result,&diagnostic)&&!result.as.u64&&!memcmp(before,f->x914,sizeof(before)));
        set->nodes[1]=saved;set->owner.generation=generation;f->parts[139].joint=joint;++negative_count;
    }
    f->x1064_thrownHitbox.jobj=set->nodes[1].model;f->x1064_thrownHitbox.state=HitCapsule_Enabled;
    nativeArticleResource_DetachBorrowers(set,&set->nodes[1]);
    CHECK(!f->parts[139].joint&&!f->x914[3].jobj&&f->x914[3].state==HitCapsule_Disabled);
    CHECK(!f->x1064_thrownHitbox.jobj&&f->x1064_thrownHitbox.state==HitCapsule_Disabled);
    for(unsigned i=0;i<2;++i){HSD_GObjFree(nodes[i]);CHECK(nativeMatchPool_Free(&graph_pool,set->nodes[i].graph));CHECK(nativeMatchPool_Free(&payload_pool,set->nodes[i].payload));}
    *set=(ResourceCollection){0};CHECK(nativeScriptItem_ClearAttachmentOwner(owner));
}
