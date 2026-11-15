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
from review_native_release import close_window


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
    os.environ['FREAK_MELEE_MENU_MOTION'] = '0'
    os.environ['FREAK_MELEE_COMMUNITY_ENDPOINT'] = 'http://localhost:1'
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
                        'Outfit Sender', window_size=(1440, 900), without_drafts=True)
        clients.append(sender)
        receiver = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output/'receiver',
                          'Outfit Receiver', window_size=(1440, 900), without_drafts=True)
        clients.append(receiver)
        room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        receipt['room'] = room
        for i, client in enumerate(clients):
            client.click('nav-1'); client.enter('lobby-code', room); client.click('join-lobby')
            client.wait_cards(i+1)
        for client in clients: client.wait_cards(2)
        # A guest can reach the rules entry, but cannot mutate host-owned rules.
        receiver.tap('b',40); receiver.click('nav-6'); receiver.click('settings-category-2'); receiver.click('settings-rules')
        assert 'page=23 ' in receiver.inspect('hub-input')
        receiver.click('settings-back'); receiver.click('settings-back'); receiver.click('return-session')
        # Leave fighters unselected so target parents cannot arrive through
        # the independent provisional-selection cache.
        sender.tap('b', 40); sender.click('nav-6');sender.click('settings-category-2'); sender.click('settings-packages')
        sender.click('collection-workshop'); sender.element('workshop-close'); sender.click('workshop-close')
        sender.element('collection-installed')
        receipt['native_menu_checks']=['guest cannot change host rules','Workshop returns to installed packages']
        def offer(identifier):
            if 'page=31 ' in sender.inspect('packages'): sender.click('collection-close')
            while 'enabled=1' in sender.inspect('collection-prev'): sender.click('collection-prev')
            while True:
                text=sender.inspect('packages')
                found=re.search(r'package index=(\d+) .*id='+identifier,text)
                if found: break
                assert 'enabled=1' in sender.inspect('collection-next'), text
                sender.click('collection-next')
            sender.click('collection-send-'+found[1])
            sender.click('collection-recipient-1')
            assert 'enabled=1' in sender.inspect('collection-confirm')
            sender.click('collection-confirm')
        baseline = {path.name for path in (receiver.data/'content/installed').glob('*.fmi')}
        for case in prepared['cases']:
            target = receiver.data/'content'
            for kind in ['parent', 'outfit']:
                assert not (target/'cache'/(case[kind]+'.fmc')).exists(), (case['label'], kind, 'unexpected precache')
                assert not (target/'installed'/(case[kind]+'.fmi')).exists()
            roster = (receiver.data/'css-layout.bin').read_bytes()
            offer(case['outfit'])
            receiver.element('play-accept')
            offered = receiver.screenshot()
            assert (receiver.data/'css-layout.bin').read_bytes() == roster
            receiver.click('play-accept')
            wait(lambda: all((target/'installed'/(case[kind]+'.fmi')).exists() for kind in ['parent', 'outfit']),
                 'coordinated outfit and exact parent installation')
            wait(lambda: 'package-transfer state=6 ' in sender.inspect('packages'), 'sender installation receipt')
            for kind in ['parent', 'outfit']:
                assert hashlib.sha256((target/'cache'/(case[kind]+'.fmc')).read_bytes()).hexdigest() == case[kind]
            changed = wait(lambda: (data if data != roster else None)
                           if (data := (receiver.data/'css-layout.bin').read_bytes()) else None,
                           'accepted parent roster tile')
            count = struct.unpack_from('<H', changed, 4)[0]
            tiles = [changed[14+48*i:46+48*i].hex() for i in range(count)]
            assert tiles.count(case['parent']) == 1
            before = {path.name: path.read_bytes() for path in (target/'installed').glob('*.fmi')}
            offer(case['outfit'])
            receiver.click('play-accept')
            wait(lambda: 'package-transfer state=6 ' in sender.inspect('packages'), 'cached repeat receipt')
            assert before == {path.name: path.read_bytes() for path in (target/'installed').glob('*.fmi')}
            assert changed == (receiver.data/'css-layout.bin').read_bytes()
            receipt['cases'].append({**case, 'offer_capture': str(offered), 'repeat': 'PASS'})
            save()
        assert len(list((receiver.data/'content/installed').glob('*.fmi'))) == len(baseline)+2*len(prepared['cases'])
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
            if client.process.poll() is None:
                close_window(client.process.pid); client.process.wait(timeout=20)
        save()


if __name__ == '__main__':
    main()
