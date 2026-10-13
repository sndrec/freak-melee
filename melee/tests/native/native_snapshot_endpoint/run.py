"""Endpoint safeguard fixture with controlled region discovery, no game/GPU launch."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve()
    if out.exists() or out.drive.upper()!='A:':parser.error('fresh A: output required')
    out.mkdir(parents=True)
    sources=[HERE/'test.c',ROOT/'src/melee/gm/native_snapshot_endpoint.c',ROOT/'src/melee/gm/native_match_pool.c']
    inputs=sources+[ROOT/'src/melee/gm/native_match_pool.h',ROOT/'src/melee/gm/native_match_snapshot_pool.h',Path(__file__).resolve()]
    inputs+=list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    compiler=args.compiler.resolve()/'clang.exe'
    inputs+=[compiler]
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    before={str(path):sha(path) for path in inputs}
    command=[str(compiler),'-std=gnu11','-O2','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS',
             '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include')]+list(map(str,sources))+['-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,
                              creationflags=subprocess.CREATE_NO_WINDOW)
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        (out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if result.returncode:raise RuntimeError(stage+' failed: '+(result.stdout+result.stderr)[-5000:])
    summary=json.loads(result.stdout)
    if any(not Path(path).is_file() or sha(Path(path))!=expected for path,expected in before.items()):
        raise RuntimeError('input changed during fixture')
    receipt={'status':'PASS','summary':summary,'commands':commands,'input_sha256':before,
             'limits':['actual endpoint save/restore and comparison; controlled region discovery/tracking flag',
                       'read-only pool byte classification uses real pool binding/allocation/validation with controlled geometry and six injected differences; rejects invalid/overlapping registrations',
                       'diagnostic safeguard only; real region discovery and gameplay tested separately',
                       'no full-game parity or save/load performance claim']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'receipt':str(out/'receipt.json'),'summary':summary}))


if __name__=='__main__':main()
