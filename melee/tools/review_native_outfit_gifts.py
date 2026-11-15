"""Send coordinated outfits and missing parents between isolated native clients."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import struct
import subprocess
import time

from review_native_css_room import Client


def wait(function, label, seconds=120):
    deadline = time.monotonic()+seconds
    while time.monotonic() < deadline:
        value = function()
        if value:
            return value
        time.sleep(.15)
    raise RuntimeError('Timed out: '+label)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'tool', 'iso', 'profile', 'prepared', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    prepared = json.loads(args.prepared.read_text())
    assert prepared['status'] == 'PASS coordinated source-family outfits'
    receipt = {'status': 'RUNNING', 'inputs': {}, 'cases': [], 'endpoint': args.endpoint}
    for path in [Path(__file__).resolve(), args.exe.resolve(), args.tool.resolve(), args.prepared.resolve()]:
        with path.open('rb') as file:
            receipt['inputs'][str(path)] = hashlib.file_digest(file, 'sha256').hexdigest()

    def save():
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')

    def cli(store, label, *parts):
        env = dict(os.environ, LOCALAPPDATA=str(output/'cli-appdata'), PATH=os.path.join(os.environ['SystemRoot'], 'System32'))
        result = subprocess.run([str(args.tool.resolve()), '--root', str(store), *map(str, parts)],
                                env=env, capture_output=True, encoding='utf-8', errors='replace', timeout=180,
                                creationflags=subprocess.CREATE_NO_WINDOW)
        (output/(label+'.log')).write_text(result.stdout+result.stderr, encoding='utf-8')
        assert result.returncode == 0, (label, result.stderr)
        return result.stdout

    sender_profile = output/'sender-profile'
    shutil.copytree(args.profile, sender_profile, ignore=shutil.ignore_patterns('projects'))
    store = sender_profile/'Freak Melee/content'
    for case in prepared['cases']:
        for kind in ['parent', 'outfit']:
            source = args.prepared.resolve().parent/case['label']/'content/cache'/(case[kind]+'.fmc')
            cli(store, case['label']+'-cache-'+kind, 'cache', source)
        cli(store, case['label']+'-accept', 'accept', case['outfit'])
    clients = []
    try:
        sender = Client(args.exe.resolve(), args.iso.resolve(), sender_profile, output/'sender',
                        'Outfit Sender', window_size=(1600, 1000), without_drafts=True)
        clients.append(sender)
        receiver = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output/'receiver',
                          'Outfit Receiver', window_size=(1600, 1000), without_drafts=True)
        clients.append(receiver)
        room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        receipt['room'] = room
        for i, client in enumerate(clients):
            client.click('nav-1'); client.enter('lobby-code', room); client.click('join-lobby')
            client.wait_cards(i+1); client.click('lobby-ready')
        for client in clients:
            client.wait_cards(2)
            state, text = client.state()
            x, y = map(float, re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+)', text).groups())
            client.move(x-2.7, y+2)
            if state['hand'] != 1:
                client.tap('b')
            client.tap('a')
        # Keep a baseline fighter selected so none of the target parents is
        # downloaded by the independent provisional-selection cache.
        receiver.click('play-ready')
        wait(lambda: '[css-selection] practice' in receiver.inspect('css-selection'), 'receiver practice')
        sender.tap('b', 40); sender.click('nav-6');sender.click('settings-category-2'); sender.click('settings-packages')
        sender.click('collection-recipient'); sender.command('ui-key', 'down'); sender.command('ui-key', 'enter')
        assert re.search(r'\[ui-inspect-value\] collection-recipient: .*value=P[^ ]+', sender.inspect('collection-recipient')), 'Recipient was not selected'
        listing = cli(sender.data/'content', 'sender-list', 'list')
        ids = re.findall(r'^([0-9a-f]{64})  ', listing, re.M)
        baseline = {path.name for path in (receiver.data/'content/installed').glob('*.fmi')}
        for case in prepared['cases']:
            target = receiver.data/'content'
            for kind in ['parent', 'outfit']:
                assert not (target/'cache'/(case[kind]+'.fmc')).exists(), (case['label'], kind, 'unexpected precache')
                assert not (target/'installed'/(case[kind]+'.fmi')).exists()
            roster = (receiver.data/'css-layout.bin').read_bytes()
            sender.click('collection-send-'+str(ids.index(case['outfit'])))
            receiver.element('play-accept')
            offered = receiver.screenshot()
            assert (receiver.data/'css-layout.bin').read_bytes() == roster
            receiver.click('play-accept')
            wait(lambda: all((target/'installed'/(case[kind]+'.fmi')).exists() for kind in ['parent', 'outfit']),
                 'coordinated outfit and exact parent installation')
            wait(lambda: 'sent to Outfit Receiver' in sender.inspect('collection-transfer'), 'sender installation receipt')
            for kind in ['parent', 'outfit']:
                assert hashlib.sha256((target/'cache'/(case[kind]+'.fmc')).read_bytes()).hexdigest() == case[kind]
            changed = wait(lambda: (data if data != roster else None)
                           if (data := (receiver.data/'css-layout.bin').read_bytes()) else None,
                           'accepted parent roster tile')
            count = struct.unpack_from('<H', changed, 4)[0]
            tiles = [changed[14+48*i:46+48*i].hex() for i in range(count)]
            assert tiles.count(case['parent']) == 1
            before = {path.name: path.read_bytes() for path in (target/'installed').glob('*.fmi')}
            sender.click('collection-send-'+str(ids.index(case['outfit'])))
            receiver.click('play-accept')
            wait(lambda: 'sent to Outfit Receiver' in sender.inspect('collection-transfer'), 'cached repeat receipt')
            assert before == {path.name: path.read_bytes() for path in (target/'installed').glob('*.fmi')}
            assert changed == (receiver.data/'css-layout.bin').read_bytes()
            receipt['cases'].append({**case, 'offer_capture': str(offered), 'repeat': 'PASS'})
            save()
        assert len(list((receiver.data/'content/installed').glob('*.fmi'))) == len(baseline)+6
        assert not re.search(r'Fighter source VM failed|Source VM.*failed',
                             (receiver.folder/'stdout.log').read_text(errors='replace')+receiver.stderr.read_text(errors='replace'))
        receipt['status'] = 'PASS native outfit and missing-parent gifts'
    except Exception as error:
        receipt.update(status='FAIL', error=repr(error))
        for client in clients:
            if client.process.poll() is None:
                client.screenshot()
        raise
    finally:
        for client in clients:
            client.stop()
        save()


if __name__ == '__main__':
    main()
