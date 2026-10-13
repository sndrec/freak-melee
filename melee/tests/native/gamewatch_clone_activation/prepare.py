"""Normal clone/import/DLL/package ownership gate; no game or human installation.

Requires a frozen snapshot with the real Game & Watch copied-base registry.
Use --install-scratch only for a store inside this fresh test output.
"""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import sys
from pathlib import Path
sys.dont_write_bytecode=True
from assemble import FIGHTER_FIELDS, FOOD_FIELDS, sha

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
sys.dont_write_bytecode=True
sys.path.insert(0,str(HERE.parent))
from fighter_script_bundle_roundtrip import portrait, bundled_id


def declaration(text, marker):
    start=text.index(marker)
    i=text.index('{',start)
    depth,j=1,i+1
    while depth:
        depth+=(text[j]=='{')-(text[j]=='}')
        j+=1
    return text[start:text.index(';',j)+1]


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--snapshot',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--candidate',type=Path,default=ROOT/'examples/gamewatch_full.fms')
    ap.add_argument('--iso',type=Path,default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso')
    ap.add_argument('--install-scratch',action='store_true')
    a=ap.parse_args()
    snap,out=a.snapshot.resolve(),a.output.resolve()
    if out.exists() or any(out==p or p in out.parents for p in (snap,HERE)):
        ap.error('fresh output outside immutable inputs required')
    manifest=json.loads((snap/'freeze.json').read_text())
    def verify():
        for name,digest in manifest.items():
            if sha(snap/name)!=digest: raise ValueError('changed snapshot '+name)
    verify()
    sdk=snap/'sdk'
    base_path=sdk/'src/melee/ft/kinds/ftGameWatch/ftgamewatch_clone.c'
    if not base_path.exists(): ap.error('snapshot does not contain the real copied Game & Watch base')
    out.mkdir(parents=True)
    commands=[]
    def run(stage,command):
        command=list(map(str,command))
        r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr,encoding='utf-8',newline='\n')
        commands.append({'stage':stage,'command':command,'exit':r.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n',newline='\n')
        if r.returncode: raise RuntimeError(stage+' failed:\n'+(r.stdout+r.stderr)[-6000:])
        return r.stdout
    tool=snap/'bin/native_content_tool.exe'
    project=out/'project'
    run('clone',[tool,'clone-stock',a.iso,'Game & Watch',project,'GameWatch ownership gate'])
    candidate=out/'candidate.fms'
    candidate.write_bytes(a.candidate.read_bytes())
    run('import',[tool,'import-parameters',project,candidate,project/'behavior.fms'])
    # Compare every imported field directly against retail source bytes from
    # the real clone, including packed RGBA and the scoped Chef attribute block.
    raw=(project/'fighter.dat').read_bytes()
    sys.path.insert(0,str(sdk/'tools'))
    from fighter_animation_io import Dat
    dat=Dat(raw)
    root=dat.roots['ftDataGamewatch']
    tuning=dat.u32(root+4)
    slots=dat.u32(root+72)
    chef=dat.u32(slots+8*4)
    chef_tuning=dat.u32(chef+4)
    params={}
    for row in (project/'fighter_parameters.fmp').read_text().splitlines():
        kind,name,equal,value=row.split()
        assert equal=='='
        params[name]=(kind,value)
    comparisons=[]
    for name,(kind,offset) in FIGHTER_FIELDS.items():
        at=32+tuning+offset
        expected=raw[at] if kind=='byte' else struct.unpack_from('>I',raw,at)[0]
        actual_kind,value=params[name]
        actual=(struct.unpack('<I',struct.pack('<f',float(value)))[0]
                if actual_kind=='float' else int(value)&0xFFFFFFFF)
        assert actual==expected,(name,actual,expected)
        comparisons.append(name)
    for name,offset in FOOD_FIELDS.items():
        _,value=params['It_Kind_GameWatch_Chef.'+name]
        actual=struct.unpack('<I',struct.pack('<f',float(value)))[0]
        expected=struct.unpack_from('>I',raw,32+chef_tuning+offset)[0]
        assert actual==expected,(name,actual,expected)
        comparisons.append('It_Kind_GameWatch_Chef.'+name)
    key=re.search(r'^code=(\S+)$',(project/'project.txt').read_text(),re.M)[1]
    generated=project/'behavior.generated.c'
    run('generate',[tool,'script-c',project/'behavior.fms',key,generated])
    code=generated.read_text()
    assert 'generated.source_kind = Ft_Kind_None;' in code
    for bad in ('fighter->u.gw','fighter->mv.gw','itemVar.gamewatch','ftGw_'):
        assert bad not in code,bad
    flags=[snap/'compiler/clang.exe','-std=gnu11','-O2','-ffp-contract=off',
           '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES',
           '-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-fuse-ld=lld']
    dll=project/'module.dll'
    run('dll',flags+['-shared',generated,'-o',dll])
    stock=(sdk/'src/melee/ft/kinds/ftGameWatch/ftgamewatch.c').read_text()
    table=declaration(stock,'MotionState ftGw_Init_MotionStateTable')
    base=base_path.read_text()
    declarations='\n'.join(declaration(base,prefix) for prefix in (
        'static const NativeFighterBone ftGameWatchClone_Bones',
        'static const NativeFighterArticleAsset ftGameWatchClone_Articles',
        'static const Fighter_DemoStrings ftGameWatchClone_DemoMotionNames',
        'const NativeFighterDefinition ftGameWatchClone_Definition'))
    pointers=sorted(set(re.findall(r'\bftGw_\w+\b',table+declarations)))
    pointers=[n for n in pointers if not any(part in n for part in ('_MS_','_MF_','_SM_','_MotionStateTable'))]
    pointers.append('ftCamera_UpdateCameraBox')
    (out/'base.h').write_text('\n'.join('#define '+n+' stock_slot' for n in pointers)+'\n'+table+'\n'+declarations+'\n'+'\n'.join('#undef '+n for n in pointers)+'\n',newline='\n')
    required=sorted(set(re.findall(r'host->(\w+)\s*==\s*0',code)))
    (out/'required_host.h').write_text(''.join('host.'+n+'=(__typeof__(host.'+n+'))unexpected;\n' for n in required if n!='common_motion_states'),newline='\n')
    header=(sdk/'src/melee/ft/native_fighter_runtime.h').read_text()
    definition=header.split('typedef struct NativeFighterDefinition {',1)[1].split('} NativeFighterDefinition;',1)[0]
    callbacks=sorted(set(re.findall(r'\(\*(\w+)\)',definition)+re.findall(r'\b(?:HSD_GObjEvent|HSD_GObjPredicate|Fighter_ItemEvent)\s+(\w+)\s*;',definition)))
    (out/'definition_callbacks.h').write_text(''.join('VERIFY_CALLBACK('+n+');\n' for n in callbacks),newline='\n')
    exe=out/'ownership.exe'
    run('ownership_compile',flags+['-I'+str(out),HERE/'ownership.c','-o',exe])
    ownership=json.loads(run('ownership',[exe,dll]))
    image=project/'portrait.png'
    portrait(image)
    store=out/'isolated-store'
    package=bundled_id(run('bundle',[tool,'--root',store,'bundle-project',project,image,a.iso]))
    run('verify',[tool,'--root',store,'verify-package',package,a.iso])
    if a.install_scratch:
        run('install_scratch',[tool,'--root',store,'install-project',project,package])
    verify()
    paths=[HERE/'assemble.py',HERE/'prepare.py',HERE/'ownership.c',base_path,
           a.candidate,candidate,generated,dll,out/'base.h',out/'required_host.h',
           out/'definition_callbacks.h',*[project/n for n in ('behavior.fms','fighter_parameters.fmp',
           'project.txt','fighter.dat','animation.dat','costume.dat','cpu_profile.fcp')]]
    receipt={'status':'PASS copied-base import/DLL/package ownership only; full normal-phase and sound policy closure pending',
             'snapshot':str(snap),'freeze_sha256':sha(snap/'freeze.json'),'commands':commands,
             'ownership':ownership,'retail_parameter_comparisons':comparisons,
             'package_id':package,'installed_only_in_test_store':a.install_scratch,
             'input_sha256':{str(p):sha(p) for p in paths}}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n')
    print(out/'receipt.json')


if __name__=='__main__':
    main()
