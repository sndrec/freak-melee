"""Review late Quick Play cancellation and recovery after an opponent exits.

Uses hidden, muted clients and isolated profiles against a supplied local
service. The opponent process is owned by this test. No public chat is sent.
"""
import argparse
import os
from pathlib import Path
import re
import time

from review_native_css_room import Client
from review_native_quick_play import choose, wait_quick
from review_native_quick_rematch import live_session


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--doubles', action='store_true')
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    clients = []
    try:
        for index in range(2):
            c = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output/str(index),
                       f'Return Review {index+1}', window_size=(960,540))
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
        assigned = [wait_quick(c, 2) for c in clients]
        # Inspect near the deadline and request B on the second local controller
        # in doubles. Either authoritative outcome is valid at this boundary.
        c = clients[0]
        deadline = time.monotonic()+40
        while time.monotonic() < deadline:
            text = c.inspect('play-queue')
            countdown = re.search(r'phase=1 countdown=(\d+)', text)
            if countdown and int(countdown[1]) < 350:
                c.tap('b', port=1 if args.doubles else 0)
                break
            time.sleep(.025)
        else:
            raise RuntimeError(f'Countdown deadline was not observed: {text}')
        deadline = time.monotonic()+50
        outcome = ''
        while time.monotonic() < deadline:
            text = c.inspect('play-queue')
            logs = [(p.folder/'stdout.log').read_text(errors='replace') for p in clients]
            assert not any('[netplay] FAILED:' in log or 'Checksum mismatch' in log for log in logs)
            if 'phase=0' in text:
                c.element('play-ready'); outcome = 'unready acknowledged'; break
            if all('[netplay] Session ' in log for log in logs):
                outcome = 'committed match preserved'; break
            time.sleep(.1)
        else:
            raise RuntimeError(f'Late cancellation left the group stuck: {text}')
        if outcome == 'unready acknowledged':
            c.click('play-ready'); c.element('play-selection')
        sessions = [live_session(p) for p in clients]
        assert sessions[0] == sessions[1]
        time.sleep(3)
        clients[1].stop()
        # A failed gameplay transport must retire the incomplete assigned group
        # and restore the survivor's original party, with its peer ID intact.
        deadline = time.monotonic()+55
        while time.monotonic() < deadline:
            try:
                text = c.inspect('play-queue')
            except RuntimeError as error:
                # The game waits for missing input on its main timeline before
                # declaring the transport lost; UI inspection cannot run there.
                if 'Inspection stalled:' in str(error) and c.process.poll() is None:
                    continue
                raise
            if 'state=0' in text and 'phase=0' in text:
                break
            assert c.process.poll() is None
            time.sleep(.2)
        else:
            raise RuntimeError(f'Opponent loss did not return to the source party: {text}')
        c.element('play-queue'); c.click('play-queue')
        waiting = wait_quick(c, 1)
        c.click('play-queue'); idle = wait_quick(c, 0)
        assert assigned[0]['peer'] == waiting['peer'] == idle['peer']
        c.screenshot()
        log = (c.folder/'stdout.log').read_text(errors='replace')
        assert log.count('[netplay] Session ') == 1, log[-1800:]
        assert 'Checksum mismatch' not in log
        (output/'passed.txt').write_text(
            f'Native {"doubles" if args.doubles else "singles"} late cancellation ({outcome}), '
            f'opponent exit, source-party return and requeue passed.\n')
        print((output/'passed.txt').read_text().strip(), flush=True)
    finally:
        for c in reversed(clients):
            c.stop()


if __name__ == '__main__':
    main()
