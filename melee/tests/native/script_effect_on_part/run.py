"""Canonical raw-part effect source/VM and unchanged native generator contract."""
import argparse, hashlib, importlib.util, json, re, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
REFERENCE = '4733d2a9b'
spec = importlib.util.spec_from_file_location('feedback_extract', HERE.parent/'script_form_feedback/extract.py')
extractor = importlib.util.module_from_spec(spec); spec.loader.exec_module(extractor)
body = extractor.body

def main():
    p = argparse.ArgumentParser(); p.add_argument('--compiler',type=Path,required=True); p.add_argument('--output',type=Path,required=True)
    a = p.parse_args(); out = a.output.resolve()
    if out.exists(): p.error('fresh private A: output required')
    hashes = extractor.extract(ROOT,out)
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    base_path = HERE.parent/'script_form_feedback/test.c'; base = base_path.read_text()
    for name in ('efSync_Spawn','hsdObjIsDescendantOf','invoke','reject','main'):
        base = base.replace(body(base,name),'')
    base = base[:base.index('static void classifier(void)')].replace('static void setup(void)','static void base_setup(void)')
    (out/'controlled_source.inc').write_text(base); hashes[str(base_path)] = sha(base_path)
    catalog = (ROOT/'src/melee/ft/native_script_bindings.c').read_text()
    chunks = [body(catalog,n) for n in ('error','valid_context','accepts_argument','sameExpression',
        'nativeScriptBindings_ResolveCall','nativeScriptBindings_Resolve','nativeScriptBindings_DescribeCall')]
    (out/'catalog_frontend.inc').write_text('\n'.join(re.sub(r'\berror\b','catalog_error',part) for part in chunks))
    native=[]
    for relative,names in [('src/sysdolphin/baselib/generator.c',['hsd_8039EFAC']),
        ('src/sysdolphin/baselib/class.c',['hsdObjIsDescendantOf']),
        ('src/melee/ft/native_fighter_module.c',['spawn_effect_on_part'])]:
        path=ROOT/relative; text=path.read_text(); hashes[str(path)]=sha(path)
        reference=subprocess.run(['git','show',REFERENCE+':'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
        for name in names:
            actual=body(text,name)
            if actual!=body(reference,name): raise RuntimeError('Native reference changed: '+name)
            if name=='spawn_effect_on_part': actual=actual.replace(name,'reference_spawn_on_part')
            native.append(actual)
    (out/'native_contract.inc').write_text('\n'.join(native))
    path=ROOT/'src/melee/ef/efsync.c'; text=path.read_text(); hashes[str(path)]=sha(path)
    at=text.index('    case 0x4EC:'); selected=text[at:text.index('    case ',at+10)]
    reference=subprocess.run(['git','show',REFERENCE+':src/melee/ef/efsync.c'],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
    at=reference.index('    case 0x4EC:')
    if selected!=reference[at:reference.index('    case ',at+10)]: raise RuntimeError('Native 0x4EC dispatch changed')
    (out/'native_sync_case.inc').write_text(selected)
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
        'unchanged native raw-part reference body, efSync0x4EC case, generator attach/ref_INC and class ancestry'],
        'controlled':['selected immutable source package and actor/storage views','fixed generator allocation/resource leaf','unrelated effect operations explicitly unavailable'],
        'limits':['no actual actor scheduler/publication or full paired source package acceptance','no particle command interpreter/rendering/game','no tick-allocation capacity claim']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n'); print(json.dumps(receipt['summary']))
if __name__=='__main__': main()
