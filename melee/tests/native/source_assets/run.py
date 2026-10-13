"""Real disc clone -> owned asset facts/record/import; no game/window/install."""
import argparse,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,required=True);p.add_argument('--iso',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();out=a.output.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('Use a fresh A: archive')
    out.mkdir();exe=a.exe.resolve();sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    source=list((ROOT/'src/melee/ft').glob('native_source_*.c'))+list((ROOT/'src/melee/ft').glob('native_source_*.h'))+[
        ROOT/'src/melee/ft/native_fighter_project.c',ROOT/'src/melee/ft/native_fighter_project.h',
        ROOT/'src/melee/gm/native_fighter_package.c',ROOT/'src/melee/gm/native_content_store.h',
        Path(__file__).resolve(),Path(__file__).with_name('test.c')]
    hashes={str(path):sha(path) for path in source};binary=sha(exe)
    command=[str(exe),str(a.iso.resolve()),str(out/'project'),str(ROOT/'examples/mario_full.fms')]
    result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
    (out/'stdout.log').write_text(result.stdout);(out/'stderr.log').write_text(result.stderr)
    receipt={'command':command,'exit':result.returncode,'binary_sha256':binary,'source_sha256':hashes,
        'scope':'Actual disc clone, callback-free owned facts/record/DAT imports, complete Mario source candidate and typed parameter overrides; no engine-table activation, game or full rollback-performance acceptance'}
    if result.returncode==0:receipt['result']=[json.loads(line) for line in result.stdout.splitlines() if line.startswith('{')]
    if sha(exe)!=binary or any(sha(Path(path))!=value for path,value in hashes.items()):raise RuntimeError('Inputs changed')
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt.get('result',{'exit':result.returncode,'stderr':result.stderr[-2500:]})))
    raise SystemExit(result.returncode)
if __name__=='__main__':main()
