"""Private frozen-source actual page-history cohort; no shared build/game."""
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
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--link', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--frozen', type=Path,
                   help='Reuse production/library/draft snapshot from an earlier run; current fixture files are copied separately.')
    a = p.parse_args()
    out = a.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        p.error('fresh private A: output required')
    build = ROOT / 'build/native'
    base = a.frozen.resolve() if a.frozen else None
    if base:
        prior = json.loads((base / 'freeze.json').read_text())
        if prior['status'] != 'FROZEN' or not all(sha(Path(path)) == value for path, value in prior['snapshot_sha256'].items()):
            p.error('prior frozen snapshot has changed')
        if sha(a.compiler.resolve()) != prior['compiler_sha256']:
            p.error('compiler differs from frozen snapshot')
        build = base / 'snapshot'
    ninja = (build / 'build.ninja').read_text()
    begin = ninja.index('build native_source_assets_test.exe:')
    block = ninja[begin:ninja.index('\n\n', begin)]
    libs = re.search(r'^  LINK_LIBRARIES = (.+)$', block, re.M).group(1).split()
    out.mkdir(parents=True)
    snapshot = out / 'snapshot'
    snapshot.mkdir()
    pairs = []
    for folder, target in [(build / 'src' if base else ROOT / 'src', snapshot / 'src'),
                           (build / 'dolphin' if base else ROOT / 'extern/dolphin/include', snapshot / 'dolphin'),
                           (build / 'aurora' if base else ROOT.parent / 'aurora/include', snapshot / 'aurora')]:
        for source in folder.rglob('*'):
            if source.is_file():
                pairs.append((source, target / source.relative_to(folder)))
    for name in ['test.c', 'run.py', 'README.md']:
        pairs.append((HERE / name, snapshot / name))
    for name in dict.fromkeys(n for n in libs if not n.startswith('-')):
        pairs.append((build / 'lib' / name if base else build / name, snapshot / 'lib' / name))
    for dll in (base if base else build).glob('*.dll'):
        pairs.append((dll, out / dll.name))
    draft = build / 'draft' if base else a.link
    for source in draft.rglob('*'):
        if source.is_file():
            pairs.append((source, snapshot / 'draft' / source.relative_to(draft)))
    pairs.append((build / 'build.ninja', snapshot / 'build.ninja'))
    original = {str(source.resolve()): sha(source) for source, _ in pairs}
    for source, target in pairs:
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
    copied = {str(target.resolve()): sha(target) for _, target in pairs}
    freeze_ok = all(sha(source) == original[str(source.resolve())] == copied[str(target.resolve())]
                    for source, target in pairs)
    frozen = {'status': 'FROZEN' if freeze_ok else 'FAIL',
              'original_sha256': original, 'snapshot_sha256': copied,
              'compiler_sha256': sha(a.compiler.resolve()),
              'production_parent_freeze': str(base / 'freeze.json') if base else None,
              'production_parent_freeze_sha256': sha(base / 'freeze.json') if base else None,
              'scope': 'Subsequent compilation uses only private copied source/includes/library/draft inputs; later live edits are independent.'}
    (out / 'freeze.json').write_text(json.dumps(frozen, indent=2) + '\n')
    print(json.dumps({'freeze': str(out / 'freeze.json'), 'status': frozen['status']}), flush=True)
    if not freeze_ok:
        raise SystemExit('live input changed while snapshot copied')
    flags = ['-std=gnu11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
             '-Wno-unused-function', '-Wno-unused-variable', '-Wno-array-parameter',
             '-fno-zero-initialized-in-bss', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
             '-D_USE_MATH_DEFINES', '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt',
             '-fuse-ld=lld', '-Wl,/opt:ref', '-I' + str(snapshot / 'src'),
             '-I' + str(snapshot / 'dolphin'), '-I' + str(snapshot / 'aurora')]
    command = [str(a.compiler.resolve())] + flags + [str(snapshot / 'test.c')]
    command += [n if n.startswith('-') else str(snapshot / 'lib' / n) for n in libs]
    command += ['-o', str(out / 'test.exe')]
    commands, results = [], []
    for stage, cmd in [('compile', command), ('fixture', [str(out / 'test.exe'), str(snapshot / 'draft')])]:
        result = subprocess.run(cmd, cwd=out, capture_output=True, text=True, timeout=120,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        (out / (stage + '.log')).write_text(result.stdout + result.stderr)
        commands.append({'stage': stage, 'command': cmd, 'exit': result.returncode})
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        results += [json.loads(line) for line in result.stdout.splitlines() if line.startswith('{')]
        if result.returncode:
            break
    unchanged = all(sha(Path(path)) == value for path, value in copied.items())
    unchanged = unchanged and sha(a.compiler.resolve()) == frozen['compiler_sha256']
    status = 'PASS' if result.returncode == 0 and unchanged else 'FAIL'
    receipt = {'status': status, 'commands': commands, 'results': results,
               'snapshot_sha256': copied, 'snapshot_unchanged': unchanged,
               'compiler_sha256': frozen['compiler_sha256'], 'freeze': str(out / 'freeze.json'),
               'scope': 'Actual production Regions_Build and Windows Pages restore of scene/graph/ID/reservation/actor/GObj/heap state, with deterministic replay.',
               'controlled': ['private unpublished candidate/typed Arrow root staging; public metadata remains blocked',
                              'runtime host tables and tracked heap tables initialized directly; unchanged kernels',
                              'captured main-heap Item payload setup and controlled cleanup invoking actual Destroy/Detach'],
               'limits': ['no production package loader, public metadata, renderer, GPU, audio, scheduler scene launch or whole-match rollback claim',
                          'native methods/empty effective material environment and real one-root Arrow family only',
                          'unchanged included runtime/heap/scene static functions suppress unused-function/variable warnings; known SDK array-parameter warning suppressed']}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'status': status, 'results': results, 'receipt': str(out / 'receipt.json')}))
    print((result.stdout + result.stderr)[-6000:])
    raise SystemExit(result.returncode or (not unchanged))


if __name__ == '__main__':
    main()
