"""Unchanged native CPU kernels and real property admission/publication."""
import re,hashlib
def body(text,name):
 m=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not m:raise RuntimeError('missing body '+name)
 at=text.index('{',m.start())+1;depth=1
 while depth:depth+=(text[at]=='{')-(text[at]=='}');at+=1
 return text[m.start():at]
def extract(root,out):
 command=root/'src/melee/ft/ftcmdscript.c';selector=root/'src/melee/ft/kinds/ftCommon/ftCo_0A01.c';text=selector.read_text()
 typedef=re.search(r'typedef struct CpuQueryIdentity \{.*?\} CpuQueryIdentity;',text,re.S)[0]
 result='\n'.join(body(command.read_text(),name) for name in ['ftCo_800B463C','ftCo_800B46B8','ftCo_800B4778'])+'\n'+typedef+'\n'
 result+='\n'.join(body(text,name) for name in ['inlineD0','inlineD1','cpu_query_identity_live','ftCo_CpuNearestEligibleTarget'])
 (out/'native.inc').write_text(result+'\n')
 (out/'destination.inc').write_text('\n'.join(body(text,n) for n in ['ftCo_CpuSetDestination','ftCo_CpuPrepareDestinationUpdate'])+'\n')
 planner_names=['ftCo_800ADE48_inline0','ftCo_800ADE48_inline1','inlineH0','ftCo_CpuPlanAttack','ftCo_CpuUpdateCommonItemTarget','ftCo_CpuUpdateSpecialItemTarget','ftCo_CpuTargetAct','ftCo_CpuSetTargetActive','ftCo_CpuClearTargetModes','ftCo_CpuSetTargetReady','ftCo_CpuSetTargetFlags','ftCo_CpuInitNoTarget','ftCo_CpuActOnNoTarget','ftCo_CpuPlanWithoutTarget','ftCo_CpuPrepareCombatPlan','ftCo_CpuUpdateRecoveryDestination']
 (out/'planner.inc').write_text('\n'.join(body(text,n) for n in planner_names)+'\n')
 (out/'grabbing.inc').write_text(body(text,'ftCo_IsGrabbing')+'\n'+body(text,'ftCo_IsGrabbing_dontinline')+'\n')
 library=root/'src/melee/ft/ftlib.c'
 (out/'history_lookup.inc').write_text(body(library.read_text(),'ftLib_8008741C')+'\n')
 properties=root/'src/melee/ft/native_script_binding_properties.c';props=properties.read_text()
 (out/'source_guards.inc').write_text('\n'.join(re.sub(r'\berror\b','cpu_property_error',body(props,n)) for n in ['error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'])+'\n')
 (out/'cpu_reads.inc').write_text(props[:props.index('static bool error')]+'\n'+'\n'.join(re.sub(r'\berror\b','cpu_property_error',body(props,n)) for n in ['property','vector_component','nativeScriptBindings_Read','nativeScriptBindings_Write'])+'\n')
 actions=root/'src/melee/ft/native_script_binding_actions.c'
 (out/'actions_read.inc').write_text(re.sub(r'\berror\b','cpu_property_error',body(actions.read_text(),'nativeScriptBindingActions_Read'))+'\n')
 return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [command,selector,properties,actions,library]}
