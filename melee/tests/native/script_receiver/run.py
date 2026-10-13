"""Actual source/Execution/VM retained receiver tests with real registry and GObj pool."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[3];HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent/'script_state'));sys.path.insert(0,str(HERE.parent/'script_transition'))
from extract_state import extract,reference
from extract_planes import extract as planes


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--compiler',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();out=a.output.resolve()
    if out.exists():p.error('Use a fresh output directory')
    out.mkdir(parents=True);kinetic=reference.extract(ROOT,out/'script_kinetics_actual.h');state=extract(ROOT,out/'script_state_actual.h');plane=planes(ROOT,out/'script_transition_planes_actual.h')
    identity_paths=[ROOT/'src/melee/it/native_item_owner.c',ROOT/'src/melee/it/native_article.c']
    identity='\n\n'.join([reference.body(identity_paths[0].read_text(),name) for name in ('nativeItem_CaptureCreator','nativeItem_Creator')]+[reference.body(identity_paths[1].read_text(),'nativeArticle_Creator')])
    (out/'script_receiver_identity_actual.h').write_text(identity+'\n')
    setup_path=ROOT/'src/melee/ft/native_script_binding_setup.c'
    setup=setup_path.read_text()
    command_bodies='\n\n'.join(reference.body(setup,name) for name in ('error','command_index','nativeScriptBindingSetup_Read','nativeScriptBindingSetup_Write'))
    (out/'script_receiver_command_actual.h').write_text(command_bodies.replace('command_index','receiverCommandIndex').replace('error(','receiverSetupError(')+'\n')
    names=['binding_effect_resolve','binding_spawn_resolve','compile', 'binding_item_resources_resolve','moves','move_lowering','constants','execution','event_payload','events','storage','runtime',
        'bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog','binding_properties','binding_kinetics','binding_state','binding_transition','receiver','binding_receiver',
        'value','math','expression','source','statement','program','bytecode','vm']
    units=[ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in names]
    units += [ROOT/'tests/native/script_bindings_unavailable_article_feedback.c', ROOT/'tests/native/script_bindings_unavailable_item_resources.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_common_movement.c', ROOT/'tests/native/script_bindings_unavailable_visuals.c', ROOT/'tests/native/script_bindings_unavailable_common_properties.c']
    units += [ROOT/'src/melee/ft/native_script_pose_metadata.c', ROOT/'src/melee/ft/native_fighter_pose_path.c', ROOT/'tests/native/script_bindings_unavailable_pose.c']
    units += [ROOT/'src/melee/ft/native_script_binding_signal_resolve.c', ROOT/'tests/native/script_bindings_unavailable_signal.c',
        ROOT/'src/melee/ft/native_script_binding_article_reference_resolve.c',ROOT/'tests/native/script_bindings_unavailable_article_reference.c']
    units += [ROOT/'tests/native'/('script_bindings_unavailable_'+n+'.c') for n in ['feedback','jump','forms','ledge', 'hitlag', 'stage', 'family']]
    units += [ROOT/'tests/native/script_bindings_unavailable_vectors.c',ROOT/'tests/native/script_bindings_unavailable_article_lifecycle.c']
    units += [ROOT/'src/melee/ft'/('native_script_'+name+'.c') for name in
        ['binding_item_model_resolve','model_metadata']]
    units += [ROOT/'src/melee/it'/('native_article_animation_'+name+'.c') for name in ['demand','texture']]
    units += [ROOT/'tests/native'/('script_bindings_unavailable_'+name+'.c') for name in
        ['item_animation','world_feedback','item_lifecycle','item_collision','article_motion',
         'ground_controls','item_model','audio','model_graph','item_observations','fighter_hitboxes',
         'article_combat','targets','cpu_control','colors','fighter_items','guard',
         'air_inputs','air_physics','fighter_observations','contact','absorber','normal_attacks',
         'item_terrain','capture','collision','held_state','held_items','item_birth']]
    units += [ROOT/'src/melee/ft/native_callback_lifetime.c',ROOT/'src/melee/gm/native_match_pool.c',
        HERE.parent/'script_kinetics/engine_fixture.c',HERE.parent/'script_kinetics/unavailable_runtime.c',HERE.parent/'script_bindings_unavailable_presentation.c',HERE/'engine_fixture.c',HERE/'test.c',HERE.parent/'script_fighter_hooks_absent.c',HERE.parent/'script_bindings_unavailable_actions.c',HERE.parent/'script_bindings_unavailable_effect.c',HERE.parent/'script_bindings_unavailable_spawn.c']
    units += [ROOT/'src/melee/ft/native_script_hitbox_metadata.c', ROOT/'tests/native/script_bindings_unavailable_hitbox_create.c', ROOT/'tests/native/script_bindings_unavailable_attachments.c']
    paths=units+[HERE/'probe.fms',HERE/'engine_fixture.h',HERE/'README.md',Path(__file__).resolve(),setup_path,*identity_paths]
    paths += list((ROOT/'src').rglob('*.h')) + list((ROOT/'extern/dolphin/include').rglob('*.h'))
    paths += [HERE.parent/'script_kinetics/engine_fixture.h',
        HERE.parent/'script_transition/engine_fixture.h',
        HERE.parent/'script_state/extract_state.py',
        Path(reference.__file__),
        HERE.parent/'script_transition/extract_planes.py',a.compiler.resolve()]
    inputs={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    inputs.update(kinetic['inputs']);inputs.update(state['inputs']);inputs.update(plane['inputs'])
    command=[str(a.compiler),'-std=c11','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections',
        '-fuse-ld=lld','-Wl,/errorlimit:0','-Wl,/opt:ref','-Wall','-Wextra','-Werror','-Wno-array-parameter',
        '-DMELEE_NATIVE=1','-DSCRIPT_FIXTURE_SOURCE_ACTOR=1','-DSCRIPT_FIXTURE_FULL_MOVE_UNIT=1','-D_USE_MATH_DEFINES','-D_CRT_SECURE_NO_WARNINGS','-I'+str(ROOT/'src'),'-I'+str(ROOT/'extern/dolphin/include'),
        '-I'+str(out),*map(str,units),'-o',str(out/'test.exe')]
    build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW,timeout=180)
    (out/'build.log').write_text(build.stdout+build.stderr);build.check_returncode()
    run=subprocess.run([str(out/'test.exe'),str(HERE/'probe.fms')],cwd=ROOT,capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW,timeout=60)
    (out/'run.log').write_text(run.stdout+run.stderr);run.check_returncode()
    for path,digest in inputs.items():
        if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError('Changed input '+path)
    result=json.loads(run.stdout);result.update({'game_acceptance':False,'compiler':command,'source_sha256':inputs,
        'scope':'Actual complete source bank / production Execution VM receiver routing / native registry+GObj pool; motion installation leaves controlled.'})
    (out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('compiler','source_sha256')},indent=2))


if __name__=='__main__':main()
