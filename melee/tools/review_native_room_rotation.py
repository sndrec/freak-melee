"""Check native seat claims, rotation and promoted shared-PC input routing."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import time

from review_native_css_room import Client
from review_native_outfit_matches import network, select_stage, settings, wait


def room(client):
    text = client.inspect('room-state')
    header = re.search(r'\[room-state\] state=(\d+) local=(\w*) owner=(\w*) revision=(\d+) epoch=(\d+) match=(\d+) rotation=(\d+) members=(\d+)', text)
    assert header, text
    state = dict(zip(['state', 'local', 'owner', 'revision', 'epoch', 'match', 'rotation', 'count'], header.groups()))
    for key in ['state', 'revision', 'epoch', 'match', 'rotation', 'count']:
        state[key] = int(state[key])
    state['members'] = {}
    for row in re.findall(r'\[room-member\] index=(\d+) peer=(\w+) seats=(\d+) active=(\d+) waiting=(\d+) ranks=([\d,]+) confirmed=(\d+) name=([^\r\n]+)', text):
        index, peer, seats, active, waiting, ranks, confirmed, name = row
        state['members'][peer] = dict(index=int(index), seats=int(seats), active=int(active),
                                     waiting=int(waiting), ranks=list(map(int, ranks.split(','))),
                                     confirmed=int(confirmed), name=name)
    assert len(state['members']) == state['count'], text
    stream = re.search(r'\[room-stream\] session=([0-9a-f]+) records=(\d+) phase=(\d+) next=(\d+) finished=(\d+) unavailable=(\d+) seed=([0-9a-f]+) digest=([0-9a-f]+) tail_kind=(\d+) tail_frame=(\d+) tail_state=([0-9a-f]+)', text)
    if stream:
        state['stream'] = {key: int(value,16 if key in ['session','seed','digest','tail_state'] else 10)
                           for key,value in zip(['session','records','phase','next','finished','unavailable','seed','digest','tail_kind','tail_frame','tail_state'],stream.groups())}
    return state


def click_card(client, card, port, mouse=False):
    text = client.inspect('css-cards')
    match = re.search(rf'\[css-card\] [^\r\n]*?center=([-\d.]+),[^\r\n]*? card={card}\r?$', text, re.M)
    assert match, text
    x, y = float(match[1]) - 4.1, -4.6
    if mouse:
        assert port == 0
        client.command('ui-click', *client.pixel(x, y))
    else:
        client.move(x, y, port)
        client.tap('A', frames=5, port=port)


def masks(clients, host, guest, active, waiting):
    def sample():
        states = [room(c) for c in clients]
        for state in states:
            if state['match'] or state['members'][host]['active'] != active or state['members'][host]['waiting'] != waiting:
                return None
            if state['members'][guest]['active'] != 1 or state['members'][guest]['waiting']:
                return None
        return states
    return wait(sample, 'matching room seat masks')


def rotation(client, policy):
    for _ in range(4):
        previous = room(client)['rotation']
        if previous == policy:
            return
        client.click('css-rotation')
        wait(lambda: room(client)['rotation'] != previous, 'rotation policy', 15)
    raise RuntimeError('Rotation policy did not update')


def choose(client, identity, port):
    import struct
    raw = (client.data/'css-layout.bin').read_bytes()
    count = struct.unpack_from('<H', raw, 4)[0]
    index = [raw[14+48*i:46+48*i].hex() for i in range(count)].index(identity)
    state, text = client.state(port)
    x, y = map(float, re.search(r'\[css-tile\] index='+str(index)+r' center=([\d.-]+),([\d.-]+)', text).groups())
    client.move(x-2.7, y+2, port)
    if state['hand'] != 1:
        client.tap('b', frames=6, port=port)
    client.tap('a', frames=6, port=port)
    assert client.state(port)[0]['chosen'] == 1


def players(client):
    text = client.inspect('netplay-state')
    result = {}
    for p, physical, kind, x, y, motion in re.findall(r'\[netplay-player\] port=(\d+) physical=(-?\d+) kind=(\d+) x=([\d.-]+) y=([\d.-]+) motion=(-?\d+)', text):
        result[int(p)] = dict(physical=int(physical), kind=int(kind), x=float(x), y=float(y), motion=int(motion))
    assert len(result) == 2, text
    return result


def session(client, count):
    def sample():
        text = (client.folder/'stdout.log').read_text(errors='replace')
        assert '[netplay] FAILED:' not in text, text[-1800:]
        found = re.findall(r'\[netplay\] Session ([0-9a-f]+) connected through room (\w+) as peer (\d+) with port mask 0x([0-9a-f]+)', text)
        return found[-1] if len(found) == count else None
    return wait(sample, 'new canonical match session')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ['exe', 'iso', 'profile', 'families', 'output']:
        parser.add_argument('--'+key, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--delay-peer',type=int,choices=[0,1],default=0)
    parser.add_argument('--delay-frames',type=int,choices=range(15),default=4)
    parser.add_argument('--require-stream',action='store_true')
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    base = output/'base'
    shutil.copytree(args.profile, base, ignore=shutil.ignore_patterns('projects'))
    settings(base)
    families = {case['label']: case['parent'] for case in json.loads(args.families.read_text())['cases']}
    receipt = {'status': 'RUNNING', 'rounds': [], 'inputs': {str(path.resolve()): hashlib.sha256(path.read_bytes()).hexdigest()
               for path in [Path(__file__), Path(__file__).with_name('review_native_css_room.py'),
                            Path(__file__).with_name('review_native_outfit_matches.py'), args.exe, args.families]}}
    clients = []
    try:
        for index in range(2):
            c = Client(args.exe.resolve(), args.iso.resolve(), base, output/str(index),
                       'RotationHost' if index == 0 else 'RotationGuest', window_size=(1280,800),
                       without_drafts=True, launch_args=('--netplay-delay',args.delay_frames if index == args.delay_peer else 0))
            clients.append(c)
            if index == 0:
                c.command('hold', 1, 'sx=0')
                c.click('nav-6')
                c.click('settings-category-4')
                c.click('local-players-2')
                c.click('settings-back')
                c.click('settings-back')
            c.click('nav-1')
        code = ''.join(secrets.choice('ABCDEFGHJKLMNPQRSTUVWXYZ23456789') for _ in range(10))
        for index, c in enumerate(clients):
            c.enter('lobby-code',code); c.click('join-lobby')
            wait(lambda: room(c)['count'] == index+1, 'room membership')
        wait(lambda: room(clients[0])['count'] == 2, 'owner sees guest')
        host, guest = [room(c)['local'] for c in clients]
        click_card(clients[0],1,1)
        masks(clients,host,guest,1,0)
        click_card(clients[1],0,0)
        masks(clients,host,guest,1,0)
        choose(clients[0],families['climbers'],1)
        masks(clients,host,guest,1,0)
        rotation(clients[0],2)
        receipt['seat_choices'] = masks(clients,host,guest,1,2)
        clients[0].tap('B',frames=4,port=1)
        masks(clients,host,guest,1,0)
        choose(clients[0],families['climbers'],1)
        masks(clients,host,guest,1,2)
        print('PASS card ownership, Manual rotation and token placement controlling the queue', flush=True)
        for number, policy in enumerate([2,1],1):
            active_physical = number-1
            inactive = 1-active_physical
            rotation(clients[0],policy)
            wait(lambda: room(clients[1])['rotation'] == policy, 'guest sees rotation')
            assert 'enabled=0' in clients[1].inspect('css-rotation')
            choose(clients[0],families['fox'],0)
            choose(clients[0],families['climbers'],1)
            choose(clients[1],families['fox'],0)
            clients[0].tap('START',frames=4,port=active_physical)
            clients[1].tap('START',frames=4,port=0)
            for c in clients:
                wait(lambda: '[sss-cursor]' in c.inspect('sss-state'), 'stage selection',90)
            chooser = wait(lambda: next(((c, int(m[1])) for c in clients
                if (m := re.search(r'\[room-stage\].*physical=([0-3])', c.inspect('room-state')))), None),
                'active stage chooser')
            select_stage(*chooser)
            sessions = [session(c,number) for c in clients]
            assert sessions[0][:2] == sessions[1][:2]
            for index,c in enumerate(clients):
                delays=re.findall(r'\[netplay\] Diagnostic outbound delay (\d+) frames',
                                  (c.folder/'stdout.log').read_text(errors='replace'))
                assert len(delays)==number and int(delays[-1])==(args.delay_frames if index==args.delay_peer else 0),delays
            wait(lambda: network(clients[0])['frame'] > 240, 'normal gameplay')
            entry = {'policy':policy, 'sessions':sessions, 'before':players(clients[0])}
            assert entry['before'][0]['physical'] == active_physical
            assert players(clients[1])[1]['physical'] == 0
            frame = network(clients[0])['frame']
            clients[0].command('hold',inactive,'sx=80')
            wait(lambda: network(clients[0])['frame'] >= frame+25, 'inactive controller sample')
            clients[0].command('hold',inactive,'sx=0')
            still = players(clients[0])
            assert abs(still[0]['x']-entry['before'][0]['x']) < .5, still
            frame = network(clients[0])['frame']
            clients[0].command('hold',active_physical,'sx=40')
            wait(lambda: network(clients[0])['frame'] >= frame+15, 'active controller sample')
            clients[0].command('hold',active_physical,'sx=0')
            entry['moved'] = players(clients[0])
            assert entry['moved'][0]['x'] > still[0]['x']+1, entry
            assert abs(entry['moved'][1]['x']-still[1]['x']) < .5, entry
            entry['network'] = [network(c) for c in clients]
            assert all(s['checksums'] for s in entry['network'])
            entry['gameplay'] = [str(c.screenshot()) for c in clients]
            # Round one: host wins and rotates out. Round two: host loses;
            # winner-stays keeps the guest and promotes the host's waiting seat.
            loser, port, direction = (clients[1],0,80) if number == 1 else (clients[0],1,-80)
            loser.command('hold',port,f'sx={direction}')
            wait(lambda: all((c.folder/'stdout.log').read_text(errors='replace').count('[netplay] Compared ') >= number for c in clients), 'completed match',90)
            loser.command('hold',port,'sx=0')
            time.sleep(3)
            clients[0].tap('start',frames=6,port=inactive)
            time.sleep(1)
            assert '[css-selection] inactive' in clients[0].inspect('css-selection')
            entry['results'] = [str(c.screenshot()) for c in clients]
            deadline = time.monotonic()+45
            while time.monotonic() < deadline:
                done = []
                for index,c in enumerate(clients):
                    returned = '[css-selection] ready=' in c.inspect('css-selection')
                    done.append(returned)
                    if not returned:
                        c.tap('start',frames=6,port=active_physical if index == 0 else 0)
                if all(done): break
                time.sleep(.3)
            else: raise RuntimeError('Assigned Results controllers did not return')
            expected_active = 2 if number == 1 else 1
            entry['returned'] = masks(clients,host,guest,expected_active,3^expected_active)
            assert all(not s['members'][host]['confirmed'] and not s['members'][guest]['confirmed'] for s in entry['returned'])
            if args.require_stream:
                stream=entry['returned'][0]['stream']
                end=re.findall(r'\[netplay\] Endpoint frame (\d+): fighter digest ([0-9a-f]+), RNG ([0-9a-f]+)',
                               (clients[0].folder/'stdout.log').read_text(errors='replace'))[-1]
                frame=int(end[0]); digest=((int(end[1],16)^int(end[2],16))*1099511628211)&((1<<64)-1)
                assert stream['session']==int(sessions[0][0],16)
                assert stream['records']==frame+1 and stream['next']==frame and stream['finished']==1
                assert not stream['unavailable'] and stream['phase']==0
                assert stream['tail_kind']==1 and stream['tail_frame']==frame and stream['tail_state']==digest
                assert entry['network'][0]['restores']>0, entry['network']
                entry['stream']=stream
            receipt['rounds'].append(entry)
            (output/'receipt.json').write_text(json.dumps(receipt,indent=2))
            print(f'PASS round {number}: active physical {active_physical}, inactive input ignored, rotation and Results',flush=True)
        for c in clients:
            text=(c.folder/'stdout.log').read_text(errors='replace')+c.stderr.read_text(errors='replace')
            assert not re.search(r'Checksum mismatch|\[netplay\] FAILED:|Fighter source VM failed|native panic stack|refused during retained history',text),text[-2000:]
        receipt['status']='PASS native shared-PC seat choices, both rotations and promoted controller routing'
    except Exception as error:
        receipt.update(status='FAIL',error=repr(error))
        for c in clients:
            if c.process.poll() is None:
                c.screenshot()
        raise
    finally:
        for c in clients:c.stop()
        (output/'receipt.json').write_text(json.dumps(receipt,indent=2))


if __name__ == '__main__':
    main()
