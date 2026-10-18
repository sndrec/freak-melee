"""Check rig archive edits against all supported stock fighters.

The animation suite checks compressed tracks, descriptor addresses, masks and
families. The reference suite checks imported physical bone fields and native
subactions. Files are extracted temporarily; installed content and project
drafts remain unchanged.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from audit_native_action_scripts import disc_fighters
from audit_native_item_scripts import DatView
from review_native_animation_nodes import banks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'tool', 'iso', 'output']:
        parser.add_argument('--'+name, required=True, type=Path)
    parser.add_argument('--suite', required=True, choices=['animation', 'references'])
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    root = Path(__file__).resolve().parents[1]
    templates = {}
    definitions = {}
    for path in (root/'src/melee/ft/kinds').rglob('*_clone.c'):
        text = path.read_text()
        key = re.search(r'\.code_key\s*=\s*"([^"]+)"', text)
        symbol = re.search(r'\.fighter_root\s*=\s*"([^"]+)"', text)
        if key and symbol:
            assert symbol[1] not in templates
            templates[symbol[1]] = key[1]
            definitions[str(path.relative_to(root))] = hashlib.sha256(path.read_bytes()).hexdigest()
    bank_data = dict(banks(args.iso.resolve())) if args.suite == 'animation' else {}
    rows = []
    seen = set()
    with tempfile.TemporaryDirectory(prefix='bank-', dir=output) as temp:
        temp = Path(temp)
        for filename, payload in disc_fighters(args.iso.resolve()):
            for symbol in DatView(payload).roots:
                if symbol not in templates:
                    continue
                seen.add(symbol)
                key = templates[symbol]
                fact = temp/'assets.fma'
                command = [str(args.tool.resolve()), 'export-fighter-facts', key, str(fact)]
                export = subprocess.run(command, capture_output=True, text=True,
                                        creationflags=subprocess.CREATE_NO_WINDOW)
                case = dict(fighter=filename, template=key, export_exit=export.returncode)
                log = export.stdout + export.stderr
                bank_name = filename[:-4]+'AJ.dat'
                if export.returncode == 0 and (args.suite == 'references' or bank_name in bank_data):
                    fighter = temp/'fighter.dat'
                    bank = temp/'animation.dat'
                    fighter.write_bytes(payload)
                    if args.suite == 'animation':
                        bank.write_bytes(bank_data[bank_name])
                    inputs = [str(fact), str(fighter)]
                    if args.suite == 'animation':
                        inputs.append(str(bank))
                    run = subprocess.run([str(args.exe.resolve()), *inputs],
                                         capture_output=True, text=True,
                                         creationflags=subprocess.CREATE_NO_WINDOW)
                    log += run.stdout + run.stderr
                    counter = 'clips' if args.suite == 'animation' else 'commands'
                    match = re.search(r'^PASS (\d+) cases (\d+) '+counter+r' (\d+) checks$', run.stdout, re.MULTILINE)
                    case.update(exit=run.returncode, passed=run.returncode == 0 and match is not None,
                                cases=int(match[1]) if match else 0,
                                **{counter: int(match[2]) if match else 0},
                                checks=int(match[3]) if match else 0,
                                fighter_sha256=hashlib.sha256(payload).hexdigest())
                    if args.suite == 'animation':
                        case['animation_sha256'] = hashlib.sha256(bank_data[bank_name]).hexdigest()
                else:
                    case.update(passed=False, missing_bank=args.suite == 'animation' and bank_name not in bank_data)
                rows.append(case)
                (output/(key+'.log')).write_text(log, encoding='utf-8')
                print(f'{key}: {"PASS" if case["passed"] else "FAIL"}', flush=True)
    missing = sorted(set(templates)-seen)
    passed = bool(rows) and not missing and all(row['passed'] for row in rows)
    counter = 'clips' if args.suite == 'animation' else 'commands'
    receipt = dict(passed=passed, suite=args.suite, fighters=len(rows), missing=missing,
                   cases=sum(row.get('cases', 0) for row in rows),
                   **{counter: sum(row.get(counter, 0) for row in rows)},
                   checks=sum(row.get('checks', 0) for row in rows),
                   exe_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest(),
                   tool_sha256=hashlib.sha256(args.tool.read_bytes()).hexdigest(),
                   definitions=definitions, rows=rows)
    (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')
    print(json.dumps({k: v for k, v in receipt.items() if k not in ['definitions', 'rows']}))
    raise SystemExit(0 if passed else 1)


if __name__ == '__main__':
    main()
