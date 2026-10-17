"""Check native Quick Play allocation against a local room service.

Uses hidden, muted clients and isolated copies of a fixture profile. It checks
queueing separately from readiness, identity-preserving handoff, cancellation,
and requeueing. Match countdown and gameplay are separate acceptance work.
"""
import argparse
import os
from pathlib import Path
import re
import time

from review_native_css_room import Client


def wait_quick(client, state, timeout=45):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        text = client.inspect('play-queue')
        found = re.search(r'\[quick-play\] mode=(\d+) state=(\d+) generation=(\d+) '
                          r'team=(\d+) stage=(\d+) room=(\w+) peer=(\w+)', text)
        if found and int(found[2]) == state:
            return dict(zip(['mode', 'state', 'generation', 'team', 'stage', 'room', 'peer'], found.groups()))
        time.sleep(.15)
    raise RuntimeError(f'{client.folder}: Quick Play state {state} not reached: {text}')


def choose(client, port=0):
    client.screenshot()
    state, text = client.state(port)
    x, y = map(float, re.search(r'\[css-tile\] index=0 center=([\d.-]+),([\d.-]+)', text).groups())
    client.move(x-2.7, y+2, port)
    if state['hand'] != 1:
        client.tap('b', port=port)
    client.tap('a', port=port)
    assert client.state(port)[0]['chosen'] == 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--doubles', action='store_true', help='Queue two shared-PC pairs')
    args = parser.parse_args()
    os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = args.endpoint
    output = args.output.resolve()
    output.mkdir(exist_ok=False, parents=True)
    clients = []
    try:
        initial = []
        for index in range(2):
            client = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(),
                            output / str(index), f'Quick Review {index+1}')
            clients.append(client)
            if args.doubles:
                client.command('hold', 1, 'sx=0')
                client.click('nav-1')
                client.click('local-players-2')
            client.click('nav-0')
            client.click('quick-doubles' if args.doubles else 'quick-singles')
            initial.append(wait_quick(client, 0))
            text = client.wait_cards(4 if args.doubles else 2)
            assert 'seat=255 physical=255 canonical=255' in text, text
            choose(client)
            if args.doubles:
                choose(client, 1)
        # Ready before searching: this PC practices while its peer stays on CSS.
        first, second = clients
        first.click('play-ready')
        first.element('play-selection')
        first.click('play-queue')
        wait_quick(first, 1)
        time.sleep(11)  # Exercise a search heartbeat before an opponent arrives.
        wait_quick(first, 1)
        second.click('play-queue')
        assigned = [wait_quick(client, 2) for client in clients]
        assert assigned[0]['room'] == assigned[1]['room']
        assert len(assigned[0]['room']) == 12
        assert assigned[0]['stage'] == assigned[1]['stage']
        assert {a['team'] for a in assigned} == {'0', '1'}
        for before, after in zip(initial, assigned):
            assert before['peer'] == after['peer'], (before, after)
            assert before['room'] != after['room']
        cards = second.wait_cards(4 if args.doubles else 2)
        if args.doubles:
            assert len(re.findall(r'local=1', cards)) == 2, cards
            assert len(re.findall(r'local=0', cards)) == 2, cards
            assert re.search(r'seat=1 physical=1 .*local=1', cards), cards
        for _ in range(120):
            if len(re.findall(r'local=0 ready=1 selected=1', cards)) == (2 if args.doubles else 1):
                break
            time.sleep(.1)
            cards = second.inspect('css-cards')
        else:
            raise RuntimeError(f'Readied fighter did not survive handoff: {cards}')
        assert 'seat=255' not in cards
        first.element('play-selection')
        for client in clients:
            client.screenshot()
        first.click('play-queue')
        returned = wait_quick(first, 0)
        assert returned['room'] == initial[0]['room'] and returned['peer'] == initial[0]['peer']
        returned = wait_quick(second, 0)
        assert returned['room'] == initial[1]['room'] and returned['peer'] == initial[1]['peer']
        for client in clients:
            client.click('play-queue')
        reassigned = [wait_quick(client, 2) for client in clients]
        assert reassigned[0]['room'] == reassigned[1]['room'] != assigned[0]['room']
        first.click('play-queue')
        for client in clients:
            wait_quick(client, 0)
        (output / 'passed.txt').write_text(
            f'Native {"shared-PC doubles" if args.doubles else "singles with ready practice"} queue, heartbeat, handoff, identity/fighter preservation, '
            'assignment cancellation and requeue passed.\n')
        print((output / 'passed.txt').read_text().strip(), flush=True)
    finally:
        for client in reversed(clients):
            client.stop()


if __name__ == '__main__':
    main()
