"""Actual projection and quake functions with ASan, no GPU or game launch."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
sys.dont_write_bytecode=True
sys.path.insert(0,str(HERE.parent/'hsd_reservation'))
from extract import function

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    out=args.output.resolve()
    if out.exists() or out.drive.upper()!='A:': parser.error('fresh A: output required')
    out.mkdir(parents=True)
    aurora=ROOT.parent/'aurora'
    vector=ROOT/'src/melee/lb/lbvector.c'
    camera=ROOT/'src/melee/cm/camera.c'
    sdk=aurora/'lib/dolphin/mtx'
    gx=aurora/'lib/dolphin/gx/GXTransform.cpp'
    includes='#include <assert.h>\n#include <math.h>\n#include <melee/lb/lbvector.h>\n#include <dolphin/gx/GXTransform.h>\n#include <sysdolphin/baselib/cobj.h>\n#include <sysdolphin/baselib/debug.h>\n'
    text=vector.read_text()
    (out/'projection.c').write_text(includes+function(text,'lbVector_WorldToScreen')+'\n')
    # A frozen undersized buffer proves the sanitizer reaches the reported bug.
    original=function(text,'lbVector_WorldToScreen').replace('Mtx44 projMtx;','Mtx projMtx;')
    assert original!=function(text,'lbVector_WorldToScreen')
    (out/'negative.c').write_text(includes+original+'\n')
    # The two SDK header trees differ in const qualification, not the C ABI.
    # Compile Aurora's body with its own headers, just as the client does.
    (out/'gx.c').write_text('#include <dolphin/mtx.h>\n'+function(gx.read_text(),'GXProject')+'\n')
    quake='#include <melee/cm/camera.h>\n#include <melee/cm/types.h>\n#include <math.h>\nCamera game_camera;\nCameraUnkGlobals cm_803BCCA0;\nHSD_CameraDescPerspective cm_803BCB64;\nint gm_IsCurrently1PMode_inline(void);\nfloat Stage_GetCamZoomRate(void);\nfloat Stage_GetCamMaxDepth(void);\n'
    quake+=function(camera.read_text(),'Camera_ApplyQuake')+'\n'
    quake+=function((ROOT/'src/melee/cm/camera_state.c').read_text(),'Camera_80030DE4')+'\n'
    (out/'quake.c').write_text(quake)
    units=[sdk/'mtx.c',sdk/'mtx44.c',sdk/'vec.c']
    flags=['-O1','-g','-fsanitize=address','-fno-inline','-ffp-contract=off','-ffunction-sections','-fdata-sections']
    compiler=str(args.compiler.resolve())
    inputs=[Path(__file__),HERE/'test.c',HERE.parent/'hsd_reservation/extract.py',vector,camera,ROOT/'src/melee/cm/camera_state.c',gx,*units]
    inputs+=sorted((ROOT/'src').rglob('*.h'))+sorted((ROOT/'extern/dolphin/include').rglob('*.h'))+sorted((aurora/'include/dolphin').rglob('*.h'))
    sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
    receipt=dict(status='RUNNING',source_sha256={str(p):sha(p) for p in inputs},commands=[],limits=[
        'Exact extracted projection/quake/GXProject bodies and actual Aurora matrix/vector source.',
        'Controlled camera accessors and stage constants; no full CObj lifecycle or live restore.',
        'Negative control intentionally uses the old undersized projection buffer.'])
    env=os.environ.copy()
    dll=next(args.compiler.resolve().parents[1].glob('lib/clang/*/lib/windows/clang_rt.asan_dynamic-x86_64.dll'))
    env['PATH']=str(dll.parent)+';'+env['PATH']
    def run(name,command,expected=0):
        r=subprocess.run(command,cwd=ROOT,env=env,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        log=r.stdout+r.stderr; (out/(name+'.log')).write_text(log)
        receipt['commands'].append(dict(name=name,command=command,exit=r.returncode,expected=expected,log_sha256=sha(out/(name+'.log'))))
        if expected==0: assert r.returncode==0,(name,log[-3000:])
        else: assert r.returncode!=0 and 'AddressSanitizer: stack-buffer-overflow' in log and 'C_MTXPerspective' in log,(name,log[-3000:])
        return log
    sdk_objects=[]
    for unit in [*units,out/'gx.c']:
        obj=out/(unit.stem+'.obj'); sdk_objects.append(str(obj))
        run('compile-'+unit.stem,[compiler,*flags,'-I'+str(aurora/'include'),'-c',str(unit),'-o',str(obj)])
    native=['-DMELEE_NATIVE','-DDEBUG','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include')]
    for label,generated in [('negative',[out/'negative.c']),('correct',[out/'projection.c',out/'quake.c'])]:
        extra=['-DPROJECTION_NEGATIVE_CONTROL'] if label=='negative' else []
        exe=out/(label+'.exe')
        run('build-'+label,[compiler,*flags,*native,*extra,str(HERE/'test.c'),*[str(p) for p in generated],*sdk_objects,'-o',str(exe),'-Wl,/OPT:REF'])
        log=run(label,[str(exe)],expected=1 if label=='negative' else 0)
        if label=='correct': assert '75 checks passed' in log,log
    assert all(sha(Path(p))==digest for p,digest in receipt['source_sha256'].items())
    receipt['status']='PASS'
    receipt['generated_sha256']={str(p):sha(p) for p in out.glob('*') if p.suffix in ('.c','.exe','.obj')}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print('PASS: actual projection/quake, ASan negative control and 75 numeric checks')

if __name__=='__main__': main()
