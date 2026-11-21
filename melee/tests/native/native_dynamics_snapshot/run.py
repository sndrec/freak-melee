"""Actual selected dynamics/wind consumer functions and explicit owner."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
sys.dont_write_bytecode=True
sys.path.insert(0,str(HERE.parent/'hsd_reservation'))
from extract import function

def main():
    p=argparse.ArgumentParser();p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();out=a.output.resolve()
    if out.exists() or out.drive.upper()!='A:':p.error('fresh A: output required')
    out.mkdir(parents=True)
    source=ROOT/'src/melee/lb/lb_00F9.c';text=source.read_text()
    names=['lbDynamics_Reset','lb_8000FD18','popDynamicsData','lb_8000FD48','lb_80011710','inlineA0','inlineA1','lb_800100B0','lb_800115F4','lb_800119DC','lb_80011A50','lb_80011ABC']
    bodies='\n\n'.join(function(text,n) for n in names)
    (out/'consumer.inc').write_text(bodies+'\n')
    units=[HERE/'test.c',ROOT/'src/melee/lb/dynamics_state.c',ROOT/'src/melee/lb/native_dynamics_snapshot.c']
    arena=ROOT/'src/melee/gm/native_match_arena.c'
    inputs=units+[arena,source,HERE/'README.md',HERE/'run.py',HERE.parent/'hsd_reservation/extract.py',a.compiler,out/'consumer.inc']
    inputs+=list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest();pins={str(path):sha(path) for path in inputs}
    flags=['-std=gnu11','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
    commands=[]
    def run(stage,command):
        r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr);commands.append(dict(stage=stage,command=command,exit=r.returncode))
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if r.returncode:raise RuntimeError(stage+': '+(r.stdout+r.stderr)[-4000:])
        return r.stdout
    run('strict',[str(a.compiler),*flags,'-Wall','-Wextra','-Werror','-Wno-array-parameter','-fsyntax-only',*map(str,units)])
    run('compile',[str(a.compiler),*flags,'-O2','-ffp-contract=off','-fuse-ld=lld',*map(str,units),str(arena),'-o',str(out/'test.exe')])
    summary=json.loads(run('fixture',[str(out/'test.exe')]))
    assert all(sha(Path(path))==digest for path,digest in pins.items())
    receipt=dict(status='PASS',summary=summary,source_sha256=pins,commands=commands,extracted_functions=names,limits=['Selected real consumer bodies; controlled matrix setup and idle predicate.','External descriptor records restored explicitly in fixture. No complete graph/match restoration or rollback deadline claim.'])
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(summary))
if __name__=='__main__':main()
