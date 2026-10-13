"""Actual fighter setup binding tests; outputs on A:, no games."""
import argparse,hashlib,json,subprocess,importlib.util
from pathlib import Path
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
spec=importlib.util.spec_from_file_location('state_extract',HERE.parent/'script_state/extract_state.py');state=importlib.util.module_from_spec(spec);spec.loader.exec_module(state)
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--compiler',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():parser.error('Use fresh output on A:')
    out.mkdir(parents=True);kin=state.reference.extract(ROOT,out/'script_kinetics_actual.h');st=state.extract(ROOT,out/'script_state_actual.h')
    names=['native_script_bindings.c', 'native_script_binding_item_resources_catalog.c', 'native_script_binding_spawn_catalog.c','native_script_binding_properties.c','native_script_binding_kinetics.c','native_script_binding_state.c','native_script_binding_setup.c','native_script_value.c','native_script_math.c','native_script_expression.c','native_callback_lifetime.c','native_script_source.c','native_script_statement.c','native_script_program.c','native_script_bytecode.c','native_script_vm.c']
    units=[ROOT/'src/melee/ft'/n for n in names]+[HERE.parent/'script_kinetics/engine_fixture.c',HERE.parent/'script_state/engine_fixture.c',HERE/'engine_fixture.c',HERE/'test.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
    def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    inputs={str(p):sha(p) for p in units};inputs.update(kin['inputs']);inputs.update(st['inputs'])
    for relative in ['src/melee/ft/inlines.h','src/melee/ft/native_fighter_module.c','tools/native_fighter_script_code.c','src/melee/ft/ftcharacter_assets.h','src/melee/ft/native_script_bindings.h',str(HERE.relative_to(ROOT)/'engine_fixture.h')]:
        source=ROOT/relative;inputs[str(source)]=sha(source)
    commands=[];exe=out/'test.exe'
    compile=[args.compiler,'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-DMELEE_NATIVE=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
    for stage,cmd in [('compile',compile+units+['-o',exe]),('run',[exe])]:
        cmd=list(map(str,cmd));result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(out/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n');commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    for p,h in inputs.items():
        if sha(Path(p))!=h:raise RuntimeError('Changed input '+p)
    receipt={'status':'PASS','result':json.loads(result.stdout),'inputs':inputs,'commands':commands,'scope':'Production setup/command/catalog/frontend/VM and actual inline item-presentation reference. Callback actor registry and native presentation leaves controlled; root fighter fixture separately tests real scheduler dispatch. No game or article/effect receiver closure.'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
