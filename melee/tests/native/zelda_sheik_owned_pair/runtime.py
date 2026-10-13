"""Isolated real alternate-family comparison. Default mode prepares only.

No package source edits, user-store installation, foreground window or OS input.
Requires a current successful normal paired pipeline receipt. Exact trace parity
and actual form/special coverage are separate gates; stop at the first failure.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
ACTION = re.compile(r'^\[action\] sample (\d+) P1 kind=(\d+) behavior=(\d+) motion=(\d+) ')


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sequence(air):
    # Input intervals are half-open. The replay is identical for stock/custom.
    events = []
    def add(start, end, buttons, x, y, purpose):
        events.append(dict(start=start, end=end, buttons=buttons, x=x, y=y, purpose=purpose))
    if air:
        add(180, 184, 0x400, 0, 0, 'Zelda jump')
        add(192, 196, 0x200, 0, -100, 'Zelda airborne transform to Sheik')
        add(480, 484, 0x400, 0, 0, 'Sheik jump for Needle charge')
        add(492, 570, 0x200, 0, 0, 'Sheik airborne Needles; release shoots')
        add(800, 804, 0x400, 0, 0, 'Sheik jump for Chain')
        add(812, 910, 0x200, 100, 0, 'Sheik airborne Chain hold then retract')
        add(1160, 1164, 0x400, 0, 0, 'Sheik jump for Vanish')
        add(1172, 1176, 0x200, 0, 100, 'Sheik airborne Vanish')
        add(1520, 1524, 0x400, 0, 0, 'Sheik jump for transform')
        add(1532, 1536, 0x200, 0, -100, 'Sheik airborne transform back to Zelda')
        add(1840, 1844, 0x400, 0, 0, 'Zelda jump for Nayru')
        add(1852, 1856, 0x200, 0, 0, 'Zelda airborne Nayru')
        add(2140, 2144, 0x400, 0, 0, 'Zelda jump for Din')
        add(2152, 2230, 0x200, 100, 0, 'Zelda airborne Din hold then detonate')
        add(2560, 2564, 0x400, 0, 0, 'Zelda jump for Farore')
        add(2572, 2576, 0x200, 0, 100, 'Zelda airborne Farore')
    else:
        add(180, 184, 0x200, 0, -100, 'Zelda grounded transform to Sheik')
        add(480, 560, 0x200, 0, 0, 'Sheik Needles charge; release shoots')
        add(800, 910, 0x200, 100, 0, 'Sheik Chain hold then retract')
        add(1160, 1164, 0x200, 0, 100, 'Sheik Vanish')
        add(1520, 1524, 0x200, 0, -100, 'Sheik transform back to Zelda')
        add(1840, 1844, 0x200, 0, 0, 'Zelda Nayru')
        add(2140, 2230, 0x200, 100, 0, 'Zelda Din hold then detonate')
        add(2560, 2564, 0x200, 0, 100, 'Zelda Farore')
    return events


def recording(events, frames):
    def pad(button=0, x=0, y=0, error=0):
        return struct.pack('<HbbbbBBBBbxI', button, x, y, 0, 0, 0, 0, 0, 0, error, 0)
    data = bytearray(struct.pack('<8sQII', b'FMREPLAY', 0, 0x12345678, 1))
    for frame in range(frames):
        event = next((s for s in events if s['start'] <= frame < s['end']), None)
        p1 = pad(event['buttons'], event['x'], event['y']) if event else pad()
        data += struct.pack('<Q', 0) + p1 + pad() + pad(error=-1) * 2 + struct.pack('<II', 0, 0)
    assert len(data) == 24 + frames * 80
    return data


def coverage(path, custom, air):
    rows = [tuple(map(int, m.groups())) for line in path.read_text(encoding='utf-8', errors='replace').splitlines()
            if (m := ACTION.match(line))]
    identities = []
    for _, kind, behavior, _ in rows:
        if kind != behavior or not (34 <= kind < 128 if custom else kind in (19, 7)):
            raise ValueError('Invalid independent identity: ' + str((kind, behavior)))
        if not identities or kind != identities[-1]:
            identities.append(kind)
    if len(identities) != 3 or identities[0] != identities[2] or identities[0] == identities[1]:
        raise ValueError('Need actual Zelda -> Sheik -> Zelda activation: ' + str(identities))
    if not custom and identities != [19, 7, 19]:
        raise ValueError('Unexpected raw form identity sequence')
    forms = {identities[0]: 'zelda', identities[1]: 'sheik'}
    seen = {name: set() for name in forms.values()}
    for _, kind, _, motion in rows:
        seen[forms[kind]].add(motion)
    # End form may land during transform/Vanish/Farore; require actual finish,
    # not every mutually exclusive ground/air variant in one trajectory.
    required = {'zelda': [[357] if air else [355], [356, 358], [342] if air else [341],
                          [346] if air else [343], [344, 347], [345, 348],
                          [352] if air else [349], [351, 354]],
                'sheik': [[362, 364], [363] if air else [361],
                          [345] if air else [341], [342, 346], [344, 348],
                          [352] if air else [349], [350, 353], [351, 354],
                          [358] if air else [355], [357, 360]]}
    missing = {form: [group for group in groups if not seen[form].intersection(group)]
               for form, groups in required.items()}
    result = dict(form_sequence=identities, motions={form: sorted(v) for form, v in seen.items()}, missing=missing)
    if any(missing.values()):
        raise ValueError('Insufficient move coverage: ' + json.dumps(result))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pipeline-receipt', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--exe', type=Path, default=ROOT/'build/native/melee_native.exe')
    parser.add_argument('--tool', type=Path, default=ROOT/'build/native/native_content_tool.exe')
    parser.add_argument('--iso', type=Path, default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso')
    parser.add_argument('--case', choices=['ground', 'air'], action='append')
    parser.add_argument('--run', action='store_true', help='Authorized isolated accept and hidden/muted games; default prepares only')
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists() or out.drive.upper() != 'A:':
        parser.error('Use a fresh isolated A: directory')
    pipeline_path = args.pipeline_receipt.resolve()
    pipeline = json.loads(pipeline_path.read_text(encoding='utf-8'))
    if not pipeline['status'].startswith('PASS'):
        parser.error('Pipeline must have passed')
    family = pipeline['family_id']
    store = pipeline_path.parent/'store'
    package = store/'cache'/(family + '.fmc')
    if digest(package) != family:
        parser.error('Cached family bytes do not match content ID')
    out.mkdir(parents=True)
    inputs = {str(p.resolve()): digest(p) for p in (Path(__file__), pipeline_path, package, args.exe, args.tool,
                                                  ROOT/'tools/compare_fighter_parity.py')}
    receipt = dict(status='PREPARED', family_id=family, content_root=str(store), inputs=inputs, cases={},
                   boundaries=['Rendered replay trace parity, not all branches/costumes/results/KO/item-transfer acceptance.',
                               'Air finish may land; coverage validates actual form identity and appropriate phase groups.',
                               'No source or user content store changes. Isolated family acceptance is a menu transition.'])
    def save():
        (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    def execute(command, log, cwd):
        command = list(map(str, command))
        with log.open('wb') as stream:
            result = subprocess.run(command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT,
                                    creationflags=subprocess.CREATE_NO_WINDOW, timeout=600)
        return dict(command=command, exit=result.returncode, log=str(log), log_sha256=digest(log))
    try:
        if args.run:
            for stage, words in [('verify', ['verify-package', family, args.iso]), ('accept', ['accept', family, args.iso])]:
                result = execute([args.tool, '--root', store, *words], out/(stage+'.log'), ROOT)
                receipt[stage] = result; save()
                if result['exit']:
                    raise RuntimeError(stage + ' failed; no game after failed package gate')
        for name in args.case or ['ground', 'air']:
            directory = out/name; directory.mkdir()
            events = sequence(name == 'air'); frames = 3200
            replay = directory/'inputs.fmr'; replay.write_bytes(recording(events, frames))
            case = dict(samples=frames, sequence=events, replay_sha256=digest(replay), commands={})
            receipt['cases'][name] = case
            for label in ['stock', 'custom']:
                command = [args.exe, '--iso', args.iso, '--match', '--stage', 'battlefield', '--p1',
                           'human:zelda' if label == 'stock' else 'human:custom', '--p2', 'human:mario',
                           '--p3', 'off', '--p4', 'off', '--mode', 'stock', '--stocks', '1', '--time', '0',
                           '--items', 'off', '--hidden', '--mute', '--fast', '--trace-actions', '--trace-fighter-state',
                           '--replay-inputs', replay, '--content-root', store]
                if label == 'custom':
                    command += ['--p1-fighter', family]
                case['commands'][label] = list(map(str, command)); save()
                if args.run:
                    result = execute(command, directory/(label+'.log'), args.exe.resolve().parent)
                    case[label] = result; save()
                    text = Path(result['log']).read_text(encoding='utf-8', errors='replace')
                    if result['exit'] or f'[replay] End after {frames} controller samples.' not in text:
                        raise RuntimeError(name + '/' + label + ' failed or incomplete')
            if args.run:
                result = execute([sys.executable, ROOT/'tools/compare_fighter_parity.py', directory/'stock.log',
                                  directory/'custom.log', '--ports', '1', '2', '--clone-ports', '1',
                                  '--independent-ports', '1'], directory/'compare.log', ROOT)
                case['compare'] = result; save()
                if result['exit']:
                    raise RuntimeError('First meaningful trace difference: ' + (directory/'compare.log').read_text(encoding='utf-8'))
                case['coverage'] = {label: coverage(directory/(label+'.log'), label == 'custom', name == 'air')
                                    for label in ['stock', 'custom']}
                case['status'] = 'PASS'; save()
        if any(digest(Path(p)) != value for p, value in inputs.items()):
            raise RuntimeError('Pinned inputs changed during run')
        receipt['status'] = 'PASS' if args.run else 'PREPARED'
    except Exception as error:
        receipt.update(status='FAIL', error=str(error)); save()
        raise
    save()
    print(json.dumps(dict(status=receipt['status'], family_id=family, receipt=str(out/'receipt.json')), indent=2))


if __name__ == '__main__':
    main()
