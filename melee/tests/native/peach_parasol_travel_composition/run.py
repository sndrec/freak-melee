"""Fresh frozen55 import/DLL and coherent original-C Parasol phase matrix."""
import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('root', 'baseline', 'output'):
        p.add_argument('--'+name, type=Path, required=True)
    p.add_argument('--prepared', action='store_true', help='Diagnostic continuation only')
    a=p.parse_args(); root,base,out=a.root.resolve(),a.baseline.resolve(),a.output.resolve()
    commands=[]
    freeze=json.loads((base/'freeze.json').read_text())
    for name,expected in freeze.items():
        assert digest(base/Path(name))==expected, 'Snapshot changed: '+name
    def run(args,log):
        args=list(map(str,args));commands.append(args)
        r=subprocess.run(args,cwd=root,capture_output=True,text=True)
        log.write_text(r.stdout+r.stderr,newline='\n')
        if r.returncode: raise RuntimeError(str(log)+'\n'+(r.stdout+r.stderr)[-6500:])
        return r.stdout
    if not a.prepared:
        if out.exists():raise ValueError('Use fresh output')
        r=subprocess.run([sys.executable,str(HERE/'prepare.py'),'--root',str(root),'--baseline',str(base),'--output',str(out),'--generate'],cwd=root,capture_output=True,text=True)
        if r.returncode:raise RuntimeError(r.stdout+r.stderr)
    case=out/'comparison';case.mkdir(exist_ok=True);sdk=base/'sdk'
    run([sys.executable,HERE/'fixture.py','--sdk',sdk,'--output',case,'--generated',out/'generated.inc','--parameters',out/'fighter_parameters.fmp','--dat',out/'project/fighter.dat'],case/'extract.log')
    args=[base/'compiler/clang.exe','-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out),'-I'+str(case),HERE/'test.c',sdk/'src/melee/ft/native_callback_lifetime.c','-o',case/'test.exe']
    batch=case/'compile.cmd';batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,args)))+'\n',newline='\n')
    run(['cmd','/c',batch],case/'compile.log')
    output=run([case/'test.exe'],case/'run.log');print(output.strip())
    m=re.search(r'(\d+) original/generated comparisons, (\d+) checks, (\d+) authored safety cases, maxfloat ([0-9.e+-]+)',output);assert m
    receipt=json.loads((out/'result.json').read_text())
    receipt.update(status='PASS unregistered travel/held source composition',comparisons=int(m[1]),checks=int(m[2]),safety_cases=int(m[3]),max_float=float(m[4]),fixture_commands=commands,verified_snapshot_files=len(freeze),census=json.loads((case/'census.json').read_text()),limits=['No installation, independence, game or constructor/render acceptance.','Actual source travel361..364 and item state0..2 execute against the full generated candidate; allocation, motion/animation internals,831CC geometry and recipient export lookup are controlled; complete neutral/landing/resume entries and exact timer branch execute with generated callbacks.','Shared common144/146 callbacks are directly bound and identities checked; root separate held-lifecycle caller receipt supplies shared entry/physics/input evidence.','Original tilt denominator double promotion is measured against authored Float arithmetic.','Transferred-holder retain phases execute actual source; missing foreign recipient exports skip unsupported private mutation and allow normal terminal removal.', 'NULL held-item landing dereference and unsupported native request indices are excluded from defined pairs; deterministic absent/refusal branches are separately exercised.','Earlier pluck/counter/Float/SideB matrices are reused unchanged, not recounted as this matrix.'])
    inputs={x for x in sdk.rglob('*') if x.is_file()};inputs.update(x for x in (base/'compiler').rglob('*') if x.is_file());inputs.update(x for x in HERE.iterdir() if x.is_file());inputs.update([base/'bin/native_content_tool.exe',base/'freeze.json',out/'project/fighter.dat',out/'fighter_parameters.fmp',HERE.parent/'peach_float_sideb_composition/candidate.fms',HERE.parent/'held_lifecycle_protocol/original54.inc',HERE.parent/'held_lifecycle_protocol/original54_manifest.json'])
    receipt['inputs']={str(x):digest(x) for x in sorted(inputs)}
    receipt['outputs']={str(x):digest(x) for x in sorted(out.rglob('*')) if x.is_file() and x!=out/'result.json'}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print('Receipt:',out/'result.json')

if __name__=='__main__':main()
