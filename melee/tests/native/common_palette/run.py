"""Actual full common import and extracted typed query/material console checks."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent

def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()

def body(text, name):
    m = re.search(r'(?m)^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w\s*]*\b'+re.escape(name)+r'\s*\([^;{}]*\)\s*\{', text)
    if not m:
        raise ValueError('definition missing: '+name)
    depth, end = 1, m.end()
    while depth:
        depth += (text[end]=='{')-(text[end]=='}')
        end += 1
    return text[m.start():end]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--snapshot', type=Path, required=True)
    ap.add_argument('--source-root', type=Path, default=ROOT)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--iso', type=Path, default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso')
    a=ap.parse_args(); snap,root,out=a.snapshot.resolve(),a.source_root.resolve(),a.output.resolve()
    if out.exists() or snap == out or snap in out.parents or HERE == out or HERE in out.parents:
        ap.error('fresh output outside inputs required')
    frozen=json.loads((snap/'freeze.json').read_text())
    def verify():
        for name,expected in frozen.items():
            if name.replace('\\','/').startswith(('sdk/','compiler/')) and sha(snap/name)!=expected:
                raise RuntimeError('frozen input changed: '+name)
    verify(); out.mkdir(parents=True); sdk=snap/'sdk'; inputs={}; cmds=[]
    # Copy frozen adjacent headers so their relative "types.h" includes use
    # only the explicitly owned field-name/type overlay, never live SDK headers.
    overlay=out/'overlay/melee/ft';overlay.mkdir(parents=True)
    for p in (sdk/'src/melee/ft').glob('*.h'):
        shutil.copy2(p,overlay/p.name);inputs[str(p)]=sha(p)
    for name in ['types.h','ftlib.h']:
        p=root/'src/melee/ft'/name;shutil.copy2(p,overlay/name);inputs[str(p)]=sha(p)
    # Whole actual importer, with only local include resolution made explicit.
    imports=[]
    for old in [False,True]:
        p=(sdk if old else root)/'src/melee/ft/ftcommon_assets.c';text=p.read_text();inputs[str(p)]=sha(p)
        text=text.replace('#include "ftcommon_assets.h"','#include <melee/ft/ftcommon_assets.h>')
        if old:text=text.replace('bool ftCommonAssets_Import(', 'bool palette_legacy_import(',1)
        dest=out/('legacy_import.c' if old else 'actual_import.c');dest.write_text(text,encoding='utf-8',newline='\n');imports.append(dest)
    current=(root/'src/melee/ft/ftmaterial_tev.c').read_text()
    constants=current[current.index('static HSD_TevDesc'):current.index('HSD_TExp* ftMaterial_800BF534')]
    blocks=[constants]
    for base,rel,name,rename in [(root,'ftlib.c','ftLib_800870BC',None),
            (root,'ftmaterial_tev.c','ftMaterial_800BF6BC',None),
            (sdk,'ftmaterial_tev.c','ftMaterial_800BF6BC','palette_legacy_material')]:
        p=base/'src/melee/ft'/rel;text=p.read_text();inputs[str(p)]=sha(p);b=body(text,name)
        if rename:b=b.replace('void '+name+'(', 'void '+rename+'(',1)
        blocks.append(b)
    generated=out/'query_material_bodies.h';generated.write_text('\n\n'.join(blocks)+'\n',encoding='utf-8',newline='\n')
    sys.path.insert(0,str(sdk/'tools'));from extract_native_animation import extract_clip
    dat,root_offset=extract_clip(a.iso,'PlCo.dat','ftLoadCommonData')
    datfile=out/'PlCo.dat';datfile.write_bytes(dat)
    units=['src/melee/ft/ftdat.c','src/melee/ft/ftcpu_data.c','src/melee/lb/lbcolanim_data.c']
    units += ['src/sysdolphin/baselib/'+n+'.c' for n in ['dat','dat_import','dat_material','dat_texture','dat_animation','dat_model','dat_scene','dat_geometry']]
    for name in units:inputs[str(sdk/name)]=sha(sdk/name)
    clang=snap/'compiler/clang.exe';exe=out/'common_palette_test.exe'
    command=[str(clang),'-std=gnu11','-O2','-ffp-contract=off','-fuse-ld=lld',
        '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES',
        '-I'+str(out/'overlay'),'-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(out),
        str(HERE/'test.c'),*map(str,imports),*[str(sdk/n) for n in units],'-o',str(exe)]
    for stage,command in [('compile',command),('run',[str(exe),str(datfile)])]:
        p=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120)
        (out/(stage+'.log')).write_text(p.stdout+p.stderr,encoding='utf-8',newline='\n')
        cmds.append({'stage':stage,'command':command,'exit':p.returncode})
        (out/'commands.json').write_text(json.dumps(cmds,indent=2)+'\n',encoding='utf-8',newline='\n')
        if p.returncode:raise RuntimeError(stage+' failed: '+p.stdout+p.stderr)
        if stage=='run':result=json.loads(p.stdout.strip())
    verify();inputs.update({str(p):sha(p) for p in [HERE/'run.py',HERE/'test.c',generated,*imports]})
    receipt={'status':'PASS scoped actual-source/import; no GX/game acceptance','snapshot_manifest_sha256':sha(snap/'freeze.json'),
        'original_dat_sha256':hashlib.sha256(dat).hexdigest(),'root_offset':root_offset,'result':result,'inputs':inputs,'commands':cmds,
        'boundaries':['HSD material/TEV register and stage sinks controlled; actual complete material selector/blending body executes',
        'actual full common importer and DAT/graph/CPU/color transcoders execute; no allocator/model/GX execution',
        'legacy importer demonstrates original byte-order defect; healthy legacy material body executes only valid selectors with corrected byte inputs',
        'type/header overlay copies frozen adjacent headers plus owned types.h/ftlib.h; no live shared-header dependency']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8',newline='\n');print(json.dumps(result))
if __name__=='__main__':main()
