"""Exercise received outfits in canonical room matches, rollback and Results."""
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


def wait(function, label, timeout=180):
    deadline = time.monotonic()+timeout
    while time.monotonic() < deadline:
        result = function()
        if result:
            return result
        time.sleep(.15)
    raise RuntimeError('Timed out: '+label)


def settings(profile):
    path = profile/'Freak Melee/versus-settings.fms'
    data = bytearray(path.read_bytes())
    assert len(data) == 32 and data[:4] == b'FMVS'
    data[4], data[5], data[6], data[10], data[15] = 1, 2, 1, 0, 255
    data[16:24] = bytes(8)
    digest = 2166136261
    for value in data[:28]:
        digest = ((digest ^ value)*16777619) & 0xffffffff
    struct.pack_into('<I', data, 28, digest)
    path.write_bytes(data)


def choose(client, fighter, title, port):
    layout = (client.data/'css-layout.bin').read_bytes()
    count = struct.unpack_from('<H', layout, 4)[0]
    ids = [layout[14+48*i:46+48*i].hex() for i in range(count)]
    index = ids.index(fighter)
    state, text = client.state(port)
    x, y = map(float, re.search(r'\[css-tile\] index='+str(index)+r' center=([\d.-]+),([\d.-]+)', text).groups())
    client.move(x-2.7, y+2, port)
    if state['hand'] != 1:
        client.tap('b', port=port)
    client.tap('a', port=port)
    for _ in range(5):
        text = client.inspect('css-cards')
        if re.search(r'\[css-card\].*physical='+str(port)+r' .*local=1 .*outfit='+re.escape(title)+r'(?:\r?\n|$)', text):
            return
        client.tap('x', port=port)
    raise RuntimeError('Installed outfit could not be selected: '+text)


def select_stage(client, port=0, confirm=True):
    text = wait(lambda: (s if '[sss-cursor]' in (s := client.inspect('sss-state')) else None), 'stage select')
    tile = re.search(r'\[sss-tile\] index=(\d+) stage=32 x=([\d.-]+) y=([\d.-]+)', text)
    assert tile, text
    index = tile[1]
    for axis in ['x', 'y']:
        for _ in range(40):
            text = client.inspect('sss-state')
            cursor = re.search(r'\[sss-cursor\] x=([\d.-]+) y=([\d.-]+)', text)
            # The stage icons slide into place during the entrance animation.
            tile = re.search(r'\[sss-tile\] index='+index+r' stage=32 x=([\d.-]+) y=([\d.-]+)', text)
            assert cursor, text
            assert tile, text
            target = float(tile[1 if axis == 'x' else 2])
            error = target-float(cursor[1 if axis == 'x' else 2])
            if abs(error) < .35:
                break
            magnitude = 80 if abs(error) > 4 else 45 if abs(error) > 1 else 35
            frames = min(20, max(1, round(abs(error)/(.03*(magnitude-30)))))
            client.command('tap', port, f's{axis}={magnitude if error > 0 else -magnitude}', f'frames={frames}')
            # Wait for the whole input pulse and its neutral sample before reading.
            time.sleep(frames/30+.4)
        else:
            raise RuntimeError('Stage cursor failed to reach '+axis)
    text = client.inspect('sss-state')
    assert 'selected='+index in text, text
    if confirm:
        client.tap('a', port=port)


def session(client):
    def sample():
        assert client.process.poll() is None
        text = (client.folder/'stdout.log').read_text(errors='replace')
        assert '[netplay] FAILED:' not in text and 'Checksum mismatch' not in text, text[-1600:]
        found = re.search(r'\[netplay\] Session ([0-9a-f]+) connected through room (\w+) as peer (\d+) with port mask 0x([0-9a-f]+)', text)
        return found.groups() if found else None
    return wait(sample, 'canonical netplay session')


def network(client):
    text = client.inspect('netplay-state')
    values = dict((key, int(value)) for key, value in re.findall(r'(frame|restores|replay_ticks|checksums|failed|input_stalls|wait_ms)=(\d+)', text))
    assert values.get('failed') == 0, text
    return values


def costumes(client, expected):
    text = client.inspect('costume-state')
    rows = re.findall(r'\[costume-player\] port=(\d+) outfit=([0-9a-f]{64}) members=(\d+)', text)
    assert len(rows) == 4, text
    for port, outfit, count in rows:
        slot = int(port)
        if slot < len(expected):
            assert outfit == expected[slot]['outfit'] and int(count) == len(expected[slot]['members']), text
        else:
            assert int(count) == 0, text
    actors = [dict(zip(['port', 'member', 'kind', 'variant', 'actor', 'dormant', 'actual', 'motion'], map(int, row)))
              for row in re.findall(r'\[costume-member\] port=(\d+) member=(\d+) kind=(\d+) variant=(\d+) actor=(\d+) dormant=(\d+) actual=(\d+) motion=(-?\d+)', text)]
    assert len(actors) == sum(len(slot['members']) for slot in expected), text
    for actor in actors:
        assert actor['actor'] and actor['variant'] == expected[actor['port']]['variant'], text
        if not actor['dormant']:
            assert actor['actual'], text
    return actors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'tool', 'iso', 'profile', 'gifts', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--case', choices=['single-alternate', 'paired', 'shared-eight-actors'])
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    gift_receipt = json.loads(args.gifts.read_text())
    assert gift_receipt['status'] == 'PASS native outfit and missing-parent gifts'
    variants = {case['label']: dict(case, variant=7, title='Coordinated '+case['label']) for case in gift_receipt['cases']}
    receipt = {'status': 'RUNNING', 'cases': [], 'inputs': {}}
    for path in [Path(__file__).resolve(), Path(__file__).with_name('review_native_css_room.py'),
                 args.exe.resolve(), args.tool.resolve(), args.gifts.resolve()]:
        with path.open('rb') as file:
            receipt['inputs'][str(path)] = hashlib.file_digest(file, 'sha256').hexdigest()

    def save():
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2), encoding='utf-8')

    # A second coordinated outfit verifies independent choices on shared PCs.
    base = output/'base'
    shutil.copytree(args.profile, base, ignore=shutil.ignore_patterns('projects'))
    settings(base)
    store = base/'Freak Melee/content'
    raw = (store/'cache'/(variants['climbers']['parent']+'.fmc')).read_bytes()
    assert hashlib.sha256(raw).hexdigest() == variants['climbers']['parent']
    offset = 208+struct.unpack_from('<I', raw, 8)[0]
    models = output/'alternate-models'
    models.mkdir()
    for member in range(raw[6]):
        entry = 80+64*member
        key = raw[entry:entry+32].split(b'\0')[0].decode()
        sizes = struct.unpack_from('<8I', raw, entry+32)
        start = offset+sum(sizes[:2])
        (models/(key+'.dat')).write_bytes(raw[start:start+sizes[2]])
        offset += sum(sizes)
    result = subprocess.run([str(args.tool.resolve()), '--root', str(store), 'import-custom-costume', str(models),
                             variants['climbers']['parent'], 'Alternate climbers', '2'], capture_output=True,
                            encoding='utf-8', errors='replace', timeout=180, creationflags=subprocess.CREATE_NO_WINDOW)
    assert result.returncode == 0, result.stderr
    (output/'alternate-import.log').write_text(result.stdout+result.stderr, encoding='utf-8')
    alt = re.search(r'ID: ([0-9a-f]{64})', result.stdout)[1]
    variants['alt'] = dict(variants['climbers'], outfit=alt, variant=2, title='Alternate climbers')
    cases = [('single-alternate', [['fox'], ['zelda']]),
             ('paired', [['zelda'], ['climbers']]),
             ('shared-eight-actors', [['climbers', 'alt'], ['alt', 'climbers']])]
    if args.case:
        cases = [case for case in cases if case[0] == args.case]
    receipt['requested_cases'] = [case[0] for case in cases]
    clients = []
    try:
        for label, seats in cases:
            print(label+': starting isolated clients', flush=True)
            root = output/label
            expected = [variants[key] for party in seats for key in party]
            case = {'label': label, 'expected': expected}
            for index, party in enumerate(seats):
                profile = root/('profile-'+str(index))
                shutil.copytree(base, profile)
                owned = {variants[key]['parent'] for key in party} | {variants[key]['outfit'] for key in party}
                remote = {variants[key]['parent'] for key in seats[1-index]} | {variants[key]['outfit'] for key in seats[1-index]}
                for identity in remote-owned:
                    for folder, suffix in [('cache', '.fmc'), ('installed', '.fmi')]:
                        (profile/'Freak Melee/content'/folder/(identity+suffix)).unlink(missing_ok=True)
                client = Client(args.exe.resolve(), args.iso.resolve(), profile, root/str(index),
                                f'Outfit Match {index}', window_size=(1280, 800), without_drafts=True,
                                launch_args=('--netplay-delay', 4 if index == 0 else 0))
                clients.append(client)
                if len(party) == 2:
                    client.command('hold', 1, 'sx=0')
                client.click('nav-1')
                if len(party) == 2:
                    client.click('local-players-2')
            room = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
            for i, client in enumerate(clients):
                client.enter('lobby-code', room); client.click('join-lobby')
                client.wait_cards(sum(len(party) for party in seats[:i+1])); client.click('lobby-ready')
            for index, client in enumerate(clients):
                client.wait_cards(len(expected))
                for port, key in enumerate(seats[index]):
                    choose(client, variants[key]['parent'], variants[key]['title'], port)
            print(label+': waiting for selected remote fighter and outfit packages', flush=True)
            for index, client in enumerate(clients):
                target = client.data/'content'
                owned = {variants[key]['parent'] for key in seats[index]} | {variants[key]['outfit'] for key in seats[index]}
                remote = {variants[key]['parent'] for key in seats[1-index]} | {variants[key]['outfit'] for key in seats[1-index]}
                for identity in remote-owned:
                    wait(lambda: (target/'cache'/(identity+'.fmc')).exists(), 'provisional cache '+identity[:12])
                    assert hashlib.sha256((target/'cache'/(identity+'.fmc')).read_bytes()).hexdigest() == identity
                    assert not (target/'installed'/(identity+'.fmi')).exists()
                client.screenshot(); client.click('play-ready')
                client.element('play-selection')
            wait(lambda: 'enabled=1' in clients[0].inspect('play-start'), 'host start with custom outfits')
            print(label+': starting canonical match', flush=True)
            clients[0].click('play-start')
            select_stage(clients[0])
            sessions = [session(client) for client in clients]
            assert sessions[0][:2] == sessions[1][:2], sessions
            human_mask = (1 << len(expected))-1
            assert int(sessions[0][3],16) & human_mask == (1 << len(seats[0]))-1, sessions
            assert int(sessions[1][3],16) & human_mask == human_mask ^ ((1 << len(seats[0]))-1), sessions
            wait(lambda: network(clients[0])['frame'] > 240, 'gameplay after Ready/Go')
            case.update(sessions=sessions, initial_actors=[costumes(client, expected) for client in clients])
            # Zelda/Sheik must retain the same coordinated outfit across a form change.
            for index, party in enumerate(seats):
                for port, key in enumerate(party):
                    if key == 'zelda':
                        canonical = sum(map(len,seats[:index]))+port
                        clients[index].command('hold', port, 'b', 'sy=-80')
                        time.sleep(.3)
                        clients[index].command('hold', port, 'sx=0')
                        def transformed():
                            actors = costumes(clients[index], expected)
                            return actors if any(a['port']==canonical and a['member']==1 and not a['dormant'] for a in actors) else None
                        try:
                            wait(transformed, 'coordinated Sheik form', 30)
                        except Exception:
                            print(label+': transform network '+json.dumps([network(c) for c in clients]), flush=True)
                            raise
                        case['transformed_actors'] = [costumes(client, expected) for client in clients]
            for index, client in enumerate(clients):
                for port in range(len(seats[index])):
                    client.command('hold', port, 'x')
                    time.sleep(.3)
                    client.command('hold', port, 'sx=0')
            wait(lambda: network(clients[0])['frame'] > 600, 'continued corrected gameplay')
            case['network'] = [network(client) for client in clients]
            print(label+': gameplay '+json.dumps(case['network']), flush=True)
            assert all(sample['checksums'] > 0 for sample in case['network'])
            assert any(sample['restores'] > 0 and sample['replay_ticks'] > 0 for sample in case['network'])
            case['captures'] = [str(client.screenshot()) for client in clients]
            # Walk every fighter except the last canonical slot off the right edge.
            last = len(expected)-1
            for index, client in enumerate(clients):
                for port in range(len(seats[index])):
                    if sum(map(len,seats[:index]))+port != last:
                        client.command('hold', port, 'sx=80')
            wait(lambda: all('[netplay] Compared ' in (client.folder/'stdout.log').read_text(errors='replace')
                             for client in clients), 'one-stock match end', 90)
            for index, client in enumerate(clients):
                for port in range(len(seats[index])):
                    client.command('hold', port, 'sx=0')
            case['results'] = [str(client.screenshot()) for client in clients]
            print(label+': leaving Results', flush=True)
            deadline = time.monotonic()+45
            while time.monotonic() < deadline:
                for index, client in enumerate(clients):
                    # Each local human has a separate Results confirmation.
                    for port in range(len(seats[index])):
                        client.tap('start', frames=6, port=port)
                if all('[css-selection] ready=' in client.inspect('css-selection') for client in clients):
                    break
            else:
                raise RuntimeError('Results did not return to home CSS')
            for client in clients:
                text = (client.folder/'stdout.log').read_text(errors='replace')+client.stderr.read_text(errors='replace')
                assert not re.search(r'Checksum mismatch|\[netplay\] FAILED:|Fighter source VM failed|native panic stack|refused during retained history', text), text[-1800:]
            receipt['cases'].append(case); save()
            print(label+': PASS', flush=True)
            for client in clients:
                client.stop()
            clients.clear()
        receipt['status'] = 'PASS received outfits in canonical matches and Results'
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
