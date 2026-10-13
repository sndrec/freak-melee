"""Hidden real-library owned secondary graph cohort; no shared build/game."""
import argparse
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compiler', type=Path, required=True)
    p.add_argument('--link', type=Path, required=True)
    p.add_argument('--young-link', type=Path, required=True)
    p.add_argument('--ness', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    out = a.output.resolve()
    if out.drive.upper() != 'A:' or out.exists():
        p.error('fresh private A: output required')
    build = ROOT / 'build/native'
    ninja = (build / 'build.ninja').read_text()
    begin = ninja.index('build native_source_assets_test.exe:')
    block = ninja[begin:ninja.index('\n\n', begin)]
    libs = re.search(r'^  LINK_LIBRARIES = (.+)$', block, re.M).group(1).split()
    inputs = [a.compiler.resolve(), build / 'build.ninja']
    inputs += [HERE / n for n in ['test.c', 'run.py', 'README.md']]
    inputs += [build / n for n in libs if n.endswith('.lib') and not n.startswith('-')]
    inputs += list((ROOT / 'src').rglob('*.c')) + list((ROOT / 'src').rglob('*.h'))
    inputs += list((ROOT / 'extern/dolphin/include').rglob('*.h')) + list(build.glob('*.dll'))
    for draft in [a.link, a.young_link, a.ness]:
        inputs += [draft / n for n in ['project.txt', 'assets.fma', 'fighter.dat', 'animation.dat', 'costume.dat', 'cpu_profile.fcp']]
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    hashes = {str(x.resolve()): sha(x) for x in inputs if x.exists()}
    out.mkdir(parents=True)
    for source, name in [(a.link, 'link'), (a.young_link, 'young-link'), (a.ness, 'ness')]:
        shutil.copytree(source, out / name)
    cmd = [str(a.compiler.resolve()), '-std=gnu11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
           '-Wno-unused-function', '-Wno-array-parameter', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
           '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt', '-fuse-ld=lld', '-Wl,/opt:ref',
           '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'extern/dolphin/include'),
           '-I' + str(ROOT.parent / 'aurora/include'), str(HERE / 'test.c')]
    cmd += libs + ['-o', str(out / 'test.exe')]
    commands, results = [], []
    for name, command, cwd in [('compile', cmd, build), ('test', [str(out / 'test.exe'), str(out / 'link'), str(out / 'young-link'), str(out / 'ness')], out)]:
        if name == 'test':
            for dll in build.glob('*.dll'):
                shutil.copy2(dll, out / dll.name)
        r = subprocess.run(command, cwd=cwd, capture_output=True, text=True, timeout=90,
                           creationflags=subprocess.CREATE_NO_WINDOW)
        (out / (name + '.log')).write_text(r.stdout + r.stderr)
        commands.append({'stage': name, 'command': command, 'exit': r.returncode})
        results.extend(json.loads(line) for line in r.stdout.splitlines() if line.startswith('{'))
        (out / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n')
        if r.returncode:
            break
    unchanged = all(sha(Path(path)) == value for path, value in hashes.items())
    status = 'PASS' if r.returncode == 0 and unchanged else 'FAIL'
    receipt = {'status': status, 'commands': commands, 'results': results,
               'input_sha256': hashes, 'inputs_unchanged': unchanged,
               'scope': 'Real owned Link/Young Link Arrow descriptors at offsets36/40 and Ness Yoyo at offsets80/84; actual native library importer, HSD graph loaders, material compiler, ID table, matrix traversal, reserved allocation and destruction. No production functions replaced.',
               'limits': ['No metadata admission, Item actor/VM scheduling, renderer, draw, audio, game or whole-match rollback proof.',
                          'Exact standard-class/empty toon-shadow environment only; other graph families are not accepted by this cohort.',
                          'Replacement sequence is explicit fixture orchestration of native primitives, not proof of a production replacement API.',
                          'Global descriptor ID mapping points to the newest duplicate and does not resurrect older mappings when that duplicate is removed.',
                          'SDK duplicate MTXPerspective array bounds warning suppressed; other fixture warnings are errors.']}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'status': status, 'results': results, 'receipt': str(out / 'receipt.json')}))
    print((r.stdout + r.stderr)[-8000:])
    raise SystemExit(r.returncode or (not unchanged))


if __name__ == '__main__':
    main()
