"""Actual source parser and pure owner-resource recipe preparation."""
import argparse,hashlib,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
    out.mkdir(parents=True)
    units=[HERE/'test.c',ROOT/'src/melee/ft/native_script_source.c',ROOT/'src/melee/ft/native_script_resource_metadata.c']
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    inputs=units+[HERE/'run.py',HERE/'README.md',a.compiler.resolve()]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    hashes={str(path):sha(path) for path in inputs};commands=[]
    command=[str(a.compiler.resolve()),'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,units),'-o',str(out/'test.exe')]
    for name,cmd in [('compile',command),('fixture',[str(out/'test.exe')])]:
        r=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=90,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(name+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':name,'command':cmd,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if r.returncode:raise RuntimeError(name+': '+(r.stdout+r.stderr)[-6000:])
    changed=[path for path,h in hashes.items() if sha(Path(path))!=h]
    if changed:raise RuntimeError('inputs changed: '+str(changed))
    receipt={'status':'PASS','summary':json.loads(r.stdout),'commands':commands,'input_sha256':hashes,'actual':['complete source hierarchy parser','owner-scoped resource metadata grammar/rig validation','success-only recipe outputs'], 'limits':['pure recipe preparation only; no ItemProgram publication/body VM/collection/native rig/DAT/game proof']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
