"""Build/run only the SDK66 linked scalar source console fixture."""
import argparse,hashlib,json,re,shutil,subprocess,sys
from pathlib import Path
HERE=Path(__file__).resolve().parent

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
    a=argparse.ArgumentParser(description=__doc__)
    for k in ('root','baseline','output','iso'):a.add_argument('--'+k,type=Path,required=True)
    x=a.parse_args();root=x.root.resolve();base=x.baseline.resolve();out=x.output.resolve()
    if not (base/'freeze.json').is_file():raise ValueError('Frozen SDK66 required')
    out.mkdir(parents=True,exist_ok=False);commands=[]
    def run(cmd,name):
        cmd=list(map(str,cmd));commands.append(cmd)
        r=subprocess.run(cmd,cwd=root,capture_output=True,text=True,encoding='utf-8',errors='replace')
        (out/name).write_text(r.stdout+r.stderr,encoding='utf-8')
        if r.returncode:raise RuntimeError(str(out/name)+'\n'+(r.stdout+r.stderr)[-8000:])
        return r.stdout
    run([sys.executable,HERE/'fixture.py','--root',root,'--baseline',base,'--output',out,'--iso',x.iso],'extract.log')
    tool=base/'bin/native_content_tool.exe';clang=base/'compiler/clang.exe';sdk=base/'sdk'
    run([tool,'script-c',out/'owned.fms','ice_linked_scalar',out/'generated.inc'],'generate.log')
    generated=(out/'generated.inc').read_text(encoding='utf-8');prov=json.loads((out/'provenance.json').read_text(encoding='utf-8'))
    params=prov['parameters'];m=re.search(r'count != (\d+) \|\| schema != (0x[0-9a-fA-F]+)ULL',generated)
    if not m or int(m[1])!=len(params):raise ValueError('Parameter schema mismatch')
    (out/'parameters.inc').write_text('static const uint32_t parameter_bits[]={'+','.join('UINT32_C(%d)'%p['bits'] for p in params)+'};\nstatic const uint64_t parameter_schema='+m[2]+'ULL;\n',encoding='utf-8')
    hosts=sorted(set(re.findall(r'host->(\w+) == 0',generated)))
    (out/'hosts.inc').write_text('\n'.join('host.'+h+' = (__typeof__(host.'+h+'))unexpected;' for h in hosts)+'\n',encoding='utf-8')
    flags=[clang,'-std=gnu11','-O2','-ffp-contract=off','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out)]
    def compile(cmd,name):
        batch=out/(name+'.cmd');batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,cmd)))+'\n',encoding='utf-8');run(['cmd','/c',batch],name+'.log')
    compile(flags+['-shared','-x','c',out/'generated.inc','-o',out/'module.dll'],'compile_module')
    compile(flags+[HERE/'test.c',sdk/'src/melee/ft/native_callback_lifetime.c','-o',out/'test.exe'],'compile_test')
    result=run([out/'test.exe'],'run.log');m=re.search(r'PASS (\d+) original/generated pairs, (\d+) checks, (\d+) authored safety cases',result)
    if not m:raise ValueError('Missing receipt')
    snapshot=out/'namespace';snapshot.mkdir()
    inputs={}
    for path in HERE.iterdir():
        if path.is_file():shutil.copyfile(path,snapshot/path.name);inputs[str(path.relative_to(root))]=sha(path)
    for folder in (sdk,base/'compiler'):
        for path in folder.rglob('*'):
            if path.is_file(): inputs[str(path)]=sha(path)
    for path in (tool,base/'freeze.json'):
        inputs[str(path)]=sha(path)
    receipt=dict(pairs=int(m[1]),checks=int(m[2]),authored_safety_cases=int(m[3]),commands=commands,baseline=str(base),input_hashes=inputs,provenance=prov,scope='22 complete callbacks; no paired recruitment/rope/lifecycle/activation acceptance',outputs={str(p.relative_to(out)):sha(p) for p in out.rglob('*') if p.is_file()})
    (out/'result.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8');print(result.strip(),flush=True)
if __name__=='__main__':main()
