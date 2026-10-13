import argparse,hashlib,importlib.util,json,subprocess
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve();assert not out.exists();out.mkdir()
spec=importlib.util.spec_from_file_location('extract',here.parent/'script_cpu_control/extract.py');extract=importlib.util.module_from_spec(spec);spec.loader.exec_module(extract)
source=root/'src/melee/gm/native_netplay_match.c';text=source.read_text();prefix=text[:text.index('static void fail')].replace('#ifdef MELEE_NATIVE','',1)
(out/'match_state.inc').write_text(prefix)
(out/'match_boundaries.inc').write_text('\n'.join(extract.body(text,name) for name in ['nativeNetplayMatch_BeginFrame','nativeNetplayMatch_EndFrame'])+'\n')
units=[here/'test.c']+[root/'src/melee/gm'/name for name in ['native_confirmed_checkpoint.c','native_match_arena.c','native_rollback_input.c']]+[root/'src/sysdolphin/baselib/audio_rollback.c']
inputs=[source,Path(__file__),here.parent/'script_cpu_control/extract.py',a.compiler]+units+list((root/'src').rglob('*.h'))+list((root/'extern/dolphin/include').rglob('*.h'))+list(out.glob('*.inc'))
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();hashes={str(p):sha(p) for p in inputs}
cmd=[str(a.compiler),'-std=gnu11','-O2','-g','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(root/'src'),'-I'+str(root/'extern/dolphin/include'),'-I'+str(root/'src/melee/gm'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
commands=[]
for name,command in [('build',cmd),('fixture',[str(out/'test.exe')])]:
 r=subprocess.run(command,cwd=root,capture_output=True,text=True,timeout=60,creationflags=subprocess.CREATE_NO_WINDOW);(out/(name+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':name,'command':command,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n');assert r.returncode==0,(r.stdout+r.stderr)[-4000:]
assert all(sha(Path(p))==digest for p,digest in hashes.items())
(out/'receipt.json').write_text(json.dumps({'status':'PASS','source_sha256':hashes,'commands':commands,'actual':['unchanged production BeginFrame/EndFrame and match host-state definitions','actual confirmed checkpoint, input history, contiguous arena and rollback audio journal'],'controlled':['transport delivery/failure and clock/wait primitives','single integer gameplay owner/digest; diagnostic and unrelated trace leaves abort'],'limits':['not socket/GPU/full-game/sudden-death acceptance','no timing claim; normal input-filter receive simulated explicitly']},indent=2)+'\n');print(r.stdout,end='')
