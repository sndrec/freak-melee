"""Interpreted Pikachu/Pichu package publication and grouped gameplay comparisons.

Prepared stock-derived projects and existing input recordings are explicit inputs.
No C generation, native module compiler, visible window or audio.
"""
from pathlib import Path
import sys,json,hashlib,shutil,subprocess,re,argparse
root=Path(__file__).resolve().parents[2];sys.path.insert(0,str(root/'tests/native'))
from fighter_script_bundle_roundtrip import portrait
from pikachu_pichu_closure_parity import catalog,options,coverage,FOCUSED
parser=argparse.ArgumentParser(description="Current source packages versus stock with hidden, muted drawn replays.")
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--prepared-root',type=Path,required=True,help='Stock-derived Pikachu/Pichu project directories')
parser.add_argument('--manifest-root',type=Path,default=root/'.local')
parser.add_argument('--case',action='append')
args=parser.parse_args();out=args.output.resolve()
if out.drive.upper()!='A:' or out.exists():parser.error('Fresh A: output required')
out.mkdir(parents=True)
tool=root/'build/native/native_content_tool.exe';exe=root/'build/native/melee_native.exe';iso=root.parent/'Super Smash Bros. Melee (USA) (v1.02).iso'
base=args.prepared_root.resolve()
portrait(out/'portrait.png');commands=[];results=[];packages={}
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
inputs=[Path(__file__).resolve(),tool,exe,iso,root/'tools/run_fighter_parity.py',root/'tools/compare_fighter_parity.py',root/'tests/native/pikachu_pichu_closure_parity.py',root/'tests/native/fighter_script_bundle_roundtrip.py',root/'build/native/native_source_package_runtime_test.exe']
inputs += list((root/'src/melee/ft').glob('native_script*.[ch]'))+list((root/'src/melee/it').glob('native_script*.[ch]'))
pins={str(p):sha(p) for p in inputs}
def run(label,cmd,timeout=180,required=True):
 cmd=list(map(str,cmd));r=subprocess.run(cmd,cwd=root,capture_output=True,text=True,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/(label+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8')
 commands.append({'label':label,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 print(label,r.returncode,(r.stdout+r.stderr)[-700:].replace('\n',' | '),flush=True)
 if required and r.returncode:raise RuntimeError(label)
 return r
for fighter in ('Pikachu','Pichu'):
 project=out/(fighter+'-project')
 shutil.copytree(base/fighter,project,ignore=shutil.ignore_patterns('fighter_parameters.fmp'))
 template=root/'examples'/(fighter.lower()+'_wip.fms');pins[str(template)]=sha(template)
 script=out/(fighter+'-owned.fms')
 run(fighter+'-parameters',[tool,'import-parameters',project,template,script])
 pins[str(script)]=sha(script)
 pins.update({str(p):sha(p) for p in project.rglob('*') if p.is_file() and p.name not in ['project.txt']})
 run(fighter+'-build',[tool,'script-build',project,script])
 store=out/(fighter+'-store');r=run(fighter+'-package',[tool,'--root',store,'package-project',project,out/'portrait.png'])
 package=re.search(r'Cached source fighter ([0-9a-f]{64})',r.stdout).group(1)
 run(fighter+'-verify',[tool,'--root',store,'verify-package',package])
 run(fighter+'-accept',[tool,'--root',store,'accept',package])
 run(fighter+'-publish',[sys.executable,root/'tests/native/source_package_runtime/run.py','--exe',root/'build/native/native_source_package_runtime_test.exe','--cache',store,'--id',package,'--iso',iso,'--output',out/(fighter+'-publication'),'--functions',210,'--items',3,'--effects',0])
 packages[fighter.lower()]={'package':package,'store':str(store),'project':str(project)}
(out/'packages.json').write_text(json.dumps(packages,indent=2)+'\n')
rows=catalog(args.manifest_root.resolve())
cases=args.case or [case for case in FOCUSED if not case.startswith('costume/')]
for fighter in ('pikachu','pichu'):
 for case in cases:
  if (fighter,case) not in rows:parser.error('Unknown recorded case: '+fighter+'/'+case)
  if options(fighter,case)['color']:parser.error('Variant costumes need a separately installed skin package')
  row=rows[(fighter,case)];recording=Path(row['recording']);assert sha(recording)==row['recording_sha256'];pins[str(recording)]=sha(recording)
  opts=options(fighter,case);folder=out/(fighter+'-'+case.replace('/','-'));p=packages[fighter]
  cmd=[sys.executable,root/'tools/run_fighter_parity.py','--exe',exe,'--iso',iso,'--inputs',recording,'--stock-fighter',fighter,'--package-id',p['package'],'--content-root',p['store'],'--out-dir',folder,'--independent','--stage',opts['stage'],'--stocks',opts['stocks'],'--p2',opts['p2'],'--p1-type',opts['p1_type'],'--cpu-level',opts['cpu_level'],*opts['flags']]
  r=run(fighter+'-'+case.replace('/','-'),cmd,required=False)
  result={'fighter':fighter,'case':case,'exit':r.returncode,'comparison':r.stdout.strip(),'output':str(folder)}
  if r.returncode==0:
   try:
    result['coverage']={label:coverage(folder/(recording.stem+'-'+label+'.log'),case,fighter) for label in ('stock','clone')}
    for log in folder.glob('*.log'):
     text=log.read_text(encoding='utf-8',errors='replace')
     assert not re.search(r'\[script\].*(?:source \d+:\d+:|faulted=1)',text),'VM fault in '+str(log)
   except (AssertionError,OSError) as e:result.update(exit=1,error=str(e))
  results.append(result);(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
assert all(sha(Path(p))==expected for p,expected in pins.items())
(out/'receipt.json').write_text(json.dumps({'status':'PASS' if all(r['exit']==0 for r in results) else 'FAIL','commands':commands,'results':results,'input_sha256':pins,'mode':'actual interpreted source packages; current stock versus clone; hidden muted accelerated drawn replays'},indent=2)+'\n')
print('finished',sum(r['exit']==0 for r in results),'/',len(results))
raise SystemExit(0 if all(r['exit']==0 for r in results) else 1)
