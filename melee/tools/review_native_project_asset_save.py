"""Exercise coordinated Workshop asset saving on an isolated copy of a draft."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', required=True, type=Path)
    parser.add_argument('--draft', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source = args.draft.resolve()
    before = {str(p.relative_to(source)): digest(p)
              for p in source.rglob('*') if p.is_file()}
    draft = output / 'draft'
    shutil.copytree(source, draft)
    process = subprocess.run([str(args.exe.resolve()), str(draft)],
                             capture_output=True, text=True, check=False,
                             creationflags=subprocess.CREATE_NO_WINDOW)
    (output / 'check.log').write_text(process.stdout + process.stderr,
                                     encoding='utf-8')
    match = re.search(r'^PASS (\d+) multi-asset save, rollback, validation and undo checks$',
                      process.stdout, re.MULTILINE)
    after = {str(p.relative_to(source)): digest(p)
             for p in source.rglob('*') if p.is_file()}
    passed = process.returncode == 0 and match is not None and before == after
    receipt = dict(passed=passed, exit=process.returncode,
                   checks=int(match[1]) if match else 0,
                   exe_sha256=digest(args.exe), source_unchanged=before == after,
                   source_files=before)
    (output / 'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in receipt.items() if k != 'source_files'}))
    if not passed:
        print(process.stdout + process.stderr)
    raise SystemExit(0 if passed else 1)


if __name__ == '__main__':
    main()
