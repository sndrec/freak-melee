"""Synchronous two-joint effect source/VM and unchanged native construction contracts."""
import argparse, hashlib, importlib.util, json, re, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
REFERENCE = '7a3cf230622b78f92c4f9043d1aac77a50ff61a7'
spec = importlib.util.spec_from_file_location('feedback_extract', HERE.parent/'script_form_feedback/extract.py')
extractor = importlib.util.module_from_spec(spec); spec.loader.exec_module(extractor)
body = extractor.body

def main():
    p = argparse.ArgumentParser(); p.add_argument('--compiler',type=Path,required=True); p.add_argument('--output',type=Path,required=True)
    a = p.parse_args(); out = a.output.resolve()
    if out.exists(): p.error('fresh private A: output required')
    out.mkdir(parents=True)
    hashes = {}
    catalog_path=ROOT/'src/melee/ft/native_script_bindings.c'; catalog=catalog_path.read_text()
    start=catalog.index('#define F '); end=catalog.index('\n};',catalog.index('static const NativeScriptBindingDescriptor catalog'))+3
    props=ROOT/'src/melee/ft/native_script_binding_properties.c'
    guards='\n'.join(re.sub(r'\berror\b','form_guard_error',body(props.read_text(),n)) for n in ('error','nativeScriptBindings_SourceAlive','nativeScriptBindings_SourceFighterLayout'))
    parts=ROOT/'src/melee/ft/ftparts_table.c'
    (out/'source_contract.inc').write_text(catalog[start:end]+'\n#include <melee/ft/native_script_binding_item_resources.h>\n#include <melee/ft/native_script_binding_spawn.h>\n'+body(catalog,'nativeScriptBindings_Descriptor')+'\n'+guards+'\n'+body(parts.read_text(),'ftParts_GetBoneIndex'))
    for path in (catalog_path,props,parts): hashes[str(path)]=hashlib.sha256(path.read_bytes()).hexdigest()
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    base_path = HERE.parent/'script_form_feedback/test.c'; base = base_path.read_text()
    for name in ('efSync_Spawn','hsdObjIsDescendantOf','lb_8000B1CC','invoke','reject','main'):
        base = base.replace(body(base,name),'')
    base = base[:base.index('static void classifier(void)')].replace('static void setup(void)','static void base_setup(void)')
    base=base.replace('#include "dispatch_expected.inc"','')
    (out/'controlled_source.inc').write_text(base); hashes[str(base_path)] = sha(base_path)
    catalog = (ROOT/'src/melee/ft/native_script_bindings.c').read_text()
    chunks = [body(catalog,n) for n in ('error','valid_context','accepts_argument','sameExpression',
        'nativeScriptBindings_ResolveCall','nativeScriptBindings_Resolve','nativeScriptBindings_DescribeCall')]
    (out/'catalog_frontend.inc').write_text('\n'.join(re.sub(r'\berror\b','catalog_error',part) for part in chunks))
    native=[]
    for relative,names in [
        ('src/sysdolphin/baselib/class.c',['hsdObjIsDescendantOf']),
        ('src/melee/ft/native_fighter_module.c',['spawn_effect_between_parts']),
        ('src/melee/lb/lbjoint_position.c',['lb_8000B1CC']),
        ('src/melee/lb/lb_00B0.c',['lb_8000C1C0','lb_8000C290']),
        ('src/melee/ef/eflib.c',['efLib_Create_Attach','efLib_Create_AttachChild_Scale','efLib_Create_Attach_Scale_FacingDir'])]:
        path=ROOT/relative; text=path.read_text(); hashes[str(path)]=sha(path)
        reference=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
        for name in names:
            actual=body(text,name)
            if actual!=body(reference,name): raise RuntimeError('Native reference changed: '+name)
            if name=='spawn_effect_between_parts': actual=actual.replace(name,'reference_spawn_between')
            native.append(actual)
    (out/'native_contract.inc').write_text('\n'.join(native))
    selected=[]
    for relative,ids in [('src/melee/ef/efsync.c',[0x4BB,0x4BC,0x50B]),('src/melee/ef/efalt.c',[0x48F])]:
        path=ROOT/relative; text=path.read_text();hashes[str(path)]=sha(path)
        reference=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
        for id in ids:
            label='    case 0x%X:'%id;at=text.index(label);case=text[at:text.index('    case ',at+len(label))]
            at=reference.index(label)
            if case!=reference[at:reference.index('    case ',at+len(label))]:raise RuntimeError('Native pair dispatch changed')
            selected.append(case)
        if ids[0]==0x4BB:
            at=text.index('    while (efLib_AnimCount != 0)'); end=text.index('\n    va_end',at)
            drain=text[at:end];at=reference.index('    while (efLib_AnimCount != 0)')
            if drain!=reference[at:reference.index('\n    va_end',at)]:raise RuntimeError('Native animation drain changed')
            (out/'native_drain.inc').write_text(drain)
    (out/'native_sync_cases.inc').write_text('\n'.join(selected))
    names=['native_script_binding_effect','native_script_binding_effect_resolve','native_script_effect_dispatch','native_callback_lifetime',
        'native_script_constants','native_script_source','native_script_expression','native_script_statement',
        'native_script_bytecode','native_script_value','native_script_vm',
        'native_script_binding_item_resources_catalog','native_script_binding_spawn_catalog']
    units=[HERE/'test.c']+[ROOT/'src/melee/ft'/(n+'.c') for n in names]+[ROOT/'src/melee/gm/native_match_pool.c']
    inputs=units+[HERE/'run.py',HERE/'README.md',a.compiler,HERE.parent/'script_form_feedback/extract.py']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+list(out.glob('*.inc'))
    hashes.update({str(path):sha(path) for path in inputs})
    cmd=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref',
        '-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-array-parameter','-Wno-missing-field-initializers',
        '-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
    commands=[]
    for stage,command in [('compile',cmd),('fixture',[str(out/'test.exe')])]:
        r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr); commands.append({'stage':stage,'command':command,'exit':r.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if r.returncode: raise RuntimeError(stage+':\n'+(r.stdout+r.stderr)[-9000:])
    for path,digest in hashes.items():
        if sha(Path(path))!=digest: raise RuntimeError('Input changed: '+path)
    receipt={'status':'PASS','summary':json.loads(r.stdout),'native_reference':REFERENCE,'source_sha256':hashes,'commands':commands,
        'scope':['actual source/statement/bytecode/VM and catalog resolution/layout','actual canonical Effect binding and retained Fighter/pool/source-layout guards',
        'unchanged native raw-pair reference, four actual dispatch cases, attachment constructors, position primitive and synchronous animation drain'],
        'controlled':['selected immutable source package and actor/storage views','fixed native effect/constraint allocation and HSD animation leaf instrumentation','unrelated effect operations explicitly unavailable'],
        'limits':['no actual actor scheduler/publication or full paired source package acceptance','no complete effect allocator/shader/animation evaluation, renderer or game','no tick-allocation capacity claim']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n'); print(json.dumps(receipt['summary']))
if __name__=='__main__': main()
