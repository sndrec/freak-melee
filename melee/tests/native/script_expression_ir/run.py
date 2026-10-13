"""Compile/run direct typed expression IR checks, with output confined to A:."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=args.output.resolve()
    if output.drive.upper()!='A:' or output.exists():parser.error('Use a fresh output directory on A:')
    paths=[ROOT/'src/melee/ft/native_script_ir.h',ROOT/'src/melee/ft/native_script_expression.h',ROOT/'src/melee/ft/native_script_expression.c',HERE/'test.c',Path(__file__).resolve()]
    def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
    inputs={str(p):sha(p) for p in paths};output.mkdir(parents=True)
    exe=output/'test.exe';commands=[]
    for stage,command in [('compile',[args.compiler,'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),paths[2],HERE/'test.c','-o',exe]),('run',[exe])]:
        command=list(map(str,command))
        result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (output/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n');commands.append({'stage':stage,'command':command,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    for p in paths:
        if sha(p)!=inputs[str(p)]:raise RuntimeError('Input changed during test: '+str(p))
    receipt={'status':'PASS','result':json.loads(result.stdout),'commands':commands,'inputs':inputs,
        'scope':'Direct parser/type/promotion/semantic resolver/arena/source diagnostics only; no bytecode runtime, engine registry, game or generated C frontend.'}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(receipt['result']))

if __name__=='__main__':main()
