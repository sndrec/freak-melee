"""Check the actual Workshop bone reader/writer with native matrix math."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]

def body(source,name):
    match=re.search(r'^(?:bool|void|s32)\s+'+name+r'\([^;]*?\)\s*\{',source,re.M)
    assert match,name
    at=source.index('{',match.start())+1
    depth=1
    while depth:
        depth+=(source[at]=='{')-(source[at]=='}');at+=1
    return source[match.start():at]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    home=ROOT/'src/melee/gm/native_home.c'
    mtx=ROOT/'src/sysdolphin/baselib/mtx.c'
    quat=ROOT/'src/sysdolphin/baselib/quatlib.c'
    blocks=[body(mtx.read_text(),'HSD_MtxSRTQuat'),body(quat.read_text(),'HSD_QuatLib_8037EB28')]
    blocks += [body(home.read_text(),name) for name in ['nativeHome_WorkshopBoneLocal','nativeHome_WorkshopSetBoneLocal']]
    (out/'bodies.inc').write_text('\n\n'.join(blocks)+'\n')
    compiler=Path('C:/Users/loser/scoop/apps/llvm/current/bin/clang.exe').resolve()
    library=ROOT/'build/native/aurora/aurora_mtx.lib'
    command=[str(compiler),'-std=gnu11','-O2','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES',
        '-D_CRT_SECURE_NO_WARNINGS','-fms-runtime-lib=dll','-D_DLL','-D_MT',
        '-I'+str(out),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),
        str(HERE/'test.c'),str(library),'-fuse-ld=lld-link','-o',str(out/'test.exe')]
    batch='@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\n'
    (out/'compile.cmd').write_text(batch+subprocess.list2cmdline(command)+'\n')
    inputs=[home,mtx,quat,library,compiler,HERE/'test.c',HERE/'run.py',out/'bodies.inc']
    receipt=dict(status='PENDING',scope='actual bone reader/writer, HSD quaternion matrix and matrix-to-Euler bodies; actual Aurora matrix library; controlled actor selection, part count and joint flag/dirty boundaries',
        inputs={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},command=command)
    try:
        for name,cmd in [('compile',['cmd','/c',str(out/'compile.cmd')]),('test',[str(out/'test.exe')])]:
            result=subprocess.run(cmd,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW,timeout=120)
            (out/(name+'.log')).write_text(result.stdout+result.stderr)
            assert result.returncode==0,result.stdout+result.stderr
            receipt[name]=result.stdout
        assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==h for p,h in receipt['inputs'].items())
        receipt['status']='PASS'
    except Exception as error:
        receipt.update(status='FAIL',error=str(error))
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(receipt['status'],receipt.get('test',receipt.get('error','')))
    if receipt['status']!='PASS':raise SystemExit(1)

if __name__=='__main__':main()
