"""Review Quick Play practice, B-to-unready, countdown and actual match launch.

Runs hidden and muted against a caller-supplied local service, with isolated
profiles. Does not send chat messages, install fighters or change the real roster.
"""
import argparse
import os
from pathlib import Path
import re
import time

from review_native_css_room import Client
from review_native_quick_play import choose, wait_quick


def countdown(client, expected=True, timeout=40):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        text = client.inspect('play-queue')
        found = re.search(r'phase=(\d+) countdown=(\d+)', text)
        if found and ((int(found[1]) == 1 and int(found[2]) > 500) if expected else int(found[1]) == 0):
            return int(found[2])
        time.sleep(.05)
    raise RuntimeError(f'{client.folder}: countdown state not reached: {text}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--doubles', action='store_true')
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve()
    output.mkdir(exist_ok=False, parents=True)
    clients = []
    try:
        for index in range(2):
            c = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                       output / str(index), f'Quick Match {index+1}', window_size=(960,540))
            clients.append(c)
            if args.doubles:
                c.command('hold', 1, 'sx=0')
                c.click('nav-1'); c.click('local-players-2')
            c.click('nav-0'); c.click('quick-doubles' if args.doubles else 'quick-singles')
            wait_quick(c, 0)
            choose(c)
            if args.doubles:
                choose(c, 1)
            c.click('play-ready'); c.element('play-selection')
        for c in clients:
            c.click('play-queue')
        for c in clients:
            wait_quick(c, 2)
        countdown(clients[0])
        clients[0].screenshot()
        clients[0].tap('b')
        clients[0].element('play-ready')
        countdown(clients[1], expected=False)
        for c in clients:
            assert '[netplay] Session ' not in (c.folder/'stdout.log').read_text(errors='replace')
        clients[0].click('play-ready'); clients[0].element('play-selection')
        countdown(clients[0])
        started = time.monotonic()
        sessions = []
        for c in clients:
            deadline = time.monotonic() + 120
            while time.monotonic() < deadline:
                stdout = (c.folder/'stdout.log').read_text(errors='replace')
                stderr = c.stderr.read_text(errors='replace')
                failed = re.search(r'\[netplay\] FAILED: (.*)', stdout)
                if failed:
                    raise RuntimeError(f'{c.folder}: {failed[1]}')
                match = re.search(r'\[netplay\] Session ([0-9a-f]+) connected through room (\w+)', stdout)
                if match:
                    sessions.append(match.groups())
                    break
                if c.process.poll() is not None:
                    raise RuntimeError(f'{c.folder}: game exited {c.process.returncode}; {stderr[-1200:]}')
                time.sleep(.15)
            else:
                raise RuntimeError(f'{c.folder}: actual netplay match did not start; {stdout[-1500:]} {stderr[-1500:]}')
        assert sessions[0] == sessions[1], sessions
        assert time.monotonic() - started >= 2.0, 'Match bypassed its countdown'
        for c in clients:
            c.command('hold', 0, 'sx=30')
        time.sleep(5)
        for c in clients:
            c.command('clear', 0)
            c.screenshot()
            assert c.process.poll() is None
            stdout = (c.folder/'stdout.log').read_text(errors='replace')
            assert '[netplay] FAILED:' not in stdout and 'Checksum mismatch' not in stdout, stdout[-1800:]
        (output/'passed.txt').write_text(
            f'Native {"doubles" if args.doubles else "singles"} practice, B-to-unready, countdown restart '
            f'and actual netplay session {sessions[0][0]} in room {sessions[0][1]} passed.\n')
        print((output/'passed.txt').read_text().strip(), flush=True)
    finally:
        for c in reversed(clients):
            c.stop()


if __name__ == '__main__':
    main()
