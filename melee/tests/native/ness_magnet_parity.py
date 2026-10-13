"""Compare PSI Magnet using hidden, muted, accelerated, drawn clients.

Requires a coordinated runtime slot and an already frozen compiled package.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

from ness_pkfire_parity import verify_import, trace_summary

ROOT = Path(__file__).resolve().parents[2]
B = 1 << 9
CASES = (
    ('ground-short-right', False, False, 181, 'idle'),
    ('ground-short-left', False, True, 181, 'idle'),
    ('ground-hold-right', False, False, 310, 'idle'),
    ('ground-hold-left', False, True, 310, 'idle'),
    ('air-short', True, False, 181, 'idle'),
    ('air-hold', True, False, 310, 'idle'),
    ('air-end-landing', True, False, 181, 'end-landing'),
    ('air-hold-landing', True, False, 310, 'hold-landing'),
    ('ground-absorb', False, False, 310, 'absorb'),
    ('air-absorb', True, False, 310, 'absorb'),
    ('air-heal', True, False, 310, 'heal'),
)


def recording(base, samples, left, release, contact):
    frames = bytearray()
    for tick in range(samples):
        sample = bytearray(base[24:104])
        x = -40 if left and 155 <= tick < 158 else 0
        buttons, y = (B, -80) if 180 <= tick < release else (0, 0)
        struct.pack_into('<Hbb', sample, 8, buttons, x, y)
        fire_tick = 250 if contact == 'heal' else 195
        buttons2, x2 = (B, -80) if contact in ('absorb', 'heal') and tick == fire_tick else (0, 0)
        struct.pack_into('<Hbb', sample, 24, buttons2, x2, 0)
        frames.extend(sample)
    return base[:24] + frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--out-dir', type=Path, required=True)
    parser.add_argument('--package-from', type=Path, required=True)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--case', action='append')
    parser.add_argument('--samples', type=int, default=700)
    parser.add_argument('--seed', type=lambda s: int(s, 0), default=0x6E657373)
    args = parser.parse_args()
    if args.samples < 500:
        parser.error('--samples must be at least 500')
    cases = list(CASES)
    if args.case:
        unknown = set(args.case) - {row[0] for row in cases}
        if unknown:
            parser.error('Unknown cases: ' + ', '.join(sorted(unknown)))
        cases = [row for row in cases if row[0] in args.case]
    saved = args.package_from.resolve()
    project, store, template, owned = (saved / name for name in
                                     ('project', 'store', 'template.fms', 'owned.fms'))
    checked = verify_import(project, template, owned)
    assert len(checked) == 24
    package = (saved / 'package.txt').read_text().strip()
    args.out_dir.mkdir(parents=True, exist_ok=False)
    base = bytearray((ROOT / 'tests/native/parity_inputs/drmario-special-neutral-ground-inputs.fmr').read_bytes())
    struct.pack_into('<I', base, 16, args.seed)
    results = []
    for name, air, left, release, contact in cases:
        inputs = args.out_dir / (name + '.fmr')
        inputs.write_bytes(recording(base, args.samples, left, release, contact))
        command = [sys.executable, str(ROOT / 'tools/run_fighter_parity.py'),
                   '--iso', str(args.iso.resolve()), '--inputs', str(inputs.resolve()),
                   '--stock-fighter', 'ness', '--package-id', package,
                   '--content-root', str(store), '--stocks', '4',
                   '--exe', str(args.exe.resolve()), '--out-dir', str((args.out_dir / name).resolve()),
                   '--expect-motion', '372' if air else '367']
        if air:
            height = {'end-landing': 40, 'hold-landing': 15, 'absorb': 45}.get(contact, 180 if release > 181 else 120)
            command += ['--fixture-airborne', f'1,175,-30,{height}']
        if contact == 'end-landing':
            command += ['--expect-motion', '375', '--expect-motion', '370']
        elif contact == 'hold-landing':
            command += ['--expect-motion', '373', '--expect-motion', '368', '--expect-motion', '370']
        elif contact in ('absorb', 'heal'):
            command += ['--p2', 'human:ness', '--fixture-airborne',
                        ('2,250,-8,100' if contact == 'heal' else '2,175,-8,90') if air else '2,140,-10,6',
                        '--expect-item-event', 'absorb', '66', '--expect-motion', '374' if air else '369']
        else:
            command += ['--expect-motion', '373' if air else '368',
                        '--expect-motion', '375' if air else '370']
        print('Comparing Ness PSI Magnet: ' + name, flush=True)
        code = subprocess.run(command, cwd=ROOT).returncode
        if code == 0 and contact == 'heal':
            for label in ('stock', 'clone'):
                text = (args.out_dir / name / (name + '-' + label + '.log')).read_text()
                changes = re.findall(r'^\[damage\] sample \d+ P1 ([0-9.]+) -> ([0-9.]+)', text, re.M)
                if not any(float(after) < float(before) for before, after in changes):
                    print('Expected observed P1 healing in ' + label, flush=True)
                    code = 1
        results.append({'case': name, 'exit': code, 'traces': {
            label: trace_summary(args.out_dir / name / (name + '-' + label + '.log'))
            for label in ('stock', 'clone')}})
        (args.out_dir / 'results.json').write_text(json.dumps({
            'package': package, 'imported_words': checked, 'seed': args.seed,
            'samples': args.samples, 'cases': results}, indent=2) + '\n')
    return int(any(row['exit'] for row in results))


if __name__ == '__main__':
    raise SystemExit(main())
