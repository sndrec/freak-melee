NativeSourcePackage target_foreign_package;
static void prepare_foreign(void)
{
 static char source[1024];static NativeScriptSourceNode nodes[64];static NativeScriptIRNode expressions[64];static uint32_t arguments[64];
 static NativeScriptFunctionDeclaration functions[4];static NativeScriptParameterDeclaration parameters[4];static NativeScriptValueDeclaration values[4];
 static NativeScriptLocal locals[4];static NativeScriptInstruction instructions[64];static NativeScriptFunctionCode codes[4];static NativeScriptExecutionFunction entries[4];static NativeScriptCompileFunctionScratch graph[4];
 static NativeScriptStorageField fields[4];static NativeScriptStorageOwner owners[4];static NativeScriptParameterSlot slots[4];
 NativeSourcePackage* p=&target_foreign_package;
 p->compiler.bank=(NativeScriptCompileBank){.source_storage=source,.source_capacity=sizeof(source),.tree={.nodes=nodes,.capacity=64},.expressions={.nodes=expressions,.node_capacity=64,.arguments=arguments,.argument_capacity=64},
  .declarations={.functions=functions,.function_capacity=4,.parameters=parameters,.parameter_capacity=4,.values=values,.value_capacity=4},.locals=locals,.local_capacity=4,
  .instructions=instructions,.instruction_capacity=64,.function_code=codes,.function_code_capacity=4,.execution_functions=entries,.execution_function_capacity=4,.graph=graph,.graph_capacity=4,
  .storage_layout={.fields=fields,.field_capacity=4,.owners=owners,.owner_capacity=4},.parameter_slots=slots,.parameter_slot_capacity=4};
 const char* text="fighter:\n    behavior independent\nstate int unrelated\naction Foreign:\n    on tick:\n        pass\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&p->compiler.bank,&request,&accepted,&diagnostic));
 CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst+1,accepted,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->target=Ft_Kind_CustomFirst+1;p->ready=p->assets.ready=true;p->assets.facts.moves=layout;p->assets.parts.parts_num=4;p->assets.facts.joints=4;
 tables[p->target]=&p->assets.parts;gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=p->assets.motions;
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);
}
