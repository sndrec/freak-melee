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


def body(source, name):
    match = re.search(r'^(?:static\s+)?(?:bool|void)\s+' + re.escape(name) + r'\s*\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('Missing native smash dispatch body: ' + name)
    end = source.index('{', match.start()) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


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
    dispatch = []
    for filename, prefix, names in [
        ('ftCo_AttackHi4.c', 'fixtureUp', ['checkLStick', 'checkLStickNoD0', 'ftCo_AttackHi4_CheckInput', 'ftCo_AttackHi4_CheckInputNoD0']),
        ('ftCo_AttackLw4.c', 'fixtureDown', ['checkLStick', 'ftCo_AttackLw4_CheckInput'])]:
        source = (snapshot / 'src/melee/ft/kinds/ftCommon' / filename).read_text(encoding='utf-8')
        renames = {name: prefix + name for name in names}
        renames.update({'doEnter': prefix + 'Default', 'ftNs_AttackHi4_Enter': 'fixtureNessUp',
            'ftNs_AttackLw4_Enter': 'fixtureNessDown', 'ftCo_800DF2D8': 'fixtureCStick',
            'ftCo_800DF3A8': 'fixtureCStick', 'ftCo_80094E54': 'fixtureItemThrowable',
            'ftCo_800DF30C': 'fixtureExtraThrow', 'ftCo_800DF3DC': 'fixtureExtraThrow',
            'ftCo_800957F4': 'fixtureItemThrow'})
        for name in names:
            actual = body(source, name)
            for old, new in renames.items():
                actual = re.sub(r'\b' + re.escape(old) + r'\b', new, actual)
            dispatch.append(actual)
    dispatch_header = snapshot / 'smash_inputs.h'
    dispatch_header.write_text('// Actual complete input bodies; leaf symbol substitutions are explicit controls.\n' + '\n\n'.join(dispatch) + '\n', encoding='utf-8')
    frozen[str(dispatch_header.resolve())] = sha(dispatch_header)
    compiler_hash = sha(args.compiler.resolve())
    flags = ['-std=gnu11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
             '-fno-zero-initialized-in-bss',
             '-Wno-missing-field-initializers', '-Wno-unused-function', '-Wno-unused-variable',
             '-Wno-array-parameter', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
             '-D_USE_MATH_DEFINES', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
             '-fuse-ld=lld', '-Wl,/opt:ref', '-I' + str(snapshot / 'src'),
             '-I' + str(snapshot / 'dolphin'), '-I' + str(snapshot / 'aurora'), '-I' + str(snapshot)]
    command = [str(args.compiler.resolve())] + flags + [str(snapshot / 'test.c')]
    command += [name if name.startswith('-') else str(snapshot / 'lib' / name) for name in libraries]
    command += ['-o', str(output / 'test.exe')]
    commands, results = [], []
    stages = [('compile', command),
              ('link', [str(output / 'test.exe'), str(snapshot / 'link'), 'It_Kind_Link_Boomerang']),
              ('young-link', [str(output / 'test.exe'), str(snapshot / 'young-link'), 'It_Kind_CLink_Boomerang']),
              ('link-primary', [str(output / 'test.exe'), str(snapshot / 'link'), 'It_Kind_Link_Boomerang', 'primary']),
              ('young-link-primary', [str(output / 'test.exe'), str(snapshot / 'young-link'), 'It_Kind_CLink_Boomerang', 'primary'])]
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
               'scope': 'Actual source compilation/Prepare/publication, Fighter smash input dispatch and actual hook VM execution, Item actor/VM animation selectors, native graph/controller comparisons, fixed AObj/FObj capacity/provenance rejection, and real Regions_Build/Pages replay.',
               'controlled': ['private unpublished candidate and runtime/main-heap tables receive explicit inputs; production kernels unchanged',
                              'captured heap Item userdata initialized directly; controlled cleanup invokes real Destroy/Detach',
                              'pool occupancy/generation and foreign-origin/end-callback faults injected explicitly before calls',
                              'actual up/down smash input bodies have controlled C-stick/item predicates and default/native entry/throw leaves; standard move startup is not exercised'],
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
