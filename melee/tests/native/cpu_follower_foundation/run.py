"""Bounded follower foundation console evidence; no game/renderer/runtime installation."""
import argparse,hashlib,json,re,subprocess
from pathlib import Path
from refactor_audit import inventory,PREFIX
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
CPU='src/melee/ft/kinds/ftCommon/ftCo_0A01.c'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def body(text,name):
 m=re.search(PREFIX+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
 if not m:raise ValueError('Missing actual definition: '+name)
 d,end=1,m.end()
 while d:d+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[m.start():end]
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--snapshot',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--compiler-test',type=Path);a=ap.parse_args()
 snapshot=a.snapshot.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);old=snapshot/'sdk'/CPU;new=ROOT/CPU
 frozen=json.loads((snapshot/'freeze.json').read_text(encoding='utf-8'));frozen={k.replace('\\','/'):v for k,v in frozen.items()};assert sha(old)==frozen['sdk/'+CPU]
 clang=snapshot/'compiler/clang.exe';assert sha(clang)==frozen['compiler/clang.exe'];old_text=old.read_text(encoding='utf-8');new_text=new.read_text(encoding='utf-8')
 refactor=inventory(old_text,new_text);(out/'refactor_sites.json').write_text(json.dumps(refactor,indent=2)+'\n',encoding='utf-8',newline='\n')
 (out/'history_stock.h').write_text(body(old_text,'inlineM0')+'\n'+body(old_text,'ftCo_800B0918')+'\n',encoding='utf-8',newline='\n')
 history_parts=[body(old_text,'ftCo_CpuShouldAct').replace('ftCo_CpuShouldAct','stock_destination_update'),body(new_text,'ftCo_CpuPrepareDestinationUpdate')]
 (out/'destination_bodies.h').write_text('\n'.join(history_parts),encoding='utf-8',newline='\n')
 dispatch=body(new_text,'ftCo_800B3900')
 for i,n in enumerate(['ftCo_800B33B0','ftCo_800B2AFC','ftCo_800B2790','ftCo_800B3E04','ftCo_800B0AF4']):dispatch=re.sub(r'\b'+n+r'\b','phase_'+str(i+1),dispatch)
 (out/'dispatch_body.h').write_text(dispatch+'\n',encoding='utf-8',newline='\n')
 parts=[body(new_text,'inlineJ0')]
 for prefix,text in [('stock_',old_text),('current_',new_text)]:
  for name in ['ftCo_800B2AFC','ftCo_800B0AF4']:
   function=body(text,name).replace('ftCo_800ADE48','ftCo_CpuPlanAttack')
   function=re.sub(r'\b'+name+r'\b',prefix+name,function)
   function=function.replace('ftCo_800A1F3C_noinline2','ftCo_CpuSetDestination')
   parts.append(function)
 (out/'hook_bodies.h').write_text('\n'.join(parts),encoding='utf-8',newline='\n')
 module_source=ROOT/'src/melee/ft/native_fighter_module.c';target_source=ROOT/'src/melee/ft/native_fighter_target.c'
 backend=[body(new_text,name) for name in ['ftCo_800A1F98','ftCo_800A5944','ftCo_CpuIslandBlocked']]
 backend.append(body(old_text,'ftCo_800A2718').replace('ftCo_800A2718','stock_island_blocked'))
 backend.extend(body(module_source.read_text(encoding='utf-8'),name) for name in ['stage_same_line_island','stage_cpu_floor_excluded','stage_cpu_island_blocked'])
 backend.extend(body(target_source.read_text(encoding='utf-8'),name) for name in ['nativeFighterTarget_PositionDelta','nativeFighterTarget_Damage','nativeFighterTarget_Facing','nativeFighterTarget_Sleeping','nativeFighterTarget_FloorLine','nativeFighterTarget_CpuBodyHalfWidth'])
 (out/'backend_bodies.h').write_text('\n'.join(backend),encoding='utf-8',newline='\n')
 capture_source=ROOT/'src/melee/ft/ftcpuattack.c'
 (out/'capture_body.h').write_text(body(capture_source.read_text(encoding='utf-8'),'ftCo_800B683C'),encoding='utf-8',newline='\n')
 common=[str(clang),'-std=c11','-O2','-fms-extensions','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),'-MD','-MF',str(out/'dependencies.d')]
 sources=re.search(r'set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+(.*?)\)',(ROOT/'CMakeLists.txt').read_text(encoding='utf-8'),re.S).group(1).split()
 operations=[]
 generator=a.compiler_test or out/'compiler.exe'
 if not a.compiler_test:operations.append(('compiler_build',common+[str(HERE/'compiler.c'),*[str(ROOT/x) for x in sources],str(ROOT/'src/melee/ft/native_fighter_parameters.c'),'-o',str(generator)]))
 operations.append(('generate',[str(generator),str(out/'follower_action.inc'),str(out/'follower_callback.inc')]))
 for name in ['history','source','hook','backend']:
  extra=[str(ROOT/'src/melee/ft/native_cpu_history.c')] if name=='history' else []
  operations.extend([(name+'_build',common+[str(HERE/(name+'_test.c')),*extra,'-o',str(out/(name+'.exe'))]),(name+'_run',[str(out/(name+'.exe'))])])
 for route in ['action','callback']:operations.extend([(route+'_build',common+['-DFOLLOWER_MODULE="follower_'+route+'.inc"',str(HERE/'module.c'),'-o',str(out/(route+'.exe'))]),(route+'_run',[str(out/(route+'.exe'))])])
 inputs=[old,new,clang,snapshot/'freeze.json',Path(__file__).resolve(),HERE/'refactor_audit.py',*[HERE/x for x in ['compiler.c','module.c','history_test.c','source_test.c','hook_test.c','backend_test.c']],module_source,target_source,capture_source,ROOT/'src/melee/ft/native_cpu_history.c',ROOT/'src/melee/ft/native_cpu_history.h',ROOT/'src/melee/ft/native_fighter_module.h',ROOT/'src/melee/ft/native_fighter_runtime.h',*[ROOT/x for x in sources]]
 receipt={'status':'WIP','scope':'actual production history and full B3900 orchestration, actual destination latch, generated fighter callbacks both routes; controlled live registry/lifetime/planner/stage queries; no mode6 follower script/renderer/game claim','original_snapshot':str(snapshot),'snapshot_sha256':sha(snapshot/'freeze.json'),'inputs':{str(x):sha(x) for x in inputs},'operations':[],'source_functions':['inlineM0','ftCo_800B0918','ftCo_CpuShouldAct','ftCo_CpuPrepareDestinationUpdate','ftCo_800B3900'],'refactor_callsites':refactor['callsite_count'],'controlled_dispatch_children':['B33B0','B2AFC','B2790','B3E04','B0AF4']}
 for name,cmd in operations:
  r=subprocess.run(cmd,cwd=ROOT,text=True,encoding='utf-8',stdout=subprocess.PIPE,stderr=subprocess.STDOUT,creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0));(out/(name+'.log')).write_text(r.stdout,encoding='utf-8',newline='\n');receipt['operations'].append({'name':name,'command':cmd,'exit_code':r.returncode,'output':r.stdout});receipt['status']='FAIL' if r.returncode else 'WIP';(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n');print(name,r.returncode,r.stdout)
  if r.returncode:raise RuntimeError(out/(name+'.log'))
  if name.endswith('_build'):
   dependencies=(out/'dependencies.d').read_text(encoding='utf-8')
   for line in dependencies.splitlines()[1:]:
    path=Path(line.strip().removesuffix('\\').strip().replace('\\ ', ' '))
    if path.is_file():receipt['inputs'][str(path.resolve())]=sha(path)
 receipt['status']='PASS';receipt['inputs'].update({str(out/name):sha(out/name) for name in ['history_stock.h','destination_bodies.h','dispatch_body.h','hook_bodies.h','capture_body.h','backend_bodies.h','follower_action.inc','follower_callback.inc','refactor_sites.json']});(out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n');print(out/'receipt.json')
if __name__=='__main__':main()
