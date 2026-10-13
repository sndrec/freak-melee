"""Actual HSD object/proc lifecycle fixture, fixed pool and global-region oracle."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();out=args.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('fresh A: output required')
    out.mkdir(parents=True);compiler=args.compiler.resolve()/'clang.exe'
    sources=[HERE/'test.c',ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/melee/gm/native_match_snapshot_pool.c',ROOT/'src/melee/gm/native_rollback_state.c',ROOT/'src/melee/gm/native_match_arena.c',ROOT/'src/melee/gm/native_match_scene.c']
    sources += [ROOT/'src/sysdolphin/baselib'/name for name in ['gobj.c','gobjinit.c','gobjplink.c','gobjproc.c','gobjgxlink.c','gobjuserdata.c','gobjobject.c','native_gobj_pool_storage.c']]
    sources += [ROOT/'src/melee/ft/native_fighter_pool.c',ROOT/'src/melee/gr/native_ground_pool.c',ROOT/'src/melee/gr/native_green_grid.c',ROOT/'src/melee/gr/native_green_grid_snapshot.c',ROOT/'src/melee/it/native_item_pool.c',ROOT/'src/melee/it/native_item_link_pool.c',ROOT/'tests/native/script_scene_boundary.c']
    sources += [ROOT/'tests/native/script_effect_actor_absent.c']
    sources += [ROOT/'tests/native/item_actor_absent.c']
    inputs=sources+[compiler,Path(__file__).resolve(),HERE/'README.md',HERE/'snapshot_cases.h',HERE/'arena_cases.h']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    before={str(path):sha(path) for path in inputs}
    command=[str(compiler),'-std=gnu11','-O2','-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref',
             '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),
             '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include')]+list(map(str,sources))+['-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if result.returncode:raise RuntimeError(stage+' failed: '+(result.stdout+result.stderr)[-5000:])
    summary=json.loads(result.stdout)
    if any(sha(Path(path))!=expected for path,expected in before.items()):raise RuntimeError('inputs changed')
    receipt={'status':'PASS','summary':summary,'commands':commands,'input_sha256':before,
        'boundaries':['actual complete HSD constructor/queue/priority/deferred removal/free bodies linked',
                      'actual nativeRollbackState_GetGlobalRegions plus scene-arena backing save/restore',
                      'model/light destructors trapped; no model assets or whole-game acceptance'],
        'limits':['provisional capacities based on one VS census; other scenes/stages pending',
                  'other owner families still require page snapshots; no complete explicit match snapshot or speed claim']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':summary}))


if __name__=='__main__':main()
