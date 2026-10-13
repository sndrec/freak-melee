"""Build isolated native/interpreted copies of real normal-pipeline modules."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
from lower import Compiler

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def run(command,output,name,cwd=ROOT):
    command=list(map(str,command))
    result=subprocess.run(command,cwd=cwd,capture_output=True,text=True,timeout=180,
        creationflags=subprocess.CREATE_NO_WINDOW)
    (output/(name+'.log')).write_text(result.stdout+result.stderr,newline='\n')
    if result.returncode:raise RuntimeError(name+' failed: '+str(output/(name+'.log'))+'\n'+result.stderr[-4000:])
    return {'stage':name,'command':command,'exit':result.returncode}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pipeline',type=Path,required=True)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--source-root',type=Path,default=ROOT)
    parser.add_argument('--no-link',action='store_true')
    args=parser.parse_args()
    output=args.output.resolve();root=args.source_root.resolve()
    if output.drive.upper()!='A:' or output.exists():parser.error('Use a fresh output directory on A:')
    output.mkdir(parents=True)
    compiler=args.compiler.resolve();inputs={str(p):sha(p) for p in HERE.glob('*.py')}
    for p in [HERE/'vm_runtime.h',HERE/'vm_runtime.c']:inputs[str(p)]=sha(p)
    # Every included live native header is frozen in the receipt, rather than
    # trusting an archived module fingerprint or a hand-maintained SDK number.
    for folder in [root/'src',root/'extern/dolphin/include']:
        for p in folder.rglob('*.h'):inputs[str(p)]=sha(p)
    common=[compiler,'-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1',
        '-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(root/'src'),'-I'+str(root/'extern/dolphin/include'),'-I'+str(HERE)]
    results={}
    for member in ['popo','nana']:
        dest=output/member;dest.mkdir()
        original=args.pipeline/member/'project/behavior.generated.c'
        inputs[str(original.resolve())]=sha(original)
        source=dest/'source.c';source.write_bytes(original.read_bytes())
        commands=[]
        # AST JSON can be large; keep it outside the checkout and capture raw
        # stdout directly instead of decoding/re-encoding compiler diagnostics.
        ast=dest/'ast.json';layouts=dest/'layouts.txt'
        for name,extra,path in [('ast',['-fsyntax-only','-Xclang','-ast-dump=json'],ast),
            ('layouts',['-fsyntax-only','-Xclang','-fdump-record-layouts-simple','-Xclang','-fdump-record-layouts-complete'],layouts)]:
            command=list(map(str,common+extra+[source]))
            with path.open('wb') as out, (dest/(name+'.log')).open('wb') as log:
                result=subprocess.run(command,cwd=root,stdout=out,stderr=log,timeout=180,
                    creationflags=subprocess.CREATE_NO_WINDOW)
            commands.append({'stage':name,'command':command,'exit':result.returncode})
            if result.returncode:raise RuntimeError(name+' failed: '+str(dest/(name+'.log')))
        lowering=Compiler(source.read_text(),json.loads(ast.read_text()),layouts.read_text())
        lowering.compile();lowering.emit(dest/'interpreted.c',dest/'native.c',dest/'lowering.json')
        for mode in ['native','interpreted']:
            if args.no_link:
                extra=['-fsyntax-only',dest/(mode+'.c')]
            else:
                extra=['-fuse-ld=lld','-shared',dest/(mode+'.c')]
                if mode=='interpreted':extra += [HERE/'vm_runtime.c']
                extra += ['-o',dest/(mode+'.dll')]
            commands.append(run(common+extra,dest,mode,root))
        results[member]={'commands':commands,'lowering':json.loads((dest/'lowering.json').read_text()),
            'source_sha256':sha(source),'native_source_sha256':sha(dest/'native.c'),
            'interpreted_source_sha256':sha(dest/'interpreted.c')}
    for path,digest in inputs.items():
        if sha(Path(path))!=digest:raise RuntimeError('Input changed during preparation: '+path)
    receipt={'status':'PASS isolated actual-module lowering and '+('syntax' if args.no_link else 'DLL')+' build',
        'members':results,'inputs':inputs,'production_changes':False,
        'scope':'Preparation only. No game runs, performance conclusion or gameplay parity claim.'}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n')
    print(json.dumps({'status':receipt['status'],'members':{k:v['lowering']['authored_function_count'] for k,v in results.items()}}))

if __name__=='__main__':main()
