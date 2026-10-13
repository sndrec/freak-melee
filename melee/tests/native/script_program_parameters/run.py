"""Bounded actual Item/Execution proof of pinned program parameters; no game."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from extract import extract

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
BASE = HERE.parent / 'script_item_actor'

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        parser.error('a fresh private A: output directory is required')
    out.mkdir(parents=True)
    extract(ROOT, out)
    compiler = args.compiler.resolve()
    if compiler.is_dir():
        compiler /= 'clang.exe'
    names = ['source', 'expression', 'statement', 'program', 'bytecode', 'vm',
             'value', 'math', 'constants', 'bindings', 'binding_item_resources_catalog', 'binding_spawn_catalog', 'compile', 'binding_item_resources_resolve', 'moves',
             'move_lowering', 'events', 'event_payload', 'storage', 'execution',
             'receiver', 'binding_receiver', 'actor_access', 'binding_effect_resolve',
             'binding_spawn_resolve', 'binding_item_model_resolve','model_metadata',
             'binding_article_reference_resolve', 'binding_signal_resolve',
             'pose_metadata']
    units = [ROOT / 'src/melee/ft' / ('native_script_' + name + '.c') for name in names]
    units += [ROOT / 'src/melee/ft/native_fighter_pose_path.c',
              ROOT / 'src/melee/ft/native_callback_lifetime.c']
    unavailable = ['actions', 'item_resources', 'effect', 'spawn', 'world_feedback', 'item_model',
                   'article_motion', 'article_combat', 'targets', 'cpu_control',
                   'article_lifecycle', 'vectors', 'signal', 'feedback', 'jump',
                   'forms', 'ledge', 'article_reference', 'article_feedback',
                   'common_movement', 'visuals', 'pose', 'normal_attacks',
                   'fighter_items', 'colors', 'absorber', 'ground_controls',
                   'item_observations', 'held_items', 'item_lifecycle',
                   'fighter_hitboxes', 'common_properties', 'hitlag', 'stage', 'family',
                   'item_animation', 'item_collision', 'audio', 'model_graph', 'guard',
                   'air_inputs', 'air_physics', 'fighter_observations', 'contact',
                   'item_terrain', 'capture', 'collision', 'held_state', 'attachments']
    units += [ROOT / 'tests/native' / ('script_bindings_unavailable_' + name + '.c') for name in unavailable]
    units += [ROOT / 'src/melee/it' / (name + '.c') for name in
              ['native_script_item_actor', 'native_script_item_snapshot']]
    units += [ROOT / 'src/melee/gm' / (name + '.c') for name in
              ['native_match_pool', 'native_match_snapshot_pool', 'native_rollback_state']]
    units += [ROOT / 'src/sysdolphin/baselib' / (name + '.c') for name in
              ['gobj', 'gobjinit', 'gobjplink', 'gobjproc', 'gobjgxlink',
               'gobjuserdata', 'gobjobject', 'native_gobj_pool_storage']]
    units += [ROOT / 'tests/native/script_effect_actor_absent.c', BASE / 'boundary.c', HERE / 'absent.c']
    units += [ROOT/'src/melee/it/native_article_animation_demand.c', ROOT/'src/melee/it/native_article_animation_texture.c']
    runtime = ROOT / 'src/melee/ft/native_script_runtime.c'
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    inputs = units + [runtime, HERE / 'test.c', HERE / 'run.py', HERE / 'README.md', HERE / 'extract.py',
                      ROOT / 'src/melee/it/native_item_owner.c', out / 'actual_owner.inc', BASE / 'boundary.h', compiler]
    inputs += list((ROOT / 'src').rglob('*.h')) + list((ROOT / 'extern/dolphin/include').rglob('*.h'))
    hashes = {str(path): sha(path) for path in inputs}
    common = ['-std=gnu11', '-O2', '-ffp-contract=off', '-Werror', '-Wno-array-parameter', '-DMELEE_NATIVE=1',
              '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES',
              '-include', str(ROOT / 'src/melee/gm/native_rollback_state_section.h'),
              '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'extern/dolphin/include'), '-I' + str(out)]
    commands = []
    stages = [
        ('compile-test', [str(compiler)] + common + ['-Wall', '-Wextra', '-Wno-array-parameter', '-c', str(HERE / 'test.c'), '-o', str(out / 'test.obj')]),
        ('compile-runtime', [str(compiler)] + common + ['-DMELEE_ROLLBACK_HOST_STATE=1', '-c', str(runtime), '-o', str(out / 'runtime.obj')]),
        ('compile', [str(compiler)] + common + ['-ffunction-sections', '-fdata-sections', '-fno-zero-initialized-in-bss',
         '-fuse-ld=lld', '-Wl,/opt:ref', '-Wl,/errorlimit:0'] + list(map(str, units)) +
         [str(out / 'test.obj'), str(out / 'runtime.obj'), '-o', str(out / 'test.exe')]),
        ('fixture', [str(out / 'test.exe')]),
    ]
    for stage, command in stages:
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                                timeout=120, creationflags=subprocess.CREATE_NO_WINDOW)
        (out / (stage + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
        commands.append({'stage': stage, 'command': command, 'exit': result.returncode})
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        if result.returncode:
            raise RuntimeError(stage + ' failed:\n' + (result.stdout + result.stderr)[-10000:])
    if any(sha(Path(path)) != expected for path, expected in hashes.items()):
        raise RuntimeError('an input changed during the run')
    receipt = {'status': 'PASS', 'summary': json.loads(result.stdout), 'input_sha256': hashes,
               'commands': commands, 'warnings': 'fixture: -Wall -Wextra -Werror; linked units: -Werror; SDK MTXPerspective duplicate bounds: -Wno-array-parameter',
               'actual': ['frontend/index/typed bytecode and Compile_Build',
                          'Item actor/Execution/VM/storage and packed Item snapshots',
                          'GObj fixed pools/native creator capture and retirement'],
               'controlled': ['entity metadata schema and data-only Article asset/spec publication',
                              'unrelated engine families explicitly unavailable; ordinary native Context uses retained physical lifetime'],
               'limits': ['typed import values are supplied at preparation, not extracted from DAT here',
                          'no full native Item construction, drawing, physics, live game or whole-match rollback claim',
                          'packed Item restore requires native object/state peers and both immutable banks to remain pinned']}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'receipt': str(out / 'receipt.json'), 'summary': receipt['summary']}))

if __name__ == '__main__':
    main()
