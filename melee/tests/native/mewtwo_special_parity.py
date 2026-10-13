"""Compare authored Mewtwo specials against stock, hidden and drawn.

The matrix checks callback behavior, fighter/item traces, visuals and RNG. It
also verifies every imported tuning word against the independent raw DAT data.
Run only after coordinating the build/runtime test slot with other workers.
"""

import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import run, portrait, bundled_id


ROOT = Path(__file__).resolve().parents[2]
B, A, SHIELD, GRAB, JUMP = 1 << 9, 1 << 8, 1 << 6, 1 << 4, 1 << 10


def root_offset(data, name):
    size, relocations, roots, externs = struct.unpack_from('>4I', data, 4)
    table = 32 + size + relocations * 4
    strings = table + (roots + externs) * 8
    for index in range(roots):
        target, offset = struct.unpack_from('>2I', data, table + index * 8)
        found = data[strings + offset:].split(b'\0', 1)[0].decode()
        if found == name:
            return 32 + target
    raise AssertionError('Missing fighter root ' + name)


def verify_import(project, template, owned):
    data = (project / 'fighter.dat').read_bytes()
    root = root_offset(data, 'ftDataMewtwo')
    pointer = lambda offset: 32 + struct.unpack_from('>I', data, offset)[0]
    base = pointer(root + 4)
    article_table = pointer(root + 72)
    bases = {
        '': base,
        'It_Kind_Mewtwo_Disable': pointer(pointer(article_table) + 4),
        'It_Kind_Mewtwo_ShadowBall': pointer(pointer(article_table + 4) + 4),
    }
    values = {
        name: (kind, value)
        for kind, name, value in re.findall(
            r'^(float|int|bool) ([\w.]+) = (.+)$',
            (project / 'fighter_parameters.fmp').read_text(), re.M)
    }
    scope, checked = '', []
    for line in template.read_text().splitlines():
        if line and not line[0].isspace() and not line.startswith('#'):
            scope = re.match(r'article (\w+)', line).group(1) if line.startswith('article ') else ''
        declaration = re.fullmatch(r' *attribute (float|int|byte) (\w+) (\d+)', line)
        if declaration is None:
            continue
        kind, name, offset = declaration.groups()
        key = scope + '.' + name if scope else name
        value_kind, value = values[key]
        assert ('int' if kind == 'byte' else kind) == value_kind, key
        bits = (struct.pack('>f', float(value)) if kind == 'float' else
                struct.pack('B', int(value)) if kind == 'byte' else
                struct.pack('>i', int(value)))
        start = bases[scope] + int(offset)
        assert data[start:start + len(bits)] == bits, key
        checked.append(key)
    assert len(checked) == 47, checked
    assert not re.search(r'^ *attribute ', owned.read_text(), re.M)
    assert 'item.attr.' not in owned.read_text()
    return checked


def recording(base, samples, variant, airborne=False, left=False, defense='idle'):
    frames = bytearray()
    start = 200 if variant == 'confusion-reflect' else 180
    for tick in range(samples):
        sample = bytearray(base[24:104])
        buttons, x, y = 0, 0, 0
        if left and (140 <= tick < 143 if variant.startswith('jump-') else 155 <= tick < 158):
            # Walk-turn without entering the long turn-run that carries P1
            # off the nearby left ledge before the special input.
            x = -40
        if tick == start and not variant.startswith(('throw-', 'jump-', 'worn-')):
            buttons = B
            if variant == 'disable':
                y = -80
            elif variant.startswith('confusion'):
                x = -80 if left else 80
            elif variant.startswith('teleport-'):
                y = 80
        if variant.startswith('worn-'):
            if tick == 120 and variant in ('worn-normal', 'worn-both'): buttons = A
            if tick == 150 and variant == 'worn-screw': buttons = A
            if tick == 185 and variant == 'worn-both': buttons = A
            if tick in (230, 260): buttons = JUMP
        if variant.startswith('throw-'):
            if tick == 180: buttons = GRAB
            if 210 <= tick <= 220: x = -80 if left else 80
        if variant.startswith('jump-') and tick == 150:
            buttons = JUMP
        if variant.startswith('jump-') and tick == 180:
            buttons = JUMP
            direction = variant.removeprefix('jump-')
            x = {'forward': -80 if left else 80, 'backward': 80 if left else -80, 'neutral': 0}[direction]
        if variant.startswith('teleport-') and 181 <= tick <= 225:
            direction = variant.removeprefix('teleport-')
            x, y = {'up': (0, 80), 'down': (0, -80),
                    'right': (80, 0), 'left': (-80, 0),
                    'diagonal': (56, 56), 'neutral': (0, 0)}[direction]
        if variant == 'short-b' and tick == 255:
            buttons = B
        elif variant == 'short-a' and tick == 255:
            buttons = A
        elif variant == 'cancel-restart':
            if tick == 255:
                buttons = SHIELD
            elif tick in (330, 390):
                buttons = B
        elif variant == 'full-release' and tick == 400:
            buttons = B
        elif variant == 'full-store':
            if tick == 400:
                buttons = SHIELD
            elif tick == 480:
                buttons = B
        elif variant == 'roll-restart':
            if tick == 255:
                buttons, x = SHIELD, 80
            elif tick in (340, 400):
                buttons = B
        elif variant == 'confusion-repeat' and tick in (235, 400):
            buttons, x = B, 80
        struct.pack_into('<Hbb', sample, 8, buttons, x, y)
        # Human Zelda responds through recorded inputs; no desktop key events.
        buttons2 = 0
        if defense in ('shield', 'capture-shield') and tick >= 175:
            buttons2 = SHIELD
        elif defense == 'reflect' and tick == 300:
            buttons2 = B
        elif defense == 'confusion-reflect' and tick == 180:
            buttons2 = B
        struct.pack_into('<Hbb', sample, 24, buttons2, 0, 0)
        frames.extend(sample)
    return base[:24] + frames


def require_retained_reflection(log):
    """Require actual reflection rather than just a live reflector bubble."""
    previous = {}
    for line in log.read_text(errors='replace').splitlines():
        match = re.match(r'^\[item\] sample (\d+) I(\d+) kind=48 .*?'
                         r'owner=P(\d+) .*?vel=\(([-\d.]+),', line)
        if match is None:
            continue
        sample, item, owner, speed = match.groups()
        sample, owner, speed = int(sample), int(owner), float(speed)
        last = previous.get(item)
        if last and sample == last[0] + 1 and last[1] == owner == 2:
            # Fireballs bounce vertically from the floor. A horizontal reversal
            # on the same live projectile confirms an ownership-retaining hit.
            if last[2] < 0 and speed > 0:
                return
        previous[item] = sample, owner, speed
    raise AssertionError('No continuously traced P2 fireball reflected with ownership retained: ' + str(log))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    parser.add_argument('--case', action='append')
    parser.add_argument('--samples', type=int, default=700)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x6D657774)
    parser.add_argument('--tool', type=Path, default=ROOT / 'build/native/native_content_tool.exe')
    parser.add_argument('--exe', type=Path, default=ROOT / 'build/native/melee_native.exe')
    parser.add_argument('--reuse-package-from', type=Path,
                        help='Compare an already accepted package with its exact saved template; no rebuild')
    parser.add_argument('--independent', action='store_true',
                        help='Verify fully authored custom behavior identity in both traces')
    args = parser.parse_args()
    if args.samples < 650:
        parser.error('--samples must be at least 650 for charge/cancel/reentry coverage')
    cases = [
        ('shadow-ground-short-b', 'short-b', False, False, 'idle', [341, 342, 345]),
        ('shadow-ground-short-a', 'short-a', False, False, 'idle', [341, 342, 345]),
        ('shadow-ground-left', 'short-b', False, True, 'idle', [341, 342, 345]),
        ('shadow-cancel-restart', 'cancel-restart', False, False, 'idle', [344, 345]),
        ('shadow-roll-restart', 'roll-restart', False, False, 'idle', [341, 345]),
        ('shadow-full-release', 'full-release', False, False, 'idle', [343, 345]),
        ('shadow-full-store', 'full-store', False, False, 'idle', [343, 344, 345]),
        ('shadow-air-short', 'short-b', True, False, 'idle', [346, 347]),
        ('shadow-shield', 'short-b', False, False, 'shield', [345]),
        ('shadow-reflector', 'short-b', False, False, 'reflect', [345]),
        ('disable-ground', 'disable', False, False, 'idle', [359]),
        ('disable-ground-left', 'disable', False, True, 'idle', [359]),
        ('disable-air-land', 'disable', True, False, 'idle', [360]),
        ('disable-shield', 'disable', False, False, 'shield', [359]),
        ('disable-target', 'disable', False, False, 'target', [359]),
        ('teleport-ground-right', 'teleport-right', False, False, 'idle', [353, 354, 355]),
        ('teleport-ground-left', 'teleport-left', False, False, 'idle', [353, 354, 357, 358]),
        ('teleport-ground-up', 'teleport-up', False, False, 'idle', [353, 357, 358]),
        ('teleport-ground-neutral', 'teleport-neutral', False, False, 'idle', [353, 357, 358]),
        ('teleport-air-right', 'teleport-right', True, False, 'idle', [356, 357, 358]),
        ('teleport-air-left', 'teleport-left', True, False, 'idle', [356, 357, 358]),
        ('teleport-air-up', 'teleport-up', True, False, 'idle', [356, 357, 358]),
        ('teleport-air-neutral', 'teleport-neutral', True, False, 'idle', [356, 357, 358]),
        ('teleport-air-diagonal', 'teleport-diagonal', True, False, 'idle', [356, 357, 358]),
        ('teleport-air-down-land', 'teleport-down', True, False, 'idle', [356, 357, 354, 355]),
        ('confusion-ground', 'confusion', False, False, 'idle', [351]),
        ('confusion-ground-left', 'confusion', False, True, 'idle', [351]),
        ('confusion-air', 'confusion', True, False, 'idle', [352]),
        ('confusion-air-land', 'confusion-land', True, False, 'idle', [352, 351]),
        ('confusion-ground-capture', 'confusion', False, False, 'capture', [351]),
        ('confusion-ground-left-capture', 'confusion', False, True, 'capture', [351]),
        ('confusion-air-capture', 'confusion', True, False, 'capture', [352]),
        ('confusion-shield-capture', 'confusion', False, False, 'capture-shield', [351]),
        ('confusion-air-repeat', 'confusion-repeat', True, False, 'idle', [352]),
        ('confusion-reflect', 'confusion-reflect', False, False, 'confusion-reflect', [351]),
        ('throw-forward-right', 'throw-forward', False, False, 'grab-target', [212, 219]),
        ('throw-forward-left', 'throw-forward', False, True, 'grab-target', [212, 219]),
        ('jump-right-forward', 'jump-forward', True, False, 'idle', [27]),
        ('jump-right-backward', 'jump-backward', True, False, 'idle', [28]),
        ('jump-left-forward', 'jump-forward', True, True, 'idle', [27]),
        ('jump-left-backward', 'jump-backward', True, True, 'idle', [28]),
        ('jump-neutral', 'jump-neutral', True, False, 'idle', [27]),
        ('worn-bunny-double-jump', 'worn-normal', False, False, 'hood', [27]),
        ('held-screw-double-jump', 'worn-screw', False, False, 'screw', [154, 155]),
        ('worn-bunny-screw-double-jump', 'worn-both', False, False, 'both', [154, 155]),
    ]
    if args.case:
        unknown = set(args.case) - {row[0] for row in cases}
        if unknown:
            parser.error('Unknown cases: ' + ', '.join(sorted(unknown)))
        cases = [row for row in cases if row[0] in args.case]
    directory = args.out_dir.resolve()
    directory.mkdir(parents=True, exist_ok=False)
    tool = str(args.tool.resolve())
    package_directory = args.reuse_package_from.resolve() if args.reuse_package_from else directory
    project, store, owned, image = (package_directory / name for name in
                                  ('project', 'store', 'owned.fms', 'portrait.png'))
    iso = args.iso.resolve()
    source = ROOT / 'examples/mewtwo_wip.fms'
    if args.reuse_package_from:
        if source.read_bytes() != (package_directory / 'template.fms').read_bytes():
            parser.error('Current template differs from the saved accepted package template')
        checked = verify_import(project, source, owned)
        package = (package_directory / 'package.txt').read_text().strip()
    else:
        (directory / 'template.fms').write_bytes(source.read_bytes())
        portrait(image)
        run(tool, 'clone-stock', iso, 'Mewtwo', project, 'Mewtwo special comparison')
        run(tool, 'import-parameters', project, source, owned)
        checked = verify_import(project, source, owned)
        run(tool, 'script-build', project, owned)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
    (directory / 'package.txt').write_text(package)
    base = bytearray((ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes())
    struct.pack_into('<I', base, 16, args.seed)
    results = []
    for name, variant, air, left, defense, motions in cases:
        inputs = directory / (name + '.fmr')
        inputs.write_bytes(recording(base, args.samples, variant, air, left, defense))
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                   '--iso', str(iso), '--inputs', str(inputs), '--stock-fighter', 'mewtwo',
                   '--package-id', package, '--content-root', str(store), '--stocks', '4',
                   '--exe', str(args.exe.resolve()),
                   '--out-dir', str(directory / name)]
        if args.independent:
            command.append('--independent')
        if air and not variant.startswith('jump-'):
            height = (45 if variant == 'teleport-down' else
                      10 if variant == 'confusion-land' else
                      150 if variant == 'confusion-repeat' else 120)
            command += ['--fixture-airborne', f'1,175,-30,{height}']
        if variant == 'confusion-repeat':
            command += ['--fixture-airborne', '1,390,-30,120']
        if variant.startswith('worn-'):
            if defense in ('hood', 'both'):
                command += ['--fixture-item', '31,100,-60,3', '--expect-item-kind', '31']
            if defense in ('screw', 'both'):
                tick = 170 if defense == 'both' else 130
                command += ['--fixture-item', f'30,{tick},-60,3', '--expect-item-kind', '30']
        if defense == 'grab-target':
            command += ['--fixture-airborne', '2,140,-72,6' if left else '2,140,-48,6',
                        '--expect-port-motion', '2', '239', '--expect-damage-port', '2',
                        '--expect-item-kind', '112']
        if defense.startswith('capture'):
            position = '2,175,-18,145' if air else ('2,140,-77,6' if left else '2,140,-48,6')
            command += ['--fixture-airborne', position]
            command += ['--expect-port-motion', '2', '304' if air else '303',
                        '--expect-port-motion', '2', '38', '--expect-damage-port', '2']
        if defense == 'confusion-reflect':
            command += ['--p2', 'human:mario', '--expect-item-kind', '48',
                        '--fixture-airborne', '2,120,0,6']
        if variant == 'disable' and defense in ('shield', 'target'):
            command += ['--fixture-airborne', '2,140,-35,6']
        for motion in motions:
            command += ['--expect-motion', str(motion)]
        # USA v1.02 item kinds verified by stock enum and initial traces.
        if variant in ('short-a', 'short-b', 'cancel-restart', 'full-release', 'full-store', 'roll-restart', 'disable'):
            command += ['--expect-item-kind', '110' if variant == 'disable' else '112']
        if defense == 'reflect':
            command += ['--expect-item-owner-transfer', '112', '1', '2']
        if defense == 'shield':
            command += ['--expect-port-motion', '2', '181']
        if defense == 'target':
            command += ['--expect-port-motion', '2', '300', '--expect-damage-port', '2']
        print('Comparing Mewtwo: ' + name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        counts = {}
        for label in ('stock', 'clone'):
            log = directory / name / (name + '-' + label + '.log')
            if log.exists():
                observed = re.findall(r'^\[action\] sample \d+ P1 .*?motion=(\d+) ',
                                      log.read_text(errors='replace'), re.M)
                counts[label] = {motion: observed.count(motion)
                                 for motion in sorted(set(observed), key=int)}
                if code == 0 and defense == 'confusion-reflect':
                    try:
                        require_retained_reflection(log)
                    except AssertionError as error:
                        print(error, flush=True)
                        code = 1
                if code == 0 and variant.startswith('worn-'):
                    text = log.read_text(errors='replace')
                    kinds = (31, 30) if defense == 'both' else (31,) if defense == 'hood' else (30,)
                    for kind in kinds:
                        if re.search(r'^\[item\] sample 26[1-9] I\d+ kind=' + str(kind) + r' state=2 .*?owner=P1 ', text, re.M) is None:
                            print('Worn fixture did not retain required item ' + str(kind) + ' through second jump: ' + str(log))
                            code = 1
                if code == 0 and variant == 'confusion-repeat':
                    entries = [int(tick) for tick in re.findall(
                        r'^\[fighter\] sample (\d+) P1 motion=352 frame=3f800000 ',
                        log.read_text(errors='replace'), re.M)]
                    if not all(any(begin <= tick < end for tick in entries)
                               for begin, end in ((180, 220), (235, 280), (400, 450))):
                        print('Repeated airborne Confusion did not enter all three requested windows: ' + str(log))
                        code = 1
        results.append(dict(case=name, exit=code, motion_counts=counts))
        (directory / 'results.json').write_text(json.dumps(
            dict(package=package, imported_words=checked, seed=args.seed,
                 samples=args.samples, cases=results), indent=2) + '\n')
    return int(any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
