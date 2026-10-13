"""Actual four-stock DAT/owned recipes/preparation; hidden and private."""
import argparse,hashlib,json,subprocess,uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,required=True);p.add_argument('--iso',type=Path,required=True);p.add_argument('--output',type=Path)
 a=p.parse_args();out=(a.output or Path('A:/FreakMelee-test-archives')/('source-owned-declarations-ctest-'+uuid.uuid4().hex)).resolve()
 if out.drive.upper()!='A:' or out.exists():p.error('Use a fresh A: archive')
 out.mkdir(parents=True);(out/'projects').mkdir();sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
 paths=list(HERE.glob('*.*'))+list((ROOT/'src/melee/ft').glob('native_source_*.*'))+[ROOT/'src/melee/ft/native_script_item_program.c',ROOT/'src/melee/ft/native_script_item_program.h',ROOT/'src/melee/ft/ftcharacter_asset_layout.c',ROOT/'src/melee/ft/native_fighter_sound_policy.c']
 hashes={str(path):sha(path) for path in paths};binary=sha(a.exe)
 command=[str(a.exe.resolve()),str(a.iso.resolve()),str(out/'projects')]
 result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180,creationflags=subprocess.CREATE_NO_WINDOW)
 (out/'stdout.log').write_text(result.stdout);(out/'stderr.log').write_text(result.stderr)
 receipt={'command':command,'exit':result.returncode,'binary_sha256':binary,'source_sha256':hashes,'scope':'actual stock-to-owned FMA/DAT importer, typed immutable recipes and four minimal source packages; no whole stock-template bodies, activation/rendering/paired/game acceptance'}
 if result.returncode==0:receipt['result']=json.loads(result.stdout)
 if sha(a.exe)!=binary or any(sha(Path(path))!=value for path,value in hashes.items()):raise RuntimeError('Inputs changed')
 (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt.get('result',{'exit':result.returncode,'error':result.stderr[-2500:]})))
 raise SystemExit(result.returncode)
if __name__=='__main__':main()
