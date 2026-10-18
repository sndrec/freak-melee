"""Extract actual home pose, companion hold, visibility and figatree loader."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def body(source, name):
    match = re.search(r'^(?:static\s+)?(?:bool|void)\s+' + name + r'\([^;]*?\)\s*\{', source, re.M)
    assert match, name
    at = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[at] == '{') - (source[at] == '}')
        at += 1
    return source[match.start():at]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--snapshot', type=Path, default=Path('A:/FreakMelee-test-archives/baseline72'))
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        parser.error('Use a fresh private output directory on A:')
    out.mkdir(parents=True)
    compiler = args.snapshot.resolve()/'compiler/clang.exe'
    home = ROOT/'src/melee/gm/native_home.c'
    anim = ROOT/'src/melee/ft/ftanim.c'
    bodies = [body(home.read_text(encoding='utf-8'), name) for name in
        ['workshop_pose_span', 'workshop_pose_tree_valid', 'nativeHome_WorkshopPoseRelease',
         'nativeHome_WorkshopFighterVisible', 'workshop_hold_companion',
         'nativeHome_WorkshopPose', 'nativeHome_WorkshopPoseInvalidate']]
    bodies.insert(0, body(anim.read_text(encoding='utf-8'), 'ftAnim_8006F7C8'))
    (out/'bodies.inc').write_text('\n\n'.join(bodies)+'\n', encoding='utf-8')
    flags = ['-std=gnu11', '-O2', '-Wall', '-fms-runtime-lib=dll', '-DMELEE_NATIVE=1',
             '-D_DLL', '-D_MT', '-D_USE_MATH_DEFINES', '-D_CRT_SECURE_NO_WARNINGS',
             '-I'+str(out), '-I'+str(ROOT/'src'), '-I'+str(ROOT/'extern/dolphin/include')]
    pool = ROOT/'src/melee/gm/native_match_pool.c'
    command = [compiler, *flags, HERE/'test.c', pool, '-fuse-ld=lld-link', '-o', out/'test.exe']
    lines = ['@echo off', 'call "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul',
             subprocess.list2cmdline(list(map(str, command)))]
    (out/'compile.cmd').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    inputs = [home, anim, pool, HERE/'test.c', HERE/'run.py', HERE/'README.md',
        out/'bodies.inc', args.snapshot.resolve()/'freeze.json']
    inputs += list((ROOT/'src').rglob('*.h')) + list((ROOT/'extern/dolphin/include').rglob('*.h')) + list(compiler.parent.rglob('*'))
    receipt = {'status':'PENDING', 'command':list(map(str,command)),
        'inputs':{str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs if p.is_file()},
        'scope':'actual home Pose/Release/Invalidate, companion process hold, fighter visibility, tree validation and ftAnim loader; actual NativeFighterLifetime header and native_match_pool.c fixed storage; controlled source registry/program publication/member IDs/actor view/callback generation, asset lookup/rig-validation, HSD animation allocation/evaluation, motion entry and camera; no actual family mapping, VM execution, gameplay, renderer pixels or UI claim'}
    try:
        for command, name in [(['cmd','/c',out/'compile.cmd'], 'compile'), ([out/'test.exe'], 'test')]:
            result = subprocess.run(list(map(str,command)), cwd=ROOT, capture_output=True,
                text=True, encoding='utf-8', errors='replace', creationflags=subprocess.CREATE_NO_WINDOW, timeout=120)
            (out/(name+'.log')).write_text(result.stdout+result.stderr, encoding='utf-8')
            if result.returncode: raise RuntimeError(result.stdout+result.stderr)
            receipt[name] = result.stdout
        changed = [p for p, expected in receipt['inputs'].items()
            if hashlib.sha256(Path(p).read_bytes()).hexdigest() != expected]
        if changed: raise RuntimeError('Pinned inputs changed: '+', '.join(changed))
        receipt['inputs_unchanged'] = True
        receipt['status'] = 'PASS'
    except Exception as error:
        receipt['status'] = 'FAIL'
        receipt['error'] = str(error)
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(receipt['status'], receipt.get('test',receipt.get('error','')))
    if receipt['status'] != 'PASS': raise SystemExit(1)

if __name__ == '__main__': main()
