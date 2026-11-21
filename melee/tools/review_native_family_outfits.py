"""Check coordinated outfits with received source families and real runtime imports."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess

from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tool', type=Path, required=True)
    parser.add_argument('--runtime-test', type=Path, required=True)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    fixture = json.loads(args.fixture.read_text())
    receipt = {'status': 'RUNNING', 'inputs': {}, 'cases': [], 'commands': []}
    for path in [Path(__file__).resolve(), args.tool.resolve(), args.runtime_test.resolve(), args.fixture.resolve()]:
        with path.open('rb') as file:
            receipt['inputs'][str(path)] = hashlib.file_digest(file, 'sha256').hexdigest()

    def save():
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')

    def run(label, argv, succeeds=True):
        env = dict(os.environ, LOCALAPPDATA=str(output/'appdata'),
                   PATH=os.path.join(os.environ['SystemRoot'], 'System32'))
        result = subprocess.run(list(map(str, argv)), cwd=args.tool.resolve().parent,
                                env=env, capture_output=True, encoding='utf-8',
                                errors='replace', timeout=180,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        text = result.stdout+result.stderr
        (output/(label+'.log')).write_text(text, encoding='utf-8')
        receipt['commands'].append({'label': label, 'argv': list(map(str, argv)), 'exit': result.returncode})
        save()
        assert (result.returncode == 0) == succeeds, (label, result.returncode, text)
        return text

    try:
        for case in fixture['cases']:
            label, parent = case['label'], case['id']
            store = output/label/'content'
            source = Path(case['received_draft']).parents[1]/'content/cache'/(parent+'.fmc')
            raw = source.read_bytes()
            assert hashlib.sha256(raw).hexdigest() == parent
            receipt['inputs'][str(source)] = parent
            count = raw[6]
            keys = [raw[80+m*64:112+m*64].split(b'\0')[0].decode() for m in range(count)]

            def cli(step, *arguments, succeeds=True):
                return run(label+'-'+step, [args.tool.resolve(), '--root', store, *arguments], succeeds)

            cli('cache-parent', 'cache', source)
            cli('accept-parent', 'accept', parent)
            draft = output/label/'draft'
            cli('clone', 'clone-installed', parent, draft, 'Outfit author '+label)
            members = output/label/'models'
            members.mkdir()
            for m, key in enumerate(keys):
                member = draft if m == 0 else draft/'partner'
                image = output/(label+'-'+key+'.png')
                Image.new('RGBA', (32, 32), (40+m*150, 240-m*150, 100, 255)).save(image)
                cli('texture-'+key, 'project-model-texture', member, 0, image)
                shutil.copyfile(member/'costume.dat', members/(key+'.dat'))
            if count == 2:
                second = members/(keys[1]+'.dat')
                second.rename(second.with_suffix('.held'))
                cli('reject-incomplete', 'import-custom-costume', members, parent, 'Incomplete outfit', 7, succeeds=False)
                second.with_suffix('.held').rename(second)
            imported = cli('import', 'import-custom-costume', members, parent, 'Coordinated '+label, 7)
            outfit = re.search(r'ID: ([0-9a-f]{64})', imported)[1]
            packet = (store/'cache'/(outfit+'.fmc')).read_bytes()
            assert hashlib.sha256(packet).hexdigest() == outfit
            assert packet[:5] == b'FMCO\x03' and packet[5:7] == bytes([7, count])
            offset = 180
            for m, key in enumerate(keys):
                entry = 108+m*36
                assert packet[entry:entry+32].split(b'\0')[0].decode() == key
                size = struct.unpack_from('<I', packet, entry+32)[0]
                assert packet[offset:offset+size] == (members/(key+'.dat')).read_bytes()
                offset += size
            assert offset == len(packet)
            receiver = output/label/'receiver'
            downloads = receiver/'downloads'
            downloads.mkdir(parents=True)
            for package_id in (parent, outfit):
                shutil.copyfile(store/'cache'/(package_id+'.fmc'), downloads/(package_id+'.fmc'))
            run(label+'-accept-outfit', [args.tool.resolve(), '--root', receiver, 'accept', outfit])
            for package_id in (parent, outfit):
                assert (receiver/'installed'/(package_id+'.fmi')).is_file()
                assert hashlib.sha256((receiver/'cache'/(package_id+'.fmc')).read_bytes()).hexdigest() == package_id
            result = run(label+'-runtime', [args.runtime_test.resolve(), args.iso.resolve(), receiver, parent, outfit])
            assert f'PASS {count} coordinated outfit members' in result
            receipt['cases'].append({'label': label, 'parent': parent, 'outfit': outfit,
                                     'members': keys, 'runtime': 'PASS'})
        receipt['status'] = 'PASS coordinated source-family outfits'
    except Exception as error:
        receipt.update(status='FAIL', error=repr(error))
        raise
    finally:
        save()


if __name__ == '__main__':
    main()
