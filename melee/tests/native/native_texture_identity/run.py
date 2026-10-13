"""Texture identity owner and actual Aurora counter functions; no game/GPU launch."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve()
    if out.exists() or out.drive.upper()!='A:':parser.error('fresh A: output required')
    out.mkdir(parents=True)
    sys.path.insert(0,str(HERE.parent/'hsd_reservation'))
    from extract import function
    donor=ROOT.parent/'aurora/lib/dolphin/gx/GXTexture.cpp'
    source=donor.read_text()
    counter='#include <dolphin/gx/GXTexture.h>\n#include <stdlib.h>\n#define FATAL(...) abort()\nextern "C" {\nu32 next_tex_obj_id();\nu32 next_tlut_obj_id();\nextern u32 sNextTexObjId,sNextTlutObjId;\n}\nu32 sNextTexObjId=1,sNextTlutObjId=1;\n'
    counter+='\n'.join(function(source,n) for n in ['next_tex_obj_id','next_tlut_obj_id','GXGetTextureStateRegions'])
    (out/'counter.cpp').write_text(counter+'\n')
    sources=[HERE/'test.c',ROOT/'src/melee/gm/native_texture_identity_snapshot.c']
    inputs=sources+[donor,out/'counter.cpp',HERE.parent/'hsd_reservation/extract.py',Path(__file__).resolve()]
    inputs+=list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))
    compiler=args.compiler.resolve()/'clang.exe'
    inputs+=[compiler]
    sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
    inputs+=list((ROOT.parent/'aurora/include').rglob('*.h'))
    before={str(path):sha(path) for path in inputs}
    command=[str(compiler),'-std=gnu11','-O2','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS',
             '-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include')]+list(map(str,sources))+[str(out/'counter.obj'),'-o',str(out/'test.exe')]
    commands=[]
    for stage,cmd in [('counter',[str(compiler),'-std=c++17','-O2','-I'+str(ROOT.parent/'aurora/include'),'-c',str(out/'counter.cpp'),'-o',str(out/'counter.obj')]),('compile',command),('fixture',[str(out/'test.exe')])]:
        result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,
                              creationflags=subprocess.CREATE_NO_WINDOW)
        commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        (out/(stage+'.log')).write_text(result.stdout+result.stderr,encoding='utf-8')
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',encoding='utf-8')
        if result.returncode:raise RuntimeError(stage+' failed: '+(result.stdout+result.stderr)[-5000:])
    summary=json.loads(result.stdout)
    if any(not Path(path).is_file() or sha(Path(path))!=expected for path,expected in before.items()):
        raise RuntimeError('input changed during fixture')
    receipt={'status':'PASS','summary':summary,'commands':commands,'input_sha256':before,
             'limits':['actual serializer plus source-extracted Aurora identity increment and region-query function bodies with C ABI declarations for the fixture',
                       'idle predicates controlled; FATAL diagnostic abort leaf; no texture/GPU object construction or renderer cache test',
                       'full rendered restoration and complete state closure tested separately; no final latency claim']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({'receipt':str(out/'receipt.json'),'summary':summary}))


if __name__=='__main__':main()
