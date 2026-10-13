"""Run a private installed Peach source package in a hidden, muted match."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import time
from pathlib import Path


def pad(button=0, x=0, y=0):
    return struct.pack('<HbbbbBBBBbxI', button, x, y, 0, 0, 0, 0, 0, 0, 0, 0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--package', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    if out.exists():
        parser.error('output must be a fresh directory')
    metadata = json.loads(args.package.read_text())
    exe = args.exe.resolve()
    cache = Path(metadata['cache']).resolve()
    immutable = [exe, args.package.resolve()] + [p for p in cache.rglob('*') if p.is_file()]
    sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
    hashes = {str(p): sha(p) for p in immutable}
    out.mkdir(parents=True)
    frames = 2400
    stream = bytearray(struct.pack('<8sQII', b'FMREPLAY', 0, 0x12345678, 1))
    for frame in range(frames):
        phase = (frame - 200) % 400 if frame >= 200 else -1
        button = x = y = 0
        if 0 <= phase < 4: button, y = 0x200, -80
        elif 65 <= phase < 69: button, x = 0x100, 80
        elif 120 <= phase < 124: button = 0x200
        elif 210 <= phase < 214: button, y = 0x200, 80
        elif 320 <= phase < 324: button, x = 0x200, -80
        stream += struct.pack('<Q', 0) + pad(button, x, y) + pad() * 3 + struct.pack('<II', 0, 0)
    replay = out / 'inputs.fmr'
    replay.write_bytes(stream)
    command = [str(exe), '--iso', str(args.iso.resolve()), '--match', '--stage', 'final-destination',
               '--mode', 'stock', '--stocks', '99', '--time', '0', '--items', 'off',
               '--hidden', '--mute', '--fast', '--trace-actions', '--trace-fighter-state',
               '--replay-inputs', str(replay), '--content-root', str(cache)]
    for port in range(1, 5):
        command += [f'--p{port}', 'human:custom' if port == 1 else 'cpu:custom:9',
                    f'--p{port}-fighter', metadata['id']]
    start = time.monotonic()
    with (out / 'stdout.log').open('w') as stdout, (out / 'stderr.log').open('w') as stderr:
        process = subprocess.Popen(command, cwd=exe.parent, stdout=stdout, stderr=stderr,
                                   creationflags=subprocess.CREATE_NO_WINDOW)
        print(json.dumps({'pid': process.pid, 'output': str(out)}), flush=True)
        try:
            code = process.wait(timeout=240)
            timeout = False
        except subprocess.TimeoutExpired:
            process.kill()
            code = process.wait()
            timeout = True
    stdout = (out / 'stdout.log').read_text()
    stderr = (out / 'stderr.log').read_text()
    actors = re.findall(r'Live fighter (\d+) kind (\d+): (\d+) completed / (\d+) requested interpreted ticks, state=([0-9a-f]+), faulted=(\d+)', stdout)
    items = sorted(set(map(int, re.findall(r'\[item\].*?kind=(\d+)', stderr))))
    errors = [line for line in stdout.splitlines() if re.search(r'\[script\] (Item \d+ function|Fighter \d+ event)', line)]
    unchanged = all(sha(Path(path)) == digest for path, digest in hashes.items())
    accepted = code == 0 and not timeout and len(actors) == 4 and all(a[-1] == '0' for a in actors)
    accepted = accepted and not errors and f'[replay] End after {frames} controller samples.' in stderr and unchanged
    receipt = {'status': 'PASS' if accepted else 'FAIL', 'command': command,
               'input_sha256': hashes, 'inputs_unchanged': unchanged, 'replay_sha256': sha(replay),
               'exit': code, 'timeout': timeout, 'wall_seconds': time.monotonic() - start,
               'actors': actors, 'items': items, 'script_errors': errors,
               'scope': 'Four source-owned actors; replay-controlled P1 and CPU P2-P4. Drawing retained. Live smoke only, no complete article coverage, stock parity, framebuffer or rollback acceptance.'}
    (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({k: v for k, v in receipt.items() if k != 'input_sha256'}), flush=True)
    raise SystemExit(0 if accepted else 1)


if __name__ == '__main__':
    main()
