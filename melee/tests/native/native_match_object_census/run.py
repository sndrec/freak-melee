"""Focused hidden census fixture; no game or global build."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();out=args.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('fresh A: output required')
    out.mkdir(parents=True)
    compiler=args.compiler.resolve()/'clang.exe'
    sources=[HERE/'test.c',ROOT/'src/melee/gm/native_match_object_census.c']
    inputs=sources+[ROOT/'src/melee/gm/native_match_object_census.h',Path(__file__).resolve(),compiler]
    inputs+=list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    before={str(path):sha(path) for path in inputs}
    env=os.environ.copy();env['FREAK_MELEE_OBJECT_CENSUS']=str(out/'census.log')
    command=[str(compiler),'-std=gnu11','-O2','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include')]+list(map(str,sources))+['-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,env=env,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        (out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if result.returncode:raise RuntimeError(stage+' failed: '+(result.stdout+result.stderr)[-5000:])
    summary=json.loads(result.stdout)
    census=(out/'census.log').read_text()
    if census.count('timing=UNTIMED')!=13 or 'classifier=256' not in census or 'classifier=291' not in census:
        raise RuntimeError('scene/classifier output incomplete')
    if any(sha(Path(path))!=expected for path,expected in before.items()):raise RuntimeError('source changed during test')
    receipt={'status':'PASS','summary':summary,'scene_summaries':13,'commands':commands,'input_sha256':before,
             'boundaries':['actual typed allocator/class registry records, controlled fixture registry-head accessor',
                           'actual typed HSD GObj/proc lists and priority tables',
                           'no actual client allocation/capacity proof or runtime performance claim']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':summary}))


if __name__=='__main__':main()
