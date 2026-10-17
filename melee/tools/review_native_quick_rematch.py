"""Exercise a real four-stock Quick Play result and rematch against a local service.

Stage votes use the native automation command while the stage-confirmation UI
awaits design review. All gameplay uses normal controller input. Clients are
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
    parser.add_argument('--doubles', action='store_true')
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    clients, assignments = [], []
    try:
        for index in range(2):
            c = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output/str(index),
                       f'Rematch Review {index+1}', window_size=(960,540))
            clients.append(c)
            if args.doubles:
                c.command('hold', 1, 'sx=0'); c.click('nav-1'); c.click('local-players-2')
            c.click('nav-0'); c.click('quick-doubles' if args.doubles else 'quick-singles')
            wait_quick(c, 0); choose(c)
            if args.doubles:
                choose(c, 1)
            c.click('play-ready'); c.element('play-selection')
        for c in clients:
            c.click('play-queue')
        for c in clients:
            assignments.append(wait_quick(c, 2))
        first = [live_session(c) for c in clients]
        assert first[0] == first[1]
        # Walk one side off stage until all four stocks are used.
        for port in range(2 if args.doubles else 1):
            clients[0].command('hold', port, 'sx=80')
        reports = []
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            reports = [re.search(r'\[quick-play\] result epoch=(\d+) winner=(\d+) queued=(\d+)',
                                 c.stderr.read_text(errors='replace')) for c in clients]
            if all(reports):
                break
            for c in clients:
                assert c.process.poll() is None
                log = (c.folder/'stdout.log').read_text(errors='replace')
                assert '[netplay] FAILED:' not in log and 'Checksum mismatch' not in log, log[-1800:]
            time.sleep(.2)
        else:
            raise RuntimeError('The four-stock round did not finish')
        for port in range(2 if args.doubles else 1):
            clients[0].command('clear', port)
        assert reports[0].groups() == reports[1].groups(), reports
        assert reports[0][2] == assignments[1]['team'] and reports[0][3] == '1'
        epoch = reports[0][1]
        time.sleep(2)
        for c in clients:
            for _ in range(30):
                for port in range(2 if args.doubles else 1):
                    c.tap('start', port=port)
                text = c.inspect('play-queue')
                if f'completed={epoch}' in text and 'phase=3' in text:
                    break
                time.sleep(.25)
            else:
                raise RuntimeError(f'{c.folder}: results did not return to the assigned CSS: {text}')
            assert f'chooser={assignments[0]["team"]}' in text
        stage = (int(assignments[0]['stage'])+1) % 6
        clients[1].command('room-stage', 0, stage)  # Winner has no pick authority.
        for seat in range(2 if args.doubles else 1):
            clients[0].command('room-stage', seat, stage)
        for c in clients:
            deadline = time.monotonic()+15
            while time.monotonic() < deadline:
                text = c.inspect('play-queue')
                if 'phase=0' in text and f'stage={stage}' in text:
                    break
                time.sleep(.1)
            else:
                raise RuntimeError(f'{c.folder}: stage agreement did not complete: {text}')
            c.click('play-ready'); c.element('play-selection')
        second = [live_session(c, first[0][0]) for c in clients]
        assert second[0] == second[1]
        assert second[0][0] != first[0][0] and second[0][1] == first[0][1]
        time.sleep(3)
        for c in clients:
            c.screenshot()
            log = (c.folder/'stdout.log').read_text(errors='replace')
            assert '[netplay] FAILED:' not in log and 'Checksum mismatch' not in log, log[-1800:]
        (output/'passed.txt').write_text(
            f'Native {"doubles" if args.doubles else "singles"} four-stock round, agreed results, '
            f'loser stage vote and same-opponent rematch {first[0][0]} -> {second[0][0]} passed.\n')
        print((output/'passed.txt').read_text().strip(), flush=True)
    finally:
        for c in reversed(clients):
            c.stop()


if __name__ == '__main__':
    main()
