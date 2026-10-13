"""Bounded hidden actual-import/native-topology console; no global build/game."""
import argparse, hashlib, json, re, shutil, struct, subprocess
from pathlib import Path
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--compiler',type=Path,required=True)
    p.add_argument('--iso',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();out=a.output.resolve()
    if out.drive.upper()!='A:' or out.exists():p.error('fresh private A: output required')
    out.mkdir(parents=True);assets=out/'retail';assets.mkdir();build=ROOT/'build/native'
    dat_hashes={}
    with a.iso.open('rb') as f:
        f.seek(0x424);at,size=struct.unpack('>II',f.read(8));f.seek(at);fst=f.read(size)
        count=struct.unpack_from('>I',fst,8)[0];names=fst[count*12:]
        for i in range(1,count):
            tag,offset,size=struct.unpack_from('>III',fst,i*12)
            if tag>>24:continue
            n=tag&0xffffff;name=names[n:names.find(b'\0',n)].decode()
            if not (re.fullmatch(r'(?:Pl[^.]{2}|Gr[^.]+)\.dat',name) or name=='ItCo.usd'):continue
            f.seek(offset);data=f.read(size);(assets/name).write_bytes(data)
            dat_hashes[name]=hashlib.sha256(data).hexdigest()
    ninja=(build/'build.ninja').read_text();start=ninja.index('build native_source_assets_test.exe:')
    block=ninja[start:ninja.index('\n\n',start)]
    libs=re.search(r'^  LINK_LIBRARIES = (.+)$',block,re.M).group(1).split()
    # Current owned collector/caller translation units are compiled directly,
    # avoiding dependence on an old archive's ItScriptEntry layout. Other actual
    # HSD construction/import/execution dependencies come from existing libs.
    stage_source=ROOT/'src/melee/gr/grdatfiles.c'
    stage_text=stage_source.read_text()
    (out/'stage_options.inc').write_text(stage_text[stage_text.index('typedef struct NativeStageArchiveSpec'):stage_text.index('static GrStageAssets* grDatFiles_ImportNativeAssets')])
    units=[HERE/'test.c',ROOT/'src/melee/it/native_item_model_admission.c',ROOT/'src/melee/it/itarticle_data.c',
           ROOT/'src/melee/it/itcommon_data.c',ROOT/'src/melee/ft/ftcharacter_assets.c',ROOT/'src/melee/gr/grstage_data.c']
    paths=[build/x for x in libs if x.endswith('.lib') and not x.startswith('-')]
    inputs=[stage_source,a.compiler.resolve(),HERE/'run.py',ROOT/'src/melee/it/itcommand_data.c',*units,
            build/'build.ninja',*paths,*list((ROOT/'src').rglob('*.h'))]
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    hashes={str(path):sha(path) for path in inputs if path.exists()}
    compile=[str(a.compiler.resolve()),'-std=gnu11','-O2','-DMELEE_NATIVE=1',
        '-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt',
        '-fuse-ld=lld','-Wl,/opt:ref','-I'+str(out),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),
        *map(str,units),*libs,'-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd,cwd in [('compile',compile,build),('test',[str(out/'test.exe'),str(assets)],out)]:
        if stage=='test':
            for dll in build.glob('*.dll'):shutil.copy2(dll,out/dll.name)
        r=subprocess.run(cmd,cwd=cwd,capture_output=True,text=True,timeout=120,
                         creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr)
        commands.append({'stage':stage,'command':cmd,'exit':r.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if r.returncode:raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-12000:])
    if any(sha(Path(path))!=value for path,value in hashes.items()):
        raise RuntimeError('source/header/library inputs changed during console proof')
    results=[json.loads(x) for x in r.stdout.splitlines() if x.startswith('{')]
    receipt={'status':'PASS','results':results,'retail_sha256':dat_hashes,
             'input_sha256':hashes,'commands':commands,
             'scope':'Actual owned importer and actual HSD JObj/common command construction/execution; bounded private native heap, no render/game or global build. Includes all retail stock fighter archives and ItCo.usd.'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(results[-1]))

if __name__=='__main__':main()
