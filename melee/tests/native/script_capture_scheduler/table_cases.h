// General constant tables: actual actor dispatch plus declaration/VM contracts.
typedef struct TableFixture {
    NativeScriptCompileBank bank;
    char text[16384];
    NativeScriptSourceNode nodes[256];
    NativeScriptIRNode expressions[256];uint32_t arguments[256];
    NativeScriptFunctionDeclaration functions[16];
    NativeScriptParameterDeclaration parameters[16];
    NativeScriptValueDeclaration values[16];NativeScriptLocal locals[16];
    NativeScriptInstruction instructions[256];NativeScriptFunctionCode code[16];
    NativeScriptExecutionFunction execution[16];NativeScriptCompileFunctionScratch graph[16];
    NativeScriptStorageField fields[16];NativeScriptStorageOwner owners[16];
    NativeScriptParameterSlot slots[16];
} TableFixture;
static TableFixture table_fixture;

static bool table_compile(const char* script)
{
    TableFixture* f=&table_fixture;
    f->bank=(NativeScriptCompileBank){.source_storage=f->text,.source_capacity=sizeof(f->text),
        .tree={.nodes=f->nodes,.capacity=256},.expressions={.nodes=f->expressions,.node_capacity=256,.arguments=f->arguments,.argument_capacity=256},
        .declarations={.functions=f->functions,.function_capacity=16,.parameters=f->parameters,.parameter_capacity=16,.values=f->values,.value_capacity=16},
        .locals=f->locals,.local_capacity=16,.instructions=f->instructions,.instruction_capacity=256,
        .function_code=f->code,.function_code_capacity=16,.execution_functions=f->execution,.execution_function_capacity=16,
        .graph=f->graph,.graph_capacity=16,.storage_layout={.fields=f->fields,.field_capacity=16,.owners=f->owners,.owner_capacity=16},
        .parameter_slots=f->slots,.parameter_slot_capacity=16};
    NativeScriptCompileRequest request={.source=script,.source_bytes=(uint32_t)strlen(script)};
    const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic diagnostic;
    bool ok=nativeScriptCompile_Build(&f->bank,&request,&accepted,&diagnostic);
    CHECK(ok?(accepted==&f->bank&&f->bank.valid):(!accepted&&!f->bank.valid));
    return ok;
}

static void table_cases(void)
{
    const float stock_recoil[]={0,-0.51f,-1.02f,-1.53f};
    for(int i=-1;i<=4;++i) {
        reset();Fighter_GObj* actor=create(true);
        NativeScriptValue index={.type=NativeScriptType_I32,.as.i64=i};
        presentation_state(actor,"items",&index);
        bool ok=helper(actor,"TableRead");
        CHECK(ok==(i>=0&&i<4));
        if(ok)CHECK(presentation_state(actor,"victim_frame",NULL).as.f32==stock_recoil[i]);
        else CHECK(nativeScriptFighter_Status(actor)==NativeScriptVM_Error);
        HSD_GObjFree(actor);++cases;
    }
    CHECK(table_compile("fighter:\n    behavior independent\n"
        "table int integers = [-2147483648, 2147483647, 2 * 3, 2147483648]\n"
        "table uint words = [4294967295, 0]\n"
        "table bool switches = [false, true, not false]\n"
        "table float constants = [-0.51, (2 + 3) / 2.0]\n"
        "func Lookup(int i) -> float:\n    return table.constants[i]\n"
        "func RootLookup(int i) -> int:\n    return program.table.integers[i]\n"));
    NativeScriptProgramIndex* p=&table_fixture.bank.declarations;
    CHECK(p->table_count==4&&p->table_element_count==11&&p->value_count==0);
    CHECK(table_fixture.bank.storage_layout.owners[0].state_bytes==0);
    const int64_t integer_values[]={INT32_MIN,INT32_MAX,6,INT32_MIN};
    for(unsigned table=0;table<4;++table)for(unsigned i=0;i<p->tables[table].count;++i) {
        NativeScriptValue selector={.type=NativeScriptType_I32,.as.i64=i},value;
        NativeScriptDiagnostic diagnostic;
        CHECK(nativeScriptProgram_ReadTable(p,0,NATIVE_SCRIPT_TABLE_SEMANTIC|table,&selector,1,&value,&diagnostic));
        if(table==0)CHECK(value.type==NativeScriptType_I32&&value.as.i64==integer_values[i]);
        if(table==1)CHECK(value.type==NativeScriptType_U32&&value.as.u64==(i?0:UINT32_MAX));
        if(table==2)CHECK(value.type==NativeScriptType_Bool&&value.as.u64==(i!=0));
        if(table==3)CHECK(value.type==NativeScriptType_F32&&value.as.f32==(i?2.5f:-0.51f));
        ++cases;
    }
    for(unsigned bad=0;bad<7;++bad) {
        NativeScriptValue selector={.type=NativeScriptType_I64,.as.i64=bad==0?-1:bad==1?4:0};
        if(bad==2){selector.type=NativeScriptType_U64;selector.as.u64=UINT64_MAX;}
        if(bad==3)selector.type=NativeScriptType_F32;
        NativeScriptValue output={.type=NativeScriptType_U64,.as.u64=99};NativeScriptDiagnostic d;
        CHECK(!nativeScriptProgram_ReadTable(p,0,NATIVE_SCRIPT_TABLE_SEMANTIC|(bad==4?4:0),
            bad==5?NULL:&selector,bad==6?0:1,&output,&d));
        CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++cases;
    }
    const char* invalid[]={
        "table float x = []\n", "table float x = [1,]\n", "table float x = [1,,2]\n",
        "table int x = [1 / 0]\n", "table float x = [1e38 * 1e38]\n",
        "table int x = [1.5]\n", "table bool x = [1]\n",
        "table float x = [1]\ntable float x = [2]\n",
        "table int x = [1]\nfunc Write():\n    table.x[0] = 2\n",
        "table int x = [1]\nfunc BadIndex() -> int:\n    return table.x[0.5]\n",
        "table int x = [1]\nfunc Nested() -> int:\n    return table.x[0][0]\n",
        "table int x = [state.missing]\n", "table int x = [random.int(2)]\n",
        "fighter:\n    table int x = [1]\n    behavior independent\n"
    };
    char source[16384];
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i) {
        snprintf(source,sizeof(source),"fighter:\n    behavior independent\n%s",invalid[i]);
        bool ok=table_compile(source);
        if(ok)fprintf(stderr,"unexpected table admission case %u: %s\n",i,invalid[i]);
        CHECK(!ok);++cases;
    }
    // Constant tables in entities retain exact scope; no engine/DAT binding is
    // involved in the data index. Source schema admission is tested separately.
    const char* scoped="table int shared = [9]\narticle Toy states 1:\n    table float shared = [0.5]\n"
        "    func Pick() -> float:\n        return table.shared[0]\n"
        "    func RootPick() -> int:\n        return program.table.shared[0]\n"
        "article Other states 1:\n    table int unrelated = [3]\n"
        "    func SharedPick() -> int:\n        return table.shared[0]\n";
    NativeScriptDiagnostic d;
    p->tree->source=scoped;p->tree->source_bytes=(uint32_t)strlen(scoped);
    p->expressions->source=scoped;p->expressions->source_bytes=(uint32_t)strlen(scoped);
    CHECK(nativeScriptSource_Parse(p->tree,&d));CHECK(nativeScriptProgram_Index(p,&d));
    CHECK(p->table_count==3);
    CHECK(p->function_count==3);
    NativeScriptProgramCompiler compiler={.index=p,.locals=table_fixture.locals,.local_capacity=16};
    for(unsigned function=0;function<3;++function) {
        p->expressions->node_count=p->expressions->argument_count=0;
        NativeScriptStatementCompiler body;
        CHECK(nativeScriptProgram_CompileBody(&compiler,function,&body,&d));
        unsigned reads=0;
        for(uint32_t n=0;n<p->expressions->node_count;++n)if(p->expressions->nodes[n].kind==NativeScriptIR_Index) {
            CHECK(p->expressions->nodes[n].semantic==(NATIVE_SCRIPT_TABLE_SEMANTIC|(function?0:1)));
            CHECK(!(p->expressions->nodes[n].flags&NativeScriptFlag_Writable));++reads;
        }
        CHECK(reads==1);++cases;
    }
    NativeScriptValue selector={.type=NativeScriptType_I32},output;
    CHECK(nativeScriptProgram_ReadTable(p,p->tables[1].owner,NATIVE_SCRIPT_TABLE_SEMANTIC|1,&selector,1,&output,&d));
    CHECK(output.type==NativeScriptType_F32&&output.as.f32==0.5f);
    CHECK(!nativeScriptProgram_ReadTable(p,p->tables[2].owner,NATIVE_SCRIPT_TABLE_SEMANTIC|1,&selector,1,&output,&d));
    CHECK(nativeScriptProgram_ReadTable(p,p->tables[1].owner,NATIVE_SCRIPT_TABLE_SEMANTIC,&selector,1,&output,&d));
    CHECK(output.as.i64==9);++cases;
    size_t at=(size_t)snprintf(source,sizeof(source),"fighter:\n    behavior independent\n");
    for(unsigned i=0;i<NATIVE_SCRIPT_TABLES;++i)at+=(size_t)snprintf(source+at,sizeof(source)-at,"table int t%u = [0]\n",i);
    CHECK(table_compile(source));CHECK(p->table_count==NATIVE_SCRIPT_TABLES);++cases;
    snprintf(source+at,sizeof(source)-at,"table int overflow = [0]\n");
    CHECK(!table_compile(source));CHECK(!p->table_count&&!p->table_element_count);++cases;
    at=(size_t)snprintf(source,sizeof(source),"fighter:\n    behavior independent\ntable int huge = [");
    for(unsigned i=0;i<NATIVE_SCRIPT_TABLE_ELEMENTS;++i)at+=(size_t)snprintf(source+at,sizeof(source)-at,i?",0":"0");
    snprintf(source+at,sizeof(source)-at,"]\n");CHECK(table_compile(source));
    CHECK(p->table_element_count==NATIVE_SCRIPT_TABLE_ELEMENTS);++cases;
    at+=(size_t)snprintf(source+at,sizeof(source)-at,",0");
    snprintf(source+at,sizeof(source)-at,"]\n");CHECK(!table_compile(source));
    CHECK(!p->table_count&&!p->table_element_count);++cases;
}
