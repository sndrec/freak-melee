"""Actual isolated paired/single draft copy, build, verify and install roundtrip."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[3]
ARCHIVES = Path('A:/FreakMelee-test-archives')
CHUNKS = ('fighter.dat', 'animation.dat', 'costume.dat', 'module.dll',
          'behavior.fms', 'fighter_parameters.fmp', 'cpu_profile.fcp')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def parse_package(path):
    data = path.read_bytes()
    assert data[:5] == b'FMCO\x03', str(path)
    assert data[7] == 0 and data[12:16] == bytes(4)
    mode, count = data[5:7]
    assert (mode == 0 and count == 1) or (mode in (1, 2) and count == 2)
    title = data[16:80].split(b'\0', 1)[0].decode('utf-8')
    portrait_size = struct.unpack_from('<I', data, 8)[0]
    cursor = 80 + 2 * 92
    portrait = data[cursor:cursor + portrait_size]
    cursor += portrait_size
    members = []
    for index in range(2):
        base = 80 + index * 92
        if index >= count:
            assert data[base:base + 92] == bytes(92)
            continue
        key = data[base:base + 32].split(b'\0', 1)[0].decode('ascii')
        code = data[base + 32:base + 64].split(b'\0', 1)[0].decode('ascii')
        sizes = struct.unpack_from('<7I', data, base + 64)
        chunks = {}
        for name, size in zip(CHUNKS, sizes):
            assert cursor + size <= len(data)
            chunks[name] = data[cursor:cursor + size]
            cursor += size
        members.append({'key': key, 'code_key': code, 'chunks': chunks})
    assert cursor == len(data)
    return {'mode': mode, 'count': count, 'title': title,
            'portrait': portrait, 'members': members, 'sha256': sha(data)}


def payload_inventory(package):
    return [{'key': member['key'], 'code_key': member['code_key'],
             'chunks': {name: {'bytes': len(data), 'sha256': sha(data)}
                        for name, data in member['chunks'].items()}}
            for member in package['members']]


def check_draft(project, package):
    assert not (project/'creation.pending').exists()
    descriptor = project/'family.txt'
    if package['count'] == 2:
        fields = dict(line.split('=', 1) for line in descriptor.read_text().splitlines())
        assert fields == {'mode': 'alternate' if package['mode'] == 1 else 'companion',
                          'primary_key': package['members'][0]['key'],
                          'partner_key': package['members'][1]['key'], 'partner': 'partner'}
    elif package['members'][0]['key'] == 'main':
        assert not descriptor.exists()
    else:
        assert descriptor.read_text() == ('mode=single\nprimary_key=' +
                                         package['members'][0]['key'] + '\n')
    for index, member in enumerate(package['members']):
        directory = project if index == 0 else project/'partner'
        assert not (directory/'creation.pending').exists()
        fields = dict(line.split('=', 1) for line in (directory/'project.txt').read_text().splitlines())
        assert fields['code'] == member['code_key']
        for name, data in member['chunks'].items():
            path = directory/name
            assert (path.read_bytes() if path.exists() else b'') == data, (index, name)
        assert (directory/'portrait.png').read_bytes() == package['portrait']


def same_payload(first, second):
    assert first['mode'] == second['mode'] and first['count'] == second['count']
    assert first['portrait'] == second['portrait']
    assert payload_inventory(first) == payload_inventory(second)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--tool', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--zelda-family', type=Path, default=ARCHIVES/'zelda-sheik-normal72-runtime-worker/store/cache/1ac7244b65e6e0664c985dfe90cccc48d50b4c9ad0314e05f5329746f323b5aa.fmc')
    parser.add_argument('--ice-family', type=Path, default=ARCHIVES/'ice-climbers-scripted-pipeline72-root-final/store/cache/83367932060e1a5c2fcc54d33a69fa112ad747522965cfc5231fc8544fd0dae3.fmc')
    parser.add_argument('--single', type=Path, default=ARCHIVES/'zelda-sheik-normal72-runtime-worker/store/cache/aac0010326e21dd16126e3ea09baccd2c4b08fa3131dbeb4d199de308fb4ead5.fmc')
    args = parser.parse_args()
    output = args.output.resolve()
    assert output.drive.upper() == 'A:' and not output.exists(), str(output)
    output.mkdir(parents=True)
    tool, iso = args.tool.resolve(), args.iso.resolve()
    sources = [ROOT/'src/melee/ft'/name for name in (
        'native_fighter_family_project.c', 'native_fighter_family_project.h',
        'native_fighter_project.c', 'native_fighter_project.h',
        'native_fighter_project_install.c', 'native_fighter_runtime.c',
        'native_fighter_runtime.h', 'native_fighter_module.c', 'native_fighter_module.h')]
    sources += [ROOT/'src/melee/gm'/name for name in (
        'native_fighter_package.c', 'native_content_store.c', 'native_content_store.h')]
    sources += [ROOT/'tools/native_content_tool.c', Path(__file__).resolve(), tool]
    sources += sorted(tool.parent.glob('*.dll'))
    inputs = {str(path): sha(path.read_bytes()) for path in sources}
    receipts = []
    try:
        for label, source, expected_mode, expected_count in (
            ('zelda-pair', args.zelda_family, 1, 2),
            ('ice-pair', args.ice_family, 2, 2),
            ('zelda-single', args.single, 0, 1)):
            source = source.resolve()
            original = parse_package(source)
            assert (original['mode'], original['count']) == (expected_mode, expected_count)
            assert source.stem == original['sha256']
            inputs[str(source)] = original['sha256']
            case = output/label
            case.mkdir()
            store = case/'store'
            env = os.environ.copy()
            for name in ('LOCALAPPDATA', 'APPDATA', 'TEMP', 'TMP'):
                path = case/('local-appdata' if name == 'LOCALAPPDATA' else name.lower())
                path.mkdir(exist_ok=True)
                env[name] = str(path)
            commands = []

            def cli(stage, *words):
                command = [str(tool), '--root', str(store), *map(str, words)]
                result = subprocess.run(command, env=env, cwd=ROOT, capture_output=True,
                                        text=True, encoding='utf-8', errors='replace',
                                        timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
                (case/(stage+'.log')).write_text(result.stdout+result.stderr, encoding='utf-8')
                commands.append({'stage': stage, 'command': command, 'exit': result.returncode})
                assert result.returncode == 0, (label, stage, result.stdout, result.stderr)
                return result.stdout

            def package_id(text):
                matches = re.findall(r'\b[0-9a-f]{64}\b', text)
                assert matches, text
                return matches[-1]

            def verify(stage, identifier):
                text = cli(stage, 'verify-package', identifier, iso)
                rows = re.findall(r'Member (\d+) \(([^)]+)\) animation binding:', text)
                assert rows == [(str(index), member['key'])
                                for index, member in enumerate(original['members'])], text
                assert 'registered and released.' in text
                return text

            assert package_id(cli('cache-original', 'cache', source)) == original['sha256']
            cli('accept-original', 'accept', original['sha256'], iso)
            project = case/'project'
            cli('clone-installed', 'clone-installed', original['sha256'], project, original['title'])
            check_draft(project, original)
            first = package_id(cli('package-project', 'package-project', project, project/'portrait.png'))
            bundled = package_id(cli('bundle-project', 'bundle-project', project, project/'portrait.png', iso))
            assert first == bundled == original['sha256']
            first_package = parse_package(store/'cache'/(first+'.fmc'))
            same_payload(original, first_package)
            first_verification = verify('verify-first', first)
            cli('install-first', 'install-project', project, first)
            assert (project/'installed.id').read_text().strip() == first
            manifest = project/'project.txt'
            old_text = manifest.read_text(encoding='utf-8')
            title = original['title'][:49]+' edited'
            assert old_text.count('title='+original['title']+'\n') == 1
            manifest.write_text(old_text.replace('title='+original['title']+'\n',
                                                'title='+title+'\n'), encoding='utf-8', newline='\n')
            second = package_id(cli('package-title-update', 'package-project', project, project/'portrait.png'))
            second_bundle = package_id(cli('bundle-title-update', 'bundle-project', project, project/'portrait.png', iso))
            assert second == second_bundle and second != first
            second_package = parse_package(store/'cache'/(second+'.fmc'))
            assert second_package['title'] == title
            same_payload(original, second_package)
            check_draft(project, original)
            second_verification = verify('verify-second', second)
            cli('install-replacement', 'install-project', project, second)
            assert (project/'installed.id').read_text().strip() == second
            installed = sorted(path.stem for path in (store/'installed').glob('*.fmi'))
            assert installed == [second], installed
            assert not (store/'installed'/(first+'.fmi')).exists()
            assert (store/'cache'/(first+'.fmc')).exists()
            copied = case/'copied-replacement'
            cli('clone-replacement', 'clone-installed', second, copied, title)
            check_draft(copied, second_package)
            assert package_id(cli('package-replacement-copy', 'package-project', copied,
                                  copied/'portrait.png')) == second
            receipts.append({'case': label, 'mode': expected_mode, 'members': expected_count,
                'source': str(source), 'original_id': first, 'replacement_id': second,
                'member_payloads': payload_inventory(original), 'portrait_sha256': sha(original['portrait']),
                'first_verify': first_verification, 'replacement_verify': second_verification,
                'installed_ids': installed, 'commands': commands})
            print(label+': exact payload roundtrip + title-only install replacement PASS', flush=True)
        assert all(sha(Path(path).read_bytes()) == digest for path, digest in inputs.items()), 'Input changed during run'
        receipt = {'status': 'PASS actual paired/single family draft CLI roundtrip',
                   'cases': receipts, 'inputs': inputs,
                   'isolation': 'Fresh A: stores and child LOCALAPPDATA/APPDATA/TEMP/TMP; CREATE_NO_WINDOW; no games',
                   'limits': ['No Workshop widget/member-switch interaction is exercised.',
                              'No changed gameplay, actor coordination, rendering or rollback is asserted.',
                              'Full cached runtime registration is verified, but member DLL bodies are not executed here.',
                              'Existing accepted source and gameplay receipts remain separate.']}
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n', encoding='utf-8')
    except Exception as error:
        (output/'failure.json').write_text(json.dumps({'error': repr(error), 'completed': receipts,
                                                      'inputs': inputs}, indent=2)+'\n', encoding='utf-8')
        raise


if __name__ == '__main__':
    main()
