"""Every renamed common CPU operation callsite, excluding explicitly removed forwarding definitions."""
import re
ALIASES={
 'ftCo_800A1F3C':'ftCo_CpuSetDestination','ftCo_800A1F3C_noinline':'ftCo_CpuSetDestination','ftCo_800A1F3C_noinline2':'ftCo_CpuSetDestination',
 'ftCo_800A2718':'ftCo_CpuIslandBlocked','ftCo_800A2718_dontinline':'ftCo_CpuIslandBlocked','ftCo_800A2718_dontinline2':'ftCo_CpuIslandBlocked','ftCo_800A2718_dontinline3':'ftCo_CpuIslandBlocked',
 'ftCo_800A8DE4':'ftCo_CpuUpdateRecoveryDestination','ftCo_800A8DE4_noinline':'ftCo_CpuUpdateRecoveryDestination',
 'ftCo_800ADE48':'ftCo_CpuPlanAttack','ftCo_800AEFB8':'ftCo_CpuPlanWithoutTarget',
 'ftCo_CpuShouldAct':'ftCo_CpuPrepareDestinationUpdate','ftCo_CpuDataShouldAct':'ftCo_CpuPrepareDestinationUpdate',
 'ftCo_800B0760':'ftCo_CpuUpdateNanaFollow'}
REMOVED={'ftCo_800A1F3C_noinline','ftCo_800A1F3C_noinline2','ftCo_800A2718_dontinline','ftCo_800A2718_dontinline2','ftCo_800A2718_dontinline3','ftCo_800A8DE4_noinline','ftCo_CpuDataShouldAct'}
PREFIX=r'^(?:(?:static|inline)\s+)*(?:void|bool|u8|s32|s8|u32|int|int32_t|float|f32|Fighter\s*\*|struct\s+CpuFighter\s*\*)\s*'
def extract(text,name):
 # The native/non-native recovery switch each opens the same conditional block.
 # Mask only the exact non-native alternative for lexical brace accounting.
 text=text.replace('#else\n    switch (fp->kind) {\n#endif', '#else\n'+(' '*len('    switch (fp->kind) {'))+'\n#endif')
 m=re.search(PREFIX+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not m:raise ValueError('Missing actual typed definition '+name)
 d,end=1,m.end()
 while d:d+=(text[end]=='{')-(text[end]=='}');end+=1
 return m,text[m.end():end-1]
def inventory(old,new):
 names=re.findall(PREFIX+r'(ftCo_\w+)\s*\([^;{}]*\)\s*\{',old,re.M)
 report=[];total=0
 for name in names:
  if name in REMOVED:continue
  m,old_body=extract(old,name)
  refs=list(re.finditer(r'\b('+'|'.join(map(re.escape,set(ALIASES)|set(ALIASES.values())))+r')\s*\(',old_body))
  if not refs:continue
  new_name=ALIASES.get(name,name);nm,new_body=extract(new,new_name)
  expected=[ALIASES.get(x.group(1),x.group(1)) for x in refs]
  if name=='ftCo_CpuUpdateNanaFollow':
   # The former forwarding function now contains the real follow computation.
   expected=['ftCo_CpuPlanAttack']
  operations=set(ALIASES.values())
  actual=re.findall(r'\b('+'|'.join(map(re.escape,operations))+r')\s*\(',new_body)
  if name=='ftCo_800B0760':
   # This refactor separates preparation from the actual native follow body.
   # Its original plan call remains in the direct follow body, in the same order.
   assert 'ftCo_CpuPrepareCombatPlan(fp);' in new_body
  assert expected==actual,(name,expected,actual)
  total+=len(refs)
  report.append({'source_function':name,'current_function':new_name,'reference_definition_line':old[:m.start()].count('\n')+1,'current_definition_line':new[:nm.start()].count('\n')+1,'calls':[{'reference_line':old[:m.end()+x.start()].count('\n')+1,'old':x.group(1),'new':ALIASES.get(x.group(1),x.group(1))}for x in refs]})
 return {'status':'PASS','callsite_count':total,'functions':report,'removed_forwarding_or_duplicate_definitions':sorted(REMOVED),'renames':ALIASES}
