"""Compile a standalone production-body/generated-module results-pill comparison.

Requires an already built project and matching frozen SDK/compiler. No game or
global build is started; all generated extraction and evidence stay in --batch.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def excerpts(batch, sdk):
    spec = importlib.util.spec_from_file_location('extract', ROOT / 'tests/native/callback_engine_fixture.py')
    extract = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(extract)
    groups = {
        'src/melee/ft/ft_0BEC.c': ['DocHelper'],
        'src/melee/ft/kinds/ftMario/ftmariospecialn.c': ['pickMegavitamin', 'ftMr_SpecialN_VitaminRandom'],
        'src/melee/it/kinds/itdrmariopill.c': ['itDrMarioPill_802C1180', 'itDrMarioPill_802C061C', 'itDrMarioPill_802C09C4', 'copy_jobj_scale', 'itDrMarioPill_Motion6_Anim', 'itDrMarioPill_Motion6_Phys', 'itDrMarioPill_Motion6_Coll'],
        'src/melee/ft/ft_0BF0.c': ['ftCo_800BF228'],
        'src/melee/it/it_2725.c': ['it_802750F8'],
        'src/melee/ft/ftlib.c': ['ftLib_8008741C', 'ftLib_80087460'],
        'src/melee/ft/native_fighter_target.c': ['nativeFighterTarget_Id'],
        'src/melee/it/kinds/inlines.h': ['Item_InitSpawn'],
        'src/melee/it/native_item_owner.c': ['nativeItem_Creator', 'find_live_item', 'find_created_item', 'nativeItem_BindFighter'],
        'src/melee/it/native_article.c': ['nativeArticle_Creator', 'article_logic', 'nativeArticle_Create'],
        'src/melee/it/item.c': ['Item_8026A810'],
    }
    pieces, facts = [], {}
    # Prototypes precede extracted callers; source bodies stay unmodified.
    pieces.append('int ftMr_SpecialN_VitaminRandom(HSD_GObj*);\nItem_GObj* itDrMarioPill_802C09C4(Fighter_GObj*,Vec3*,s32,ItemKind,s32,Fighter_Part,f32);\nbool ftCo_800BF228(Fighter_GObj*);\nvoid it_802750F8(Item_GObj*);')
    for path, names in groups.items():
        source = (sdk / path).read_text()
        facts[path] = hashlib.sha256((sdk / path).read_bytes()).hexdigest()
        for name in names:
            pieces.append('// Actual ' + path + '\n' + extract.function(source, name))
    (batch / 'drmario_results_original.inc').write_text('\n\n'.join(pieces)+'\n')
    return facts


def unused_host(batch, sdk, project):
    required = set(re.findall(r'host->(\w+) == 0', (project / 'behavior.generated.c').read_text()))
    provided = {'callback_source_alive', 'random_int_excluding', 'create_article_at',
        'signal_owned_article', 'bind_fighter_item', 'item_creator', 'item_change_state_flags',
        'item_set_lifetime', 'item_use_manual_lifetime', 'item_set_appearance', 'item_reset_spin',
        'item_refresh_physics', 'item_scale_by_attribute', 'item_set_uniform_scale',
        'fighter_root_scale_y', 'fighter_incapacitated', 'item_joint_value', 'item_set_joint_value'}
    text = (sdk/'src/melee/ft/native_fighter_module.h').read_text().split('typedef struct NativeFighterHost {',1)[1].split('} NativeFighterHost;',1)[0]
    signatures = {m[2]:(m[1].strip(),m[3].strip()) for m in re.finditer(r'^\s*([^;\n]+?)\(\*(\w+)\)\((.*?)\);',text,re.M|re.S)}
    for name in re.findall(r'^\s*HSD_GObjEvent (\w+);',text,re.M): signatures[name]=('void','HSD_GObj*')
    code, assignments = [], []
    for name in sorted(required-provided):
        result, arguments = signatures[name]
        parts, depth, begin = [],0,0
        for at,ch in enumerate(arguments):
            depth += (ch=='(')-(ch==')')
            if ch==',' and depth==0: parts.append(arguments[begin:at].strip());begin=at+1
        parts.append(arguments[begin:].strip())
        named=[]
        for i,arg in enumerate(parts):
            named.append(arg if arg in ('void','...') else arg.replace('(*)','(*unused_'+str(i)+')') if '(*)' in arg else arg+' unused_'+str(i))
        code.append('static '+result+' trap_'+name+'('+','.join(named)+') { require(false,"Unexpected Host '+name+'"); abort(); }\n')
        assignments.append('host->'+name+'=trap_'+name+';')
    (batch/'drmario_results_unused_host.inc').write_text(''.join(code)+'\nstatic void supply_unused_host(NativeFighterHost* host) {\n'+'\n'.join(assignments)+'\n}\n')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--batch',required=True,type=Path)
    p.add_argument('--snapshot',required=True,type=Path)
    p.add_argument('--project',required=True,type=Path)
    a=p.parse_args();batch=a.batch.resolve();snapshot=a.snapshot.resolve();project=a.project.resolve()
    batch.mkdir(parents=True,exist_ok=True);sdk=snapshot/'sdk'
    facts=excerpts(batch,sdk);unused_host(batch,sdk,project)
    exe=batch/'drmario_results_stock_test.exe'
    flags=[str(snapshot/'compiler/clang.exe'),'-fms-runtime-lib=dll','-std=gnu11','-O2',
        '-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT',
        '-I'+str(sdk/'src'),'-I'+str(sdk/'extern/dolphin/include'),'-I'+str(batch)]
    module=batch/'drmario_results_generated.dll'
    module_command=flags+['-shared',str(project/'behavior.generated.c'),'-o',str(module)]
    command=flags+[
        str(ROOT/'tests/native/drmario_results_stock_test.c'),
        str(sdk/'src/melee/ft/native_fighter_parameters.c'),str(sdk/'src/melee/ft/native_callback_lifetime.c'),str(ROOT/'tests/native/model_effect_absent_fixture.c'),'-o',str(exe)]
    setup=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Auxiliary/Build/vcvars64.bat')
    script=batch/'compile-results-fixture.cmd'
    script.write_text('@echo off\ncall '+subprocess.list2cmdline([str(setup)])+' -vcvars_ver=14.44 >nul\n'+subprocess.list2cmdline(module_command)+'\nif errorlevel 1 exit /b 1\n'+subprocess.list2cmdline(command)+'\n')
    with (batch/'build.log').open('wb') as log:
        r=subprocess.run(['cmd','/c',str(script)],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
    if r.returncode: print((batch/'build.log').read_text());return r.returncode
    r=subprocess.run([str(exe),str(module),str(project/'fighter_parameters.fmp')],cwd=ROOT,text=True,capture_output=True)
    output=r.stdout+r.stderr;(batch/'result.log').write_text(output)
    paths=[ROOT/'tests/native/drmario_results_stock_compare.py',ROOT/'tests/native/drmario_results_stock_test.c',batch/'drmario_results_original.inc',module,project/'behavior.generated.c',project/'fighter_parameters.fmp',project/'behavior.fms',snapshot/'freeze.json',snapshot/'compiler/clang.exe']
    (batch/'result.json').write_text(json.dumps(dict(snapshot=str(snapshot),exit_code=r.returncode,output=output,source_sha256=facts,sha256={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}),indent=2)+'\n')
    print(output,end='');return r.returncode


if __name__=='__main__': raise SystemExit(main())
