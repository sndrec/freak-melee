"""Exercise explicit GM state inside the production match loop."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists() or out.drive.upper() != 'A:':
        parser.error('fresh A: output required')
    out.mkdir(parents=True)
    frozen = out/'snapshot'
    relative = ['src/melee/gm/'+name for name in
                ('gm_1A45.c', 'gm_1A36.c', 'native_match_pool.c', 'native_match_snapshot_pool.c', 'native_match_arena.c', 'native_match_scene.c')]
    relative += ['src/sysdolphin/baselib/'+name for name in
                ('controller.c', 'rumble.c', 'rumble_output.c', 'native_controller_snapshot.c', 'random.c', 'native_render_phase.c')]
    relative.append('src/melee/lb/lb_0195.c')
    relative += ['src/melee/lb/dynamics_state.c','src/melee/lb/native_dynamics_snapshot.c']
    relative += ['src/melee/pl/'+name for name in
                ('player_state.c', 'plstale.c', 'plattack.c', 'native_player_snapshot.c')]
    relative.append('src/melee/ft/native_fighter_pool.c')
    relative.append('src/melee/it/native_item_pool.c')
    relative.append('src/melee/gr/stage_start_state.c')
    relative.append('src/melee/if/ifall_state.c')
    relative.append('src/melee/if/ifnametag_state.c')
    relative += ['src/melee/gr/stage_generator_state.c','src/melee/gr/native_stage_generator_snapshot.c','src/melee/gr/stage_spawn_state.c','src/melee/gr/native_stage_spawn_snapshot.c']
    relative += ['src/melee/ef/efdata.c','src/melee/ef/native_effect_control_snapshot.c']
    relative += ['src/melee/gm/gmvs_state.c','src/melee/gm/native_versus_control_snapshot.c','src/melee/it/itspawn_state.c','src/melee/if/ifstatus_state.c']
    relative += ['src/sysdolphin/baselib/particle_state.c','src/sysdolphin/baselib/psdisp_state.c','src/sysdolphin/baselib/native_particle_control_snapshot.c']
    relative.append('src/melee/cm/camera_subjects.c')
    relative += ['src/melee/cm/'+name for name in ('camera_state.c','native_camera_snapshot.c')]
    relative += ['src/melee/gr/'+name for name in ('stage_state.c','native_stage_snapshot.c','native_ground_pool.c','stage_hazard_state.c','native_stage_hazard_snapshot.c','native_green_grid.c','native_green_grid_snapshot.c')]
    pairs = [(ROOT/name, frozen/name) for name in relative]
    for directory, target in [(ROOT/'src', frozen/'src'),
                              (ROOT/'extern/dolphin/include', frozen/'dolphin'),
                              (ROOT.parent/'aurora/include', frozen/'aurora'),
                              (ROOT/'build/native/_deps/sdl3_prebuilt-src/include', frozen/'sdl')]:
        assert directory.is_dir(), directory
        pairs += [(path, target/path.relative_to(directory)) for path in directory.rglob('*.h')]
    pairs += [(HERE/name, frozen/name) for name in ('run.py', 'README.md', 'test.c', 'boundaries.c', 'camera_targets_test.c')]
    pairs += [(HERE.parent/'hsd_reservation_snapshot/camera_endpoints.c',frozen/'camera_endpoints.c')]
    originals = {str(source): sha(source) for source, _ in pairs}
    for source, target in pairs:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    pins = {str(target): sha(target) for _, target in pairs}
    flags = ['-std=gnu11', '-O2', '-ffp-contract=off', '-ffunction-sections', '-fdata-sections',
             '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS', '-D_USE_MATH_DEFINES']
    flags += ['-I'+str(frozen/name) for name in ('src', 'dolphin', 'aurora', 'sdl')]
    units = [str(frozen/name) for name in relative] + [str(frozen/name) for name in ('test.c', 'boundaries.c', 'camera_targets_test.c','camera_endpoints.c')]
    commands = [
        ('strict', [str(args.compiler), *flags, '-Wall', '-Wextra', '-Werror', '-Wno-array-parameter',
                    '-fsyntax-only', str(frozen/'src/melee/pl/native_player_snapshot.c'),
                    str(frozen/'src/melee/cm/camera_subjects.c'),
                    str(frozen/'src/melee/cm/native_camera_snapshot.c'), *units[-4:]]),
        ('compile', [str(args.compiler), *flags, '-fuse-ld=lld', '-Wl,/opt:ref', *units, '-o', str(out/'test.exe')]),
        ('fixture', [str(out/'test.exe')])]
    receipt = dict(status='RUNNING', original_sha256=originals, snapshot_sha256=pins,
                   compiler_sha256=sha(args.compiler), commands=[], limits=[
                       'Production GM loop, initializer and repeat algorithms; controlled external scheduler/host/device/render endpoints.',
                       'Actual generic pool component tests root resurrection ordering; not full HSD graph restoration.',
                       'No client/window/audio or complete aggregate/performance acceptance.'])
    path = out/'receipt.json'
    path.write_text(json.dumps(receipt, indent=2)+'\n')
    try:
        for stage, command in commands:
            result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True,
                                    timeout=120, creationflags=subprocess.CREATE_NO_WINDOW)
            (out/(stage+'.log')).write_text(result.stdout+result.stderr)
            receipt['commands'].append(dict(stage=stage, command=command, exit=result.returncode))
            path.write_text(json.dumps(receipt, indent=2)+'\n')
            if result.returncode:
                raise RuntimeError(stage+': '+(result.stdout+result.stderr)[-5000:])
        receipt['summary'] = json.loads(result.stdout)
        assert receipt['summary']['status'] == 'PASS'
        assert all(sha(p) == value for p, value in pins.items())
        assert all(sha(p) == value for p, value in originals.items())
        receipt['status'] = 'PASS'
    except Exception as error:
        receipt.update(status='FAIL', error=str(error))
    path.write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'], summary=receipt.get('summary'),
                         error=receipt.get('error'), receipt=str(path))))
    return int(receipt['status'] != 'PASS')


if __name__ == '__main__':
    raise SystemExit(main())
