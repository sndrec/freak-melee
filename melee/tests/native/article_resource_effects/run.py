"""One console batch: real parser, generated routes, resource geometry/lifetime."""
import argparse, hashlib, json, re, subprocess, sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/'tests/native'))
from article_resource_geometry_source_fixture import extract

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--snapshot',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    a=ap.parse_args(); base=a.snapshot.resolve();out=a.output.resolve()
    if base==out or base in out.parents: ap.error('output cannot modify snapshot')
    out.mkdir(parents=True,exist_ok=False)
    # Live SDK headers are intentional for the new Host layout. Only the
    # compiler and original stock geometry bodies come from the snapshot.
    compiler=base/'compiler/clang.exe'
    script_text=(ROOT/'CMakeLists.txt').read_text(encoding='utf-8')
    block=re.search(r'set\(NATIVE_FIGHTER_SCRIPT_SOURCES\s+(.*?)\)',script_text,re.S).group(1)
    units=[ROOT/s for s in block.split()]
    inputs=set(units)|set(HERE.glob('*'))|{ROOT/'CMakeLists.txt',compiler,
        ROOT/'src/melee/it/native_article_resource.c',ROOT/'tests/native/article_resource_geometry_test.c',
        ROOT/'tests/native/article_resource_geometry_source_fixture.py',
        ROOT/'src/melee/lb/native_model_joint.c',ROOT/'src/melee/ft/native_callback_lifetime.c',
        ROOT/'src/melee/ft/native_fighter_parameters.c',ROOT/'src/melee/ef/efasync.c',ROOT/'src/melee/ef/efsync.c'}
    inputs.update((ROOT/'src').rglob('*.h'));inputs.update((ROOT/'tools').glob('*.h'))
    inputs.update((ROOT/'extern/dolphin/include').rglob('*.h'))
    originals=extract(base/'sdk',out/'resource_geometry_originals.h')
    position=json.loads((out/'resource_effect_payloads.json').read_text(encoding='utf-8'))
    inputs.update(base/'sdk/src'/s for s in originals)
    hashes={str(p):sha(p) for p in sorted(inputs) if p.is_file()}
    commands=[]
    def run(cmd,name):
        cmd=list(map(str,cmd));commands.append(cmd)
        result=subprocess.run(cmd,cwd=ROOT,text=True,encoding='utf-8',errors='replace',capture_output=True)
        (out/(name+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8',newline='\n')
        if result.returncode:raise RuntimeError(name+' failed:\n'+(result.stdout+result.stderr)[-10000:])
        return result.stdout
    flags=[compiler,'-std=gnu11','-O2','-fms-runtime-lib=dll','-DMELEE_NATIVE=1','-D_DLL','-D_MT',
        '-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),
        '-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
    def compile(cmd,name):
        batch=out/(name+'.cmd')
        batch.write_text('@echo off\ncall "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat" >nul\n'+subprocess.list2cmdline(list(map(str,cmd)))+'\n',encoding='utf-8',newline='\n')
        run(['cmd','/c',batch],name)
    compile([*flags,HERE/'compiler.c',*units,ROOT/'src/melee/ft/native_fighter_parameters.c','-o',out/'compiler.exe'],'compiler_build')
    summaries={'compiler':json.loads(run([out/'compiler.exe',out/'action.c',out/'callback.c'],'compiler'))}
    for route in ['action','callback']:
        compile([*flags,'-DRESOURCE_EFFECT_MODULE="'+str(out/(route+'.c')).replace('\\','/')+'"',HERE/'module.c','-o',out/(route+'.exe')],route+'_build')
        summaries[route]=json.loads(run([out/(route+'.exe')],route))
    compile([*flags,ROOT/'tests/native/article_resource_geometry_test.c',ROOT/'src/melee/it/native_article_resource.c',
        ROOT/'src/melee/lb/native_model_joint.c',ROOT/'src/melee/ft/native_callback_lifetime.c','-o',out/'geometry.exe'],'geometry_build')
    summaries['geometry']=json.loads(run([out/'geometry.exe'],'geometry'))
    for p,h in hashes.items():
        if sha(Path(p))!=h:raise RuntimeError('Changed source during batch: '+p)
    receipt={'status':'PASS','compiler_snapshot':str(base),'snapshot_manifest_sha256':sha(base/'freeze.json'),
        'live_sdk':True,'inputs':hashes,'commands':commands,'results':summaries,
        'position_payload_cases':position,'payloads':{'0x3F3':'efAsync_Dispatch: Vec3* -> generator 0xB','0x41C':'efAsync_Dispatch: Vec3* -> generator 0x5D',
                    '0x3F1':'efAsync_Dispatch: Vec3*,float* -> generator 0x14B with negated facing'},
        'limits':['Real parser/generated action+callback routes, live resource/joint/registry and original geometry helper bodies.',
                  'efSync allocation/dispatch, HSD allocation and ECB responses controlled; no GX, particles or gameplay acceptance.',
                  'Generated module Host effect boundary validates node arguments separately from real resource backend matrix.',
                  'Only source-backed independent-generator signatures are allowed; joint/model/unknown IDs reject before dispatch.']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n')
    print(json.dumps({'receipt':str(out/'receipt.json'),'results':summaries}))
if __name__=='__main__':main()
