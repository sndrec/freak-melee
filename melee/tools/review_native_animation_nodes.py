"""Check bone insertion/removal/reordering against every retail animation bank.

Each bank is read from the user's ISO into a temporary file. The native check
compares all retained compressed tracks and their descriptors byte for byte.
No source project is modified, and the game is not launched.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from prepare_native_assets import read_exact


def banks(iso):
    with iso.open('rb') as file:
        header = read_exact(file, 0, 1088)
        if header[:6] != b'GALE01' or header[7] != 2:
            raise ValueError('Expected GALE01 v1.02')
        offset, size = struct.unpack_from('>II', header, 1060)
        fst = read_exact(file, offset, size)
        count = struct.unpack_from('>I', fst, 8)[0]
        names = fst[count * 12:]
        for i in range(1, count):
            name, offset, size = struct.unpack_from('>III', fst, i * 12)
            if name >> 24:
                continue
            start = name & 0xffffff
            name = names[start:names.index(0, start)].decode()
            if re.fullmatch(r'Pl[A-Z][a-z]AJ\.dat', name):
                yield name, read_exact(file, offset, size)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--iso', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    rows = []
    with tempfile.TemporaryDirectory(prefix='freak-animation-nodes-') as temp:
        path = Path(temp) / 'bank.dat'
        for name, payload in banks(args.iso):
            path.write_bytes(payload)
            result = subprocess.run([str(args.exe.resolve()), str(path)],
                                    capture_output=True, text=True, check=False)
            (args.output / (name + '.log')).write_text(
                result.stdout + result.stderr, encoding='utf-8')
            match = re.search(r'^PASS (\d+) trees (\d+) checks$',
                              result.stdout, re.MULTILINE)
            passed = result.returncode == 0 and match is not None
            rows.append(dict(name=name, bytes=len(payload),
                             sha256=hashlib.sha256(payload).hexdigest(),
                             passed=passed,
                             trees=int(match[1]) if match else 0,
                             checks=int(match[2]) if match else 0))
            print(f'{name}: {"PASS" if passed else "FAIL"}', flush=True)
    passed = bool(rows) and all(row['passed'] for row in rows)
    receipt = dict(passed=passed,
                   exe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                   banks=len(rows), trees=sum(row['trees'] for row in rows),
                   checks=sum(row['checks'] for row in rows), rows=rows)
    (args.output / 'receipt.json').write_text(json.dumps(receipt, indent=2),
                                             encoding='utf-8')
    print(json.dumps({k: v for k, v in receipt.items() if k != 'rows'}))
    raise SystemExit(0 if passed else 1)


if __name__ == '__main__':
    main()
