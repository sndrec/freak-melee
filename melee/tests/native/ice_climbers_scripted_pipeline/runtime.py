"""Isolated rendered pair smoke comparison; hidden, muted, replay inputs only."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[3]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--content-root', type=Path, required=True)
    parser.add_argument('--fighter')
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--compare-stock', type=Path,
                        help='Completed stock game.log to compare with this custom run')
    args = parser.parse_args()
    if args.compare_stock and (not args.fighter or args.prepare_only):
        parser.error('--compare-stock requires a rendered custom run')
    out = args.output.resolve()
    if out.exists() or out.drive.upper() != 'A:':
        parser.error('Use a fresh A: directory')
    out.mkdir(parents=True)
    replay = out / 'inputs.fmr'
    frames = 1200
    samples = bytearray(struct.pack('<8sQII', b'FMREPLAY', 0, 0x12345678, 1))
    sequence = [
        {'from': 180, 'to': 183, 'buttons': 0x200, 'x': 0, 'y': 0, 'purpose': 'Ice Shot'},
        {'from': 240, 'to': 266, 'buttons': 0, 'x': 70, 'y': 0, 'purpose': 'walking follower'},
        {'from': 360, 'to': 363, 'buttons': 0x200, 'x': -100, 'y': 0, 'purpose': 'Squall'},
        {'from': 600, 'to': 620, 'buttons': 0x200, 'x': 0, 'y': -100, 'purpose': 'Blizzard'},
        {'from': 860, 'to': 864, 'buttons': 0x400, 'x': 0, 'y': 0, 'purpose': 'jump'},
        {'from': 885, 'to': 888, 'buttons': 0x200, 'x': 0, 'y': 100, 'purpose': 'Belay'},
    ]
    def pad(button=0, x=0, y=0, error=0):
        return struct.pack('<HbbbbBBBBbxI', button, x, y, 0, 0, 0, 0, 0, 0, error, 0)
    for frame in range(frames):
        action = next((s for s in sequence if s['from'] <= frame < s['to']), None)
        p1 = pad(action['buttons'], action['x'], action['y']) if action else pad()
        samples += struct.pack('<Q', 0) + p1 + pad() + pad(error=-1) * 2 + struct.pack('<II', 0, 0)
    assert len(samples) == 24 + frames * 80
    replay.write_bytes(samples)
    command = [str(ROOT/'build/native/melee_native.exe'), '--iso',
        str(ROOT.parent/'Super Smash Bros. Melee (USA) (v1.02).iso'),
        '--match', '--stage', 'battlefield', '--p1',
        'human:custom' if args.fighter else 'human:iceclimbers',
        '--p2', 'human:mario', '--p3', 'off', '--p4', 'off',
        '--mode', 'stock', '--stocks', '1', '--time', '0', '--items', 'off',
        '--hidden', '--mute', '--fast', '--trace-actions', '--trace-fighter-state',
        '--replay-inputs', str(replay), '--content-root', str(args.content_root.resolve())]
    if args.fighter:
        command += ['--p1-fighter', args.fighter]
    receipt = {'scope': 'rendered smoke, not whole-game parity acceptance',
        'requested_samples': frames, 'sequence': sequence, 'command': command,
        'replay_sha256': hashlib.sha256(samples).hexdigest(), 'status': 'PREPARED'}
    if not args.prepare_only:
        with (out/'game.log').open('wb') as log:
            result = subprocess.run(command, cwd=ROOT/'build/native', stdout=log,
                stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW, timeout=600)
        text = (out/'game.log').read_text(encoding='utf-8', errors='replace')
        receipt.update(exit_code=result.returncode,
            replay_finished=f'[replay] End after {frames} controller samples.' in text,
            status='PASS' if result.returncode == 0 and f'[replay] End after {frames} controller samples.' in text else 'FAIL')
        if receipt['status'] == 'PASS' and args.compare_stock:
            comparison = subprocess.run([
                'python', str(ROOT/'tools/compare_fighter_parity.py'),
                str(args.compare_stock.resolve()), str(out/'game.log'),
                '--ports', '1', '2', '--clone-ports', '1',
                '--independent-ports', '1', '--expect-motion', '341',
                '--expect-motion', '344', '--expect-motion', '357',
                '--expect-motion', '352'], cwd=ROOT, capture_output=True,
                text=True, encoding='utf-8', errors='replace',
                creationflags=subprocess.CREATE_NO_WINDOW)
            receipt['comparison'] = {'exit_code': comparison.returncode,
                                     'stdout': comparison.stdout,
                                     'stderr': comparison.stderr}
            receipt['status'] = 'PASS' if comparison.returncode == 0 else 'FAIL'
    (out/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(receipt, indent=2))
    if receipt['status'] == 'FAIL':
        raise SystemExit(1)


if __name__ == '__main__':
    main()
