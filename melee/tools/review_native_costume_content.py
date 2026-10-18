"""Check standalone costume dependencies using isolated native content stores."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from PIL import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--tool', type=Path, required=True)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    fixture = json.loads(args.fixture.read_text())
    fox = next(case for case in fixture['cases'] if case['label'] == 'fox')
    parent = fox['id']
    source = Path(fox['received_draft']).parents[1]/'content/cache'/(parent+'.fmc')
    assert hashlib.sha256(source.read_bytes()).hexdigest() == parent
    receipt = {'status': 'RUNNING', 'parent': parent, 'fixture': str(args.fixture.resolve()), 'commands': []}
    for path in [Path(__file__).resolve(), args.tool.resolve(), source]:
        with path.open('rb') as file:
            receipt.setdefault('inputs', {})[str(path)] = hashlib.file_digest(file, 'sha256').hexdigest()

    def save():
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')

    def command(store, label, *arguments, succeeds=True):
        env = dict(os.environ, LOCALAPPDATA=str(output/'appdata'),
                   PATH=os.path.join(os.environ['SystemRoot'], 'System32'))
        argv = [str(args.tool.resolve()), '--root', str(store), *map(str, arguments)]
        result = subprocess.run(argv, cwd=output, env=env, capture_output=True,
                                encoding='utf-8', errors='replace', timeout=180,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        text = result.stdout+result.stderr
        (output/(label+'.log')).write_text(text, encoding='utf-8')
        receipt['commands'].append({'label': label, 'argv': argv, 'exit': result.returncode})
        save()
        assert (result.returncode == 0) == succeeds, (label, result.returncode, text)
        return text

    try:
        author, guest = output/'author', output/'guest'
        command(author, 'cache-parent', 'cache', source)
        command(author, 'accept-parent', 'accept', parent)
        draft = output/'outfit-draft'
        command(author, 'clone-outfit', 'clone-installed', parent, draft, 'Alternate Fox')
        assert (draft/'portrait.png').exists()
        built = command(author, 'other-parent', 'package-project', draft, draft/'portrait.png')
        other = re.search(r'Cached source fighter ([0-9a-f]{64})', built)[1]
        assert other != parent
        image = Image.new('RGBA', (32, 32))
        for y in range(32):
            for x in range(32):
                image.putpixel((x, y), (50, 240, 100, 255) if (x//8+y//8)%2 else (250, 250, 250, 255))
        image.save(output/'outfit.png')
        command(author, 'edit-texture', 'project-model-texture', draft, 0, output/'outfit.png')
        imported = command(author, 'import-outfit', 'import-custom-costume', draft/'costume.dat', parent, 'Green Fox Outfit', 3)
        outfit = re.search(r'ID: ([0-9a-f]{64})', imported)[1]
        receipt.update(outfit=outfit, other_parent=other)
        exported = output/'outfit.fmc'
        command(author, 'export-outfit', 'export', outfit, exported)
        assert hashlib.sha256(exported.read_bytes()).hexdigest() == outfit
        command(guest, 'cache-outfit-first', 'cache', exported)
        assert not list((guest/'installed').glob('*.fmi'))
        command(guest, 'reject-missing-parent', 'accept', outfit, succeeds=False)
        command(guest, 'cache-other-parent', 'cache', author/'cache'/(other+'.fmc'))
        command(guest, 'reject-wrong-parent', 'accept', outfit, succeeds=False)
        assert not list((guest/'installed').glob('*.fmi'))
        command(guest, 'cache-exact-parent', 'cache', source)
        assert not list((guest/'installed').glob('*.fmi'))
        marker = guest/'installed'/(outfit+'.fmi')
        marker.parent.mkdir(parents=True, exist_ok=True)
        marker.mkdir()
        command(guest, 'reject-blocked-outfit-marker', 'accept', outfit, succeeds=False)
        assert not (guest/'installed'/(parent+'.fmi')).exists()
        command(guest, 'retain-parent-before-fault', 'accept', parent)
        command(guest, 'preserve-existing-parent-on-fault', 'accept', outfit, succeeds=False)
        assert (guest/'installed'/(parent+'.fmi')).exists()
        marker.rmdir()
        command(guest, 'remove-explicit-parent', 'uninstall', parent)
        command(guest, 'accept-outfit', 'accept', outfit)
        assert (guest/'installed'/(outfit+'.fmi')).exists()
        assert (guest/'installed'/(parent+'.fmi')).exists(), 'Explicit outfit acceptance must install its exact fighter dependency'
        assert not (guest/'installed'/(other+'.fmi')).exists()
        before = {p.name: p.read_bytes() for p in (guest/'installed').glob('*.fmi')}
        command(guest, 'accept-duplicate', 'accept', outfit)
        assert before == {p.name: p.read_bytes() for p in (guest/'installed').glob('*.fmi')}
        receipt['status'] = 'PASS single-fighter costume dependency acceptance'
    except Exception as error:
        receipt.update(status='FAIL', error=repr(error))
        raise
    finally:
        save()


if __name__ == '__main__':
    main()
