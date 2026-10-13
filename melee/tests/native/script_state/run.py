"""Source-checked state/collision fixture, no game sessions."""
import argparse,hashlib,json,subprocess
from pathlib import Path
from extract_state import extract,reference,reference_path,ROOT
HERE=Path(__file__).resolve().parent
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--compiler',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if out.drive.upper()!='A:' or out.exists():parser.error('Use fresh output on A:')
    out.mkdir(parents=True);kin=reference.extract(ROOT,out/'script_kinetics_actual.h');state=extract(ROOT,out/'script_state_actual.h')
    import importlib.util
    spec=importlib.util.spec_from_file_location('combat_extract',HERE.parent/'script_combat/extract_combat.py');combat=importlib.util.module_from_spec(spec);spec.loader.exec_module(combat)
    combat_bodies=combat.extract(ROOT,out/'script_combat_actual.h')
    # Exact current primitive-target source closure; no build is invoked.
    relative_units=[
        'tests/native/script_state/test.c',
        'src/melee/ft/native_script_source.c',
        'src/melee/ft/native_script_expression.c',
        'src/melee/ft/native_script_statement.c',
        'src/melee/ft/native_script_program.c',
        'src/melee/ft/native_script_value.c',
        'src/melee/ft/native_script_bytecode.c',
        'src/melee/ft/native_script_vm.c',
        'src/melee/ft/native_script_bindings.c', 'src/melee/ft/native_script_binding_item_resources_catalog.c', 'src/melee/ft/native_script_binding_spawn_catalog.c',
        'tests/native/script_bindings_unavailable_item.c', 'tests/native/script_bindings_unavailable_item_resources.c',
        'src/melee/ft/native_script_binding_properties.c',
        'src/melee/ft/native_script_binding_kinetics.c',
        'src/melee/ft/native_script_math.c',
        'src/melee/ft/native_script_constants.c',
        'src/melee/ft/native_script_binding_transition.c',
        'tests/native/script_bindings_unavailable_presentation.c',
        'src/melee/ft/native_callback_lifetime.c',
        'tests/native/script_kinetics/engine_fixture.c',
        'src/melee/ft/native_script_binding_setup.c',
        'tests/native/script_setup/engine_fixture.c',
        'src/melee/ft/native_script_binding_state.c',
        'tests/native/script_state/engine_fixture.c',
        'src/melee/ft/native_script_binding_combat.c',
        'src/sysdolphin/baselib/random.c',
        'tests/native/script_combat/engine_fixture.c',
        'tests/native/script_bindings_unavailable_spawn.c',
        'src/melee/ft/native_script_binding_effect_resolve.c',
        'tests/native/script_bindings_unavailable_effect.c',
        'tests/native/script_bindings_unavailable_actions.c',
        'tests/native/script_fighter_hooks_absent.c',
        'tests/native/script_bindings_unavailable_ledge.c',
        'tests/native/script_bindings_unavailable_jump.c',
        'tests/native/script_bindings_unavailable_forms.c',
        'tests/native/script_bindings_unavailable_feedback.c',
        'tests/native/script_bindings_unavailable_pose.c',
        'tests/native/script_bindings_unavailable_article_motion.c',
        'tests/native/script_bindings_unavailable_world_feedback.c',
        'tests/native/script_bindings_unavailable_item_model.c',
        'tests/native/script_bindings_unavailable_article_combat.c',
        'tests/native/script_bindings_unavailable_targets.c',
        'tests/native/script_bindings_unavailable_cpu_control.c',
        'tests/native/script_bindings_unavailable_article_lifecycle.c',
        'tests/native/script_bindings_unavailable_vectors.c',
        'tests/native/script_bindings_unavailable_signal.c',
        'tests/native/script_bindings_unavailable_article_feedback.c',
        'tests/native/script_bindings_unavailable_common_movement.c',
        'tests/native/script_bindings_unavailable_visuals.c',
        'tests/native/script_bindings_unavailable_item_birth.c',
        'tests/native/script_bindings_unavailable_common_properties.c',
    ]
    units=[ROOT/path for path in relative_units]+[ROOT/'src/melee/gm/native_match_pool.c',HERE.parent/'script_receiver_unavailable_pool.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_held_items.c']
    units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in ['item_animation','item_lifecycle','item_collision','ground_controls','audio','model_graph','item_observations','fighter_hitboxes','guard','air_inputs','air_physics','fighter_observations','contact','absorber','item_terrain','capture','collision','held_state']]
    def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
    inputs={str(p):sha(p) for p in units+[HERE/'README.md',HERE/'engine_fixture.h',HERE/'box_cases.h',Path(__file__).resolve(),HERE/'extract_state.py',reference_path]};inputs.update(kin['inputs']);inputs.update(state['inputs']);inputs.update(combat_bodies['inputs']);commands=[];exe=out/'test.exe'
    compile=[args.compiler,'-std=c11','-O2','-ffp-contract=off','-fuse-ld=lld','-Wall','-Wextra','-Werror','-Wno-array-parameter','-Wno-missing-field-initializers','-DMELEE_NATIVE=1','-DSCRIPT_FIXTURE_SOURCE_ACTOR=1','-DSCRIPT_FIXTURE_SOURCE_ATTACHED=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),'-I'+str(out)]
    for stage,cmd in [('compile',compile+units+['-o',exe]),('run',[exe])]:
        cmd=list(map(str,cmd));result=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
        (out/(stage+'.log')).write_text(result.stdout+result.stderr,newline='\n');commands.append({'stage':stage,'command':cmd,'exit':result.returncode})
        if result.returncode:raise RuntimeError(stage+' failed:\n'+result.stdout+result.stderr)
    for p,h in inputs.items():
        if sha(Path(p))!=h:raise RuntimeError('Changed input '+p)
    receipt={'status':'PASS','result':json.loads(result.stdout),'inputs':inputs,'commands':commands,'state_bodies':state,'kinetic_bodies':kin,
        'scope':'Actual fall/freefall/landing/collision bodies and production frontend/VM; neutral/Wait unavailable, covered by connected movement fixture; motion installation, resource/geometry/audio and rare interceptor branches controlled. No game or full native entry metadata acceptance.'}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n',newline='\n');print(json.dumps(receipt['result']))
if __name__=='__main__':main()
