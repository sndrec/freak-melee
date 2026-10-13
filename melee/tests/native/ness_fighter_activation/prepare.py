"""Prepare and validate an unregistered independent Ness package; never install/run."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

HERE=Path(__file__).resolve().parent
COMPOSITION=HERE.parent/'ness_fighter_composition'
sys.path.insert(0,str(COMPOSITION))
from compose import compose
from census import census
from run import prepare_comparisons, run, sha
sys.path.insert(0,str(HERE.parent))
from fighter_script_bundle_roundtrip import portrait, bundled_id


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',required=True,type=Path)
    p.add_argument('--iso',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    args=p.parse_args()
    snapshot,iso,out=args.snapshot.resolve(),args.iso.resolve(),args.output.resolve()
    if out.exists(): raise RuntimeError('Use a new output directory.')
    if any(root==out or root in out.parents for root in [snapshot,HERE,COMPOSITION]):
        raise RuntimeError('Output must not modify inputs.')
    manifest=json.loads((snapshot/'freeze.json').read_text())
    for name,expected in manifest.items():
        name=name.replace('\\','/')
        if name.startswith(('sdk/','compiler/','bin/')) and sha(snapshot/name)!=expected:
            raise RuntimeError('Changed snapshot input: '+name)
    text=compose(out)
    text=text.replace('# UNREGISTERED full resource/move/common composition.\n'
        '# Do not activate or declare independent before closure acceptance.\n',
        '# UNREGISTERED independent activation candidate; console gate only.\n'
        'fighter:\n    behavior independent\n\n',1)
    if '    behavior independent\n' not in text: raise RuntimeError('Missing directive')
    candidate=out/'candidate.fms'; candidate.write_text(text,encoding='utf-8')
    original_census=census(snapshot,out)
    tool=snapshot/'bin/native_content_tool.exe';project=out/'project'
    run([str(tool),'clone-stock',str(iso),'Ness',str(project),
         'Unregistered independent Ness'],out/'clone.log')
    run([str(tool),'import-parameters',str(project),str(candidate),str(project/'behavior.fms')],out/'import.log')
    key=re.search(r'(?m)^code=(\S+)$',(project/'project.txt').read_text())[1]
    generated=project/'behavior.generated.c'; module=project/'module.dll'
    run([str(tool),'script-c',str(project/'behavior.fms'),key,str(generated)],out/'generate.log')
    code=generated.read_text()
    if 'generated.source_kind = Ft_Kind_None;' not in code: raise RuntimeError('Lost independence')
    prepare_comparisons(snapshot,out,code)
    # Enumerate every Definition callback field from this exact SDK, not a
    # hardcoded subset. Nested immutable article/model metadata is audited too.
    header=(snapshot/'sdk/src/melee/ft/native_fighter_runtime.h').read_text()
    definition=header.split('typedef struct NativeFighterDefinition {',1)[1].split('} NativeFighterDefinition;',1)[0]
    callbacks=re.findall(r'\(\*(\w+)\)',definition)+re.findall(r'\b(?:HSD_GObjEvent|HSD_GObjPredicate|Fighter_ItemEvent)\s+(\w+)\s*;',definition)
    callbacks=sorted(set(callbacks))
    (out/'definition_callbacks.h').write_text(''.join('VERIFY_CALLBACK('+name+');\n' for name in callbacks))
    vswhere=Path(os.environ.get('ProgramFiles(x86)',r'C:\Program Files (x86)'))/'Microsoft Visual Studio/Installer/vswhere.exe'
    installation=subprocess.run([str(vswhere),'-latest','-products','*','-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64','-property','installationPath'],
        text=True,capture_output=True,check=True).stdout.strip()
    setup=Path(installation)/'VC/Auxiliary/Build/vcvars64.bat';clang=snapshot/'compiler/clang.exe'
    flags=[str(clang),'-std=gnu11','-O2','-ffp-contract=off','-DMELEE_NATIVE=1',
        '-D_CRT_SECURE_NO_WARNINGS','-D_DLL','-D_MT','-Xclang','--dependent-lib=msvcrt',
        '-I'+str(snapshot/'sdk/src'),'-I'+str(snapshot/'sdk/extern/dolphin/include')]
    commands=[flags+['-shared','-fuse-ld=lld-link',str(generated),'-o',str(module)],
        flags+['-I'+str(out),str(HERE/'ownership.c'),'-fuse-ld=lld-link','-Wl,/OPT:REF','-o',str(out/'ownership.exe')]]
    batch=out/'compile.cmd'; batch.write_text('@echo off\ncall "'+str(setup)+'" -vcvars_ver=14.44 >nul\n'
        'if errorlevel 1 exit /b 1\n'+''.join(subprocess.list2cmdline(c)+'\nif errorlevel 1 exit /b 1\n' for c in commands))
    run(['cmd.exe','/d','/c',str(batch)],out/'compile.log')
    result=run([str(out/'ownership.exe'),str(module)],out/'ownership.log')
    if '0 failures' not in result: raise RuntimeError(result)
    from audit import audit
    source_audit=audit(snapshot,out,candidate)
    image=out/'portrait.png';portrait(image);store=out/'isolated-store'
    package=bundled_id(run([str(tool),'--root',str(store),'bundle-project',str(project),str(image),str(iso)],out/'bundle.log'))
    run([str(tool),'--root',str(store),'verify-package',package,str(iso)],out/'verify.log')
    receipt={'status':'independent console/package validation; unregistered, never installed, no gameplay acceptance',
        'snapshot':str(snapshot),'snapshot_sha256':sha(snapshot/'freeze.json'),'tool_sha256':sha(tool),
        'clang_sha256':sha(clang),'iso_sha256':sha(iso),'code_key':key,'package_id':package,
        'candidate_sha256':sha(candidate),'imported_sha256':sha(project/'behavior.fms'),
        'generated_sha256':sha(generated),'module_sha256':sha(module),
        'parameters_sha256':sha(project/'fighter_parameters.fmp'),
        'project_asset_sha256':{name:sha(project/name) for name in
            ['fighter.dat','animation.dat','costume.dat','cpu_profile.fcp']},
        'project_metadata':(project/'project.txt').read_text(),
        'stock_gameplay_phases':original_census['nonnull_gameplay_phases'],
        'audit_sha256':sha(out/'audit.json'),
        'generic_contract_gaps':source_audit['remaining_generic_contract_gaps'],
        'definition_callback_fields':callbacks,'ownership_result':result.strip(),
        'component_inputs':json.loads((out/'component_inputs.json').read_text()),
        'fixture_inputs':{p.name:sha(p) for p in HERE.iterdir() if p.is_file()}}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt,indent=2))


if __name__=='__main__': main()
