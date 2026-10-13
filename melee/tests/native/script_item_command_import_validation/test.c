#include <melee/it/itcommand_data.c>
#include <melee/it/itcommon_data.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/gr/grstage_data.h>
#include "stage_options.inc"
#include <melee/lb/lbcommand.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/object.h>
#include <stdio.h>

static unsigned checks, cases, corpus_articles, corpus_scripts, corpus_instructions;
static unsigned named_classes, instances, max_bones;
static unsigned stage_files, stage_scripts, stage_instructions, stages_needing_owner;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line%u %s\n",__LINE__,#x); exit(1); } } while (0)
static void put32(u8* p,u32 n) { p[0]=n>>24;p[1]=n>>16;p[2]=n>>8;p[3]=n; }
static Article article; static ItemModelDesc model;
static u8 arena[1024*1024];
static bool commands(const u32* words,u32 count,const u32* reloc,u32 reloc_count,
                     const Article* owner,u32 expected_error)
{
    ++cases;
    u8 file[8192]={0}; HSD_Dat dat;HSD_DatImport import;ItCommandData result;
    u32 data_size=count*4,size=32+data_size+reloc_count*4;
    CHECK(size<=sizeof(file));put32(file,size);put32(file+4,data_size);put32(file+8,reloc_count);
    for(u32 i=0;i<count;++i)put32(file+32+4*i,words[i]);
    for(u32 i=0;i<reloc_count;++i)put32(file+32+data_size+4*i,reloc[i]);
    CHECK(HSD_DatOpen(&dat,file,size)==HSD_DAT_OK);
    CHECK(HSD_DatImportInit(&import,&dat,arena,sizeof(arena),32)==HSD_DAT_OK);
    union CmdUnion* destination=(void*)(uintptr_t)0x1234;
    ItScriptEntry entry={&destination,0,owner};ItScriptCollect collect={&entry,1,1};
    bool okay=itCommandData_Import(&import,&collect,&result);
    if(expected_error!=UINT32_MAX) {
        if(expected_error==UINT32_MAX-1) {
            CHECK(!okay&&import.status==HSD_DAT_CAPACITY);
        } else if(okay||import.error_offset!=expected_error)fprintf(stderr,"case%u opcode%lu okay%d status%d actual%lu expected%lu\n",cases,words[0]>>26,okay,import.status,import.error_offset,expected_error);
        CHECK(!okay&&(expected_error==UINT32_MAX-1||import.error_offset==expected_error));
        CHECK(destination==(void*)(uintptr_t)0x1234&&!result.commands&&!result.word_count);
    } else {
        CHECK(okay&&destination==result.commands&&result.entry_count==1);
        // Actual common native command execution, not a second importer VM.
        CommandInfo state={.u=destination}; unsigned steps=0;
        while(state.u&&steps++<1000) {
            u32 opcode=state.u->Command_00.code;
            if(opcode>=10)break;
            if(opcode==3&&state.u->Command_03.value==0)break;
            CHECK(Command_Execute(&state,opcode));
            CHECK(state.loop_count<=COMMAND_STACK_SIZE);
        }
    }
    return okay;
}

static void scalar_cases(void)
{
    article.x10_modelDesc=&model;
    for(u32 op=11;op<=14;++op) {
        u32 good[7]={op<<26,0,0,0,0,0,0};
        good[0]|=op==14?3:3<<23; u32 count=op==11?7:2;
        CHECK(commands(good,count,NULL,0,&article,UINT32_MAX));
        good[0]=(op<<26)|(op==14?4:4<<23);
        CHECK(!commands(good,count,NULL,0,&article,0));
    }
    for(u32 op=15;op<=25;++op) {
        if(op==16)continue;
        u32 words[]={op<<26|0x3FFFFFF,0};
        CHECK(commands(words,2,NULL,0,NULL,UINT32_MAX));
    }
    u32 hit[]={11<<26,0,0,0,0,0,0};
    CHECK(commands(hit,7,NULL,0,NULL,UINT32_MAX));
    hit[0]|=1<<13;CHECK(!commands(hit,7,NULL,0,&article,0));
    CHECK(!commands(hit,7,NULL,0,NULL,0));
    model.x4_bone_count=1;hit[0]=(11<<26)|(99<<13);
    CHECK(commands(hit,7,NULL,0,&article,UINT32_MAX)); // unpopulated NULL slot
    hit[0]=(11<<26)|(100<<13);CHECK(!commands(hit,7,NULL,0,&article,0));
    u32 gfx[]={10<<26|99<<16,0,0,0,0,0};
    CHECK(commands(gfx,6,NULL,0,&article,UINT32_MAX));
    gfx[0]=10<<26|100<<16;CHECK(!commands(gfx,6,NULL,0,&article,0));
    model.x4_bone_count=0;gfx[0]=10<<26|1023<<16;
    CHECK(commands(gfx,6,NULL,0,&article,UINT32_MAX)); // optional child ordinal
    CHECK(!commands(gfx,6,NULL,0,NULL,0));
}

static void flow_cases(void)
{
    const u32 call_loop[]={5<<26,16,0,0,3<<26|2,1<<26|1,4<<26,6<<26};
    const u32 reloc[]={4};CHECK(commands(call_loop,8,reloc,1,NULL,UINT32_MAX));
    const u32 nested[]={3<<26|2,3<<26|2,4<<26,4<<26,0};
    CHECK(!commands(nested,5,NULL,0,NULL,4));
    const u32 loop1[]={3<<26|1,1<<26|1,4<<26,0};
    CHECK(commands(loop1,4,NULL,0,NULL,UINT32_MAX));
    const u32 loop0[]={3<<26,4<<26,0};
    CHECK(commands(loop0,3,NULL,0,NULL,UINT32_MAX));
    const u32 underflow[]={6<<26};CHECK(!commands(underflow,1,NULL,0,NULL,0));
    const u32 endloop[]={4<<26,0};CHECK(!commands(endloop,2,NULL,0,NULL,0));
    const u32 mismatch[]={3<<26|1,6<<26,0};CHECK(!commands(mismatch,3,NULL,0,NULL,4));
    const u32 recurse[]={5<<26,12,0,5<<26,12,6<<26}; const u32 rr[]={4,16};
    CHECK(!commands(recurse,6,rr,2,NULL,12));
    const u32 calls[]={5<<26,12,0,5<<26,24,6<<26,5<<26,36,6<<26,6<<26};
    const u32 rc[]={4,16,28};CHECK(commands(calls,10,rc,3,NULL,UINT32_MAX));
    const u32 jump[]={7<<26,8,7<<26,8}; const u32 rj[]={4,12};
    CHECK(commands(jump,4,rj,2,NULL,UINT32_MAX));
    const u32 nullcall[]={5<<26,0,0};CHECK(commands(nullcall,3,NULL,0,NULL,UINT32_MAX));
    // End may halt with a nonempty stack; this is not an underflow.
    const u32 halt[]={3<<26|2,0};CHECK(commands(halt,2,NULL,0,NULL,UINT32_MAX));
}

static void transaction_cases(void)
{
    ++cases;
    u8 file[64]={0};put32(file,64);put32(file+4,32);
    put32(file+32,11<<26|1<<13); // six-word hit-create, then End
    HSD_Dat dat;HSD_DatImport import;ItCommandData output;
    CHECK(HSD_DatOpen(&dat,file,sizeof(file))==HSD_DAT_OK);
    CHECK(HSD_DatImportInit(&import,&dat,arena,sizeof(arena),32)==HSD_DAT_OK);
    ItemModelDesc table={.x4_bone_count=1},untabled={0};
    Article indexed={.x10_modelDesc=&table},unindexed={.x10_modelDesc=&untabled};
    union CmdUnion* a=(void*)(uintptr_t)11,*b=(void*)(uintptr_t)12;
    ItScriptEntry entries[]={{&a,0,&indexed},{&b,0,&unindexed}};
    ItScriptCollect collect={entries,2,2};size_t before=import.arena_used;
    CHECK(!itCommandData_Import(&import,&collect,&output));
    CHECK(a==(void*)(uintptr_t)11&&b==(void*)(uintptr_t)12&&import.arena_used==before&&!output.commands);
    CHECK(import.error_offset==0); // same bytecode, different actual owners
    import.status=HSD_DAT_OK;entries[1].article=&indexed;
    CHECK(itCommandData_Import(&import,&collect,&output)&&a==b&&output.entry_count==2);

    // Finite, valid-depth fanout: many distinct return-address contexts exceed
    // the explicit analysis-state budget. It is rejected as capacity, never
    // mislabeled as an operand or stack overflow and never partially published.
    const u32 n=200,words_count=n*4+4;
    u32* words=calloc(words_count,sizeof(*words));u32* reloc=malloc(n*2*sizeof(*reloc));CHECK(words&&reloc);
    u32 function_a=(n*2+1)*4,function_b=(n*4+2)*4;
    for(u32 i=0;i<n;++i){words[2*i]=5<<26;words[2*i+1]=function_a;reloc[i]=4*(2*i+1);}
    for(u32 i=0;i<n;++i){u32 at=n*2+1+2*i;words[at]=5<<26;words[at+1]=function_b;reloc[n+i]=4*(at+1);}
    words[n*4+1]=6<<26;words[n*4+2]=1<<26|1;words[n*4+3]=6<<26;
    CHECK(!commands(words,words_count,reloc,n*2,NULL,UINT32_MAX-1));free(reloc);free(words);
}

static u32 actual_walk(HSD_JObj* root)
{
    u32 count=0;HSD_JObj* node=root;
    while(node&&count<=ITEM_BONE_CAPACITY) {
        ++count;
        if(HSD_JObjGetChild(node))node=HSD_JObjGetChild(node);
        else if(HSD_JObjGetNext(node))node=HSD_JObjGetNext(node);
        else while(true) {
            HSD_JObj* parent=HSD_JObjGetParent(node);
            if(!parent){node=NULL;break;}
            if(HSD_JObjGetNext(parent)){node=HSD_JObjGetNext(parent);break;}
            node=parent;
        }
    }
    return count;
}

static void compare_graph(HSD_Joint* root,bool accepted,u32 expected)
{
    ++cases; HSD_DatImport import={0};ItemModelDesc desc={.x0_joint=root,.x4_bone_count=1};
    u32 simulated=0;bool okay=nativeItemModel_Admit(&import,&desc,0,&simulated);
    HSD_IDSetup();HSD_JObj* native=HSD_JObjLoadJoint(root);CHECK(native);
    u32 actual=actual_walk(native);CHECK(okay==accepted&&actual==expected);
    if(okay)CHECK(simulated==actual);
    // Cyclic instance graphs cannot be recursively destroyed. They remain in
    // the private heap until process exit; bounded observation never renders.
    if(accepted)HSD_JObjRemoveAll(native);
}

static void model_cases(void)
{
    void* heap=malloc(16*1024*1024);CHECK(heap);
    void* start=OSInitAlloc(heap,(u8*)heap+16*1024*1024,1);
    OSHeapHandle handle=OSCreateHeap(start,(u8*)heap+16*1024*1024);CHECK(handle>=0);
    HSD_SetHeap(handle);OSSetCurrentHeap(handle);HSD_ObjSetHeap(16*1024*1024,NULL);HSD_ObjInit();
    HSD_Joint a[101]={0};for(u32 i=0;i<101;++i)a[i].scale=(Vec3){1,1,1};
    a[0].child=&a[1];a[1].next=&a[2];a[1].child=&a[3];a[0].next=&a[4];
    compare_graph(a,true,5);
    memset(a,0,sizeof(a));a[0].child=&a[1];a[0].next=&a[2];
    a[1].flags=JOBJ_INSTANCE;a[1].child=&a[2];compare_graph(a,true,3);
    // Same descriptor loaded twice; the later postorder insertion wins.
    a[1].next=&a[2];compare_graph(a,true,3);
    a[0].next=NULL;a[1].flags=0;a[1].child=NULL;
    a[2].flags=JOBJ_INSTANCE;a[2].child=&a[1]; // recursive instance walk >100
    compare_graph(a,false,101);
    memset(a,0,sizeof(a));for(u32 i=0;i<99;++i)a[i].child=&a[i+1];
    compare_graph(a,true,100);a[99].child=&a[100];compare_graph(a,false,101);
    HSD_DatImport import={0};ItemModelDesc desc={.x0_joint=a,.x4_bone_count=1};u32 n;
    a[0].class_name="unproved_custom_class";CHECK(!nativeItemModel_Admit(&import,&desc,123,&n)&&import.status==HSD_DAT_SYMBOL&&import.error_offset==123);
    a[0].class_name=NULL;a[0].child=a;CHECK(!nativeItemModel_Admit(&import,&desc,123,&n));
    desc.x0_joint=NULL;CHECK(nativeItemModel_Admit(&import,&desc,123,&n)&&n==1);
    HSD_Joint* many=calloc(NATIVE_ITEM_MODEL_NODE_LIMIT+1,sizeof(*many));CHECK(many);
    for(u32 i=0;i<NATIVE_ITEM_MODEL_NODE_LIMIT;++i)many[i].next=&many[i+1];
    desc.x0_joint=many;CHECK(!nativeItemModel_Admit(&import,&desc,123,&n)&&import.status==HSD_DAT_CAPACITY);free(many);
    // Missing instance target is rejected before native resolver's assertion.
    memset(a,0,sizeof(a));a[0].flags=JOBJ_INSTANCE;a[0].child=&a[1];
    desc.x0_joint=a;CHECK(!nativeItemModel_Admit(&import,&desc,123,&n));
}

static void report_models(const HSD_DatImport* import)
{
    for(u32 i=0;i<import->record_count;++i)if(import->records[i].type==DAT_JOINT) {
        const HSD_Joint* j=import->records[i].object;
        named_classes+=j->class_name!=NULL;instances+=(j->flags&JOBJ_INSTANCE)!=0;
    }
}

static void* read_file(const char* directory,const char* name,size_t* size)
{
    char path[1024];snprintf(path,sizeof(path),"%s/%s",directory,name);FILE* f=fopen(path,"rb");
    if(!f)return NULL;CHECK(fseek(f,0,SEEK_END)==0);long n=ftell(f);CHECK(n>=32&&fseek(f,0,SEEK_SET)==0);
    void* bytes=malloc(n);CHECK(bytes&&fread(bytes,1,n,f)==(size_t)n&&fclose(f)==0);*size=n;return bytes;
}

static void corpus(const char* directory)
{
    void* storage=malloc(IT_COMMON_IMPORT_ARENA_SIZE);CHECK(storage);unsigned files=0;
    for(unsigned kind=0;kind<Ft_Kind_StockCount;++kind) {
        const FtDataArchiveName* entry=&ftData_803C1F40[kind];size_t size;void* bytes=read_file(directory,entry->filename,&size);
        if(!bytes)continue;FtCharacterAssets* assets=calloc(1,sizeof(*assets));CHECK(assets);
        bool okay=ftCharacterAssets_Import(bytes,size,&ftCharacterStockLayouts[kind],entry->root_name,
            ftData_Table_Unk0[kind].count,ftData_UnkIntPairs[kind].count,CostumeListsForeachCharacter[kind].numCostumes,
            storage,FT_CHARACTER_IMPORT_ARENA_SIZE,FT_CHARACTER_IMPORT_RECORD_CAPACITY,assets);
        printf("{\"file\":\"%s\",\"accepted\":%s,\"status\":%u,\"offset\":%lu,\"articles\":%lu,\"scripts\":%lu,\"instructions\":%lu}\n",
            entry->filename,okay?"true":"false",assets->import.status,assets->import.error_offset,
            assets->article_stats.article_count,assets->article_stats.script_count,assets->item_command_data.instruction_count);
        CHECK(okay);++files;corpus_articles+=assets->article_stats.article_count;corpus_scripts+=assets->article_stats.script_count;
        corpus_instructions+=assets->item_command_data.instruction_count;report_models(&assets->import);
        for(unsigned i=0;i<FT_CHARACTER_ITEM_CAPACITY;++i)if(ftCharacterStockLayouts[kind].article_mask&(1u<<i)) {
            const Article* a=assets->fighter_items[i];if(a&&a->x10_modelDesc&&a->x10_modelDesc->x4_bone_count) {
                u32 count;CHECK(nativeItemModel_Admit(&assets->import,a->x10_modelDesc,0,&count));if(count>max_bones)max_bones=count;
            }
        }
        free(assets);free(bytes);
    }
    CHECK(files==Ft_Kind_StockCount);
    size_t size;void* bytes=read_file(directory,"ItCo.usd",&size);CHECK(bytes);
    ItCommonAssets* common=calloc(1,sizeof(*common));CHECK(common);
    bool okay=itCommonAssets_Import(bytes,size,storage,IT_COMMON_IMPORT_ARENA_SIZE,IT_COMMON_IMPORT_RECORD_CAPACITY,common);
    printf("{\"file\":\"ItCo.usd\",\"accepted\":%s,\"status\":%u,\"offset\":%lu,\"articles\":%lu,\"scripts\":%lu,\"instructions\":%lu}\n",
           okay?"true":"false",common->import.status,common->import.error_offset,common->article_stats.article_count,common->article_stats.script_count,common->command_data.instruction_count);
    CHECK(okay);corpus_articles+=common->article_stats.article_count;corpus_scripts+=common->article_stats.script_count;
    corpus_instructions+=common->command_data.instruction_count;report_models(&common->import);
    // Keep the genuine imported common owner alive while stage scripts are
    // converted. Root's actual loader selects this same Random Article.
    const Article* random=common->character_articles[It_PKind_Random-It_Kind_Kuriboh];CHECK(random);
    void* stage_arena=malloc(4*1024*1024);CHECK(stage_arena);
    for(u32 i=0;i<ARRAY_SIZE(native_stage_archives);++i) {
        const NativeStageArchiveSpec* spec=&native_stage_archives[i];
        char filename[64];snprintf(filename,sizeof(filename),"%s%s",spec->filename+1,strchr(spec->filename,'.')?"":".dat");
        size_t stage_size;void* stage_bytes=read_file(directory,filename,&stage_size);CHECK(stage_bytes);
        GrStageAssets* stage=calloc(1,sizeof(*stage));CHECK(stage);
        GrStageImportOptions options=spec->options;options.stage_item_script_article=random;
        bool accepted=grStageAssets_Import(stage_bytes,stage_size,stage_arena,4*1024*1024,32768,&options,stage);
        printf("{\"file\":\"%s\",\"scope\":\"actual_stage_import\",\"accepted\":%s,\"status\":%u,\"offset\":%lu,\"articles\":%lu,\"scripts\":%lu,\"instructions\":%lu}\n",
            filename,accepted?"true":"false",stage->import.status,stage->import.error_offset,stage->item_article_stats.article_count,
            stage->item_command_data.entry_count,stage->item_command_data.instruction_count);
        CHECK(accepted);++stage_files;stage_scripts+=stage->item_command_data.entry_count;stage_instructions+=stage->item_command_data.instruction_count;
        // Measure the actual need for external geometry context. This is an
        // import-only negative; no accepted stage or engine tables are changed.
        options.stage_item_script_article=NULL;
        bool absent=grStageAssets_Import(stage_bytes,stage_size,stage_arena,4*1024*1024,32768,&options,stage);
        if(!absent) {
            CHECK(stage->import.status==HSD_DAT_DESCRIPTOR);
            u32 raw;CHECK(HSD_DatReadU32(&stage->dat,stage->import.error_offset,&raw));
            CHECK((raw>>26)==10||((raw>>26)==11&&((raw>>13)&127)!=0));
            ++stages_needing_owner;
            printf("{\"file\":\"%s\",\"scope\":\"absent_random_owner_rejection\",\"status\":%u,\"offset\":%lu,\"opcode\":%lu}\n",
                filename,stage->import.status,stage->import.error_offset,raw>>26);
        }
        free(stage);free(stage_bytes);
    }
    CHECK(stage_files==ARRAY_SIZE(native_stage_archives));free(stage_arena);
    free(common);free(bytes);free(storage);
}

int main(int argc,char** argv)
{
    setvbuf(stdout,NULL,_IONBF,0);CHECK(argc==2);scalar_cases();flow_cases();transaction_cases();model_cases();corpus(argv[1]);
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"corpus_articles\":%u,\"corpus_scripts\":%u,\"corpus_instructions\":%u,\"named_joint_classes\":%u,\"instance_descriptors\":%u,\"max_fighter_indexed_walk\":%u,\"stage_files\":%u,\"stage_scripts\":%u,\"stage_instructions\":%u,\"stages_needing_owner\":%u}\n",
           checks,cases,corpus_articles,corpus_scripts,corpus_instructions,named_classes,instances,max_bones,stage_files,stage_scripts,stage_instructions,stages_needing_owner);
    return 0;
}
