"""Review CSS return, configured hands, and Quick Play's direct stage flow.

Uses isolated profiles, hidden/muted owned clients, and ordinary game input.
"""
import argparse
import json
import os
from pathlib import Path
import re
import secrets
import time

from review_native_css_room import Client
from review_native_release import close_window
from review_native_room_rotation import choose, room
from review_native_outfit_matches import wait, network
from review_native_quick_play import wait_quick
from review_native_quick_rematch import finish_round, live_session


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    args = parser.parse_args()
    assert args.endpoint.startswith(('http://127.0.0.1:', 'http://localhost:'))
    os.environ.update(FREAK_MELEE_SERVICE_ENDPOINT=args.endpoint,
                      FREAK_MELEE_COMMUNITY_ENDPOINT='http://127.0.0.1:1',
                      FREAK_MELEE_MENU_MOTION='0')
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    clients = []
    result = {'status': 'RUNNING', 'screenshots': {}}

    def new(name):
        client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                        output/name, name, window_size=(1440, 900))
        clients.append(client)
        return client

    def inspect(client, kind, pattern, timeout=90):
        return wait(lambda: text if re.search(pattern, text := client.inspect(kind)) else None,
                    pattern, timeout)

    def select(client):
        raw = (client.data/'css-layout.bin').read_bytes()
        choose(client, raw[14:46].hex(), 0)

    def close():
        for client in clients:
            if client.process.poll() is None:
                close_window(client.process.pid)
                client.process.wait(timeout=20)
        clients.clear()

    try:
        client = new('css-return')
        client.click('nav-1')
        client.enter('lobby-code', 'UI'+secrets.token_hex(4).upper())
        client.click('join-lobby')
        wait(lambda: room(client)['count'] == 1, 'one-person lobby', 25)
        text = client.inspect('css-selection')
        assert re.findall(r'\[css-slot\] slot=\d+ kind=(\d+)', text) == ['0', '3', '3', '3'], text
        ready = client.element('play-ready')
        copy = client.element('css-chat-code')
        assert copy[1] > ready[1] and abs(copy[0]-ready[0]) < 2
        result['screenshots']['one-local-hand'] = str(client.screenshot())
        select(client)
        client.tap('START', frames=4)
        inspect(client, 'practice-state', r'practice=1 ready=1 paused=0')
        inspect(client, 'practice-state', r'hud=1')
        client.tap('START', frames=4)
        inspect(client, 'practice-state', r'practice=1 ready=0 paused=2')
        client.command('tap', 0, 'L', 'R', 'A', 'START', 'frames=4')
        inspect(client, 'css-selection', r'\[css-slot\] slot=0 ')
        assert 'origin=' not in client.inspect('return-session')
        client.element('play-ready')
        result['screenshots']['exit-to-css'] = str(client.screenshot())
        close()

        for index in range(2):
            client = new('quick-'+str(index))
            client.click('nav-0')
            client.click('quick-singles')
            select(client)
            client.click('play-queue')
        assignments = [wait_quick(client, 2) for client in clients]
        for client in clients:
            client.tap('START', frames=4)
            inspect(client, 'practice-state', r'practice=1 ready=1 paused=0')
        first = [live_session(client) for client in clients]
        assert first[0] == first[1]
        wait(lambda: network(clients[0]).get('frame', 0) > 120, 'first Quick Play match', 90)
        epoch, winner = finish_round(clients, [1, 1], [0])
        assert winner == int(assignments[1]['team'])
        picker, observer = clients
        for client in clients:
            deadline = time.monotonic()+45
            while time.monotonic() < deadline:
                if 'origin=' in client.inspect('stage-pick-3'): break
                # The round report may reach phase 3 before the results
                # animation accepts START to leave the results scene.
                if '[css-slot]' not in client.inspect('css-selection'):
                    client.tap('START', frames=4)
                time.sleep(.4)
            else: raise AssertionError('Results did not open Quick Play stage choice')
        picker.command('ui-move', *picker.element('stage-pick-3'))
        inspect(observer, 'room-state', rf'\[room-cursor\] peer={assignments[0]["peer"]} seat=0 tile=3 ')
        result['screenshots']['quick-stage-observer'] = str(observer.screenshot())
        picker.click('stage-pick-3')
        for client in clients:
            inspect(client, 'quick-play', rf'phase=0 .*completed={epoch}')
            client.element('play-ready')
            client.tap('START', frames=4)
        second = [live_session(client, first[0][0]) for client in clients]
        assert second[0] == second[1]
        for client in clients:
            log = (client.folder/'stdout.log').read_text(errors='replace')+client.stderr.read_text(errors='replace')
            assert log.count('[match-setup] Automatic stage;') >= 2 and '[stage-select] Entered' not in log
            assert '[netplay] FAILED:' not in log and 'native panic stack' not in log
        result.update(status='PASS', first_session=first[0], rematch_session=second[0], completed_epoch=epoch)
    finally:
        close()
        (output/'review.json').write_text(json.dumps(result, indent=2)+'\n')
    print('PASS configured hands, practice exit, Quick Play cursor and direct starts')


if __name__ == '__main__':
    main()
