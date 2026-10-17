"""Exercise a real four-stock Quick Play result and rematch against a local service.

Stage selection uses the native page and teammate confirmation controls.
All gameplay uses normal controller input. Clients are
hidden, muted, resized and isolated from the installation's actual profile.
"""
import argparse
import os
from pathlib import Path
import re
import time

from review_native_css_room import Client
from review_native_quick_play import choose, wait_quick


def live_session(client, previous=None, timeout=90):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        log = (client.folder/'stdout.log').read_text(errors='replace')
        if '[netplay] FAILED:' in log or 'Checksum mismatch' in log:
            raise RuntimeError(f'{client.folder}: {log[-2000:]}')
        matches = re.findall(r'\[netplay\] Session ([0-9a-f]+) connected through room (\w+)', log)
        if matches and matches[-1][0] != previous:
            return matches[-1]
        if client.process.poll() is not None:
            raise RuntimeError(f'{client.folder}: game exited {client.process.returncode}')
        time.sleep(.15)
    raise RuntimeError(f'{client.folder}: match did not start')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--control',type=Path)
    parser.add_argument('--next-stage',type=int,choices=range(6))
    parser.add_argument('--rematch-seconds',type=float,default=3)
    parser.add_argument('--jump-in-place',action='store_true',help='Exercise repeated jump/landing in the rematch')
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--doubles', action='store_true')
    modes.add_argument('--split-party', action='store_true',
                       help='Two separate-PC teammates against a shared-PC pair')
    modes.add_argument('--four-solos', action='store_true',
                       help='Four independent clients in the doubles queue')
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    clients, assignments, initials = [], [], []
    players = [1,1,1,1] if args.four_solos else [1,1,2] if args.split_party else [2,2] if args.doubles else [1,1]
    leaders = [0,2] if args.split_party else list(range(len(players)))
    doubles = args.doubles or args.split_party or args.four_solos
    party_room = ''
    try:
        for index, seats in enumerate(players):
            c = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output/str(index),
                       f'Rematch Review {index+1}', window_size=(960,540),
                       control=args.control.resolve() if args.control else None)
            clients.append(c)
            if seats == 2:
                c.command('hold', 1, 'sx=0'); c.click('nav-1'); c.click('local-players-2')
            if args.split_party and index == 1:
                c.click('nav-1'); c.enter('lobby-code', party_room); c.click('join-lobby')
                c.wait_cards(4); c.click('lobby-ready')
            else:
                c.click('nav-0'); c.click('quick-doubles' if doubles else 'quick-singles')
            initial = wait_quick(c, 0)
            initials.append(initial)
            if index == 0:
                party_room = initial['room']
            elif args.split_party and index == 1:
                assert initial['room'] == party_room and initial['mode'] == '2'
                assert 'enabled=0' in c.inspect('play-queue'), 'Only the party leader enters queue'
            choose(c)
            if seats == 2:
                choose(c, 1)
            c.click('play-ready'); c.element('play-selection')
        for index in leaders:
            clients[index].click('play-queue')
        for c in clients:
            assignments.append(wait_quick(c, 2))
        assert all(initial['peer'] == assigned['peer'] for initial, assigned in zip(initials, assignments))
        losers = [i for i, assignment in enumerate(assignments) if assignment['team'] == assignments[0]['team']]
        winner = next(i for i in range(len(clients)) if i not in losers)
        if args.split_party:
            assert assignments[0]['team'] == assignments[1]['team'] != assignments[2]['team']
        first = [live_session(c) for c in clients]
        assert all(session == first[0] for session in first)
        # Move one side off stage, jumping past opponents that block the ledge.
        for index in losers:
            for port in range(players[index]):
                clients[index].command('hold', port, 'sx=80')
        reports = []
        deadline = time.monotonic() + 90
        next_jump = time.monotonic()+2
        while time.monotonic() < deadline:
            reports = [re.search(r'\[quick-play\] result epoch=(\d+) winner=(\d+) queued=(\d+)',
                                 c.stderr.read_text(errors='replace')) for c in clients]
            if all(reports):
                break
            if time.monotonic() >= next_jump:
                for index in losers:
                    for port in range(players[index]):
                        clients[index].command('hold',port,'x','sx=80')
                time.sleep(.08)
                for index in losers:
                    for port in range(players[index]):
                        clients[index].command('hold',port,'sx=80')
                next_jump=time.monotonic()+3
            for c in clients:
                assert c.process.poll() is None
                log = (c.folder/'stdout.log').read_text(errors='replace')
                assert '[netplay] FAILED:' not in log and 'Checksum mismatch' not in log, log[-1800:]
            time.sleep(.2)
        else:
            raise RuntimeError('The four-stock round did not finish')
        for index in losers:
            for port in range(players[index]):
                clients[index].command('clear', port)
        assert all(report.groups() == reports[0].groups() for report in reports), reports
        assert reports[0][2] == assignments[winner]['team'] and reports[0][3] == '1'
        epoch = reports[0][1]
        time.sleep(2)
        for index, c in enumerate(clients):
            for _ in range(30):
                for port in range(players[index]):
                    c.tap('start', port=port)
                text = c.inspect('quick-play')
                if f'completed={epoch}' in text and 'phase=3' in text:
                    break
                time.sleep(.25)
            else:
                raise RuntimeError(f'{c.folder}: results did not return to the assigned CSS: {text}')
            assert f'chooser={assignments[0]["team"]}' in text
            if doubles:
                cards = c.wait_cards(4)
                assert len(re.findall(r'\[css-card\].*local=1', cards)) == players[index], cards
        stage = args.next_stage if args.next_stage is not None else (int(assignments[0]['stage'])+1) % 6
        for c in clients:
            c.element('stage-leave')
        clients[losers[0]].tap('b')
        clients[losers[0]].element('play-stage')
        clients[losers[0]].click('play-stage')
        clients[losers[0]].element('stage-leave')
        for candidate in range(6):
            assert 'enabled=0' in clients[winner].inspect(f'stage-pick-{candidate}')
        clients[winner].command('room-stage', 0, stage)  # Service also rejects a winner.
        picker=clients[losers[0]]
        pick_seat=players[losers[0]]-1
        if pick_seat:picker.click(f'stage-seat-{pick_seat}')
        if doubles:
            for candidate in range(6):
                picker.click(f'stage-pick-{candidate}')
                deadline=time.monotonic()+10
                while time.monotonic()<deadline:
                    text=picker.inspect('quick-play')
                    if re.search(r'votes=(?:\d+,)*'+str(candidate)+r'(?:,|\s)',text):break
                    time.sleep(.1)
                else:raise RuntimeError('Stage selection did not arrive: '+text)
            picker.screenshot()
            picker.click(f'stage-withdraw-{pick_seat}')
            assert 'phase=3' in picker.inspect('quick-play')
        picker.click(f'stage-pick-{stage}')
        if doubles:
            confirmer=picker if len(losers)==1 else clients[losers[1]]
            confirm_seat=0
            confirmer.element(f'stage-confirm-{confirm_seat}')
            confirmer.screenshot()
            # Hover with the ordinary pointer; A comes from the confirming pad.
            confirmer.command('ui-move',*confirmer.element(f'stage-confirm-{confirm_seat}'))
            if pick_seat and confirmer is picker:
                confirmer.tap('a',port=pick_seat)
                assert 'phase=3' in confirmer.inspect('quick-play'), 'Picker confirmed for their teammate'
            confirmer.tap('a',port=confirm_seat)
        for c in clients:
            deadline = time.monotonic()+15
            while time.monotonic() < deadline:
                text = c.inspect('quick-play')
                if 'phase=0' in text and f'stage={stage}' in text:
                    break
                time.sleep(.1)
            else:
                raise RuntimeError(f'{c.folder}: stage agreement did not complete: {text}')
            c.click('play-ready'); c.element('play-selection')
        second = [live_session(c, first[0][0]) for c in clients]
        assert all(session == second[0] for session in second)
        assert second[0][0] != first[0][0] and second[0][1] == first[0][1]
        deadline=time.monotonic()+max(3,args.rematch_seconds)
        next_jump=time.monotonic()
        while time.monotonic()<deadline:
            if args.jump_in_place and time.monotonic()>=next_jump:
                clients[0].command('tap',0,'x','frames=2')
                next_jump=time.monotonic()+3
            for c in clients:
                assert c.process.poll() is None, f'{c.folder}: exited {c.process.returncode}'
            time.sleep(.1)
        for c in clients:
            c.screenshot()
            log = (c.folder/'stdout.log').read_text(errors='replace')
            assert '[netplay] FAILED:' not in log and 'Checksum mismatch' not in log, log[-1800:]
        (output/'passed.txt').write_text(
            f'Native {"four-solo doubles" if args.four_solos else "separate-PC premade doubles" if args.split_party else "shared-PC doubles" if doubles else "singles"} '
            f'four-stock round, agreed results, '
            f'loser stage vote and same-opponent rematch {first[0][0]} -> {second[0][0]} passed.\n')
        print((output/'passed.txt').read_text().strip(), flush=True)
    finally:
        for c in reversed(clients):
            c.stop()


if __name__ == '__main__':
    main()
