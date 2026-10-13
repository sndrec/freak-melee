"""Hidden real-library owned secondary animation descriptor import cohort; no shared build/game."""
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
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    out.mkdir(parents=True)
    snapshot = out / 'source'
    source_hashes = {}
    def copy(path, destination):
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, destination)
        source_hashes[str(path.resolve())] = sha(destination)
    for folder in [ROOT / 'src', ROOT / 'extern/dolphin/include', ROOT.parent / 'aurora/include']:
        for path in folder.rglob('*'):
            if path.is_file() and path.suffix in ['.c', '.h', '.hpp']:
                relative = path.relative_to(ROOT) if path.is_relative_to(ROOT) else Path('aurora/include') / path.relative_to(ROOT.parent / 'aurora/include')
                copy(path, snapshot / relative)
    for path in [HERE / 'test.c', HERE / 'run.py', HERE / 'README.md',
                 ROOT / 'tests/native/article_model_roots_import_test.c',
                 ROOT / 'tests/native/article_animation_array_import_test.c']:
        copy(path, snapshot / path.relative_to(ROOT))
    copy(build / 'build.ninja', out / 'build.ninja')
    for dll in build.glob('*.dll'):
        copy(dll, out / dll.name)
    linked = []
    for library in libs:
        path = build / library
        if path.is_file():
            destination = out / 'link' / path.name
            copy(path, destination)
            linked.append(str(destination))
        else:
            linked.append(library)
    for draft, name in [(a.link, 'link-draft'), (a.young_link, 'young-link-draft'), (a.ness, 'ness-draft')]:
        shutil.copytree(draft, out / name)
        for path in draft.rglob('*'):
            if path.is_file(): source_hashes[str(path.resolve())] = sha(out / name / path.relative_to(draft))
    own = snapshot / HERE.relative_to(ROOT)
    cmd = [str(a.compiler.resolve()), '-std=gnu11', '-O2', '-g', '-Wall', '-Wextra', '-Werror',
           '-Wno-unused-function', '-Wno-array-parameter', '-DMELEE_NATIVE=1', '-D_CRT_SECURE_NO_WARNINGS',
           '-D_DLL', '-D_MT', '-Xclang', '--dependent-lib=msvcrt', '-fuse-ld=lld', '-Wl,/opt:ref',
           '-I' + str(snapshot / 'src'), '-I' + str(snapshot / 'extern/dolphin/include'),
           '-I' + str(snapshot / 'aurora/include'), str(own / 'test.c'),
           str(snapshot / 'tests/native/article_model_roots_import_test.c'),
           str(snapshot / 'tests/native/article_animation_array_import_test.c'),
           str(snapshot / 'src/melee/it/itarticle_data.c'),
           str(snapshot / 'src/melee/it/native_article_animation_demand.c'),
           str(snapshot / 'src/melee/ft/native_source_entity_animations.c'),
           str(snapshot / 'src/melee/ft/native_source_entity_assets.c'),
           str(snapshot / 'src/melee/ft/native_source_entity_models.c'),
           str(snapshot / 'src/melee/ft/native_script_model_metadata.c')]
    cmd += linked + ['-o', str(out / 'test.exe')]
    hashes = {str(path): sha(path) for path in out.rglob('*') if path.is_file()}
    hashes[str(a.compiler.resolve())] = sha(a.compiler.resolve())
    commands, results = [], []
    for name, command, cwd in [('compile', cmd, build), ('test', [str(out / 'test.exe'), str(out / 'link-draft'), str(out / 'young-link-draft'), str(out / 'ness-draft')], out)]:
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
               'source_snapshot_sha256': source_hashes,
               'live_sources_match_snapshot': all(Path(path).exists() and sha(Path(path)) == value for path, value in source_hashes.items()),
               'link_provenance': 'Copied already-built actual native libraries/DLLs; current typed Article bundle/source entity import, animation census and model metadata grammar units compiled explicitly from private source snapshot. Other linked production units are not rebuilt by this runner.',
               'scope': 'Real owned Link/Young Link Boomerang and Ness material-only Yo-yo animation/secondary root descriptors imported before HSD heap/class initialization; exact immutable owner tables/cache identities and per-bundle descriptor census observations; typed array/NULL/error and sparse/stream/cycle cases, actual stack-leaf FObj scalar evaluation, plus existing explicitly synthetic low-level regressions.',
               'limits': ['No native animation pool object allocation, graph attachment/playback or full channel/value admission. Native scalar evaluation uses a stack FObj and recording callback only.',
                          'No Item actor/VM, renderer/shader/GPU/audio/game/page-history acceptance.',
                          'Compiler metadata identity and malformed component syntax are checked here; live playback is covered separately by the Item VM fixture.',
                          'Import names use lexical identifier shape; compiler keyword/symbol admission is separate (and/not import but fail compiler namespace validation).',
                          'Duplicate owner inputs use a controlled facts alias of an actual retail Article.',
                          'SDK duplicate MTXPerspective array bounds warning suppressed; other fixture warnings are errors.']}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({'status': status, 'results': results, 'receipt': str(out / 'receipt.json')}))
    print((r.stdout + r.stderr)[-8000:])
    raise SystemExit(r.returncode or (not unchanged))


if __name__ == '__main__':
    main()
