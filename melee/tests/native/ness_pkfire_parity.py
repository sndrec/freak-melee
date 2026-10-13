"""Compare Ness PK Fire on hidden, muted, accelerated, drawn clients.

Coordinate the shared build and runtime slot before invoking this matrix.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from fighter_script_bundle_roundtrip import bundled_id, portrait, run
from ness_pkfire_stock_compare import root_offset

ROOT = Path(__file__).resolve().parents[2]
B, SHIELD = 1 << 9, 1 << 6
FIRE, FLAME = 66, 67


def verify_import(project, template, owned):
    data = (project / 'fighter.dat').read_bytes()
    root = root_offset(data, 'ftDataNess')
    pointer = lambda at: 32 + struct.unpack_from('>I', data, at)[0]
    table = pointer(root + 72)
    bases = {'': pointer(root + 4), 'It_Kind_Ness_PKFire': pointer(pointer(table) + 4),
             'It_Kind_Ness_PKFire_Flame': pointer(pointer(table + 4) + 4)}
    parameters = {name: (kind, value) for kind, name, value in re.findall(
        r'^(float|int|bool) ([\w.]+) = (.+)$',
        (project / 'fighter_parameters.fmp').read_text(), re.M)}
    scope, checked = '', []
    for line in template.read_text().splitlines():
        if line and not line[0].isspace() and not line.startswith('#'):
            scope = re.match(r'article (\w+)', line).group(1) if line.startswith('article ') else ''
        declaration = re.fullmatch(r' *attribute (float|int) (\w+) (\d+)', line)
        if declaration is None:
            continue
        expected_kind, name, offset = declaration.groups()
        key = scope + '.' + name if scope else name
        kind, value = parameters[key]
        assert kind == expected_kind, key
        packed = struct.pack('>f', float(value)) if kind == 'float' else struct.pack('>i', int(value, 0))
        assert data[bases[scope] + int(offset):bases[scope] + int(offset) + 4] == packed, key
        checked.append(key)
    assert len(checked) in (12, 24), checked
    assert not re.search(r'^ *attribute ', owned.read_text(), re.M)
    assert 'item.attr.' not in owned.read_text()
    return checked


def recording(base, samples, left, defense):
    frames = bytearray()
    for tick in range(samples):
        sample = bytearray(base[24:104])
        buttons, x, y = 0, 0, 0
        if left and 155 <= tick < 158:
            x = -40
        if tick == 180:
            buttons, x = B, -80 if left else 80
        struct.pack_into('<Hbb', sample, 8, buttons, x, y)
        buttons2, y2 = 0, 0
        if defense == 'shield' and tick >= 175:
            buttons2 = SHIELD
        elif defense == 'reflect' and tick == 180:
            buttons2 = B
        elif defense == 'absorb' and 175 <= tick < 350:
            buttons2, y2 = B, -80
        struct.pack_into('<Hbb', sample, 24, buttons2, 0, y2)
        frames.extend(sample)
    return base[:24] + frames


def trace_summary(path):
    if not path.is_file():
        return None
    text = path.read_text(errors='replace')
    return {
        'states': {tag: len(re.findall(r'^\[' + tag + r'\]', text, re.M))
                   for tag in ('fighter', 'action', 'item', 'item-event', 'damage', 'rng')},
        'p1_motions': sorted({int(value) for value in re.findall(
            r'^\[action\] sample \d+ P1 .*? motion=(\d+)', text, re.M)}),
        'p2_motions': sorted({int(value) for value in re.findall(
            r'^\[action\] sample \d+ P2 .*? motion=(\d+)', text, re.M)}),
        'item_kinds': sorted({int(value) for value in re.findall(
            r'^\[item\] sample \d+ I\d+ kind=(\d+)', text, re.M)}),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    parser.add_argument('--case', action='append')
    parser.add_argument('--samples', type=int, default=700)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x6E657373)
    parser.add_argument('--tool', type=Path, default=ROOT / 'build/native/native_content_tool.exe')
    parser.add_argument('--exe', type=Path, default=ROOT / 'build/native/melee_native.exe')
    parser.add_argument('--reuse-package-from', type=Path)
    parser.add_argument('--template', type=Path, default=ROOT / 'examples/ness_wip.fms')
    args = parser.parse_args()
    if args.samples < 500:
        parser.error('--samples must be at least 500')
    cases = [('ground-right', False, False, 'idle'), ('ground-left', False, True, 'idle'),
             ('air-right', True, False, 'idle'), ('air-left', True, True, 'idle'),
             ('ground-damage', False, False, 'damage'), ('air-damage', True, False, 'damage'),
             ('ground-shield', False, False, 'shield'), ('ground-reflect', False, False, 'reflect'),
             ('ground-absorb', False, False, 'absorb'), ('air-landing', True, False, 'landing')]
    if args.case:
        unknown = set(args.case) - {row[0] for row in cases}
        if unknown:
            parser.error('Unknown cases: ' + ', '.join(sorted(unknown)))
        cases = [row for row in cases if row[0] in args.case]
    out = args.out_dir.resolve()
    out.mkdir(parents=True, exist_ok=False)
    package_dir = args.reuse_package_from.resolve() if args.reuse_package_from else out
    project, store, owned, image = (package_dir / name for name in
                                  ('project', 'store', 'owned.fms', 'portrait.png'))
    source = args.template.resolve()
    tool, iso = str(args.tool.resolve()), args.iso.resolve()
    if args.reuse_package_from:
        if source.read_bytes() != (package_dir / 'template.fms').read_bytes():
            parser.error('Current template differs from saved package template')
        checked = verify_import(project, source, owned)
        package = (package_dir / 'package.txt').read_text().strip()
    else:
        (out / 'template.fms').write_bytes(source.read_bytes())
        portrait(image)
        run(tool, 'clone-stock', iso, 'Ness', project, 'Ness PK Fire comparison')
        run(tool, 'import-parameters', project, source, owned)
        checked = verify_import(project, source, owned)
        run(tool, 'script-build', project, owned)
        package = bundled_id(run(tool, '--root', store, 'bundle-project', project, image, iso))
        run(tool, '--root', store, 'accept', package, iso)
    (out / 'package.txt').write_text(package)
    base = bytearray((ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes())
    struct.pack_into('<I', base, 16, args.seed)
    results = []
    for name, air, left, defense in cases:
        inputs = out / (name + '.fmr')
        inputs.write_bytes(recording(base, args.samples, left, defense))
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                   '--iso', str(iso), '--inputs', str(inputs), '--stock-fighter', 'ness',
                   '--package-id', package, '--content-root', str(store), '--stocks', '4',
                   '--exe', str(args.exe.resolve()), '--out-dir', str(out / name),
                   '--expect-motion', '357' if air else '356', '--expect-item-kind', str(FIRE)]
        if air:
            command += ['--fixture-airborne', '1,175,-30,45' if defense in ('damage', 'landing') else '1,175,-30,120']
        if defense in ('damage', 'shield', 'reflect', 'absorb'):
            command += ['--fixture-airborne', '2,140,-8,6' if air else '2,140,-26,6']
        if defense == 'damage':
            command += ['--expect-item-kind', str(FLAME), '--expect-damage-port', '2']
        elif defense == 'shield':
            command += ['--expect-port-motion', '2', '181']
        elif defense == 'reflect':
            command += ['--expect-item-owner-transfer', str(FIRE), '1', '2']
        elif defense == 'absorb':
            command += ['--p2', 'human:ness', '--expect-item-event', 'absorb', str(FIRE)]
        elif defense == 'landing':
            command += ['--expect-motion', '43']
        print('Comparing Ness PK Fire: ' + name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        results.append({'case': name, 'exit': code, 'traces': {
            label: trace_summary(out / name / (name + '-' + label + '.log'))
            for label in ('stock', 'clone')}})
        (out / 'results.json').write_text(json.dumps({
            'package': package, 'imported_words': checked, 'seed': args.seed,
            'samples': args.samples, 'cases': results}, indent=2) + '\n')
    return int(any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
