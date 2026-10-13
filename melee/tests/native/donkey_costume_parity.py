"""Compare authored DK punch contact and aerial cargo throws in all costumes.

The source DATs come from the retail ISO. Only two public symbol names change
for the custom fighter's single costume slot; models, textures and animations
are not reauthored. Hidden clients retain normal drawing.
"""

import argparse
import datetime
import json
from pathlib import Path
import re
import subprocess
import sys
from fighter_script_bundle_roundtrip import bundled_id, portrait, run
from donkey_special_parity import recording as special_recording, check_coverage
from donkey_cargo_throw_parity import recording as cargo_recording, covered

ROOT = Path(__file__).resolve().parents[2]


def independent_costume(data, tag):
    for suffix in ('joint', 'matanim_joint'):
        old = ('PlyDonkey5K' + tag + '_Share_' + suffix).encode() + b'\0'
        new = ('PlyDonkey5K_Share_' + suffix).encode() + b'\0'
        assert data.count(old) == 1
        data = data.replace(old, new + b'\0' * (len(old) - len(new)))
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--out-dir', type=Path, default=ROOT / '.local' /
                        ('donkey-costume-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S')))
    args = parser.parse_args()
    batch = args.out_dir.resolve()
    batch.mkdir(parents=True, exist_ok=False)
    iso = args.iso.resolve()
    tool = str(ROOT / 'build/native/native_content_tool.exe')
    project, store = batch / 'project', batch / 'store'
    image, owned = batch / 'portrait.png', batch / 'owned.fms'
    portrait(image)
    run(tool, 'clone-stock', iso, 'Donkey Kong', project, 'DK costume comparison')
    run(tool, 'import-parameters', project, ROOT / 'examples/donkey_wip.fms', owned)
    run(tool, 'script-build', project, owned)
    package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
    run(tool, '--root', store, 'accept', package, iso)
    (batch / 'package.txt').write_text(package)
    skins = {}
    for color, tag in enumerate(('Bk', 'Re', 'Bu', 'Gr'), 1):
        original = batch / f'retail-{color}.dat'
        run(tool, 'extract-stock', iso, 'Donkey Kong', color, original)
        path = batch / f'costume-{color}.dat'
        path.write_bytes(independent_costume(original.read_bytes(), tag))
        output = run(tool, '--root', store, 'import-custom-costume', path, package, 'DK costume ' + tag)
        skins[color] = re.search(r'ID: ([a-f0-9]{64})', output).group(1)
    (batch / 'skins.json').write_text(json.dumps(skins, indent=2) + '\n')
    punch, cargo = batch / 'punch-contact.fmr', batch / 'cargo-air-back-left.fmr'
    special_recording(punch, 'neutral', 'ground', 'right', False, True, 'release')
    cargo_recording(cargo, 'back', 'air', 'left')
    results = []
    for color in range(5):
        for scenario, inputs, motion in (('punch-contact', punch, 372), ('cargo-air-back-left', cargo, 366)):
            name = f'color-{color}-{scenario}'
            print('Comparing ' + name, flush=True)
            directory = batch / name
            case_inputs = batch / (name + '.fmr')
            case_inputs.write_bytes(inputs.read_bytes())
            command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                       '--iso', str(iso), '--inputs', str(case_inputs), '--stock-fighter', 'donkeykong',
                       '--stock-color', str(color), '--package-id', package, '--content-root', str(store),
                       '--stocks', '2', '--out-dir', str(directory), '--expect-motion', str(motion),
                       '--expect-damage-port', '2']
            if color: command.extend(('--clone-skin', skins[color]))
            result = subprocess.run(command, cwd=ROOT)
            exit_code = result.returncode
            if not exit_code:
                valid = check_coverage(directory, name, 'neutral', 'ground', 'right', False, 'release') \
                    if scenario == 'punch-contact' else covered(directory, name, motion, 'left', 'air')
                if not valid: exit_code = 1
            results.append(dict(case=name, scenario=scenario, color=color, package=package,
                                skin=skins.get(color), exit=exit_code))
            (batch / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
    return int(any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
