"""Prepare or execute the real hidden/muted eight-actor rollback benchmark."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[4]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--content-root', type=Path, required=True)
    parser.add_argument('--exe', type=Path, default=ROOT/'build/native/melee_native.exe')
    parser.add_argument('--iso', type=Path,
                        default=ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso')
    parser.add_argument('--fighter', help='Installed paired fighter ID, applied to all four CPU slots')
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x12345678)
    parser.add_argument('--run', action='store_true', help='Execute after preparing; default only writes artifacts')
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists() or out.drive.upper() != 'A:':
        parser.error('Use a fresh A: output directory')
    if not 0 <= args.seed <= 0xFFFFFFFF:
        parser.error('seed must be uint32')
    out.mkdir(parents=True)
    # Actual CPU/follower logic runs in the full game; this fixture supplies
    # connected neutral hardware pads, rather than synthesizing CPU callbacks.
    frames = 2000
    pad = struct.pack('<HbbbbBBBBbxI', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    sample = struct.pack('<Q', 0) + pad*4 + struct.pack('<II', 0, 0)
    assert len(pad) == 16 and len(sample) == 80
    replay = out/'inputs.fmr'
    replay.write_bytes(struct.pack('<8sQII', b'FMREPLAY', 0, args.seed, 1) + sample*frames)
    exe = args.exe.resolve()
    content = args.content_root.resolve()
    command = [str(exe), '--iso', str(args.iso.resolve()), '--match',
               '--stage', 'battlefield', '--mode', 'stock', '--stocks', '99',
               '--time', '0', '--items', 'off', '--hidden', '--mute', '--fast',
               '--rollback-benchmark', '--replay-inputs', str(replay),
               '--content-root', str(content)]
    for port in range(1, 5):
        command += [f'--p{port}', 'cpu:custom:9' if args.fighter else 'cpu:iceclimbers:9']
        if args.fighter:
            command += [f'--p{port}-fighter', args.fighter]
    inputs = [exe, replay, Path(__file__).resolve(),
              ROOT/'src/melee/gm/native_rollback_benchmark.c',
              ROOT/'src/melee/gm/native_rollback_benchmark.h',
              ROOT/'src/melee/gm/gm_1A45.c',
              ROOT/'src/melee/gm/native_match_launch.c']
    if args.fighter:
        # Hash the entire isolated content store, including actor behavior and
        # assets. No assumptions about paired package filenames are needed.
        inputs += sorted(path for path in content.rglob('*') if path.is_file())
    before = {str(path): sha(path) for path in inputs if path.is_file()}
    receipt = {'status': 'PREPARED', 'command': command,
               'seed': args.seed, 'replay_samples': frames,
               'depth': 15, 'windows': 64, 'warmup_forward_ticks': 600,
               'input_sha256': before,
               'timing_contract': 'restore + 15 full replay ticks + one advancing full simulation/draw tick; endpoint hashes excluded and separately reported',
               'limits': ['CPU wall time through actual Aurora end-frame calls; no separate GPU timestamp/completion measurement',
                          'Unpaced work compared against 16.666667ms; no claim of paced scheduling',
                          'Eight actors must remain live with unchanged pair identities through every window',
                          'CPU workload callback coverage is not complete script language coverage']}
    (out/'command.json').write_text(json.dumps(command, indent=2)+'\n', encoding='utf-8')
    if args.run:
        if str(exe) not in before:
            parser.error('built executable is required for --run')
        with (out/'game.log').open('wb') as log:
            result = subprocess.run(command, cwd=exe.parent, stdout=log,
                                    stderr=subprocess.STDOUT,
                                    creationflags=subprocess.CREATE_NO_WINDOW, timeout=600)
        text = (out/'game.log').read_text(encoding='utf-8', errors='replace')
        windows = []
        summary = None
        for line in text.splitlines():
            if line.startswith('[rollback-benchmark] window=') or line.startswith('[rollback-benchmark] PASS '):
                fields = dict(re.findall(r'(\w+)=([\d.]+)', line))
                fields = {key: float(value) if '.' in value else int(value)
                          for key, value in fields.items()}
                if 'window' in fields:
                    windows.append(fields)
                else:
                    summary = fields
        unchanged = all(Path(path).is_file() and sha(Path(path)) == expected
                        for path, expected in before.items())
        complete = result.returncode == 0 and summary is not None and len(windows) == 64
        if complete:
            complete = summary.get('verified') == 64 and summary.get('actors') == 8 and summary.get('depth') == 15
            complete &= all(window.get('window') == index and window.get('actors') == 8
                            and window.get('replay_ticks') == 15
                            for index, window in enumerate(windows))
        receipt.update(status='PASS' if complete and unchanged else 'FAIL',
                       exit_code=result.returncode, inputs_unchanged=unchanged,
                       summary=summary, measured_windows=windows,
                       deadline_met=bool(complete and summary.get('deadline_misses') == 0))
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'status': receipt['status'], 'receipt': str(out/'receipt.json'),
                      'summary': receipt.get('summary')}))
    if receipt['status'] == 'FAIL':
        raise SystemExit(1)


if __name__ == '__main__':
    main()
