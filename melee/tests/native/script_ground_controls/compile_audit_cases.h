// Audit recovery uses the production compiler, never a permissive resolver.
static void compile_audit_cases(void)
{
 static const char broken[]=
  "fighter:\n    behavior independent\n"
  "func First():\n    missing_first()\n"
  "func Good() -> int:\n    return 7\n"
  "func Second():\n    let int value = 1.5\n"
  "func Third():\n    targets.call(1, \"unused_export\")\n    missing_third()\n"
  "func Last():\n    pass\n";
 static const char metadata_errors[]=
  "fighter:\n    behavior independent\n    unknown_one yes\n    unknown_two yes\n"
  "func Good():\n    pass\n";
 NativeScriptDiagnostic entries[4],diagnostic;
 NativeScriptCompileDiagnostics report={.entries=entries,.capacity=4};
 NativeScriptCompileRequest request={.source=broken,.source_bytes=sizeof(broken)-1,.diagnostics=&report};
 const NativeScriptCompileBank* accepted=NULL;
 initialize_bank();
 CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 CHECK(!accepted&&!bank.valid&&report.count==3&&report.total==3);
 CHECK(report.metadata_complete&&report.bodies_complete&&report.functions_checked==5);
 CHECK(entries[0].source.line==4&&entries[1].source.line==8&&entries[2].source.line==11);
 CHECK(!memcmp(&diagnostic,&entries[0],sizeof(diagnostic)));
 CHECK(bank.declarations.export_name_count==0);
 // Failed bodies cannot leave IR/argument/code that crowds out later bodies.
 CHECK(bank.function_code[1].instruction_count&&bank.function_code[4].instruction_count);
 CHECK(!bank.function_code[0].instruction_count&&!bank.function_code[2].instruction_count&&!bank.function_code[3].instruction_count);
 ++cases;
 memset(entries,0x5a,sizeof(entries));NativeScriptDiagnostic sentinel=entries[1];
 report.capacity=1;
 CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 CHECK(!accepted&&!bank.valid&&report.count==1&&report.total==3&&report.functions_checked==5);
 CHECK(!memcmp(&entries[1],&sentinel,sizeof(sentinel)));++cases;
 request.diagnostics=NULL;
 CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 CHECK(!accepted&&!bank.valid&&diagnostic.source.line==4);++cases;
 report.capacity=4;request=(NativeScriptCompileRequest){.source=metadata_errors,.source_bytes=sizeof(metadata_errors)-1,.diagnostics=&report};
 CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 CHECK(!accepted&&!bank.valid&&report.count==2&&report.total==2&&report.metadata_complete&&!report.bodies_complete&&!report.functions_checked);
 CHECK(entries[0].source.line==3&&entries[1].source.line==4);++cases;
 // Structural failure is incomplete, not a claim that every body was checked.
 request.source="func bad\n";request.source_bytes=(uint32_t)strlen(request.source);
 CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 CHECK(!accepted&&!report.metadata_complete&&!report.bodies_complete&&!report.count&&diagnostic.message[0]);++cases;
 request.source="fighter:\n    behavior independent\nfunc Good() -> int:\n    return 7\n";
 request.source_bytes=(uint32_t)strlen(request.source);
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 CHECK(accepted==&bank&&bank.valid&&!report.total&&!report.count&&report.functions_checked==1&&report.metadata_complete&&report.bodies_complete);++cases;
}
