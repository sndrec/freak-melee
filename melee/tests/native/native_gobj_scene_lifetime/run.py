"""Complete source GM scene runner with actual HSD pool/lifetime ownership."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def body(source,name):
    match=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',source,re.M)
    if not match:raise RuntimeError('complete source body missing: '+name)
    end=source.index('{',match.start())+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[match.start():end]


def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();out=args.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('fresh A: output required')
    out.mkdir(parents=True);compiler=args.compiler.resolve()/'clang.exe'
    original=ROOT/'src/melee/gm/gm_1A3F.c';text=original.read_text()
    routing=[]
    for name in ['routingInfo','stateMachine']:
        match=re.search(r'struct '+name+r'\s*\{.*?\};',text,re.S)
        if not match:raise RuntimeError('source type missing: '+name)
        routing.append(match.group())
    (out/'routing.inc').write_text('\n\n'.join(routing)+'\n',encoding='utf-8')
    functions=['preloadState','firstState','nextState','findState','gm_801A4014','gm_SetNextGameModeStateId']
    bodies=[body(text,name) for name in functions]
    (out/'pipeline.inc').write_text('\n\n'.join(bodies)+'\n',encoding='utf-8')
    sources=[HERE/'test.c',ROOT/'src/melee/gm/native_match_pool.c']
    sources += [ROOT/'src/sysdolphin/baselib'/name for name in ['gobj.c','gobjinit.c','gobjplink.c','gobjproc.c','gobjgxlink.c','gobjuserdata.c','gobjobject.c','native_gobj_pool_storage.c']]
    sources += [ROOT/'src/melee/ft/native_fighter_pool.c',ROOT/'src/melee/gr/native_ground_pool.c',ROOT/'src/melee/gr/native_green_grid.c',ROOT/'src/melee/gr/native_green_grid_snapshot.c',ROOT/'src/melee/it/native_item_pool.c',ROOT/'src/melee/it/native_item_link_pool.c',
                ROOT/'src/melee/gm/native_match_snapshot_pool.c',ROOT/'tests/native/script_scene_boundary.c']
    sources += [ROOT/'tests/native/script_effect_actor_absent.c']
    sources += [ROOT/'tests/native/item_actor_absent.c']
    inputs=sources+[original,compiler,Path(__file__).resolve()]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    before={str(path):sha(path) for path in inputs}
    command=[str(compiler),'-std=gnu11','-O2','-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref',
        '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+list(map(str,sources))+['-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if result.returncode:raise RuntimeError(stage+' failed: '+(result.stdout+result.stderr)[-6000:])
    summary=json.loads(result.stdout)
    if any(sha(Path(path))!=expected for path,expected in before.items()):raise RuntimeError('inputs changed')
    receipt={'status':'PASS','summary':summary,'commands':commands,'input_sha256':before,
        'extracted_bodies':{name:hashlib.sha256(value.encode()).hexdigest() for name,value in zip(functions,bodies)},
        'boundaries':['complete actual GM scene runner and preload scheduling body, no rewriting',
                      'actual HSD GObj/proc pool constructor/callback/draw/retirement implementations',
                      'mode entry/exit routing helpers, scene asset/actor construction, preload heaps and frame loop controlled'],
        'limits':['source pipeline lifetime evidence only, no actual CSS/results/practice asset or interactive acceptance',
                  'no all-game-mode coverage or performance claim']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':summary}))


if __name__=='__main__':main()
