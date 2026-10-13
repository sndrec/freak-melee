"""Private actual Item VM/native animation and Pages cohort; no shared build/game."""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--compiler', type=Path, required=True)
    parser.add_argument('--link', type=Path, required=True)
    parser.add_argument('--young-link', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.drive.upper() != 'A:' or output.exists():
        parser.error('fresh private A: output required')
    build = ROOT / 'build/native'
    ninja = (build / 'build.ninja').read_text()
    begin = ninja.index('build native_source_assets_test.exe:')
    block = ninja[begin:ninja.index('\n\n', begin)]
    libraries = re.search(r'^  LINK_LIBRARIES = (.+)$', block, re.M).group(1).split()
    snapshot = output / 'snapshot'
    pairs = []
    for source, destination in [(ROOT / 'src', snapshot / 'src'),
                                (ROOT / 'extern/dolphin/include', snapshot / 'dolphin'),
                                (ROOT.parent / 'aurora/include', snapshot / 'aurora')]:
        pairs += [(path, destination / path.relative_to(source)) for path in source.rglob('*')
                  if path.is_file() and path.suffix in ('.c', '.h', '.hpp')]
    pairs += [(HERE / name, snapshot / name) for name in ('test.c', 'run.py', 'README.md')]
    pairs += [(ROOT / name, snapshot / name) for name in ('docs/scripting/article_models.md', 'examples/link_full.fms', 'examples/young_link_full.fms')]
    pairs += [(build / name, snapshot / 'lib' / name) for name in dict.fromkeys(libraries)
              if not name.startswith('-')]
    pairs += [(path, output / path.name) for path in build.glob('*.dll')]
    pairs += [(build / 'build.ninja', snapshot / 'build.ninja')]
    for draft, name in [(args.link, 'link'), (args.young_link, 'young-link')]:
        pairs += [(path, snapshot / name / path.relative_to(draft))
                  for path in draft.rglob('*') if path.is_file()]
    original = {str(source.resolve()): sha(source) for source, _ in pairs}
    for source, destination in pairs:
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    frozen = {str(destination.resolve()): sha(destination) for _, destination in pairs}
    compiler_hash = sha(args.compiler.resolve())
    flags = ['-std=gnu11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
             '-fno-zero-initialized-in-bss',
             '-Wno-missing-field-initializers', '-Wno-unused-function', '-Wno-unused-variable',
             '-Wno-array-parameter', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
             '-D_USE_MATH_DEFINES', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
             '-fuse-ld=lld', '-Wl,/opt:ref', '-I' + str(snapshot / 'src'),
             '-I' + str(snapshot / 'dolphin'), '-I' + str(snapshot / 'aurora')]
    command = [str(args.compiler.resolve())] + flags + [str(snapshot / 'test.c')]
    command += [name if name.startswith('-') else str(snapshot / 'lib' / name) for name in libraries]
    command += ['-o', str(output / 'test.exe')]
    commands, results = [], []
    stages = [('compile', command),
              ('link-boomerang', [str(output / 'test.exe'), str(snapshot / 'link'), 'It_Kind_Link_Boomerang']),
              ('young-link-boomerang', [str(output / 'test.exe'), str(snapshot / 'young-link'), 'It_Kind_CLink_Boomerang']),
              ('link-bomb', [str(output / 'test.exe'), str(snapshot / 'link'), 'It_Kind_Link_Bomb']),
              ('young-link-bomb', [str(output / 'test.exe'), str(snapshot / 'young-link'), 'It_Kind_CLink_Bomb']),
              ('synthetic-siblings', [str(output / 'test.exe'), str(snapshot / 'link'), 'It_Kind_Link_Boomerang', 'siblings'])]
    for stage, current in stages:
        result = subprocess.run(current, cwd=output, capture_output=True, text=True,
                                timeout=120, creationflags=subprocess.CREATE_NO_WINDOW)
        (output / (stage + '.log')).write_text(result.stdout + result.stderr)
        commands.append({'stage': stage, 'command': current, 'exit': result.returncode})
        results += [json.loads(line) for line in result.stdout.splitlines() if line.startswith('{')]
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        if result.returncode:
            break
    unchanged = all(sha(path) == value for path, value in frozen.items()) and sha(args.compiler.resolve()) == compiler_hash
    live_unchanged = all(sha(path) == value for path, value in original.items())
    status = 'PASS' if result.returncode == 0 and unchanged and live_unchanged else 'FAIL'
    receipt = {'status': status, 'commands': commands, 'results': results,
               'snapshot_sha256': frozen, 'snapshot_unchanged': unchanged,
               'live_input_sha256': original, 'live_inputs_unchanged': live_unchanged,
               'compiler_sha256': compiler_hash,
               'production_units': ['src/melee/ft/native_script_binding_item_animation.c', 'src/melee/ft/native_script_binding_item_animation_prepare.c', 'src/melee/ft/native_script_item_program.c', 'src/melee/it/item.c', 'src/melee/it/native_article_animation_texture.c', 'src/melee/it/native_article_animation_demand.c', 'src/melee/it/native_article_animation_origins.c', 'src/melee/it/native_article_animation_live.c', 'src/melee/it/iteffect.c'],
               'scope': 'Actual descriptor Set/Add source compiler/Prepare/publication, Item VM, native Item_80268D34 and HSD_JObjAddAnimAll controller comparisons, fixed-pool refusal, bounded CON-only TIMG image-index admission with genuine Bomb0/1-to3 image switches and RemoveAnim retention, Regions_Build/Pages replay, and actual it_80278800 zero-jitter RNG/spawn-argument comparisons with a silent effect sink.',
               'controlled': ['efSync_Spawn is an explicit silent argument/result sink; unused efSync_ModelRecipe aborts',
                              'synthetic root-sibling descriptors and effective texture-ID corpus are separate from genuine imported assets',
                              'private unpublished candidate and runtime/main-heap tables receive explicit inputs; production kernels unchanged',
                              'captured heap Item userdata initialized directly; controlled cleanup invokes real Destroy/Detach',
                              'pool occupancy/generation and foreign-origin/end-callback faults injected explicitly before calls'],
               'limits': ['no production package loader/Item constructor/scheduler scene launch or renderer/GPU/audio/game acceptance',
                          'bounded two-owner model/animation corpus; no arbitrary shader/material/shape/callback-channel admission',
                          'Pages proof covers this actual VM/animation cohort, not whole-match rollback',
                          'included runtime/heap/scene static functions suppress unused warnings; known SDK array-parameter warning suppressed']}
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'status': status, 'results': results, 'receipt': str(output / 'receipt.json')}))
    print((result.stdout + result.stderr)[-6000:])
    raise SystemExit(result.returncode or (not unchanged) or (not live_unchanged))


if __name__ == '__main__':
    main()
