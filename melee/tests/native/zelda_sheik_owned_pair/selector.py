"""Compile the complete actual down-throw selector with a controlled transfer boundary."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def body(text, name):
    m = re.search(r'(?m)^[\w\s*]+\b'+re.escape(name)+r'\([^;{}]*\)\s*\{',text)
    assert m, name
    end, depth = m.end(), 1
    while depth:
        depth += (text[end]=='{')-(text[end]=='}')
        end += 1
    return text[m.start():end].strip()


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--clang',type=Path,required=True)
    p.add_argument('--source-root',type=Path,default=ROOT)
    p.add_argument('--selector-source',type=Path,
                   help='Explicit actual source overlay if the SDK predates the AnimationFamily repair')
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args(); out=a.output.resolve(); clang=a.clang.resolve(); source=a.source_root.resolve()
    if out.drive.upper()!='A:' or out.exists(): p.error('Fresh A: output required')
    paths=[source/'src/melee/ft/kinds/ftCommon/ftCo_Throw.c',source/'src/melee/ft/native_fighter_runtime.c',
           source/'src/melee/ft/native_callback_lifetime.c',HERE/'selector.c',Path(__file__).resolve()]
    if a.selector_source: paths[0]=a.selector_source.resolve()
    inputs={str(x):sha(x) for x in paths}
    inputs.update({str(x):sha(x) for directory in (source/'src',source/'extern/dolphin/include')
                  for x in directory.rglob('*.h')})
    inputs[str(clang)]=sha(clang)
    out.mkdir(parents=True)
    chunks=[body(paths[1].read_text(encoding='utf-8'),'nativeFighterRuntime_AnimationFamily'),
            body(paths[0].read_text(encoding='utf-8'),'ftCo_800DD4B0')]
    (out/'selector_bodies.h').write_text('\n\n'.join(chunks)+'\n',encoding='utf-8',newline='\n')
    command=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-DMELEE_NATIVE=1',
        '-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(source/'src'),
        '-I'+str(source/'extern/dolphin/include'),'-I'+str(out),HERE/'selector.c',paths[2],'-o',out/'selector.exe']
    batch=out/'compile.cmd'
    batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" -vcvars_ver=14.44 >nul\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(list(map(str,command)))+'\n',encoding='utf-8',newline='\n')
    commands=[]
    for stage,cmd in [('compile',['cmd','/d','/c',str(batch)]),('run',[str(out/'selector.exe')])]:
        r=subprocess.run(cmd,cwd=ROOT,text=True,encoding='utf-8',capture_output=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n')
        commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
        if r.returncode: raise RuntimeError(stage+'\n'+r.stdout+r.stderr)
    assert all(sha(Path(x))==v for x,v in inputs.items())
    receipt={'status':'PASS complete actual selector, controlled transfer boundary', 'result':json.loads(r.stdout),
        'header_source_root':str(source),'selector_source':str(paths[0]),
        'inputs':inputs,'commands':commands,'bodies_sha256':sha(out/'selector_bodies.h'),
        'limits':['Actual shared selector and actual AnimationFamily body; downstream throw transfer is controlled.',
                  'Actual registry generation guard covers retired PAGE_NOACCESS and recycled source.',
                  'No whole throw/capture/render/game acceptance; same-generation kind/Definition mutation is not newly guarded by this production selector.']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps(receipt['result']))


if __name__=='__main__':main()
