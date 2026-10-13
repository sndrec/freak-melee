"""Compile complete owned-move source and execute transitions on a registered fighter."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent/"script_state"))
from extract_state import extract,reference
from extract_planes import extract as extract_planes


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if out.exists():parser.error("Use a fresh output directory")
    out.mkdir(parents=True)
    kinetic=reference.extract(ROOT,out/"script_kinetics_actual.h")
    state=extract(ROOT,out/"script_state_actual.h")
    planes=extract_planes(ROOT,out/"script_transition_planes_actual.h")
    # Exact current primitive-target source closure; no build is invoked.
    relative_units=[
        'tests/native/script_transition/test.c',
        'tests/native/script_transition/engine_fixture.c',
        'tests/native/script_kinetics/engine_fixture.c',
        'tests/native/script_kinetics/unavailable_runtime.c',
        'tests/native/script_bindings_unavailable_presentation.c',
        'src/melee/ft/native_script_source.c',
        'src/melee/ft/native_script_expression.c',
        'src/melee/ft/native_script_statement.c',
        'src/melee/ft/native_script_program.c',
        'src/melee/ft/native_script_value.c',
        'src/melee/ft/native_script_bytecode.c',
        'src/melee/ft/native_script_vm.c',
        'src/melee/ft/native_script_compile.c', 'src/melee/ft/native_script_binding_item_resources_resolve.c',
        'src/melee/ft/native_script_moves.c',
        'src/melee/ft/native_script_move_lowering.c',
        'src/melee/ft/native_script_constants.c',
        'src/melee/ft/native_script_execution.c',
        'src/melee/ft/native_script_event_payload.c',
        'src/melee/ft/native_script_events.c',
        'src/melee/ft/native_script_storage.c',
        'src/melee/ft/native_script_bindings.c', 'src/melee/ft/native_script_binding_item_resources_catalog.c', 'src/melee/ft/native_script_binding_spawn_catalog.c',
        'tests/native/script_bindings_unavailable_item.c', 'tests/native/script_bindings_unavailable_item_resources.c',
        'src/melee/ft/native_script_binding_properties.c',
        'src/melee/ft/native_script_binding_kinetics.c',
        'src/melee/ft/native_script_binding_state.c',
        'src/melee/ft/native_script_binding_transition.c',
        'src/melee/ft/native_script_math.c',
        'src/melee/ft/native_callback_lifetime.c',
        'src/melee/ft/native_script_receiver.c',
        'src/melee/ft/native_script_binding_receiver.c',
        'src/melee/ft/native_script_runtime.c',
        'src/melee/gm/native_match_pool.c',
        'tests/native/script_receiver_unavailable_identity.c',
        'tests/native/script_receiver_unavailable_pool.c',
        'tests/native/script_bindings_unavailable_spawn.c',
        'src/melee/ft/native_script_binding_spawn_resolve.c',
        'src/melee/ft/native_script_binding_effect_resolve.c',
        'tests/native/script_bindings_unavailable_effect.c',
        'tests/native/script_bindings_unavailable_actions.c',
        'tests/native/script_fighter_hooks_absent.c',
        'tests/native/script_bindings_unavailable_ledge.c',
        'tests/native/script_bindings_unavailable_jump.c',
        'tests/native/script_bindings_unavailable_forms.c',
        'tests/native/script_bindings_unavailable_feedback.c',
        'tests/native/script_receiver_unavailable_item_relations.c',
        'src/melee/ft/native_script_binding_item_model_resolve.c', 'src/melee/ft/native_script_model_metadata.c',
        'src/melee/ft/native_script_binding_article_reference_resolve.c',
        'tests/native/script_bindings_unavailable_article_reference.c',
        'src/melee/ft/native_script_binding_signal_resolve.c',
        'src/melee/ft/native_script_pose_metadata.c',
        'src/melee/ft/native_fighter_pose_path.c',
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
    units=[ROOT/path for path in relative_units]
    units += [ROOT/'tests/native/script_bindings_unavailable_normal_attacks.c', ROOT/'tests/native/script_bindings_unavailable_hitlag.c', ROOT/'tests/native/script_bindings_unavailable_stage.c', ROOT/'tests/native/script_bindings_unavailable_family.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_fighter_items.c']
    units += [ROOT/'tests/native/script_bindings_unavailable_colors.c']
    units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c',ROOT/'tests/native/script_bindings_unavailable_item_animation.c']
    units += [ROOT/'src/melee/ft/native_script_hitbox_metadata.c', ROOT/'tests/native/script_bindings_unavailable_hitbox_create.c', ROOT/'tests/native/script_bindings_unavailable_attachments.c']
    units = sorted(set(units + list((ROOT/'tests/native').glob('script_bindings_unavailable_*.c'))))
    paths=units+[HERE/"README.md",HERE.parent/"script_state/extract_state.py",HERE.parent/"script_kinetics/extract_primitives.py",HERE/"engine_fixture.h",HERE/"probe.fms",HERE/"extract_planes.py",Path(__file__).resolve()]
    inputs={str(path):hashlib.sha256(path.read_bytes()).hexdigest() for path in paths}
    inputs.update(kinetic["inputs"]);inputs.update(state["inputs"]);inputs.update(planes["inputs"])
    command=[str(args.compiler),"-std=c11","-O2","-ffp-contract=off","-fuse-ld=lld","-Wl,/errorlimit:0","-Wall","-Wextra","-Werror",
        "-Wno-array-parameter","-DMELEE_NATIVE=1","-DSCRIPT_FIXTURE_SOURCE_ACTOR=1","-DSCRIPT_FIXTURE_FULL_MOVE_UNIT=1","-D_USE_MATH_DEFINES","-D_CRT_SECURE_NO_WARNINGS",
        "-I"+str(ROOT/"src"),"-I"+str(ROOT/"extern/dolphin/include"),"-I"+str(out),
        *map(str,units),"-o",str(out/"test.exe")]
    build=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
    (out/"build.log").write_text(build.stdout+build.stderr);build.check_returncode()
    run=subprocess.run([str(out/"test.exe"),str(HERE/"probe.fms")],cwd=ROOT,capture_output=True,text=True,
        timeout=120,creationflags=subprocess.CREATE_NO_WINDOW)
    (out/"run.log").write_text(run.stdout+run.stderr);run.check_returncode()
    for path,digest in inputs.items():
        if hashlib.sha256(Path(path).read_bytes()).hexdigest()!=digest:raise RuntimeError("Changed test input: "+path)
    receipt=json.loads(run.stdout)
    receipt.update({"game_acceptance":False,"compiler":command,"inputs":inputs,"state_bodies":state,"kinetic_bodies":kinetic,
        "plane_bodies":planes,
        "scope":"Complete source bank / owned move lowering / real execution, VM, storage and registry. Motion installation and graph/animation leaves controlled; no game launch."})
    (out/"receipt.json").write_text(json.dumps(receipt,indent=2)+"\n")
    print(json.dumps({key:value for key,value in receipt.items() if key not in ("compiler","inputs","state_bodies","kinetic_bodies","plane_bodies")},indent=2))


if __name__=="__main__":main()
