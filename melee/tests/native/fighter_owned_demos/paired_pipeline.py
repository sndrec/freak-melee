"""Refresh isolated accepted paired drafts with newly constructed owned clips.

Uses the real cache/accept/clone/bundle/verify paths. Existing compiled scripts
are pinned inputs; this is an asset-ownership gate, not script parity testing.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
ARCHIVES = Path('A:/FreakMelee-test-archives')
sys.path.insert(0, str(ROOT / 'tests/native/fighter_family_workshop'))
from roundtrip import parse_package


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--constructed', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--tool', type=Path, default=ROOT / 'build/native/native_content_tool.exe')
    parser.add_argument('--iso', type=Path, default=ROOT.parent / 'Super Smash Bros. Melee (USA) (v1.02).iso')
    args = parser.parse_args()
    output, constructed = args.output.resolve(), args.constructed.resolve()
    assert output.drive.upper() == 'A:' and not output.exists()
    constructor_receipt = json.loads((constructed / 'receipt.json').read_text(encoding='utf-8'))
    assert constructor_receipt['status'].startswith('PASS')
    output.mkdir(parents=True)
    env = os.environ.copy()
    for name in ('LOCALAPPDATA', 'APPDATA', 'TEMP', 'TMP'):
        path = output / name.lower()
        path.mkdir()
        env[name] = str(path)
    tool, iso, store = args.tool.resolve(), args.iso.resolve(), output / 'store'
    pinned = {str(path): sha(path) for path in (Path(__file__), tool, constructed / 'receipt.json')}
    receipt = {'status': 'RUNNING', 'inputs': pinned, 'families': {}, 'commands': [],
               'limits': ['Accepted SDK72 module/FMS/FMP/CPU inputs reused unchanged.',
                          'This gate registers/releases owned clips but does not execute results actors.'],
               'isolation': 'Fresh A: store, drafts and app-data; CREATE_NO_WINDOW; no games.'}

    def save():
        (output / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')

    def cli(directory, stage, *words):
        command = list(map(str, [tool, '--root', store, *words]))
        result = subprocess.run(command, env=env, cwd=ROOT, capture_output=True,
                                encoding='utf-8', errors='replace', timeout=180,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        (directory / (stage + '.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
        receipt['commands'].append({'stage': stage, 'command': command, 'exit': result.returncode})
        save()
        assert result.returncode == 0, (stage, result.stdout, result.stderr)
        return result.stdout

    def identifier(text):
        found = re.findall(r'\b[0-9a-f]{64}\b', text)
        assert found, text
        return found[-1]

    cases = (
        ('zelda-sheik', 'zelda-sheik-normal72-runtime-worker',
         '1ac7244b65e6e0664c985dfe90cccc48d50b4c9ad0314e05f5329746f323b5aa', ('Zd', 'Sk')),
        ('ice-climbers', 'ice-climbers-scripted-pipeline72-root-final',
         '83367932060e1a5c2fcc54d33a69fa112ad747522965cfc5231fc8544fd0dae3', ('Pp', 'Nn')),
    )
    try:
        for label, source_directory, original_id, shorts in cases:
            directory = output / label
            directory.mkdir()
            source = ARCHIVES / source_directory / 'store/cache' / (original_id + '.fmc')
            original = parse_package(source)
            assert sha(source) == original_id
            pinned[str(source)] = original_id
            assert identifier(cli(directory, 'cache-original', 'cache', source)) == original_id
            cli(directory, 'accept-original', 'accept', original_id, iso)
            project = directory / 'project'
            cli(directory, 'clone-installed', 'clone-installed', original_id, project, original['title'])
            for member, short in enumerate(shorts):
                destination = project if member == 0 else project / 'partner'
                for name in ('fighter.dat', 'animation.dat'):
                    incoming = constructed / short / 'project' / name
                    pinned[str(incoming)] = sha(incoming)
                    shutil.copyfile(incoming, destination / name)
            new_id = identifier(cli(directory, 'bundle-owned', 'bundle-project', project, project / 'portrait.png', iso))
            verified = cli(directory, 'verify-owned', 'verify-package', new_id, iso)
            assert re.findall(r'owned demo clips: (\d+)/(\d+) decoded', verified) == [('13', '13')] * 2, verified
            replacement = parse_package(store / 'cache' / (new_id + '.fmc'))
            assert replacement['mode'] == original['mode'] and replacement['count'] == 2
            assert replacement['portrait'] == original['portrait']
            for previous, current in zip(original['members'], replacement['members']):
                assert previous['key'] == current['key'] and previous['code_key'] == current['code_key']
                for name in ('module.dll', 'behavior.fms', 'fighter_parameters.fmp', 'cpu_profile.fcp', 'costume.dat'):
                    assert previous['chunks'][name] == current['chunks'][name], (label, name)
            cli(directory, 'accept-owned', 'accept', new_id, iso)
            receipt['families'][label] = {'original_id': original_id, 'id': new_id,
                'members': list(shorts), 'project': str(project), 'verify': verified}
            save()
            print(label + ': both members own 13 decoded demo clips PASS', flush=True)
        assert all(sha(Path(path)) == digest for path, digest in pinned.items()), 'Pinned input changed'
        receipt.update(status='PASS actual paired owned demo asset refresh/package/registration', store=str(store))
    except Exception as error:
        receipt.update(status='FAIL', error=repr(error))
        save()
        raise
    save()


if __name__ == '__main__':
    main()
