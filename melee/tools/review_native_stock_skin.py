"""Round-trip all retail costume meshes through the native skin editor.

Each costume is extracted to one temporary input file, processed by the native
test, then removed. The receipt retains every success and failure so one bad
archive does not hide the remaining cases. No game window is opened.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from audit_native_item_scripts import DatView
from audit_native_model_layouts import costumes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--test', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    executable = args.test.resolve()
    receipt = dict(status='RUNNING', executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                   cases=[], failures=[], meshes=0, checks=0, brushes=0)
    source = output/'current-costume.dat'
    for name, payload in costumes(args.iso.resolve()):
        roots = [root for root in DatView(payload).roots if root.endswith('_Share_joint')]
        case = dict(archive=name, roots=roots)
        if len(roots) == 1:
            source.write_bytes(payload)
            process = subprocess.run([str(executable), str(source), roots[0]],
                capture_output=True, text=True, creationflags=subprocess.CREATE_NO_WINDOW)
            case.update(exit_code=process.returncode, stdout=process.stdout, stderr=process.stderr)
            match = re.search(r'PASS: meshes=(\d+) checks=(\d+) brushes=(\d+)', process.stdout)
            if process.returncode == 0 and match:
                receipt['meshes'] += int(match[1])
                receipt['checks'] += int(match[2])
                receipt['brushes'] += int(match[3])
            else:
                receipt['failures'].append(name)
        else:
            receipt['failures'].append(name)
        receipt['cases'].append(case)
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
        print(f'{name}: {case.get("stdout", "").strip()} {case.get("stderr", "").strip()}', flush=True)
    source.unlink(missing_ok=True)
    receipt['status'] = 'PASS' if receipt['cases'] and not receipt['failures'] else 'FAIL'
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in receipt.items() if key != 'cases'}, indent=2))
    return 0 if receipt['status'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
