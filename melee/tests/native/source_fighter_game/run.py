"""Hidden, muted stock/source match comparisons using a batch of authored inputs."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def sha(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def records(text, pattern):
    return {tuple(m.groups()[:-1]): dict(f.split('=', 1) for f in m.groups()[-1].split())
            for m in re.finditer(pattern, text, re.M)}


def comparison(left, right):
    differences = {}
    for record in sorted(left.keys() | right.keys()):
        a, b = left.get(record, {}), right.get(record, {})
        for field in sorted(a.keys() | b.keys()):
            if a.get(field) != b.get(field):
                row = differences.setdefault(field, {'count': 0, 'first': {
                    'record': record, 'stock': a.get(field), 'source': b.get(field)}})
                row['count'] += 1
    return {'equal': left == right, 'stock_records': len(left),
            'source_records': len(right), 'differences': differences}


def game(command, directory, mode, samples, expected):
    logs = [directory / (mode + suffix) for suffix in ('.stdout.log', '.stderr.log')]
    begin = time.monotonic()
    timed_out = False
    with logs[0].open('wb') as stdout, logs[1].open('wb') as stderr:
        process = subprocess.Popen(command, cwd=ROOT, stdout=stdout, stderr=stderr,
                                   creationflags=subprocess.CREATE_NO_WINDOW)
        print(f'{directory.name} {mode}: hidden muted PID {process.pid}', flush=True)
        try:
            code = process.wait(timeout=180)
        except subprocess.TimeoutExpired:
            timed_out = True
            process.kill()
            code = process.wait()
    stdout, stderr = [p.read_text(errors='replace') for p in logs]
    actions = re.findall(r'^\[action\] sample (\d+) P(\d+) kind=\d+ behavior=\d+ '
                         r'motion=(\d+) anim=(-?\d+) frame=([^\n]+)', stderr, re.M)
    fighters = records(stderr, r'^\[fighter\] sample (\d+) P(\d+) (.*)$')
    items = records(stderr, r'^\[item\] sample (\d+) I(\d+) (.*)$')
    visible = {key: {k: v for k, v in row.items()
                    if k in ('kind', 'state', 'owner', 'pos', 'vel')}
               for key, row in items.items()}
    return {'exit': code, 'timed_out': timed_out, 'elapsed': time.monotonic() - begin,
            'command': command, 'eof': f'[replay] End after {samples} controller samples.' in stderr,
            'script_fault': bool(re.search(r'^\[script\] (?:Fighter \d+ event|Item \d+ function)'
                                          r'|faulted=[1-9]', stdout + stderr, re.M)),
            'live': re.findall(r'^\[script\] Live fighter [^\n]+', stdout, re.M),
            'entered': not expected or any(a[1] == '1' and int(a[2]) in expected for a in actions),
            'actions': actions, 'fighters': fighters, 'items': visible,
            'item_fields': records(stderr, r'^\[item-fields\] sample (\d+) I(\d+) (.*)$'),
            'logs': {str(p): sha(p) for p in logs}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('exe', 'iso', 'content-root', 'source', 'draft', 'cases', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--package', required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.drive.upper() != 'A:' or output.exists():
        parser.error('Use a fresh private output directory on A:')
    if not re.fullmatch('[a-f0-9]{64}', args.package):
        parser.error('Package must be a complete content ID')
    packet = args.content_root / 'cache' / (args.package + '.fmc')
    source = args.source.read_bytes()
    if source != (args.draft / 'behavior.fms').read_bytes() or source not in packet.read_bytes():
        parser.error('Draft and packet must contain the unchanged canonical source')
    cases = json.loads(args.cases.read_text())
    samples = cases['samples']
    files = [args.exe, args.iso, args.source, args.cases, packet, Path(__file__)]
    files += [p for p in args.draft.rglob('*') if p.is_file()]
    files += [p for p in (ROOT / 'src').rglob('*') if p.suffix in ('.c', '.h', '.cpp')]
    pins = {str(p.resolve()): sha(p) for p in files}
    output.mkdir(parents=True)
    results = []
    for case in cases['cases']:
        directory = output / case['name']
        directory.mkdir()
        replay = directory / 'input.fmreplay'
        data = bytearray(struct.pack('<8sQII', b'FMREPLAY', 0, 0x4D45584C, 1))
        for sample in range(samples):
            pads = bytearray(64)
            for event in case.get('inputs', []):
                if event['start'] <= sample < event['start'] + event.get('hold', 1):
                    struct.pack_into('<Hbb', pads, 0, event['buttons'], event.get('x', 0), event.get('y', 0))
            data.extend(bytes(8) + pads + bytes(8))
        replay.write_bytes(data)
        pair = {}
        for mode in ('stock', 'source'):
            command = [str(args.exe.resolve()), '--iso', str(args.iso.resolve()), '--match',
                       '--stage', 'final-destination', '--p1',
                       'human:custom' if mode == 'source' else 'human:' + cases['stock'],
                       '--p2', 'human:mario', '--p3', 'off', '--p4', 'off', '--mode', 'stock',
                       '--stocks', '1', '--time', '0', '--items', 'off', '--hidden', '--mute', '--fast',
                       '--replay-inputs', str(replay), '--trace-actions', '--trace-fighter-state']
            if mode == 'source':
                command += ['--content-root', str(args.content_root.resolve()), '--p1-fighter', args.package]
            pair[mode] = game(command, directory, mode, samples, case.get('motions', []))
        comparisons = {field: comparison(pair['stock'][field], pair['source'][field])
                       for field in ('fighters', 'items', 'item_fields')}
        actions_equal = pair['stock']['actions'] == pair['source']['actions']
        healthy = all(r['exit'] == 0 and r['eof'] and r['entered'] and
                      not r['timed_out'] and not r['script_fault'] for r in pair.values())
        healthy = healthy and bool(pair['source']['live']) and all(
            sum(k[1] == '1' for k in r['fighters']) == samples for r in pair.values())
        row = {'case': case['name'], 'status': 'PASS traced comparison' if healthy and
               actions_equal and comparisons['fighters']['equal'] and comparisons['items']['equal']
               else 'FAIL traced comparison', 'action_timeline_equal': actions_equal,
               'comparisons': comparisons, 'input_sha256': sha(replay),
               'runs': {mode: {k: v for k, v in r.items() if k not in
                              ('fighters', 'items', 'item_fields')} for mode, r in pair.items()}}
        results.append(row)
        print(json.dumps({'case': row['case'], 'status': row['status'],
                          'actions_equal': actions_equal, 'execution_healthy': healthy,
                          'first_differences': {k: list(v['differences'])[:5]
                                                for k, v in comparisons.items()}}), flush=True)
        unchanged = all(sha(Path(p)) == value for p, value in pins.items())
        receipt = {'status': 'PASS' if unchanged and len(results) == len(cases['cases']) and
                   all(r['status'].startswith('PASS') for r in results) else 'INCOMPLETE or FAIL',
                   'inputs': pins, 'inputs_unchanged': unchanged, 'results': results,
                   'limits': ['Inputs-only playback does not verify RNG or replay digests.',
                              'Rounded emitted fields are not complete state, image or rollback parity.',
                              'Item detailed digests are reported separately, not a pass gate.',
                              'Neutral opponent, grounded entries; damage/interruption/airborne remain separate.']}
        (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    raise SystemExit(0 if receipt['status'] == 'PASS' else 1)


if __name__ == '__main__':
    main()
