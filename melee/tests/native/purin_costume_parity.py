"""Compare an authored Jigglypuff against stock in all five costumes.

This manual runtime check keeps copied disc assets in the output directory.
Cloth hats exercise 64-bit field access, accessory descriptor import, shared
render/visibility operations and scene cleanup. Recorded Rollout contact must
also occur; missing assets or missed inputs must not produce a passing result.
"""

import argparse
import json
import re
import struct
import subprocess
import sys
from pathlib import Path

import hashlib

from fighter_script_bundle_roundtrip import portrait, run

ROOT = Path(__file__).resolve().parents[2]


def disc_files(stream):
    stream.seek(0x424)
    offset, size = struct.unpack('>II', stream.read(8))
    stream.seek(offset)
    fst = stream.read(size)
    count = struct.unpack_from('>I', fst, 8)[0]
    strings = count * 12
    files = {}
    for index in range(1, count):
        kind, offset, size = struct.unpack_from('>III', fst, index * 12)
        if kind >> 24:
            continue
        start = strings + (kind & 0xffffff)
        end = fst.index(b'\0', start)
        files[fst[start:end].decode()] = (offset, size)
    return files


def independent_costume(data, tag):
    # The custom fighter has one costume slot. Rename just its main public
    # symbols to that slot's names, preserving every byte offset and hat root.
    for suffix in ('joint', 'matanim_joint'):
        old = ('PlyPurin5K' + tag + '_Share_' + suffix).encode() + b'\0'
        new = ('PlyPurin5K_Share_' + suffix).encode() + b'\0'
        assert data.count(old) == 1
        data = data.replace(old, new + b'\0' * (len(old) - len(new)))
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    args = parser.parse_args()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    iso = args.iso.resolve()
    tool = str(ROOT / 'build/native/native_content_tool.exe')
    project, store = batch / 'project', batch / 'store'
    image, owned = batch / 'portrait.png', batch / 'owned.fms'
    portrait(image)
    run(tool, 'clone-stock', iso, 'Jigglypuff', project, 'Puff costume comparison')
    print(run(tool, 'import-parameters', project, ROOT / 'examples/purin_wip.fms', owned), flush=True)
    print(run(tool, 'script-build', project, owned), flush=True)
    package = re.search(r'Cached source fighter ([a-f0-9]{64})',
        run(tool, '--root', store, 'package-project', project, image))[1]
    print(run(tool, '--root', store, 'verify-package', package), flush=True)
    print(run(tool, '--root', store, 'accept', package), flush=True)
    (batch / 'package.txt').write_text(package)
    skins = {}
    with iso.open('rb') as stream:
        files = disc_files(stream)
        for color, tag in enumerate(('Re', 'Bu', 'Gr', 'Ye'), 1):
            offset, size = files['PlPr' + tag + '.dat']
            stream.seek(offset)
            data = stream.read(size)
            assert len(data) == size
            path = batch / f'costume-{color}.dat'
            path.write_bytes(independent_costume(data, tag))
            output = run(tool, '--root', store, 'import-custom-costume', path,
                         package, 'Puff costume ' + tag, str(color))
            print(output, flush=True)
            skins[color] = re.search(r'ID: ([a-f0-9]{64})', output).group(1)
    (batch / 'skins.json').write_text(json.dumps(skins, indent=2))
    inputs = ROOT / 'tests/native/parity_inputs/purin-rollout-short-ground-right-inputs.fmr'
    files = [Path(__file__), ROOT / 'tests/native/fighter_script_bundle_roundtrip.py',
        ROOT / 'tools/run_fighter_parity.py', ROOT / 'tools/compare_fighter_parity.py',
        ROOT / 'build/native/melee_native.exe', Path(tool), owned, image, inputs,
        ROOT / 'src/melee/ft/ftdrawcommon.c', ROOT / 'src/melee/ft/ftparts.c',
        ROOT / 'src/melee/ft/native_fighter_attachment.c',
        *[p for directory in [project, store] for p in directory.rglob('*') if p.is_file()],
        *batch.glob('costume-*.dat')]
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    (batch / 'manifest.json').write_text(json.dumps(hashes, indent=2) + '\n')
    results = []
    for color in range(5):
        name = 'costume-' + str(color)
        print('Comparing', name, flush=True)
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                   '--iso', str(iso), '--inputs',
                   str(inputs),
                   '--stock-fighter', 'jigglypuff', '--stock-color', str(color),
                   '--package-id', package, '--content-root', str(store),
                   '--stocks', '2', '--out-dir', str(batch / 'traces' / name), '--independent',
                   '--expect-motion', '346', '--expect-motion', '350',
                   '--expect-motion', '362', '--expect-damage-port', '2']
        if color:
            command.extend(('--clone-skin', skins[color], '--expect-attachment'))
        log = batch / (name + '-result.log')
        with log.open('wb') as stream:
            result = subprocess.run(command, cwd=ROOT, stdout=stream, stderr=subprocess.STDOUT,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
        code = result.returncode
        clone_log = batch / 'traces' / name / (inputs.stem + '-clone.log')
        actors = re.findall(r'Live fighter (\d+) kind (\d+): (\d+) completed / (\d+) requested interpreted ticks, state=([0-9a-f]+), faulted=(\d+)',
            clone_log.read_text(errors='replace') if clone_log.exists() else '')
        if len(actors) != 1 or actors[0][5] != '0' or int(actors[0][3]) != 1200:
            code = 1
        results.append(dict(case=name, package=package, exit=code, actors=actors, log=str(log)))
        (batch / 'summary.json').write_text(json.dumps(results, indent=2))
        print(f'{name}: exit {code}', flush=True)
    if any(hashlib.sha256(Path(p).read_bytes()).hexdigest() != h for p, h in hashes.items()):
        raise AssertionError('Input changed during costume batch')
    print('Passed', sum(result['exit'] == 0 for result in results), 'of', len(results), flush=True)
    return int(any(result['exit'] for result in results))


if __name__ == '__main__':
    sys.exit(main())
