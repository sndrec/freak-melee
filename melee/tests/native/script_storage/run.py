"""Compile actual declaration storage and VM replay tests with outputs on A:."""
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
    units=['native_script_storage.c','native_script_program.c','native_script_expression.c','native_script_value.c',
        'native_script_source.c','native_script_statement.c','native_script_bytecode.c','native_script_vm.c']
    files=[ROOT/'src/melee/ft'/p for p in units]+[HERE/'test.c',HERE/'probe.fms',Path(__file__).resolve()]
    files += [ROOT/'src/melee/ft'/p for p in ['native_script_storage.h','native_script_program.h','native_script_vm.h','native_script_bytecode.h','native_script_ir.h']]
    def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
    inputs={str(p):sha(p) for p in files};output.mkdir(parents=True);exe=output/'test.exe';commands=[]
    compile=[args.compiler,'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src')]
    compile += [ROOT/'src/melee/ft'/p for p in units]+[HERE/'test.c','-o',exe]
    for stage,command in [('compile',compile),('run',[exe,HERE/'probe.fms'])]:
        command=list(map(str,command));result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (output/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n');commands.append({'stage':stage,'command':command,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    for p in files:
        if sha(p)!=inputs[str(p)]:raise RuntimeError('Input changed during test: '+str(p))
    receipt={'status':'PASS','result':json.loads(result.stdout),'commands':commands,'inputs':inputs,
        'scope':'Actual ProgramIndex, direct frontend, typed storage, bytecode and VM; caller-owned instances and snapshots. No game/engine/module integration or object resurrection acceptance.'}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(receipt['result']))

if __name__=='__main__':main()
