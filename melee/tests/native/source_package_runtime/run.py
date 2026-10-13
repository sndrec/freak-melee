"""Real cached retail assets through production staging/publication and retirement."""
import argparse, hashlib, json, subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True)
    p.add_argument('--cache',type=Path,required=True)
    p.add_argument('--id',required=True)
    p.add_argument('--iso',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--functions',type=int,default=77)
    p.add_argument('--items',type=int,default=2)
    p.add_argument('--effects',type=int,default=1)
    a=p.parse_args();out=a.output.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('Use a fresh A: output directory')
    out.mkdir();sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    files=[a.exe.resolve(),a.iso.resolve(),Path(__file__).resolve(),Path(__file__).with_name('test.c')]
    files+=list((ROOT/'src/melee/ft').glob('native_source_*.[ch]'))
    files+=[ROOT/'src/melee/ft/native_fighter_runtime.c',ROOT/'src/melee/ft/native_fighter_runtime.h',
            ROOT/'src/melee/gm/native_rollback_pages.c',ROOT/'src/melee/gm/native_fighter_package.c']
    inputs={str(path):sha(path) for path in files}
    cache={str(path):sha(path) for path in a.cache.rglob('*') if path.is_file()}
    command=[str(a.exe.resolve()),str(a.cache.resolve()),a.id,str(a.iso.resolve()),
             str(a.functions),str(a.items),str(a.effects)]
    r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,
                     creationflags=subprocess.CREATE_NO_WINDOW)
    (out/'stdout.log').write_text(r.stdout);(out/'stderr.log').write_text(r.stderr)
    unchanged=all(sha(Path(path))==digest for path,digest in inputs.items())
    unchanged=unchanged and cache=={str(path):sha(path) for path in a.cache.rglob('*') if path.is_file()}
    if not unchanged:raise RuntimeError('Source/binary/cache changed during the fixture')
    result=[json.loads(line) for line in r.stdout.splitlines() if line.startswith('{')]
    receipt={'command':command,'exit':r.returncode,'input_sha256':inputs,'cache_sha256':cache,'result':result,
        'scope':'Actual source package preparation, stable staging and engine/program table publication; lease and real initialized page-history block retirement without partial mutation',
        'limitations':['No Fighter/Item/Effect actors created; actual scene/gameplay acceptance is separate',
                       'No claims about complete explicit snapshots or rollback performance']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({'exit':r.returncode,'result':result,'receipt':str(out/'receipt.json')}))
    raise SystemExit(r.returncode)
if __name__=='__main__':main()
