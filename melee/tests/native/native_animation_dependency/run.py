"""Actual AObj/FObj decoder/ownership dependency proof, no game launch."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def body(text,name):
    match=re.search(r'^[\w \t*]+\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{',text,re.M)
    if not match:raise RuntimeError('missing actual body: '+name)
    end=text.index('{',match.start())+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[match.start():end]


def extract(root,out):
    """Generate the exact production body/layout includes for console or CTest."""
    root=Path(root);out=Path(out);out.mkdir(parents=True,exist_ok=True)
    paths=[root/'src/sysdolphin/baselib/object.c',root/'src/sysdolphin/baselib/initialize.c',
           root/'src/melee/lb/lbdvd.c',root/'src/sysdolphin/baselib/native_animation_pool.c',
           root/'src/sysdolphin/baselib/spline.c']
    data=[path.read_bytes() for path in paths];texts=[value.decode('utf-8') for value in data]
    names=['HSD_ObjInit','HSD_CreateMainHeap','lbDvd_80018254']
    values=[body(text,name) for text,name in zip(texts[:3],names)]
    (out/'heap.inc').write_text('\n\n'.join(values)+'\n',encoding='utf-8')
    layout=re.search(r'typedef struct AnimationSnapshotHeader \{[^}]*\} AnimationSnapshotHeader;',texts[3]).group()
    (out/'snapshot_layout.inc').write_text(layout+'\n',encoding='utf-8')
    (out/'hermite.inc').write_text(body(texts[4],'splGetHelmite')+'\n',encoding='utf-8')
    return {'names':names,'values':values,'paths':paths,
            'input_sha256':{str(path):hashlib.sha256(value).hexdigest() for path,value in zip(paths,data)}}


def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();out=a.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('fresh A: output required')
    out.mkdir(parents=True);compiler=a.compiler.resolve()/'clang.exe'
    extracted=extract(ROOT,out);names=extracted['names'];values=extracted['values']
    sources=[HERE/'test.c',ROOT/'src/sysdolphin/baselib/aobj.c',ROOT/'src/sysdolphin/baselib/fobj.c',ROOT/'src/sysdolphin/baselib/random.c',ROOT/'src/melee/lb/lbanim.c',
        ROOT/'src/sysdolphin/baselib/native_animation_pool.c',ROOT/'src/melee/gm/native_match_pool.c',ROOT/'src/melee/gm/native_match_snapshot_pool.c',ROOT/'src/melee/gm/native_rollback_state.c',ROOT/'src/melee/gm/native_match_arena.c',ROOT/'src/sysdolphin/baselib/native_hsd_arena.c']
    inputs=sources+extracted['paths']+[compiler,Path(__file__).resolve()]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();before={str(path):sha(path) for path in inputs}
    if any(before[path]!=expected for path,expected in extracted['input_sha256'].items()):raise RuntimeError('extraction input changed')
    command=[str(compiler),'-std=gnu11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections','-fno-zero-initialized-in-bss','-fuse-ld=lld','-Wl,/opt:ref',
        '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-include',str(ROOT/'src/melee/gm/native_rollback_state_section.h'),
        '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]+list(map(str,sources))+['-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode});(out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if result.returncode:raise RuntimeError(stage+' failed: '+(result.stdout+result.stderr)[-5000:])
    summaries=[json.loads(line) for line in result.stdout.splitlines()];summary={'owner':summaries[0],'decoder':summaries[1]}
    if any(sha(Path(path))!=expected for path,expected in before.items()):raise RuntimeError('input changed')
    receipt={'status':'PASS','summary':summary,'commands':commands,'input_sha256':before,
        'body_sha256':{name:hashlib.sha256(value.encode()).hexdigest() for name,value in zip(names,values)},
        'boundaries':['actual full AObj/FObj/RNG/lbAnim sources and actual typed pool/snapshot utilities',
                      'actual ObjInit/CreateMainHeap bodies; controlled outer heap/class/other-owner hooks, ID lookup, JObj unref/flags and mutation callbacks'],
        'limits':['FLOAT CONSTANT/LINEAR/SPL0/SPL/KEY streams; slope-only/quantized/constraint/real rendering closure pending',
                  'full record replay proof includes target/RNG/global counters; frame-only replay intentionally fails']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(json.dumps({'receipt':str(out/'receipt.json'),'summary':summary}))


if __name__=='__main__':main()
