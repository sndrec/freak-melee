"""Finish a Quick Play match after losing only one signaling connection.

The test forwards one client's local-service TCP connection, then closes that
connection during gameplay. Native ICE gameplay is untouched. Clients are
hidden, muted and use isolated profiles. No public chat or service is used.
"""
import argparse
import asyncio
import os
from pathlib import Path
import re
import subprocess
import sys
import time
from urllib.parse import urlparse

from review_native_css_room import Client
from review_native_quick_play import choose, wait_quick
from review_native_quick_rematch import live_session


async def proxy(host, port, folder):
    connections = set()

    async def copy(reader, writer):
        while data := await reader.read(65536):
            writer.write(data)
            await writer.drain()

    async def connect(reader, writer):
        remote = None
        connections.add(writer)
        try:
            source, remote = await asyncio.open_connection(host, port)
            connections.add(remote)
            tasks = [asyncio.create_task(copy(reader, remote)), asyncio.create_task(copy(source, writer))]
            try:
                await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
            finally:
                for task in tasks:
                    task.cancel()
                await asyncio.gather(*tasks, return_exceptions=True)
        except (ConnectionError, OSError):
            pass
        finally:
            for stream in [writer, remote]:
                if stream:
                    connections.discard(stream)
                    stream.close()
                    try:
                        await stream.wait_closed()
                    except (ConnectionError, OSError):
                        pass

    server = await asyncio.start_server(connect, '127.0.0.1', 0)
    (folder/'port.txt').write_text(str(server.sockets[0].getsockname()[1]))
    dropped = False
    async with server:
        while True:
            if not dropped and (folder/'disconnect').exists():
                # Both stream endpoints are counted, so one WebSocket is two.
                count = len(connections)
                for stream in list(connections):
                    stream.close()
                (folder/'dropped.txt').write_text(str(count))
                dropped = True
            await asyncio.sleep(.05)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['exe', 'iso', 'profile', 'output']:
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--endpoint', required=True)
    parser.add_argument('--doubles', action='store_true')
    args = parser.parse_args()
    origin = urlparse(args.endpoint)
    assert origin.scheme == 'http' and origin.hostname in ['127.0.0.1', 'localhost']
    output = args.output.resolve(); output.mkdir(parents=True, exist_ok=False)
    proxy_folder = output/'proxy'; proxy_folder.mkdir()
    clients = []
    with (proxy_folder/'log.txt').open('w') as log:
        process = subprocess.Popen([sys.executable, str(Path(__file__).resolve()), '--proxy',
                                    origin.hostname, str(origin.port or 80), str(proxy_folder)],
                                   stdout=log, stderr=log, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        for _ in range(100):
            if (proxy_folder/'port.txt').exists():
                break
            assert process.poll() is None, (proxy_folder/'log.txt').read_text()
            time.sleep(.05)
        proxy_origin = 'http://127.0.0.1:'+(proxy_folder/'port.txt').read_text()
        initial = []
        for index in range(2):
            os.environ['FREAK_MELEE_SERVICE_ENDPOINT'] = proxy_origin if index == 0 else args.endpoint
            c = Client(args.exe.resolve(), args.iso.resolve(), args.profile.resolve(), output/str(index),
                       f'Signal Review {index+1}', window_size=(960,540))
            clients.append(c)
            if args.doubles:
                c.command('hold', 1, 'sx=0'); c.click('nav-1'); c.click('local-players-2')
            c.click('nav-0'); c.click('quick-doubles' if args.doubles else 'quick-singles')
            initial.append(wait_quick(c, 0)); choose(c)
            if args.doubles:
                choose(c, 1)
            c.click('play-ready'); c.element('play-selection')
        for c in clients:
            c.click('play-queue')
        assigned = [wait_quick(c, 2) for c in clients]
        first = [live_session(c) for c in clients]
        assert first[0] == first[1]
        time.sleep(3)
        (proxy_folder/'disconnect').touch()
        for _ in range(100):
            if (proxy_folder/'dropped.txt').exists():
                break
            time.sleep(.05)
        assert int((proxy_folder/'dropped.txt').read_text()) >= 2, 'No live signaling connection was cut'
        time.sleep(8)  # Longer than the gameplay missing-input timeout.
        for c in clients:
            text = (c.folder/'stdout.log').read_text(errors='replace')
            assert c.process.poll() is None
            assert '[netplay] FAILED:' not in text and 'Checksum mismatch' not in text, text[-1800:]
        for port in range(2 if args.doubles else 1):
            clients[0].command('hold', port, 'sx=80')
        deadline = time.monotonic()+90
        while time.monotonic() < deadline:
            reports = [re.search(r'\[quick-play\] result epoch=(\d+) winner=(\d+) queued=(\d+)',
                                 c.stderr.read_text(errors='replace')) for c in clients]
            if all(reports):
                break
            for c in clients:
                assert c.process.poll() is None
            time.sleep(.2)
        else:
            raise RuntimeError('The P2P match did not finish after signaling loss')
        assert reports[0].groups()[:2] == reports[1].groups()[:2]
        assert reports[0][3] == '0' and reports[1][3] == '1'
        assert reports[0][2] == assigned[1]['team']
        for port in range(2 if args.doubles else 1):
            clients[0].command('clear', port)
        for index, c in enumerate(clients):
            deadline = time.monotonic()+45
            while time.monotonic() < deadline:
                text = c.inspect('play-queue')
                if 'state=0' in text and 'phase=0' in text:
                    break
                for port in range(2 if args.doubles else 1):
                    c.tap('start', port=port)
                time.sleep(.3)
            else:
                raise RuntimeError(f'{c.folder}: did not return to the original party: {text}')
            current = wait_quick(c, 0)
            assert current['room'] == initial[index]['room']
            assert current['peer'] == initial[index]['peer'] == assigned[index]['peer']
            c.click('play-ready'); c.element('play-selection')
        for c in clients:
            c.click('play-queue')
        second = [live_session(c, first[0][0]) for c in clients]
        assert second[0] == second[1] and second[0][0] != first[0][0]
        time.sleep(3)
        for c in clients:
            c.screenshot()
            text = (c.folder/'stdout.log').read_text(errors='replace')
            assert '[netplay] FAILED:' not in text and 'Checksum mismatch' not in text, text[-1800:]
        (output/'passed.txt').write_text(
            f'Native {"doubles" if args.doubles else "singles"} P2P completion after signaling loss, '
            f'original-party recovery and second match {first[0][0]} -> {second[0][0]} passed.\n')
        print((output/'passed.txt').read_text().strip(), flush=True)
    finally:
        for c in reversed(clients):
            c.stop()
        if process.poll() is None:
            process.terminate(); process.wait(timeout=10)


if __name__ == '__main__':
    if len(sys.argv) == 5 and sys.argv[1] == '--proxy':
        asyncio.run(proxy(sys.argv[2], int(sys.argv[3]), Path(sys.argv[4])))
    else:
        main()
