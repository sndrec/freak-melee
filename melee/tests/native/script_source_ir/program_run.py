"""Validate direct source declaration indexing across the real authored corpus."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=args.output.resolve()
    if output.exists() or output.drive.upper()!='A:':parser.error('Use fresh output on A:')
    units=['native_script_program','native_script_source','native_script_expression',
        'native_script_statement','native_script_value']
    sources=[ROOT/'src/melee/ft'/f'{unit}.c' for unit in units]
    headers=[ROOT/'src/melee/ft'/f'{unit}.h' for unit in units if unit!='native_script_value']
    headers += [ROOT/'src/melee/ft'/name for name in ['native_script_ir.h','native_script_vm.h','native_script_bytecode.h']]
    corpus=sorted((ROOT/'examples').rglob('*.fms'))
    files=sources+headers+corpus+[HERE/'program_test.c',Path(__file__).resolve(),args.compiler.resolve()]
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    before={str(path):sha(path) for path in files};output.mkdir(parents=True)
    commands=[];executable=output/'test.exe'
    build=[args.compiler.resolve(),'-std=c11','-O2','-ffp-contract=off','-Wall','-Wextra','-Werror',
        '-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),HERE/'program_test.c',*sources,'-o',executable]
    for stage,command in [('build',build),('run',[executable,*corpus])]:
        command=list(map(str,command));result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,
            timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (output/(stage+'.log')).write_text(result.stdout+result.stderr)
        commands.append({'stage':stage,'command':command,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    if any(sha(Path(path))!=digest for path,digest in before.items()):raise RuntimeError('Input changed during run')
    receipt={'status':'PASS declaration indexing only','result':json.loads(result.stdout),
        'input_sha256':before,'commands':commands,
        'pending':['Complete callback signatures, metadata and native binding families',
            'Engine/Workshop/package integration and gameplay acceptance']}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt['result']))

if __name__=='__main__':main()
