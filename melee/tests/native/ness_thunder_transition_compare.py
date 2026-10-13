"""Actual source-extracted common transition ordering/lifetime fixture; no game."""
import argparse,hashlib,json,subprocess
from pathlib import Path
from ness_thunder_stock_compare import function
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--sdk-root',type=Path,required=True);p.add_argument('--reference-root',type=Path,required=True);p.add_argument('--out-dir',type=Path,required=True);p.add_argument('--compiler',type=Path,required=True);a=p.parse_args();a.out_dir.mkdir(parents=True,exist_ok=True)
inputs=[]
for label,root in [('original',a.reference_root),('current',ROOT)]:
    fall=root/'src/melee/ft/kinds/ftCommon/ftCo_FallSpecial.c';down=root/'src/melee/ft/kinds/ftCommon/ftCo_DownBound.c';inputs.extend([fall,down])
    text='\n'.join(function(fall.read_text(),n) for n in ['inline0','ftCo_80096900','ftCo_800969D8'])+'\n'+'\n'.join(function(down.read_text(),n) for n in ['ftCo_8009794C','inlineA1','ftCo_80097D40'])
    if label=='original':
        for old,new in [('inline0','reference_inline0'),('ftCo_80096900','reference_freefall'),('ftCo_800969D8','reference_freefall_blend'),('ftCo_8009794C','reference_down_ground'),('inlineA1','reference_inlineA1'),('ftCo_80097D40','reference_down_bound')]:text=text.replace(old,new)
    (a.out_dir/f'common_transition_{label}.generated.h').write_text(text)
source=ROOT/'tests/native/ness_thunder_transition_test.c';inputs.extend([source,Path(__file__),a.compiler,a.sdk_root/'src/melee/ft/types.h'])
(a.out_dir/'source_manifest.json').write_text(json.dumps({str(x.resolve()):hashlib.sha256(x.read_bytes()).hexdigest() for x in inputs},indent=2)+'\n')
exe=a.out_dir/'ness_thunder_transition_test.exe'
cmd=[str(a.compiler),'-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt','-I'+str(a.sdk_root/'src'),'-I'+str(a.sdk_root/'extern/dolphin/include'),'-I'+str(a.out_dir),str(source),'-fuse-ld=lld','-o',str(exe)]
(a.out_dir/'command.json').write_text(json.dumps(cmd,indent=2)+'\n');r=subprocess.run(cmd,capture_output=True,text=True);(a.out_dir/'compile.log').write_text(r.stdout+r.stderr)
if r.returncode:print(r.stdout+r.stderr);raise SystemExit(r.returncode)
r=subprocess.run([str(exe)],capture_output=True,text=True);(a.out_dir/'comparison.log').write_text(r.stdout+r.stderr);print(r.stdout+r.stderr);raise SystemExit(r.returncode)
