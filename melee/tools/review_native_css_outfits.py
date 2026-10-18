"""Review X/Y outfit selection and player-card labels in a hidden, muted game."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import time

from review_native_css_room import Client


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'tool', 'profile', 'prepared', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    prepared = json.loads(args.prepared.read_text())
    assert prepared['status'] == 'PASS coordinated source-family outfits'
    receipt = {'status': 'RUNNING', 'cases': [], 'inputs': {}}
    for path in [Path(__file__).resolve(), args.exe.resolve(), args.tool.resolve(), args.prepared.resolve()]:
        with path.open('rb') as file:
            receipt['inputs'][str(path)] = hashlib.file_digest(file, 'sha256').hexdigest()

    def save():
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')

    base = output/'profile'
    shutil.copytree(args.profile, base, ignore=shutil.ignore_patterns('projects'))
    store = base/'Freak Melee/content'
    env = dict(os.environ, LOCALAPPDATA=str(base), PATH=os.path.join(os.environ['SystemRoot'], 'System32'))
    for case in prepared['cases']:
        package = args.prepared.resolve().parent/case['label']/'content/cache'/(case['outfit']+'.fmc')
        for command in [('cache', package), ('accept', case['outfit'])]:
            result = subprocess.run([str(args.tool.resolve()), '--root', str(store), *map(str, command)],
                                    env=env, capture_output=True, timeout=180,
                                    creationflags=subprocess.CREATE_NO_WINDOW)
            assert result.returncode == 0, result.stderr
    client = None
    try:
        client = Client(args.exe.resolve(), args.iso.resolve(), base, output/'client', 'Outfit review', window_size=(1280, 800))
        client.click('local-practice')
        time.sleep(.5)
        layout = (client.data/'css-layout.bin').read_bytes()
        count = struct.unpack_from('<H', layout, 4)[0]
        ids = [layout[14+48*i:46+48*i].hex() for i in range(count)]
        for case in prepared['cases']:
            index = ids.index(case['parent'])
            state, text = client.state()
            tile = re.search(r'\[css-tile\] index='+str(index)+r' center=([\d.-]+),([\d.-]+) custom=1', text)
            assert tile, text
            x, y = map(float, tile.groups())
            if state['hand'] != 1:
                client.tap('b')
            client.move(x-2.7, y+2)
            client.tap('a')
            client.tap('x')
            text = client.inspect('css-cards')
            assert 'outfit=Coordinated '+case['label'] in text, text
            # Picking up and replacing a cookie on the same fighter preserves its outfit.
            client.tap('b')
            client.tap('a')
            assert 'outfit=Coordinated '+case['label'] in client.inspect('css-cards')
            image = client.screenshot()
            client.tap('y')
            default = client.inspect('css-cards')
            assert not re.search(r'\[css-card\] display=0 .*outfit=Coordinated', default), default
            client.tap('y')
            wrapped = client.inspect('css-cards')
            assert 'outfit=Coordinated '+case['label'] in wrapped, wrapped
            client.tap('x')
            assert not re.search(r'\[css-card\] display=0 .*outfit=Coordinated', client.inspect('css-cards'))
            client.tap('x')
            client.click('play-ready')
            deadline = time.monotonic()+90
            while '[css-selection] practice' not in client.inspect('css-selection'):
                assert time.monotonic() < deadline, 'Outfit did not reach practice'
                time.sleep(.2)
            time.sleep(5)
            practice = client.screenshot()
            client.click('play-selection')
            client.click('local-practice')
            client.state()
            receipt['cases'].append({'label': case['label'], 'card': text, 'screenshot': str(image),
                                     'practice': str(practice), 'cycling': 'PASS'})
            save()
        receipt['status'] = 'PASS native CSS coordinated outfit controls'
    except Exception as error:
        receipt.update(status='FAIL', error=repr(error))
        raise
    finally:
        if client:
            client.stop()
        save()


if __name__ == '__main__':
    main()
