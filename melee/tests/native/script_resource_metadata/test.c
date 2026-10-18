#include <melee/ft/native_script_resource_metadata.h>
#include <melee/ft/native_source_asset_facts.h>
#include <melee/ft/native_fighter_rig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NativeScriptSourceNode nodes[256];
static NativeScriptSourceTree tree;
static NativeSourceAssetFacts facts;
static NativeScriptDiagnostic diagnostic;
static unsigned checks,cases;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"resources %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void parse(const char* source)
{
    tree=(NativeScriptSourceTree){.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=nodes,.capacity=256};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
}
static uint32_t owner(const char* name)
{
    for(uint32_t i=1;i<tree.count;++i)
        if((nodes[i].kind==NativeScriptSource_Article||nodes[i].kind==NativeScriptSource_Item)&&nativeScriptSource_SpanEquals(&tree,nodes[i].name,name))return i;
    CHECK(false);return 0;
}
static uint32_t resource(void)
{
    for(uint32_t i=1;i<tree.count;++i)if(nodes[i].kind==NativeScriptSource_Metadata&&nativeScriptSource_SpanEquals(&tree,nodes[i].name,"resources"))return i;
    CHECK(false);return 0;
}
static void recipe(const char* line,bool accepted,uint16_t capacity,int16_t part,const NativeSourceAssetFacts* rig)
{
    char source[1024];snprintf(source,sizeof(source),"article Probe states 1:\n    %s\n    state 0 anim -1:\n",line);parse(source);
    NativeScriptResourceRecipe sentinel={UINT16_MAX,INT16_MIN},out=sentinel;
    bool result=nativeScriptResourceMetadata_Prepare(&tree,owner("Probe"),rig,NULL,&out,&diagnostic);
    CHECK(result==accepted);
    if(accepted)CHECK(out.capacity==capacity&&out.reserved_part==part);
    else CHECK(!memcmp(&out,&sentinel,sizeof(out))&&diagnostic.message[0]);
    ++cases;
}
int main(void)
{
    _Static_assert(NATIVE_SCRIPT_RESOURCE_CAPACITY_MAX==NATIVE_ARTICLE_RESOURCE_NODES,"shared native node limit");
    facts.joints=139;
    parse("article Probe states 1:\n    state 0 anim -1:\n");NativeScriptResourceRecipe out={90,139};
    CHECK(nativeScriptResourceMetadata_Prepare(&tree,owner("Probe"),NULL,NULL,&out,&diagnostic));CHECK(out.capacity==0&&out.reserved_part==-1);++cases;
    for(unsigned capacity=2;capacity<=90;++capacity){char line[80];snprintf(line,sizeof(line),"resources capacity %u",capacity);recipe(line,true,(uint16_t)capacity,-1,NULL);}
    recipe("resources\tcapacity\t0x5a\treserve_part\t0x8b",true,90,139,&facts);
    for(unsigned joints=1;joints<=140;++joints){facts.joints=(uint16_t)joints;recipe("resources capacity 30 reserve_part 139",joints<=139,30,139,&facts);}
    facts.joints=61;recipe("resources capacity 30 reserve_part 61",true,30,61,&facts);
    recipe("resources capacity 30 reserve_part 60",false,0,0,&facts);
    recipe("resources capacity 30 reserve_part 140",false,0,0,&facts);
    recipe("resources capacity 30 reserve_part 61",false,0,0,NULL);
    facts.joints=0;recipe("resources capacity 30 reserve_part 61",false,0,0,&facts);
    facts.joints=141;recipe("resources capacity 30 reserve_part 139",false,0,0,&facts);facts.joints=61;
    facts.joints=75;facts.omission_count=1;
    recipe("resources capacity 30 reserve_part 75",false,0,0,&facts);
    recipe("resources capacity 30 reserve_part 76",true,30,76,&facts);
    for(unsigned omitted=0;omitted<=NATIVE_SOURCE_OMISSIONS;++omitted){
        facts.joints=107;facts.omission_count=(uint16_t)omitted;
        recipe("resources capacity 30 reserve_part 139",true,30,139,&facts);
    }
    facts.joints=139;facts.omission_count=1;
    recipe("resources capacity 30 reserve_part 139",false,0,0,&facts); // Final count 140 has no extra slot.
    facts.joints=138;
    recipe("resources capacity 30 reserve_part 139",true,30,139,&facts);
    facts.joints=139;facts.omission_count=2;
    recipe("resources capacity 30 reserve_part 139",false,0,0,&facts); // Sum above physical capacity.
    facts.joints=1;facts.omission_count=NATIVE_SOURCE_OMISSIONS+1;
    recipe("resources capacity 30 reserve_part 139",false,0,0,&facts);
    facts.omission_count=UINT16_MAX;
    recipe("resources capacity 30 reserve_part 139",false,0,0,&facts);
    facts.joints=UINT16_MAX;
    recipe("resources capacity 30 reserve_part 139",false,0,0,&facts); // Never narrow the sum to uint16_t.
    recipe("resources capacity 30",true,30,-1,&facts); // No part reservation needs rig facts.
    facts.joints=61;facts.omission_count=0;
    const char* invalid[]={"resources", "resources capacity", "resources capacity 0", "resources capacity 1", "resources capacity 91",
        "resources capacity 4294967295", "resources capacity 4294967296", "resources capacity 18446744073709551616",
        "resources capacity -2", "resources capacity +2", "resources capacity 2.0", "resources capacity 0x", "resources capacity 0x100000000",
        "resources capacity 2x", "resources capacity 2 capacity 3", "resources capacity 2 reserve_part", "resources capacity 2 reserve_part -1",
        "resources capacity 2 reserve_part 4294967296", "resources capacity 2 reserve_part 61 reserve_part 62", "resources capacity 2 trailing",
        "resources reserve_part 61 capacity 2", "resources capacity 2 reserve_part 61 capacity 3"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i)recipe(invalid[i],false,0,0,&facts);
    parse("article First states 1:\n    resources capacity 2\n    state 0 anim -1:\nitem Second damage 1 angle 2 lifetime 3:\n    resources capacity 90 reserve_part 139\n");
    CHECK(nativeScriptResourceMetadata_Prepare(&tree,owner("First"),NULL,NULL,&out,&diagnostic));CHECK(out.capacity==2&&out.reserved_part==-1);
    CHECK(nativeScriptResourceMetadata_Prepare(&tree,owner("Second"),&facts,NULL,&out,&diagnostic));CHECK(out.capacity==90&&out.reserved_part==139);++cases;
    parse("article Probe states 1:\n    resources capacity 2\n    resources capacity 3\n    state 0 anim -1:\n");
    NativeScriptResourceRecipe sentinel={UINT16_MAX,INT16_MIN};out=sentinel;
    CHECK(!nativeScriptResourceMetadata_Prepare(&tree,owner("Probe"),&facts,NULL,&out,&diagnostic));CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    parse("article Probe states 1:\n    resources capacity 2\n    state 0 anim -1:\n");uint32_t entity=owner("Probe"),field=resource();
    nodes[field].opens=true;out=sentinel;CHECK(!nativeScriptResourceMetadata_Prepare(&tree,entity,&facts,NULL,&out,&diagnostic));CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    nodes[field].opens=false;nodes[field].first_child=field;CHECK(!nativeScriptResourceMetadata_Prepare(&tree,entity,&facts,NULL,&out,&diagnostic));CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    nodes[field].first_child=UINT32_MAX;nodes[field].parent=tree.count-1;CHECK(!nativeScriptResourceMetadata_Prepare(&tree,entity,&facts,NULL,&out,&diagnostic));CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    nodes[field].parent=entity;nodes[field].value.offset=UINT32_MAX;CHECK(!nativeScriptResourceMetadata_Prepare(&tree,entity,&facts,NULL,&out,&diagnostic));CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    CHECK(!nativeScriptResourceMetadata_Prepare(&tree,0,&facts,NULL,&out,&diagnostic));CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    parse("article Probe states 1:\n    resources capacity 2 reserve_part 61\n    state 0 anim -1:\n");
    CHECK(nativeScriptResourceMetadata_Prepare(&tree,owner("Probe"),&facts,NULL,&out,&diagnostic));facts.joints=62;out=sentinel;
    CHECK(!nativeScriptResourceMetadata_Prepare(&tree,owner("Probe"),&facts,NULL,&out,&diagnostic));CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    facts.joints=61;facts.omission_count=1;out=sentinel;
    CHECK(!nativeScriptResourceMetadata_Prepare(&tree,owner("Probe"),&facts,NULL,&out,&diagnostic));CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    // A script keeps its reserved bone ID when a structural edit moves the
    // physical slot. Lower that ID before checking the live rig boundary.
    NativeFighterRig mapped={.count=61,.next_id=500001};
    for(unsigned i=0;i<MAX_FT_PARTS;++i)mapped.ids[i]=i;
    mapped.ids[139]=500000;
    CHECK(nativeFighterRig_Index(&mapped,NULL,0));facts.omission_count=0;
    parse("article Probe states 1:\n    resources capacity 2 reserve_part 500000\n    state 0 anim -1:\n");
    CHECK(nativeScriptResourceMetadata_Prepare(&tree,owner("Probe"),&facts,&mapped,&out,&diagnostic));
    CHECK(out.capacity==2&&out.reserved_part==139);++cases;
    parse("article Probe states 1:\n    resources capacity 2 reserve_part 139\n    state 0 anim -1:\n");out=sentinel;
    CHECK(!nativeScriptResourceMetadata_Prepare(&tree,owner("Probe"),&facts,&mapped,&out,&diagnostic));
    CHECK(!memcmp(&out,&sentinel,sizeof(out)));++cases;
    printf("{\"status\":\"PASS resource metadata\",\"checks\":%u,\"cases\":%u}\n",checks,cases);return 0;
}
