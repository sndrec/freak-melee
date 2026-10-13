"""Hidden checked native TEV kernel proof against the pre-edit compiler."""
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
sys.dont_write_bytecode=True
sys.path.insert(0,str(HERE.parent/'hsd_reservation'))
from extract import function

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():parser.error('Fresh A: output required')
    out.mkdir(parents=True);commands=[]
    def run(stage,command):
        p=subprocess.run(command,cwd=ROOT,text=True,capture_output=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(p.stdout+p.stderr)
        commands.append({'stage':stage,'command':command,'exit':p.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if p.returncode:raise RuntimeError(stage+': '+(p.stdout+p.stderr)[-12000:])
        return p.stdout
    baseline=run('reference-head',['git','rev-parse','HEAD']).strip()
    base=ROOT/'src/sysdolphin/baselib';texts={}
    for name in ('texp.c','texpdag.c','texp.h','texpdag.h'):
        texts[name]=run('reference-'+name.replace('.','-'),['git','show',baseline+':src/sysdolphin/baselib/'+name])
    public=set(re.findall(r'(?m)^(?!static|#)[^\n;{}]*\b([A-Za-z_]\w*)\([^;{}]*\)\s*\{',texts['texp.c']+texts['texpdag.c']))
    prefix=''.join('#define '+n+' Reference_'+n+'\n' for n in sorted(public))
    units=[]
    for folder,source,prelude in [('current',{n:(base/n).read_text() for n in texts},''),('reference',texts,prefix)]:
        directory=out/folder;directory.mkdir()
        for name in ('texp.h','texpdag.h'):(directory/name).write_text(source[name])
        for name in ('texp.c','texpdag.c'):
            text=source[name]
            if name=='texp.c':
                for omitted in ('HSD_TExpSetReg','HSD_TExpSetupTev'):text=text.replace(function(text,omitted),'')
            path=directory/name;path.write_text(prelude+text);units.append(path)
    stage=out/'stage.c';stage.write_text('#include <sysdolphin/baselib/tev.h>\n'+function((base/'tev.c').read_text(),'HSD_Index2TevStage')+'\n');units.append(stage)
    inputs=[base/n for n in texts]+[base/'tev.c',HERE/'test.c',HERE/'run.py',HERE/'README.md',HERE.parent/'hsd_reservation/extract.py',args.compiler]
    inputs+=list(base.rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    hashes={str(p):digest(p) for p in inputs}
    run('compile',[str(args.compiler),'-std=c11','-O2','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(base),'-I'+str(ROOT/'extern/dolphin/include'),'-ffunction-sections','-fdata-sections','-fuse-ld=lld','-Wl,/opt:ref',*map(str,units),str(HERE/'test.c'),'-o',str(out/'test.exe')])
    summary=json.loads(run('fixture',[str(out/'test.exe')]))
    for path,value in hashes.items():
        if digest(Path(path))!=value:raise RuntimeError('Input changed: '+path)
    receipt={'status':'PASS','summary':summary,'reference_head':baseline,'source_sha256':hashes,'commands':commands,
             'actual':['complete shared TEV optimizer/scheduler/register-packing/compiler/cleanup kernels',
                       'pre-edit native shader descriptors/constant assignment versus Abort and Checked modes',
                       'cycle, bounded DAG, scratch capacity, register packing, partial descriptor exhaustion and output preservation'],
             'controlled':['calloc/free allocation leaf with deterministic descriptor-allocation failure injection',
                           'trusted synthetic native expression graphs; native assertion exits immediately'],
             'limits':['no imported shader/model graph loader or native heap/reservation proof in this fixture',
                       'builders remain native assertion-based; caller preflight/reservation owns construction admission',
                       'no renderer/game or rollback acceptance']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(summary))
if __name__=='__main__':main()
