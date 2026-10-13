"""Build source-checked generic kinetic bodies and direct production VM tests."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
from extract_primitives import extract,ROOT

HERE=Path(__file__).resolve().parent

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();output=args.output.resolve()
    if output.exists():parser.error('Use a fresh output directory')
    output.mkdir(parents=True);extracted=extract(ROOT,output/'script_kinetics_actual.h')
    names=['native_script_bindings.c', 'native_script_binding_item_resources_catalog.c', 'native_script_binding_spawn_catalog.c','native_script_binding_properties.c','native_script_binding_kinetics.c',
        'native_script_value.c','native_script_math.c','native_script_expression.c','native_callback_lifetime.c',
        'native_script_source.c','native_script_statement.c','native_script_bytecode.c','native_script_vm.c']
    units=[ROOT/'src/melee/ft'/p for p in names]+[HERE/'engine_fixture.c',HERE/'unavailable_bindings.c',HERE/'boundary.c',HERE/'test.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_pose.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
    units += [ROOT/'src/melee/ft/native_script_constants.c', ROOT/'src/melee/ft/native_script_binding_effect_resolve.c']
    units += [ROOT/'src/melee/gm/native_match_pool.c']
    units = sorted(set(units + list((ROOT/'tests/native').glob('script_bindings_unavailable*.c'))))
    paths=units+[HERE/'engine_fixture.h',HERE/'extract_primitives.py',Path(__file__).resolve(),ROOT/'src/melee/ft/native_script_bindings.h']
    paths += list((ROOT/'src').rglob('*.h')) + list((ROOT/'extern/dolphin/include').rglob('*.h')) + [args.compiler.resolve()]
    def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
    inputs={str(p):sha(p) for p in paths};inputs.update(extracted['inputs']);commands=[];exe=output/'test.exe'
    compile=[args.compiler,'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter',
        '-DMELEE_NATIVE=1','-DSCRIPT_FIXTURE_SOURCE_ACTOR=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(output)]
    for stage,command in [('compile',compile+units+['-o',exe]),('run',[exe])]:
        command=list(map(str,command));result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (output/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n');commands.append({'stage':stage,'command':command,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    for path,digest in inputs.items():
        if sha(Path(path))!=digest:raise RuntimeError('Input changed during test: '+path)
    receipt={'status':'PASS','result':json.loads(result.stdout),'inputs':inputs,'commands':commands,'extracted':extracted,
        'scope':'Actual generic movement/animation bodies and production semantic frontend/native bindings/VM. HSD graph/audio/definition and four animation stage leaves are controlled boundaries. Healthy start ordering compared to its four native stage leaves; shared helper physical/logical continuation is covered by script_ledge_entry; retirement injected only at controlled stage exits. No renderer/game/state transition or whole-game acceptance.'}
    (output/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(receipt['result']))

if __name__=='__main__':main()
