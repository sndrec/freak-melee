"""Pure typed event records; strict hidden compile and immutable private receipt."""
import argparse,hashlib,json,subprocess
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();out=a.output.resolve()
    if out.exists():p.error('fresh private A: output required')
    out.mkdir(parents=True)
    units=[HERE/'test.c']+[ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in ['source','expression','statement','program','bytecode','value','vm','events','event_payload']]
    inputs=units+[HERE/'run.py',HERE/'README.md',a.compiler]+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();hashes={str(path):sha(path) for path in inputs}
    cmd=[str(a.compiler),'-std=c11','-Wall','-Wextra','-Werror','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),*map(str,units),'-o',str(out/'test.exe')]
    commands=[]
    for stage,command in [('compile',cmd),('fixture',[str(out/'test.exe')])]:
        r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append({'stage':stage,'command':command,'exit':r.returncode});(out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if r.returncode:raise RuntimeError(stage+':\n'+(r.stdout+r.stderr)[-9000:])
    changed=[name for name,h in hashes.items() if sha(Path(name))!=h]
    if changed:raise RuntimeError('Inputs changed: '+str(changed))
    receipt={'status':'PASS','summary':json.loads(r.stdout),'input_sha256':hashes,'commands':commands,'scope':['actual source/ProgramIndex/body/compiler/bytecode/VM','actual immutable event schema and copied record kernel'],'controlled':['native lifetime provider'],'limits':['no Fighter/Execution/afterimage/render/game acceptance','shipped native schema exercises I32/Float only']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt['summary']))
if __name__=='__main__':main()
