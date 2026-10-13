"""Compile direct source hierarchy reader and inspect the real FMS corpus."""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compiler',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args();out=args.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('Fresh A: output required')
    out.mkdir(parents=True)
    files=sorted((ROOT/'examples').glob('*.fms'))
    inputs=files+[HERE/'test.c',HERE/'run.py',ROOT/'src/melee/ft/native_script_source.c',
        ROOT/'src/melee/ft/native_script_source.h',ROOT/'src/melee/ft/native_script_ir.h',args.compiler.resolve()]
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    before={str(path):sha(path) for path in inputs}
    cmd=[str(args.compiler.resolve()),'-std=c11','-O2','-Wall','-Wextra','-Werror',
        '-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),str(HERE/'test.c'),
        str(ROOT/'src/melee/ft/native_script_source.c'),'-o',str(out/'test.exe')]
    commands=[]
    for name,command in [('build',cmd),('corpus',[str(out/'test.exe'),*[path.as_posix() for path in files]])]:
        result=subprocess.run(command,cwd=ROOT,text=True,capture_output=True,
            creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(name+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        commands.append({'command':command,'exit':result.returncode})
        if result.returncode:raise RuntimeError(name+' failed:\n'+(result.stdout+result.stderr)[-5000:])
    if any(sha(Path(path))!=value for path,value in before.items()):raise RuntimeError('Input changed during corpus run')
    rows=[json.loads(line) for line in result.stdout.splitlines()]
    receipt={'status':'PASS source syntax/hierarchy only','files':rows[:-1],'summary':rows[-1],
        'input_sha256':before,'commands':commands,
        'pending':['Typed metadata/binding resolution and executable bytecode generation','Production runtime/editor/package integration']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'files':len(rows)-1,'summary':rows[-1]}))

if __name__=='__main__':main()
