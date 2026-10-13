"""Complete actual swept-point/resource synchronous-lifetime console proof."""
import argparse, hashlib, json, subprocess, sys
from pathlib import Path
HERE = Path(__file__).resolve().parent

def main():
    parser = argparse.ArgumentParser()
    for key in ('root', 'baseline', 'output'):
        parser.add_argument('--'+key, type=Path, required=True)
    args = parser.parse_args()
    root, base, out = args.root.resolve(), args.baseline.resolve(), args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    commands = []
    sources = [HERE/'test.c', root/'src/melee/ft/native_callback_lifetime.c']
    inputs = set(sources) | {HERE/'fixture.py',HERE/'run.py',HERE/'README.md',
        base/'sdk/src/melee/mp/mpcoll.c',root/'src/melee/mp/mpcoll.c',
        root/'src/melee/it/native_article_resource.c',root/'src/melee/lb/lbvector.c'}
    inputs.update(path for path in (root/'src').rglob('*.h'))
    inputs.update(path for path in (root/'extern/dolphin/include').rglob('*.h'))
    inputs.update(path for path in (base/'compiler').rglob('*') if path.is_file())
    hashes = {str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(inputs)}
    def run(command, log):
        command = list(map(str, command)); commands.append(command)
        result = subprocess.run(command, cwd=root, capture_output=True, text=True)
        (out/log).write_text(result.stdout+result.stderr, encoding='utf-8')
        if result.returncode:
            raise RuntimeError(str(out/log)+'\n'+(result.stdout+result.stderr)[-9000:])
        return result.stdout
    run([sys.executable, HERE/'fixture.py', '--root',root,'--baseline',base,'--output',out], 'extract.log')
    command = [base/'compiler/clang.exe','-std=gnu11','-O2','-fms-runtime-lib=dll',
        '-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS',
        '-I'+str(root/'src'),'-I'+str(root/'extern/dolphin/include'),'-I'+str(out),
        *sources,'-o',out/'test.exe']
    batch = out/'compile.cmd'
    batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,command)))+'\n',encoding='utf-8')
    run(['cmd','/c',batch], 'compile.log')
    result = run([out/'test.exe'], 'run.log')
    for path in inputs:
        if hashes[str(path)] != hashlib.sha256(path.read_bytes()).hexdigest():
            raise RuntimeError('Source changed during comparison: '+str(path))
    receipt = {'status':'PASS actual swept-point and pinned resource lifetime',
        'baseline':str(base),'result':result.strip(),'commands':commands,
        'inputs':hashes,
        'outputs':{str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in sorted(out.rglob('*')) if path.is_file()},
        'limits':['Complete frozen63/current point kernel, history/interpolation/bounds and stage result application; analytic stage topology/intersection/remap controlled.',
                  'Actual resource collection lookup, pin, retirement, teardown and live generation registry; ItemLink allocation/publication and HSD free controlled.',
                  'Stage sweep is a distinct 471F8 dispatch probe invoking actual End for callbacks; not an ECB solver comparison.',
                  'No authored Chain/generated script or gameplay acceptance; model-root borrower detachment unchanged and outside this fixture.']}
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(result.strip());print(out/'result.json')
if __name__ == '__main__': main()
