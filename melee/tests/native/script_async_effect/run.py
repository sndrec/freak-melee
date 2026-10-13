"""Actual source/VM feedback and native async queue; fresh hidden A: output."""
import argparse, hashlib, importlib.util, json, re, subprocess, sys
from pathlib import Path
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
REFERENCE = '7b077ac85'
spec = importlib.util.spec_from_file_location('feedback_extract', HERE.parent/'script_form_feedback/extract.py')
extractor = importlib.util.module_from_spec(spec)
spec.loader.exec_module(extractor)
body = extractor.body

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args(); out = a.output.resolve()
    if out.exists(): p.error('fresh private A: output required')
    hashes = extractor.extract(ROOT, out)
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    base_path = HERE.parent/'script_form_feedback/test.c'
    base = base_path.read_text()
    for name in ('efSync_Spawn', 'efAsync_Spawn', 'hsdObjIsDescendantOf', 'main'):
        base = base.replace(body(base, name), '')
    base = base[:base.index('static void classifier(void)')]
    base = base.replace('static void setup(void)', 'static void base_setup(void)')
    (out/'controlled_source.inc').write_text(base)
    hashes[str(base_path)] = sha(base_path)
    catalog = (ROOT/'src/melee/ft/native_script_bindings.c').read_text()
    chunks = [body(catalog, n) for n in ('error','valid_context','accepts_argument','sameExpression',
               'nativeScriptBindings_ResolveCall','nativeScriptBindings_Resolve','nativeScriptBindings_DescribeCall')]
    chunks = [re.sub(r'\berror\b', 'catalog_error', s) for s in chunks]
    (out/'catalog_frontend.inc').write_text('\n'.join(chunks))
    paths = {
        'src/melee/ef/efasync.c': ['efAsync_QueueProcessDeferred','efAsync_QueueFlush','efAsync_QueueClear','efAsync_Spawn','efAsync_QueueInit'],
        'src/melee/ef/eflib.c': ['efLib_CreateGenerator_Attach'],
        'src/sysdolphin/baselib/generator.c': ['hsd_8039EFAC'],
        'src/sysdolphin/baselib/class.c': ['hsdObjIsDescendantOf'],
    }
    native = []
    for relative, names in paths.items():
        path = ROOT/relative; text = path.read_text(); hashes[str(path)] = sha(path)
        ref = subprocess.run(['git','show',REFERENCE+':'+relative],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
        for name in names:
            actual = body(text, name)
            if actual != body(ref,name): raise RuntimeError('Native reference body changed: '+name)
            native.append(actual)
    sync_path = ROOT/'src/melee/ef/efsync.c'; sync = sync_path.read_text()
    start = sync.index('    case 0x4D4:'); end = sync.index('    case ',start+10)
    selected = sync[start:end]
    ref_sync = subprocess.run(['git','show',REFERENCE+':src/melee/ef/efsync.c'],cwd=ROOT,capture_output=True,text=True,check=True,creationflags=subprocess.CREATE_NO_WINDOW).stdout
    ref_start = ref_sync.index('    case 0x4D4:')
    if selected != ref_sync[ref_start:ref_sync.index('    case ',ref_start+10)]:
        raise RuntimeError('Native1236 reference case changed')
    (out/'native_queue.inc').write_text('\n'.join(native))
    (out/'native_sync_case.inc').write_text(selected)
    hashes[str(sync_path)] = sha(sync_path)
    units = [ROOT/'src/melee/ft/native_script_binding_item_resources_catalog.c', ROOT/'src/melee/ft/native_script_binding_spawn_catalog.c', HERE/'test.c']+[ROOT/('src/melee/ft/'+n+'.c') for n in (
        'native_script_binding_feedback','native_script_effect_dispatch','native_callback_lifetime',
        'native_script_constants','native_script_source','native_script_expression','native_script_statement',
        'native_script_bytecode','native_script_value','native_script_vm')]+[ROOT/'src/melee/gm/native_match_pool.c']
    inputs = units+[HERE/'run.py',HERE/'README.md',a.compiler,HERE.parent/'script_form_feedback/extract.py']+list((ROOT/'src').rglob('*.h'))+list((ROOT/'extern/dolphin/include').rglob('*.h'))+list(out.glob('*.inc'))
    hashes.update({str(path):sha(path) for path in inputs})
    cmd = [str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-ffunction-sections','-fdata-sections','-Wl,/opt:ref','-Wall','-Wextra','-Werror','-Wno-unused-function','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-D_CRT_SECURE_NO_WARNINGS','-D_USE_MATH_DEFINES','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
    commands=[]
    for stage, command in [('compile',cmd),('fixture',[str(out/'test.exe')])]:
        r=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(r.stdout+r.stderr)
        commands.append({'stage':stage,'command':command,'exit':r.returncode})
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        if r.returncode: raise RuntimeError(stage+':\n'+(r.stdout+r.stderr)[-9000:])
    for path,digest in hashes.items():
        if sha(Path(path))!=digest: raise RuntimeError('Input changed: '+path)
    receipt={'status':'PASS','summary':json.loads(r.stdout),'native_reference':REFERENCE,'source_sha256':hashes,'commands':commands,
        'scope':['actual source/statement/compiler/VM and complete native catalog resolution/layout','real feedback/source guards, native registry and physical pool; controlled source package/actor view publication','unchanged prebatch native async spawn/process/flush/clear/init, exact1236 sync case and native generator attach/ref_INC','controlled fixed8 queue allocator and bounded generator allocation/resource leaf'],
        'limits':['no actual particle command interpreter/rendering, real source actor/scheduler, full Peach or game acceptance','native production ObjAlloc may grow native heap; fixed fixture reservation is not production capacity evidence','frozen/motion cleanup scheduling is audited source order, not executed full Fighter scheduler','no packed async snapshot codec acceptance']}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n'); print(json.dumps(receipt['summary']))
if __name__=='__main__': main()
